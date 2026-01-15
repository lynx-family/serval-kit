// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/android/markdown_java_canvas_helper.h"

#include "markdown/platform/android/tttext_run_delegate.h"
MarkdownJavaCanvasHelper::MarkdownJavaCanvasHelper() {
  stream_ = &GetBuffer();
}
MarkdownJavaCanvasHelper::~MarkdownJavaCanvasHelper() = default;
void MarkdownJavaCanvasHelper::DrawRunDelegate(
    const tttext::RunDelegate* delegate, float left, float top, float right,
    float bottom, tttext::Painter* painter) {
  auto* java_delegate = static_cast<const TTTextRunDelegate*>(delegate);
  stream_->WriteInt8(static_cast<int8_t>(tttext::CanvasOp::kDrawRunDelegate));
  stream_->WriteInt32(java_delegate->GetID());
  stream_->WriteFloat(left);
  stream_->WriteFloat(top);
  stream_->WriteFloat(right);
  stream_->WriteFloat(bottom);
}
