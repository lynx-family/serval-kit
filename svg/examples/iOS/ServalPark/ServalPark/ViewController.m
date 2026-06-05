// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ViewController.h"
#import <ServalSVG/SrSVGView.h>

@interface ViewController () <UIPickerViewDelegate, UIPickerViewDataSource>

@property(nonatomic, strong) UIPickerView* pickerView;
@property(nonatomic, strong) UIScrollView* scrollView;
@property(nonatomic, strong) NSArray<NSString*>* categories;
@property(nonatomic, strong)
    NSDictionary<NSString*, NSArray<NSString*>*>* categorizedFiles;
@property(nonatomic, copy) NSString* selectedCategory;

@end

@interface SrSVGPreviewMetadata : NSObject
@property(nonatomic, copy) NSString* title;
@property(nonatomic, copy) NSArray<NSString*>* tags;
@property(nonatomic, copy) NSArray<NSString*>* descriptions;
@end

@implementation SrSVGPreviewMetadata
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
static NSArray<NSString*>* kPreviewMetadataFiles() {
  return @[
    @"svg_root_metadata", @"svg_shape_metadata", @"svg_use_metadata",
    @"svg_gradient_metadata"
  ];
}

- (NSArray<NSString*>*)orderedCategories {
  return @[
    kCategoryShape, kCategoryColorParsing, kCategoryCurrentColor, kCategoryMask,
    kCategoryFilter, kCategoryIllegalParsing, kCategoryPattern,
    kCategorySvgRoot, kCategoryUse, kCategoryGradient, kCategoryVectorEffect,
    kCategoryOthers
  ];
}

- (SrSVGPreviewMetadata*)metadataWithTitle:(NSString*)title
                                      tags:(NSArray<NSString*>*)tags
                              descriptions:(NSArray<NSString*>*)descriptions {
  SrSVGPreviewMetadata* metadata = [[SrSVGPreviewMetadata alloc] init];
  metadata.title = title;
  metadata.tags = tags;
  metadata.descriptions = descriptions;
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
    NSDictionary* json = [NSJSONSerialization JSONObjectWithData:data
                                                         options:0
                                                           error:&error];
    if (![json isKindOfClass:[NSDictionary class]]) {
      continue;
    }
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
          [caseInfo[@"tags"] isKindOfClass:[NSArray class]] ? caseInfo[@"tags"]
                                                            : @[];
      NSArray<NSString*>* descriptions =
          [caseInfo[@"descriptions"] isKindOfClass:[NSArray class]]
              ? caseInfo[@"descriptions"]
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

- (void)reloadPreviewRows {
  const CGFloat cardGap = 12.0;
  const CGFloat cardPadding = 12.0;
  const CGFloat cardRadius = 8.0;
  UIColor* cardBackgroundColor = [UIColor whiteColor];
  UIColor* titleColor = [UIColor colorWithRed:32.0 / 255.0
                                        green:33.0 / 255.0
                                         blue:36.0 / 255.0
                                        alpha:1.0];
  UIColor* previewBorderColor = [UIColor colorWithRed:217.0 / 255.0
                                                green:220.0 / 255.0
                                                 blue:227.0 / 255.0
                                                alpha:1.0];
  for (UIView* subview in self.scrollView.subviews) {
    [subview removeFromSuperview];
  }

  NSArray<NSString*>* files = [self filesForSelectedCategory];
  CGFloat contentWidth = self.scrollView.bounds.size.width;
  CGFloat rowWidth = MAX(contentWidth - 32.0, 0.0);
  CGFloat previewWidth = 260.0;
  CGFloat previewHeight = 195.0;
  CGFloat yOffset = cardGap;

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
            : nil;
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
            : nil;
    CGFloat textWidth = rowWidth - 24.0;
    CGSize unlimitedSize = CGSizeMake(textWidth, CGFLOAT_MAX);
    CGFloat currentY = cardPadding;
    UILabel* measureLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    measureLabel.numberOfLines = 0;
    measureLabel.font = [UIFont systemFontOfSize:16.0
                                          weight:UIFontWeightSemibold];
    measureLabel.text = titleText;
    CGFloat titleHeight =
        ceil([measureLabel sizeThatFits:unlimitedSize].height);
    currentY += titleHeight + 4.0;
    measureLabel.font = [UIFont systemFontOfSize:12.0];
    measureLabel.text = caseText;
    CGFloat caseHeight = ceil([measureLabel sizeThatFits:unlimitedSize].height);
    currentY += caseHeight;
    CGFloat tagsHeight = 0.0;
    if (tagsText.length > 0) {
      measureLabel.text = tagsText;
      tagsHeight = ceil([measureLabel sizeThatFits:unlimitedSize].height);
      currentY += 4.0 + tagsHeight;
    }
    CGFloat descriptionHeight = 0.0;
    if (descriptionText.length > 0) {
      measureLabel.text = descriptionText;
      descriptionHeight =
          ceil([measureLabel sizeThatFits:unlimitedSize].height);
      currentY += 8.0 + descriptionHeight;
    }
    currentY += 12.0;
    CGFloat rowHeight = currentY + previewHeight + cardPadding;
    UIView* row = [[UIView alloc]
        initWithFrame:CGRectMake(16.0, yOffset, rowWidth, rowHeight)];
    row.backgroundColor = cardBackgroundColor;
    row.layer.cornerRadius = cardRadius;

    UILabel* titleLabel =
        [[UILabel alloc] initWithFrame:CGRectMake(cardPadding, cardPadding,
                                                  textWidth, titleHeight)];
    titleLabel.text = titleText;
    titleLabel.numberOfLines = 0;
    titleLabel.font = [UIFont systemFontOfSize:16.0
                                        weight:UIFontWeightSemibold];
    titleLabel.textColor = titleColor;
    [row addSubview:titleLabel];

    UILabel* caseLabel = [[UILabel alloc]
        initWithFrame:CGRectMake(cardPadding,
                                 CGRectGetMaxY(titleLabel.frame) + 4.0,
                                 textWidth, caseHeight)];
    caseLabel.text = caseText;
    caseLabel.numberOfLines = 1;
    caseLabel.font = [UIFont systemFontOfSize:12.0];
    caseLabel.textColor = [UIColor colorWithRed:95.0 / 255.0
                                          green:99.0 / 255.0
                                           blue:104.0 / 255.0
                                          alpha:1.0];
    [row addSubview:caseLabel];

    CGFloat metadataBottom = CGRectGetMaxY(caseLabel.frame);
    if (tagsText.length > 0) {
      UILabel* tagsLabel = [[UILabel alloc]
          initWithFrame:CGRectMake(cardPadding, metadataBottom + 4.0, textWidth,
                                   tagsHeight)];
      tagsLabel.text = tagsText;
      tagsLabel.numberOfLines = 0;
      tagsLabel.font = [UIFont systemFontOfSize:12.0];
      tagsLabel.textColor = caseLabel.textColor;
      [row addSubview:tagsLabel];
      metadataBottom = CGRectGetMaxY(tagsLabel.frame);
    }
    if (descriptionText.length > 0) {
      UILabel* descriptionLabel = [[UILabel alloc]
          initWithFrame:CGRectMake(cardPadding, metadataBottom + 8.0, textWidth,
                                   descriptionHeight)];
      descriptionLabel.text = descriptionText;
      descriptionLabel.numberOfLines = 0;
      descriptionLabel.font = [UIFont systemFontOfSize:12.0];
      descriptionLabel.textColor = titleColor;
      [row addSubview:descriptionLabel];
      metadataBottom = CGRectGetMaxY(descriptionLabel.frame);
    }

    CGFloat previewX = (rowWidth - previewWidth) / 2.0;
    SrSVGView* previewView = [[SrSVGView alloc]
        initWithFrame:CGRectMake(previewX, metadataBottom + 12.0, previewWidth,
                                 previewHeight)];
    previewView.color = [self hostColorForFile:fileName];
    previewView.opaque = NO;
    previewView.layer.borderWidth = 1.0;
    previewView.layer.borderColor = previewBorderColor.CGColor;
    previewView.backgroundColor = [UIColor clearColor];

    NSString* content = [self svgContentForFile:fileName];
    if (content.length > 0) {
      SrSVGRenderResult* result = [previewView parseContentWithResult:content];
      if (result.hasError) {
        NSLog(@"SVGDiagnostic file=%@ errorMessage=%@", fileName,
              result.errorMessage);
      }
    }
    [row addSubview:previewView];
    [self.scrollView addSubview:row];
    yOffset += rowHeight + cardGap;
  }

  self.scrollView.contentSize = CGSizeMake(
      contentWidth, MAX(yOffset, self.scrollView.bounds.size.height));
}

