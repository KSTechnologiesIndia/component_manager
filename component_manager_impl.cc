// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <iostream>

#include "apps/component_manager/component_manager_impl.h"
#include "apps/component_manager/fake_network.h"
#include "lib/ftl/functional/make_copyable.h"
#include "lib/ftl/logging.h"
#include "third_party/rapidjson/rapidjson/document.h"
#include "third_party/rapidjson/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/rapidjson/writer.h"

namespace component_manager {

constexpr char kProgramFacet[] = "fuchsia:program";
constexpr char kProgramUrlProperty[] = "url";

namespace {

bool JsonToFacetData(const rapidjson::Value& value, mojo::FacetDataPtr* facet_data) {
  FTL_DCHECK(facet_data != NULL);

  if (value.IsString()) {
    (*facet_data)->set_string(value.GetString());
    return true;
  } else if (value.IsArray()) {
    auto array = mojo::Array<mojo::FacetDataPtr>::New(value.Size());
    for (size_t i = 0; i < value.Size(); i++) {
      if (!JsonToFacetData(value[i], &array[i])) {
        return false;
      }
    }
    (*facet_data)->set_array(array.Pass());
    return true;
  } else if (value.IsObject()) {
    mojo::Map<mojo::String, mojo::FacetDataPtr> map;
    for (auto i = value.MemberBegin(); i != value.MemberEnd(); ++i) {
      auto member = mojo::FacetData::New();
      if (JsonToFacetData(i->value, &member)) {
        map.insert(i->name.GetString(), std::move(member));
      }
    }
    (*facet_data)->set_object(map.Pass());
    return true;
  }

  // Failed to convert rapidjson::Value.

  return false;
}

std::string ProgramUrlFromManifest(const mojo::ComponentManifestPtr* component_manifest) {
  const mojo::FacetDataPtr& program_facet = (*component_manifest)->facets[kProgramFacet];
  if (!program_facet) {
    FTL_LOG(ERROR) << "manifest for " << (*component_manifest)->id << " missing " << kProgramFacet;
    return "";
  }
  if (!program_facet->is_object()) {
    FTL_LOG(ERROR) << "manifest for " << (*component_manifest)->id << " " << kProgramFacet
                   << " isn't an object";
    return "";
  }
  const mojo::FacetDataPtr& url = program_facet->get_object()[kProgramUrlProperty];
  if (!url->is_string()) {
    FTL_LOG(ERROR) << "manifest for " << (*component_manifest)->id << " doesn't have a string "
                   << kProgramFacet << "/" << kProgramUrlProperty;
    return "";
  }

  return url->get_string();
}

}  // namespace

void ComponentManagerImpl::Initialize(mojo::ApplicationConnectorPtr application_connector) {
  application_connector_ = application_connector.Pass();
}

void ComponentManagerImpl::GetComponentManifest(const mojo::String& component_id,
                                                const GetComponentManifestCallback& callback) {
  FTL_LOG(INFO) << "ComponentManagerImpl::GetComponentManifest(\"" << component_id << "\")";

  std::string contents;
  if (!fake_network::Get(component_id, &contents)) {
    FTL_LOG(ERROR) << "Failed to load component manifest: " << component_id;
    callback.Run(NULL);
    return;
  }

  rapidjson::Document doc;
  if (doc.Parse(contents.c_str()).HasParseError()) {
    FTL_LOG(ERROR) << "Failed to parse component manifest at: " << component_id;
    callback.Run(NULL);
    return;
  }

  if (!doc.IsObject()) {
    FTL_LOG(ERROR) << "Component manifest " << component_id << " is not a JSON object";
    callback.Run(NULL);
    return;
  }

  auto manifest = mojo::ComponentManifest::New();
  manifest->id = component_id;

  for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
    auto facet_data = mojo::FacetData::New();
    if (JsonToFacetData(i->value, &facet_data)) {
      manifest->facets.insert(i->name.GetString(), std::move(facet_data));
    } else {
      FTL_LOG(ERROR) << "Failed to parse facet " << i->name.GetString();
    }
  }

  callback.Run(std::move(manifest));
}

void ComponentManagerImpl::ConnectToComponent(
    const mojo::String& component_id,
    mojo::InterfaceRequest<mojo::ServiceProvider> service_provider) {
  FTL_LOG(INFO) << "ComponentManagerImpl::ConnectToComponent(\"" << component_id << "\")";
  if (!application_connector_) {
    FTL_LOG(ERROR) << "Request for " << component_id << " before component manager is initialized";
    return;
  }

  mojo::ApplicationConnectorPtr application_connector;
  application_connector_->Duplicate(GetProxy(&application_connector));

  std::function<void(mojo::ComponentManifestPtr)> callback = ftl::MakeCopyable([
    application_connector = std::move(application_connector),
    service_provider = std::move(service_provider)
  ](mojo::ComponentManifestPtr component_manifest) mutable {
    FTL_LOG(INFO) << "callback with " << component_manifest->id;

    auto url = ProgramUrlFromManifest(&component_manifest);
    FTL_LOG(INFO) << " url=" << url;
    application_connector->ConnectToApplication(url, std::move(service_provider));
  });

  GetComponentManifest(component_id, callback);
}

}  // namespace component_manager
