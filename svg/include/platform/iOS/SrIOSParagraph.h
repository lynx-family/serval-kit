// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_IOS_SRIOSPARAGRAPH_H_
#define SVG_INCLUDE_PLATFORM_IOS_SRIOSPARAGRAPH_H_

#include <deque>
#include <memory>
#include <string>

#include "canvas/SrParagraph.h"

namespace serval {
namespace svg {
namespace canvas {

// TextKit implementation of Paragraph.

class ParagraphTK : public Paragraph {
 public:
  ParagraphTK(NSTextStorage* text_storage, NSTextContainer* text_container,
              NSLayoutManager* layout_manager, CGFloat ascent_offset,
              SrParagraphStyle&& paragraph_style);
  ~ParagraphTK() = default;
  void Layout(float max_width);
  void Draw(SrCanvas* canvas, float x, float y);

 private:
  NSTextStorage* text_storage_;
  NSTextContainer* text_container_;
  NSLayoutManager* layout_manager_;
  CGFloat ascent_offset_;
  SrParagraphStyle paragraph_style_;
};

class ParagraphFactoryTK : public ParagraphFactory {
 public:
  ~ParagraphFactoryTK() = default;
  std::unique_ptr<Paragraph> CreateParagraph();
  void PushTextStyle(const SrTextStyle& style);
  void PopTextStyle();
  void SetParagraphStyle(SrParagraphStyle&& style);
  void AddText(const std::string& text);
  void Reset();
  ParagraphFactoryTK();

 private:
  std::deque<SrTextStyle> style_stack_;
  NSTextStorage* text_storage_;
  NSTextContainer* text_container_;
  NSLayoutManager* layout_manager_;
  CGFloat max_ascent_{-1};
  SrParagraphStyle paragraph_style_;
};

}  // namespace canvas
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_IOS_SRIOSPARAGRAPH_H_
