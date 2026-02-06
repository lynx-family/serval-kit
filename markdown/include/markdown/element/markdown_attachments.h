// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_ELEMENT_MARKDOWN_ATTACHMENTS_H_
#define MARKDOWN_ELEMENT_MARKDOWN_ATTACHMENTS_H_

#include "markdown/style/markdown_style.h"
#include "markdown/style/markdown_style_value.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx::markdown {
class MarkdownPage;
class MarkdownAttachmentLineStyle {
 public:
  MarkdownLineType line_type_{MarkdownLineType::kNone};
  uint32_t color_{0};
  std::unique_ptr<tttext::RunDelegate> gradient_{nullptr};
  std::unique_ptr<MarkdownStyleValue> width_{nullptr};
  std::unique_ptr<MarkdownStyleValue> element_size_{nullptr};
  std::unique_ptr<MarkdownStyleValue> empty_size_{nullptr};
};

class MarkdownAttachmentRectStyle {
 public:
  std::unique_ptr<MarkdownStyleValue> left_{nullptr};
  std::unique_ptr<MarkdownStyleValue> right_{nullptr};
  std::unique_ptr<MarkdownStyleValue> top_{nullptr};
  std::unique_ptr<MarkdownStyleValue> bottom_{nullptr};
  std::unique_ptr<MarkdownStyleValue> radius_{nullptr};
  uint32_t color_{0};
  std::unique_ptr<tttext::RunDelegate> gradient_;
  std::unique_ptr<MarkdownStyleValue> stroke_width_{nullptr};
  uint32_t stroke_color_{0};
};

enum class CharIndexType { kParsedContent, kSource };
enum class AttachmentLayer { kBackground, kForeGround };
class MarkdownTextAttachment {
 public:
  MarkdownTextAttachment() = default;
  ~MarkdownTextAttachment() = default;
  void DrawOnRect(tttext::ICanvasHelper* canvas, RectF rect) const;
  void DrawOnMultiLines(tttext::ICanvasHelper* canvas,
                        const std::vector<RectF>& lines_rect,
                        float total_length = 0) const;
  int32_t start_index_{0};
  int32_t end_index_{0};
  CharIndexType index_type_{CharIndexType::kParsedContent};
  AttachmentLayer attachment_layer_{AttachmentLayer::kBackground};
  std::string id_;
  bool clickable_{false};

  MarkdownAttachmentRectStyle rect_;
  MarkdownAttachmentLineStyle border_left_;
  MarkdownAttachmentLineStyle border_right_;
  MarkdownAttachmentLineStyle border_top_;
  MarkdownAttachmentLineStyle border_bottom_;

 protected:
  static void DrawLine(tttext::ICanvasHelper* canvas, PointF start, PointF end,
                       const MarkdownLengthContext& context,
                       const MarkdownAttachmentLineStyle& style, float width);
  static void DrawRect(tttext::ICanvasHelper* canvas, RectF rect,
                       const MarkdownAttachmentRectStyle& style);
};
}  // namespace lynx::markdown

#endif  // MARKDOWN_ELEMENT_MARKDOWN_ATTACHMENTS_H_
