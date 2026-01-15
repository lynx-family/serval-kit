// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_CANVAS_SRPARAGRAPH_H_
#define SVG_INCLUDE_CANVAS_SRPARAGRAPH_H_

#include <memory>
#include <string>

#include "canvas/SrCanvas.h"
#include "element/SrSVGTypes.h"

namespace serval::svg::canvas {
class Paragraph {
 public:
  virtual ~Paragraph() = default;
  virtual void Layout(float max_width) = 0;
  virtual void Draw(SrCanvas* canvas, float x, float y) = 0;
};

class ParagraphFactory {
 public:
  virtual ~ParagraphFactory() = default;
  virtual std::unique_ptr<Paragraph> CreateParagraph() = 0;
  virtual void PushTextStyle(const SrTextStyle& style) = 0;
  virtual void PopTextStyle() = 0;
  virtual void SetParagraphStyle(SrParagraphStyle&& style) = 0;
  virtual void AddText(const std::string& text) = 0;
  virtual void Reset() = 0;
};

std::unique_ptr<ParagraphFactory> CreateParagraphFactoryFactory(
    const SrCanvas* srCanvas);
}  // namespace serval::svg::canvas

#endif  // SVG_INCLUDE_CANVAS_SRPARAGRAPH_H_