- (NSString*)hostColorForFile:(NSString*)fileName {
  if ([fileName isEqualToString:kHostColorCompareFile] ||
      [fileName isEqualToString:kHostColorOverrideFile]) {
    return kHostDefaultColor;
  }
  return nil;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.backgroundColor = [UIColor colorWithWhite:0.96 alpha:1.0];

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
  self.categories = [self orderedCategories];
  self.categorizedFiles = [self buildCategorizedFiles:svgFiles];
  self.selectedCategory = kCategoryShape;

  // Setup Picker View
  self.pickerView = [[UIPickerView alloc]
      initWithFrame:CGRectMake(0, 50, self.view.bounds.size.width, 120)];
  self.pickerView.delegate = self;
  self.pickerView.dataSource = self;
  [self.view addSubview:self.pickerView];

  self.scrollView = [[UIScrollView alloc]
      initWithFrame:CGRectMake(0, 170, self.view.bounds.size.width,
                               self.view.bounds.size.height - 170)];
  self.scrollView.alwaysBounceVertical = YES;
  self.scrollView.backgroundColor = [UIColor clearColor];
  [self.view addSubview:self.scrollView];

  NSInteger categoryIndex =
      [self.categories indexOfObject:self.selectedCategory];
  [self.pickerView selectRow:MAX(categoryIndex, 0) inComponent:0 animated:NO];
  [self reloadPreviewRows];
}

#pragma mark - UIPickerViewDataSource

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView*)pickerView {
  return 1;
}

- (NSInteger)pickerView:(UIPickerView*)pickerView
    numberOfRowsInComponent:(NSInteger)component {
  return self.categories.count;
}

#pragma mark - UIPickerViewDelegate

- (NSString*)pickerView:(UIPickerView*)pickerView
            titleForRow:(NSInteger)row
           forComponent:(NSInteger)component {
  return self.categories[row];
}

- (void)pickerView:(UIPickerView*)pickerView
      didSelectRow:(NSInteger)row
       inComponent:(NSInteger)component {
  self.selectedCategory = self.categories[row];
  [self reloadPreviewRows];
}

@end
