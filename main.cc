// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/component_manager/component_manager_impl.h"
#include "apps/component_manager/services/component.fidl.h"
#include "apps/modular/lib/app/application_context.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/macros.h"
#include "lib/mtl/tasks/message_loop.h"
#include "lib/fidl/cpp/bindings/binding_set.h"

namespace component {

class App {
 public:
  App() : context_(modular::ApplicationContext::CreateFromStartupInfo()) {
    context_->outgoing_services()->AddService<ComponentManager>(
        [this](fidl::InterfaceRequest<ComponentManager> request) {
          bindings_.AddBinding(&impl_, std::move(request));
        });
  }

 private:
  ComponentManagerImpl impl_;
  fidl::BindingSet<ComponentManager> bindings_;
  std::unique_ptr<modular::ApplicationContext> context_;

  FTL_DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace component

int main(int argc, const char** argv) {
  FTL_LOG(INFO) << "component_manager MojoMain";
  mtl::MessageLoop loop;
  component::App app;
  loop.Run();
  return 0;
}
