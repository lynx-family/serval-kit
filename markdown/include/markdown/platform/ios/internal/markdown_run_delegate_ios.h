// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_RUN_DELEGATE_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_RUN_DELEGATE_IOS_H_

#import <ServalMarkdown/IMarkdownPlatformViewHandle.h>
#import <ServalMarkdown/IMarkdownResourceDelegate.h>
#import <UIKit/UIKit.h>

#include <cstdint>

#include "markdown/element/markdown_drawable.h"

NS_ASSUME_NONNULL_BEGIN

namespace serval::markdown {

enum class MarkdownRunDelegateType : uint8_t {
  kImage = 0,
  kView,
  kBackground,
  kFormula,
};

class MarkdownRunDelegate : public MarkdownDrawable {
 public:
  MarkdownRunDelegate(float desire_width, float desire_height,
                      MarkdownRunDelegateType type);
  ~MarkdownRunDelegate() override = default;
  MarkdownRunDelegateType GetMarkdownRunDelegateType() const {
    return delegate_type_;
  }

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;

  MarkdownRunDelegateType delegate_type_;
  float desire_width_;
  float desire_height_;
};

class MarkdownFormulaRunDelegate final : public MarkdownRunDelegate {
 public:
  explicit MarkdownFormulaRunDelegate(id<IMarkdownFormula> formula)
      : MarkdownRunDelegate(formula.advance, formula.descent - formula.ascent,
                            MarkdownRunDelegateType::kFormula),
        formula_(formula) {}
  void DrawFormula(CGContextRef context, CGRect rect) const {
    [formula_ drawInContext:context rect:rect];
  }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    return {desire_width_, desire_height_,
            static_cast<float>(-formula_.ascent)};
  }

 private:
  id<IMarkdownFormula> formula_;
};

class MarkdownImageRunDelegate : public MarkdownRunDelegate {
 public:
  MarkdownImageRunDelegate(UIImage* _Nullable image, float desire_width,
                           float desire_height, float max_width,
                           float max_height, float border_radius);
  MarkdownImageRunDelegate(UIImage* image,
                           id<IMarkdownResourceDelegate> resource_delegate,
                           float desire_width, float desire_height,
                           float max_width, float max_height,
                           float border_radius);
  ~MarkdownImageRunDelegate() override = default;
  UIImage* _Nullable GetImage() const { return image_; }
  UIImageView* _Nullable GetAnimatedImageView() const;
  float GetBorderRadius() const { return border_radius_; }

 private:
  MarkdownImageRunDelegate(CGSize image_size, float desire_width,
                           float desire_height, float max_width,
                           float max_height, float border_radius);

  UIImage* _Nullable image_{nil};
  __weak id<IMarkdownResourceDelegate> _Nullable resource_delegate_{nil};
  mutable UIImageView* _Nullable animated_image_view_{nil};
  float border_radius_;
};

class MarkdownInlineViewRunDelegate : public MarkdownRunDelegate {
 public:
  explicit MarkdownInlineViewRunDelegate(
      id<IMarkdownPlatformViewHandle> handle);
  ~MarkdownInlineViewRunDelegate() override = default;
  void Align(float x, float y) override;
  void SetBounds(RectF bounds) override;
  id<IMarkdownPlatformViewHandle> GetHandle() const { return handle_; }
  MarkdownVerticalAlign GetVerticalAlign() const override;
  float GetVerticalAlignLength() const override;

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;

 private:
  id<IMarkdownPlatformViewHandle> handle_;
  bool visible_ = true;
};

}  // namespace serval::markdown

NS_ASSUME_NONNULL_END

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_RUN_DELEGATE_IOS_H_
