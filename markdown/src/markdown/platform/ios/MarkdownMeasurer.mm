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
#include "markdown/platform/ios/internal/markdown_platform_ios.h"
#include "markdown/platform/ios/internal/markdown_value_convert.h"
#include "markdown/view/markdown_view_measurer.h"

namespace {

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

@implementation MarkdownMeasurer

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
  const auto max_lines = ConvertMaxLines(maxLines);
  measurer.SetTextMaxLines(max_lines);
  serval::markdown::MeasureSpec spec{
      .width_ = static_cast<float>(maxWidth),
      .width_mode_ = tttext::LayoutMode::kAtMost,
      .height_ = serval::markdown::MeasureSpec::LAYOUT_MAX_SIZE,
      .height_mode_ = tttext::LayoutMode::kIndefinite,
  };
  const auto size = measurer.Measure(spec);
  return [[MarkdownMeasureResult alloc]
      initWithWidth:static_cast<CGFloat>(size.width_)
             height:static_cast<CGFloat>(size.height_)
              lines:ConvertLineTexts(measurer.GetDocument()->GetLineTexts())];
}

@end
