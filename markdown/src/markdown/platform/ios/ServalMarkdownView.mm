// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "markdown/platform/ios/ServalMarkdownView.h"
#import "markdown/platform/ios/MarkdownCustomDrawView.h"

#include <memory>
#include "markdown/platform/ios/internal/markdown_custom_view_ios.h"
#include "markdown/platform/ios/internal/markdown_event_ios.h"
#include "markdown/platform/ios/internal/markdown_exposure_ios.h"
#include "markdown/platform/ios/internal/markdown_main_view_ios.h"
#include "markdown/platform/ios/internal/markdown_resource_loader_ios.h"
#include "markdown/platform/ios/internal/markdown_value_convert.h"
#include "markdown/view/markdown_view.h"
@interface ServalMarkdownView () {
  std::unique_ptr<serval::markdown::MarkdownEventIOS> event_listener_;
  std::unique_ptr<serval::markdown::MarkdownExposureIOS> exposure_listener_;
  std::unique_ptr<serval::markdown::MarkdownResourceLoaderIOS> resource_loader_;
  std::unique_ptr<serval::markdown::MarkdownMainViewIOS> markdown_view_handle_;
}
@property(nonatomic, strong) CADisplayLink* displayLink;
@property(nonatomic, strong) NSMutableArray<UIView*>* customSubviews;

- (MarkdownCustomDrawView*)createCustomView;
- (MarkdownCustomDrawView*)createRegionView;
- (void)removeSubview:(serval::markdown::MarkdownPlatformView*)subview;
- (void)removeAllCustomViews;

- (serval::markdown::MarkdownView*)getMarkdownView;
@end

@implementation ServalMarkdownView
- (instancetype)init {
  self = [super init];
  if (self != nil) {
    self.customSubviews = [[NSMutableArray alloc] init];
    markdown_view_handle_ =
        std::make_unique<serval::markdown::MarkdownMainViewIOS>(self);
    markdown_view_handle_->AttachDrawable(
        std::make_unique<serval::markdown::MarkdownView>(
            markdown_view_handle_.get()));
    event_listener_ = std::make_unique<serval::markdown::MarkdownEventIOS>();
    resource_loader_ =
        std::make_unique<serval::markdown::MarkdownResourceLoaderIOS>();
    exposure_listener_ =
        std::make_unique<serval::markdown::MarkdownExposureIOS>();
    auto* view = [self getMarkdownView];
    view->SetEventListener(event_listener_.get());
    view->SetResourceLoader(resource_loader_.get());
    __unsafe_unretained id weakSelf = self;
    self.displayLink =
        [CADisplayLink displayLinkWithTarget:weakSelf
                                    selector:@selector(onVSync:)];
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                           forMode:NSRunLoopCommonModes];
  }
  return self;
}
- (void)dealloc {
  [self.displayLink invalidate];
  self.displayLink = nil;
  auto* view = [self getMarkdownView];
  if (view != nullptr) {
    view->SetExposureListener(nullptr);
    view->SetEventListener(nullptr);
    view->SetResourceLoader(nullptr);
  }
  exposure_listener_.reset();
  event_listener_.reset();
  resource_loader_.reset();
}
- (MarkdownCustomDrawView*)createCustomView {
  MarkdownCustomDrawView* view = [[MarkdownCustomDrawView alloc] init];
  [self addSubview:view];
  [self.customSubviews addObject:view];
  return view;
}
- (MarkdownCustomDrawView*)createRegionView {
  MarkdownCustomDrawView* view = [[MarkdownCustomDrawView alloc] init];
  [self insertSubview:view atIndex:0];
  [self.customSubviews addObject:view];
  return view;
}
- (void)removeSubview:(serval::markdown::MarkdownPlatformView*)subview {
  auto* ios_view =
      static_cast<serval::markdown::MarkdownPlatformViewIOS*>(subview);
  UIView* view = static_cast<UIView*>(ios_view->GetHandle());
  [self.customSubviews removeObject:view];
  [view removeFromSuperview];
}
- (void)removeAllCustomViews {
  for (UIView* view in self.customSubviews) {
    [view removeFromSuperview];
  }
  [self.customSubviews removeAllObjects];
}
- (void)onVSync:(CADisplayLink*)sender {
  if (markdown_view_handle_ == nullptr) {
    return;
  }
  const auto timestamp = static_cast<int64_t>(sender.targetTimestamp * 1000);
  markdown_view_handle_->OnVSync(timestamp);
}
- (serval::markdown::MarkdownView*)getMarkdownView {
  return static_cast<serval::markdown::MarkdownView*>(
      markdown_view_handle_->GetDrawable());
}

