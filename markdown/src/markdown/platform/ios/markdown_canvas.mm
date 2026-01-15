// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/ios/internal/markdown_canvas.h"
// Removed Lynx dependency for demo build
#import "textra/platform/ios/typeface_coretext.h"
void MarkdownCanvas::Save() {
  if (context_ == nullptr)
    return;
  CGContextSaveGState(context_);
  translate_stack_.emplace_back(translate_point_);
}

void MarkdownCanvas::Restore() {
  if (context_ == nullptr)
    return;
  CGContextRestoreGState(context_);
  translate_point_ = translate_stack_.back();
  translate_stack_.pop_back();
}

void MarkdownCanvas::Translate(float dx, float dy) {
  if (context_ == nullptr)
    return;
  CGContextTranslateCTM(context_, dx, dy);
  translate_point_.Translate(dx, dy);
}

void MarkdownCanvas::DrawGlyphs(const ITypefaceHelper* font,
                                uint32_t glyph_count, const uint16_t* glyphs,
                                const char* text, uint32_t text_bytes, float ox,
                                float oy, float* x, float* y,
                                tttext::Painter* painter) {
  if (context_ == nullptr)
    return;
  return IOSCanvasBase::DrawGlyphs(font, glyph_count, glyphs, text, text_bytes,
                                   ox, oy, x, y, painter);
}

void MarkdownCanvas::DrawRunDelegate(const tttext::RunDelegate* run_delegate,
                                     float left, float top, float right,
                                     float bottom, tttext::Painter* painter) {
  if (context_ == nullptr)
    return;
  CGRect rect = CGRectMake(left, top, right - left, bottom - top);
  auto* markdown_delegate =
      reinterpret_cast<const MarkdownRunDelegate*>(run_delegate);
  if (markdown_delegate->GetMarkdownRunDelegateType() ==
      MarkdownRunDelegateType::kView) {
    auto* inline_view_delegate =
        reinterpret_cast<const MarkdownInlineView*>(markdown_delegate);
    [callback_
        SetInlineViewVisible:[NSString
                                 stringWithUTF8String:inline_view_delegate
                                                          ->GetIdSelector()
                                                          .c_str()]];
  } else if (markdown_delegate->GetMarkdownRunDelegateType() ==
             MarkdownRunDelegateType::kImage) {
    auto* m_image = reinterpret_cast<const MarkdownImage*>(markdown_delegate);
    UIImage* image = m_image->GetImage();
    CGContextSaveGState(context_);
    CGContextTranslateCTM(context_, rect.origin.x,
                          rect.origin.y + rect.size.height);
    CGRect image_rect = CGRectMake(0, 0, image.size.width, image.size.height);
    CGContextScaleCTM(context_, rect.size.width / image_rect.size.width,
                      -rect.size.height / image_rect.size.height);
    CGContextDrawImage(context_, image_rect, image.CGImage);
    CGContextRestoreGState(context_);
  }
}
