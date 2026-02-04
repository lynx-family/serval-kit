// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_MOCK_RUN_DELEGATE_H_
#define MARKDOWN_TESTING_MARKDOWN_MOCK_RUN_DELEGATE_H_
#include <string>

#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx::markdown::testing {
enum class MockDelegateType {
  kImage,
  kInlineView,
  kGradient,
};
class MockDelegate : public tttext::RunDelegate {
 public:
  explicit MockDelegate(MockDelegateType type) : type_(type) {}
  ~MockDelegate() override = default;
  MockDelegateType type_;
};
class MockImage : public MockDelegate {
 public:
  MockImage(const char* src, float desire_width, float desire_height,
            float max_width, float max_height, float radius)
      : MockDelegate(MockDelegateType::kImage), src_(src), radius_(radius) {
    float img_w = max_width * 0.1;
    float img_h = 20;
    if (desire_width > 0 && desire_height > 0) {
      width_ = desire_width;
      height_ = desire_height;
    } else if (desire_width > 0) {
      width_ = desire_width;
      height_ = desire_width * img_h / img_w;
    } else if (desire_height > 0) {
      height_ = desire_height;
      width_ = desire_height * img_w / img_h;
    } else {
      width_ = img_w;
      height_ = img_h;
    }
  }
  ~MockImage() override = default;

  float GetAscent() const override { return -height_; }
  float GetDescent() const override { return 0; }
  float GetAdvance() const override { return width_; }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

  std::string src_;
  float width_;
  float height_;
  float radius_{0};
};
class MockInlineView : public MockDelegate {
 public:
  MockInlineView(const char* id_selector, float max_width, float max_height)
      : MockDelegate(MockDelegateType::kInlineView),
        id_(id_selector),
        width_(max_width * 0.2),
        height_(30) {}
  ~MockInlineView() override = default;

  float GetAscent() const override { return -height_; }
  float GetDescent() const override { return 0; }
  float GetAdvance() const override { return width_; }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

  std::string id_;
  float width_;
  float height_;
};
class MockGradient : public MockDelegate {
 public:
  explicit MockGradient(const char* gradient)
      : MockDelegate(MockDelegateType::kGradient), gradient_(gradient) {}
  float GetAscent() const override { return 0; }
  float GetDescent() const override { return 0; }
  float GetAdvance() const override { return 0; }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {}
  std::string gradient_;
};
}  // namespace lynx::markdown::testing
#endif  // MARKDOWN_TESTING_MARKDOWN_MOCK_RUN_DELEGATE_H_
