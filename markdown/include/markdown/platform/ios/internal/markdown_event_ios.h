// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_EVENT_IOS_H_
#define THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_EVENT_IOS_H_
#include "markdown/markdown_event_listener.h"
#import "markdown/platform/ios/i_resource_delegate.h"

namespace lynx {
namespace markdown {
class MarkdownEventIOS final : public MarkdownEventListener {
 public:
  MarkdownEventIOS() {}

  void SetDelegate(id<IEventDelegate> delegate) { delegate_ = delegate; }
  void OnTextOverflow(MarkdownTextOverflow overflow) {
    if (delegate_ != nil) {
      NSString* type = nil;
      if (overflow == MarkdownTextOverflow::kEllipsis) {
        type = @"ellipsis";
      } else {
        type = @"clip";
      }
      [delegate_ dispatchCustomEvent:@"overflow" Detail:@{@"type": type}];
    }
  }

  void OnParseEnd() override {}
  void OnDrawStart() override {}
  void OnDrawEnd() override {}
  void OnAnimationStep(int32_t, int32_t) override {}
  void OnLinkClicked(const char*, const char*) override {}
  void OnImageClicked(const char*) override {}
  void OnSelectionChanged(int32_t, int32_t, SelectionHandleType,
                          SelectionState) override {}

 private:
  id<IEventDelegate> delegate_{nil};
};
}  // namespace markdown
}  // namespace lynx
#endif  // THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_EVENT_IOS_H_
