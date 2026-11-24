// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_IOS_SRIOSPARAGRAPH_H_
#define SVG_INCLUDE_PLATFORM_IOS_SRIOSPARAGRAPH_H_

#import <UIKit/UIKit.h>

#include <deque>
#include <memory>
#include <string>

#include "canvas/SrParagraph.h"

namespace serval {
namespace svg {
namespace canvas {

// UIKit implementation of Paragraph.

class ParagraphTK : public Paragraph {
 public:
  ParagraphTK(NSAttributedString* text, CGFloat ascent_offset,
              SrParagraphStyle paragraph_style);
  void Layout(float max_width) override;
  void Draw(SrCanvas* canvas, float x, float y) override;

 private:
  NSAttributedString* text_;
  CGFloat text_width_{0};
  CGFloat ascent_offset_;
  SrParagraphStyle paragraph_style_;
};

class ParagraphFactoryTK : public ParagraphFactory {
 public:
  std::unique_ptr<Paragraph> CreateParagraph() override;
  void PushTextStyle(const SrTextStyle& style) override;
  void PopTextStyle() override;
  void SetParagraphStyle(SrParagraphStyle&& style) override;
  void AddText(const std::string& text) override;
  void Reset() override;
  ParagraphFactoryTK();

 private:
  std::deque<SrTextStyle> style_stack_;
  NSMutableAttributedString* text_;
  CGFloat max_ascent_{0};
  SrParagraphStyle paragraph_style_{SR_SVG_TEXT_ANCHOR_START};
};

}  // namespace canvas
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_IOS_SRIOSPARAGRAPH_H_
