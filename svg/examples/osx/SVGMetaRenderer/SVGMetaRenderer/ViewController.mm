// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ViewController.h"

#import "SVGMetalView.h"

@interface ViewController ()

@property(nonatomic, strong) NSPopUpButton* categoryPopUpButton;
@property(nonatomic, strong) NSSegmentedControl* renderModeControl;
@property(nonatomic, strong) NSScrollView* scrollView;
@property(nonatomic, strong) NSView* documentView;
@property(nonatomic, strong) NSArray<NSString*>* categories;
@property(nonatomic, strong)
    NSDictionary<NSString*, NSArray<NSString*>*>* categorizedFiles;
@property(nonatomic, copy) NSString* selectedCategory;
@property(nonatomic, copy) NSString* externalSVGPath;
@property(nonatomic, copy) NSString* externalSVGDisplayName;
@property(nonatomic, strong) NSDictionary<NSString*, NSValue*>* rowFrames;
@property(nonatomic, copy) NSString* pendingScrollTarget;
@property(nonatomic, assign) CGFloat lastContentWidth;
@property(nonatomic, assign) BOOL useSoftwareCanvas;

@end

@interface SrSVGPreviewMetadata : NSObject
@property(nonatomic, copy) NSString* title;
@property(nonatomic, copy) NSArray<NSString*>* tags;
@property(nonatomic, copy) NSArray<NSString*>* descriptions;
@end

@implementation SrSVGPreviewMetadata
@end

@interface SrSVGFlippedView : NSView
@end

@implementation SrSVGFlippedView

- (BOOL)isFlipped {
  return YES;
}

@end

@implementation ViewController

static NSString* const kHostColorCompareFile =
    @"currentcolor-host-default-compare.svg";
static NSString* const kHostColorOverrideFile =
    @"currentcolor-content-color-override.svg";
static NSString* const kHostDefaultColor = @"#4F6BFF";
static NSString* const kCategoryOthers = @"Others";
static NSString* const kCategoryColorParsing = @"ColorParsing";
static NSString* const kCategoryCurrentColor = @"CurrentColor";
static NSString* const kCategoryIllegalParsing = @"IllegalParsing";
static NSString* const kCategoryMask = @"Mask";
static NSString* const kCategoryFilter = @"Filter";
static NSString* const kCategoryPattern = @"Pattern";
static NSString* const kCategorySvgRoot = @"SvgRoot";
static NSString* const kCategoryUse = @"Use";
static NSString* const kCategoryGradient = @"Gradient";
static NSString* const kCategoryShape = @"Shape";
static NSString* const kCategoryVectorEffect = @"VectorEffect";
static NSString* const kCategoryUnsupported = @"Unsupported";
static NSString* const kExternalPreviewKey = @"__external_svg_preview__";

static NSArray<NSString*>* kPreviewMetadataFiles() {
  return @[
    @"svg_root_metadata", @"svg_shape_metadata", @"svg_use_metadata",
    @"svg_gradient_metadata"
  ];
}

- (void)loadView {
  self.view =
      [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 1100.0, 760.0)];
  self.view.wantsLayer = YES;
  self.view.layer.backgroundColor =
      [NSColor colorWithWhite:0.96 alpha:1.0].CGColor;
  self.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
}

- (NSArray<NSString*>*)orderedCategories {
  return @[
    kCategoryShape, kCategoryColorParsing, kCategoryCurrentColor, kCategoryMask,
    kCategoryFilter, kCategoryIllegalParsing, kCategoryPattern,
    kCategorySvgRoot, kCategoryUse, kCategoryGradient, kCategoryVectorEffect,
    kCategoryUnsupported, kCategoryOthers
  ];
}

- (SrSVGPreviewMetadata*)metadataWithTitle:(NSString*)title
                                      tags:(NSArray<NSString*>*)tags
                              descriptions:(NSArray<NSString*>*)descriptions {
  SrSVGPreviewMetadata* metadata = [[SrSVGPreviewMetadata alloc] init];
  metadata.title = title;
  metadata.tags = tags ?: @[];
  metadata.descriptions = descriptions ?: @[];
  return metadata;
}

