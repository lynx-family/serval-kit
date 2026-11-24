// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/sr_harmony_paragraph.h"

#include <cmath>
#include <utility>

#include "platform/harmony/sr_harmony_canvas.h"

namespace serval {
namespace svg {
namespace canvas {

namespace {

// SVG text positions use an anchor and an alphabetic baseline, while native
// paragraph painters use the top-left corner of the laid-out paragraph.
std::pair<float, float> ResolveDrawOrigin(float x, float y, float width, float baseline, SrTextAnchor anchor) {
    if (anchor == SR_SVG_TEXT_ANCHOR_MIDDLE) {
        x -= width / 2.f;
    } else if (anchor == SR_SVG_TEXT_ANCHOR_END) {
        x -= width;
    }
    return {x, y - baseline};
}

}  // namespace

std::unique_ptr<canvas::ParagraphFactory> CreateParagraphFactoryFactory(const SrCanvas *srCanvas) {
    return std::make_unique<SrHarmonyParagraphFactory>(srCanvas);
}

SrHarmonyParagraph::SrHarmonyParagraph(OH_Drawing_Typography *typography,
                                       std::shared_ptr<OH_Drawing_FontCollection> fonts, SrParagraphStyle style)
    : fonts_(std::move(fonts)), typography_(typography, OH_Drawing_DestroyTypography), style_(style) {}

void SrHarmonyParagraph::Layout(float max_width) {
    if (typography_ && std::isfinite(max_width) && max_width > 0.f) {
        OH_Drawing_TypographyLayout(typography_.get(), max_width);
    }
}

void SrHarmonyParagraph::Draw(SrCanvas *canvas, float x, float y) {
    if (!typography_ || !canvas) {
        return;
    }
    auto *harmony_canvas = static_cast<harmony::SrHarmonyCanvas *>(canvas);
    const auto origin =
        ResolveDrawOrigin(x, y, OH_Drawing_TypographyGetLongestLine(typography_.get()),
                          OH_Drawing_TypographyGetAlphabeticBaseline(typography_.get()), style_.text_anchor);
    OH_Drawing_TypographyPaint(typography_.get(), harmony_canvas->Context(), origin.first, origin.second);
}

SrHarmonyParagraphFactory::SrHarmonyParagraphFactory(const SrCanvas *sr_canvas)
    : fonts_(OH_Drawing_CreateSharedFontCollection(), OH_Drawing_DestroyFontCollection),
      typography_style_(OH_Drawing_CreateTypographyStyle(), OH_Drawing_DestroyTypographyStyle) {
    if (typography_style_) {
        OH_Drawing_SetTypographyTextDirection(typography_style_.get(), TEXT_DIRECTION_LTR);
        OH_Drawing_SetTypographyTextAlign(typography_style_.get(), TEXT_ALIGN_LEFT);
    }
    Reset();
}

std::unique_ptr<Paragraph> SrHarmonyParagraphFactory::CreateParagraph() {
    if (!handler_) {
        return nullptr;
    }
    auto *typography = OH_Drawing_CreateTypography(handler_.get());
    if (!typography) {
        return nullptr;
    }
    return std::make_unique<SrHarmonyParagraph>(typography, fonts_, paragraph_style_);
}

void SrHarmonyParagraphFactory::PushTextStyle(const SrTextStyle &style) {
    if (!handler_) {
        return;
    }
    std::unique_ptr<OH_Drawing_TextStyle, decltype(&OH_Drawing_DestroyTextStyle)> text_style(
        OH_Drawing_CreateTextStyle(), OH_Drawing_DestroyTextStyle);
    if (!text_style) {
        return;
    }
    OH_Drawing_SetTextStyleColor(text_style.get(), style.color);
    OH_Drawing_SetTextStyleFontSize(text_style.get(), style.font_size);
    OH_Drawing_SetTextStyleBaseLine(text_style.get(), TEXT_BASELINE_ALPHABETIC);
    OH_Drawing_TypographyHandlerPushTextStyle(handler_.get(), text_style.get());
    ++style_depth_;
}

void SrHarmonyParagraphFactory::PopTextStyle() {
    if (handler_ && style_depth_ > 1) {
        OH_Drawing_TypographyHandlerPopTextStyle(handler_.get());
        --style_depth_;
    }
}

void SrHarmonyParagraphFactory::SetParagraphStyle(SrParagraphStyle &&style) { paragraph_style_ = std::move(style); }

void SrHarmonyParagraphFactory::AddText(const std::string &text) {
    if (handler_ && !text.empty()) {
        OH_Drawing_TypographyHandlerAddText(handler_.get(), text.c_str());
    }
}

void SrHarmonyParagraphFactory::Reset() {
    handler_.reset();
    style_depth_ = 0;
    paragraph_style_ = {SR_SVG_TEXT_ANCHOR_START};
    if (typography_style_ && fonts_) {
        handler_.reset(OH_Drawing_CreateTypographyHandler(typography_style_.get(), fonts_.get()));
        PushTextStyle({NSVG_RGBA(0, 0, 0, 255), 14.f});
    }
}

}  // namespace canvas
}  // namespace svg
}  // namespace serval
