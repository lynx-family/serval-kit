// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/harmony/internal/harmony_event_listener.h"
#include <string>
#include <utility>
#include "base/include/string/string_utils.h"
#include "markdown/platform/harmony/internal/harmony_utils.h"

namespace lynx::markdown {
HarmonyEventListener::HarmonyEventListener(napi_env env) : env_(env) {}
void HarmonyEventListener::BindEvent(const std::string& name,
                                     HarmonyValueRef callback) {
  if (name == "parseEnd") {
    parse_end_ = std::move(callback);
  } else if (name == "textOverflow") {
    text_overflow_ = std::move(callback);
  } else if (name == "drawStart") {
    draw_start_ = std::move(callback);
  } else if (name == "drawEnd") {
    draw_end_ = std::move(callback);
  } else if (name == "animationStep") {
    animation_step_ = std::move(callback);
  } else if (name == "linkClicked") {
    link_clicked_ = std::move(callback);
  } else if (name == "imageClicked") {
    image_clicked_ = std::move(callback);
  } else if (name == "selectionChanged") {
    selection_changed_ = std::move(callback);
  }
}
void HarmonyEventListener::OnParseEnd() {
  if (parse_end_.IsNull())
    return;
  HarmonyValues::CallFunction(env_, nullptr, parse_end_.GetValue());
}
void HarmonyEventListener::OnTextOverflow(MarkdownTextOverflow overflow) {
  if (text_overflow_.IsNull())
    return;
  HarmonyValues::CallFunction(env_, nullptr, text_overflow_.GetValue(),
                              ToString(overflow));
}
void HarmonyEventListener::OnDrawStart() {
  if (draw_start_.IsNull())
    return;
  HarmonyValues::CallFunction(env_, nullptr, draw_start_.GetValue());
}
void HarmonyEventListener::OnDrawEnd() {
  if (draw_end_.IsNull())
    return;
  HarmonyValues::CallFunction(env_, nullptr, draw_end_.GetValue());
}
void HarmonyEventListener::OnAnimationStep(int32_t animation_step,
                                           int32_t max_animation_step) {
  if (animation_step_.IsNull())
    return;
  HarmonyUIThread::PostTask([this, animation_step, max_animation_step]() {
    HarmonyValues::CallFunction(env_, nullptr, animation_step_.GetValue(),
                                animation_step, max_animation_step);
  });
}
void HarmonyEventListener::OnLinkClicked(const char* url, const char* content) {
  if (link_clicked_.IsNull())
    return;
  HarmonyValues::CallFunction(env_, nullptr, link_clicked_.GetValue(), url,
                              content);
}
void HarmonyEventListener::OnImageClicked(const char* url) {
  if (image_clicked_.IsNull())
    return;
  HarmonyValues::CallFunction(env_, nullptr, image_clicked_.GetValue(), url);
}
std::string_view HarmonyEventListener::ToString(MarkdownTextOverflow overflow) {
  switch (overflow) {
    case MarkdownTextOverflow::kClip:
      return "clip";
    case MarkdownTextOverflow::kEllipsis:
      return "ellipsis";
  }
}
std::string_view HarmonyEventListener::ToString(
    SelectionHandleType handle_type) {
  switch (handle_type) {
    case SelectionHandleType::kLeftHandle:
      return "forward";
    case SelectionHandleType::kRightHandle:
      return "backward";
  }
}

std::string_view HarmonyEventListener::ToString(SelectionState state) {
  switch (state) {
    case SelectionState::kEnter:
      return "enter";
    case SelectionState::kMove:
      return "move";
    case SelectionState::kStop:
      return "stop";
    case SelectionState::kExit:
      return "exit";
  }
}

void HarmonyEventListener::OnSelectionChanged(int32_t start_index,
                                              int32_t end_index,
                                              SelectionHandleType handle,
                                              SelectionState state) {
  if (selection_changed_.IsNull())
    return;
  HarmonyValues::CallFunction(env_, nullptr, selection_changed_.GetValue(),
                              start_index, end_index, ToString(handle),
                              ToString(state));
}
}  // namespace lynx::markdown
