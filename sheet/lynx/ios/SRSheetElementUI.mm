// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "SRSheetElementUI.h"
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <ServalSheet/SRSheetSession.h>
#include <cmath>

@implementation SRSheetElementUI {
  SRSheetSession *_session;
  SRSheetConfiguration *_configuration;
  BOOL _initialOpen;
  BOOL _initialApplied;
  NSInteger _initialIndex;
  NSString *_host;
  NSMutableDictionary<NSString *, NSString *> *_errors;
  BOOL _dismissible;
  BOOL _clickToClose;
}

- (instancetype)init {
  if ((self = [super init])) {
    _errors = [NSMutableDictionary new];
    _dismissible = YES;
    _clickToClose = YES;
    _configuration = [SRSheetConfiguration new];
    _configuration.detents = @[ @1 ];
    _configuration.backgroundColor = UIColor.whiteColor;
  }
  return self;
}

- (void)emit:(NSString *)name detail:(NSDictionary *)detail {
  [self.context.eventEmitter sendCustomEvent:[[LynxDetailEvent alloc] initWithName:name
                                                                        targetSign:self.sign
                                                                            detail:detail]];
}

- (void)configurationError:(NSString *)code field:(NSString *)field message:(NSString *)message {
  _errors[field] = code;
  [self emit:@"error" detail:@{@"code" : code, @"message" : message}];
}

LYNX_PROP_SETTER("initial-open", setInitialOpen, BOOL) {
  if (!_initialApplied) _initialOpen = requestReset ? NO : value;
}
LYNX_PROP_SETTER("initial-snap-index", setInitialSnapIndex, id) {
  if (_initialApplied) return;
  double index =
      requestReset ? 0 : ([value isKindOfClass:NSNumber.class] ? [value doubleValue] : NAN);
  if (!std::isfinite(index) || index < 0 || index > INT_MAX || std::floor(index) != index) {
    [self configurationError:@"invalid-snap-points"
                       field:@"initial-snap-index"
                     message:@"Expected a non-negative integer"];
    return;
  }
  _initialIndex = (NSInteger)index;
  [_errors removeObjectForKey:@"initial-snap-index"];
}
LYNX_PROP_SETTER("snap-points", setSnapPoints, id) {
  id points = requestReset ? @[ @1 ] : value;
  if ([points isEqual:@"fit"]) {
    _configuration.detents = @[ @(-1) ];
    self.fitContent = YES;
    [_errors removeObjectForKey:@"snap-points"];
    return;
  }
  if (![points isKindOfClass:NSArray.class] || [points count] < 1 || [points count] > 3) {
    [self configurationError:@"invalid-snap-points"
                       field:@"snap-points"
                     message:@"Expected fit or one to three increasing fractions"];
    return;
  }
  double previous = 0;
  for (id point in points) {
    if (![point isKindOfClass:NSNumber.class] || !std::isfinite([point doubleValue]) ||
        [point doubleValue] <= previous || [point doubleValue] > 1) {
      [self configurationError:@"invalid-snap-points"
                         field:@"snap-points"
                       message:@"Fractions must increase within (0,1]"];
      return;
    }
    previous = [point doubleValue];
  }
  [_errors removeObjectForKey:@"snap-points"];
  self.fitContent = NO;
  _configuration.detents = points;
}
LYNX_PROP_SETTER("dismissible", setDismissible, BOOL) { _dismissible = requestReset || value; }
LYNX_PROP_SETTER("animation", setAnimation, id) {
  [_errors removeObjectForKey:@"animation"];
  if (requestReset || [value isEqual:@"default"] || [value isEqual:@"platform"])
    _configuration.animated = YES;
  else if ([value isEqual:@"none"])
    _configuration.animated = NO;
  else
    [self configurationError:@"unsupported-capability"
                       field:@"animation"
                     message:@"Supported animation policies are default and none"];
}
LYNX_PROP_SETTER("presentation-host", setPresentationHost, NSString *) {
  [_errors removeObjectForKey:@"presentation-host"];
  _host = requestReset ? nil : value;
  if (_host && ![_host isEqual:@"page"] && ![_host isEqual:@"top"])
    [self configurationError:@"unavailable-host"
                       field:@"presentation-host"
                     message:@"Supported presentation hosts are page and top"];
}
LYNX_PROP_SETTER("appearance", setAppearance, NSDictionary *) {
  NSDictionary *appearance = requestReset ? @{} : value;
  _configuration.backgroundColor = appearance[@"backgroundColor"]
                                       ? [self colorFromString:appearance[@"backgroundColor"]]
                                       : UIColor.whiteColor;
  double radius = appearance[@"cornerRadius"] ? [appearance[@"cornerRadius"] doubleValue] : -1;
  if (!_configuration.backgroundColor || !std::isfinite(radius) ||
      (appearance[@"cornerRadius"] && radius < 0)) {
    [self configurationError:@"invalid-state"
                       field:@"appearance"
                     message:@"Expected a valid color and non-negative radius"];
    return;
  }
  [_errors removeObjectForKey:@"appearance"];
  _configuration.preferredCornerRadius = radius;
  _configuration.prefersGrabberVisible = [appearance[@"grabberVisible"] boolValue];
}
LYNX_PROP_SETTER("backdrop", setBackdrop, id) {
  [_errors removeObjectForKey:@"backdrop"];
  _clickToClose = YES;
  if (requestReset || [value isEqual:@"default"] || [value isEqual:@"platform"]) {
    _configuration.backdropColor = UIColor.blackColor;
    _configuration.backdropOpacity = 0.5;
    return;
  }
  if (![value isKindOfClass:NSDictionary.class]) {
    [self configurationError:@"unsupported-capability"
                       field:@"backdrop"
                     message:@"Expected a backdrop configuration"];
    return;
  }
  NSDictionary *backdrop = value;
  CGFloat opacity = backdrop[@"opacity"] ? [backdrop[@"opacity"] doubleValue] : 0.5;
  if (!std::isfinite(opacity) || opacity < 0 || opacity > 1) {
    [self configurationError:@"invalid-state"
                       field:@"backdrop"
                     message:@"Backdrop opacity must be within [0,1]"];
    return;
  }
  _configuration.backdropOpacity = opacity;
  _configuration.backdropColor =
      backdrop[@"color"] ? [self colorFromString:backdrop[@"color"]] : UIColor.blackColor;
  if (!_configuration.backdropColor) {
    [self configurationError:@"invalid-state"
                       field:@"backdrop"
                     message:@"Expected a valid backdrop color"];
    return;
  }
  _clickToClose = !backdrop[@"clickToClose"] || [backdrop[@"clickToClose"] boolValue];
}

