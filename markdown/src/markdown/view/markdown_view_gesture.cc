// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/view/markdown_view_gesture.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "markdown/element/markdown_document.h"
#include "markdown/element/markdown_page.h"
#include "markdown/markdown_event_listener.h"
#include "markdown/utils/markdown_screen_metrics.h"
#include "markdown/view/markdown_platform_view.h"
#include "markdown/view/markdown_selection_view.h"
#include "markdown/view/markdown_view_renderer.h"

namespace serval::markdown {
MarkdownViewGesture::MarkdownViewGesture(MarkdownViewContainerHandle* handle,
                                         MarkdownContext* context,
                                         MarkdownViewRenderer* renderer)
    : handle_(handle),
      context_(context),
      renderer_(renderer),
      selection_handle_size_(MarkdownScreenMetrics::DPToPx(10)),
      selection_handle_touch_margin_(MarkdownScreenMetrics::DPToPx(20)) {}

void MarkdownViewGesture::SetRenderer(MarkdownViewRenderer* renderer) {
  renderer_ = renderer;
  ResetPan();
}

void MarkdownViewGesture::SetDocument(
    std::shared_ptr<MarkdownDocument> document) {
  if (document_.get() == document.get()) {
    document_ = std::move(document);
    return;
  }
  document_ = std::move(document);
  if (is_in_selection_) {
    UpdateSelectionRects(SelectionState::kMove);
  }
}

void MarkdownViewGesture::SetEventListener(MarkdownEventListener* listener) {
  event_listener_ = listener;
}

void MarkdownViewGesture::SetEnableSelection(bool enable_selection) {
  enable_selection_ = enable_selection;
  CreateSelectionHandles();
}

void MarkdownViewGesture::SetSelectionHandleSize(float size) {
  selection_handle_size_ = size;
  if (selection_handles_.left_ != nullptr) {
    GetSelectionHandle(selection_handles_.left_)
        ->SetSize(selection_handle_size_);
    selection_handles_.left_->Measure(MeasureSpec{});
    GetSelectionHandle(selection_handles_.right_)
        ->SetSize(selection_handle_size_);
    selection_handles_.right_->Measure(MeasureSpec{});
  }
}

void MarkdownViewGesture::SetSelectionHandleTouchMargin(float margin) {
  selection_handle_touch_margin_ = margin;
}

void MarkdownViewGesture::SetSelectionHandleColor(uint32_t color) {
  selection_handle_color_ = color;
  if (selection_highlight_ != nullptr) {
    GetSelectionHighlight(selection_highlight_)
        ->SetHandleColor(selection_handle_color_);
  }
  if (selection_handles_.left_ != nullptr) {
    GetSelectionHandle(selection_handles_.left_)
        ->SetColor(selection_handle_color_);
    GetSelectionHandle(selection_handles_.right_)
        ->SetColor(selection_handle_color_);
  }
}

void MarkdownViewGesture::SetSelectionHighlightColor(uint32_t color) {
  selection_highlight_color_ = color;
  if (selection_highlight_ != nullptr) {
    GetSelectionHighlight(selection_highlight_)
        ->SetColor(selection_highlight_color_);
  }
}

void MarkdownViewGesture::SetTextSelection(const Range char_range,
                                           int32_t visible_char_count) {
  select_start_index_ = char_range.start_;
  select_end_index_ = char_range.end_;
  if (char_range.start_ >= 0 && char_range.end_ >= 0) {
    is_in_selection_ = true;
    select_start_index_ = std::min(visible_char_count, select_start_index_);
    select_end_index_ = std::min(visible_char_count, select_end_index_);
    UpdateSelectionRects(SelectionState::kStop);
  } else {
    is_in_selection_ = false;
    UpdateSelectionRects(SelectionState::kExit);
  }
  RecalculateSelectionPosition();
}

Range MarkdownViewGesture::GetSelectedRange() const {
  return {select_start_index_, select_end_index_};
}

std::string MarkdownViewGesture::GetSelectedText() const {
  if (document_ == nullptr) {
    return {};
  }
  return document_->GetContentByCharPos(select_start_index_, select_end_index_);
}

const std::vector<RectF>& MarkdownViewGesture::GetSelectedLineBoundingRect()
    const {
  return selection_highlight_rects_;
}

PointF MarkdownViewGesture::GetSelectionHandlePosition() const {
  if (!is_in_selection_ || selection_highlight_rects_.empty()) {
    return {-1, -1};
  }
  if (is_adjust_start_pos_) {
    const auto& rect = selection_highlight_rects_.front();
    return {rect.GetLeft(), rect.GetBottom()};
  }
  const auto& rect = selection_highlight_rects_.back();
  return {rect.GetRight(), rect.GetBottom()};
}

float MarkdownViewGesture::GetSelectionHandleRadius() const {
  return selection_handle_size_ * 0.5f;
}

bool MarkdownViewGesture::OnTap(PointF position, GestureEventType event) {
  if (event != GestureEventType::kDown) {
    return false;
  }
  if (is_in_selection_) {
    ExitSelection();
    return true;
  }
  if (document_ == nullptr || event_listener_ == nullptr) {
    return false;
  }
  const auto* link = document_->GetLinkByTouchPosition(position);
  if (link != nullptr) {
    event_listener_->OnLinkClicked(link->url_.c_str(), link->content_.c_str());
    return true;
  }
  const auto image = document_->GetImageByTouchPosition(position);
  if (!image.empty()) {
    event_listener_->OnImageClicked(image.c_str());
    return true;
  }
  return false;
}

bool MarkdownViewGesture::OnLongPress(PointF position, GestureEventType event) {
  if (document_ == nullptr || event != GestureEventType::kDown) {
    return false;
  }
  if (enable_selection_ && !is_in_selection_) {
    EnterSelection(position);
    return true;
  }
  if (is_in_selection_) {
    SendSelectionChanged(SelectionState::kStop);
    RecalculateSelectionPosition();
    return true;
  }
  return false;
}

bool MarkdownViewGesture::ShouldBeginPan(PointF position, PointF motion) {
  if (pan_target_ != PanTarget::kNone) {
    return true;
  }
  auto result = HitTestPanTarget(position, motion);
  if (result.target_ == PanTarget::kSelectionEndHandle ||
      result.target_ == PanTarget::kSelectionStartHandle) {
    return true;
  }
  if (CanBeginLongPressSelectionDrag(position, motion)) {
    return true;
  }
  if (long_press_selection_drag_pending_ && motion.LengthToZero() > 0) {
    ClearLongPressSelectionDrag();
  }
  if (result.target_ == PanTarget::kScrollXRegion && motion.x_ != 0) {
    return true;
  }
  return false;
}

bool MarkdownViewGesture::OnPan(PointF position, PointF motion,
                                GestureEventType event) {
  switch (event) {
    case GestureEventType::kDown:
      return BeginPan(position, motion);
    case GestureEventType::kMove:
      return UpdatePan(position, motion);
    case GestureEventType::kUp:
    case GestureEventType::kCancel:
      return EndPan(position, motion, event);
    case GestureEventType::kUnknown:
    default:
      return false;
  }
}

void MarkdownViewGesture::OnScrollXRegionChanged() {
  if (!is_in_selection_) {
    return;
  }
  UpdateSelectionRects();
  RecalculateSelectionPosition();
  if (selection_highlight_ != nullptr) {
    selection_highlight_->RequestDraw();
  }
  if (selection_handles_.left_ != nullptr) {
    selection_handles_.left_->RequestDraw();
  }
  if (selection_handles_.right_ != nullptr) {
    selection_handles_.right_->RequestDraw();
  }
}

bool MarkdownViewGesture::HitTestSelectionHandle(
    PointF position, SelectionHandleType* handle_type) const {
  if (!is_in_selection_ || !HasSelectionViews()) {
    return false;
  }
  const auto hit_handle = [](const std::shared_ptr<MarkdownPlatformView>& view,
                             PointF point, float margin) {
    if (view == nullptr) {
      return false;
    }
    const auto origin = view->GetAlignedPosition();
    const auto size = view->GetMeasuredSize();
    return RectF::MakeLTWH(origin.x_ - margin, origin.y_ - margin,
                           size.width_ + 2 * margin, size.height_ + 2 * margin)
        .Contains(point.x_, point.y_);
  };
  if (hit_handle(selection_handles_.left_, position,
                 selection_handle_touch_margin_)) {
    *handle_type = SelectionHandleType::kLeftHandle;
    return true;
  }
  if (hit_handle(selection_handles_.right_, position,
                 selection_handle_touch_margin_)) {
    *handle_type = SelectionHandleType::kRightHandle;
    return true;
  }
  return false;
}

bool MarkdownViewGesture::GetScrollXRegionAtPosition(
    PointF position, uint32_t* region_index, float* scroll_offset) const {
  const auto page = GetPage();
  if (page == nullptr) {
    return false;
  }
  const uint32_t region_count = page->GetRegionCount();
  for (uint32_t i = 0; i < region_count; ++i) {
    const auto* region = page->GetRegion(i);
    if (region == nullptr || !region->scroll_x_) {
      continue;
    }
    if (!region->scroll_x_view_rect_.Contains(position.x_, position.y_)) {
      continue;
    }
    if (region_index != nullptr) {
      *region_index = i;
    }
    if (scroll_offset != nullptr) {
      *scroll_offset = region->scroll_x_offset_;
    }
    return true;
  }
  return false;
}

bool MarkdownViewGesture::ScrollXRegionTo(uint32_t region_index,
                                          float scroll_offset) {
  if (document_ == nullptr) {
    return false;
  }
  if (!document_->GetScrollXRegionOffset(region_index, nullptr)) {
    return false;
  }
  if (!document_->SetScrollXRegionOffset(region_index, scroll_offset)) {
    return true;
  }
  document_->UpdateInlineViewBoundsInRegion(region_index);
  OnScrollXRegionChanged();
  if (renderer_ != nullptr) {
    renderer_->RequestDrawRegion(region_index);
  }
  return true;
}

int32_t MarkdownViewGesture::GetCharIndexByPosition(PointF position) const {
  auto page = GetPage();
  if (!page) {
    return -1;
  }
  auto [start, end] = MarkdownSelection::GetCharRangeByPoint(
      page.get(), position, MarkdownSelection::CharRangeType::kChar);
  return start;
}

Range MarkdownViewGesture::GetCharRangeByPosition(
    PointF position, MarkdownSelection::CharRangeType char_range_type) const {
  auto page = GetPage();
  if (page == nullptr) {
    return {};
  }
  return MarkdownSelection::GetCharRangeByPoint(page.get(), position,
                                                char_range_type);
}

std::vector<RectF> MarkdownViewGesture::GetTextLineBoundingRect(
    Range range) const {
  auto page = GetPage();
  if (page == nullptr) {
    return {};
  }
  return MarkdownSelection::GetSelectionRectByCharPos(page.get(), range.start_,
                                                      range.end_);
}

RectF MarkdownViewGesture::GetTextBoundingRect(Range range) const {
  auto page = GetPage();
  if (page == nullptr) {
    return {};
  }
  return MarkdownSelection::GetSelectionClosedRectByCharPos(
      page.get(), range.start_, range.end_);
}

std::string MarkdownViewGesture::GetParsedContent(Range char_range) const {
  if (document_ == nullptr) {
    return {};
  }
  return document_->GetContentByCharPos(char_range.start_, char_range.end_);
}

MarkdownViewGesture::PanHitTestResult MarkdownViewGesture::HitTestPanTarget(
    PointF position, PointF motion) const {
  PanHitTestResult result;
  const PointF start_position = position - motion;
  SelectionHandleType handle;
  const bool hit_handle = HitTestSelectionHandle(start_position, &handle);
  if (hit_handle && handle == SelectionHandleType::kLeftHandle) {
    result.target_ = PanTarget::kSelectionStartHandle;
    return result;
  }
  if (hit_handle && handle == SelectionHandleType::kRightHandle) {
    result.target_ = PanTarget::kSelectionEndHandle;
    return result;
  }
  if (GetScrollXRegionAtPosition(start_position, &result.scroll_x_region_index_,
                                 &result.scroll_x_origin_offset_) &&
      IsHorizontalPan(motion)) {
    result.target_ = PanTarget::kScrollXRegion;
  }
  return result;
}

bool MarkdownViewGesture::IsHorizontalPan(PointF motion) {
  const float abs_x = std::abs(motion.x_);
  const float abs_y = std::abs(motion.y_);
  return abs_x >= abs_y;
}

bool MarkdownViewGesture::CanBeginLongPressSelectionDrag(PointF position,
                                                         PointF motion) const {
  if (!long_press_selection_drag_pending_ || !is_in_selection_ ||
      !enable_selection_ || motion.LengthToZero() <= 0) {
    return false;
  }
  return true;
}

void MarkdownViewGesture::ClearLongPressSelectionDrag() {
  long_press_selection_drag_pending_ = false;
  long_press_selection_drag_origin_ = {};
}

bool MarkdownViewGesture::BeginPan(PointF position, PointF motion) {
  if (pan_target_ == PanTarget::kSelectionStartHandle ||
      pan_target_ == PanTarget::kSelectionEndHandle) {
    return true;
  }
  if (pan_target_ == PanTarget::kScrollXRegion) {
    return IsHorizontalPan(motion);
  }
  const auto hit_result = HitTestPanTarget(position, motion);
  if (hit_result.target_ == PanTarget::kSelectionStartHandle) {
    pan_target_ = PanTarget::kSelectionStartHandle;
    return DispatchSelectionHandlePan(position, {0, 0},
                                      GestureEventType::kDown);
  }
  if (hit_result.target_ == PanTarget::kSelectionEndHandle) {
    pan_target_ = PanTarget::kSelectionEndHandle;
    return DispatchSelectionHandlePan(position, {0, 0},
                                      GestureEventType::kDown);
  }
  if (CanBeginLongPressSelectionDrag(position, motion)) {
    ClearLongPressSelectionDrag();
    pan_target_ = PanTarget::kSelectionEndHandle;
    return DispatchSelectionHandlePan(position, {0, 0},
                                      GestureEventType::kDown);
  }
  if (long_press_selection_drag_pending_) {
    ClearLongPressSelectionDrag();
  }
  if (hit_result.target_ == PanTarget::kScrollXRegion) {
    pan_target_ = PanTarget::kScrollXRegion;
    scroll_x_region_index_ = hit_result.scroll_x_region_index_;
    scroll_x_origin_offset_ = hit_result.scroll_x_origin_offset_;
    scroll_x_scrolling_ = false;
    return true;
  }
  return false;
}

bool MarkdownViewGesture::UpdatePan(PointF position, PointF motion) {
  switch (pan_target_) {
    case PanTarget::kSelectionStartHandle:
    case PanTarget::kSelectionEndHandle:
      return DispatchSelectionHandlePan(position, motion,
                                        GestureEventType::kMove);
    case PanTarget::kScrollXRegion: {
      if (!scroll_x_scrolling_) {
        const float abs_x = std::abs(motion.x_);
        const float abs_y = std::abs(motion.y_);
        if (abs_x <= 1.f || abs_x < abs_y) {
          return false;
        }
      }
      scroll_x_scrolling_ = true;
      (void)ScrollXRegionTo(scroll_x_region_index_,
                            scroll_x_origin_offset_ + motion.x_);
      return true;
    }
    case PanTarget::kNone:
    default:
      return false;
  }
}

bool MarkdownViewGesture::EndPan(PointF position, PointF motion,
                                 GestureEventType event) {
  bool handled = false;
  switch (pan_target_) {
    case PanTarget::kSelectionStartHandle:
    case PanTarget::kSelectionEndHandle:
      handled = DispatchSelectionHandlePan(position, motion, event);
      break;
    case PanTarget::kScrollXRegion:
      handled = true;
      break;
    case PanTarget::kNone:
    default:
      handled = false;
      break;
  }
  ResetPan();
  return handled;
}

bool MarkdownViewGesture::DispatchSelectionHandlePan(PointF position,
                                                     PointF motion,
                                                     GestureEventType event) {
  if (pan_target_ == PanTarget::kSelectionStartHandle) {
    return OnStartHandleMove(position, motion, event);
  }
  if (pan_target_ == PanTarget::kSelectionEndHandle) {
    return OnEndHandleMove(position, motion, event);
  }
  return false;
}

void MarkdownViewGesture::ResetPan() {
  pan_target_ = PanTarget::kNone;
  scroll_x_region_index_ = 0;
  scroll_x_origin_offset_ = 0;
  scroll_x_scrolling_ = false;
  ClearLongPressSelectionDrag();
}

bool MarkdownViewGesture::HasSelectionViews() const {
  return selection_highlight_ != nullptr &&
         selection_handles_.left_ != nullptr &&
         selection_handles_.right_ != nullptr;
}

std::shared_ptr<MarkdownPage> MarkdownViewGesture::GetPage() const {
  if (document_ == nullptr) {
    return nullptr;
  }
  return document_->GetPage();
}

void MarkdownViewGesture::EnterSelection(PointF position) {
  ResetPan();
  is_in_selection_ = true;
  select_start_position_ = position;
  select_end_position_ = position;
  select_start_index_ = GetCharIndexByPosition(position);
  select_end_index_ = select_start_index_ + 1;
  is_adjust_start_pos_ = false;
  is_adjust_end_pos_ = false;
  handle_pan_before_motion_ = {0, 0};
  long_press_selection_drag_pending_ = true;
  long_press_selection_drag_origin_ = position;
  UpdateSelectionRects(SelectionState::kEnter);
}

void MarkdownViewGesture::ExitSelection() {
  is_in_selection_ = false;
  ClearLongPressSelectionDrag();
  UpdateSelectionRects(SelectionState::kExit);
}

void MarkdownViewGesture::UpdateSelectionStart() {
  auto new_start_index = GetCharIndexByPosition(select_start_position_);
  if (new_start_index == select_end_index_) {
    new_start_index++;
  }
  if (new_start_index == select_start_index_) {
    return;
  }
  select_start_index_ = new_start_index;
  if (select_start_index_ > select_end_index_) {
    SwapSelectionStartAndEnd();
  }
  UpdateSelectionRects(SelectionState::kMove);
}

void MarkdownViewGesture::UpdateSelectionEnd() {
  auto new_end_index = GetCharIndexByPosition(select_end_position_);
  new_end_index++;
  if (new_end_index == select_start_index_ && new_end_index > 0) {
    new_end_index--;
  }
  if (new_end_index == select_end_index_) {
    return;
  }
  select_end_index_ = new_end_index;
  if (select_end_index_ < select_start_index_) {
    SwapSelectionStartAndEnd();
  }
  UpdateSelectionRects(SelectionState::kMove);
}

void MarkdownViewGesture::SwapSelectionStartAndEnd() {
  std::swap(select_start_index_, select_end_index_);
  std::swap(select_start_position_, select_end_position_);
  std::swap(is_adjust_start_pos_, is_adjust_end_pos_);
}

void MarkdownViewGesture::RecalculateSelectionPosition() {
  is_adjust_start_pos_ = false;
  is_adjust_end_pos_ = false;
  handle_pan_before_motion_ = {0, 0};
  ClearLongPressSelectionDrag();
  if (selection_highlight_rects_.empty()) {
    return;
  }
  const auto& rect_before = selection_highlight_rects_.front();
  select_start_position_ =
      PointF{rect_before.GetLeft(),
             rect_before.GetTop() + rect_before.GetHeight() / 2};
  const auto& rect_end = selection_highlight_rects_.back();
  select_end_position_ =
      PointF{rect_end.GetRight(), rect_end.GetTop() + rect_end.GetHeight() / 2};
}

void MarkdownViewGesture::UpdateSelectionViews() const {
  if (!HasSelectionViews()) {
    return;
  }
  const bool visible = is_in_selection_ && !selection_highlight_rects_.empty();
  selection_highlight_->SetVisibility(visible);
  selection_handles_.left_->SetVisibility(visible);
  selection_handles_.right_->SetVisibility(visible);
  if (!visible) {
    return;
  }
  auto* highlight = GetSelectionHighlight(selection_highlight_);
  highlight->SetRects(selection_highlight_rects_);
  highlight->UpdateViewRect(selection_highlight_.get());
  selection_highlight_->RequestMeasure();

  const auto rect_start = selection_highlight_rects_.front();
  auto* handle_start = GetSelectionHandle(selection_handles_.left_);
  handle_start->SetTextHeight(rect_start.GetHeight());
  handle_start->UpdateViewRect({rect_start.GetLeft(), rect_start.GetBottom()},
                               selection_handles_.left_.get());
  selection_handles_.left_->RequestMeasure();

  const auto& rect_end = selection_highlight_rects_.back();
  auto* handle_end = GetSelectionHandle(selection_handles_.right_);
  handle_end->SetTextHeight(rect_end.GetHeight());
  handle_end->UpdateViewRect({rect_end.GetRight(), rect_end.GetBottom()},
                             selection_handles_.right_.get());
  selection_handles_.right_->RequestMeasure();
}

void MarkdownViewGesture::UpdateSelectionRects() {
  selection_highlight_rects_.clear();
  if (is_in_selection_) {
    auto page = GetPage();
    if (page != nullptr) {
      selection_highlight_rects_ = MarkdownSelection::GetSelectionRectByCharPos(
          page.get(), select_start_index_, select_end_index_);
    }
  }
  UpdateSelectionViews();
}

void MarkdownViewGesture::UpdateSelectionRects(SelectionState state) {
  UpdateSelectionRects();
  SendSelectionChanged(state);
}

void MarkdownViewGesture::CreateSelectionHandles() {
  if (selection_highlight_ != nullptr || handle_ == nullptr) {
    return;
  }
  selection_highlight_ =
      handle_->CreateSelectionHighlightSubView(selection_highlight_color_);
  GetSelectionHighlight(selection_highlight_)
      ->SetHandleColor(selection_handle_color_);
  selection_highlight_->SetVisibility(false);
  selection_handles_.left_ = handle_->CreateSelectionHandleSubView(
      SelectionHandleType::kLeftHandle, selection_handle_size_,
      selection_handle_color_);
  GetSelectionHandle(selection_handles_.left_)->SetMarkdownContext(context_);
  selection_handles_.left_->Measure(MeasureSpec{});
  selection_handles_.left_->SetVisibility(false);
  selection_handles_.right_ = handle_->CreateSelectionHandleSubView(
      SelectionHandleType::kRightHandle, selection_handle_size_,
      selection_handle_color_);
  GetSelectionHandle(selection_handles_.right_)->SetMarkdownContext(context_);
  selection_handles_.right_->Measure(MeasureSpec{});
  selection_handles_.right_->SetVisibility(false);
}

bool MarkdownViewGesture::OnStartHandleMove(PointF position, PointF motion,
                                            GestureEventType type) {
  if (!is_in_selection_) {
    return false;
  }
  if (!is_adjust_start_pos_ && !is_adjust_end_pos_) {
    is_adjust_start_pos_ = true;
  }
  return OnHandleMove(position, motion, type);
}

bool MarkdownViewGesture::OnEndHandleMove(PointF position, PointF motion,
                                          GestureEventType type) {
  if (!is_in_selection_) {
    return false;
  }
  if (!is_adjust_start_pos_ && !is_adjust_end_pos_) {
    is_adjust_end_pos_ = true;
  }
  return OnHandleMove(position, motion, type);
}

bool MarkdownViewGesture::OnHandleMove(PointF position, PointF motion,
                                       GestureEventType type) {
  if (type == GestureEventType::kDown) {
    handle_pan_before_motion_ = motion;
    return true;
  }
  PointF delta = motion - handle_pan_before_motion_;
  handle_pan_before_motion_ = motion;
  if (is_adjust_start_pos_) {
    select_start_position_ += delta;
    UpdateSelectionStart();
  } else if (is_adjust_end_pos_) {
    select_end_position_ += delta;
    UpdateSelectionEnd();
  }
  if (type == GestureEventType::kUp || type == GestureEventType::kCancel) {
    RecalculateSelectionPosition();
    SendSelectionChanged(SelectionState::kStop);
  }
  return true;
}

void MarkdownViewGesture::SendSelectionChanged(SelectionState state) const {
  if (event_listener_ != nullptr) {
    const auto start = is_in_selection_ ? select_start_index_ : -1;
    const auto end = is_in_selection_ ? select_end_index_ : -1;
    const auto handle = is_adjust_start_pos_
                            ? SelectionHandleType::kLeftHandle
                            : SelectionHandleType::kRightHandle;
    event_listener_->OnSelectionChanged(start, end, handle, state);
  }
}

MarkdownSelectionHandle* MarkdownViewGesture::GetSelectionHandle(
    const std::shared_ptr<MarkdownPlatformView>& view) {
  return static_cast<MarkdownSelectionHandle*>(
      view->GetCustomViewHandle()->GetDrawable());
}

MarkdownSelectionHighlight* MarkdownViewGesture::GetSelectionHighlight(
    const std::shared_ptr<MarkdownPlatformView>& view) {
  return static_cast<MarkdownSelectionHighlight*>(
      view->GetCustomViewHandle()->GetDrawable());
}
}  // namespace serval::markdown
