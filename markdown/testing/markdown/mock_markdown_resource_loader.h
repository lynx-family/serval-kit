// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_RESOURCE_LOADER_H_
#define MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_RESOURCE_LOADER_H_
#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/string/string_utils.h"
#include "markdown/parser/markdown_resource_loader.h"
#include "testing/markdown/mock_run_delegate.h"
namespace lynx {
namespace markdown {
namespace testing {

class MockMarkdownResourceLoader : public MarkdownResourceLoader {
 public:
  std::unique_ptr<tttext::RunDelegate> LoadImage(
      const char* src, float desire_width, float desire_height, float max_width,
      float max_height, float radius) override {
    if (base::StringEqual(src, "invalid")) {
      return nullptr;
    }
    return std::make_unique<MockImage>(src, desire_width, desire_height,
                                       max_width, max_height, radius);
  }
  std::unique_ptr<tttext::RunDelegate> LoadInlineView(
      const char* id_selector, float max_width, float max_height) override {
    return std::make_unique<MockInlineView>(id_selector, max_width, max_height);
  }
  std::unique_ptr<tttext::RunDelegate> LoadReplacementView(
      void* ud, int32_t id, float max_width, float max_height) override {
    return nullptr;
  }
  void* LoadFont(const char* family, MarkdownFontWeight weight) override {
    std::string font = family;
    if (weight > MarkdownFontWeight::kBold) {
      font += "_" + std::to_string(static_cast<int32_t>(weight)) + "_";
    }
    const auto find = font_cache_.find(font);
    if (find == font_cache_.end()) {
      const auto index = font_cache_.size();
      font_cache_[font] = index;
      family_cache_[index] = font;
      return reinterpret_cast<void*>(index);
    } else {
      return reinterpret_cast<void*>(find->second);
    }
  }
  std::unique_ptr<tttext::RunDelegate> LoadGradient(
      const char* gradient, float font_size, float root_font_size) override {
    return std::make_unique<MockGradient>(gradient);
  }

  std::unordered_map<std::string, size_t> font_cache_;
  std::unordered_map<size_t, std::string> family_cache_;
};
}  // namespace testing
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_RESOURCE_LOADER_H_
