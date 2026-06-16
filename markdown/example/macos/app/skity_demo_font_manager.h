// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_EXAMPLE_MACOS_APP_SKITY_DEMO_FONT_MANAGER_H_
#define MARKDOWN_EXAMPLE_MACOS_APP_SKITY_DEMO_FONT_MANAGER_H_

#include <textra/i_font_manager.h>
#include <textra/platform/skity/skity_typeface_helper.h>

#include <memory>
#include <skity/text/font_manager.hpp>
#include <skity/text/typeface.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

namespace serval::markdown::example {

class SkityDemoFontManager final : public tttext::IFontManager {
 public:
  SkityDemoFontManager() {
    std::vector<std::pair<tttext::FontDescriptor, const char*>> fonts = {
        {tttext::FontDescriptor{{"sans-serif"}, tttext::FontStyle::Normal(), 0},
         FONT_ROOT "Noto_Sans_CJK/NotoSansCJK-Regular.ttc"},
        {tttext::FontDescriptor{{"Inter"}, tttext::FontStyle::Normal(), 0},
         FONT_ROOT "Inter/Inter-VariableFont_opsz,wght.ttf"},
        {tttext::FontDescriptor{{"NotoSansCJK"}, tttext::FontStyle::Normal(),
                                0},
         FONT_ROOT "Noto_Sans_CJK/NotoSansCJK-Regular.ttc"},
        {tttext::FontDescriptor{{"NotoSansSC"}, tttext::FontStyle::Normal(), 0},
         FONT_ROOT "Noto_Sans_SC/NotoSansSC-VariableFont_wght.ttf"},
        {tttext::FontDescriptor{{"Menlo"}, tttext::FontStyle::Normal(), 0},
         FONT_ROOT "JetBrains_Mono/JetBrainsMono-VariableFont_wght.ttf"},
        {tttext::FontDescriptor{{"NotoColorEmoji"},
                                tttext::FontStyle::Normal(), 0},
         FONT_ROOT "Noto_Color_Emoji/NotoColorEmoji-Regular.ttf"},
        {tttext::FontDescriptor{{"NotoKufiArabic"},
                                tttext::FontStyle::Normal(), 0},
         FONT_ROOT "Noto_Kufi_Arabic/NotoKufiArabic-VariableFont_wght.ttf"},
        {tttext::FontDescriptor{{"NotoSansHebrew"},
                                tttext::FontStyle::Normal(), 0},
         FONT_ROOT "Noto_Sans_Hebrew/NotoSansHebrew-VariableFont_wdth,wght.ttf"},
        {tttext::FontDescriptor{{"NotoSansThai"},
                                tttext::FontStyle::Normal(), 0},
         FONT_ROOT "Noto_Sans_Thai/NotoSansThai-VariableFont_wdth,wght.ttf"},
    };

    for (auto& item : fonts) {
      auto typeface = MakeTypeface(item.second, 0);
      if (typeface != nullptr) {
        font_map_[item.first] = std::move(typeface);
      }
    }
  }

  ~SkityDemoFontManager() override = default;

  int countFamilies() const override {
    return static_cast<int>(font_map_.size());
  }

  ::TypefaceRef matchFamilyStyle(const char family_name[],
                                 const tttext::FontStyle& style) override {
    (void)style;
    if (family_name == nullptr) {
      return MatchByFamily("sans-serif");
    }
    return MatchByFamily(family_name);
  }

  ::TypefaceRef matchFamilyStyleCharacter(
      const char family_name[], const tttext::FontStyle& style,
      const char* bcp47[], int bcp47_count, uint32_t character) override {
    (void)family_name;
    (void)style;
    (void)bcp47;
    (void)bcp47_count;
    for (const auto& item : font_map_) {
      if (item.second != nullptr &&
          item.second->UnicharToGlyph(character, 0) != 0) {
        return item.second;
      }
    }
    return nullptr;
  }

  ::TypefaceRef makeFromFile(const char path[], int ttc_index) override {
    return MakeTypeface(path, ttc_index);
  }

  ::TypefaceRef legacyMakeTypeface(
      const char family_name[], tttext::FontStyle style) const override {
    (void)style;
    return MatchByFamily(family_name == nullptr ? "sans-serif" : family_name);
  }

  ::TypefaceRef createTypefaceFromPlatformFont(const void* platform_font)
      override {
    if (platform_font == nullptr) {
      return nullptr;
    }
    return const_cast<tttext::ITypefaceHelper*>(
               reinterpret_cast<const tttext::ITypefaceHelper*>(platform_font))
        ->shared_from_this();
  }

  void* getPlatformFontFromTypeface(::TypefaceRef typeface) override {
    return typeface.get();
  }

  void* GetPlatformFont(const char* family_name,
                        tttext::FontStyle style = tttext::FontStyle::Normal()) {
    auto typeface = matchFamilyStyle(family_name, style);
    return typeface == nullptr ? nullptr : typeface.get();
  }

 private:
  ::TypefaceRef MatchByFamily(const char* family_name) const {
    tttext::FontDescriptor desc{{family_name}, tttext::FontStyle::Normal(), 0};
    const auto iter = font_map_.find(desc);
    return iter == font_map_.end() ? nullptr : iter->second;
  }

  static ::TypefaceRef MakeTypeface(const char* path, int ttc_index) {
    (void)ttc_index;
    auto skity_typeface = skity::Typeface::MakeFromFile(path);
    if (skity_typeface == nullptr) {
      return nullptr;
    }
    return std::make_shared<skity::textlayout::SkityTypefaceHelper>(
        std::move(skity_typeface));
  }

 private:
  std::unordered_map<tttext::FontDescriptor, ::TypefaceRef,
                     tttext::FontDescriptor::Hasher>
      font_map_;
};

}  // namespace serval::markdown::example

#endif  // MARKDOWN_EXAMPLE_MACOS_APP_SKITY_DEMO_FONT_MANAGER_H_
