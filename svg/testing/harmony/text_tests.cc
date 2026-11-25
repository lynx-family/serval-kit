// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <hilog/log.h>
#include <native_drawing/drawing_bitmap.h>
#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_font_collection.h>
#include <native_drawing/drawing_text_typography.h>

#include "parser/SrSVGDOM.h"
#include "platform/harmony/sr_harmony_canvas.h"
#include "platform/harmony/sr_harmony_paragraph.h"

namespace {

constexpr int kWidth = 400;
constexpr int kHeight = 180;
std::string report;

struct Surface {
  OH_Drawing_Bitmap* bitmap = OH_Drawing_BitmapCreate();
  OH_Drawing_Canvas* canvas = OH_Drawing_CanvasCreate();
  Surface() {
    const OH_Drawing_BitmapFormat format{COLOR_FORMAT_RGBA_8888,
                                         ALPHA_FORMAT_PREMUL};
    OH_Drawing_BitmapBuild(bitmap, kWidth, kHeight, &format);
    OH_Drawing_CanvasBind(canvas, bitmap);
    OH_Drawing_CanvasClear(canvas, 0);
  }
  ~Surface() {
    OH_Drawing_CanvasDestroy(canvas);
    OH_Drawing_BitmapDestroy(bitmap);
  }
  const uint8_t* pixels() const {
    return static_cast<const uint8_t*>(OH_Drawing_BitmapGetPixels(bitmap));
  }
};

struct Run {
  const char* text;
  float size;
  uint32_t color;
};

// Build the expected image directly with the SDK, independent of Serval's
// paragraph factory, SVG inheritance, and anchor/baseline conversion.
void PaintReference(Surface& surface, const std::vector<Run>& runs, float x,
                    float y, int anchor = 0) {
  auto* fonts = OH_Drawing_CreateSharedFontCollection();
  auto* style = OH_Drawing_CreateTypographyStyle();
  OH_Drawing_SetTypographyTextDirection(style, TEXT_DIRECTION_LTR);
  OH_Drawing_SetTypographyTextAlign(style, TEXT_ALIGN_LEFT);
  auto* handler = OH_Drawing_CreateTypographyHandler(style, fonts);
  for (const auto& run : runs) {
    auto* text_style = OH_Drawing_CreateTextStyle();
    OH_Drawing_SetTextStyleFontSize(text_style, run.size);
    OH_Drawing_SetTextStyleColor(text_style, run.color);
    OH_Drawing_SetTextStyleBaseLine(text_style, TEXT_BASELINE_ALPHABETIC);
    OH_Drawing_TypographyHandlerPushTextStyle(handler, text_style);
    OH_Drawing_TypographyHandlerAddText(handler, run.text);
    OH_Drawing_TypographyHandlerPopTextStyle(handler);
    OH_Drawing_DestroyTextStyle(text_style);
  }
  auto* typography = OH_Drawing_CreateTypography(handler);
  OH_Drawing_TypographyLayout(typography, std::numeric_limits<float>::max());
  const double width = OH_Drawing_TypographyGetLongestLine(typography);
  const double baseline =
      OH_Drawing_TypographyGetAlphabeticBaseline(typography);
  OH_Drawing_TypographyPaint(typography, surface.canvas,
                             x - width * anchor / 2.0, y - baseline);
  OH_Drawing_DestroyTypography(typography);
  OH_Drawing_DestroyTypographyHandler(handler);
  OH_Drawing_DestroyTypographyStyle(style);
  OH_Drawing_DestroyFontCollection(fonts);
}

bool RenderSVG(Surface& surface, const std::string& body,
               uint32_t host_color = 0xFF000000) {
  const std::string svg =
      "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 400 180\">" +
      body + "</svg>";
  std::vector<serval::svg::parser::SrSVGDiagnostic> diagnostics;
  auto dom =
      serval::svg::parser::SrSVGDOM::make(svg.data(), svg.size(), &diagnostics);
  if (!dom) {
    return false;
  }
  dom->SetDefaultColor(host_color);
  serval::svg::harmony::SrHarmonyCanvas canvas(surface.canvas);
  dom->Render(&canvas, {0.f, 0.f, kWidth, kHeight});
  return true;
}

bool SamePixels(const char* name, const Surface& actual,
                const Surface& expected, bool expect_visible = true) {
  int different = 0;
  int visible = 0;
  for (int i = 0; i < kWidth * kHeight * 4; ++i) {
    if (std::abs(int(actual.pixels()[i]) - int(expected.pixels()[i])) > 2) {
      ++different;
    }
    if (i % 4 == 3 && actual.pixels()[i] != 0) {
      ++visible;
    }
  }
  const bool passed =
      different == 0 && (expect_visible ? visible > 0 : visible == 0);
  std::printf("%s %s: different_channels=%d visible_pixels=%d\n",
              passed ? "PASS" : "FAIL", name, different, visible);
  OH_LOG_Print(LOG_APP, LOG_INFO, 0, "SVGTEXT",
               "%{public}s %{public}s: different_channels=%{public}d "
               "visible_pixels=%{public}d",
               passed ? "PASS" : "FAIL", name, different, visible);
  report += std::string(passed ? "PASS " : "FAIL ") + name +
            ": different_channels=" + std::to_string(different) +
            " visible_pixels=" + std::to_string(visible) + "\n";
  return passed;
}

bool Check(const char* name, const std::string& body,
           const std::vector<Run>& runs, int anchor = 0,
           uint32_t host_color = 0xFF000000) {
  Surface actual;
  Surface expected;
  if (!RenderSVG(actual, body, host_color)) {
    std::printf("FAIL %s: SVG parse failed\n", name);
    return false;
  }
  PaintReference(expected, runs, 200.f, 90.f, anchor);
  return SamePixels(name, actual, expected);
}

bool FactoryLifetimeAndReset() {
  Surface actual;
  Surface expected;
  serval::svg::harmony::SrHarmonyCanvas canvas(actual.canvas);
  std::unique_ptr<serval::svg::canvas::Paragraph> first;
  std::unique_ptr<serval::svg::canvas::Paragraph> second;
  {
    serval::svg::canvas::SrHarmonyParagraphFactory factory(&canvas);
    factory.PushTextStyle({0xFF0000FF, 40.f});
    factory.AddText("discarded");
    factory.Reset();
    factory.PopTextStyle();  // Must not remove the default style.
    factory.AddText("Reset");
    first = factory.CreateParagraph();
    second = factory.CreateParagraph();
  }
  if (!first || !second) {
    return false;
  }
  first->Layout(std::numeric_limits<float>::max());
  first->Draw(&canvas, 200.f, 90.f);
  PaintReference(expected, {{"Reset", 14.f, 0xFF000000}}, 200.f, 90.f);
  return SamePixels("factory lifetime/reset", actual, expected);
}

}  // namespace

