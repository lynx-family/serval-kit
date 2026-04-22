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

@implementation ViewController

static NSString* const kHostColorCompareFile =
    @"currentcolor-host-default-compare.svg";
static NSString* const kHostColorOverrideFile =
    @"currentcolor-content-color-override.svg";
static NSString* const kHostDefaultColor = @"#4F6BFF";
static NSString* const kCategoryCore = @"Core";
static NSString* const kCategoryColorParsing = @"ColorParsing";
static NSString* const kCategoryCurrentColor = @"CurrentColor";
static NSString* const kCategoryMask = @"Mask";
static NSString* const kCategoryPattern = @"Pattern";
static NSString* const kCategoryVectorEffect = @"VectorEffect";

- (NSArray<NSString*>*)orderedCategories {
  return @[
    kCategoryCore, kCategoryColorParsing, kCategoryCurrentColor, kCategoryMask,
    kCategoryPattern, kCategoryVectorEffect
  ];
}

- (NSString*)categoryForFile:(NSString*)fileName {
  if ([fileName hasPrefix:@"color-parsing-"]) {
    return kCategoryColorParsing;
  }
  if ([fileName hasPrefix:@"currentcolor-"]) {
    return kCategoryCurrentColor;
  }
  if ([fileName hasPrefix:@"mask-"]) {
    return kCategoryMask;
  }
  if ([fileName hasPrefix:@"pattern-"] ||
      [fileName isEqualToString:@"stroke-gradient-vs-pattern.svg"]) {
    return kCategoryPattern;
  }
  if ([fileName hasPrefix:@"vector-effect-"]) {
    return kCategoryVectorEffect;
  }
  return kCategoryCore;
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
  CGFloat rowHeight = 251.0;
  CGFloat yOffset = cardGap;

  for (NSString* fileName in files) {
    UIView* row = [[UIView alloc]
        initWithFrame:CGRectMake(16.0, yOffset, rowWidth, rowHeight)];
    row.backgroundColor = cardBackgroundColor;
    row.layer.cornerRadius = cardRadius;

    UILabel* titleLabel =
        [[UILabel alloc] initWithFrame:CGRectMake(cardPadding, cardPadding,
                                                  rowWidth - 24.0, 20.0)];
    titleLabel.text = fileName;
    titleLabel.numberOfLines = 1;
    titleLabel.font = [UIFont systemFontOfSize:14.0];
    titleLabel.textColor = titleColor;
    [row addSubview:titleLabel];

    CGFloat previewX = (rowWidth - previewWidth) / 2.0;
    SrSVGView* previewView = [[SrSVGView alloc]
        initWithFrame:CGRectMake(previewX, 44.0, previewWidth, previewHeight)];
    previewView.color = [self hostColorForFile:fileName];
    previewView.opaque = NO;
    previewView.layer.borderWidth = 1.0;
    previewView.layer.borderColor = previewBorderColor.CGColor;
    previewView.backgroundColor = [UIColor clearColor];

    NSString* content = [self svgContentForFile:fileName];
    if (content.length > 0) {
      [previewView parseContent:content];
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
  self.selectedCategory = [self categoryForFile:@"basic_shapes.svg"];

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
