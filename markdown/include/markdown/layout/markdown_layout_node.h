// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_NODE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_NODE_H_
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_node.h"
#include "textra/i_canvas_helper.h"
#include "textra/layout_definition.h"
#include "textra/text_line.h"
namespace serval::markdown {
class MarkdownDrawable;
class MarkdownContext;

class MarkdownLayoutNode : public MarkdownNode {
 public:
  struct LayoutParams {
    float width{0};
    tttext::LayoutMode width_mode{tttext::LayoutMode::kDefinite};
    float height{0};
    tttext::LayoutMode height_mode{tttext::LayoutMode::kDefinite};
    int32_t max_lines{std::numeric_limits<int32_t>::max()};
    bool last_element{true};
  };
  struct LayoutResult {
    float width{0};
    float height{0};
    float baseline{0};
    int32_t char_count{0};
    int32_t line_count{0};
    bool overflow{false};
    bool truncated{false};
  };
  struct AlignParams {
    PointF absolute_position{};
    int32_t absolute_char_pos{0};
  };
  struct RenderParams {
    tttext::ICanvasHelper* canvas;
    int32_t max_char_count;
    MarkdownDrawable* cursor;
  };
  struct TypewriterInfo {
    float max_height{0};
    PointF cursor_position{};
  };
  enum class RectType { kSelection, kLineBounding, kCharBounding };
  enum class RectCoordinate { kAbsolute, kRelative };
  enum class CharRangeType { kChar, kWord, kSentence, kParagraph };
  enum class LayoutMainAxis { kVertical, kHorizontal };

 public:
  explicit MarkdownLayoutNode(MarkdownContext* context);
  ~MarkdownLayoutNode() override;

  const Margins& GetMargins() const { return margins_; }
  void SetMargins(Margins margins) { margins_ = margins; }
  const Paddings& GetPaddings() const { return paddings_; }
  void SetPaddings(Paddings paddings) { paddings_ = paddings; }
  const Borders& GetBorders() const { return borders_; }
  void SetBorders(Borders borders) { borders_ = borders; }
  const Background& GetBackground() const { return background_; }
  void SetBackground(Background background) {
    background_ = std::move(background);
  }
  LayoutMainAxis GetLayoutMainAxis() const { return layout_main_axis_; }
  void SetLayoutMainAxis(LayoutMainAxis axis) { layout_main_axis_ = axis; }
  void EnableCollapseMargin(bool enable);

  LayoutResult Layout(LayoutParams params);
  void Align(AlignParams params);
  void Render(RenderParams params);
  int32_t GetCharCount() const;
  int32_t GetLineCount() const;
  bool HasLayout() const;
  RectF GetLayoutRect() const;
  void UpdateLayoutRect(RectF rect);

 public:
  virtual Range GetCharRangeByPoint(PointF point, CharRangeType type);
  virtual void GetSelectionRectByCharPos(std::vector<RectF>* result,
                                         int32_t char_pos_start,
                                         int32_t char_pos_end, RectType type,
                                         RectCoordinate coordinate);
  virtual void GetContentByCharPos(std::string* result, int32_t char_pos_start,
                                   int32_t char_pos_end);
  virtual void GetLineEndCharIndices(std::vector<int32_t>* result);

 protected:
  virtual LayoutResult OnLayout(LayoutParams params);
  void CleanChildrenLayoutResult() const;
  LayoutResult LayoutChildren(LayoutParams params, float offset_left,
                              float offset_top, float offset_right,
                              float offset_bottom);
  virtual void OnAlign(AlignParams params);
  virtual void OnRender(RenderParams params);
  virtual bool ForceAddTruncation();

  // for list item marker align
  virtual tttext::TextLine* GetMarkerBaseLine();

  float CollapseMargin(float margin_1, float margin_2) const;
  void DrawBackground(const RenderParams& params) const;
  void DrawBorder(const RenderParams& params) const;
  void DrawChildren(RenderParams params) const;
  float GetBorderRadius() const;

  MarkdownContext* context_;
  LayoutResult layout_result_;

  // rect relative parent
  RectF layout_rect_{0, 0, 0, 0};
  bool has_layout_{false};

  PointF absolute_position_{0, 0};
  int32_t absolute_char_pos_{0};

  Margins margins_;
  Paddings paddings_;
  Borders borders_{};
  Background background_;
  LayoutMainAxis layout_main_axis_{LayoutMainAxis::kVertical};
  bool collapse_margin_{true};
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_NODE_H_
