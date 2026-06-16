// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_MARKDOWN_CASE_BUILDER_H_
#define MARKDOWN_TESTING_MARKDOWN_MARKDOWN_CASE_BUILDER_H_

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_value.h"
#include "markdown/view/markdown_view_measurer.h"
#include "rapidjson/document.h"

namespace serval::markdown::testing {

using MarkdownCaseValuePtr = std::unique_ptr<Value>;

struct MarkdownCaseFrameStep {
  int64_t interval_ms_{16};
  RectF visible_rect_{};
  bool has_visible_rect_{false};
};

struct MarkdownCaseConfig {
  std::string markdown_;
  ValueMap style_map_;

  float width_ = 0;
  float height_ = 0;
  int32_t max_lines_ = -1;
  MarkdownAnimationType animation_type_{MarkdownAnimationType::kNone};
  bool use_char_based_drawer_ = false;
  bool draw_cursor_if_complete_ = false;
  bool content_complete_ = true;
  int32_t animation_step_ = 0;
  double animation_velocity_ = 0;
  MarkdownCaseValuePtr attachments_ = nullptr;
  bool region_view_ = false;
  RectF region_rect_;
  SourceType source_type_ = SourceType::kMarkdown;

  int32_t frame_step_count_ = 1;
  int64_t frame_step_interval_ms_ = 16;
  bool has_frame_visible_rect_ = false;
  RectF frame_visible_rect_;
  std::vector<RectF> frame_visible_rects_;
  std::vector<MarkdownCaseFrameStep> frame_steps_;

  bool generate_ground_truth_ = false;
};

struct MarkdownCaseEntry {
  std::string name_;
  std::filesystem::path directory_;
  std::string markdown_;
  MarkdownCaseValuePtr attributes_;
};

class MarkdownCaseBuilder {
 public:
  explicit MarkdownCaseBuilder(MarkdownCaseConfig& config)
      : config_(config) {}

  void ApplyAttributes(Value* attributes);
  bool LoadCaseInDirectory(const std::filesystem::path& path);

  static std::vector<MarkdownCaseEntry> LoadCases(
      const std::filesystem::path& root);
  static std::vector<MarkdownCaseFrameStep> BuildFrameSteps(
      const MarkdownCaseConfig& config);

  static MarkdownCaseValuePtr ReadJsonFileToValue(
      const std::filesystem::path& path);
  static MarkdownCaseValuePtr ConvertJson(const rapidjson::Value& value);

 private:
  static std::string ReadFileToString(const std::filesystem::path& path);
  static MarkdownCaseValuePtr CloneValue(Value* value);
  static void MergeMap(ValueMap& dst, ValueMap& src);
  static bool ParseRectValue(Value* value, RectF* rect);

  MarkdownCaseConfig& config_;
};

}  // namespace serval::markdown::testing

#endif  // MARKDOWN_TESTING_MARKDOWN_MARKDOWN_CASE_BUILDER_H_
