// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGText.h"

namespace serval::svg::element {

static inline uint32_t ResolveEffectiveColor(
    const SrSVGNodeBase& node, const SrSVGRenderContext& context) {
  if (node.color_) {
    return node.color_->color;
  }
  if (node.inherit_color_) {
    return node.inherit_color_->color;
  }
  if (context.has_default_color) {
    return context.default_color;
  }
  return NSVG_RGBA(0, 0, 0, 255);
}

void SrSVGRawText::AppendToParagraph(canvas::ParagraphFactory* paragraph,
                                     SrSVGRenderContext& context) const {
  paragraph->AddText(text_);
}

bool SrSVGTextContainer::ParseAndSetAttribute(const char* name,
                                              const char* value) {
  if (strcmp("font-size", name) == 0) {
    return ParseLengthAttribute(value, font_size_, LengthRange::kNonNegative);
  }
  return SrSVGBaseText::ParseAndSetAttribute(name, value);
}

void SrSVGTextContainer::AppendChild(SrSVGNodeBase* node) {
  if (node->Tag() == SrSVGTag::kTextLiteral ||
      node->Tag() == SrSVGTag::kTSpan) {
    children_.emplace_back(reinterpret_cast<SrSVGBaseText*>(node));
  }
}

bool SrSVGTextContainer::HasChildren() const {
  return !children_.empty();
}

void SrSVGTextContainer::AppendToParagraph(canvas::ParagraphFactory* paragraph,
                                           SrSVGRenderContext& context) const {
  SrTextStyle style = {NSVG_RGBA(0, 0, 0, 255), 14.0};
  if (fill_ && fill_->type == SERVAL_PAINT_COLOR) {
    if (fill_->content.color.type == SERVAL_CURRENT_COLOR) {
      style.color = ResolveEffectiveColor(*this, context);
    } else {
      style.color = fill_->content.color.color;
    }
  }
  float font_size;
  if (!resolve_serval_length(&font_size_, &context, SR_SVG_LENGTH_TYPE_NUMERIC,
                             &font_size) ||
      font_size <= 0.f)
    return;
  style.font_size = font_size;
  paragraph->PushTextStyle(style);
  for (auto& child : children_) {
    child->AppendToParagraph(paragraph, context);
  }
  paragraph->PopTextStyle();
}

void SrSVGText::OnRender(canvas::SrCanvas* canvas,
                         SrSVGRenderContext& context) {
  float xform[6];
  ResolvedTransform(xform, context, canvas->PathFactory());
  canvas->Transform(xform);
  float x, y;
  if (!resolve_serval_length(&x_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                             &x) ||
      !resolve_serval_length(&y_, &context, SR_SVG_LENGTH_TYPE_VERTICAL, &y))
    return;
  const auto paragraph_builder = canvas::CreateParagraphFactoryFactory(canvas);
  if (!paragraph_builder)
    return;
  //TODO(renzhongyue hujing) need separate the paragraph_builder with the factory
  AppendToParagraph(paragraph_builder.get(), context);

  SrParagraphStyle paragraph_style;
  paragraph_style.text_anchor = text_anchor_;
  paragraph_builder->SetParagraphStyle(std::move(paragraph_style));

  const auto paragraph = paragraph_builder->CreateParagraph();
  // TODO(renzhongyue): Layout the paragraph with the width limits. Now using a unlimited width, all text will display on single line.
  if (paragraph) {
    paragraph->Layout(std::numeric_limits<float>::max());
    paragraph->Draw(canvas, x, y);
  }
}

bool SrSVGText::OnPrepareToRender(canvas::SrCanvas* canvas,
                                  SrSVGRenderContext& context) const {
  SrSVGNode::OnPrepareToRender(canvas, context);
  if (!HasChildren()) {
    return false;
  }
  return true;
}

bool SrSVGText::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp("x", name) == 0) {
    parse_serval_length(value, &x_);
  } else if (strcmp("y", name) == 0) {
    parse_serval_length(value, &y_);
  } else if (strcmp("text-anchor", name) == 0) {
    if (strcmp(value, "start") == 0) {
      text_anchor_ = SR_SVG_TEXT_ANCHOR_START;
    } else if (strcmp(value, "middle") == 0) {
      text_anchor_ = SR_SVG_TEXT_ANCHOR_MIDDLE;
    } else if (strcmp(value, "end") == 0) {
      text_anchor_ = SR_SVG_TEXT_ANCHOR_END;
    }
    return true;
  } else {
    return SrSVGTextContainer::ParseAndSetAttribute(name, value);
  }
  return true;
}

}  // namespace serval::svg::element
