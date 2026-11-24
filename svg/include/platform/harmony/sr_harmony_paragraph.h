// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_PARAGRAPH_H_
#define SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_PARAGRAPH_H_

#include <memory>
#include <string>

#include <native_drawing/drawing_font_collection.h>
#include <native_drawing/drawing_text_typography.h>

#include "canvas/SrParagraph.h"

namespace serval {
namespace svg {
namespace canvas {

class SrHarmonyParagraph : public Paragraph {
 public:
  SrHarmonyParagraph(OH_Drawing_Typography* typography,
                     std::shared_ptr<OH_Drawing_FontCollection> fonts,
                     SrParagraphStyle style);
  void Layout(float max_width) override;
  void Draw(SrCanvas* canvas, float x, float y) override;

 private:
  // Keep the font collection alive until the typography has been destroyed.
  std::shared_ptr<OH_Drawing_FontCollection> fonts_;
  std::unique_ptr<OH_Drawing_Typography,
                  decltype(&OH_Drawing_DestroyTypography)>
      typography_{nullptr, OH_Drawing_DestroyTypography};
  SrParagraphStyle style_;
};

class SrHarmonyParagraphFactory : public ParagraphFactory {
 public:
  explicit SrHarmonyParagraphFactory(const SrCanvas* sr_canvas);
  std::unique_ptr<Paragraph> CreateParagraph() override;
  void PushTextStyle(const SrTextStyle& style) override;
  void PopTextStyle() override;
  void SetParagraphStyle(SrParagraphStyle&& style) override;
  void AddText(const std::string& text) override;
  void Reset() override;

 private:
  std::shared_ptr<OH_Drawing_FontCollection> fonts_;
  std::unique_ptr<OH_Drawing_TypographyStyle,
                  decltype(&OH_Drawing_DestroyTypographyStyle)>
      typography_style_{nullptr, OH_Drawing_DestroyTypographyStyle};
  std::unique_ptr<OH_Drawing_TypographyCreate,
                  decltype(&OH_Drawing_DestroyTypographyHandler)>
      handler_{nullptr, OH_Drawing_DestroyTypographyHandler};
  SrParagraphStyle paragraph_style_{SR_SVG_TEXT_ANCHOR_START};
  size_t style_depth_{0};
};

}  // namespace canvas
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_PARAGRAPH_H_
