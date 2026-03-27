// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "markdown/platform/ios/ServalMarkdownView.h"
#import "markdown/platform/ios/MarkdownCustomDrawView.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include "markdown/platform/ios/internal/markdown_custom_view_ios.h"
#include "markdown/platform/ios/internal/markdown_event_ios.h"
#include "markdown/platform/ios/internal/markdown_exposure_ios.h"
#include "markdown/platform/ios/internal/markdown_main_view_ios.h"
#include "markdown/platform/ios/internal/markdown_resource_loader_ios.h"
#include "markdown/platform/ios/internal/markdown_value_convert.h"
#include "markdown/view/markdown_view.h"

namespace {
serval::markdown::MarkdownSelection::CharRangeType ConvertCharRangeType(
    ServalMarkdownCharRangeType range_type) {
  switch (range_type) {
    case kServalMarkdownCharRangeTypeWord:
      return serval::markdown::MarkdownSelection::CharRangeType::kWord;
    case kServalMarkdownCharRangeTypeSentence:
      return serval::markdown::MarkdownSelection::CharRangeType::kSentence;
    case kServalMarkdownCharRangeTypeParagraph:
      return serval::markdown::MarkdownSelection::CharRangeType::kParagraph;
    case kServalMarkdownCharRangeTypeChar:
    default:
      return serval::markdown::MarkdownSelection::CharRangeType::kChar;
  }
}

}  // namespace

@interface ServalMarkdownView () {
  std::unique_ptr<serval::markdown::MarkdownEventIOS> event_listener_;
  std::unique_ptr<serval::markdown::MarkdownExposureIOS> exposure_listener_;
  std::unique_ptr<serval::markdown::MarkdownResourceLoaderIOS> resource_loader_;
  std::unique_ptr<serval::markdown::MarkdownMainViewIOS> markdown_view_handle_;
  BOOL animationPaused_;
  BOOL disableInternalVSync_;
  int64_t currentTimeMs_;
  int64_t pauseStartTimeMs_;
  int64_t totalPausedDurationMs_;
}
@property(nonatomic, strong) CADisplayLink* displayLink;
@property(nonatomic, strong) NSMutableArray<UIView*>* customSubviews;

