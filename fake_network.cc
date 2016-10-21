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
#include "lib/mtl/shared_buffer/strings.h"
#include "lib/mtl/threading/create_thread.h"

namespace component_manager {

namespace {

constexpr char kBasePath[] = "/boot/fake_network/";

std::string PathForUrl(const std::string& url) {
  std::regex re("[:/]+");
  return files::SimplifyPath(kBasePath + std::regex_replace(url, re, "/"));
}

}  // namespace

class FakeURLLoader : public mojo::URLLoader {
 public:
  FakeURLLoader(ftl::RefPtr<ftl::TaskRunner> task_runner)
      : task_runner_(task_runner) {}
  void Start(mojo::URLRequestPtr request,
             const StartCallback& callback) override {
    mojo::URLResponsePtr response = mojo::URLResponse::New();
    response->url = request->url;

    if ((request->response_body_mode !=
         mojo::URLRequest::ResponseBodyMode::BUFFER) &&
        (request->response_body_mode !=
         mojo::URLRequest::ResponseBodyMode::BUFFER_OR_STREAM)) {
      // Only know how to return buffered responses.
      response->status_code = 500;
      response->error = mojo::NetworkError::New();
      response->error->description = "Only Buffered Responses Supported.";
      callback.Run(std::move(response));
      return;
    }

    std::string contents;
    mojo::ScopedSharedBufferHandle shared_buffer;
    auto path = PathForUrl(request->url);
    if (!files::ReadFileToString(path, &contents) ||
        !mtl::SharedBufferFromString(contents, &shared_buffer)) {
      response->status_code = 404;
      response->error = mojo::NetworkError::New();
      response->error->code = errno;
      response->error->description = strerror(errno);
    } else {
      response->status_code = 200;
      response->body = mojo::URLBody::New();
      response->body->set_buffer(std::move(shared_buffer));
    }
    callback.Run(std::move(response));
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

std::shared_ptr<mojo::URLLoader> FakeNetwork::MakeURLLoader() {
  return std::shared_ptr<mojo::URLLoader>(new FakeURLLoader(task_runner_));
}

}  // namespace
