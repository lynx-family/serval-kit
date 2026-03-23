// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "testing/markdown/mock_markdown_frame_driver.h"

#include <utility>

namespace serval::markdown::testing {

rapidjson::Value MockMarkdownFrameDriver::MakeRectValue(
    RectF rect, rapidjson::Document::AllocatorType& alloc) {
  rapidjson::Value value;
  value.SetObject();
  value.AddMember("left", rect.GetLeft(), alloc);
  value.AddMember("top", rect.GetTop(), alloc);
  value.AddMember("right", rect.GetRight(), alloc);
  value.AddMember("bottom", rect.GetBottom(), alloc);
  return value;
}

bool MockMarkdownFrameDriver::FlushView(MockMarkdownPlatformView* view) const {
  if (view == nullptr) {
    return false;
  }
  const bool need_measure = view->TakeNeedsMeasure();
  const bool need_align = view->TakeNeedsAlign() || need_measure;
  const bool need_draw = view->TakeNeedsDraw();
  const bool visible = view->IsVisible();

  if (need_measure) {
    view->Measure(measure_spec_);
  }
  if (need_align) {
    view->Align(align_position_.x_, align_position_.y_);
  }

  const bool should_draw = need_measure || need_align || need_draw;
  if (should_draw && visible) {
    if (view == main_view_) {
      view->Draw(canvas_, 0, 0);
    } else {
      const auto position = view->GetAlignedPosition();
      canvas_->Save();
      canvas_->Translate(position.x_, position.y_);
      view->Draw(canvas_, 0, 0);
      canvas_->Restore();
    }
  }
  return should_draw;
}

rapidjson::Document MockMarkdownFrameDriver::Run(
    const std::vector<MockMarkdownFrameStep>& steps) {
  rapidjson::Document result;
  result.SetArray();
  auto& alloc = result.GetAllocator();

  std::vector<MockMarkdownFrameStep> actual_steps = steps;
  if (actual_steps.empty()) {
    actual_steps.emplace_back();
  }

  for (size_t i = 0; i < actual_steps.size(); i++) {
    const auto& step = actual_steps[i];
    const auto visible_rect =
        step.has_visible_rect_ ? step.visible_rect_ : default_visible_rect_;
    main_view_->SetViewRectInScreen(visible_rect);
    canvas_->ResetResult();

    bool need_draw = false;
    int32_t draw_view_count = 0;

    const auto flush_all_pending = [&]() {
      constexpr int32_t kMaxFlushPass = 8;
      for (int32_t pass = 0; pass < kMaxFlushPass; pass++) {
        bool flushed = false;
        if (FlushView(main_view_)) {
          need_draw = true;
          draw_view_count++;
          flushed = true;
        }
        const auto subviews = main_view_->GetSubviews();
        for (auto* view : subviews) {
          if (FlushView(view)) {
            need_draw = true;
            draw_view_count++;
            flushed = true;
          }
        }
        if (!flushed) {
          break;
        }
      }
    };

    flush_all_pending();
    current_timestamp_ms_ += step.interval_ms_;
    main_view_->OnVSync(current_timestamp_ms_);
    flush_all_pending();

    rapidjson::Value frame;
    frame.SetObject();
    frame.AddMember("step", static_cast<int32_t>(i), alloc);
    frame.AddMember("timestamp", current_timestamp_ms_, alloc);
    frame.AddMember("interval", step.interval_ms_, alloc);
    frame.AddMember("visible_rect", MakeRectValue(visible_rect, alloc), alloc);
    frame.AddMember("need_draw", need_draw, alloc);
    frame.AddMember("draw_view_count", draw_view_count, alloc);
    rapidjson::Value ops;
    ops.CopyFrom(canvas_->GetJson(), alloc);
    frame.AddMember("ops", std::move(ops), alloc);
    result.PushBack(std::move(frame), alloc);
  }
  return result;
}

}  // namespace serval::markdown::testing
