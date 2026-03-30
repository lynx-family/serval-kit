// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "AppDelegate.h"
#import "ViewController.h"
#import "SVGMetalView.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                  backing:NSBackingStoreBuffered
                                                    defer:NO];
    [self.window setTitle:@"SVG Metal Renderer"];
    [self.window center];
    
    ViewController *viewController = [[ViewController alloc] init];
    [self.window setContentViewController:viewController];
    
    [self setupMenu];
    
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)setupMenu {
    NSMenu *mainMenu = [NSApp mainMenu];
    
    // File menu
    NSMenuItem *fileMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:fileMenuItem];
    
    NSMenu *fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
    [fileMenuItem setSubmenu:fileMenu];
    
    // Open SVG file
    NSMenuItem *openItem = [[NSMenuItem alloc] initWithTitle:@"Open..." action:@selector(openFile:) keyEquivalent:@"o"];
    [openItem setTarget:self];
    [fileMenu addItem:openItem];
    
    [fileMenu addItem:[NSMenuItem separatorItem]];
    
    // SVG submenu
    NSMenuItem *svgMenuItem = [[NSMenuItem alloc] initWithTitle:@"Open SVG" action:nil keyEquivalent:@""];
    [fileMenu addItem:svgMenuItem];
    
    NSMenu *svgMenu = [[NSMenu alloc] initWithTitle:@"Open SVG"];
    [svgMenuItem setSubmenu:svgMenu];
    
    // Add SVG files
    NSArray *svgFiles = @[
        @"basic_shapes.svg",
        @"clip_path.svg",
        @"defs_use.svg",
        @"gradients.svg",
        @"image.svg",
        @"mask-alpha-units-test.svg",
        @"mask-comprehensive-test.svg",
        @"mask-luminance-gradient-test.svg",
        @"paths.svg",
        @"text.svg",
        @"transforms.svg"
    ];
    
    for (NSString *fileName in svgFiles) {
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:fileName
                                                        action:@selector(openSVG:)
                                                 keyEquivalent:@""];
        [item setTarget:self];
        [item setRepresentedObject:fileName];
        [svgMenu addItem:item];
    }
    
    [fileMenu addItem:[NSMenuItem separatorItem]];
    
    NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                        action:@selector(terminate:)
                                                 keyEquivalent:@"q"];
    [fileMenu addItem:quitItem];
}

- (void)openSVG:(id)sender {
    if (![sender isKindOfClass:[NSMenuItem class]]) {
        return;
    }
    
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    NSString *fileName = [menuItem representedObject];
    if (!fileName) {
        return;
    }
    
    ViewController *viewController = (ViewController *)self.window.contentViewController;
    if ([viewController respondsToSelector:@selector(loadSVGFile:)]) {
        [viewController loadSVGFile:fileName];
    }
}

- (void)openFile:(id)sender {
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseFiles:YES];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setAllowsMultipleSelection:NO];
    [openPanel setAllowedFileTypes:@[@"svg"]];
    [openPanel setTitle:@"Open SVG File"];
    
    [openPanel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
        if (result == NSModalResponseOK) {
            NSURL *fileURL = [openPanel URL];
            NSString *filePath = [fileURL path];
            
            ViewController *viewController = (ViewController *)self.window.contentViewController;
            if ([viewController respondsToSelector:@selector(loadSVGFile:)]) {
                // Pass full path directly
                [(SVGMetalView *)viewController.metalView loadSVGFromFile:filePath];
            }
        }
    }];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end