- (NSDictionary<NSString*, SrSVGPreviewMetadata*>*)loadPreviewMetadataMap {
  NSMutableDictionary<NSString*, SrSVGPreviewMetadata*>* result =
      [NSMutableDictionary dictionary];
  for (NSString* file in kPreviewMetadataFiles()) {
    NSString* path = [[NSBundle mainBundle] pathForResource:file
                                                     ofType:@"json"];
    if (path.length == 0) {
      continue;
    }
    NSData* data = [NSData dataWithContentsOfFile:path];
    if (data.length == 0) {
      continue;
    }
    NSError* error = nil;
    id jsonObject = [NSJSONSerialization JSONObjectWithData:data
                                                    options:0
                                                      error:&error];
    if (![jsonObject isKindOfClass:[NSDictionary class]]) {
      continue;
    }
    NSDictionary* json = (NSDictionary*)jsonObject;
    NSArray* cases = json[@"cases"];
    if (![cases isKindOfClass:[NSArray class]]) {
      continue;
    }
    for (id item in cases) {
      if (![item isKindOfClass:[NSDictionary class]]) {
        continue;
      }
      NSDictionary* caseInfo = (NSDictionary*)item;
      NSString* fileName = caseInfo[@"fileName"];
      if (![fileName isKindOfClass:[NSString class]] || fileName.length == 0) {
        continue;
      }
      NSString* title = [caseInfo[@"title"] isKindOfClass:[NSString class]]
                            ? caseInfo[@"title"]
                            : fileName;
      NSArray<NSString*>* tags =
          [caseInfo[@"tags"] isKindOfClass:[NSArray class]]
              ? (NSArray<NSString*>*)caseInfo[@"tags"]
              : @[];
      NSArray<NSString*>* descriptions =
          [caseInfo[@"descriptions"] isKindOfClass:[NSArray class]]
              ? (NSArray<NSString*>*)caseInfo[@"descriptions"]
              : @[];
      result[fileName] = [self metadataWithTitle:title
                                            tags:tags
                                    descriptions:descriptions];
    }
  }
  return result;
}

- (SrSVGPreviewMetadata*)previewMetadataForFile:(NSString*)fileName {
  static NSDictionary<NSString*, SrSVGPreviewMetadata*>* metadataMap = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ metadataMap = [self loadPreviewMetadataMap]; });
  return metadataMap[fileName];
}

- (NSString*)categoryForFile:(NSString*)fileName {
  if ([fileName hasPrefix:@"color-parsing-"]) {
    return kCategoryColorParsing;
  }
  if ([fileName hasPrefix:@"currentcolor-"]) {
    return kCategoryCurrentColor;
  }
  if ([fileName hasPrefix:@"invalid-"]) {
    return kCategoryIllegalParsing;
  }
  if ([fileName hasPrefix:@"mask-"]) {
    return kCategoryMask;
  }
  if ([fileName hasPrefix:@"filter-"]) {
    return kCategoryFilter;
  }
  if ([fileName containsString:@"unsupported"]) {
    return kCategoryUnsupported;
  }
  if ([fileName hasPrefix:@"pattern-"] ||
      [fileName isEqualToString:@"stroke-gradient-vs-pattern.svg"]) {
    return kCategoryPattern;
  }
  if ([fileName hasPrefix:@"svg-root-"] ||
      [fileName hasPrefix:@"svg-preserve-aspect-ratio-"]) {
    return kCategorySvgRoot;
  }
  if ([fileName hasPrefix:@"use-"]) {
    return kCategoryUse;
  }
  if ([fileName hasPrefix:@"gradient-"]) {
    return kCategoryGradient;
  }
  if ([fileName hasPrefix:@"shape-"]) {
    return kCategoryShape;
  }
  if ([fileName hasPrefix:@"vector-effect-"]) {
    return kCategoryVectorEffect;
  }
  return kCategoryOthers;
}

