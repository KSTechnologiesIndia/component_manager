// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/system/main.h>
#include <stdio.h>

#include <string>

#include "apps/component_manager/interfaces/component.mojom.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/macros.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"

using mojo::ComponentManagerPtr;

namespace {

class RequestComponentApp : public mojo::ApplicationImplBase {
 public:
  RequestComponentApp() {}
  ~RequestComponentApp() override {}

  void OnInitialize() override {
    mojo::ConnectToService(shell(), "mojo:component_manager", GetProxy(&component_manager_));

    mojo::ServiceProviderPtr service_provider;
    component_manager_->ConnectToComponent("fuchsia:hello_component", GetProxy(&service_provider));
  }

 private:
  ComponentManagerPtr component_manager_;

  FTL_DISALLOW_COPY_AND_ASSIGN(RequestComponentApp);
};

}  // namespace

MojoResult MojoMain(MojoHandle application_request) {
  RequestComponentApp Request_component_app;
  return mojo::RunApplication(application_request, &Request_component_app);
}
