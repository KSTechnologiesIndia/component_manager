// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/component_manager/component_manager_impl.h"

#include <string>
#include <iostream>

#include "apps/component_manager/fake_network.h"
#include "apps/component_manager/services/component.fidl.h"
#include "lib/ftl/functional/make_copyable.h"
#include "lib/ftl/logging.h"
#include "lib/mtl/vmo/strings.h"
#include "third_party/rapidjson/rapidjson/document.h"
#include "third_party/rapidjson/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/rapidjson/writer.h"

namespace component {

/*
constexpr char kProgramFacet[] = "fuchsia:program";
constexpr char kProgramUrlProperty[] = "url";
*/

namespace {

bool JsonToFacetData(const rapidjson::Value& value, FacetDataPtr* facet_data) {
  FTL_DCHECK(facet_data != nullptr);

  if (value.IsString()) {
    (*facet_data)->set_string(value.GetString());
    return true;
  }

  if (value.IsArray()) {
    auto array = fidl::Array<FacetDataPtr>::New(value.Size());
    for (size_t i = 0; i < value.Size(); i++) {
      if (!JsonToFacetData(value[i], &array[i])) {
        return false;
      }
    }
    (*facet_data)->set_array(std::move(array));
    return true;
  }

  if (value.IsObject()) {
    fidl::Map<fidl::String, FacetDataPtr> map;
    for (auto i = value.MemberBegin(); i != value.MemberEnd(); ++i) {
      auto member = FacetData::New();
      if (JsonToFacetData(i->value, &member)) {
        map.insert(i->name.GetString(), std::move(member));
      }
    }
    (*facet_data)->set_object(std::move(map));
    return true;
  }

  // Failed to convert rapidjson::Value.

  return false;
}

/*
std::string ProgramUrlFromManifest(const ComponentManifestPtr* component_manifest) {
  const FacetDataPtr& program_facet = (*component_manifest)->facets[kProgramFacet];
  if (!program_facet) {
    FTL_LOG(ERROR) << "manifest for " << (*component_manifest)->id << " missing " << kProgramFacet;
    return "";
  }
  if (!program_facet->is_object()) {
    FTL_LOG(ERROR) << "manifest for " << (*component_manifest)->id << " " << kProgramFacet
                   << " isn't an object";
    return "";
  }
  const FacetDataPtr& url = program_facet->get_object()[kProgramUrlProperty];
  if (!url->is_string()) {
    FTL_LOG(ERROR) << "manifest for " << (*component_manifest)->id << " doesn't have a string "
                   << kProgramFacet << "/" << kProgramUrlProperty;
    return "";
  }

  return url->get_string();
}
*/

network::NetworkErrorPtr MakeNetworkError(int code, const std::string& description) {
  auto error = network::NetworkError::New();
  error->code = code;
  error->description = description;
  return error;
}

}  // namespace

void ComponentManagerImpl::GetComponentManifest(const fidl::String& component_id,
                                                const GetComponentManifestCallback& callback) {
  FTL_LOG(INFO) << "ComponentManagerImpl::GetComponentManifest(\"" << component_id << "\")";

  network::URLRequestPtr request = network::URLRequest::New();
  request->response_body_mode = network::URLRequest::ResponseBodyMode::BUFFER;
  request->url = component_id;

  auto url_loader = fake_network_.MakeURLLoader();

  url_loader->Start(
      std::move(request), [component_id, callback, url_loader](network::URLResponsePtr response) {
        FTL_LOG(INFO) << "URL Loader Start Callback";
        if (response->error) {
          FTL_LOG(ERROR) << "URL response contained error: " << response->error->description;
          callback(nullptr, std::move(response->error));
          return;
        }

        // TODO(ianloic): impose some limits on manifest size to prevent DoS.

        std::string contents;
        FTL_DCHECK(response->body->is_buffer());

        if (!mtl::StringFromVmo(std::move(response->body->get_buffer()), &contents)) {
          FTL_LOG(ERROR) << "Failed to read URL response.";
          callback(nullptr, MakeNetworkError(0, "Failed to read URL response."));
          return;
        }

        rapidjson::Document doc;
        if (doc.Parse(contents.c_str()).HasParseError()) {
          FTL_LOG(ERROR) << "Failed to parse component manifest at: " << component_id;
          callback(nullptr, MakeNetworkError(0, "Failed to parse component manifest."));
          return;
        }

        if (!doc.IsObject()) {
          FTL_LOG(ERROR) << "Component manifest " << component_id << " is not a JSON object";
          callback(nullptr, MakeNetworkError(0, "Component manifest is not a JSON object"));
          return;
        }

        auto manifest = ComponentManifest::New();
        manifest->id = component_id;

        for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
          auto facet_data = FacetData::New();
          if (JsonToFacetData(i->value, &facet_data)) {
            manifest->facets.insert(i->name.GetString(), std::move(facet_data));
          } else {
            FTL_LOG(ERROR) << "Failed to parse facet " << i->name.GetString();
          }
        }

        callback(std::move(manifest), nullptr);
      });
}

/*
void ComponentManagerImpl::ConnectToComponent(
    const mojo::String& component_id,
    mojo::InterfaceRequest<mojo::ServiceProvider> service_provider) {
  FTL_LOG(INFO) << "ComponentManagerImpl::ConnectToComponent(\"" << component_id << "\")";
  if (!application_connector_) {
    FTL_LOG(ERROR) << "Request for " << component_id << " before component manager is initialized";
    return;
  }

  std::function<void(mojo::ComponentManifestPtr, mojo::NetworkErrorPtr)> callback =
      ftl::MakeCopyable([ service_provider = std::move(service_provider), this ](
          mojo::ComponentManifestPtr component_manifest,
          mojo::NetworkErrorPtr network_error) mutable {
        if (network_error) {
          FTL_LOG(ERROR) << "network error: " << network_error->description;
          return;
        }
        FTL_DCHECK(component_manifest);
        FTL_LOG(INFO) << "callback with " << component_manifest->id;

        auto url = ProgramUrlFromManifest(&component_manifest);
        FTL_LOG(INFO) << " url=" << url;
        application_connector_->ConnectToApplication(url, std::move(service_provider));
      });

  GetComponentManifest(component_id, callback);
}
*/

}  // namespace component
