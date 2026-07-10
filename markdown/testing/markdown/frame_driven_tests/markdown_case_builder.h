// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_FRAME_DRIVEN_TESTS_MARKDOWN_CASE_BUILDER_H_
#define MARKDOWN_TESTING_MARKDOWN_FRAME_DRIVEN_TESTS_MARKDOWN_CASE_BUILDER_H_

#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_value.h"
#include "markdown/view/markdown_view_gesture.h"
#include "markdown/view/markdown_view_measurer.h"
#include "rapidjson/document.h"

namespace serval::markdown {
class MarkdownView;
}
namespace serval::markdown::testing {
namespace fs = std::filesystem;
using MarkdownCaseValuePtr = std::unique_ptr<Value>;

enum class MarkdownActionType {
  kNone,
  kModifyMeasureSpec,
  kModifyVisibleRect,
  kModifyContent,

  kTap,
  kLongPress,
  kPan,
};
struct GestureInfo {
  GestureEventType type;
  PointF point;
  PointF motion;
};
struct MarkdownAction {
  MarkdownActionType type;
  std::variant<std::monostate, MeasureSpec, RectF, std::string,
               std::unique_ptr<Value>, GestureInfo>
      value;
};

struct MarkdownFrameStep {
  int64_t timestamp;
  std::vector<MarkdownAction> actions;
};

struct MarkdownAttributes {
  bool generate_ground_truth{false};
  MarkdownCaseValuePtr ground_truth;

  float width{500};
  float height{1e6};
  RectF visible_rect{RectF::MakeLTRB(0, 0, std::numeric_limits<float>::max(),
                                     std::numeric_limits<float>::max())};

  std::string markdown;
  std::unique_ptr<Value> style{Value::MakeMap()};
  MarkdownAnimationType animation_type{MarkdownAnimationType::kNone};
  float animation_velocity{1};
  int32_t initial_animation_step{0};
  bool content_complete{true};
  int32_t max_lines{std::numeric_limits<int32_t>::max()};
  std::unique_ptr<Value> attachments{nullptr};
  std::unique_ptr<Value> effects{nullptr};
  SourceType source_type{SourceType::kMarkdown};
  Range content_range{0, std::numeric_limits<int32_t>::max()};
  Paddings paddings{0, 0, 0, 0};
  bool allow_break_around_punctuation{false};
  bool typewriter_dynamic_height{false};
  float typewriter_height_transition_duration{0};
  bool typewriter_height_transition_prefetch{true};
  bool enable_selection{false};
};

struct MarkdownCaseEntry {
  std::string name;
  fs::path directory;
  MarkdownAttributes attributes;
  std::vector<MarkdownFrameStep> steps;
};

class MarkdownCaseBuilder {
 public:
  explicit MarkdownCaseBuilder() = default;

  bool LoadCaseInDirectory(const std::filesystem::path& path);
  std::vector<MarkdownFrameStep> GetFrameSteps() {
    return std::move(frame_steps_);
  }

  static MarkdownCaseValuePtr ReadJsonFileToValue(
      const std::filesystem::path& path);
  static MarkdownCaseValuePtr ConvertJson(const rapidjson::Value& value);
  static MarkdownCaseEntry LoadSingleCase(const fs::path& directory);
  static std::vector<MarkdownCaseEntry> LoadCases(const fs::path& root);

  MarkdownFrameStep ReadStep(ValueMap& step) const;
  MarkdownAction ReadAction(ValueMap& action) const;
  static GestureInfo ReadGestureInfo(ValueMap& gesture);
  void DecodeAttributes(Value* attributes);
  void UpdateSteps(Value* steps);

  static std::string ReadFileToString(const std::filesystem::path& path);
  static MarkdownCaseValuePtr CloneValue(Value* value);
  static void MergeMap(ValueMap& dst, ValueMap& src);
  static bool ParseRectValue(Value* value, RectF* rect);
  static bool ParseRangeValue(Value* value, Range* range);
  static bool ParsePaddingsValue(Value* value, Paddings* paddings);

  static void ApplyAttributes(MarkdownAttributes& attributes,
                              MarkdownView* view);

  MarkdownAttributes attributes_;
  std::vector<MarkdownFrameStep> frame_steps_;
};

}  // namespace serval::markdown::testing

#endif  // MARKDOWN_TESTING_MARKDOWN_FRAME_DRIVEN_TESTS_MARKDOWN_CASE_BUILDER_H_
