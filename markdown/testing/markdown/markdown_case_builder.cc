// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "testing/markdown/markdown_case_builder.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

namespace serval::markdown::testing {
namespace fs = std::filesystem;

MarkdownCaseValuePtr MarkdownCaseBuilder::ConvertJson(
    const rapidjson::Value& value) {
  switch (value.GetType()) {
    case rapidjson::kFalseType:
      return Value::MakeBool(false);
    case rapidjson::kTrueType:
      return Value::MakeBool(true);
    case rapidjson::kObjectType: {
      ValueMap map;
      for (auto iter = value.MemberBegin(); iter != value.MemberEnd(); ++iter) {
        map[std::string(iter->name.GetString(), iter->name.GetStringLength())] =
            ConvertJson(iter->value);
      }
      return Value::MakeMap(std::move(map));
    }
    case rapidjson::kArrayType: {
      ValueArray array;
      for (uint32_t i = 0; i < value.Size(); i++) {
        array.emplace_back(ConvertJson(value[i]));
      }
      return Value::MakeArray(std::move(array));
    }
    case rapidjson::kStringType:
      return Value::MakeString(
          std::string(value.GetString(), value.GetStringLength()));
    case rapidjson::kNumberType:
      return Value::MakeDouble(value.GetDouble());
    default:
      return Value::MakeNull();
  }
}

std::string MarkdownCaseBuilder::ReadFileToString(const fs::path& path) {
  std::ifstream input(path, std::ios::in | std::ios::binary);
  if (!input) {
    return {};
  }
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

MarkdownCaseValuePtr MarkdownCaseBuilder::ReadJsonFileToValue(
    const fs::path& path) {
  const auto json = ReadFileToString(path);
  if (json.empty()) {
    return nullptr;
  }
  rapidjson::Document doc;
  doc.Parse(json.c_str());
  if (doc.HasParseError()) {
    return nullptr;
  }
  return ConvertJson(doc);
}

MarkdownCaseValuePtr MarkdownCaseBuilder::CloneValue(Value* value) {
  if (value == nullptr) {
    return Value::MakeNull();
  }
  switch (value->GetType()) {
    case ValueType::kMap: {
      ValueMap map;
      for (auto& [key, child] : value->AsMap()) {
        map[key] = CloneValue(child.get());
      }
      return Value::MakeMap(std::move(map));
    }
    case ValueType::kArray: {
      ValueArray array;
      for (auto& child : value->AsArray()) {
        array.emplace_back(CloneValue(child.get()));
      }
      return Value::MakeArray(std::move(array));
    }
    case ValueType::kBool:
      return Value::MakeBool(value->AsBool());
    case ValueType::kInt:
      return Value::MakeInt(value->AsInt());
    case ValueType::kLong:
      return Value::MakeLong(value->AsLong());
    case ValueType::kDouble:
      return Value::MakeDouble(value->AsDouble());
    case ValueType::kString:
      return Value::MakeString(value->GetString());
    case ValueType::kNull:
      return Value::MakeNull();
  }
  return Value::MakeNull();
}

void MarkdownCaseBuilder::MergeMap(ValueMap& dst, ValueMap& src) {
  for (auto& [key, value] : src) {
    if (value == nullptr) {
      dst[key] = Value::MakeNull();
      continue;
    }
    auto dst_iter = dst.find(key);
    if (value->GetType() == ValueType::kMap && dst_iter != dst.end() &&
        dst_iter->second != nullptr &&
        dst_iter->second->GetType() == ValueType::kMap) {
      MergeMap(dst_iter->second->AsMap(), value->AsMap());
    } else {
      dst[key] = CloneValue(value.get());
    }
  }
}

bool MarkdownCaseBuilder::ParseRectValue(Value* value, RectF* rect) {
  if (value == nullptr || rect == nullptr ||
      value->GetType() != ValueType::kArray) {
    return false;
  }
  const auto& array = value->AsArray();
  if (array.size() < 4) {
    return false;
  }
  *rect = RectF::MakeLTRB(static_cast<float>(array[0]->GetDouble()),
                          static_cast<float>(array[1]->GetDouble()),
                          static_cast<float>(array[2]->GetDouble()),
                          static_cast<float>(array[3]->GetDouble()));
  return true;
}

void MarkdownCaseBuilder::ApplyAttributes(Value* attributes) {
  if (attributes == nullptr || attributes->GetType() != ValueType::kMap) {
    return;
  }
  auto& map = attributes->AsMap();
  if (const auto iter = map.find("width"); iter != map.end()) {
    config_.width_ = iter->second->AsDouble();
  }
  if (const auto iter = map.find("height"); iter != map.end()) {
    config_.height_ = iter->second->AsDouble();
  }
  if (const auto iter = map.find("animation-type"); iter != map.end()) {
    const auto type = iter->second->GetString();
    if (type == "typewriter") {
      config_.animation_type_ = MarkdownAnimationType::kTypewriter;
    } else if (type == "line-expand") {
      config_.animation_type_ = MarkdownAnimationType::kLineExpand;
    } else {
      config_.animation_type_ = MarkdownAnimationType::kNone;
    }
  }
  if (const auto iter = map.find("use-char-based-drawer"); iter != map.end()) {
    config_.use_char_based_drawer_ = iter->second->AsBool();
  }
  if (const auto iter = map.find("generate"); iter != map.end()) {
    config_.generate_ground_truth_ = iter->second->AsBool();
  }
  if (const auto iter = map.find("initial-animation-step");
      iter != map.end()) {
    config_.animation_step_ = iter->second->GetInt();
  }
  if (const auto iter = map.find("text-maxlines"); iter != map.end()) {
    config_.max_lines_ = iter->second->GetInt();
  }
  if (const auto iter = map.find("content-complete"); iter != map.end()) {
    config_.content_complete_ = iter->second->AsBool();
    config_.draw_cursor_if_complete_ = !config_.content_complete_;
  }
  if (const auto iter = map.find("style");
      iter != map.end() && iter->second->GetType() == ValueType::kMap) {
    MergeMap(config_.style_map_, iter->second->AsMap());
  }
  if (const auto iter = map.find("text-mark-attachments"); iter != map.end()) {
    config_.attachments_ = CloneValue(iter->second.get());
  }
  if (const auto iter = map.find("animation-velocity"); iter != map.end()) {
    config_.animation_velocity_ = iter->second->GetDouble();
  }
  if (const auto iter = map.find("enable-region-view"); iter != map.end()) {
    config_.region_view_ = iter->second->AsBool();
  }
  if (const auto iter = map.find("region-rect");
      iter != map.end() && iter->second->GetType() == ValueType::kArray) {
    ParseRectValue(iter->second.get(), &config_.region_rect_);
  }
  if (const auto iter = map.find("source-type"); iter != map.end()) {
    config_.source_type_ = iter->second->GetString() == "plainText"
                               ? SourceType::kPlainText
                               : SourceType::kMarkdown;
  }
  if (const auto iter = map.find("frame-step-count"); iter != map.end()) {
    config_.frame_step_count_ = std::max(1, iter->second->GetInt());
  }
  if (const auto iter = map.find("frame-step-interval-ms");
      iter != map.end()) {
    const auto value = static_cast<int64_t>(iter->second->GetDouble());
    config_.frame_step_interval_ms_ = std::max<int64_t>(0, value);
  }
  if (const auto iter = map.find("frame-visible-rect"); iter != map.end()) {
    config_.has_frame_visible_rect_ =
        ParseRectValue(iter->second.get(), &config_.frame_visible_rect_);
  }
  if (const auto iter = map.find("frame-visible-rects");
      iter != map.end() && iter->second->GetType() == ValueType::kArray) {
    config_.frame_visible_rects_.clear();
    for (const auto& value : iter->second->AsArray()) {
      RectF rect;
      if (ParseRectValue(value.get(), &rect)) {
        config_.frame_visible_rects_.push_back(rect);
      }
    }
  }
  if (const auto iter = map.find("frame-steps");
      iter != map.end() && iter->second->GetType() == ValueType::kArray) {
    config_.frame_steps_.clear();
    for (const auto& step_value : iter->second->AsArray()) {
      if (step_value->GetType() != ValueType::kMap) {
        continue;
      }
      MarkdownCaseFrameStep step;
      const auto& step_map = step_value->AsMap();
      if (const auto interval_iter = step_map.find("interval-ms");
          interval_iter != step_map.end()) {
        step.interval_ms_ =
            std::max<int64_t>(0, interval_iter->second->GetDouble());
      }
      if (const auto interval_iter = step_map.find("interval");
          interval_iter != step_map.end()) {
        step.interval_ms_ =
            std::max<int64_t>(0, interval_iter->second->GetDouble());
      }
      RectF rect;
      if (const auto rect_iter = step_map.find("visible-rect");
          rect_iter != step_map.end() &&
          ParseRectValue(rect_iter->second.get(), &rect)) {
        step.has_visible_rect_ = true;
        step.visible_rect_ = rect;
      }
      config_.frame_steps_.push_back(step);
    }
  }
}

bool MarkdownCaseBuilder::LoadCaseInDirectory(const fs::path& path) {
  const auto attributes_path = path / "attributes.json";
  const auto markdown_path = path / "markdown.md";
  if (!fs::is_regular_file(markdown_path)) {
    return false;
  }
  config_.markdown_ = ReadFileToString(markdown_path);
  if (fs::is_regular_file(attributes_path)) {
    const auto attributes = ReadJsonFileToValue(attributes_path);
    ApplyAttributes(attributes.get());
  }
  return true;
}

std::vector<MarkdownCaseEntry> MarkdownCaseBuilder::LoadCases(
    const fs::path& root) {
  std::vector<MarkdownCaseEntry> cases;
  std::error_code ec;
  auto iterator = fs::directory_iterator(root, ec);
  if (ec) {
    return cases;
  }

  for (const auto& entry : iterator) {
    std::error_code entry_ec;
    if (!entry.is_directory(entry_ec)) {
      continue;
    }
    const auto directory = entry.path();
    const auto markdown_path = directory / "markdown.md";
    if (!fs::is_regular_file(markdown_path, entry_ec)) {
      continue;
    }
    MarkdownCaseEntry markdown_case;
    markdown_case.name_ = directory.filename().string();
    markdown_case.directory_ = directory;
    markdown_case.markdown_ = ReadFileToString(markdown_path);
    const auto attributes_path = directory / "attributes.json";
    if (fs::is_regular_file(attributes_path, entry_ec)) {
      markdown_case.attributes_ = ReadJsonFileToValue(attributes_path);
    }
    cases.emplace_back(std::move(markdown_case));
  }

  std::sort(cases.begin(), cases.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.name_ < rhs.name_;
  });
  return cases;
}

std::vector<MarkdownCaseFrameStep> MarkdownCaseBuilder::BuildFrameSteps(
    const MarkdownCaseConfig& config) {
  if (!config.frame_steps_.empty()) {
    return config.frame_steps_;
  }
  std::vector<MarkdownCaseFrameStep> steps;
  steps.reserve(config.frame_step_count_);
  for (int32_t i = 0; i < config.frame_step_count_; i++) {
    MarkdownCaseFrameStep step;
    step.interval_ms_ = config.frame_step_interval_ms_;
    if (i < static_cast<int32_t>(config.frame_visible_rects_.size())) {
      step.has_visible_rect_ = true;
      step.visible_rect_ = config.frame_visible_rects_[i];
    } else if (config.has_frame_visible_rect_) {
      step.has_visible_rect_ = true;
      step.visible_rect_ = config.frame_visible_rect_;
    }
    steps.push_back(step);
  }
  return steps;
}

}  // namespace serval::markdown::testing
