// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This fakes a network implementation until we have a real one.
// It's synchronous because that's easier.

#include "fake_network.h"

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <regex>

#include "lib/ftl/files/file.h"
#include "lib/ftl/files/path.h"
#include "lib/ftl/logging.h"
#include "lib/mtl/vmo/strings.h"
#include "lib/mtl/threading/create_thread.h"
#include "mx/vmo.h"

namespace component {

namespace {

constexpr char kBasePath[] = "/system/fake_network/";

std::string PathForUrl(const std::string& url) {
  std::regex re("[:/]+");
  return files::SimplifyPath(kBasePath + std::regex_replace(url, re, "/"));
}

}  // namespace

class FakeURLLoader : public network::URLLoader {
 public:
  FakeURLLoader(ftl::RefPtr<ftl::TaskRunner> task_runner)
      : task_runner_(task_runner) {}
  void Start(network::URLRequestPtr request,
             const StartCallback& callback) override {
    network::URLResponsePtr response = network::URLResponse::New();
    response->url = request->url;

    if ((request->response_body_mode !=
         network::URLRequest::ResponseBodyMode::BUFFER) &&
        (request->response_body_mode !=
         network::URLRequest::ResponseBodyMode::BUFFER_OR_STREAM)) {
      // Only know how to return buffered responses.
      response->status_code = 500;
      response->error = network::NetworkError::New();
      response->error->description = "Only Buffered Responses Supported.";
      callback(std::move(response));
      return;
    }

    std::string contents;
    mx::vmo shared_buffer;
    auto path = PathForUrl(request->url);
    if (!files::ReadFileToString(path, &contents) ||
        !mtl::VmoFromString(contents, &shared_buffer)) {
      response->status_code = 404;
      response->error = network::NetworkError::New();
      response->error->code = errno;
      response->error->description = strerror(errno);
    } else {
      response->status_code = 200;
      response->body = network::URLBody::New();
      response->body->set_buffer(std::move(shared_buffer));
    }
    callback(std::move(response));
  }
  void FollowRedirect(const FollowRedirectCallback& callback) override {
    FTL_NOTIMPLEMENTED();
  }
  void QueryStatus(const QueryStatusCallback& callback) override {
    FTL_NOTIMPLEMENTED();
  }

 private:
  ftl::RefPtr<ftl::TaskRunner> task_runner_;
};

FakeNetwork::FakeNetwork() {
  thread_ = mtl::CreateThread(&task_runner_);
}

std::shared_ptr<network::URLLoader> FakeNetwork::MakeURLLoader() {
  return std::shared_ptr<network::URLLoader>(new FakeURLLoader(task_runner_));
}

}  // namespace component