- (void)setResourceDelegate:(id<IMarkdownResourceDelegate>)resourceDelegate {
  _resourceDelegate = resourceDelegate;
  if (resource_loader_ != nullptr) {
    resource_loader_->SetDelegate(resourceDelegate);
  }
}

- (void)setEventDelegate:(id<IMarkdownEventDelegate>)eventDelegate {
  _eventDelegate = eventDelegate;
  if (event_listener_ != nullptr) {
    event_listener_->SetDelegate(eventDelegate);
  }
}

- (void)setExposureDelegate:(id<IMarkdownExposureDelegate>)exposureDelegate {
  _exposureDelegate = exposureDelegate;
  if (exposure_listener_ != nullptr) {
    exposure_listener_->SetDelegate(exposureDelegate);
  }
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return;
  }
  if (exposureDelegate == nil) {
    view->SetExposureListener(nullptr);
    return;
  } else {
    view->SetExposureListener(exposure_listener_.get());
  }
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
- (void)setAnimationType:(ServalMarkdownAnimationType)animationType {
  auto* view = [self getMarkdownView];
  view->SetAnimationType(
      static_cast<serval::markdown::MarkdownAnimationType>(animationType));
  _animationType = animationType;
}
- (void)setAnimationVelocity:(float)animationVelocity {
  _animationVelocity = animationVelocity;
  auto* view = [self getMarkdownView];
  view->SetAnimationVelocity(animationVelocity);
}
- (void)setInitialAnimationStep:(int)initialAnimationStep {
  _initialAnimationStep = initialAnimationStep;
  auto* view = [self getMarkdownView];
  view->SetAnimationStep(initialAnimationStep);
}
- (void)setNumberProp:(ServalMarkdownProps)prop Value:(double)value {
  auto* view = [self getMarkdownView];
  view->SetNumberProp(static_cast<serval::markdown::MarkdownProps>(prop),
                      value);
}
- (void)setStringProp:(ServalMarkdownProps)prop Value:(NSString*)value {
  auto* view = [self getMarkdownView];
  view->SetStringProp(static_cast<serval::markdown::MarkdownProps>(prop),
                      [value UTF8String]);
}
- (void)setBooleanProp:(ServalMarkdownProps)prop Value:(BOOL)value {
  auto* view = [self getMarkdownView];
  view->SetNumberProp(static_cast<serval::markdown::MarkdownProps>(prop),
                      value ? 1 : 0);
}
- (void)setColorProp:(ServalMarkdownProps)prop Value:(uint32_t)color {
  auto* view = [self getMarkdownView];
  view->SetNumberProp(static_cast<serval::markdown::MarkdownProps>(prop),
                      color);
}
- (void)setArrayProp:(ServalMarkdownProps)prop Value:(NSArray*)array {
  auto* view = [self getMarkdownView];
  auto result = MarkdownValueConvert::ConvertArray(array);
  view->SetArrayProp(static_cast<serval::markdown::MarkdownProps>(prop),
                     result->AsArray());
}
- (void)setMapProp:(ServalMarkdownProps)prop Value:(NSDictionary*)dict {
  auto* view = [self getMarkdownView];
  auto result = MarkdownValueConvert::ConvertMap(dict);
  view->SetMapProp(static_cast<serval::markdown::MarkdownProps>(prop),
                   result->AsMap());
}
@end
