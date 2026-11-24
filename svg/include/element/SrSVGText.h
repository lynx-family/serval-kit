// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGTEXT_H_
#define SVG_INCLUDE_ELEMENT_SRSVGTEXT_H_
#include <list>
#include <optional>
#include <string>

#include "canvas/SrParagraph.h"
#include "element/SrSVGNode.h"

namespace serval::svg::element {

class SrSVGTextParagraph;

struct SrSVGTextStyle {
  const SrSVGPaint* fill{nullptr};
  uint32_t color{NSVG_RGBA(0, 0, 0, 255)};
  float font_size{14.f};
  float fill_opacity{1.f};
  bool preserve_space{false};
};

class SrSVGBaseText : public SrSVGNode {
 public:
  virtual void AppendToParagraph(
      SrSVGTextParagraph& paragraph, const SrSVGRenderContext& context,
      const SrSVGTextStyle& inherited_style) const = 0;

 protected:
  explicit SrSVGBaseText(const SrSVGTag t) : SrSVGNode(t) {}
};

class SrSVGRawText final : public SrSVGBaseText {
 public:
  static SrSVGRawText* Make() { return new SrSVGRawText(); }
  void SetText(const char* text) { text_ = text ? text : ""; }
  void AppendToParagraph(SrSVGTextParagraph& paragraph,
                         const SrSVGRenderContext& context,
                         const SrSVGTextStyle& inherited_style) const override;

 protected:
  SrSVGRawText() : SrSVGBaseText(SrSVGTag::kTextLiteral) {}

 private:
  std::string text_;
};

class SrSVGTextContainer : public SrSVGBaseText {
 public:
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void AppendChild(SrSVGNodeBase*) override;
  void AppendToParagraph(SrSVGTextParagraph& paragraph,
                         const SrSVGRenderContext& context,
                         const SrSVGTextStyle& inherited_style) const override;
  bool HasChildren() const override;

 protected:
  explicit SrSVGTextContainer(const SrSVGTag t) : SrSVGBaseText(t) {}
  bool ResolveTextStyle(const SrSVGRenderContext& context,
                        SrSVGTextStyle& style) const;
  void AppendChildrenToParagraph(SrSVGTextParagraph& paragraph,
                                 const SrSVGRenderContext& context,
                                 const SrSVGTextStyle& style) const;
  std::list<SrSVGBaseText*> children_;

 private:
  std::optional<SrSVGLength> font_size_;
};

class SrSVGText final : public SrSVGTextContainer {
 public:
  static SrSVGText* Make() { return new SrSVGText(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;

 protected:
  void OnRender(canvas::SrCanvas* canvas, SrSVGRenderContext& context) override;
  bool OnPrepareToRender(canvas::SrCanvas* canvas,
                         SrSVGRenderContext& context) const override;

 private:
  SrSVGText() : SrSVGTextContainer(SrSVGTag::kText) {}
  SrSVGLength x_{0, SR_SVG_UNITS_PX};
  SrSVGLength y_{0, SR_SVG_UNITS_PX};
  SrTextAnchor text_anchor_{SR_SVG_TEXT_ANCHOR_START};
};

class SrSVGTextSpan final : public SrSVGTextContainer {
 public:
  static SrSVGTextSpan* Make() { return new SrSVGTextSpan(); }

 private:
  SrSVGTextSpan() : SrSVGTextContainer(SrSVGTag::kTSpan) {}
};

}  // namespace serval::svg::element

#endif  // SVG_INCLUDE_ELEMENT_SRSVGTEXT_H_
