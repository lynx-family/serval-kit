// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_CONTEXT_H_
#define MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_CONTEXT_H_

#include <cstdint>
#include <memory>
#include <utility>

#include "markdown/utils/markdown_platform.h"

namespace serval::markdown {

class MarkdownContext {
 public:
  enum class HexColorFormat : uint8_t {
    kRGBA,
    kARGB,
  };

  explicit MarkdownContext(std::unique_ptr<MarkdownPlatform> platform)
      : platform_(std::move(platform)) {}

  void SetHashHexColorFormat(HexColorFormat format) {
    hash_hex_color_format_ = format;
  }
  HexColorFormat GetHashHexColorFormat() const {
    return hash_hex_color_format_;
  }

  void SetHarmonyShaperForceLowAPI(bool force_low_api) {
    harmony_shaper_force_low_api_ = force_low_api;
  }
  bool IsHarmonyShaperForceLowAPI() const {
    return harmony_shaper_force_low_api_;
  }

  tttext::TextLayout* GetTextLayout() const {
    return platform_ == nullptr ? nullptr : platform_->GetTextLayout();
  }

  MarkdownCanvasExtend* GetMarkdownCanvasExtend(
      tttext::ICanvasHelper* canvas) const {
    return platform_ == nullptr ? nullptr
                                : platform_->GetMarkdownCanvasExtend(canvas);
  }

 private:
  HexColorFormat hash_hex_color_format_{HexColorFormat::kRGBA};
  bool harmony_shaper_force_low_api_{true};
  std::unique_ptr<MarkdownPlatform> platform_;
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_CONTEXT_H_