- (void)ensureSession {
  if (_session) return;
  _session = [[SRSheetSession alloc] initWithContentView:self.transferReceiver
                                              anchorView:self.view];
  __weak __typeof__(self) weakSelf = self;
  _session.onEvent = ^(NSString *name, NSDictionary *detail) {
    [weakSelf emit:name detail:detail];
  };
  _session.controller.measureContentHeight = ^CGFloat {
    return [weakSelf naturalContentHeight];
  };
  _session.controller.resolveArea = ^CGRect(UIWindow *window) {
    UIEdgeInsets insets = window.safeAreaInsets;
    return UIEdgeInsetsInsetRect(window.bounds,
                                 UIEdgeInsetsMake(insets.top, insets.left, 0, insets.right));
  };
  _session.resolveHost = ^SRSheetHost * {
    __typeof__(self) owner = weakSelf;
    if (!owner) return nil;
    UIResponder *responder = owner.view.nextResponder;
    while (responder && ![responder isKindOfClass:UIViewController.class])
      responder = responder.nextResponder;
    UIViewController *controller = (UIViewController *)responder;
    if ([owner->_host isEqual:@"top"])
      while (controller.presentedViewController) controller = controller.presentedViewController;
    if (!controller) return nil;
    SRSheetHost *host = [SRSheetHost new];
    host.controller = controller;
    host.container = controller.view;
    return host;
  };
  self.onHostChanged = ^{
    __typeof__(self) owner = weakSelf;
    if (!owner) return;
    [owner->_session hostDidChange];
  };
  self.onContentLayout = ^{
    __typeof__(self) owner = weakSelf;
    if (!owner) return;
    [owner->_session contentDidChange];
  };
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  [self ensureSession];
  if (_errors.count > 0) return;
  _configuration.preventNativeDismissChannels = !_dismissible
                                                    ? @[ @"back", @"drag", @"backdrop" ]
                                                    : (!_clickToClose ? @[ @"backdrop" ] : @[]);
  [_session configure:_configuration];
  if (!_initialApplied) {
    _initialApplied = YES;
    if (_initialOpen) [_session presentAtIndex:_initialIndex requestId:nil];
  }
}

