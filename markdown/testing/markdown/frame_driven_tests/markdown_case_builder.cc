// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "testing/markdown/frame_driven_tests/markdown_case_builder.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>
#include "markdown/utils/markdown_string_utils.h"
#include "markdown/view/markdown_view.h"

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

bool MarkdownCaseBuilder::ParseRangeValue(Value* value, Range* range) {
  if (value == nullptr || range == nullptr ||
      value->GetType() != ValueType::kArray) {
    return false;
  }
  const auto& array = value->AsArray();
  if (array.size() < 2) {
    return false;
  }
  range->start_ = static_cast<int32_t>(array[0]->GetDouble());
  range->end_ = static_cast<int32_t>(array[1]->GetDouble());
  return true;
}

bool MarkdownCaseBuilder::ParsePaddingsValue(Value* value, Paddings* paddings) {
  if (value == nullptr || paddings == nullptr) {
    return false;
  }
  if (value->GetType() == ValueType::kArray) {
    const auto& array = value->AsArray();
    if (array.size() < 4) {
      return false;
    }
    paddings->left_ = static_cast<float>(array[0]->GetDouble());
    paddings->top_ = static_cast<float>(array[1]->GetDouble());
    paddings->right_ = static_cast<float>(array[2]->GetDouble());
    paddings->bottom_ = static_cast<float>(array[3]->GetDouble());
    return true;
  }
  const auto padding = static_cast<float>(value->GetDouble());
  paddings->left_ = padding;
  paddings->top_ = padding;
  paddings->right_ = padding;
  paddings->bottom_ = padding;
  return true;
}

void MarkdownCaseBuilder::DecodeAttributes(Value* attributes) {
  if (attributes == nullptr || attributes->GetType() != ValueType::kMap) {
    return;
  }
  auto& map = attributes->AsMap();
  if (const auto iter = map.find("width"); iter != map.end()) {
    attributes_.width = iter->second->AsDouble();
  }
  if (const auto iter = map.find("height"); iter != map.end()) {
    attributes_.height = iter->second->AsDouble();
  }
  if (const auto iter = map.find("animation-type"); iter != map.end()) {
    const auto type = iter->second->GetString();
    if (type == "typewriter") {
      attributes_.animation_type = MarkdownAnimationType::kTypewriter;
    } else if (type == "line-expand") {
      attributes_.animation_type = MarkdownAnimationType::kLineExpand;
    } else {
      attributes_.animation_type = MarkdownAnimationType::kNone;
    }
  }
  if (const auto iter = map.find("generate"); iter != map.end()) {
    attributes_.generate_ground_truth = iter->second->AsBool();
  }
  if (const auto iter = map.find("initial-animation-step"); iter != map.end()) {
    attributes_.initial_animation_step = iter->second->GetInt();
  }
  if (const auto iter = map.find("text-maxlines"); iter != map.end()) {
    attributes_.max_lines = iter->second->GetInt();
  }
  if (const auto iter = map.find("content-complete"); iter != map.end()) {
    attributes_.content_complete = iter->second->AsBool();
  }
  if (const auto iter = map.find("style");
      iter != map.end() && iter->second->GetType() == ValueType::kMap) {
    MergeMap(attributes_.style->AsMap(), iter->second->AsMap());
  }
  if (const auto iter = map.find("text-mark-attachments"); iter != map.end()) {
    attributes_.attachments = CloneValue(iter->second.get());
  }
  if (const auto iter = map.find("markdown-effect"); iter != map.end()) {
    attributes_.effects = CloneValue(iter->second.get());
  }
  if (const auto iter = map.find("animation-velocity"); iter != map.end()) {
    attributes_.animation_velocity = iter->second->GetDouble();
  }
  if (const auto iter = map.find("region-rect");
      iter != map.end() && iter->second->GetType() == ValueType::kArray) {
    ParseRectValue(iter->second.get(), &attributes_.visible_rect);
  }
  if (const auto iter = map.find("source-type"); iter != map.end()) {
    attributes_.source_type = iter->second->GetString() == "plainText"
                                  ? SourceType::kPlainText
                                  : SourceType::kMarkdown;
  }
  if (const auto iter = map.find("content-range");
      iter != map.end() &&
      ParseRangeValue(iter->second.get(), &attributes_.content_range)) {}
  if (const auto iter = map.find("content-range-start"); iter != map.end()) {
    attributes_.content_range.start_ = iter->second->GetInt();
  }
  if (const auto iter = map.find("content-range-end"); iter != map.end()) {
    attributes_.content_range.end_ = iter->second->GetInt();
  }
  if (const auto iter = map.find("paddings");
      iter != map.end() &&
      ParsePaddingsValue(iter->second.get(), &attributes_.paddings)) {}
  if (const auto iter = map.find("padding");
      iter != map.end() &&
      ParsePaddingsValue(iter->second.get(), &attributes_.paddings)) {}
  if (const auto iter = map.find("padding-left"); iter != map.end()) {
    attributes_.paddings.left_ = static_cast<float>(iter->second->GetDouble());
  }
  if (const auto iter = map.find("padding-top"); iter != map.end()) {
    attributes_.paddings.top_ = static_cast<float>(iter->second->GetDouble());
  }
  if (const auto iter = map.find("padding-right"); iter != map.end()) {
    attributes_.paddings.right_ = static_cast<float>(iter->second->GetDouble());
  }
  if (const auto iter = map.find("padding-bottom"); iter != map.end()) {
    attributes_.paddings.bottom_ =
        static_cast<float>(iter->second->GetDouble());
  }
  if (const auto iter = map.find("allow-break-around-punctuation");
      iter != map.end()) {
    attributes_.allow_break_around_punctuation = iter->second->AsBool();
  }
  if (const auto iter = map.find("typewriter-dynamic-height");
      iter != map.end()) {
    attributes_.typewriter_dynamic_height = iter->second->AsBool();
  }
  if (const auto iter = map.find("typewriter-height-transition-duration");
      iter != map.end()) {
    attributes_.typewriter_height_transition_duration =
        static_cast<float>(iter->second->GetDouble());
  }
  if (const auto iter = map.find("typewriter-height-transition-prefetch");
      iter != map.end()) {
    attributes_.typewriter_height_transition_prefetch = iter->second->AsBool();
  }
  if (const auto iter = map.find("enable-text-selection"); iter != map.end()) {
    attributes_.enable_selection = iter->second->AsBool();
  }
}

