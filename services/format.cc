// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/component_manager/services/format.h"

namespace component {

bool JsonToFacetInfo(const rapidjson::Value& value, FacetInfoPtr* facet_info) {
  FTL_DCHECK(facet_info != nullptr);

  if (value.IsString()) {
    (*facet_info)->set_string(value.GetString());
    return true;
  }

  if (value.IsArray()) {
    auto array = fidl::Array<FacetInfoPtr>::New(value.Size());
    for (size_t i = 0; i < value.Size(); i++) {
      if (!JsonToFacetInfo(value[i], &array[i])) {
        return false;
      }
    }
    (*facet_info)->set_array(std::move(array));
    return true;
  }

  if (value.IsObject()) {
    fidl::Map<fidl::String, FacetInfoPtr> map;
    for (auto i = value.MemberBegin(); i != value.MemberEnd(); ++i) {
      auto member = FacetInfo::New();
      if (JsonToFacetInfo(i->value, &member)) {
        map.insert(i->name.GetString(), std::move(member));
      }
    }
    (*facet_info)->set_object(std::move(map));
    return true;
  }

  // Failed to convert rapidjson::Value.

  return false;
}

namespace {
bool FacetInfoToJsonValue(const FacetInfoPtr& facet_info,
                          rapidjson::Document::AllocatorType& allocator,
                          rapidjson::Value* value) {
  FTL_DCHECK(value != nullptr);

  if (facet_info->is_string()) {
    value->SetString(facet_info->get_string().get().c_str(), allocator);
    return true;
  }

  if (facet_info->is_array()) {
    value->SetArray();
    for (size_t i = 0; i < facet_info->get_array().size(); ++i) {
      rapidjson::Value v;
      if (!FacetInfoToJsonValue(facet_info->get_array()[i], allocator, &v)) {
        return false;
      }
      value->PushBack(v, allocator);
    }
    return true;
  }

  if (facet_info->is_object()) {
    value->SetObject();
    const auto& facet_object_data = facet_info->get_object();
    for (auto it = facet_object_data.cbegin(); it != facet_object_data.cend();
         ++it) {
      rapidjson::Value v;
      if (!FacetInfoToJsonValue(it.GetValue(), allocator, &v)) {
        return false;
      }
      rapidjson::Value key;
      key.SetString(it.GetKey().get().c_str(), allocator);
      value->AddMember(key, v, allocator);
    }
    return true;
  }

  // Failed to convert 'facet_info'.

  return false;
}

}  // namespace

bool FacetInfoToJson(const FacetInfoPtr& facet_info, rapidjson::Document* doc) {
  return FacetInfoToJsonValue(facet_info, doc->GetAllocator(), doc);
}

std::string FacetInfoToString(const FacetInfoPtr& facet_info) {
  rapidjson::Document doc;
  FTL_CHECK(FacetInfoToJson(facet_info, &doc));

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

}  // namespace
