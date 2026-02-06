// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_SHAPER_H_
#define MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_SHAPER_H_
#include <memory>
#include "src/textlayout/tt_shaper.h"
namespace lynx {
namespace markdown {
namespace testing {
class MockMarkdownTypefaceHelper : public tttext::ITypefaceHelper {
 public:
  MockMarkdownTypefaceHelper() : tttext::ITypefaceHelper(0) {}
  ~MockMarkdownTypefaceHelper() override = default;
  float GetHorizontalAdvance(tttext::GlyphID glyph_id,
                             float font_size) const override {
    return 0;
  }
  void GetHorizontalAdvances(tttext::GlyphID* glyph_ids, uint32_t count,
                             float* widths, float font_size) const override {}
  void GetWidthBound(float* rect_f, tttext::GlyphID glyph_id,
                     float font_size) const override {}
  void GetWidthBounds(float* rect_ltrb, tttext::GlyphID glyphs[],
                      uint32_t glyph_count, float font_size) override{};
  const void* GetFontData() const override { return nullptr; }
  size_t GetFontDataSize() const override { return 0; }
  int GetFontIndex() const override { return 0; }
  uint32_t GetUnitsPerEm() const override { return 0; }
  uint16_t UnicharToGlyph(tttext::Unichar codepoint,
                          uint32_t variationSelector = 0) const override {
    return 0;
  }
  void UnicharsToGlyphs(const tttext::Unichar* unichars, uint32_t count,
                        tttext::GlyphID* glyphs) const override {}

  static TypefaceRef GetInstance() {
    static auto instance = std::make_shared<MockMarkdownTypefaceHelper>();
    return instance;
  }

 protected:
  void OnCreateFontInfo(tttext::FontInfo* info,
                        float font_size) const override {
    info->SetAscent(-font_size);
    info->SetDescent(0.1 * font_size);
    info->SetFontSize(font_size);
  }
};

class MockMarkdownShapingResultReader
    : public tttext::PlatformShapingResultReader {
 public:
  MockMarkdownShapingResultReader() = default;
  ~MockMarkdownShapingResultReader() override = default;
  uint32_t GlyphCount() const override { return text_.length(); }
  uint32_t TextCount() const override { return text_.length(); }
  tttext::GlyphID ReadGlyphID(uint32_t idx) const override {
    return text_[idx];
  }
  float ReadAdvanceX(uint32_t idx) const override { return font_size_; }
  uint32_t ReadIndices(uint32_t idx) const override { return idx; }
  TypefaceRef ReadFontId(uint32_t idx) const override {
    return MockMarkdownTypefaceHelper::GetInstance();
  }
  float font_size_;
  std::u32string text_;
};
class MockMarkdownShaper : public tttext::TTShaper {
 public:
  MockMarkdownShaper() : TTShaper(tttext::FontmgrCollection(nullptr)) {}
  ~MockMarkdownShaper() override = default;
  void OnShapeText(const tttext::ShapeKey& key,
                   tttext::ShapeResult* result) const override {
    MockMarkdownShapingResultReader reader;
    reader.text_ = key.text_;
    reader.font_size_ = key.style_.GetFontSize();
    result->AppendPlatformShapingResult(reader);
  }
};
}  // namespace testing
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_SHAPER_H_
