// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_HARMONY_MARKDOWN_MEASURER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_HARMONY_MARKDOWN_MEASURER_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/platform/harmony/harmony_resource_loader.h"
#include "markdown/utils/markdown_value.h"
#include "markdown/view/markdown_view.h"

namespace serval::markdown {

class NativeServalMarkdownView;

class NativeMarkdownMeasurer : public MarkdownResourceLoader,
                               public MarkdownViewMeasureHost {
 public:
  NativeMarkdownMeasurer();
  ~NativeMarkdownMeasurer() override;

  MarkdownView* GetMarkdownView() const { return view_.get(); }
  void SetContent(const std::string& content) const;
  void SetStyle(const ValueMap& style) const;
  void MarkDirty() const;
  void SetConfig(const ValueMap& config) const;
  MeasureResult Measure(MeasureSpec spec);

  bool BindView(NativeServalMarkdownView* view);
  void DetachView(NativeServalMarkdownView* view);
  void SetResourceLoader(IHarmonyResourceLoader* loader);
  void SetRequestMeasureCallback(std::function<void()> callback);
  void RequestMeasure() override;

  void* LoadFont(const char* family, MarkdownFontWeight weight) override;
  std::shared_ptr<MarkdownDrawable> LoadInlineView(const char* id_selector,
                                                   float max_width,
                                                   float max_height) override;
  std::shared_ptr<MarkdownDrawable> LoadImage(const char* src,
                                              float desire_width,
                                              float desire_height,
                                              float max_width, float max_height,
                                              float border_radius) override;
  MarkdownReplacementView LoadReplacementView(void* ud, int32_t id,
                                              float max_width,
                                              float max_height) override;

 private:
  std::shared_ptr<MarkdownPlatformView> WrapEtsView(ArkUI_NodeHandle handle);

  std::shared_ptr<MarkdownView> view_;
  NativeServalMarkdownView* bound_view_{nullptr};
  IHarmonyResourceLoader* loader_{nullptr};
  std::function<void()> request_measure_callback_;
  std::unordered_map<ArkUI_NodeHandle, std::shared_ptr<MarkdownPlatformView>>
      view_cache_;
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_HARMONY_MARKDOWN_MEASURER_H_
