// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_COMPONENT_INDEX_IMPL_H_
#define APPS_COMPONENT_INDEX_IMPL_H_

#include "apps/component_manager/services/component.fidl.h"
#include "apps/network/services/network_service.fidl.h"
#include "apps/network/services/url_loader.fidl.h"
#include "lib/ftl/macros.h"
#include "third_party/rapidjson/rapidjson/document.h"

namespace component {

class ComponentIndexImpl : public component::ComponentIndex {
 public:
  ComponentIndexImpl(network::NetworkServicePtr network_service);

  void GetComponent(
      const ::fidl::String& component_id,
      ::fidl::InterfaceRequest<ComponentResources> component_resources,
      const GetComponentCallback& callback) override;

  void FindComponentManifests(
      fidl::Map<fidl::String, fidl::String> filter,
      const FindComponentManifestsCallback& callback) override;

 private:
  network::NetworkServicePtr network_service_;

  // A list of component URIs that are installed locally.
  std::vector<std::string> local_index_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ComponentIndexImpl);
};

}  // namespace component

#endif  // APPS_COMPONENT_INDEX_IMPL_H_
