// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/component_manager/services/component.fidl.h"
#include "third_party/rapidjson/rapidjson/document.h"
#include "third_party/rapidjson/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/rapidjson/writer.h"

namespace component {

bool JsonToFacetInfo(const rapidjson::Value& value, FacetInfoPtr* facet_info);
bool FacetInfoToJson(const FacetInfoPtr& facet_info, rapidjson::Document* doc);

std::string FacetInfoToString(const FacetInfoPtr& facet_info);

}  // namespace
