// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/ios/internal/markdown_main_view_ios.h"

#include <algorithm>
#include <memory>
#include <vector>

#import <UIKit/UIKit.h>

#import "markdown/platform/ios/MarkdownCustomDrawView.h"
#import "markdown/platform/ios/ServalMarkdownView.h"

#include "markdown/platform/ios/internal/markdown_custom_view_ios.h"
#include "markdown/view/markdown_selection_view.h"
#include "markdown/view/markdown_view.h"

@interface ServalMarkdownView (MainViewInternal)
- (MarkdownCustomDrawView*)createCustomView;
- (MarkdownCustomDrawView*)createRegionView;
- (void)removeSubview:(serval::markdown::MarkdownPlatformView*)subview;
- (void)removeAllCustomViews;
@end

namespace serval::markdown {
void MarkdownMainViewIOS::RequestMeasure() {
  MarkdownPlatformViewIOS::RequestMeasure();
}
void MarkdownMainViewIOS::RequestAlign() {
  MarkdownPlatformViewIOS::RequestAlign();
}
std::shared_ptr<MarkdownPlatformView>
MarkdownMainViewIOS::CreateCustomSubView() {
  MarkdownCustomDrawView* view = [GetServalView() createCustomView];
  auto subview = std::make_shared<MarkdownCustomViewIOS>(view);
  subviews_.push_back(subview);
  return std::static_pointer_cast<MarkdownPlatformView>(subview);
}

std::shared_ptr<MarkdownPlatformView>
MarkdownMainViewIOS::CreateRegionSubView() {
  MarkdownCustomDrawView* view = [GetServalView() createRegionView];
  auto subview = std::make_shared<MarkdownCustomViewIOS>(view);
  subviews_.push_back(subview);
  return std::static_pointer_cast<MarkdownPlatformView>(subview);
}

std::shared_ptr<MarkdownPlatformView>
MarkdownMainViewIOS::CreateSelectionHandleSubView(SelectionHandleType type,
                                                  float size, float margin,
                                                  uint32_t color) {
  const auto view = CreateCustomSubView();
  auto selection_handle =
      std::make_unique<MarkdownSelectionHandle>(size, margin, type, color);
  view->GetCustomViewHandle()->AttachDrawable(std::move(selection_handle));
  return view;
}

std::shared_ptr<MarkdownPlatformView>
MarkdownMainViewIOS::CreateSelectionHighlightSubView(uint32_t color) {
  const auto view = CreateCustomSubView();
  auto highlight = std::make_unique<MarkdownSelectionHighlight>();
  highlight->SetColor(color);
  view->GetCustomViewHandle()->AttachDrawable(std::move(highlight));
  return view;
}

void MarkdownMainViewIOS::RemoveSubView(MarkdownPlatformView* subview) {
  auto iter = std::find_if(
      subviews_.begin(), subviews_.end(),
      [subview](const std::shared_ptr<MarkdownCustomViewIOS>& view) {
        return static_cast<MarkdownPlatformView*>(view.get()) == subview;
      });
  if (iter == subviews_.end()) {
    return;
  }
  [GetServalView() removeSubview:subview];
  subviews_.erase(iter);
}

void MarkdownMainViewIOS::RemoveAllSubViews() {
  [GetServalView() removeAllCustomViews];
  subviews_.clear();
}

RectF MarkdownMainViewIOS::CalculateViewRectInScreen() {
  auto* view = GetServalView();
  if (view == nil) {
    return {};
  }
  if (view.window == nil) {
    return {};
  }
  CGRect rect_in_window = [view convertRect:view.bounds toView:nil];
  CGRect window_bounds = view.window.bounds;
  CGRect visible_in_window = CGRectIntersection(rect_in_window, window_bounds);
  if (CGRectIsNull(visible_in_window) || CGRectIsEmpty(visible_in_window)) {
    return {};
  }
  CGRect visible_in_view = [view convertRect:visible_in_window fromView:nil];
  const float left = static_cast<float>(visible_in_view.origin.x);
  const float top = static_cast<float>(visible_in_view.origin.y);
  const float right =
      static_cast<float>(visible_in_view.origin.x + visible_in_view.size.width);
  const float bottom = static_cast<float>(visible_in_view.origin.y +
                                          visible_in_view.size.height);
  return RectF::MakeLTRB(left, top, right, bottom);
}

void MarkdownMainViewIOS::OnVSync(int64_t timestamp) {
  cached_view_rect_in_screen_ = CalculateViewRectInScreen();
  auto* markdown_view = static_cast<MarkdownView*>(GetDrawable());
  if (markdown_view != nullptr) {
    markdown_view->OnNextFrame(timestamp);
  }
}

RectF MarkdownMainViewIOS::GetViewRectInScreen() {
  return cached_view_rect_in_screen_;
}

}  // namespace serval::markdown
