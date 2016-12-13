// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_COMPONENT_MANAGER_CACHE_H_
#define APPS_COMPONENT_MANAGER_CACHE_H_

#include <string>

namespace component {

namespace cache {
  bool Load(const std::string& url, std::string* contents);
  std::string PathForUrl(const std::string& url);
}  // namespace cache

}  // namespace component

#endif  // APPS_COMPONENT_MANAGER_CACHE_H_