- (NSDictionary<NSString*, NSArray<NSString*>*>*)buildCategorizedFiles:
    (NSArray<NSString*>*)svgFiles {
  NSMutableDictionary<NSString*, NSMutableArray<NSString*>*>* filesByCategory =
      [NSMutableDictionary dictionary];
  for (NSString* category in [self orderedCategories]) {
    filesByCategory[category] = [NSMutableArray array];
  }
  for (NSString* fileName in svgFiles) {
    NSMutableArray<NSString*>* files =
        filesByCategory[[self categoryForFile:fileName]];
    [files addObject:fileName];
  }
  NSMutableDictionary<NSString*, NSArray<NSString*>*>* result =
      [NSMutableDictionary dictionary];
  for (NSString* category in [self orderedCategories]) {
    result[category] = [filesByCategory[category] copy];
  }
  return result;
}

- (NSArray<NSString*>*)filesForSelectedCategory {
  return self.categorizedFiles[self.selectedCategory] ?: @[];
}

- (NSString*)svgContentForFile:(NSString*)fileName {
  if ([fileName isEqualToString:@"string_test.svg"]) {
    return @"<svg width=\"200\" height=\"200\" viewBox=\"0 0 200 200\" "
           @"xmlns=\"http://www.w3.org/2000/svg\">"
           @"<defs>"
           @"<pattern id=\"TrianglePattern\" x=\"0\" y=\"0\" width=\"20\" "
           @"height=\"20\" patternUnits=\"userSpaceOnUse\">"
           @"<path d=\"M 0 0 L 10 0 L 5 10 Z\" fill=\"red\" />"
           @"</pattern>"
           @"</defs>"
           @"<rect x=\"0\" y=\"0\" width=\"200\" height=\"200\" "
           @"fill=\"url(#TrianglePattern)\" />"
           @"</svg>";
  }

  NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
  NSString* svgPath =
      [resourcePath stringByAppendingPathComponent:
                        [@"svg" stringByAppendingPathComponent:fileName]];
  NSError* error = nil;
  NSString* fileContent =
      [NSString stringWithContentsOfFile:svgPath
                                encoding:NSUTF8StringEncoding
                                   error:&error];
  if (!fileContent) {
    NSString* nameWithoutExt = [fileName stringByDeletingPathExtension];
    NSString* path = [[NSBundle mainBundle] pathForResource:nameWithoutExt
                                                     ofType:@"svg"];
    if (path) {
      fileContent = [NSString stringWithContentsOfFile:path
                                              encoding:NSUTF8StringEncoding
                                                 error:&error];
    }
    if (!fileContent) {
      path = [[NSBundle mainBundle] pathForResource:nameWithoutExt
                                             ofType:@"svg"
                                        inDirectory:@"svg"];
      if (path) {
        fileContent = [NSString stringWithContentsOfFile:path
                                                encoding:NSUTF8StringEncoding
                                                   error:&error];
      }
    }
  }
  return fileContent;
}

- (NSString*)svgContentForExternalPath:(NSString*)path {
  if (path.length == 0) {
    return nil;
  }
  NSError* error = nil;
  NSString* content = [NSString stringWithContentsOfFile:path
                                                encoding:NSUTF8StringEncoding
                                                   error:&error];
  if (content.length == 0) {
    const char* errorDescription = error.description.UTF8String ?: "";
    fprintf(stderr, "Failed to load external SVG path=%s error=%s\n",
            path.UTF8String ?: "", errorDescription);
  }
  return content;
}

- (NSString*)hostColorForFile:(NSString*)fileName {
  if ([fileName isEqualToString:kHostColorCompareFile] ||
      [fileName isEqualToString:kHostColorOverrideFile]) {
    return kHostDefaultColor;
  }
  return nil;
}

