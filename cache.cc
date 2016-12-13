// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This fakes a network implementation until we have a real one.
// It's synchronous because that's easier.

#include "cache.h"

#include <regex>

#include "lib/ftl/files/file.h"
#include "lib/ftl/files/path.h"

namespace component {

namespace {

constexpr char kBasePath[] = "/system/components/";

}  // namespace

namespace cache {

std::string PathForUrl(const std::string& url) {
  std::regex re("[:/]+");
  return files::SimplifyPath(kBasePath + std::regex_replace(url, re, "/"));
}

bool Load(const std::string& url, std::string* contents) {
  return files::ReadFileToString(PathForUrl(url), contents);
}

}

}  // namespace component
