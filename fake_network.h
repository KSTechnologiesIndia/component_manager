// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This fakes a network implementation until we have a real one.
// It's synchronous because that's easier.

#ifndef APPS_COMPONENT_MANAGER_FAKE_NETWORK_H_
#define APPS_COMPONENT_MANAGER_FAKE_NETWORK_H_

#include <string>
#include <thread>

#include "apps/network/interfaces/url_loader.mojom.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/memory/ref_ptr.h"
#include "lib/ftl/tasks/task_runner.h"

namespace component_manager {

class FakeNetwork {
 public:
  FakeNetwork();
  std::shared_ptr<mojo::URLLoader> MakeURLLoader();

 private:
  std::thread thread_;
  ftl::RefPtr<ftl::TaskRunner> task_runner_;
};

}  // namespace component_manager

#endif  // APPS_COMPONENT_MANAGER_FAKE_NETWORK_H_
