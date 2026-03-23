// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_EVENT_IOS_H_
#define THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_EVENT_IOS_H_
#include "markdown/markdown_event_listener.h"
#import "markdown/platform/ios/IMarkdownEventDelegate.h"
#import "markdown/platform/ios/IMarkdownResourceDelegate.h"

namespace lynx {
namespace markdown {
class MarkdownEventIOS final : public MarkdownEventListener {
 public:
  MarkdownEventIOS() {}

  void SetDelegate(id<IMarkdownEventDelegate> delegate) {
    delegate_ = delegate;
  }
  void OnParseEnd() override {
    if (delegate_ != nil) {
      [delegate_ onParseEnd];
    }
  }
  void OnTextOverflow(MarkdownTextOverflow overflow) override {
    if (delegate_ != nil) {
      const auto type = static_cast<ServalMarkdownTextOverflow>(overflow);
      [delegate_ onTextOverflow:type];
    }
  }

  void OnDrawStart() override {
    if (delegate_ != nil) {
      [delegate_ onDrawStart];
    }
  }
  void OnDrawEnd() override {
    if (delegate_ != nil) {
      [delegate_ onDrawEnd];
    }
  }
  void OnAnimationStep(int32_t animation_step,
                       int32_t max_animation_step) override {
    if (delegate_ != nil) {
      [delegate_ onAnimationStep:animation_step
                MaxAnimationStep:max_animation_step];
    }
  }
  void OnLinkClicked(const char* url, const char* content) override {
    if (delegate_ != nil && url != nullptr && content != nullptr) {
      [delegate_ onLinkClicked:[NSString stringWithUTF8String:url]
                       Content:[NSString stringWithUTF8String:content]];
    }
  }
  void OnImageClicked(const char* url) override {
    if (delegate_ != nil && url != nullptr) {
      [delegate_ onImageClicked:[NSString stringWithUTF8String:url]];
    }
  }
  void OnSelectionChanged(int32_t start_index, int32_t end_index,
                          SelectionHandleType handle,
                          SelectionState state) override {
    if (delegate_ != nil) {
      [delegate_
          onSelectionChanged:start_index
                    EndIndex:end_index
                      Handle:static_cast<ServalMarkdownSelectionHandleType>(
                                 handle)
                       State:static_cast<ServalMarkdownSelectionState>(state)];
    }
  }

 private:
  __weak id<IMarkdownEventDelegate> delegate_{nil};
};
}  // namespace markdown
}  // namespace lynx
#endif  // THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_EVENT_IOS_H_
