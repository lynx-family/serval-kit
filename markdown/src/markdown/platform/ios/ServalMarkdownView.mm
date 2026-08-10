// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "markdown/platform/ios/ServalMarkdownView.h"
#import "markdown/platform/ios/MarkdownCustomDrawView.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "markdown/platform/ios/internal/markdown_custom_view_ios.h"
#include "markdown/platform/ios/internal/markdown_main_view_ios.h"
#include "markdown/platform/ios/internal/markdown_measurer_ios.h"
#include "markdown/view/markdown_view.h"

namespace {
serval::markdown::PointF ConvertPoint(CGPoint point) {
  return {static_cast<float>(point.x), static_cast<float>(point.y)};
}

NSArray<NSString*>* ConvertStrings(const std::vector<std::string>& strings) {
  NSMutableArray<NSString*>* result =
      [NSMutableArray arrayWithCapacity:strings.size()];
  for (const auto& string : strings) {
    NSString* value =
        string.empty() ? @""
                       : [[NSString alloc] initWithBytes:string.data()
                                                  length:string.size()
                                                encoding:NSUTF8StringEncoding];
    [result addObject:value == nil ? @"" : value];
  }
  return result;
}

NSArray<NSValue*>* ConvertRects(
    const std::vector<serval::markdown::RectF>& rects) {
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

serval::markdown::MarkdownSelection::CharRangeType ConvertCharRangeType(
    ServalMarkdownCharRangeType type) {
  switch (type) {
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

@interface MarkdownCustomDrawView (AlignInternal)
- (void)alignDrawable:(CGFloat)left top:(CGFloat)top;
@end

@interface ServalMarkdownView () <UIGestureRecognizerDelegate> {
  std::unique_ptr<serval::markdown::MarkdownMainViewIOS> markdown_view_handle_;
  BOOL disableInternalVSync_;
  UITapGestureRecognizer* tapGestureRecognizer_;
  UILongPressGestureRecognizer* longPressGestureRecognizer_;
  UIPanGestureRecognizer* panGestureRecognizer_;
  BOOL isLongPress_;
}
@property(nonatomic, strong, readwrite) MarkdownMeasurer* markdownMeasurer;
@property(nonatomic, strong) CADisplayLink* displayLink;
@property(nonatomic, strong) NSMutableArray<UIView*>* customSubviews;

- (MarkdownCustomDrawView*)createCustomView;
- (MarkdownCustomDrawView*)createRegionView;
- (void)removeSubview:(serval::markdown::MarkdownPlatformView*)subview;
- (void)removeAllCustomViews;
- (void)updateInternalDisplayLinkState;
- (void)onTapGesture:(UITapGestureRecognizer*)recognizer;
- (void)onLongPressGesture:(UILongPressGestureRecognizer*)recognizer;
- (void)onPanGesture:(UIPanGestureRecognizer*)recognizer;

- (serval::markdown::MarkdownView*)getMarkdownView;
@end

@implementation ServalMarkdownView
- (void)alignDrawable:(CGFloat)left top:(CGFloat)top {
  [self.markdownMeasurer align:left top:top];
}

- (instancetype)init {
  return [self initWithCreateMeasurer:YES];
}
- (instancetype)initWithCreateMeasurer:(BOOL)createMeasurer {
  self = [super init];
  if (self != nil) {
    self.customSubviews = [[NSMutableArray alloc] init];
    markdown_view_handle_ =
        std::make_unique<serval::markdown::MarkdownMainViewIOS>(self);
    if (createMeasurer) {
      [self attachMarkdownMeasurer:[[MarkdownMeasurer alloc] init]];
    }
    [self setupGesture];
    __unsafe_unretained id weakSelf = self;
    self.displayLink =
        [CADisplayLink displayLinkWithTarget:weakSelf
                                    selector:@selector(onVSync:)];
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                           forMode:NSRunLoopCommonModes];
    disableInternalVSync_ = NO;
    isLongPress_ = NO;
    [self updateInternalDisplayLinkState];
  }
  return self;
}
- (void)dealloc {
  [self.displayLink invalidate];
  self.displayLink = nil;
  if (self.markdownMeasurer != nil && markdown_view_handle_ != nullptr) {
    [self.markdownMeasurer detachFromView:markdown_view_handle_.get()];
  }
}

- (BOOL)attachMarkdownMeasurer:(MarkdownMeasurer*)measurer {
  if (measurer == nil || self.markdownMeasurer != nil ||
      markdown_view_handle_ == nullptr ||
      ![measurer bindToView:markdown_view_handle_.get()]) {
    return NO;
  }
  self.markdownMeasurer = measurer;
  return YES;
}

- (void)setupGesture {
  tapGestureRecognizer_ =
      [[UITapGestureRecognizer alloc] initWithTarget:self
                                              action:@selector(onTapGesture:)];
  tapGestureRecognizer_.delegate = self;
  [self addGestureRecognizer:tapGestureRecognizer_];
  longPressGestureRecognizer_ = [[UILongPressGestureRecognizer alloc]
      initWithTarget:self
              action:@selector(onLongPressGesture:)];
  [self addGestureRecognizer:longPressGestureRecognizer_];
  longPressGestureRecognizer_.delegate = self;
  panGestureRecognizer_ =
      [[UIPanGestureRecognizer alloc] initWithTarget:self
                                              action:@selector(onPanGesture:)];
  panGestureRecognizer_.delegate = self;
  [self addGestureRecognizer:panGestureRecognizer_];
}

- (void)onTapGesture:(UITapGestureRecognizer*)recognizer {
  if (recognizer.state != UIGestureRecognizerStateEnded || isLongPress_) {
    return;
  }
  auto* markdown_view = [self getMarkdownView];
  if (markdown_view == nullptr) {
    return;
  }
  const CGPoint point = [recognizer locationInView:self];
  markdown_view->OnTap(ConvertPoint(point),
                       serval::markdown::GestureEventType::kDown);
}

- (void)onLongPressGesture:(UILongPressGestureRecognizer*)recognizer {
  if (recognizer.state == UIGestureRecognizerStateEnded ||
      recognizer.state == UIGestureRecognizerStateCancelled ||
      recognizer.state == UIGestureRecognizerStateFailed) {
    isLongPress_ = NO;
  }
  if (recognizer.state != UIGestureRecognizerStateBegan) {
    return;
  }
  isLongPress_ = YES;
  auto* markdown_view = [self getMarkdownView];
  if (markdown_view == nullptr) {
    return;
  }
  const CGPoint point = [recognizer locationInView:self];
  markdown_view->OnLongPress(ConvertPoint(point),
                             serval::markdown::GestureEventType::kDown);
}

- (void)onPanGesture:(UIPanGestureRecognizer*)recognizer {
  auto* markdown_view = [self getMarkdownView];
  if (markdown_view == nullptr) {
    return;
  }
  const CGPoint point = [recognizer locationInView:self];
  const CGPoint motion = [recognizer translationInView:self];
  const auto position = ConvertPoint(point);
  const auto movement = ConvertPoint(motion);
  switch (recognizer.state) {
    case UIGestureRecognizerStateBegan:
      markdown_view->OnPan(position, movement,
                           serval::markdown::GestureEventType::kDown);
      break;
    case UIGestureRecognizerStateChanged:
      markdown_view->OnPan(position, movement,
                           serval::markdown::GestureEventType::kMove);
      break;
    case UIGestureRecognizerStateEnded:
      markdown_view->OnPan(position, movement,
                           serval::markdown::GestureEventType::kUp);
      break;
    case UIGestureRecognizerStateCancelled:
    case UIGestureRecognizerStateFailed:
      markdown_view->OnPan(position, movement,
                           serval::markdown::GestureEventType::kCancel);
      break;
    default:
      break;
  }
}

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer*)gestureRecognizer {
  if (![gestureRecognizer isKindOfClass:UIPanGestureRecognizer.class]) {
    return YES;
  }
  auto* markdown_view = [self getMarkdownView];
  if (markdown_view == nullptr) {
    return NO;
  }
  const CGPoint point = [gestureRecognizer locationInView:self];
  const CGPoint motion =
      [(UIPanGestureRecognizer*)gestureRecognizer translationInView:self];
  bool should_pan =
      markdown_view->ShouldBeginPan(ConvertPoint(point), ConvertPoint(motion));
  if (gestureRecognizer == panGestureRecognizer_) {
    return should_pan;
  } else {
    return !should_pan;
  }
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  if ((gestureRecognizer == panGestureRecognizer_ &&
       [otherGestureRecognizer isKindOfClass:UIPanGestureRecognizer.self]) ||
      (otherGestureRecognizer == panGestureRecognizer_ &&
       [gestureRecognizer isKindOfClass:UIPanGestureRecognizer.self])) {
    return NO;
  }
  return YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldBeRequiredToFailByGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  if ((gestureRecognizer == longPressGestureRecognizer_ &&
       otherGestureRecognizer != panGestureRecognizer_) ||
      (gestureRecognizer == panGestureRecognizer_ &&
       otherGestureRecognizer == tapGestureRecognizer_)) {
    return YES;
  }
  return NO;
}

- (void)requestMeasure {
  [self invalidateIntrinsicContentSize];
  [self setNeedsLayout];
  [self setNeedsDisplay];
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
      static_cast<int64_t>(sender.timestamp * 1000000000.0);
  [self.markdownMeasurer onLayoutFrame:frame_time_nanos];
  [self onRendererFrame:frame_time_nanos];
}
- (serval::markdown::MarkdownView*)getMarkdownView {
  return markdown_view_handle_ == nullptr
             ? nullptr
             : static_cast<serval::markdown::MarkdownView*>(
                   markdown_view_handle_->GetDrawable());
}

- (void)setResourceDelegate:(id<IMarkdownResourceDelegate>)resourceDelegate {
  self.markdownMeasurer.resourceDelegate = resourceDelegate;
}

- (id<IMarkdownResourceDelegate>)resourceDelegate {
  return self.markdownMeasurer.resourceDelegate;
}

- (void)setEventDelegate:(id<IMarkdownEventDelegate>)eventDelegate {
  self.markdownMeasurer.eventDelegate = eventDelegate;
}

- (id<IMarkdownEventDelegate>)eventDelegate {
  return self.markdownMeasurer.eventDelegate;
}

- (void)setExposureDelegate:(id<IMarkdownExposureDelegate>)exposureDelegate {
  self.markdownMeasurer.exposureDelegate = exposureDelegate;
}

- (id<IMarkdownExposureDelegate>)exposureDelegate {
  return self.markdownMeasurer.exposureDelegate;
}

- (void)setContent:(NSString*)content {
  self.markdownMeasurer.content = content;
}

- (NSString*)content {
  return self.markdownMeasurer.content;
}

- (void)markDirty {
  [self.markdownMeasurer markDirty];
}

- (NSString*)getContent {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @"";
  }
  const auto value = view->GetContent();
  return [[NSString alloc] initWithBytes:value.data()
                                  length:value.size()
                                encoding:NSUTF8StringEncoding]
             ?: @"";
}

- (NSString*)getContentID {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @"";
  }
  const auto value = view->GetContentID();
  return [[NSString alloc] initWithBytes:value.data()
                                  length:value.size()
                                encoding:NSUTF8StringEncoding]
             ?: @"";
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
  const auto value =
      view->GetParsedContent({.start_ = range_start, .end_ = range_end});
  return [[NSString alloc] initWithBytes:value.data()
                                  length:value.size()
                                encoding:NSUTF8StringEncoding]
             ?: @"";
}

