// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_MEASURER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_MEASURER_H_
#include <cstdint>
#include <memory>
#include <string_view>
#include "markdown/element/markdown_document.h"
#include "markdown/element/markdown_drawable.h"
#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/utils/markdown_value.h"
namespace lynx::markdown {
enum class MarkdownAnimationType {
  kNone,
  kTypewriter,
};
enum class SourceType { kPlainText, kMarkdown };
class MarkdownViewMeasurer {
 public:
  explicit MarkdownViewMeasurer(
      MarkdownResourceLoader* resource_loader = nullptr);
  ~MarkdownViewMeasurer() = default;

  void SetResourceLoader(MarkdownResourceLoader* resource_loader);
  void SetEventListener(MarkdownEventListener* event_listener);

  void SetContent(std::string_view content);
  void SetContentID(std::string_view id);
  void SetContentComplete(bool complete);
  void SetContentRange(Range range);
  void SetParserType(std::string_view parser_type, void* parser_ud = nullptr);
  void SetSourceType(SourceType type);

  void SetStyle(const ValueMap& style_map);
  void SetStyle(const MarkdownStyle& style);
  void ApplyStyleInRange(const ValueMap& style_map, int32_t char_start,
                         int32_t char_end);

  void SetTextMaxLines(int32_t max_lines);
  void SetEnableBreakAroundPunctuation(bool allow);
  void SetTrimLastParagraphSpace(bool trim);
  void SetPaddings(Paddings paddings);

  void NeedsMeasure();

  SizeF Measure(MeasureSpec spec);
  SizeF GetMeasuredSize() const { return {measured_width_, measured_height_}; }

  void Align();

  void InitialDocument();
  bool DidLayoutInLastMeasure() const { return did_layout_in_last_measure_; }

  std::shared_ptr<MarkdownDocument> GetDocument();

 private:
  std::shared_ptr<MarkdownDocument> document_;
  std::string content_;
  std::string content_id_;
  bool content_complete_{true};
  MarkdownStyle style_{};
  int32_t text_max_lines_{-1};
  int32_t content_start_{0};
  int32_t content_end_{std::numeric_limits<int32_t>::max()};
  bool break_around_punctuation_{false};
  bool trim_last_paragraph_space_{false};

  std::string parser_type_{};
  void* parser_ud_{nullptr};
  SourceType source_type_{SourceType::kMarkdown};

  MeasureSpec last_measure_spec_{};
  float measured_width_{0};
  float measured_height_{0};
  bool needs_measure_{true};
  bool did_layout_in_last_measure_{false};

  Paddings paddings_{};
  MarkdownResourceLoader* resource_loader_{nullptr};
  MarkdownEventListener* event_listener_{nullptr};
};
}  // namespace lynx::markdown
#endif  //MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_MEASURER_H_
