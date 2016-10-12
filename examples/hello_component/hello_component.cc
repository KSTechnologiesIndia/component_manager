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

class HelloComponentApp : public mojo::ApplicationImplBase {
 public:
  HelloComponentApp() {}
  ~HelloComponentApp() override {}

  void OnInitialize() override { FTL_LOG(INFO) << "HelloComponentApp::OnInitialize()"; }

 private:
  FTL_DISALLOW_COPY_AND_ASSIGN(HelloComponentApp);
};

}  // namespace

MojoResult MojoMain(MojoHandle application_request) {
  HelloComponentApp hello_component_app;
  return mojo::RunApplication(application_request, &hello_component_app);
}
