// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_COMPONENT_RESOURCES_IMPL_H_
#define APPS_COMPONENT_RESOURCES_IMPL_H_

#include "apps/component_manager/resource_loader.h"
#include "apps/component_manager/services/component.fidl.h"
#include "apps/network/services/network_service.fidl.h"
#include "lib/fidl/cpp/bindings/binding.h"

namespace component {

class ComponentResourcesImpl : public ComponentResources {
 public:
  ComponentResourcesImpl(fidl::Map<fidl::String, fidl::String> resource_urls,
                         std::shared_ptr<ResourceLoader> resource_loader)
      : resource_urls_(std::move(resource_urls)),
        resource_loader_(resource_loader) {}
  void GetResourceNames(const GetResourceNamesCallback& callback) override;
  void GetResourceURLs(const GetResourceURLsCallback& callback) override;
  void GetResource(const fidl::String& resource_name,
                   const GetResourceCallback& callback) override;

 private:
  fidl::Map<fidl::String, fidl::String> resource_urls_;
  std::shared_ptr<ResourceLoader> resource_loader_;
};

}  // namespace component

#endif  // APPS_COMPONENT_RESOURCES_IMPL_H_
