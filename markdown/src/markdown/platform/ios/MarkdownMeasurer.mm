// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "markdown/platform/ios/MarkdownMeasurer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "markdown/element/markdown_context.h"
#include "markdown/platform/ios/internal/markdown_event_ios.h"
#include "markdown/platform/ios/internal/markdown_exposure_ios.h"
#include "markdown/platform/ios/internal/markdown_main_view_ios.h"
#include "markdown/platform/ios/internal/markdown_measurer_ios.h"
#include "markdown/platform/ios/internal/markdown_platform_ios.h"
#include "markdown/platform/ios/internal/markdown_resource_loader_ios.h"
#include "markdown/platform/ios/internal/markdown_value_convert.h"
#include "markdown/view/markdown_view.h"
#include "markdown/view/markdown_view_measurer.h"

namespace {
class MarkdownMeasureHostIOS;

int32_t ConvertMaxLines(NSInteger max_lines) {
  if (max_lines <= 0) {
    return -1;
  }
  return static_cast<int32_t>(std::min(
      max_lines, static_cast<NSInteger>(std::numeric_limits<int32_t>::max())));
}

NSArray<NSString*>* ConvertLineTexts(
    const std::vector<std::string>& line_texts) {
  NSMutableArray<NSString*>* lines =
      [NSMutableArray arrayWithCapacity:line_texts.size()];
  for (const auto& text : line_texts) {
    NSString* line =
        text.empty() ? @""
                     : [[NSString alloc] initWithBytes:text.data()
                                                length:text.size()
                                              encoding:NSUTF8StringEncoding];
    [lines addObject:line == nil ? @"" : line];
  }
  return [lines copy];
}

}  // namespace

@interface MarkdownMeasureResult ()

- (instancetype)initWithWidth:(CGFloat)width
                       height:(CGFloat)height
                        lines:(NSArray<NSString*>*)lines;

@end

@implementation MarkdownMeasureResult

- (instancetype)initWithWidth:(CGFloat)width
                       height:(CGFloat)height
                        lines:(NSArray<NSString*>*)lines {
  self = [super init];
  if (self != nil) {
    _width = width;
    _height = height;
    _lines = [lines copy];
  }
  return self;
}

- (NSInteger)lineCount {
  return self.lines.count;
}

@end

@interface MarkdownMeasurer () {
  std::unique_ptr<MarkdownMeasureHostIOS> measure_host_;
  std::shared_ptr<serval::markdown::MarkdownView> native_view_;
  std::unique_ptr<serval::markdown::MarkdownEventIOS> event_listener_;
  std::unique_ptr<serval::markdown::MarkdownExposureIOS> exposure_listener_;
  std::unique_ptr<serval::markdown::MarkdownResourceLoaderIOS> resource_loader_;
  serval::markdown::MarkdownMainViewIOS* bound_view_;
  BOOL animation_paused_;
  int64_t current_time_ms_;
  int64_t pause_start_time_ms_;
  int64_t total_paused_duration_ms_;
}
- (void)requestMeasure;
- (void)align:(CGFloat)left top:(CGFloat)top;
@end

namespace {
class MarkdownMeasureHostIOS final
    : public serval::markdown::MarkdownViewMeasureHost {
 public:
  explicit MarkdownMeasureHostIOS(MarkdownMeasurer* measurer)
      : measurer_(measurer) {}

  void RequestMeasure() override { [measurer_ requestMeasure]; }

 private:
  __unsafe_unretained MarkdownMeasurer* measurer_;
};
}  // namespace

@implementation MarkdownMeasurer

- (instancetype)init {
  self = [super init];
  if (self != nil) {
    measure_host_ = std::make_unique<MarkdownMeasureHostIOS>(self);
    native_view_ = std::make_shared<serval::markdown::MarkdownView>(
        nullptr, measure_host_.get(),
        std::make_shared<serval::markdown::MarkdownContext>(
            serval::markdown::CreateIOSMarkdownPlatform()));
    event_listener_ = std::make_unique<serval::markdown::MarkdownEventIOS>();
    exposure_listener_ =
        std::make_unique<serval::markdown::MarkdownExposureIOS>();
    resource_loader_ =
        std::make_unique<serval::markdown::MarkdownResourceLoaderIOS>();
    bound_view_ = nullptr;
    animation_paused_ = NO;
    current_time_ms_ = 0;
    pause_start_time_ms_ = 0;
    total_paused_duration_ms_ = 0;
    auto* view = [self getMarkdownView];
    view->SetEventListener(event_listener_.get());
    view->SetResourceLoader(resource_loader_.get());
  }
  return self;
}

