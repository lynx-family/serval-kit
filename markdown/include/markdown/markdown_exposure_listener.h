// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_EXPOSURE_LISTENER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_EXPOSURE_LISTENER_H_
namespace lynx::markdown {
class MarkdownExposureListener {
 public:
  virtual ~MarkdownExposureListener() = default;
  virtual void OnLinkAppear(const char* url, const char* content) = 0;
  virtual void OnLinkDisappear(const char* url, const char* content) = 0;
  virtual void OnImageAppear(const char* url) = 0;
  virtual void OnImageDisappear(const char* url) = 0;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_EXPOSURE_LISTENER_H_