- (CGFloat)heightForText:(NSString*)text
                    font:(NSFont*)font
                   width:(CGFloat)width {
  if (text.length == 0 || width <= 0.0) {
    return 0.0;
  }
  NSRect rect =
      [text boundingRectWithSize:NSMakeSize(width, CGFLOAT_MAX)
                         options:NSStringDrawingUsesLineFragmentOrigin |
                                 NSStringDrawingUsesFontLeading
                      attributes:@{NSFontAttributeName: font}];
  return ceil(NSHeight(rect));
}

- (NSTextField*)labelWithText:(NSString*)text
                         font:(NSFont*)font
                    textColor:(NSColor*)textColor
                        frame:(NSRect)frame {
  NSTextField* label = [[NSTextField alloc] initWithFrame:frame];
  label.stringValue = text ?: @"";
  label.editable = NO;
  label.bezeled = NO;
  label.drawsBackground = NO;
  label.selectable = NO;
  label.font = font;
  label.textColor = textColor;
  label.lineBreakMode = NSLineBreakByWordWrapping;
  label.usesSingleLineMode = NO;
  label.maximumNumberOfLines = 0;
  NSTextFieldCell* cell = (NSTextFieldCell*)label.cell;
  cell.wraps = YES;
  cell.scrollable = NO;
  return label;
}

- (void)applyCategoryPopUpStyle {
  NSColor* titleColor = [NSColor colorWithRed:32.0 / 255.0
                                        green:33.0 / 255.0
                                         blue:36.0 / 255.0
                                        alpha:1.0];
  NSColor* borderColor = [NSColor colorWithRed:120.0 / 255.0
                                         green:126.0 / 255.0
                                          blue:136.0 / 255.0
                                         alpha:1.0];
  NSColor* backgroundColor = NSColor.whiteColor;
  NSFont* titleFont = [NSFont systemFontOfSize:13.0 weight:NSFontWeightMedium];
  NSDictionary* attributes = @{
    NSForegroundColorAttributeName: titleColor,
    NSFontAttributeName: titleFont
  };

  for (NSMenuItem* item in self.categoryPopUpButton.itemArray) {
    item.attributedTitle =
        [[NSAttributedString alloc] initWithString:item.title
                                        attributes:attributes];
  }

  self.categoryPopUpButton.font = titleFont;
  self.categoryPopUpButton.contentTintColor = titleColor;
  self.categoryPopUpButton.bordered = NO;
  self.categoryPopUpButton.wantsLayer = YES;
  self.categoryPopUpButton.layer.backgroundColor = backgroundColor.CGColor;
  self.categoryPopUpButton.layer.borderColor = borderColor.CGColor;
  self.categoryPopUpButton.layer.borderWidth = 1.0;
  self.categoryPopUpButton.layer.cornerRadius = 6.0;
  self.categoryPopUpButton.layer.masksToBounds = YES;
}

- (void)applyRenderModeControlStyle {
  NSColor* borderColor = [NSColor colorWithRed:75.0 / 255.0
                                         green:82.0 / 255.0
                                          blue:95.0 / 255.0
                                         alpha:1.0];
  NSColor* backgroundColor = [NSColor colorWithRed:226.0 / 255.0
                                             green:230.0 / 255.0
                                              blue:238.0 / 255.0
                                             alpha:1.0];
  self.renderModeControl.segmentStyle = NSSegmentStyleRounded;
  self.renderModeControl.font = [NSFont systemFontOfSize:13.0
                                                  weight:NSFontWeightMedium];
  [self.renderModeControl setLabel:@"Hardware" forSegment:0];
  [self.renderModeControl setLabel:@"Software" forSegment:1];
  [self.renderModeControl setWidth:92.0 forSegment:0];
  [self.renderModeControl setWidth:112.0 forSegment:1];
  self.renderModeControl.wantsLayer = YES;
  self.renderModeControl.layer.backgroundColor = backgroundColor.CGColor;
  self.renderModeControl.layer.borderColor = borderColor.CGColor;
  self.renderModeControl.layer.borderWidth = 1.0;
  self.renderModeControl.layer.cornerRadius = 6.0;
  self.renderModeControl.layer.masksToBounds = YES;
}