- (NSString*)getSelectedText {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return @"";
  }
  const auto value = view->GetSelectedText();
  return [[NSString alloc] initWithBytes:value.data()
                                  length:value.size()
                                encoding:NSUTF8StringEncoding]
             ?: @"";
}

- (NSArray<NSString*>*)getAllImageUrl {
  auto* view = [self getMarkdownView];
  return view == nullptr ? @[] : ConvertStrings(view->GetAllImageUrl());
}

- (NSArray<NSString*>*)getLinkUrl {
  auto* view = [self getMarkdownView];
  return view == nullptr ? @[] : ConvertStrings(view->GetLinkUrl());
}

- (NSArray<NSString*>*)getLinkContent {
  auto* view = [self getMarkdownView];
  return view == nullptr ? @[] : ConvertStrings(view->GetLinkContent());
}

- (NSArray<NSValue*>*)getLinkBoundingRect {
  auto* view = [self getMarkdownView];
  return view == nullptr ? @[] : ConvertRects(view->GetLinkBoundingRect());
}

- (NSArray<NSValue*>*)getSyntaxSourceRanges:(NSString*)tag {
  auto* view = [self getMarkdownView];
  if (view == nullptr || tag == nil || tag.UTF8String == nullptr) {
    return @[];
  }
  const auto ranges = view->GetSyntaxSourceRanges(tag.UTF8String);
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
  return range.start_ < 0 || range.end_ < range.start_
             ? NSMakeRange(NSNotFound, 0)
             : NSMakeRange(range.start_, range.end_ - range.start_);
}

