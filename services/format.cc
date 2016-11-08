// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/component_manager/services/format.h"

namespace component {

bool JsonToFacetData(const rapidjson::Value& value, FacetDataPtr* facet_data) {
  FTL_DCHECK(facet_data != nullptr);

  if (value.IsString()) {
    (*facet_data)->set_string(value.GetString());
    return true;
  }

  if (value.IsArray()) {
    auto array = fidl::Array<FacetDataPtr>::New(value.Size());
    for (size_t i = 0; i < value.Size(); i++) {
      if (!JsonToFacetData(value[i], &array[i])) {
        return false;
      }
    }
    (*facet_data)->set_array(std::move(array));
    return true;
  }

  if (value.IsObject()) {
    fidl::Map<fidl::String, FacetDataPtr> map;
    for (auto i = value.MemberBegin(); i != value.MemberEnd(); ++i) {
      auto member = FacetData::New();
      if (JsonToFacetData(i->value, &member)) {
        map.insert(i->name.GetString(), std::move(member));
      }
    }
    (*facet_data)->set_object(std::move(map));
    return true;
  }

  // Failed to convert rapidjson::Value.

  return false;
}

namespace {
bool FacetDataToJsonValue(const FacetDataPtr& facet_data,
                          rapidjson::Document::AllocatorType& allocator,
                          rapidjson::Value* value) {
  FTL_DCHECK(value != nullptr);

  if (facet_data->is_string()) {
    value->SetString(facet_data->get_string().get().c_str(), allocator);
    return true;
  }

  if (facet_data->is_array()) {
    value->SetArray();
    for (size_t i = 0; i < facet_data->get_array().size(); ++i) {
      rapidjson::Value v;
      if (!FacetDataToJsonValue(facet_data->get_array()[i], allocator, &v)) {
        return false;
      }
      value->PushBack(v, allocator);
    }
    return true;
  }

  if (facet_data->is_object()) {
    value->SetObject();
    const auto& facet_object_data = facet_data->get_object();
    for (auto it = facet_object_data.cbegin(); it != facet_object_data.cend();
         ++it) {
      rapidjson::Value v;
      if (!FacetDataToJsonValue(it.GetValue(), allocator, &v)) {
        return false;
      }
      rapidjson::Value key;
      key.SetString(it.GetKey().get().c_str(), allocator);
      value->AddMember(key, v, allocator);
    }
    return true;
  }

  // Failed to convert 'facet_data'.

  return false;
}

}  // namespace

bool FacetDataToJson(const FacetDataPtr& facet_data, rapidjson::Document* doc) {
  return FacetDataToJsonValue(facet_data, doc->GetAllocator(), doc);
}

std::string FacetDataToString(const FacetDataPtr& facet_data) {
  rapidjson::Document doc;
  FTL_CHECK(FacetDataToJson(facet_data, &doc));

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

}  // namespace
