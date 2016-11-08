// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/component_manager/services/component.fidl.h"
#include "third_party/rapidjson/rapidjson/document.h"
#include "third_party/rapidjson/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/rapidjson/writer.h"

namespace component {

bool JsonToFacetData(const rapidjson::Value& value, FacetDataPtr* facet_data);
bool FacetDataToJson(const FacetDataPtr& facet_data, rapidjson::Document* doc);

std::string FacetDataToString(const FacetDataPtr& facet_data);

}  // namespace
