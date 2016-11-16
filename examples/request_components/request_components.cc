// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "apps/component_manager/services/component.fidl.h"
#include "apps/modular/lib/app/application_context.h"
#include "apps/modular/lib/app/connect.h"
#include "lib/fidl/cpp/bindings/interface_request.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/macros.h"
#include "lib/mtl/tasks/message_loop.h"

namespace {

class RequestComponentApp {
 public:
  RequestComponentApp()
      : context_(modular::ApplicationContext::CreateFromStartupInfo()) {
    component_index_ =
        context_->ConnectToEnvironmentService<component::ComponentIndex>();
    component_index_->GetComponent(
        "fuchsia:hello_component", nullptr /* component_resources */,
        [this](component::ComponentManifestPtr manifest,
               network::NetworkErrorPtr error) {
          FTL_LOG(INFO) << "GetComponent returned.";
        });
  }

 private:
  std::unique_ptr<modular::ApplicationContext> context_;
  fidl::InterfacePtr<component::ComponentIndex> component_index_;

  FTL_DISALLOW_COPY_AND_ASSIGN(RequestComponentApp);
};

}  // namespace

int main(int argc, const char** argv) {
  mtl::MessageLoop loop;
  RequestComponentApp app;
  loop.Run();
  return 0;
}
