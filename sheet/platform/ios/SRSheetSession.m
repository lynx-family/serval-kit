// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "SRSheetSession.h"

@implementation SRSheetHost
@end

@interface SRSheetSession () <SRSheetControllerDelegate>
@property(nonatomic, readwrite) SRSheetController *controller;
@end

@implementation SRSheetSession {
  BOOL _open;
  BOOL _disposed;
  BOOL _disposing;
  NSUInteger _nextTransition;
  NSUInteger _generation;
  NSDictionary *_active;
  NSDictionary *_pending;
  NSInteger _lastSettledIndex;
  BOOL _snappingOperation;
  SRSheetConfiguration *_deferredConfiguration;
}

- (instancetype)initWithContentView:(UIView *)contentView anchorView:(UIView *)anchorView {
  if ((self = [super init])) {
    _controller = [[SRSheetController alloc] initWithContentView:contentView];
    _controller.anchorView = anchorView;
    _controller.delegate = self;
    _lastSettledIndex = -1;
  }
  return self;
}

- (BOOL)isOpen {
  return _open;
}

- (void)emit:(NSString *)name detail:(NSDictionary *)detail {
  if (!_disposed && self.onEvent) self.onEvent(name, detail);
}

- (void)configure:(SRSheetConfiguration *)configuration {
  NSAssert(NSThread.isMainThread, @"Sheet configuration requires the UI thread");
  if (_disposed) return;
  if (_active) {
    _deferredConfiguration = [configuration copy];
    return;
  }
  self.controller.configuration = configuration;
  if (_open && !_active) [self.controller applyConfigurationApplyingInitialDetent:NO];
}

- (BOOL)validIndex:(NSInteger)index {
  SRSheetConfiguration *configuration = _deferredConfiguration ?: self.controller.configuration;
  return index >= 0 && index < MAX((NSInteger)configuration.detents.count, 1);
}

- (NSDictionary *)submit:(NSString *)kind
                   index:(NSInteger)index
                animated:(BOOL)animated
                  reason:(NSString *)reason
               requestId:(NSString *)requestId {
  NSAssert(NSThread.isMainThread, @"Sheet operations require the UI thread");
  if (_disposed || _disposing) return @{@"accepted" : @NO, @"code" : @"invalid-state"};
  if (![kind isEqual:@"dismiss"] && ![self validIndex:index]) {
    return @{@"accepted" : @NO, @"code" : @"invalid-snap-points"};
  }
  if ([kind isEqual:@"snap"] && !_open) return @{@"accepted" : @NO, @"code" : @"invalid-state"};
  if (_active && [_active[@"operation"] isEqual:kind] &&
      [_active[@"index"] integerValue] == index) {
    NSDictionary *current = _active;
    NSDictionary *superseded = _pending;
    _pending = nil;
    if (superseded) [self terminal:superseded result:@"cancelled"];
    return @{@"accepted" : @YES, @"transitionId" : current[@"transitionId"], @"coalesced" : @YES};
  }
  if (!_active && !_pending &&
      (([kind isEqual:@"dismiss"] && !_open) ||
       (![kind isEqual:@"dismiss"] && _open && index == self.controller.currentIndex))) {
    return @{@"accepted" : @YES, @"noop" : @YES};
  }
  NSMutableDictionary *operation = [@{
    @"operation" : kind,
    @"index" : @(index),
    @"animate" : @(animated),
    @"reason" : reason,
    @"transitionId" : @(++_nextTransition)
  } mutableCopy];
  if (requestId) operation[@"requestId"] = requestId;
  NSDictionary *superseded = _pending;
  _pending = operation;
  if (superseded) [self terminal:superseded result:@"cancelled"];
  [self drain];
  return @{@"accepted" : @YES, @"transitionId" : operation[@"transitionId"]};
}

- (NSDictionary *)presentAtIndex:(NSInteger)index requestId:(NSString *)requestId {
  return [self presentAtIndex:index
                     animated:(_deferredConfiguration ?: self.controller.configuration).animated
                    requestId:requestId];
}

