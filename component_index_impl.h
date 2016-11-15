// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_COMPONENT_INDEX_IMPL_H_
#define APPS_COMPONENT_INDEX_IMPL_H_

#include "apps/component_manager/fake_network.h"
#include "apps/component_manager/services/component.fidl.h"
#include "apps/network/services/url_loader.fidl.h"
#include "lib/ftl/macros.h"
#include "third_party/rapidjson/rapidjson/document.h"

namespace component {

class ComponentIndexImpl : public component::ComponentIndex {
 public:
  ComponentIndexImpl();

  void GetComponent(
      const ::fidl::String& component_id,
      ::fidl::InterfaceRequest<ComponentResources> component_resources,
      const GetComponentCallback& callback) override;

  void FindComponentManifests(
      fidl::Map<fidl::String, fidl::String> filter,
      const FindComponentManifestsCallback& callback) override;

 private:
  ComponentFacetPtr MakeComponentFacet(const rapidjson::Document& json);
  ResourcesFacetPtr MakeResourcesFacet(const rapidjson::Document& json,
                                       const std::string& base_url);
  ApplicationFacetPtr MakeApplicationFacet(const rapidjson::Document& json);

  FakeNetwork fake_network_;

  // A list of component URIs that are installed locally.
  std::vector<std::string> local_index_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ComponentIndexImpl);
};

}  // namespace component

#endif  // APPS_COMPONENT_INDEX_IMPL_H_
