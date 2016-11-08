// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_COMPONENT_MANAGER_IMPL_H_
#define APPS_COMPONENT_MANAGER_IMPL_H_

#include "apps/component_manager/fake_network.h"
#include "apps/component_manager/services/component.fidl.h"
#include "apps/network/services/url_loader.fidl.h"
#include "lib/ftl/macros.h"

namespace component {

class ComponentManagerImpl : public component::ComponentManager {
 public:
  ComponentManagerImpl();

  void GetComponentManifest(const fidl::String& component_id,
                            const GetComponentManifestCallback& callback) override;

  void FindComponentManifests(
      fidl::Map<fidl::String, FacetDataPtr> facet_values,
      const FindComponentManifestsCallback& callback) override;

 private:
  FakeNetwork fake_network_;

  // A list of component URIs that are installed locally.
  std::vector<std::string> local_index_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ComponentManagerImpl);
};

}  // namespace component

#endif  // APPS_COMPONENT_MANAGER_IMPL_H_
