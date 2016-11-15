// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/component_manager/component_resources_impl.h"

#include "apps/component_manager/fake_network.h"

namespace component {

void ComponentResourcesImpl::GetResourceNames(
    const GetResourceNamesCallback& callback) {
  fidl::Array<fidl::String> resource_names;
  for (auto i = resource_urls_.cbegin(); i != resource_urls_.cend(); ++i) {
    resource_names.push_back(i.GetKey());
  }
  callback(std::move(resource_names));
}

void ComponentResourcesImpl::GetResourceURLs(
    const GetResourceURLsCallback& callback) {
  callback(resource_urls_.Clone());
}

void ComponentResourcesImpl::GetResource(const fidl::String& resource_name,
                                         const GetResourceCallback& callback) {
  FTL_LOG(FATAL) << "NOT IMPLEMENTED";
}

void ComponentResourcesImpl::GetResourcePath(
    const fidl::String& resource_name,
    const GetResourcePathCallback& callback) {
  FTL_LOG(INFO) << "GetResourcePath " << resource_name;
  auto i = resource_urls_.find(resource_name);
  if (i == resource_urls_.end()) {
    FTL_LOG(WARNING) << "No resource found for: " << resource_name;
    callback("");
  } else {
    const fidl::String& url = i.GetValue();
    callback(FakeNetwork::PathForUrl(url));
  }
}

}  // namespace component
