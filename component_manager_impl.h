// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_COMPONENT_MANAGER_IMPL_H_
#define APPS_COMPONENT_MANAGER_IMPL_H_

#include "apps/component_manager/interfaces/component.mojom.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "mojo/public/interfaces/application/application_connector.mojom.h"

namespace component_manager {

class ComponentManagerImpl : public mojo::ComponentManager {
 public:
  ComponentManagerImpl() {}
  void Initialize(mojo::ApplicationConnectorPtr application_connector);

  void GetComponentManifest(const mojo::String& component_id,
                            const GetComponentManifestCallback& callback) override;
  void ConnectToComponent(const mojo::String& component_id,
                          mojo::InterfaceRequest<mojo::ServiceProvider> service_provider) override;

 private:
  mojo::ApplicationConnectorPtr application_connector_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ComponentManagerImpl);
};

}  // namespace component_manager

#endif  // APPS_COMPONENT_MANAGER_IMPL_H_
