// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/android/markdown_java_canvas_helper.h"

#include "markdown/draw/markdown_path.h"
#include "markdown/platform/android/markdown_run_delegate.h"
MarkdownJavaCanvasHelper::MarkdownJavaCanvasHelper() {
  stream_ = &GetBuffer();
}
MarkdownJavaCanvasHelper::~MarkdownJavaCanvasHelper() = default;
void MarkdownJavaCanvasHelper::DrawRunDelegate(
    const tttext::RunDelegate* delegate, float left, float top, float right,
    float bottom, tttext::Painter* painter) {
  auto* java_delegate = static_cast<const MarkdownRunDelegate*>(delegate);
  float radius = java_delegate->GetRadius();
  stream_->WriteInt8(static_cast<int8_t>(tttext::CanvasOp::kDrawRunDelegate));
  stream_->WriteInt32(java_delegate->GetID());
  stream_->WriteFloat(left);
  stream_->WriteFloat(top);
  stream_->WriteFloat(right);
  stream_->WriteFloat(bottom);
  stream_->WriteFloat(radius);
}

void MarkdownJavaCanvasHelper::ClipPath(lynx::markdown::MarkdownPath* path) {
  stream_->WriteInt8(kCanvasOPExtend);
  stream_->WriteInt8(static_cast<int8_t>(MarkdownCanvasOpExtend::kClipPath));
  WritePath(path);
}

void MarkdownJavaCanvasHelper::DrawMarkdownPath(
    lynx::markdown::MarkdownPath* path, tttext::Painter* painter) {
  stream_->WriteInt8(kCanvasOPExtend);
  stream_->WriteInt8(static_cast<int8_t>(MarkdownCanvasOpExtend::kDrawPath));
  WritePath(path);
  WritePaint(painter);
}

void MarkdownJavaCanvasHelper::DrawDelegateOnPath(
    tttext::RunDelegate* run_delegate, lynx::markdown::MarkdownPath* path,
    tttext::Painter* painter) {
  stream_->WriteInt8(kCanvasOPExtend);
  stream_->WriteInt8(
      static_cast<int8_t>(MarkdownCanvasOpExtend::kDrawDelegateOnPath));
  auto* java_delegate = static_cast<const TTTextRunDelegate*>(run_delegate);
  stream_->WriteInt32(java_delegate->GetID());
  WritePath(path);
  WritePaint(painter);
}

void MarkdownJavaCanvasHelper::WritePaint(tttext::Painter* painter) {
  stream_->WriteFloat(painter->GetStrokeWidth());
  stream_->WriteInt32(painter->GetFillColor());
  stream_->WriteInt32(painter->GetStrokeColor());
  stream_->WriteFloat(painter->GetTextSize());
  int8_t flag = 0;
  if (painter->IsBold()) {
    flag = flag | (1 << 2);
  }
  if (painter->IsItalic()) {
    flag = flag | (1 << 3);
  }
  if (painter->IsUnderLine()) {
    flag = flag | (1 << 4);
  }
  stream_->WriteInt8(flag);
}

void MarkdownJavaCanvasHelper::WritePath(lynx::markdown::MarkdownPath* path) {
  auto size = static_cast<int32_t>(path->path_ops_.size());
  stream_->WriteInt32(size);
  for (const auto& op : path->path_ops_) {
    stream_->WriteInt8(static_cast<int8_t>(op.op_));
    switch (op.op_) {
      case lynx::markdown::MarkdownPath::kArc: {
        auto& arc = op.data_.arc_;
        WritePoint(arc.center_);
        stream_->WriteFloat(arc.radius_);
        stream_->WriteFloat(arc.start_angle_);
        stream_->WriteFloat(arc.end_angle_);
      } break;
      case lynx::markdown::MarkdownPath::kOval: {
        WriteRect(op.data_.rect_);
      } break;
      case lynx::markdown::MarkdownPath::kRect: {
        WriteRect(op.data_.rect_);
        break;
      }
      case lynx::markdown::MarkdownPath::kRoundRect: {
        WriteRect(op.data_.round_rect_.rect_);
        stream_->WriteFloat(op.data_.round_rect_.radius_x_);
        stream_->WriteFloat(op.data_.round_rect_.radius_y_);
        break;
      }
      case lynx::markdown::MarkdownPath::kMoveTo: {
        WritePoint(op.data_.point_);
        break;
      }
      case lynx::markdown::MarkdownPath::kLineTo: {
        WritePoint(op.data_.point_);
        break;
      }
      case lynx::markdown::MarkdownPath::kCubicTo: {
        WritePoint(op.data_.cubic_.control_1_);
        WritePoint(op.data_.cubic_.control_2_);
        WritePoint(op.data_.cubic_.end_);
        break;
      }
      case lynx::markdown::MarkdownPath::kQuadTo: {
        WritePoint(op.data_.quad_.control_);
        WritePoint(op.data_.quad_.end_);
        break;
      }
    }
  }
}
