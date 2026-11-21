// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Portions Copyright (c) 2026 Software Mansion.
// See sheet/LICENSE.screens for the MIT license covering those portions.

#import <Lynx/LynxCustomMeasureShadowNode.h>

NS_ASSUME_NONNULL_BEGIN

/** ShadowNode that lays out transferred Lynx children with native receiver constraints. */
@interface SRSheetContentShadowNode : LynxCustomMeasureShadowNode

/** Updates the native receiver constraints used for the next Lynx child layout pass. */
- (void)updateHostConstraintsWithWidth:(CGFloat)width
                             widthMode:(LynxMeasureMode)widthMode
                                height:(CGFloat)height
                            heightMode:(LynxMeasureMode)heightMode
             measureContentHeightToFit:(BOOL)measureContentHeightToFit;

@end

NS_ASSUME_NONNULL_END