- (void)invoke:(NSString *)operation
        params:(NSDictionary *)params
      callback:(LynxUIMethodCallbackBlock)callback {
  [self ensureSession];
  if (_errors.count > 0) {
    callback(kUIMethodParamInvalid, @{@"code" : @"invalid-state"});
    return;
  }
  id indexValue = params[@"index"];
  double numericIndex = [indexValue isKindOfClass:NSNumber.class] ? [indexValue doubleValue] : NAN;
  if (([operation isEqual:@"snap"] && !indexValue) ||
      (indexValue && (!std::isfinite(numericIndex) || numericIndex < 0 || numericIndex > INT_MAX ||
                      std::floor(numericIndex) != numericIndex)) ||
      (params[@"requestId"] && ![params[@"requestId"] isKindOfClass:NSString.class]) ||
      (params[@"animate"] && ![params[@"animate"] isKindOfClass:NSNumber.class])) {
    callback(kUIMethodParamInvalid, @{@"accepted" : @NO, @"code" : @"invalid-state"});
    return;
  }
  BOOL animate = params[@"animate"] ? [params[@"animate"] boolValue] : _configuration.animated;
  NSInteger index = params[@"index"]
                        ? [params[@"index"] integerValue]
                        : (_session.isOpen ? _session.controller.currentIndex : _initialIndex);
  NSDictionary *result;
  if ([operation isEqual:@"present"])
    result = [_session presentAtIndex:index animated:animate requestId:params[@"requestId"]];
  else if ([operation isEqual:@"dismiss"])
    result = [_session dismissAnimated:animate requestId:params[@"requestId"]];
  else
    result = [_session snapToIndex:index animated:animate requestId:params[@"requestId"]];
  callback([result[@"accepted"] boolValue] ? kUIMethodSuccess : kUIMethodParamInvalid, result);
}
LYNX_UI_METHOD(present) { [self invoke:@"present" params:params callback:callback]; }
LYNX_UI_METHOD(dismiss) { [self invoke:@"dismiss" params:params callback:callback]; }
LYNX_UI_METHOD(snapTo) { [self invoke:@"snap" params:params callback:callback]; }

- (UIColor *)colorFromString:(NSString *)value {
  if (![value isKindOfClass:NSString.class]) return nil;
  if ([value isEqual:@"transparent"]) return UIColor.clearColor;
  if (![value hasPrefix:@"#"]) return nil;
  NSString *hex = [value substringFromIndex:1];
  unsigned long long number = 0;
  NSScanner *scanner = [NSScanner scannerWithString:hex];
  if ((hex.length != 3 && hex.length != 6 && hex.length != 8) ||
      ![scanner scanHexLongLong:&number] || !scanner.isAtEnd)
    return nil;
  if (hex.length == 3) {
    number = ((number >> 8) & 15) * 0x110000 + ((number >> 4) & 15) * 0x1100 + (number & 15) * 0x11;
  }
  if (hex.length == 8)
    return [UIColor colorWithRed:((number >> 24) & 255) / 255.0
                           green:((number >> 16) & 255) / 255.0
                            blue:((number >> 8) & 255) / 255.0
                           alpha:(number & 255) / 255.0];
  return [UIColor colorWithRed:((number >> 16) & 255) / 255.0
                         green:((number >> 8) & 255) / 255.0
                          blue:(number & 255) / 255.0
                         alpha:1];
}

- (void)dealloc {
  [_session invalidate];
}
@end