- (void)scrollToPendingTargetIfNeeded {
  if (self.pendingScrollTarget.length == 0) {
    return;
  }
  NSValue* rowValue = self.rowFrames[self.pendingScrollTarget];
  if (!rowValue) {
    self.pendingScrollTarget = nil;
    return;
  }
  NSRect rowFrame = rowValue.rectValue;
  CGFloat maxOffset = MAX(NSHeight(self.documentView.frame) -
                              NSHeight(self.scrollView.contentView.bounds),
                          0.0);
  CGFloat targetY = MIN(MAX(NSMinY(rowFrame) - 12.0, 0.0), maxOffset);
  [self.scrollView.contentView scrollToPoint:NSMakePoint(0.0, targetY)];
  [self.scrollView reflectScrolledClipView:self.scrollView.contentView];
  self.pendingScrollTarget = nil;
}

- (void)reloadPreviewRows {
  const CGFloat cardGap = 12.0;
  const CGFloat cardPadding = 12.0;
  const CGFloat cardRadius = 8.0;
  NSColor* cardBackgroundColor = NSColor.whiteColor;
  NSColor* titleColor = [NSColor colorWithRed:32.0 / 255.0
                                        green:33.0 / 255.0
                                         blue:36.0 / 255.0
                                        alpha:1.0];
  NSColor* subtitleColor = [NSColor colorWithRed:95.0 / 255.0
                                           green:99.0 / 255.0
                                            blue:104.0 / 255.0
                                           alpha:1.0];
  NSColor* previewBorderColor = [NSColor colorWithRed:217.0 / 255.0
                                                green:220.0 / 255.0
                                                 blue:227.0 / 255.0
                                                alpha:1.0];

  for (NSView* subview in [self.documentView.subviews copy]) {
    [subview removeFromSuperview];
  }

  NSArray<NSString*>* files = [self filesForSelectedCategory];
  CGFloat contentWidth = NSWidth(self.scrollView.contentView.bounds);
  CGFloat rowWidth = MAX(contentWidth - 32.0, 0.0);
  CGFloat previewWidth = MIN(320.0, MAX(rowWidth - 24.0, 180.0));
  CGFloat previewHeight = 195.0;
  CGFloat yOffset = cardGap;
  NSMutableDictionary<NSString*, NSValue*>* rowFrames =
      [NSMutableDictionary dictionary];

  NSMutableArray<NSDictionary*>* previewItems = [NSMutableArray array];
  if (self.externalSVGPath.length > 0) {
    NSString* externalContent =
        [self svgContentForExternalPath:self.externalSVGPath];
    if (externalContent.length > 0) {
      [previewItems addObject:@{
        @"key" : kExternalPreviewKey,
        @"fileName" : self.externalSVGDisplayName ?: self.externalSVGPath.lastPathComponent ?: @"External SVG",
        @"title" : self.externalSVGDisplayName ?: self.externalSVGPath.lastPathComponent ?: @"External SVG",
        @"caseText" : [NSString stringWithFormat:@"external: %@", self.externalSVGPath.lastPathComponent ?: self.externalSVGPath],
        @"tagsText" : @"tags: External",
        @"descriptionText" : @"1. Loaded from a local file path.\n2. Previewed with the same Metal rendering pipeline as the built-in cases.",
        @"content" : externalContent,
        @"hostColor" : @""
      }];
    }
  }

  for (NSString* fileName in files) {
    SrSVGPreviewMetadata* metadata = [self previewMetadataForFile:fileName];
    NSString* titleText = metadata ? metadata.title : fileName;
    NSString* caseText = [NSString
        stringWithFormat:@"case: %@", [fileName stringByDeletingPathExtension]];
    NSString* tagsText =
        metadata.tags.count > 0
            ? [NSString stringWithFormat:@"tags: %@",
                                         [metadata.tags
                                             componentsJoinedByString:@", "]]
            : @"";
    NSMutableArray<NSString*>* descriptionLines = [NSMutableArray array];
    [metadata.descriptions
        enumerateObjectsUsingBlock:^(NSString* _Nonnull description,
                                     NSUInteger idx, BOOL* _Nonnull stop) {
          [descriptionLines
              addObject:[NSString stringWithFormat:@"%lu. %@",
                                                   (unsigned long)idx + 1,
                                                   description]];
        }];
    NSString* descriptionText =
        descriptionLines.count > 0
            ? [descriptionLines componentsJoinedByString:@"\n"]
            : @"";
    NSString* content = [self svgContentForFile:fileName];
    [previewItems addObject:@{
      @"key": fileName,
      @"fileName": fileName,
      @"title": titleText,
      @"caseText": caseText,
      @"tagsText": tagsText,
      @"descriptionText": descriptionText,
      @"content": content ?: @"",
      @"hostColor": [self hostColorForFile:fileName] ?: @""
    }];
  }

  for (NSDictionary* item in previewItems) {
    NSString* key = item[@"key"];
    NSString* titleText = item[@"title"];
    NSString* caseText = item[@"caseText"];
    NSString* tagsText = item[@"tagsText"];
    NSString* descriptionText = item[@"descriptionText"];
    NSString* content = item[@"content"];
    NSString* hostColor = item[@"hostColor"];

    CGFloat textWidth = MAX(rowWidth - 24.0, 0.0);
    CGFloat currentY = cardPadding;
    NSFont* titleFont = [NSFont systemFontOfSize:16.0
                                          weight:NSFontWeightSemibold];
    NSFont* bodyFont = [NSFont systemFontOfSize:12.0];
    CGFloat titleHeight = [self heightForText:titleText
                                         font:titleFont
                                        width:textWidth];
    currentY += titleHeight + 4.0;
    CGFloat caseHeight = [self heightForText:caseText
                                        font:bodyFont
                                       width:textWidth];
    currentY += caseHeight;
    CGFloat tagsHeight = 0.0;
    if (tagsText.length > 0) {
      tagsHeight = [self heightForText:tagsText font:bodyFont width:textWidth];
      currentY += 4.0 + tagsHeight;
    }
    CGFloat descriptionHeight = 0.0;
    if (descriptionText.length > 0) {
      descriptionHeight = [self heightForText:descriptionText
                                         font:bodyFont
                                        width:textWidth];
      currentY += 8.0 + descriptionHeight;
    }
    currentY += 12.0;
    CGFloat rowHeight = currentY + previewHeight + cardPadding;
    NSView* row = [[SrSVGFlippedView alloc]
        initWithFrame:NSMakeRect(16.0, yOffset, rowWidth, rowHeight)];
    row.wantsLayer = YES;
    row.layer.backgroundColor = cardBackgroundColor.CGColor;
    row.layer.cornerRadius = cardRadius;

    NSTextField* titleLabel =
        [self labelWithText:titleText
                       font:titleFont
                  textColor:titleColor
                      frame:NSMakeRect(cardPadding, cardPadding, textWidth,
                                       titleHeight)];
    [row addSubview:titleLabel];

    NSTextField* caseLabel = [self
        labelWithText:caseText
                 font:bodyFont
            textColor:subtitleColor
                frame:NSMakeRect(cardPadding, NSMaxY(titleLabel.frame) + 4.0,
                                 textWidth, caseHeight)];
    [row addSubview:caseLabel];

    CGFloat metadataBottom = NSMaxY(caseLabel.frame);
    if (tagsText.length > 0) {
      NSTextField* tagsLabel =
          [self labelWithText:tagsText
                         font:bodyFont
                    textColor:subtitleColor
                        frame:NSMakeRect(cardPadding, metadataBottom + 4.0,
                                         textWidth, tagsHeight)];
      [row addSubview:tagsLabel];
      metadataBottom = NSMaxY(tagsLabel.frame);
    }
    if (descriptionText.length > 0) {
      NSTextField* descriptionLabel =
          [self labelWithText:descriptionText
                         font:bodyFont
                    textColor:titleColor
                        frame:NSMakeRect(cardPadding, metadataBottom + 8.0,
                                         textWidth, descriptionHeight)];
      [row addSubview:descriptionLabel];
      metadataBottom = NSMaxY(descriptionLabel.frame);
    }

    CGFloat previewX = MAX((rowWidth - previewWidth) / 2.0, cardPadding);
    SVGMetalView* previewView = [[SVGMetalView alloc]
        initWithFrame:NSMakeRect(previewX, metadataBottom + 12.0, previewWidth,
                                 previewHeight)];
    previewView.color = hostColor.length > 0 ? hostColor : nil;
    previewView.useSoftwareCanvas = self.useSoftwareCanvas;
    previewView.wantsLayer = YES;
    previewView.metalLayer.borderWidth = 1.0;
    previewView.metalLayer.borderColor = previewBorderColor.CGColor;
    previewView.metalLayer.backgroundColor = NSColor.clearColor.CGColor;
    [previewView setSVGContent:content];
    [row addSubview:previewView];

    [self.documentView addSubview:row];
    rowFrames[key] = [NSValue valueWithRect:row.frame];
    yOffset += rowHeight + cardGap;
  }

  self.rowFrames = rowFrames;
  self.documentView.frame = NSMakeRect(
      0.0, 0.0, contentWidth, MAX(yOffset, NSHeight(self.scrollView.bounds)));
  [self scrollToPendingTargetIfNeeded];
}

