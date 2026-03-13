// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ViewController.h"

#import <SrSVGView.h>

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  // Do any additional setup after loading the view.
  SrSVGView* svgView = [[SrSVGView alloc]
      initWithString:
          @"<svg viewBox=\"0 0 100 100\">\n"
           "<path\n    fill=\"none\"\n    stroke=\"#f00\"\n    d=\"M 10,30\n   "
           "    A 20,20 0,0,1 50,30\n       A 20,20 0,0,1 90,30\n       Q "
           "90,60 50,90\n       Q 10,60 10,30 z\" />"
           "  <polyline points=\"0,100 50,25 50,75 100,0\" fill=\"#000\" />\n"
           "\n"
           "  <polyline points=\"100,100 150,25 150,75 200,0\" fill=\"none\" "
           "stroke=\"#f00\" />"
           "</svg>"];

  [self.view addSubview:svgView];
  [svgView setOpaque:NO];
  [svgView setFrame:CGRectMake(30, 30, 200, 200)];
  //  [svgView setBackgroundColor:[UIColor blueColor]];
  [svgView setNeedsDisplay];
}

@end
