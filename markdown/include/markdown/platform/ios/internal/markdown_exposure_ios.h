// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_EXPOSURE_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_EXPOSURE_IOS_H_

#include "markdown/markdown_exposure_listener.h"

#import "markdown/platform/ios/IMarkdownExposureDelegate.h"

namespace lynx::markdown {
class MarkdownExposureIOS final : public MarkdownExposureListener {
 public:
  MarkdownExposureIOS() = default;
  ~MarkdownExposureIOS() override = default;

  void SetDelegate(id<IMarkdownExposureDelegate> delegate) {
    delegate_ = delegate;
  }

  void OnLinkAppear(const char* url, const char* content) override {
    if (delegate_ != nil && url != nullptr && content != nullptr) {
      [delegate_ onLinkAppear:[NSString stringWithUTF8String:url]
                      Content:[NSString stringWithUTF8String:content]];
    }
  }

  void OnLinkDisappear(const char* url, const char* content) override {
    if (delegate_ != nil && url != nullptr && content != nullptr) {
      [delegate_ onLinkDisappear:[NSString stringWithUTF8String:url]
                         Content:[NSString stringWithUTF8String:content]];
    }
  }

  void OnImageAppear(const char* url) override {
    if (delegate_ != nil && url != nullptr) {
      [delegate_ onImageAppear:[NSString stringWithUTF8String:url]];
    }
  }

  void OnImageDisappear(const char* url) override {
    if (delegate_ != nil && url != nullptr) {
      [delegate_ onImageDisappear:[NSString stringWithUTF8String:url]];
    }
  }

 private:
  __weak id<IMarkdownExposureDelegate> delegate_{nil};
};
}  // namespace lynx::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_EXPOSURE_IOS_H_
