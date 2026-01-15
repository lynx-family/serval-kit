// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "markdown/platform/ios/serval_markdown_view.h"

#include "markdown/platform/ios/internal/markdown_custom_view_ios.h"
#include "markdown/platform/ios/internal/markdown_value_convert.h"
#include "markdown/view/markdown_selection_view.h"
#include "markdown/view/markdown_view.h"
@interface ServalMarkdownView ()
@property(nonatomic, strong) UIView* customViews;

- (MarkdownCustomViewImpl*)createCustomView;
- (void)removeSubview:(lynx::markdown::MarkdownPlatformView*)subview;
- (void)removeAllCustomViews;

- (lynx::markdown::MarkdownView*)getMarkdownView;
@end

namespace lynx::markdown {
class MarkdownMainViewIOS : public MarkdownCustomViewIOS,
                            public MarkdownMainViewHandle {
 public:
  MarkdownMainViewIOS(ServalMarkdownView* view) : MarkdownCustomViewIOS(view) {}
  ~MarkdownMainViewIOS() override = default;
  void SetFrameRate(int32_t frame_rate) override {}
  MarkdownPlatformView* CreateCustomSubView() override {
    MarkdownCustomViewImpl* view = [GetServalView() createCustomView];
    return view.markdownViewHandle;
  }
  void RemoveSubView(MarkdownPlatformView* subview) override {
    [GetServalView() removeSubview:subview];
  }
  void RemoveAllSubViews() override { [GetServalView() removeAllCustomViews]; }
  RectF GetViewRectInScreen() override { return {}; }
  ServalMarkdownView* GetServalView() { return (ServalMarkdownView*)(view_); }
  MarkdownMainViewHandle* GetMainViewHandle() override { return this; }
};

MarkdownPlatformView* MarkdownSelectionHandle::CreateView(
    MarkdownMainViewHandle* handle, SelectionHandleType type, float size,
    float margin, uint32_t color) {
  return nullptr;
}

MarkdownPlatformView* MarkdownSelectionHighlight::CreateView(
    MarkdownMainViewHandle* handle, uint32_t color) {
  return nullptr;
}

}  // namespace lynx::markdown

@implementation ServalMarkdownView
- (instancetype)init {
  self = [super init];
  if (self != nil) {
    self.customViews = [[UIView alloc] init];
    [self addSubview:self.customViews];
    self.markdownViewHandle->AttachDrawable(
        std::make_unique<lynx::markdown::MarkdownView>(
            self.markdownViewHandle));
  }
  return self;
}
- (void)setFrame:(CGRect)frame {
  [super setFrame:frame];
  [self.customViews
      setFrame:CGRectMake(0, 0, frame.size.width, frame.size.height)];
}
- (void)setBounds:(CGRect)bounds {
  [super setBounds:bounds];
  [self.customViews
      setBounds:CGRectMake(0, 0, bounds.size.width, bounds.size.height)];
}
- (void)initHandle {
  self.markdownViewHandle = new lynx::markdown::MarkdownMainViewIOS(self);
}
- (MarkdownCustomViewImpl*)createCustomView {
  MarkdownCustomViewImpl* view = [[MarkdownCustomViewImpl alloc] init];
  [self.customViews addSubview:view];
  return view;
}
- (void)removeSubview:(lynx::markdown::MarkdownPlatformView*)subview {
  auto* ios_view =
      static_cast<lynx::markdown::MarkdownPlatformViewIOS*>(subview);
  [ios_view->GetUIView() removeFromSuperview];
}
- (void)removeAllCustomViews {
  for (UIView* view in self.customViews.subviews) {
    [view removeFromSuperview];
  }
}
- (lynx::markdown::MarkdownView*)getMarkdownView {
  return static_cast<lynx::markdown::MarkdownView*>(
      self.markdownViewHandle->GetDrawable());
}
- (void)setContent:(NSString*)content {
  auto* view = [self getMarkdownView];
  auto* str = [content UTF8String];
  view->SetContent(str);
  _content = content;
}
- (void)setStyle:(NSDictionary*)style {
  auto* view = [self getMarkdownView];
  auto map = MarkdownValueConvert::ConvertMap(style);
  view->SetStyle(map->AsMap());
  _style = style;
}
@end
