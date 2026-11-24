// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/iOS/SrIOSParagraph.h"
#include "platform/iOS/SrIOSCanvas.h"
#include "platform/iOS/SrIOSColorUtils.h"

namespace serval {
namespace svg {
namespace canvas {

std::unique_ptr<ParagraphFactory> CreateParagraphFactoryFactory(
    const SrCanvas* srCanvas) {
  return std::make_unique<ParagraphFactoryTK>();
}

ParagraphFactoryTK::ParagraphFactoryTK() {
  Reset();
}

void ParagraphFactoryTK::PushTextStyle(const SrTextStyle& style) {
  style_stack_.push_back(style);
}

void ParagraphFactoryTK::PopTextStyle() {
  style_stack_.pop_back();
}

void ParagraphFactoryTK::SetParagraphStyle(SrParagraphStyle&& style) {
  paragraph_style_ = std::move(style);
}

void ParagraphFactoryTK::AddText(const std::string& text) {
  NSMutableDictionary* attribute = [[NSMutableDictionary alloc] init];

  const SrTextStyle& style = style_stack_.back();
  UIFont* font = [UIFont systemFontOfSize:style.font_size];
  [attribute setObject:ios::SrIOSColorUtils::UIColorFromARGB(style.color)
                forKey:NSForegroundColorAttributeName];
  [attribute setObject:font forKey:NSFontAttributeName];
  max_ascent_ = MAX(font.ascender, max_ascent_);

  NSString* string = [[NSString alloc] initWithUTF8String:text.data()];
  NSAttributedString* attributed_string =
      [[NSAttributedString alloc] initWithString:string attributes:attribute];
  [text_ appendAttributedString:attributed_string];
}

std::unique_ptr<Paragraph> ParagraphFactoryTK::CreateParagraph() {
  return std::make_unique<ParagraphTK>([text_ copy], max_ascent_,
                                       paragraph_style_);
}

ParagraphTK::ParagraphTK(NSAttributedString* text, CGFloat ascent_offset,
                         SrParagraphStyle paragraph_style)
    : text_(text),
      ascent_offset_(ascent_offset),
      paragraph_style_(paragraph_style) {}

void ParagraphTK::Layout(float /*max_width*/) {
  // Measure with the same UIKit API that draws the unwrapped text.
  if (paragraph_style_.text_anchor != SR_SVG_TEXT_ANCHOR_START) {
    text_width_ = [text_ size].width;
  }
}

void ParagraphTK::Draw(SrCanvas* canvas, float x, float y) {
  ios::SrIOSCanvas* canvas_q2d = reinterpret_cast<ios::SrIOSCanvas*>(canvas);
  CGContextRef ctx = canvas_q2d->Context();
  CGFloat draw_x = x;
  switch (paragraph_style_.text_anchor) {
    case SR_SVG_TEXT_ANCHOR_START:
      break;
    case SR_SVG_TEXT_ANCHOR_MIDDLE:
      draw_x -= text_width_ / 2;
      break;
    case SR_SVG_TEXT_ANCHOR_END:
      draw_x -= text_width_;
      break;
  }
  UIGraphicsPushContext(ctx);
  [text_ drawAtPoint:CGPointMake(draw_x, y - ascent_offset_)];
  UIGraphicsPopContext();
}

void ParagraphFactoryTK::Reset() {
  text_ = [[NSMutableAttributedString alloc] init];
  max_ascent_ = 0;
  paragraph_style_ = {SR_SVG_TEXT_ANCHOR_START};
  style_stack_.clear();
  style_stack_.emplace_back((SrTextStyle){NSVG_RGB(0, 0, 0), 14.f});
}

}  // namespace canvas
}  // namespace svg
}  // namespace serval
