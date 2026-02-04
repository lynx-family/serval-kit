// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_EVENT_LISTENER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_EVENT_LISTENER_H_

#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/style/markdown_style.h"
#include "markdown/view/markdown_selection_view.h"

namespace lynx {
namespace markdown {
enum class SelectionState {
  kEnter,
  kMove,
  kStop,
  kExit,
};
class MarkdownEventListener {
 public:
  virtual ~MarkdownEventListener() = default;
  virtual void OnParseEnd() = 0;
  virtual void OnTextOverflow(MarkdownTextOverflow overflow) = 0;
  virtual void OnDrawStart() = 0;
  virtual void OnDrawEnd() = 0;
  virtual void OnAnimationStep(int32_t animation_step,
                               int32_t max_animation_step) = 0;
  virtual void OnLinkClicked(const char* url, const char* content) = 0;
  virtual void OnImageClicked(const char* url) = 0;
  virtual void OnSelectionChanged(int32_t start_index, int32_t end_index,
                                  SelectionHandleType handle,
                                  SelectionState state) = 0;
};
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_EVENT_LISTENER_H_
