// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_MAIN_VIEW_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_MAIN_VIEW_IOS_H_
#import <ServalMarkdown/ServalMarkdownView.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "markdown/platform/ios/internal/markdown_custom_view_ios.h"
namespace serval::markdown {
class MarkdownMainViewIOS : public MarkdownCustomViewIOS,
                            public MarkdownViewContainerHandle {
 public:
  explicit MarkdownMainViewIOS(ServalMarkdownView* view)
      : MarkdownCustomViewIOS(view) {}
  ~MarkdownMainViewIOS() override = default;

  void RequestMeasure() override;
  void RequestAlign() override;

  std::shared_ptr<MarkdownPlatformView> CreateCustomSubView() override;
  std::shared_ptr<MarkdownPlatformView> CreateRegionSubView() override;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHandleSubView(
      SelectionHandleType type, float size, float margin,
      uint32_t color) override;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHighlightSubView(
      uint32_t color) override;

  void RemoveSubView(MarkdownPlatformView* subview) override;

  void RemoveAllSubViews() override;

  RectF GetViewRectInScreen() override;
  void OnVSync(int64_t timestamp);

  MarkdownViewContainerHandle* GetViewContainerHandle() override {
    return static_cast<MarkdownViewContainerHandle*>(this);
  }

 private:
  RectF CalculateViewRectInScreen();

  ServalMarkdownView* GetServalView() {
    return static_cast<ServalMarkdownView*>(this->GetHandle());
  }

  std::vector<std::shared_ptr<MarkdownCustomViewIOS>> subviews_;
  RectF cached_view_rect_in_screen_{};
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_MAIN_VIEW_IOS_H_