- (void)dealloc {
  if (bound_view_ != nullptr) {
    [self detachFromView:bound_view_];
  }
  auto* view = [self getMarkdownView];
  if (view != nullptr) {
    view->SetExposureListener(nullptr);
    view->SetEventListener(nullptr);
    view->SetResourceLoader(nullptr);
  }
}

- (serval::markdown::MarkdownView*)getMarkdownView {
  return native_view_.get();
}

- (BOOL)bindToView:(serval::markdown::MarkdownMainViewIOS*)view {
  if (view == nullptr || bound_view_ != nullptr || native_view_ == nullptr) {
    return NO;
  }
  bound_view_ = view;
  bound_view_->AttachDrawable(native_view_);
  native_view_->SetView(bound_view_);
  return YES;
}

- (void)detachFromView:(serval::markdown::MarkdownMainViewIOS*)view {
  if (view == nullptr || view != bound_view_) {
    return;
  }
  bound_view_ = nullptr;
}

- (void)requestMeasure {
  MarkdownRequestMeasureCallback callback = self.requestMeasureCallback;
  if (callback != nil) {
    callback();
  } else if (bound_view_ != nullptr) {
    bound_view_->RequestMeasure();
  }
}

- (ServalMarkdownMeasureResult)
    measureWithWidth:(CGFloat)width
           widthMode:(ServalMarkdownLayoutMode)widthMode
              height:(CGFloat)height
          heightMode:(ServalMarkdownLayoutMode)heightMode {
  if (!std::isfinite(width) || !std::isfinite(height) || width < 0 ||
      height < 0 || width > std::numeric_limits<float>::max() ||
      height > std::numeric_limits<float>::max()) {
    @throw [NSException
        exceptionWithName:NSInvalidArgumentException
                   reason:
                       @"Markdown measure sizes must be finite and non-negative"
                 userInfo:nil];
  }
  const auto result = native_view_->Measure(
      {.width_ = static_cast<float>(width),
       .width_mode_ = static_cast<tttext::LayoutMode>(widthMode),
       .height_ = static_cast<float>(height),
       .height_mode_ = static_cast<tttext::LayoutMode>(heightMode)});
  return {
      .width = result.width_,
      .height = result.height_,
      .baseline = result.baseline_,
  };
}

- (void)align:(CGFloat)left top:(CGFloat)top {
  if (native_view_ != nullptr) {
    native_view_->Align(static_cast<float>(left), static_cast<float>(top));
  }
}

- (void)setContent:(NSString*)content {
  const char* value = content == nil ? "" : content.UTF8String;
  [self getMarkdownView]->SetContent(value == nullptr ? "" : value);
  _content = [content copy];
}

- (void)setStyle:(NSDictionary*)style {
  auto value = serval::markdown::MarkdownValueConvert::ConvertMap(style);
  [self getMarkdownView]->SetStyle(value->AsMap());
  _style = [style copy];
}

- (void)setAnimationType:(ServalMarkdownAnimationType)animationType {
  [self getMarkdownView]->SetAnimationType(
      static_cast<serval::markdown::MarkdownAnimationType>(animationType));
  _animationType = animationType;
}

- (void)setAnimationVelocity:(float)animationVelocity {
  [self getMarkdownView]->SetAnimationVelocity(animationVelocity);
  _animationVelocity = animationVelocity;
}

- (void)setInitialAnimationStep:(int)initialAnimationStep {
  [self getMarkdownView]->SetInitialAnimationStep(initialAnimationStep);
  _initialAnimationStep = initialAnimationStep;
}

- (void)setResourceDelegate:(id<IMarkdownResourceDelegate>)delegate {
  _resourceDelegate = delegate;
  resource_loader_->SetDelegate(delegate);
}

- (void)setEventDelegate:(id<IMarkdownEventDelegate>)delegate {
  _eventDelegate = delegate;
  event_listener_->SetDelegate(delegate);
}

- (void)setExposureDelegate:(id<IMarkdownExposureDelegate>)delegate {
  _exposureDelegate = delegate;
  exposure_listener_->SetDelegate(delegate);
  [self getMarkdownView]->SetExposureListener(
      delegate == nil ? nullptr : exposure_listener_.get());
}

- (void)markDirty {
  [self getMarkdownView]->MarkDirty();
}

- (void)setTextSelection:(int)start end:(int)end {
  [self getMarkdownView]->SetTextSelection({start, end});
}

- (int)getAnimationStep {
  return [self getMarkdownView]->GetAnimationStep();
}

- (void)setAnimationStep:(int)animationStep {
  [self getMarkdownView]->SetAnimationStep(animationStep);
}