- (NSArray<NSValue*>*)getSelectedLineBoundingRect {
  auto* view = [self getMarkdownView];
  return view == nullptr ? @[]
                         : ConvertRects(view->GetSelectedLineBoundingRect());
}

- (CGPoint)getSelectionHandlePosition {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return CGPointMake(-1, -1);
  }
  const auto point = view->GetSelectionHandlePosition();
  return CGPointMake(point.x_, point.y_);
}

- (float)getSelectionHandleRadius {
  auto* view = [self getMarkdownView];
  return view == nullptr ? 0 : view->GetSelectionHandleRadius();
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
  return ConvertRects(view->GetTextLineBoundingRect(
      {.start_ = range_start, .end_ = range_end}));
}

- (int)getCharIndexByPoint:(float)x
                         y:(float)y
                 indexType:(ServalMarkdownIndexType)indexType {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return -1;
  }
  auto result = view->GetCharIndexByPosition({x, y});
  return indexType == kServalMarkdownIndexTypeSource && result >= 0
             ? view->CharOffsetToSourceOffset(result)
             : result;
}

- (NSRange)getCharRangeByPoint:(float)x
                             y:(float)y
                     indexType:(ServalMarkdownIndexType)indexType
                     rangeType:(ServalMarkdownCharRangeType)rangeType {
  auto* view = [self getMarkdownView];
  if (view == nullptr) {
    return NSMakeRange(NSNotFound, 0);
  }
  auto range =
      view->GetCharRangeByPosition({x, y}, ConvertCharRangeType(rangeType));
  if (indexType == kServalMarkdownIndexTypeSource) {
    if (range.start_ >= 0) {
      range.start_ = view->CharOffsetToSourceOffset(range.start_);
    }
    if (range.end_ >= 0) {
      range.end_ = view->CharOffsetToSourceOffset(range.end_);
    }
  }
  return range.start_ < 0 || range.end_ < range.start_
             ? NSMakeRange(NSNotFound, 0)
             : NSMakeRange(static_cast<NSUInteger>(range.start_),
                           static_cast<NSUInteger>(range.end_ - range.start_));
}
- (void)setTextSelection:(int)start end:(int)end {
  [self.markdownMeasurer setTextSelection:start end:end];
}
- (void)setStyle:(NSDictionary*)style {
  self.markdownMeasurer.style = style;
}
- (NSDictionary*)style {
  return self.markdownMeasurer.style;
}
- (void)setAnimationType:(ServalMarkdownAnimationType)animationType {
  self.markdownMeasurer.animationType = animationType;
}
- (ServalMarkdownAnimationType)animationType {
  return self.markdownMeasurer == nil ? kServalMarkdownAnimationTypeNone
                                      : self.markdownMeasurer.animationType;
}
- (void)setAnimationVelocity:(float)animationVelocity {
  self.markdownMeasurer.animationVelocity = animationVelocity;
}
- (float)animationVelocity {
  return self.markdownMeasurer == nil ? 0
                                      : self.markdownMeasurer.animationVelocity;
}
- (void)setInitialAnimationStep:(int)initialAnimationStep {
  self.markdownMeasurer.initialAnimationStep = initialAnimationStep;
}
- (int)initialAnimationStep {
  return self.markdownMeasurer == nil
             ? 0
             : self.markdownMeasurer.initialAnimationStep;
}
- (int)getAnimationStep {
  return self.markdownMeasurer == nil
             ? 0
             : [self.markdownMeasurer getAnimationStep];
}
- (int)getRenderedAnimationStep {
  auto* view = [self getMarkdownView];
  return view == nullptr ? 0 : view->GetRenderedAnimationStep();
}
- (void)pauseRenderUpdate {
  if (auto* view = [self getMarkdownView]; view != nullptr) {
    view->PauseRenderUpdate();
  }
}
- (void)resumeRenderUpdate {
  if (auto* view = [self getMarkdownView]; view != nullptr) {
    view->ResumeRenderUpdate();
  }
}
- (void)disableInternalVSync:(BOOL)disable {
  if (disableInternalVSync_ == disable) {
    return;
  }
  disableInternalVSync_ = disable;
  [self updateInternalDisplayLinkState];
}
- (void)onRendererFrame:(int64_t)frameTimeNanos {
  if (markdown_view_handle_ == nullptr) {
    return;
  }
  const int64_t current_time_ms = frameTimeNanos / 1000000;
  markdown_view_handle_->OnRendererFrame(current_time_ms);
}
- (void)setNumberProp:(ServalMarkdownProps)prop Value:(double)value {
  [self.markdownMeasurer setNumberProp:prop Value:value];
}
- (void)setStringProp:(ServalMarkdownProps)prop Value:(NSString*)value {
  [self.markdownMeasurer setStringProp:prop Value:value];
}
- (void)setBooleanProp:(ServalMarkdownProps)prop Value:(BOOL)value {
  [self.markdownMeasurer setBooleanProp:prop Value:value];
}
- (void)setColorProp:(ServalMarkdownProps)prop Value:(uint32_t)color {
  [self.markdownMeasurer setColorProp:prop Value:color];
}
- (void)setArrayProp:(ServalMarkdownProps)prop Value:(NSArray*)array {
  [self.markdownMeasurer setArrayProp:prop Value:array];
}
- (void)setMapProp:(ServalMarkdownProps)prop Value:(NSDictionary*)dict {
  [self.markdownMeasurer setMapProp:prop Value:dict];
}
- (void)onFontLoaded:(NSString*)family Weight:(int)weight Style:(int)style {
  [self.markdownMeasurer onFontLoaded:family Weight:weight Style:style];
}
- (void)onImageLoaded:(NSString*)url {
  [self.markdownMeasurer onImageLoaded:url];
}
@end
