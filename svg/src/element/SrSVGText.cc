// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGText.h"

#include "parser/SrSVGTraversalState.h"

namespace serval::svg::element {

namespace {

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

bool PreserveInheritedSpace(const SrSVGNode* parent) {
  for (; parent; parent = parent->authored_parent_) {
    if (parent->preserve_xml_space_) {
      return *parent->preserve_xml_space_;
    }
  }
  return false;
}

bool IsXmlSpace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

}  // namespace

class SrSVGTextParagraph {
 public:
  SrSVGTextParagraph(canvas::ParagraphFactory& factory, float opacity)
      : factory_(factory), opacity_(opacity) {}

  bool HasText() const { return has_text_; }

  void AddText(const std::string& text, const SrSVGTextStyle& style) {
    if (text.empty() || style.font_size <= 0.f) {
      return;
    }
    const SrTextStyle run_style = ResolveRunStyle(style);
    std::string normalized;
    normalized.reserve(text.size());
    if (style.preserve_space) {
      FlushSpace(run_style, normalized);
      for (char c : text) {
        normalized.push_back(IsXmlSpace(c) ? ' ' : c);
      }
      AddRun(normalized, run_style);
      has_text_ = true;
      return;
    }
    for (char c : text) {
      if (IsXmlSpace(c)) {
        if (has_text_ && !pending_space_) {
          pending_space_ = run_style;
        }
        continue;
      }
      FlushSpace(run_style, normalized);
      normalized.push_back(c);
      has_text_ = true;
    }
    if (!normalized.empty()) {
      AddRun(normalized, run_style);
    }
  }

 private:
  void FlushSpace(const SrTextStyle& style, std::string& text) {
    if (!pending_space_) {
      return;
    }
    // Collapsed space keeps the style of the first space in the sequence.
    if (pending_space_->color == style.color &&
        pending_space_->font_size == style.font_size) {
      text.push_back(' ');
    } else {
      AddRun(" ", *pending_space_);
    }
    pending_space_.reset();
  }

  SrTextStyle ResolveRunStyle(const SrSVGTextStyle& style) const {
    // Use the legacy solid black fallback for paint server fills.
    uint32_t color = NSVG_RGBA(0, 0, 0, 255);
    if (style.fill) {
      if (style.fill->type == SERVAL_PAINT_COLOR) {
        color = style.fill->content.color.type == SERVAL_CURRENT_COLOR
                    ? style.color
                    : style.fill->content.color.color;
      } else if (style.fill->type == SERVAL_PAINT_NONE) {
        color = 0;
      }
    }
    const auto alpha = static_cast<uint32_t>(
        std::round(((color >> 24) & 0xFF) * style.fill_opacity * opacity_));
    return {(color & 0x00FFFFFF) | (alpha << 24), style.font_size};
  }

  void AddRun(const std::string& text, const SrTextStyle& style) {
    factory_.PushTextStyle(style);
    factory_.AddText(text);
    factory_.PopTextStyle();
  }

  canvas::ParagraphFactory& factory_;
  const float opacity_;
  std::optional<SrTextStyle> pending_space_;
  bool has_text_{false};
};

void SrSVGRawText::AppendToParagraph(
    SrSVGTextParagraph& paragraph, const SrSVGRenderContext& context,
    const SrSVGTextStyle& inherited_style) const {
  paragraph.AddText(text_, inherited_style);
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

void SrSVGTextContainer::AppendToParagraph(
    SrSVGTextParagraph& paragraph, const SrSVGRenderContext& context,
    const SrSVGTextStyle& inherited_style) const {
  SrSVGTextStyle style = inherited_style;
  if (ResolveTextStyle(context, style)) {
    AppendChildrenToParagraph(paragraph, context, style);
  }
}

bool SrSVGTextContainer::ResolveTextStyle(const SrSVGRenderContext& context,
                                          SrSVGTextStyle& style) const {
  if (color_) {
    style.color = color_->color;
  }
  if (fill_) {
    style.fill = fill_;
  } else if (inherit_fill_paint_) {
    style.fill = inherit_fill_paint_;
  }
  if (fill_opacity_) {
    style.fill_opacity = ClampOpacity(*fill_opacity_);
  }
  style.preserve_space = preserve_xml_space_.value_or(style.preserve_space);
  if (font_size_) {
    auto font_context = context;
    font_context.font_size = style.font_size;
    if (font_size_->unit == SR_SVG_UNITS_PERCENTAGE) {
      style.font_size = font_size_->value * font_context.font_size / 100.f;
    } else if (!resolve_serval_length(&*font_size_, &font_context,
                                      SR_SVG_LENGTH_TYPE_NUMERIC,
                                      &style.font_size)) {
      return false;
    }
  }
  return std::isfinite(style.font_size);
}

void SrSVGTextContainer::AppendChildrenToParagraph(
    SrSVGTextParagraph& paragraph, const SrSVGRenderContext& context,
    const SrSVGTextStyle& style) const {
  for (const auto* child : children_) {
    child->AppendToParagraph(paragraph, context, style);
  }
}

void SrSVGText::OnRender(canvas::SrCanvas* canvas,
                         SrSVGRenderContext& context) {
  const float opacity = ClampOpacity(opacity_.value_or(1.f));
  if (opacity <= 0.f) {
    return;
  }
  float xform[6];
  ResolvedTransform(xform, context, canvas->PathFactory());
  canvas->Transform(xform);
  SrSVGTextStyle style;
  style.color = ResolveEffectiveColor(*this, context);
  style.fill_opacity = ClampOpacity(inherit_fill_opacity_.value_or(1.f));
  style.preserve_space = PreserveInheritedSpace(authored_parent_);
  if (!ResolveTextStyle(context, style)) {
    return;
  }
  auto position_context = context;
  position_context.font_size = style.font_size;
  float x, y;
  if (!resolve_serval_length(&x_, &position_context,
                             SR_SVG_LENGTH_TYPE_HORIZONTAL, &x) ||
      !resolve_serval_length(&y_, &position_context,
                             SR_SVG_LENGTH_TYPE_VERTICAL, &y)) {
    return;
  }
  const auto paragraph_builder = canvas::CreateParagraphFactoryFactory(canvas);
  if (!paragraph_builder)
    return;
  const float* fallback_opacity = context.marker_fallback_opacity;
  SrSVGTextParagraph text(*paragraph_builder,
                          fallback_opacity ? *fallback_opacity : 1.f);
  AppendChildrenToParagraph(text, context, style);
  if (!text.HasText()) {
    return;
  }

  SrParagraphStyle paragraph_style;
  paragraph_style.text_anchor = text_anchor_;
  paragraph_builder->SetParagraphStyle(std::move(paragraph_style));

  const auto paragraph = paragraph_builder->CreateParagraph();
  if (!paragraph) {
    return;
  }
  paragraph->Layout(std::numeric_limits<float>::max());
  parser::SrSVGOffscreenScope opacity_budget(context);
  const bool has_opacity_layer = opacity < 1.f &&
                                 opacity_budget.Reserve(*canvas, 1) &&
                                 canvas->TryBeginOpacityLayer(nullptr, opacity);
  paragraph->Draw(canvas, x, y);
  if (has_opacity_layer) {
    canvas->EndOpacityLayer();
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