void MarkdownCaseBuilder::ApplyAttributes(MarkdownAttributes& attributes,
                                          MarkdownView* markdown_view) {

  markdown_view->SetContent(attributes.markdown);
  markdown_view->SetStyle(attributes.style->AsMap());
  markdown_view->SetAnimationType(attributes.animation_type);
  markdown_view->SetAnimationVelocity(attributes.animation_velocity);
  markdown_view->SetAnimationStep(attributes.initial_animation_step);
  markdown_view->SetContentComplete(attributes.content_complete);
  markdown_view->SetTextMaxLines(attributes.max_lines);
  markdown_view->SetTextAttachments(std::move(attributes.attachments));
  markdown_view->SetMarkdownEffect(std::move(attributes.effects));
  markdown_view->SetSourceType(attributes.source_type);
  markdown_view->SetPaddings(
      attributes.paddings.left_, attributes.paddings.top_,
      attributes.paddings.right_, attributes.paddings.bottom_);
  markdown_view->SetEnableBreakAroundPunctuation(
      attributes.allow_break_around_punctuation);
  markdown_view->SetTypewriterDynamicHeight(
      attributes.typewriter_dynamic_height);
  markdown_view->SetHeightTransitionDuration(
      attributes.typewriter_height_transition_duration);
  markdown_view->SetTypewriterHeightTransitionPrefetch(
      attributes.typewriter_height_transition_prefetch);
  markdown_view->SetEnableSelection(attributes.enable_selection);
}

void MarkdownCaseBuilder::UpdateSteps(Value* steps) {
  std::vector<MarkdownAction> actions;
  actions.emplace_back(MarkdownAction{
      .type = MarkdownActionType::kModifyMeasureSpec,
      .value =
          MeasureSpec{
              .width_ = attributes_.width,
              .width_mode_ = tttext::LayoutMode::kAtMost,
              .height_ = attributes_.height,
              .height_mode_ = tttext::LayoutMode::kAtMost,
          },
  });
  actions.emplace_back(MarkdownAction{
      .type = MarkdownActionType::kModifyVisibleRect,
      .value = attributes_.visible_rect,
  });

  frame_steps_.emplace_back(MarkdownFrameStep{0, std::move(actions)});
  if (steps != nullptr) {
    if (steps->GetType() == ValueType::kMap) {
      frame_steps_.emplace_back(ReadStep(steps->AsMap()));
    } else if (steps->GetType() == ValueType::kArray) {
      auto& array = steps->AsArray();
      for (auto& step : array) {
        frame_steps_.emplace_back(ReadStep(step->AsMap()));
      }
    }
  }
}

MarkdownFrameStep MarkdownCaseBuilder::ReadStep(ValueMap& step) const {
  MarkdownFrameStep result{.timestamp = 0, .actions = {}};
  if (step.find("timestamp") != step.end()) {
    result.timestamp = step["timestamp"]->GetLong();
  }
  if (step.find("actions") != step.end()) {
    auto& actions = step["actions"];
    if (actions->GetType() == ValueType::kArray) {
      for (auto& action : actions->AsArray()) {
        result.actions.emplace_back(ReadAction(action->AsMap()));
      }
    }
  }
  return result;
}