- (void)setupViews {
  self.categoryPopUpButton = [[NSPopUpButton alloc] initWithFrame:NSZeroRect
                                                        pullsDown:NO];
  [self.categoryPopUpButton addItemsWithTitles:self.categories];
  [self applyCategoryPopUpStyle];
  self.categoryPopUpButton.target = self;
  self.categoryPopUpButton.action = @selector(categoryDidChange:);
  [self.view addSubview:self.categoryPopUpButton];

  self.renderModeControl =
      [[NSSegmentedControl alloc] initWithFrame:NSZeroRect];
  self.renderModeControl.segmentCount = 2;
  self.renderModeControl.trackingMode = NSSegmentSwitchTrackingSelectOne;
  self.renderModeControl.selectedSegment = 0;
  [self applyRenderModeControlStyle];
  self.renderModeControl.target = self;
  self.renderModeControl.action = @selector(renderModeDidChange:);
  [self.view addSubview:self.renderModeControl];

  self.scrollView = [[NSScrollView alloc] initWithFrame:NSZeroRect];
  self.scrollView.hasVerticalScroller = YES;
  self.scrollView.borderType = NSNoBorder;
  self.scrollView.drawsBackground = NO;
  self.scrollView.autohidesScrollers = NO;
  self.scrollView.scrollerStyle = NSScrollerStyleLegacy;

  self.documentView = [[SrSVGFlippedView alloc] initWithFrame:NSZeroRect];
  self.documentView.wantsLayer = YES;
  self.documentView.layer.backgroundColor = NSColor.clearColor.CGColor;
  self.scrollView.documentView = self.documentView;
  self.scrollView.verticalScroller.knobStyle = NSScrollerKnobStyleDark;
  [self.view addSubview:self.scrollView];
}

