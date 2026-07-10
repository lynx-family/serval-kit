// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown_frame_driver.h"

#include <string>
#include <utility>

namespace serval::markdown::testing {
int64_t MarkdownFrameDriver::CurrentTimestamp() const {
  return current_timestamp_ms_;
}
void MarkdownFrameDriver::SetMeasureSpec(MeasureSpec spec) {
  spec_ = spec;
  spec_changed_ = true;
}
void MarkdownFrameDriver::FlushFrame() {
  current_timestamp_ms_ += FRAME_INTERVAL;
  if (main_view_->needs_measure_ || spec_changed_) {
    main_view_->Measure(spec_);
  }
  if (main_view_->needs_align_ || spec_changed_) {
    main_view_->Align(0, 0);
  }
  if (main_view_->needs_draw_ || spec_changed_) {
    canvas_->StartPaint();
    main_view_->Draw(canvas_, 0, 0);
    canvas_->EndPaint();
    auto& canvas_result = canvas_->GetJson();
    if (!canvas_result.Empty()) {
      rapidjson::Value frame_result;
      frame_result.SetObject();
      frame_result.AddMember("type", "render", result_.GetAllocator());
      frame_result.AddMember("timestamp", current_timestamp_ms_,
                             result_.GetAllocator());
      rapidjson::Value canvas_result_value;
      canvas_result_value.CopyFrom(canvas_result, result_.GetAllocator());
      frame_result.AddMember("result", std::move(canvas_result_value),
                             result_.GetAllocator());
      result_.PushBack(frame_result, result_.GetAllocator());
    }
  }
  if (spec_changed_) {
    spec_changed_ = false;
  }
  main_view_->OnVSync(current_timestamp_ms_);
}
const rapidjson::Document& MarkdownFrameDriver::RunSteps(
    const std::vector<MarkdownFrameStep>& steps) {
  result_.SetArray();
  for (auto& step : steps) {
    while (step.timestamp > CurrentTimestamp()) {
      FlushFrame();
    }
    for (auto& action : step.actions) {
      Act(action);
    }
    rapidjson::Value action_result;
    action_result.SetObject();
    action_result.AddMember("type", "action", result_.GetAllocator());
    action_result.AddMember("timestamp", current_timestamp_ms_,
                            result_.GetAllocator());
    action_result.AddMember("step_time", step.timestamp,
                            result_.GetAllocator());
    action_result.AddMember("action_count",
                            static_cast<int32_t>(step.actions.size()),
                            result_.GetAllocator());
    result_.PushBack(action_result, result_.GetAllocator());
  }
  while (spec_changed_ || main_view_->needs_measure_ ||
         main_view_->needs_draw_ || main_view_->needs_align_) {
    FlushFrame();
  }
  return result_;
}

void MarkdownFrameDriver::Act(const MarkdownAction& action) {
  switch (action.type) {
    case MarkdownActionType::kNone:
      break;
    case MarkdownActionType::kModifyMeasureSpec:
      SetMeasureSpec(std::get<MeasureSpec>(action.value));
      break;
    case MarkdownActionType::kModifyVisibleRect:
      main_view_->SetViewRectInScreen(std::get<RectF>(action.value));
      break;
    case MarkdownActionType::kModifyContent:
      main_view_->GetMarkdownView()->SetContent(
          std::get<std::string>(action.value));
      break;
    case MarkdownActionType::kTap: {
      auto info = std::get<GestureInfo>(action.value);
      main_view_->OnTap(info.point, info.type);
    } break;
    case MarkdownActionType::kLongPress: {
      auto info = std::get<GestureInfo>(action.value);
      main_view_->OnLongPress(info.point, info.type);
    } break;
    case MarkdownActionType::kPan: {
      auto info = std::get<GestureInfo>(action.value);
      main_view_->OnPan(info.point, info.motion, info.type);
    } break;
  }
}

}  // namespace serval::markdown::testing
