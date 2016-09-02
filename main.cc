// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/system/main.h>

#include "lib/ftl/logging.h"
#include "apps/component_manager/component_manager_impl.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "apps/component_manager/interfaces/component.mojom.h"

namespace component_manager {

class ComponentManagerApp : public mojo::ApplicationImplBase {
 public:
  ComponentManagerApp() {}

  void OnInitialize() override {
    FTL_LOG(INFO) << "ComponentManagerApp.OnInitialize()";

    mojo::ApplicationConnectorPtr application_connector;
    shell()->CreateApplicationConnector(mojo::GetProxy(&application_connector));

    impl_.Initialize(application_connector.Pass());
  }

  bool OnAcceptConnection(mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<mojo::ComponentManager>(
        [this](const mojo::ConnectionContext& connection_context,
               mojo::InterfaceRequest<mojo::ComponentManager> request) {
          bindings_.AddBinding(&impl_, request.Pass());
        });
    return true;
  }

 private:
  ComponentManagerImpl impl_;
  mojo::BindingSet<mojo::ComponentManager> bindings_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ComponentManagerApp);
};

}  // namespace component_manager

MojoResult MojoMain(MojoHandle application_request) {
  FTL_LOG(INFO) << "component_manager MojoMain";
  component_manager::ComponentManagerApp app;
  return mojo::RunApplication(application_request, &app);
}
