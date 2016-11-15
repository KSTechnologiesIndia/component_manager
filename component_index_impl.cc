// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/component_manager/component_index_impl.h"

#include <iostream>
#include <string>

#include "apps/component_manager/component_resources_impl.h"
#include "apps/component_manager/fake_network.h"
#include "apps/component_manager/services/component.fidl.h"
#include "lib/ftl/files/file.h"
#include "lib/ftl/functional/make_copyable.h"
#include "lib/ftl/logging.h"
#include "lib/mtl/vmo/strings.h"
#include "lib/url/gurl.h"
#include "third_party/rapidjson/rapidjson/document.h"
#include "third_party/rapidjson/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/rapidjson/writer.h"

namespace component {

// Standard facet names.
constexpr char kComponentFacet[] = "fuchsia:component";
constexpr char kResourcesFacet[] = "fuchsia:resources";
constexpr char kApplicationFacet[] = "fuchsia:program";

// This path must be in sync with //packages/gn/component_manager.
constexpr char kLocalIndexPath[] = "/system/components/index.json";

namespace {

network::NetworkErrorPtr MakeNetworkError(int code,
                                          const std::string& description) {
  auto error = network::NetworkError::New();
  error->code = code;
  error->description = description;
  return error;
}

void CopyJSONFieldToFidl(const rapidjson::Value& object, const char* key,
                         fidl::String* string) {
  if (!object.IsObject()) {
    FTL_LOG(ERROR) << "Not a JSON object";
    return;
  }
  auto iterator = object.FindMember(key);
  if (iterator == object.MemberEnd()) {
    return;
  }
  if (!iterator->value.IsString()) {
    FTL_LOG(ERROR) << "Can't copy non-string to string";
    return;
  }
  *string = iterator->value.GetString();
}

class BarrierCallback {
 public:
  BarrierCallback(int n, std::function<void()> callback)
      : n_(n), callback_(callback) {}

  void Decrement() {
    n_--;
    if (n_ == 0) {
      callback_();
      delete this;
    }
  }

