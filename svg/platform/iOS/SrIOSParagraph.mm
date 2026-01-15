// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/iOS/SrIOSParagraph.h"
#include "canvas/SrParagraph.h"
#include "platform/iOS/SrIOSCanvas.h"

namespace serval {
namespace svg {
namespace canvas {

static UIColor* GetUIColorFromI32(uint32_t color) {
  CGFloat alpha = ((color & 0xFF000000) >> 24) / 255.f;
  CGFloat red = ((color & 0x00FF0000) >> 16) / 255.f;
  CGFloat green = ((color & 0x0000FF00) >> 8) / 255.f;
  CGFloat blue = (color & 0x000000FF) / 255.f;
  return [UIColor colorWithRed:red green:green blue:blue alpha:alpha];
}

std::unique_ptr<ParagraphFactory> CreateParagraphFactoryFactory(
    const SrCanvas* srCanvas) {
  return std::unique_ptr<ParagraphFactory>(new ParagraphFactoryTK());
}

ParagraphFactoryTK::ParagraphFactoryTK() {
  text_storage_ = [[NSTextStorage alloc] init];
  text_container_ = [[NSTextContainer alloc] init];
  layout_manager_ = [[NSLayoutManager alloc] init];

  [layout_manager_ addTextContainer:text_container_];
  [text_storage_ addLayoutManager:layout_manager_];
  // Top default style.
  style_stack_.emplace_back((SrTextStyle){NSVG_RGB(0, 0, 0), 14.f});
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
  [attribute setObject:GetUIColorFromI32(style.color)
                forKey:NSForegroundColorAttributeName];
  [attribute setObject:font forKey:NSFontAttributeName];
  max_ascent_ = MAX(font.ascender, max_ascent_);

  NSString* string = [[NSString alloc] initWithUTF8String:text.data()];
  NSMutableAttributedString* attributed_string =
      [[NSMutableAttributedString alloc] initWithString:string
                                             attributes:attribute];
  [text_storage_ appendAttributedString:attributed_string];
}

std::unique_ptr<Paragraph> ParagraphFactoryTK::CreateParagraph() {
  return std::unique_ptr<Paragraph>(
      new ParagraphTK(text_storage_, text_container_, layout_manager_,
                      max_ascent_, std::move(paragraph_style_)));
}

ParagraphTK::ParagraphTK(NSTextStorage* text_storage,
                         NSTextContainer* text_container,
                         NSLayoutManager* layout_manager, CGFloat ascent_offset,
                         SrParagraphStyle&& paragraph_style)
    : text_storage_(text_storage),
      text_container_(text_container),
      layout_manager_(layout_manager),
      ascent_offset_(ascent_offset),
      paragraph_style_(std::move(paragraph_style)) {}

void ParagraphTK::Layout(float max_width) {
  // no limits on height now.
  [text_container_ setSize:CGSizeMake(max_width, -1)];
}

void ParagraphTK::Draw(SrCanvas* canvas, float x, float y) {
  ios::SrIOSCanvas* canvas_q2d = reinterpret_cast<ios::SrIOSCanvas*>(canvas);
  CGContextRef ctx = CGContextRetain(canvas_q2d->Context());
  NSRange range = [layout_manager_ glyphRangeForTextContainer:text_container_];
  [layout_manager_ ensureLayoutForTextContainer:text_container_];
  CGRect rect = [layout_manager_ boundingRectForGlyphRange:range
                                           inTextContainer:text_container_];
  switch (paragraph_style_.text_anchor) {
    case SR_SVG_TEXT_ANCHOR_START:
      break;
    case SR_SVG_TEXT_ANCHOR_MIDDLE:
      x = x - rect.size.width / 2;
      break;
    case SR_SVG_TEXT_ANCHOR_END:
      x = x - rect.size.width;
      break;
  }
  y = y - ascent_offset_;
  rect.origin.x = x;
  rect.origin.y = y;
  UIGraphicsPushContext(ctx);
  [text_storage_ drawAtPoint:CGPointMake(x, y)];
  UIGraphicsPopContext();
}

void ParagraphFactoryTK::Reset() {
  text_storage_ = [[NSTextStorage alloc] init];
  text_container_ = [[NSTextContainer alloc] init];
  layout_manager_ = [[NSLayoutManager alloc] init];

  [layout_manager_ addTextContainer:text_container_];
  [text_storage_ addLayoutManager:layout_manager_];
  style_stack_.clear();
  style_stack_.emplace_back((SrTextStyle){NSVG_RGB(0, 0, 0), 14.f});
}

}  // namespace canvas
}  // namespace svg
}  // namespace serval