- (NSDictionary *)presentAtIndex:(NSInteger)index
                        animated:(BOOL)animated
                       requestId:(NSString *)requestId {
  return [self submit:@"present"
                index:index
             animated:animated
               reason:@"command"
            requestId:requestId];
}

- (NSDictionary *)dismissWithRequestId:(NSString *)requestId {
  return [self dismissAnimated:(_deferredConfiguration ?: self.controller.configuration).animated
                     requestId:requestId];
}

- (NSDictionary *)dismissAnimated:(BOOL)animated requestId:(NSString *)requestId {
  return [self submit:@"dismiss" index:0 animated:animated reason:@"command" requestId:requestId];
}

- (NSDictionary *)snapToIndex:(NSInteger)index
                     animated:(BOOL)animated
                    requestId:(NSString *)requestId {
  return [self submit:@"snap" index:index animated:animated reason:@"command" requestId:requestId];
}

- (void)terminal:(NSDictionary *)operation result:(NSString *)result {
  NSMutableDictionary *detail = [operation mutableCopy];
  detail[@"result"] = result;
  [self emit:@"transitionend" detail:detail];
}

- (void)finish:(NSUInteger)generation {
  if (_disposed || generation != _generation || !_active) return;
  NSDictionary *operation = _active;
  _active = nil;
  _snappingOperation = NO;
  if ([operation[@"operation"] isEqual:@"present"]) {
    [self sheetSnapSettled:self.controller.currentIndex height:self.controller.currentHeight];
  }
  if (_deferredConfiguration) {
    SRSheetConfiguration *configuration = _deferredConfiguration;
    _deferredConfiguration = nil;
    [self configure:configuration];
  }
  [self terminal:operation result:@"completed"];
  [self drain];
}

- (void)drain {
  if (_disposed || _active || !_pending) return;
  NSDictionary *operation = _pending;
  NSString *kind = operation[@"operation"];
  if (![kind isEqual:@"dismiss"] && ![self validIndex:[operation[@"index"] integerValue]]) {
    _pending = nil;
    [self emit:@"error"
        detail:@{@"code" : @"invalid-snap-points", @"transitionId" : operation[@"transitionId"]}];
    [self terminal:operation result:@"cancelled"];
    return;
  }
  SRSheetHost *host = nil;
  if ([kind isEqual:@"present"] && !_open) {
    if (!self.controller.anchorView.window) return;
    host = self.resolveHost ? self.resolveHost() : nil;
    if (!host.controller ||
        (host.container && host.container.window != self.controller.anchorView.window)) {
      _pending = nil;
      [self emit:@"error"
          detail:@{@"code" : @"unavailable-host", @"transitionId" : operation[@"transitionId"]}];
      [self terminal:operation result:@"cancelled"];
      return;
    }
  }
  _pending = nil;
  _active = operation;
  NSUInteger generation = ++_generation;
  [self emit:@"transitionstart" detail:operation];
  if (_disposed || generation != _generation) return;
  BOOL animate = [operation[@"animate"] boolValue];
  NSInteger index = [operation[@"index"] integerValue];
  if ([kind isEqual:@"dismiss"] && !_open) {
    [self finish:generation];
    return;
  }
  if ([kind isEqual:@"snap"] || ([kind isEqual:@"present"] && _open)) {
    _snappingOperation = YES;
    [self.controller snapToIndex:index animated:animate];
    return;
  }
  BOOL opening = [kind isEqual:@"present"];
  _open = opening;
  NSMutableDictionary *change = [operation mutableCopy];
  change[@"open"] = @(opening);
  [self emit:@"openchange" detail:change];
  if (_disposed || generation != _generation) return;
  if (opening) {
    SRSheetConfiguration *configuration = [self.controller.configuration copy];
    configuration.initialDetentIndex = index;
    self.controller.configuration = configuration;
    [self.controller applyConfigurationApplyingInitialDetent:YES];
    __weak __typeof__(self) weakSelf = self;
    void (^completion)(void) = ^{
      [weakSelf finish:generation];
    };
    if (host.container) {
      [self.controller presentSheetInParentViewController:host.controller
                                            containerView:host.container
                                                 animated:animate
                                               completion:completion];
    } else {
      [host.controller presentViewController:self.controller
                                    animated:animate
                                  completion:completion];
    }
  } else {
    __weak __typeof__(self) weakSelf = self;
    if (animate)
      [self.controller dismissSheetWithCompletion:^{
        [weakSelf finish:generation];
      }];
    else
      [self.controller dismissSheetImmediatelyWithCompletion:^{
        [weakSelf finish:generation];
      }];
  }
}

