// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ViewController.h"
#import <ServalMarkdown/serval_markdown_view.h>
@interface ViewController ()
@property(nonatomic, strong) ServalMarkdownView* markdown;
@end

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  self.markdown = [[ServalMarkdownView alloc] init];
  self.markdown.backgroundColor = [UIColor whiteColor];
  self.markdown.content = @"This is a **Markdown**!!";
  self.markdown.style = @{};
  self.markdown.frame = CGRectMake(100, 100, 600, 1000);
  [self.markdown requestMeasure];
  [self.view addSubview:self.markdown];
  self.view.backgroundColor = [UIColor whiteColor];
  [self.view setNeedsDisplay];
}

@end
