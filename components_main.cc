// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "apps/component_manager/services/component.fidl.h"
#include "apps/component_manager/services/format.h"
#include "apps/modular/lib/app/connect.h"
#include "apps/modular/lib/app/application_context.h"
#include "apps/modular/services/application/service_provider.fidl.h"
#include "lib/fidl/cpp/bindings/interface_ptr.h"
#include "lib/fidl/cpp/bindings/interface_request.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/macros.h"
#include "lib/mtl/tasks/message_loop.h"
#include "lib/fidl/cpp/bindings/binding_set.h"

namespace component {

class App {
 public:
  App() : context_(modular::ApplicationContext::CreateFromStartupInfo()) {
    auto launch_info = modular::ApplicationLaunchInfo::New();
    launch_info->url = "file:///system/apps/component_manager";
    modular::ServiceProviderPtr services;
    launch_info->services = fidl::GetProxy(&services);
    FTL_CHECK(context_->launcher());
    context_->launcher()->CreateApplication(
        std::move(launch_info), GetProxy(&component_manager_controller_));

    modular::ConnectToService(services.get(),
                              fidl::GetProxy(&component_manager_));
  }

  void Query(int argc, const char** argv) {
    if (argc == 0) {
      std::cerr << "query requires a filter expression\n";
      exit(1);
    }

    fidl::Map<fidl::String, FacetDataPtr> query;
    for (; argc > 0; --argc, ++argv) {
      std::string facet_type(argv[0]);
      FacetDataPtr data;
      if (argc > 1) {
        rapidjson::Document doc;
        if (doc.Parse(argv[1]).HasParseError()) {
          FTL_LOG(FATAL) << "Failed to parse JSON facet data: " << argv[1];
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        data = FacetData::New();
        JsonToFacetData(doc, &data);
        --argc;
        ++argv;
      }

      query[fidl::String(facet_type)] = std::move(data);
    }

    component_manager_->FindComponentManifests(
        std::move(query), [this](fidl::Array<ComponentManifestPtr> results) {
          FTL_LOG(INFO) << "Got " << results.size() << " results...";
          for (auto it = results.begin(); it != results.end(); ++it) {
            std::cout << "=== " << (*it)->id << "\n";
            for (auto f_it = (*it)->facets.begin(); f_it != (*it)->facets.end();
                 ++f_it) {
              std::cout << " - " << f_it.GetKey() << ": "
                        << FacetDataToString(f_it.GetValue()) << "\n";
            }
          }

          exit(0);
        });
  }

 private:
  std::function<void()> quit_callback_;
  std::unique_ptr<modular::ApplicationContext> context_;
  modular::ApplicationControllerPtr component_manager_controller_;

  fidl::InterfacePtr<ComponentManager> component_manager_;

  FTL_DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace component

void Usage(const char* argv0) {
  std::cerr << "Usage: " << argv0 << " <command>\n\n"
     << "commands:\n"
     << "  query [facet1 [info1], ...]]\n"
     << "    Queries for existence of all 'facetN' and optionally matches\n"
     << "    'infoN' against that facet's info. 'infoN' will match if it\n"
     << "    is a subset of 'facetN's info. 'infoN' should be provided\n"
     << "    as JSON.\n";

  exit(1);
}

int main(int argc, const char** argv) {
  if (argc < 2) {
    Usage(argv[0]);
  }

  mtl::MessageLoop loop;
  component::App app;

  if (strcmp(argv[1], "query") == 0) {
    app.Query(argc - 2, argv + 2);
  } else {
    Usage(argv[0]);
  }

  loop.Run();
  return 0;
}
