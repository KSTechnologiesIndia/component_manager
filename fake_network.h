// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This fakes a network implementation until we have a real one.
// It's synchronous because that's easier.

#ifndef APPS_COMPONENT_MANAGER_FAKE_NETWORK_H_
#define APPS_COMPONENT_MANAGER_FAKE_NETWORK_H_

#include <iostream>
#include <regex>
#include <string>

#include "lib/ftl/files/file.h"
#include "lib/ftl/files/path.h"
#include "lib/ftl/logging.h"

namespace component_manager {
namespace fake_network {

bool Get(const std::string& url, std::string* content) {
  FTL_DCHECK(content != NULL);

  const std::string base_path = "/boot/components/";
  std::regex re("[:/]+");
  std::string path = files::SimplifyPath(base_path + std::regex_replace(url, re, "/"));
  // TODO(ianloic): check that path is inside base_path.
  if (!files::ReadFileToString(path, content)) {
    FTL_LOG(ERROR) << "Warning: Couldn't read " << path << " for " << url;
    return false;
  }
  return true;
}

}  // namespace fake_network
}  // namespace component_manager

#endif  // APPS_COMPONENT_MANAGER_FAKE_NETWORK_H_
