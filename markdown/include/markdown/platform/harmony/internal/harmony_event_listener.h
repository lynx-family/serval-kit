// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_EVENT_LISTENER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_EVENT_LISTENER_H_
#include <string>
#include "markdown/markdown_event_listener.h"
#include "markdown/platform/harmony/internal/harmony_value_ref.h"
#include "napi/native_api.h"
namespace lynx::markdown {
class HarmonyEventListener : public MarkdownEventListener {
 public:
  explicit HarmonyEventListener(napi_env env);
  ~HarmonyEventListener() override = default;
  void BindEvent(const std::string& name, HarmonyValueRef callback);
  void OnParseEnd() override;
  void OnTextOverflow(MarkdownTextOverflow overflow) override;
  void OnDrawStart() override;
  void OnDrawEnd() override;
  void OnAnimationStep(int32_t animation_step,
                       int32_t max_animation_step) override;
  void OnLinkClicked(const char* url, const char* content) override;
  void OnImageClicked(const char* url) override;
  void OnSelectionChanged(int32_t start_index, int32_t end_index,
                          SelectionHandleType handle,
                          SelectionState state) override;

 private:
  static std::string_view ToString(MarkdownTextOverflow overflow);
  static std::string_view ToString(SelectionHandleType handle_type);
  static std::string_view ToString(SelectionState state);
  napi_env env_;
  HarmonyValueRef parse_end_;
  HarmonyValueRef text_overflow_;
  HarmonyValueRef draw_start_;
  HarmonyValueRef draw_end_;
  HarmonyValueRef animation_step_;
  HarmonyValueRef link_clicked_;
  HarmonyValueRef image_clicked_;
  HarmonyValueRef selection_changed_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_EVENT_LISTENER_H_
