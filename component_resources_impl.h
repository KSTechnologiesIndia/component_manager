// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_COMPONENT_RESOURCES_IMPL_H_
#define APPS_COMPONENT_RESOURCES_IMPL_H_

#include "apps/component_manager/services/component.fidl.h"
#include "lib/fidl/cpp/bindings/binding.h"

namespace component {

class ComponentResourcesImpl : public ComponentResources {
 public:
  ComponentResourcesImpl(fidl::InterfaceRequest<ComponentResources> request,
                         fidl::Map<fidl::String, fidl::String> resource_urls)
      : binding_(this, std::move(request)),
        resource_urls_(std::move(resource_urls)) {}
  void GetResourceNames(const GetResourceNamesCallback& callback) override;
  void GetResourceURLs(const GetResourceURLsCallback& callback) override;
  void GetResource(const fidl::String& resource_name,
                   const GetResourceCallback& callback) override;

 private:
  fidl::Binding<ComponentResources> binding_;
  fidl::Map<fidl::String, fidl::String> resource_urls_;
};

}  // namespace component

#endif  // APPS_COMPONENT_RESOURCES_IMPL_H_