MarkdownAction MarkdownCaseBuilder::ReadAction(ValueMap& action) const {
  MarkdownAction result{.type = MarkdownActionType::kNone, .value = {}};
  auto type = action["type"]->GetString();
  if (type == "modify_measure_spec") {
    result.type = MarkdownActionType::kModifyMeasureSpec;
    float width = action["width"]->GetDouble();
    float height = action["height"]->GetDouble();
    result.value = MeasureSpec{.width_ = width,
                               .width_mode_ = tttext::LayoutMode::kAtMost,
                               .height_ = height,
                               .height_mode_ = tttext::LayoutMode::kAtMost};
  } else if (type == "modify_visible_rect") {
    result.type = MarkdownActionType::kModifyVisibleRect;
    float left = action["left"]->GetDouble();
    float top = action["top"]->GetDouble();
    float right = action["right"]->GetDouble();
    float bottom = action["bottom"]->GetDouble();
    result.value = RectF::MakeLTRB(left, top, right, bottom);
  } else if (type == "modify_content") {
    result.type = MarkdownActionType::kModifyContent;
    if (action.find("content") != action.end()) {
      result.value = action["content"]->GetString();
    } else {
      int32_t start = CIndexToUTF8Index(attributes_.markdown.c_str(),
                                        attributes_.markdown.length(),
                                        action["start"]->GetLong());
      int32_t end = CIndexToUTF8Index(attributes_.markdown.c_str(),
                                      attributes_.markdown.length(),
                                      action["end"]->GetLong());
      result.value = attributes_.markdown.substr(start, end);
    }
  } else if (type == "tap") {
    result.type = MarkdownActionType::kTap;
    result.value = ReadGestureInfo(action);
  } else if (type == "long_press") {
    result.type = MarkdownActionType::kLongPress;
    result.value = ReadGestureInfo(action);
  } else if (type == "pan") {
    result.type = MarkdownActionType::kPan;
    result.value = ReadGestureInfo(action);
  }
  return result;
}

GestureInfo MarkdownCaseBuilder::ReadGestureInfo(ValueMap& gesture) {
  GestureInfo info{
      .type = GestureEventType::kUnknown, .point = {0, 0}, .motion = {0, 0}};
  if (gesture.find("type") != gesture.end()) {
    auto type = gesture["type"]->GetString();
    if (type == "down") {
      info.type = GestureEventType::kDown;
    } else if (type == "move") {
      info.type = GestureEventType::kMove;
    } else if (type == "up") {
      info.type = GestureEventType::kUp;
    } else if (type == "cancel") {
      info.type = GestureEventType::kCancel;
    }
  }
  if (gesture.find("point") != gesture.end()) {
    auto& point = gesture["point"]->AsMap();
    info.point = {
        static_cast<float>(point["x"]->GetDouble()),
        static_cast<float>(point["y"]->GetDouble()),
    };
  }
  if (gesture.find("motion") != gesture.end()) {
    auto& motion = gesture["motion"]->AsMap();
    info.motion = {
        static_cast<float>(motion["x"]->GetDouble()),
        static_cast<float>(motion["y"]->GetDouble()),
    };
  }
  return info;
}

bool MarkdownCaseBuilder::LoadCaseInDirectory(const fs::path& path) {
  const auto attributes_path = path / "attributes.json";
  const auto markdown_path = path / "markdown.md";
  const auto steps_path = path / "frames.md";
  const auto ground_truth_path = path / "ground_truth.json";
  if (!fs::is_regular_file(markdown_path)) {
    return false;
  }
  attributes_.markdown = ReadFileToString(markdown_path);
  if (fs::is_regular_file(attributes_path)) {
    const auto attributes = ReadJsonFileToValue(attributes_path);
    DecodeAttributes(attributes.get());
  }
  if (fs::is_regular_file(steps_path)) {
    const auto steps = ReadJsonFileToValue(steps_path);
    UpdateSteps(steps.get());
  } else {
    UpdateSteps(nullptr);
  }
  attributes_.ground_truth = ReadJsonFileToValue(ground_truth_path);
  return true;
}

MarkdownCaseEntry MarkdownCaseBuilder::LoadSingleCase(
    const fs::path& directory) {
  std::error_code entry_ec;
  const auto markdown_path = directory / "markdown.md";
  if (!fs::is_regular_file(markdown_path, entry_ec)) {
    return {};
  }
  MarkdownCaseEntry markdown_case;
  markdown_case.name = directory.filename().string();

  MarkdownCaseBuilder builder;
  builder.LoadCaseInDirectory(directory);
  markdown_case.attributes = std::move(builder.attributes_);
  markdown_case.steps = builder.GetFrameSteps();
  markdown_case.directory = directory;
  return markdown_case;
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
    const auto& directory = entry.path();
    auto markdown_case = LoadSingleCase(directory);
    if (markdown_case.attributes.markdown.empty()) {
      continue;
    }
    cases.emplace_back(std::move(markdown_case));
  }

  std::sort(cases.begin(), cases.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.name < rhs.name;
  });
  return cases;
}

}  // namespace serval::markdown::testing
