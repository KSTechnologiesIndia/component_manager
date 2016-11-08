// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/component_manager/component_manager_impl.h"

#include <string>
#include <iostream>

#include "apps/component_manager/fake_network.h"
#include "apps/component_manager/services/component.fidl.h"
#include "apps/component_manager/services/format.h"
#include "lib/ftl/files/file.h"
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
// This path must be in sync with //packages/gn/component_manager.
constexpr char kLocalIndexPath[] = "/system/components/index.json";

namespace {

network::NetworkErrorPtr MakeNetworkError(int code, const std::string& description) {
  auto error = network::NetworkError::New();
  error->code = code;
  error->description = description;
  return error;
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

bool FacetDataMatches(const FacetDataPtr& facet_data,
                      const FacetDataPtr& filter_data) {
  if (!filter_data) {
    // This was just an existence filter, so return true.
    return true;
  }
  if (facet_data->which() != filter_data->which())
    return false;

  if (facet_data->is_object()) {
    // Go through each key in 'filter_data' and recursively check for the same
    // equal property in 'facet_data'. If any values in 'filter_data' don't
    // match, return false. In short ensure that 'filter_data' is a subset
    // of 'facet_data'.
    for (auto it = filter_data->get_object().cbegin();
         it != filter_data->get_object().cend(); ++it) {
      auto data_it = facet_data->get_object().find(it.GetKey());
      if (data_it == facet_data->get_object().end())
        return false;
      return FacetDataMatches(data_it.GetValue(), it.GetValue());
    }
  } else if (facet_data->is_array()) {
    // Every array element in 'filter_data' should match an element
    // in facet_data.
    FTL_LOG(FATAL) << "Filtering by array not implemented.";
    return false;
  } else if (facet_data->is_string()) {
    return facet_data->get_string() == filter_data->get_string();
  }

  // Unreachable.
  return true;
}

bool ManifestMatches(const ComponentManifestPtr& manifest,
                     const fidl::Map<fidl::String, FacetDataPtr>& filter) {
  for (auto it = filter.cbegin(); it != filter.cend(); ++it) {
    // See if the manifest under consideration has this facet.
    const auto& facet_type = it.GetKey();
    if (manifest->facets.find(facet_type) == manifest->facets.end()) {
      return false;
    }

    // See if the filter's FacetData matches that in the facet.
    const auto& filter_data = it.GetValue();
    const auto& facet_data = manifest->facets[facet_type];

    if (!FacetDataMatches(facet_data, filter_data)) {
      // Nope.
      return false;
    }
  }

  return true;
}

}  // namespace

ComponentManagerImpl::ComponentManagerImpl() {
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

void ComponentManagerImpl::FindComponentManifests(
    fidl::Map<fidl::String, FacetDataPtr> facet_values,
    const FindComponentManifestsCallback& callback) {
  std::vector<ComponentManifestPtr>* results =
      new std::vector<ComponentManifestPtr>();
  // Self-deleting.
  BarrierCallback* barrier =
      new BarrierCallback(local_index_.size(), [results, callback] {
        fidl::Array<ComponentManifestPtr> fidl_results;
        fidl_results.Swap(results);
        delete results;
        callback(std::move(fidl_results));
      });

  for (const std::string& uri : local_index_) {
    auto facet_values_copy = facet_values.Clone();
    GetComponentManifest(
        fidl::String(uri), [ results, barrier, &filter = facet_values_copy ](
                               ComponentManifestPtr manifest,
                               network::NetworkErrorPtr network_error) mutable {
          // Check if the manifest matches.
          if (!network_error && ManifestMatches(manifest, filter)) {
            results->push_back(std::move(manifest));
          }
          barrier->Decrement();
        });
  }
}

}  // namespace component