- (void)sheetLifecycle:(NSString *)phase {
  [self emit:@"lifecycle" detail:@{@"phase" : phase}];
  if ([phase isEqual:@"didDisappear"] && _open && !_active &&
      !self.controller.parentViewController && !self.controller.presentingViewController) {
    _open = NO;
    [self emit:@"openchange" detail:@{@"open" : @NO, @"reason" : @"parent"}];
  }
}

- (void)sheetTargetIndexChanged:(NSInteger)index {
  [self emit:@"targetchange" detail:@{@"index" : @(index)}];
}

- (void)sheetSnapSettled:(NSInteger)index height:(CGFloat)height {
  if ([_active[@"operation"] isEqual:@"present"] && !_snappingOperation) return;
  if (index != _lastSettledIndex) {
    _lastSettledIndex = index;
    NSMutableDictionary *detail = [@{
      @"index" : @(index),
      @"heightPx" :
          @(height * (self.controller.anchorView.window.screen.scale ?: UIScreen.mainScreen.scale))
    } mutableCopy];
    NSArray *detents = self.controller.configuration.detents;
    if (index < (NSInteger)detents.count && [detents[index] doubleValue] >= 0)
      detail[@"fraction"] = detents[index];
    [self emit:@"snapchange" detail:detail];
  }
  NSString *kind = _active[@"operation"];
  if ([kind isEqual:@"snap"] ||
      ([kind isEqual:@"present"] &&
       (self.controller.parentViewController || self.controller.presentingViewController) &&
       [self.controller isViewLoaded] && self.controller.view.window)) {
    if (_snappingOperation) {
      _snappingOperation = NO;
      [self finish:_generation];
    }
    // Present completion belongs to the controller's appearance transition.
  }
}

- (void)sheetDismissPrevented:(NSString *)channel {
  [self emit:@"dismissprevented" detail:@{@"channel" : channel}];
}

- (void)sheetRequestDismiss:(NSString *)channel {
  if (!_open || _disposed) return;
  if ([self.controller.configuration isDismissPreventedForChannel:channel]) {
    [self sheetDismissPrevented:channel];
    return;
  }
  [self submit:@"dismiss"
          index:0
       animated:(_deferredConfiguration ?: self.controller.configuration).animated
         reason:channel
      requestId:nil];
}

- (void)hostDidChange {
  if (_disposed) return;
  if (!self.controller.anchorView.window) {
    ++_generation;
    NSDictionary *active = _active;
    _active = nil;
    if (active) [self terminal:active result:@"cancelled"];
    _open = NO;
    [self.controller dismissSheetImmediatelyWithCompletion:^{
    }];
  } else {
    [self.controller invalidateDetents];
    [self drain];
  }
}

- (void)contentDidChange {
  if (!_disposed && !_active) [self.controller invalidateDetents];
}

- (void)invalidate {
  if (_disposed || _disposing) return;
  _disposing = YES;
  ++_generation;
  if (_active) [self terminal:_active result:@"cancelled"];
  if (_pending) [self terminal:_pending result:@"cancelled"];
  _active = nil;
  _pending = nil;
  _deferredConfiguration = nil;
  _disposed = YES;
  _open = NO;
  self.onEvent = nil;
  self.resolveHost = nil;
  self.controller.delegate = nil;
  self.controller.measureContentHeight = nil;
  self.controller.resolveArea = nil;
  [self.controller dismissSheetImmediatelyWithCompletion:^{
  }];
}
@end