 private:
  int n_;
  std::function<void()> callback_;
};

bool FacetInfoMatches(const rapidjson::Value& facet_data,
                      const rapidjson::Value& filter_data) {
  if (filter_data.IsNull()) {
    // This was just an existence filter, so return true.
    return true;
  }
  if (facet_data.GetType() != filter_data.GetType()) return false;

  if (facet_data.IsObject()) {
    // Go through each key in 'filter_data' and recursively check for the same
    // equal property in 'facet_data'. If any values in 'filter_data' don't
    // match, return false. In short ensure that 'filter_data' is a subset
    // of 'facet_data'.
    for (auto it = filter_data.MemberBegin(); it != filter_data.MemberEnd();
         ++it) {
      if (!facet_data.HasMember(it->name)) {
        return false;
      }
      if (!FacetInfoMatches(facet_data[it->name], it->value)) {
        return false;
      }
    }
    return true;
  } else if (facet_data.IsArray()) {
    // Every array element in 'filter_data' should match an element
    // in facet_data.
    FTL_LOG(FATAL) << "Filtering by array not implemented.";
    return false;
  } else {
    // For primitive values we can use rapidjson's comparison operator.
    return facet_data == filter_data;
  }

  // Unreachable.
  return true;
}

bool ManifestMatches(const ComponentManifestPtr& manifest,
                     const std::map<std::string, rapidjson::Document>& filter) {
  rapidjson::Document manifest_json_doc;
  manifest_json_doc.Parse(manifest->raw.get().c_str());

  for (auto it = filter.cbegin(); it != filter.cend(); ++it) {
    // See if the manifest under consideration has this facet.
    const auto& facet_type = it->first;
    if (!manifest_json_doc.HasMember(facet_type.c_str())) {
      return false;
    }

    // See if the filter's FacetInfo matches that in the facet.
    const auto& filter_data = it->second;
    const auto& facet_data = manifest_json_doc[facet_type.c_str()];

    if (!FacetInfoMatches(facet_data, filter_data)) {
      // Nope.
      return false;
    }
  }

  return true;
}

}  // namespace

ComponentIndexImpl::ComponentIndexImpl() {
  // Initialize the local index.
  std::string contents;
  FTL_CHECK(files::ReadFileToString(kLocalIndexPath, &contents));

  rapidjson::Document doc;
  if (doc.Parse(contents.c_str()).HasParseError()) {
    FTL_LOG(FATAL) << "Failed to parse JSON component index at: "
                   << kLocalIndexPath;
  }

  if (!doc.IsArray()) {
    FTL_LOG(FATAL) << "Malformed component index at: " << kLocalIndexPath;
  }

  for (const rapidjson::Value& uri : doc.GetArray()) {
    local_index_.push_back(uri.GetString());
  }
}

void ComponentIndexImpl::GetComponent(
    const ::fidl::String& component_id,
    ::fidl::InterfaceRequest<ComponentResources> component_resources,
    const GetComponentCallback& callback) {
  FTL_LOG(INFO) << "ComponentIndexImpl::GetComponent(\"" << component_id
                << "\")";

  network::URLRequestPtr request = network::URLRequest::New();
  request->response_body_mode = network::URLRequest::ResponseBodyMode::BUFFER;
  request->url = component_id;

  auto url_loader = fake_network_.MakeURLLoader();

  url_loader->Start(
      std::move(request), ftl::MakeCopyable([
        this, component_id,
        component_resources = std::move(component_resources), callback,
        url_loader
      ](network::URLResponsePtr response) mutable {
        FTL_LOG(INFO) << "URL Loader Start Callback";
        if (response->error) {
          FTL_LOG(ERROR) << "URL response contained error: "
                         << response->error->description;
          callback(nullptr, std::move(response->error));
          return;
        }

        // TODO(ianloic): impose some limits on manifest size to prevent DoS.

        std::string contents;
        FTL_DCHECK(response->body->is_buffer());

        if (!mtl::StringFromVmo(std::move(response->body->get_buffer()),
                                &contents)) {
          FTL_LOG(ERROR) << "Failed to read URL response.";
          callback(nullptr,
                   MakeNetworkError(0, "Failed to read URL response."));
          return;
        }

        rapidjson::Document doc;
        if (doc.Parse(contents.c_str()).HasParseError()) {
          FTL_LOG(ERROR) << "Failed to parse component manifest at: "
                         << component_id;
          callback(nullptr,
                   MakeNetworkError(0, "Failed to parse component manifest."));
          return;
        }

        if (!doc.IsObject()) {
          FTL_LOG(ERROR) << "Component manifest " << component_id
                         << " is not a JSON object";
          callback(nullptr, MakeNetworkError(
                                0, "Component manifest is not a JSON object"));
          return;
        }

        if (!doc.HasMember(kComponentFacet)) {
          FTL_LOG(ERROR) << "Component " << component_id
                         << " doesn't have a component facet";
          callback(nullptr,
                   MakeNetworkError(
                       0, "Component manifest missing component facet"));
          return;
        }

        auto manifest = ComponentManifest::New();
        manifest->raw = contents;
        manifest->component = MakeComponentFacet(doc);

        if (doc.HasMember(kResourcesFacet)) {
          manifest->resources =
              MakeResourcesFacet(doc, manifest->component->url);
          if (component_resources) {
            new ComponentResourcesImpl(
                std::move(component_resources),
                manifest->resources->resource_urls.Clone());
          }
        }

        if (doc.HasMember(kApplicationFacet)) {
          manifest->application = MakeApplicationFacet(doc);
        }

        callback(std::move(manifest), nullptr);
      }));
}

ComponentFacetPtr ComponentIndexImpl::MakeComponentFacet(
    const rapidjson::Document& doc) {
  const auto& json = doc[kComponentFacet];
  auto fidl = ComponentFacet::New();
  CopyJSONFieldToFidl(json, "url", &fidl->url);
  CopyJSONFieldToFidl(json, "name", &fidl->name);
  CopyJSONFieldToFidl(json, "version", &fidl->version);
  CopyJSONFieldToFidl(json, "other_versions", &fidl->other_versions);
  return fidl;
}

ResourcesFacetPtr ComponentIndexImpl::MakeResourcesFacet(
    const rapidjson::Document& doc, const std::string& base_url) {
  url::GURL component_url(base_url);

  const auto& json = doc[kResourcesFacet];
  auto fidl = ResourcesFacet::New();
  for (auto i = json.MemberBegin(); i != json.MemberEnd(); ++i) {
    // TODO(ianloic): support more advanced resources facets.
    const std::string& relative_url = i->value.GetString();
    std::string absolute_url = component_url.Resolve(relative_url).spec();
    fidl->resource_urls.insert(i->name.GetString(), absolute_url);
  }
  return fidl;
}

ApplicationFacetPtr ComponentIndexImpl::MakeApplicationFacet(
    const rapidjson::Document& doc) {
  const auto& json = doc[kApplicationFacet];
  auto fidl = ApplicationFacet::New();
  // TODO(ianloic): support arguments.
  CopyJSONFieldToFidl(json, "resource", &fidl->resource);
  CopyJSONFieldToFidl(json, "runner", &fidl->runner);
  CopyJSONFieldToFidl(json, "name", &fidl->name);
  return fidl;
}

void ComponentIndexImpl::FindComponentManifests(
    fidl::Map<fidl::String, fidl::String> filter_fidl,
    const FindComponentManifestsCallback& callback) {
  // Convert the filter from a FIDL Map of JSON to something we work on
  // internally.
  std::map<std::string, rapidjson::Document>* filter =
      new std::map<std::string, rapidjson::Document>();
  for (auto i = filter_fidl.cbegin(); i != filter_fidl.cend(); ++i) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(i.GetValue().get().c_str());
    if (filter_doc.HasParseError()) {
      FTL_LOG(ERROR) << "Failed to parse JSON for facet " << i.GetKey() << " : "
                     << i.GetValue();
      delete filter;
      callback(nullptr);
      return;
    }
    filter->insert(
        std::make_pair(std::string(i.GetKey()), std::move(filter_doc)));
  }

  std::vector<ComponentManifestPtr>* results =
      new std::vector<ComponentManifestPtr>();
  // Self-deleting.
  BarrierCallback* barrier =
      new BarrierCallback(local_index_.size(), [results, filter, callback] {
        fidl::Array<ComponentManifestPtr> fidl_results;
        fidl_results.Swap(results);
        delete results;
        delete filter;
        callback(std::move(fidl_results));
      });

  for (const std::string& uri : local_index_) {
    GetComponent(fidl::String(uri), nullptr /* component_resources */,
                 [results, barrier, filter](
                     ComponentManifestPtr manifest,
                     network::NetworkErrorPtr network_error) mutable {
                   // Check if the manifest matches.
                   if (!network_error && ManifestMatches(manifest, *filter)) {
                     results->push_back(std::move(manifest));
                   }
                   barrier->Decrement();
                 });
  }
}

}  // namespace component
