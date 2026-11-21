// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "SRSheetSession.h"

@interface SRSheetSession (Testing)
- (void)sheetRequestDismiss:(NSString *)channel;
@end

@interface SheetSessionTests : XCTestCase
@property(nonatomic, strong) UIWindow *window;
@property(nonatomic, strong) UIView *content;
@property(nonatomic, strong) UIView *anchor;
@property(nonatomic, strong) SRSheetSession *session;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *events;
@end

@implementation SheetSessionTests
- (void)setUp {
  [super setUp];
  self.window = [[UIWindow alloc] initWithFrame:CGRectMake(0, 0, 390, 844)];
  self.window.rootViewController = [UIViewController new];
  [self.window makeKeyAndVisible];
  // Finish the host window appearance before beginning a contained transition.
  [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
  self.anchor = [UIView new];
  [self.window.rootViewController.view addSubview:self.anchor];
  self.content = [UIView new];
  self.events = [NSMutableArray new];
  self.session = [[SRSheetSession alloc] initWithContentView:self.content anchorView:self.anchor];
  __weak __typeof__(self) weakSelf = self;
  self.session.resolveHost = ^SRSheetHost * {
    SRSheetHost *host = [SRSheetHost new];
    host.controller = weakSelf.window.rootViewController;
    host.container = host.controller.view;
    return host;
  };
  self.session.onEvent = ^(NSString *name, NSDictionary *detail) {
    [weakSelf.events addObject:@{@"name" : name, @"detail" : detail}];
  };
  SRSheetConfiguration *config = [SRSheetConfiguration new];
  config.detents = @[ @0.3, @0.6, @0.9 ];
  config.animated = NO;
  [self.session configure:config];
}
- (void)tearDown {
  [self.session invalidate];
  self.window.hidden = YES;
  [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
  self.window = nil;
  [super tearDown];
}
- (NSArray *)eventsNamed:(NSString *)name {
  return [self.events
      filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSDictionary *event,
                                                                        NSDictionary *bindings) {
        return [event[@"name"] isEqual:name];
      }]];
}
- (void)testPresentSnapDismissHaveSeparateTerminalResults {
  XCTAssertTrue([[self.session presentAtIndex:1 requestId:@"open"][@"accepted"] boolValue]);
  XCTAssertTrue(self.session.isOpen);
  XCTAssertEqual(self.session.controller.currentIndex, 1);
  [self.session snapToIndex:2 animated:NO requestId:@"expand"];
  XCTAssertEqual(self.session.controller.currentIndex, 2);
  [self.session dismissWithRequestId:@"close"];
  XCTAssertFalse(self.session.isOpen);
  NSArray *ends = [self eventsNamed:@"transitionend"];
  XCTAssertEqual(ends.count, 3u);
  XCTAssertEqualObjects(ends[0][@"detail"][@"requestId"], @"open");
  XCTAssertEqualObjects(ends[1][@"detail"][@"requestId"], @"expand");
  XCTAssertEqualObjects(ends[2][@"detail"][@"requestId"], @"close");
  XCTAssertEqualObjects([self eventsNamed:@"snapchange"][0][@"detail"][@"index"], @1);
}
- (void)testPerCommandAnimationDoesNotChangeTheConfiguredDefault {
  SRSheetConfiguration *config = [self.session.controller.configuration copy];
  config.animated = YES;
  [self.session configure:config];
  [self.session presentAtIndex:1 animated:NO requestId:@"instant-open"];
  XCTAssertTrue(self.session.isOpen);
  XCTAssertEqual([self eventsNamed:@"transitionend"].count, 1u);
  XCTAssertTrue(self.session.controller.configuration.animated);
  [self.session dismissAnimated:NO requestId:@"instant-close"];
  XCTAssertFalse(self.session.isOpen);
  XCTAssertEqual([self eventsNamed:@"transitionend"].count, 2u);
}
- (void)testRepeatedOpenDoesNotDuplicateLogicalNotification {
  [self.session presentAtIndex:0 requestId:nil];
  [self.session presentAtIndex:0 requestId:nil];
  XCTAssertEqual([self eventsNamed:@"openchange"].count, 1u);
  XCTAssertEqual([self eventsNamed:@"transitionend"].count, 1u);
}
- (void)testPresentAtAnotherIndexWhileOpenCompletes {
  [self.session presentAtIndex:0 requestId:nil];
  [self.session presentAtIndex:2 requestId:@"resize"];
  XCTAssertEqual(self.session.controller.currentIndex, 2);
  XCTAssertEqual([self eventsNamed:@"transitionend"].count, 2u);
}
- (void)testUserDismissalCanBeBlockedWithoutBlockingCommands {
  SRSheetConfiguration *config = [self.session.controller.configuration copy];
  config.preventNativeDismissChannels = @[ @"backdrop", @"drag" ];
  [self.session configure:config];
  [self.session presentAtIndex:0 requestId:nil];
  [self.session sheetRequestDismiss:@"backdrop"];
  XCTAssertTrue(self.session.isOpen);
  XCTAssertEqual([self eventsNamed:@"dismissprevented"].count, 1u);
  [self.session dismissWithRequestId:nil];
  XCTAssertFalse(self.session.isOpen);
}
- (void)testInvalidIndexAndHiddenSnapAreRejected {
  XCTAssertFalse([[self.session snapToIndex:0 animated:NO requestId:nil][@"accepted"] boolValue]);
  XCTAssertFalse([[self.session presentAtIndex:3 requestId:nil][@"accepted"] boolValue]);
  XCTAssertEqual([self eventsNamed:@"openchange"].count, 0u);
}
- (void)testFitContentCanGrowAfterAnEmptyFirstLayout {
  SRSheetConfiguration *config = [self.session.controller.configuration copy];
  config.detents = @[ @(-1) ];
  [self.session configure:config];
  __block CGFloat naturalHeight = 1;
  self.session.controller.measureContentHeight = ^CGFloat {
    return naturalHeight;
  };
  [self.session presentAtIndex:0 requestId:nil];
  CGFloat initialHeight = self.content.bounds.size.height;
  naturalHeight = 260;
  [self.session contentDidChange];
  [self.session.controller.view layoutIfNeeded];
  XCTAssertGreaterThan(self.content.bounds.size.height, initialHeight);
  XCTAssertEqualWithAccuracy(self.content.bounds.size.height, 260, 1);
}
- (void)testDisposeIsIdempotentAndRejectsLaterCommands {
  [self.session presentAtIndex:0 requestId:nil];
  [self.session invalidate];
  [self.session invalidate];
  XCTAssertNil(self.session.controller.parentViewController);
  XCTAssertNil(self.session.controller.presentingViewController);
  XCTAssertFalse([[self.session presentAtIndex:0 requestId:nil][@"accepted"] boolValue]);
}
- (void)testQueuedInitialOpenCanBeWithdrawnBeforeHostAttachment {
  [self.anchor removeFromSuperview];
  [self.session presentAtIndex:0 requestId:@"queued"];
  [self.session dismissWithRequestId:@"withdraw"];
  [self.window.rootViewController.view addSubview:self.anchor];
  [self.session hostDidChange];
  XCTAssertFalse(self.session.isOpen);
}
- (void)testRepeatedActiveOpenWithdrawsQueuedDismissal {
  XCTestExpectation *opened = [self expectationWithDescription:@"presentation completes"];
  __weak __typeof__(self) weakSelf = self;
  self.session.onEvent = ^(NSString *name, NSDictionary *detail) {
    [weakSelf.events addObject:@{@"name" : name, @"detail" : detail}];
    if ([name isEqual:@"transitionend"] && [detail[@"requestId"] isEqual:@"open"] &&
        [detail[@"result"] isEqual:@"completed"])
      [opened fulfill];
  };
  [self.session presentAtIndex:1 animated:YES requestId:@"open"];
  [self.session dismissWithRequestId:@"withdrawn-close"];
  NSDictionary *receipt = [self.session presentAtIndex:1 requestId:@"keep-open"];
  XCTAssertTrue([receipt[@"coalesced"] boolValue]);
  [self waitForExpectations:@[ opened ] timeout:3];
  XCTAssertTrue(self.session.isOpen);
  NSArray *ends = [self eventsNamed:@"transitionend"];
  XCTAssertEqual(ends.count, 2u);
  XCTAssertEqualObjects(ends[0][@"detail"][@"requestId"], @"withdrawn-close");
  XCTAssertEqualObjects(ends[0][@"detail"][@"result"], @"cancelled");
}
- (void)testPendingSnapIsCancelledWhenSupersededByDismiss {
  SRSheetConfiguration *config = [self.session.controller.configuration copy];
  config.animated = YES;
  [self.session configure:config];
  XCTestExpectation *closed = [self expectationWithDescription:@"queued dismissal completes"];
  __weak __typeof__(self) weakSelf = self;
  self.session.onEvent = ^(NSString *name, NSDictionary *detail) {
    [weakSelf.events addObject:@{@"name" : name, @"detail" : detail}];
    if ([name isEqual:@"transitionend"] && [detail[@"requestId"] isEqual:@"close"] &&
        [detail[@"result"] isEqual:@"completed"])
      [closed fulfill];
  };
  [self.session presentAtIndex:0 requestId:@"open"];
  [self.session snapToIndex:2 animated:YES requestId:@"superseded"];
  [self.session dismissWithRequestId:@"close"];
  [self waitForExpectations:@[ closed ] timeout:3];
  XCTAssertFalse(self.session.isOpen);
  NSArray *ends = [self eventsNamed:@"transitionend"];
  XCTAssertEqual(ends.count, 3u);
  XCTAssertEqualObjects(ends[0][@"detail"][@"requestId"], @"superseded");
  XCTAssertEqualObjects(ends[0][@"detail"][@"result"], @"cancelled");
}
@end