- (void)pauseAnimation {
  if (animation_paused_) {
    return;
  }
  animation_paused_ = YES;
  pause_start_time_ms_ = current_time_ms_;
}

- (void)resumeAnimation {
  [self resumeAnimation:-1];
}

- (void)resumeAnimation:(int)animationStep {
  if (animationStep != -1) {
    [self setAnimationStep:animationStep];
  }
  if (!animation_paused_) {
    return;
  }
  animation_paused_ = NO;
  if (pause_start_time_ms_ > 0 && current_time_ms_ > pause_start_time_ms_) {
    total_paused_duration_ms_ += current_time_ms_ - pause_start_time_ms_;
  }
}

- (void)onLayoutFrame:(int64_t)frameTimeNanos {
  current_time_ms_ = frameTimeNanos / 1000000;
  if (!animation_paused_ && native_view_ != nullptr) {
    native_view_->OnLayoutFrame(current_time_ms_ - total_paused_duration_ms_);
  }
}

- (void)setNumberProp:(ServalMarkdownProps)prop Value:(double)value {
  [self getMarkdownView]->SetNumberProp(
      static_cast<serval::markdown::MarkdownProps>(prop), value);
}

- (void)setStringProp:(ServalMarkdownProps)prop Value:(NSString*)value {
  [self getMarkdownView]->SetStringProp(
      static_cast<serval::markdown::MarkdownProps>(prop),
      value == nil || value.UTF8String == nullptr ? "" : value.UTF8String);
}

- (void)setBooleanProp:(ServalMarkdownProps)prop Value:(BOOL)value {
  [self setNumberProp:prop Value:value ? 1 : 0];
}

- (void)setColorProp:(ServalMarkdownProps)prop Value:(uint32_t)value {
  [self setNumberProp:prop Value:value];
}

- (void)setArrayProp:(ServalMarkdownProps)prop Value:(NSArray*)array {
  auto value = serval::markdown::MarkdownValueConvert::ConvertArray(array);
  [self getMarkdownView]->SetArrayProp(
      static_cast<serval::markdown::MarkdownProps>(prop), value->AsArray());
}

- (void)setMapProp:(ServalMarkdownProps)prop Value:(NSDictionary*)dict {
  auto value = serval::markdown::MarkdownValueConvert::ConvertMap(dict);
  [self getMarkdownView]->SetMapProp(
      static_cast<serval::markdown::MarkdownProps>(prop), value->AsMap());
}

- (void)onFontLoaded:(NSString*)family Weight:(int)weight Style:(int)style {
  if (family != nil && family.UTF8String != nullptr) {
    [self getMarkdownView]->OnFontLoaded(family.UTF8String, weight, style);
    [self requestMeasure];
  }
}

- (void)onImageLoaded:(NSString*)url {
  if (url != nil && url.UTF8String != nullptr) {
    [self getMarkdownView]->OnImageLoaded(url.UTF8String);
    [self requestMeasure];
  }
}

+ (MarkdownMeasureResult*)measure:(NSString*)markdown
                            style:(NSDictionary*)style
                         maxWidth:(CGFloat)maxWidth
                         maxLines:(NSInteger)maxLines {
  if (!std::isfinite(maxWidth) || maxWidth < 0 ||
      maxWidth > std::numeric_limits<float>::max()) {
    @throw [NSException
        exceptionWithName:NSInvalidArgumentException
                   reason:@"maxWidth must be finite and non-negative"
                 userInfo:nil];
  }

  auto context = std::make_shared<serval::markdown::MarkdownContext>(
      serval::markdown::CreateIOSMarkdownPlatform());
  serval::markdown::MarkdownViewMeasurer measurer(std::move(context));
  auto style_value = serval::markdown::MarkdownValueConvert::ConvertMap(style);
  measurer.SetStyle(style_value->AsMap());
  const char* content = markdown == nil ? "" : markdown.UTF8String;
  measurer.SetContent(content == nullptr ? "" : content);
  measurer.SetTextMaxLines(ConvertMaxLines(maxLines));
  serval::markdown::MeasureSpec spec{
      .width_ = static_cast<float>(maxWidth),
      .width_mode_ = tttext::LayoutMode::kAtMost,
      .height_ = serval::markdown::MeasureSpec::LAYOUT_MAX_SIZE,
      .height_mode_ = tttext::LayoutMode::kIndefinite,
  };
  const auto result = measurer.Measure(spec);
  return [[MarkdownMeasureResult alloc]
      initWithWidth:static_cast<CGFloat>(result.width_)
             height:static_cast<CGFloat>(result.height_)
              lines:ConvertLineTexts(measurer.GetDocument()->GetLineTexts())];
}

@end