- (NSArray<NSString*>*)loadBuiltInSVGFiles {
  NSMutableArray<NSString*>* svgFiles = [NSMutableArray array];
  NSString* svgDirectory = [[[NSBundle mainBundle] resourcePath]
      stringByAppendingPathComponent:@"svg"];
  NSArray<NSString*>* bundleFiles =
      [[NSFileManager defaultManager] contentsOfDirectoryAtPath:svgDirectory
                                                          error:nil];
  NSArray<NSString*>* sortedBundleFiles = [bundleFiles
      sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
  for (NSString* fileName in sortedBundleFiles) {
    if ([[fileName pathExtension] isEqualToString:@"svg"] &&
        ![fileName isEqualToString:@"string_test.svg"]) {
      [svgFiles addObject:fileName];
    }
  }
  [svgFiles addObject:@"string_test.svg"];
  return svgFiles;
}

- (void)viewDidLoad {
  [super viewDidLoad];

  self.categories = [self orderedCategories];
  self.categorizedFiles =
      [self buildCategorizedFiles:[self loadBuiltInSVGFiles]];
  self.selectedCategory = kCategoryShape;
  self.lastContentWidth = -1.0;
  self.useSoftwareCanvas = NO;

  [self setupViews];

  NSInteger categoryIndex =
      [self.categories indexOfObject:self.selectedCategory];
  [self.categoryPopUpButton selectItemAtIndex:MAX(categoryIndex, 0)];
}

- (void)viewDidLayout {
  [super viewDidLayout];

  const CGFloat horizontalInset = 16.0;
  const CGFloat topInset = 16.0;
  const CGFloat controlHeight = 32.0;
  const CGFloat spacing = 12.0;
  CGFloat viewWidth = NSWidth(self.view.bounds);
  CGFloat viewHeight = NSHeight(self.view.bounds);

  self.categoryPopUpButton.frame =
      NSMakeRect(horizontalInset, viewHeight - topInset - controlHeight, 220.0,
                 controlHeight);
  self.renderModeControl.frame =
      NSMakeRect(NSMaxX(self.categoryPopUpButton.frame) + spacing,
                 viewHeight - topInset - controlHeight, 204.0, controlHeight);
  self.scrollView.frame =
      NSMakeRect(0.0, 0.0, viewWidth,
                 MAX(viewHeight - topInset - controlHeight - spacing, 0.0));

  CGFloat contentWidth = NSWidth(self.scrollView.contentView.bounds);
  if (fabs(contentWidth - self.lastContentWidth) > 0.5) {
    self.lastContentWidth = contentWidth;
    [self reloadPreviewRows];
    return;
  }

  if (self.documentView.subviews.count == 0) {
    [self reloadPreviewRows];
  }
}

- (void)categoryDidChange:(id)sender {
  NSString* selectedTitle = self.categoryPopUpButton.titleOfSelectedItem;
  if (selectedTitle.length == 0 ||
      [selectedTitle isEqualToString:self.selectedCategory]) {
    return;
  }
  self.selectedCategory = selectedTitle;
  self.pendingScrollTarget = nil;
  [self reloadPreviewRows];
}

- (void)applyRenderModeToPreviewViewsInView:(NSView*)view {
  if ([view isKindOfClass:[SVGMetalView class]]) {
    SVGMetalView* previewView = (SVGMetalView*)view;
    previewView.useSoftwareCanvas = self.useSoftwareCanvas;
    [previewView render];
    return;
  }
  for (NSView* subview in view.subviews) {
    [self applyRenderModeToPreviewViewsInView:subview];
  }
}

- (void)renderModeDidChange:(id)sender {
  BOOL useSoftware = self.renderModeControl.selectedSegment == 1;
  if (self.useSoftwareCanvas == useSoftware) {
    return;
  }
  self.useSoftwareCanvas = useSoftware;
  [self applyRenderModeToPreviewViewsInView:self.documentView];
}

- (void)loadSVGFile:(NSString*)fileName {
  if (fileName.length == 0) {
    return;
  }
  self.externalSVGPath = nil;
  self.externalSVGDisplayName = nil;
  self.selectedCategory = [self categoryForFile:fileName];
  NSInteger categoryIndex =
      [self.categories indexOfObject:self.selectedCategory];
  if (categoryIndex != NSNotFound) {
    [self.categoryPopUpButton selectItemAtIndex:categoryIndex];
  }
  self.pendingScrollTarget = fileName;
  [self reloadPreviewRows];
}

- (void)openSVGAtPath:(NSString*)path {
  if (path.length == 0) {
    return;
  }
  self.externalSVGPath = [path copy];
  self.externalSVGDisplayName = path.lastPathComponent;
  self.pendingScrollTarget = kExternalPreviewKey;
  [self reloadPreviewRows];
}

@end
