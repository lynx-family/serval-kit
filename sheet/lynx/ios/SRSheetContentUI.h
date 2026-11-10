// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Portions Copyright (c) 2026 Software Mansion.
// See sheet/LICENSE.screens for the MIT license covering those portions.

#import <Lynx/LynxUIView.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class SRSheetContentUI;

/** Native receiver that hosts Lynx child views outside their original view hierarchy. */
@interface SRSheetContentView : UIView

/** The transfer UI whose Lynx subtree is hosted by this receiver. */
@property(nonatomic, weak, readonly) SRSheetContentUI *transfer;

- (instancetype)initWithTransfer:(SRSheetContentUI *)transfer;

@end

/** Base Lynx UI for native components that render their children in another native hierarchy. */
@interface SRSheetContentUI : LynxUIView
@property(nonatomic) BOOL fitContent;
@property(nonatomic, copy, nullable) void (^onContentLayout)(void);
@property(nonatomic, copy, nullable) void (^onHostChanged)(void);
- (CGFloat)naturalContentHeight;

/** Receiver containing the transferred Lynx child views. */
@property(nonatomic, strong, readonly) SRSheetContentView *transferReceiver;

/** Measures fit content independently of the current native panel height. */
- (BOOL)shouldMeasureContentHeightToFit;

/** Override to provide a specialized receiver view. */
- (SRSheetContentView *)createTransferReceiver;

@end

NS_ASSUME_NONNULL_END