int RunHarmonyTextTests() {
  report.clear();
  bool passed = true;
  for (int anchor = 0; anchor < 3; ++anchor) {
    const char* names[] = {"start", "middle", "end"};
    const std::string body =
        "<text x=\"200\" y=\"90\" font-size=\"28\" text-anchor=\"" +
        std::string(names[anchor]) + "\">Hello 中文 SVG</text>";
    passed &= Check(names[anchor], body, {{"Hello 中文 SVG", 28.f, 0xFF000000}},
                    anchor);
  }
  passed &= Check("banana alphabetic baseline",
                  "<text x='200' y='90' font-size='20' text-anchor='middle'>"
                  "🍌</text>",
                  {{"🍌", 20.f, 0xFF000000}}, 1);
  passed &= Check("monkey alphabetic baseline",
                  "<text x='200' y='90' font-size='20' text-anchor='middle'>"
                  "🐒</text>",
                  {{"🐒", 20.f, 0xFF000000}}, 1);
  passed &= Check("nested tspan inheritance",
                  "<text x=\"200\" y=\"90\" font-size=\"28\" fill=\"#ff0000\">A"
                  "<tspan fill=\"#00ff00\">B<tspan "
                  "font-size=\"40\">C</tspan>D</tspan>E</text>",
                  {{"A", 28.f, 0xFFFF0000},
                   {"B", 28.f, 0xFF00FF00},
                   {"C", 40.f, 0xFF00FF00},
                   {"D", 28.f, 0xFF00FF00},
                   {"E", 28.f, 0xFFFF0000}});
  passed &=
      Check("relative span font sizes",
            "<text x=\"200\" y=\"90\" font-size=\"28\">A"
            "<tspan font-size=\"150%\">B<tspan font-size=\"0.5em\">C</tspan>"
            "D</tspan>E</text>",
            {{"A", 28.f, 0xFF000000},
             {"B", 42.f, 0xFF000000},
             {"C", 21.f, 0xFF000000},
             {"D", 42.f, 0xFF000000},
             {"E", 28.f, 0xFF000000}});
  passed &=
      Check("currentColor in nested spans",
            "<text x=\"200\" y=\"90\" font-size=\"28\" fill=\"currentColor\">A"
            "<tspan color=\"#0000ff\">B</tspan>C</text>",
            {{"A", 28.f, 0xFF008800},
             {"B", 28.f, 0xFF0000FF},
             {"C", 28.f, 0xFF008800}},
            0, 0xFF008800);
  passed &=
      Check("currentColor on the color property",
            "<g color='#008800'><text x='200' y='90' color='currentColor' "
            "fill='currentColor' font-size='28'>A"
            "<tspan color='currentColor'>B</tspan>"
            "</text></g>",
            {{"A", 28.f, 0xFF008800}, {"B", 28.f, 0xFF008800}});
  passed &= Check("absolute span size under zero size text",
                  "<text x='200' y='90' font-size='0'>Hidden"
                  "<tspan font-size='28'>Visible</tspan>"
                  "<tspan font-size='150%'>Hidden</tspan></text>",
                  {{"Visible", 28.f, 0xFF000000}});
  passed &= Check("default whitespace across spans",
                  "<text x='200' y='90' text-anchor='middle' font-size='28'>"
                  "\n  A\t B\n  <tspan> C</tspan>\n</text>",
                  {{"A B", 28.f, 0xFF000000}, {" C", 28.f, 0xFF000000}}, 1);
  passed &= Check("fill-opacity inheritance",
                  "<g fill=\"#ff0000\" fill-opacity=\"0.5\">"
                  "<text x=\"200\" y=\"90\" font-size=\"28\">A<tspan>B</tspan>"
                  "<tspan fill-opacity=\"1\">C</tspan></text></g>",
                  {{"A", 28.f, 0x80FF0000},
                   {"B", 28.f, 0x80FF0000},
                   {"C", 28.f, 0xFFFF0000}});
  passed &= Check("inline style",
                  "<text x=\"200\" y=\"90\" "
                  "style=\"font-size:28px;fill:#0000ff\">Style</text>",
                  {{"Style", 28.f, 0xFF0000FF}});
  {
    Surface actual;
    Surface expected;
    passed &=
        RenderSVG(actual,
                  "<g opacity=\"0.5\"><text x=\"200\" y=\"90\" font-size=\"28\""
                  " opacity=\"0.5\" transform=\"translate(10 5) scale(0.75)\">"
                  "Opacity 中文</text></g>");
    OH_Drawing_CanvasTranslate(expected.canvas, 10.f, 5.f);
    OH_Drawing_CanvasScale(expected.canvas, 0.75f, 0.75f);
    PaintReference(expected, {{"Opacity 中文", 28.f, 0x40000000}}, 200.f, 90.f);
    passed &= SamePixels("transform and nested opacity", actual, expected);
  }
  {
    Surface actual;
    Surface expected;
    passed &=
        RenderSVG(actual,
                  "<text x=\"200\" y=\"90\" fill=\"none\">Hidden</text>"
                  "<text x=\"200\" y=\"90\" opacity=\"0\">Hidden</text>"
                  "<text x=\"200\" y=\"90\" font-size=\"0\">Hidden</text>");
    passed &= SamePixels("invisible text", actual, expected, false);
  }
  passed &= FactoryLifetimeAndReset();
  std::printf("TEXT_TESTS_RESULT=%s\n", passed ? "PASS" : "FAIL");
  report += std::string("TEXT_TESTS_RESULT=") + (passed ? "PASS\n" : "FAIL\n");
  return passed ? 0 : 1;
}

const char* HarmonyTextTestReport() {
  return report.c_str();
}

#ifndef SERVAL_TEXT_TESTS_EMBEDDED
int main() {
  return RunHarmonyTextTests();
}
#endif
