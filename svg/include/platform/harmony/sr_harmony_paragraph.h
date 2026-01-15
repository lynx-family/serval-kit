// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_PARAGRAPH_H_
#define SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_PARAGRAPH_H_

#include <memory>
#include <string>

#include "canvas/SrParagraph.h"

namespace serval {
namespace svg {
namespace canvas {

class SrHarmonyParagraph : public Paragraph {
 public:
  SrHarmonyParagraph() = default;
  ~SrHarmonyParagraph() = default;
  void Layout(float max_width) override{};
  void Draw(SrCanvas* canvas, float x, float y) override{};
};

class SrHarmonyParagraphFactory : public ParagraphFactory {
 public:
  SrHarmonyParagraphFactory(const SrCanvas* srCanvas){
      //TODO  create paragraphFactory
  };
  ~SrHarmonyParagraphFactory() = default;

  std::unique_ptr<Paragraph> CreateParagraph() override {
    //TODO  adapt to paragraph
    return std::unique_ptr<SrHarmonyParagraph>(nullptr);
  };
  void PushTextStyle(const SrTextStyle& style) override{};
  void PopTextStyle() override{};
  void SetParagraphStyle(SrParagraphStyle&& style) override{};
  void AddText(const std::string& text) override{};
  void Reset() override{};
};

}  // namespace canvas
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_PARAGRAPH_H_
