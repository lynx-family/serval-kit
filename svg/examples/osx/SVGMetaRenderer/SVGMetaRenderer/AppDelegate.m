// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "AppDelegate.h"
#import "ViewController.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  NSRect frame = NSMakeRect(0.0, 0.0, 1100.0, 760.0);
  self.window = [[NSWindow alloc]
      initWithContentRect:frame
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskResizable |
                          NSWindowStyleMaskMiniaturizable
                  backing:NSBackingStoreBuffered
                    defer:NO];
  [self.window setTitle:@"Serval SVG Examples"];
  [self.window center];

  ViewController* viewController = [[ViewController alloc] init];
  [self.window setContentViewController:viewController];

  [self setupMenu];

  [self.window makeKeyAndOrderFront:nil];
  [NSApp activateIgnoringOtherApps:YES];
}

- (void)setupMenu {
  NSMenu* mainMenu = [[NSMenu alloc] initWithTitle:@""];
  [NSApp setMainMenu:mainMenu];

  NSMenuItem* appMenuItem = [[NSMenuItem alloc] initWithTitle:@"App"
                                                       action:nil
                                                keyEquivalent:@""];
  [mainMenu addItem:appMenuItem];
  NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"App"];
  [appMenuItem setSubmenu:appMenu];

  NSString* appName =
      NSProcessInfo.processInfo.processName ?: @"SVGMetaRenderer";
  NSMenuItem* quitItem = [[NSMenuItem alloc]
      initWithTitle:[NSString stringWithFormat:@"Quit %@", appName]
             action:@selector(terminate:)
      keyEquivalent:@"q"];
  [appMenu addItem:quitItem];

  NSMenuItem* fileMenuItem = [[NSMenuItem alloc] initWithTitle:@"File"
                                                        action:nil
                                                 keyEquivalent:@""];
  [mainMenu addItem:fileMenuItem];
  NSMenu* fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
  [fileMenuItem setSubmenu:fileMenu];

  NSMenuItem* openItem = [[NSMenuItem alloc] initWithTitle:@"Open..."
                                                    action:@selector(openFile:)
                                             keyEquivalent:@"o"];
  [openItem setTarget:self];
  [fileMenu addItem:openItem];
}

- (void)openFile:(id)sender {
  NSOpenPanel* openPanel = [NSOpenPanel openPanel];
  openPanel.canChooseFiles = YES;
  openPanel.canChooseDirectories = NO;
  openPanel.allowsMultipleSelection = NO;
  openPanel.allowedFileTypes = @[@"svg"];
  openPanel.title = @"Open SVG File";

  [openPanel beginSheetModalForWindow:self.window
                    completionHandler:^(NSModalResponse result) {
                      if (result != NSModalResponseOK) {
                        return;
                      }
                      NSString* filePath = openPanel.URL.path;
                      if (filePath.length == 0) {
                        return;
                      }
                      ViewController* viewController =
                          (ViewController*)self.window.contentViewController;
                      [viewController openSVGAtPath:filePath];
                    }];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  return YES;
}

@end