- (MarkdownCustomDrawView*)createCustomView;
- (MarkdownCustomDrawView*)createRegionView;
- (void)removeSubview:(serval::markdown::MarkdownPlatformView*)subview;
- (void)removeAllCustomViews;
- (void)updateInternalDisplayLinkState;

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
    animationPaused_ = NO;
    disableInternalVSync_ = NO;
    currentTimeMs_ = 0;
    pauseStartTimeMs_ = 0;
    totalPausedDurationMs_ = 0;
    [self updateInternalDisplayLinkState];
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
- (void)updateInternalDisplayLinkState {
  self.displayLink.paused = disableInternalVSync_;
}
- (void)onVSync:(CADisplayLink*)sender {
  const int64_t frame_time_nanos =
      static_cast<int64_t>(sender.targetTimestamp * 1000000000.0);
  [self onLayoutFrame:frame_time_nanos];
  [self onRendererFrame:frame_time_nanos];
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
- (NSString*)getContent {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @"";
  }
  const auto content = view->GetContent();
  NSString* content_string =
      [[NSString alloc] initWithBytes:content.data()
                               length:content.size()
                             encoding:NSUTF8StringEncoding];
  return content_string != nil ? content_string : @"";
}
- (NSString*)getContentID {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @"";
  }
  const auto content_id = view->GetContentID();
  NSString* content_id_string =
      [[NSString alloc] initWithBytes:content_id.data()
                               length:content_id.size()
                             encoding:NSUTF8StringEncoding];
  return content_id_string != nil ? content_id_string : @"";
}
- (NSString*)getContent:(int)start
                    end:(int)end
              indexType:(ServalMarkdownIndexType)indexType {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @"";
  }
  int32_t range_start = std::max(0, std::min(start, end));
  int32_t range_end = std::max(0, std::max(start, end));
  if (indexType == kServalMarkdownIndexTypeSource) {
    range_start = view->SourceOffsetToCharOffset(range_start);
    range_end = view->SourceOffsetToCharOffset(range_end);
  }
  const auto content = view->GetParsedContent({range_start, range_end});
  NSString* content_string =
      [[NSString alloc] initWithBytes:content.data()
                               length:content.size()
                             encoding:NSUTF8StringEncoding];
  return content_string != nil ? content_string : @"";
}
- (NSString*)getSelectedText {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @"";
  }
  const auto content = view->GetSelectedText();
  NSString* content_string =
      [[NSString alloc] initWithBytes:content.data()
                               length:content.size()
                             encoding:NSUTF8StringEncoding];
  return content_string != nil ? content_string : @"";
}
- (NSArray<NSString*>*)getAllImageUrl {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @[];
  }
  const auto urls = view->GetAllImageUrl();
  NSMutableArray<NSString*>* result =
      [NSMutableArray arrayWithCapacity:urls.size()];
  for (const auto& url : urls) {
    NSString* value = [[NSString alloc] initWithBytes:url.data()
                                               length:url.size()
                                             encoding:NSUTF8StringEncoding];
    [result addObject:value != nil ? value : @""];
  }
  return result;
}
- (NSArray<NSString*>*)getLinkUrl {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @[];
  }
  const auto urls = view->GetLinkUrl();
  NSMutableArray<NSString*>* result =
      [NSMutableArray arrayWithCapacity:urls.size()];
  for (const auto& url : urls) {
    NSString* value = [[NSString alloc] initWithBytes:url.data()
                                               length:url.size()
                                             encoding:NSUTF8StringEncoding];
    [result addObject:value != nil ? value : @""];
  }
  return result;
}
- (NSArray<NSString*>*)getLinkContent {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @[];
  }
  const auto contents = view->GetLinkContent();
  NSMutableArray<NSString*>* result =
      [NSMutableArray arrayWithCapacity:contents.size()];
  for (const auto& content : contents) {
    NSString* value = [[NSString alloc] initWithBytes:content.data()
                                               length:content.size()
                                             encoding:NSUTF8StringEncoding];
    [result addObject:value != nil ? value : @""];
  }
  return result;
}
- (NSArray<NSValue*>*)getLinkBoundingRect {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @[];
  }
  const auto rects = view->GetLinkBoundingRect();
  NSMutableArray<NSValue*>* result =
      [NSMutableArray arrayWithCapacity:rects.size()];
  for (const auto& rect : rects) {
    [result addObject:[NSValue valueWithCGRect:CGRectMake(rect.GetLeft(),
                                                          rect.GetTop(),
                                                          rect.GetWidth(),
                                                          rect.GetHeight())]];
  }
  return result;
}
- (NSArray<NSValue*>*)getSyntaxSourceRanges:(NSString*)tag {
  auto* view = [self getMarkdownView];
  if (view == nullptr || tag == nil) {
    return @[];
  }
  const auto* tag_chars = [tag UTF8String];
  if (tag_chars == nullptr) {
    return @[];
  }
  const auto ranges = view->GetSyntaxSourceRanges(tag_chars);
  NSMutableArray<NSValue*>* result =
      [NSMutableArray arrayWithCapacity:ranges.size()];
  for (const auto& range : ranges) {
    if (range.start_ < 0 || range.end_ < range.start_) {
      continue;
    }
    [result
        addObject:[NSValue
                      valueWithRange:NSMakeRange(
                                         static_cast<NSUInteger>(range.start_),
                                         static_cast<NSUInteger>(
                                             range.end_ - range.start_))]];
  }
  return result;
}
- (NSRange)getSelectedRange {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return NSMakeRange(NSNotFound, 0);
  }
  const auto range = view->GetSelectedRange();
  if (range.start_ < 0 || range.end_ < range.start_) {
    return NSMakeRange(NSNotFound, 0);
  }
  return NSMakeRange(static_cast<NSUInteger>(range.start_),
                     static_cast<NSUInteger>(range.end_ - range.start_));
}
- (NSArray<NSValue*>*)getSelectedLineBoundingRect {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @[];
  }
  const auto& rects = view->GetSelectedLineBoundingRect();
  NSMutableArray<NSValue*>* result =
      [NSMutableArray arrayWithCapacity:rects.size()];
  for (const auto& rect : rects) {
    [result addObject:[NSValue valueWithCGRect:CGRectMake(rect.GetLeft(),
                                                          rect.GetTop(),
                                                          rect.GetWidth(),
                                                          rect.GetHeight())]];
  }
  return result;
}
- (CGPoint)getSelectionHandlePosition {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return CGPointMake(-1, -1);
  }
  const auto position = view->GetSelectionHandlePosition();
  return CGPointMake(position.x_, position.y_);
}
- (float)getSelectionHandleRadius {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return 0;
  }
  return view->GetSelectionHandleRadius();
}
- (NSArray<NSValue*>*)getTextBoundingRect:(int)start
                                      end:(int)end
                                indexType:(ServalMarkdownIndexType)indexType {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @[];
  }
  int32_t range_start = std::max(0, std::min(start, end));
  int32_t range_end = std::max(0, std::max(start, end));
  if (indexType == kServalMarkdownIndexTypeSource) {
    range_start = view->SourceOffsetToCharOffset(range_start);
    range_end = view->SourceOffsetToCharOffset(range_end);
  }
  const auto rects = view->GetTextLineBoundingRect({range_start, range_end});
  NSMutableArray<NSValue*>* result =
      [NSMutableArray arrayWithCapacity:rects.size()];
  for (const auto& rect : rects) {
    [result addObject:[NSValue valueWithCGRect:CGRectMake(rect.GetLeft(),
                                                          rect.GetTop(),
                                                          rect.GetWidth(),
                                                          rect.GetHeight())]];
  }
  return result;
}
- (int)getCharIndexByPoint:(float)x
                         y:(float)y
                 indexType:(ServalMarkdownIndexType)indexType {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return -1;
  }
  int32_t char_index = view->GetCharIndexByPosition({x, y});
  if (indexType == kServalMarkdownIndexTypeSource && char_index >= 0) {
    return view->CharOffsetToSourceOffset(char_index);
  }
  return char_index;
}
- (NSRange)getCharRangeByPoint:(float)x
                             y:(float)y
                     indexType:(ServalMarkdownIndexType)indexType
                     rangeType:(ServalMarkdownCharRangeType)rangeType {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return NSMakeRange(NSNotFound, 0);
  }
  auto char_range =
      view->GetCharRangeByPosition({x, y}, ConvertCharRangeType(rangeType));
  if (indexType == kServalMarkdownIndexTypeSource) {
    if (char_range.start_ >= 0) {
      char_range.start_ = view->CharOffsetToSourceOffset(char_range.start_);
    }
    if (char_range.end_ >= 0) {
      char_range.end_ = view->CharOffsetToSourceOffset(char_range.end_);
    }
  }
  if (char_range.start_ < 0 || char_range.end_ < char_range.start_) {
    return NSMakeRange(NSNotFound, 0);
  }
  return NSMakeRange(
      static_cast<NSUInteger>(char_range.start_),
      static_cast<NSUInteger>(char_range.end_ - char_range.start_));
}
- (void)setTextSelection:(int)start end:(int)end {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return;
  }
  view->SetTextSelection({start, end});
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
- (int)getAnimationStep {
  auto* view = [self getMarkdownView];
  return view == nullptr ? 0 : view->GetAnimationStep();
}
- (void)disableInternalVSync:(BOOL)disable {
  if (disableInternalVSync_ == disable) {
    return;
  }
  disableInternalVSync_ = disable;
  [self updateInternalDisplayLinkState];
}
- (void)onLayoutFrame:(int64_t)frameTimeNanos {
  currentTimeMs_ = frameTimeNanos / 1000000;
  if (markdown_view_handle_ == nullptr || animationPaused_) {
    return;
  }
  const auto adjusted_time_ms = currentTimeMs_ - totalPausedDurationMs_;
  markdown_view_handle_->OnLayoutFrame(adjusted_time_ms);
}
- (void)onRendererFrame:(int64_t)frameTimeNanos {
  if (markdown_view_handle_ == nullptr) {
    return;
  }
  const int64_t current_time_ms = frameTimeNanos / 1000000;
  markdown_view_handle_->OnRendererFrame(current_time_ms);
}
- (void)pauseAnimation {
  if (animationPaused_) {
    return;
  }
  animationPaused_ = YES;
  pauseStartTimeMs_ = currentTimeMs_;
}
- (void)resumeAnimation {
  [self resumeAnimation:-1];
}
- (void)resumeAnimation:(int)animationStep {
  auto* view = [self getMarkdownView];
  if (animationStep != -1 && view != nullptr) {
    view->SetAnimationStep(animationStep);
  }
  if (animationPaused_) {
    animationPaused_ = NO;
    if (pauseStartTimeMs_ > 0 && currentTimeMs_ > pauseStartTimeMs_) {
      totalPausedDurationMs_ += currentTimeMs_ - pauseStartTimeMs_;
    }
  }
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
