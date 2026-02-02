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
  self.markdown.content = @"This is a **Markdown**!! This is a **Markdown**!! "
                          @"This is a **Markdown**!!";
  self.markdown.style = @{};
  self.markdown.animationType = kServalMarkdownAnimationTypeTypewriter;
  self.markdown.animationVelocity = 10;
  self.markdown.initialAnimationStep = 5;
  [self.view addSubview:self.markdown];
  self.markdown.translatesAutoresizingMaskIntoConstraints = NO;
  [NSLayoutConstraint activateConstraints:@[
    [self.markdown.leadingAnchor
        constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor
                       constant:16.0],
    [self.markdown.trailingAnchor
        constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor
                       constant:-16.0],
    [self.markdown.topAnchor
        constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor
                       constant:24.0],
  ]];
  [self.markdown setContentHuggingPriority:UILayoutPriorityRequired
                                   forAxis:UILayoutConstraintAxisVertical];
  [self.markdown
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisVertical];
  [self.markdown requestMeasure];
  self.view.backgroundColor = [UIColor whiteColor];
  [self.view setNeedsLayout];
}

@end
