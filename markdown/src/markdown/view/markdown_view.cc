// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/view/markdown_view.h"

#include "markdown/draw/markdown_typewriter_drawer.h"
#include "markdown/layout/markdown_layout.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/view/markdown_platform_view.h"
#include "markdown/view/markdown_selection_view.h"
#include "markdown/view/markdown_view_animator.h"

namespace lynx::markdown {
MarkdownView::MarkdownView(MarkdownPlatformView* view)
    : view_(view), handle_(view->GetViewContainerHandle()) {
  view_->SetTapListener([this](PointF position, GestureEventType event) {
    OnTap(position, event);
  });
  view_->SetLongPressListener([this](PointF position, GestureEventType event) {
    OnLongPress(position, event);
  });
  view_->SetPanGestureListener(
      [this](PointF position, PointF motion, GestureEventType event) {
        OnPan(position, motion, event);
      });
}
MarkdownView::~MarkdownView() = default;
void MarkdownView::SetResourceLoader(MarkdownResourceLoader* loader) {
  resource_loader_ = loader;
  measurer_.SetResourceLoader(loader);
  NeedsMeasure();
}
MarkdownResourceLoader* MarkdownView::GetResourceLoader() const {
  return resource_loader_;
}
void MarkdownView::SetEventListener(MarkdownEventListener* listener) {
  event_listener_ = listener;
  measurer_.SetEventListener(listener);
  animator_.SetEventListener(listener);
}
void MarkdownView::SetExposureListener(MarkdownExposureListener* listener) {
  exposure_listener_ = listener;
}
void MarkdownView::SetContent(const std::string_view content) {
  measurer_.SetContent(content);
  NeedsMeasure();
}
void MarkdownView::SetContentID(std::string_view id) {
  measurer_.SetContentID(id);
}
void MarkdownView::SetContentComplete(bool complete) {
  measurer_.SetContentComplete(complete);
}
void MarkdownView::SetContentRange(Range range) {
  measurer_.SetContentRange(range);
  NeedsMeasure();
}
void MarkdownView::SetStyle(const ValueMap& style_map) {
  measurer_.SetStyle(style_map);
  NeedsMeasure();
}
void MarkdownView::ApplyStyleInRange(const ValueMap& style_map,
                                     int32_t char_start, int32_t char_end) {
  measurer_.ApplyStyleInRange(style_map, char_start, char_end);
}
void MarkdownView::SetTextMaxLines(int32_t max_lines) {
  measurer_.SetTextMaxLines(max_lines);
  NeedsMeasure();
}
void MarkdownView::SetEnableBreakAroundPunctuation(bool allow) {
  measurer_.SetEnableBreakAroundPunctuation(allow);
  NeedsMeasure();
}
void MarkdownView::SetTextAttachments(std::unique_ptr<Value> attachments) {
  attachments_ = std::move(attachments);
}
void MarkdownView::SetAnimationType(const MarkdownAnimationType type) {
  animator_.SetAnimationType(type);
  NeedsMeasure();
}
void MarkdownView::SetAnimationStep(int32_t animation_step) {
  animator_.SetAnimationStep(animation_step);
  NeedsMeasure();
}
void MarkdownView::SetTypewriterDynamicHeight(bool enable) {
  animator_.SetTypewriterDynamicHeight(enable);
  NeedsMeasure();
}
void MarkdownView::SetHeightTransitionDuration(float duration) {
  animator_.SetHeightTransitionDuration(duration);
}
void MarkdownView::SetTypewriterHeightTransitionPrefetch(bool enable) {
  typewriter_height_transition_prefetch_ = enable;
  view_->RequestMeasure();
}
void MarkdownView::SetAnimationVelocity(float velocity) {
  animator_.SetAnimationVelocity(velocity);
}
void MarkdownView::SetInitialAnimationStep(int32_t step) {
  animator_.SetInitialAnimationStep(step);
}
void MarkdownView::SetParserType(std::string_view parser_type,
                                 void* parser_ud) {
  measurer_.SetParserType(parser_type, parser_ud);
  NeedsMeasure();
}
void MarkdownView::SetSourceType(SourceType type) {
  measurer_.SetSourceType(type);
  NeedsMeasure();
}
void MarkdownView::SetEnableSelection(bool enable_selection) {
  enable_selection_ = enable_selection;
  CreateSelectionHandles();
}
void MarkdownView::SetSelectionHandleSize(float size) {
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
void MarkdownView::SetSelectionHandleTouchMargin(float margin) {
  selection_handle_touch_margin_ = margin;
  if (selection_handles_.left_ != nullptr) {
    GetSelectionHandle(selection_handles_.left_)->SetTouchMargin(margin);
    selection_handles_.left_->RequestMeasure();
    GetSelectionHandle(selection_handles_.right_)->SetTouchMargin(margin);
    selection_handles_.right_->RequestMeasure();
  }
}
void MarkdownView::SetSelectionHandleColor(uint32_t color) {
  selection_handle_color_ = color;
  if (selection_handles_.left_ != nullptr) {
    GetSelectionHandle(selection_handles_.left_)
        ->SetColor(selection_handle_color_);
    GetSelectionHandle(selection_handles_.right_)
        ->SetColor(selection_handle_color_);
  }
}
void MarkdownView::SetSelectionHighlightColor(uint32_t color) {
  selection_highlight_color_ = color;
  if (selection_highlight_ != nullptr) {
    GetSelectionHighlight(selection_highlight_)
        ->SetColor(selection_highlight_color_);
  }
}
void MarkdownView::CreateSelectionHandles() {
  if (selection_highlight_ != nullptr) {
    return;
  }
  selection_highlight_ = MarkdownSelectionHighlight::CreateView(
      handle_, selection_highlight_color_);
  selection_highlight_->SetVisibility(false);
  selection_handles_.left_ = MarkdownSelectionHandle::CreateView(
      handle_, SelectionHandleType::kLeftHandle, selection_handle_size_,
      selection_handle_touch_margin_, selection_handle_color_);
  selection_handles_.left_->Measure(MeasureSpec{});
  selection_handles_.left_->SetPanGestureListener(
      [this](PointF position, PointF motion, GestureEventType event) {
        OnStartHandleMove(position, motion, event);
      });
  selection_handles_.left_->SetVisibility(false);
  selection_handles_.right_ = MarkdownSelectionHandle::CreateView(
      handle_, SelectionHandleType::kRightHandle, selection_handle_size_,
      selection_handle_touch_margin_, selection_handle_color_);
  selection_handles_.right_->Measure(MeasureSpec{});
  selection_handles_.right_->SetPanGestureListener(
      [this](PointF position, PointF motion, GestureEventType event) {
        OnEndHandleMove(position, motion, event);
      });
  selection_handles_.right_->SetVisibility(false);
}
MeasureResult MarkdownView::OnMeasure(MeasureSpec spec) {
  measurer_.Measure(spec);
  if (measurer_.DidLayoutInLastMeasure()) {
    ClearForParse();
    auto before_views = GetInlineViews();
    document_ = measurer_.GetDocument();
    document_updated_ = true;
    auto after_views = GetInlineViews();
    RemoveUnusedViews(before_views, after_views);
    animator_.SetMaxAnimationStep(GetCharCount());
    draw_end_sent_ = false;
  }

  const auto measured = measurer_.GetMeasuredSize();
  float result_width = measured.width_;
  float result_height = measured.height_;
  float transition_target_height = result_height;
  SendDrawStart();
  if (document_ != nullptr &&
      animator_.GetAnimationType() == MarkdownAnimationType::kTypewriter &&
      animator_.GetTypewriterDynamicHeight()) {
    result_height = CalculateHeightByAnimationStep();
    transition_target_height = result_height;
    if (typewriter_height_transition_prefetch_ &&
        animator_.GetHeightTransitionDuration() > 0) {
      const int32_t prefetch_step = animator_.GetAnimationStepAfter(
          animator_.GetHeightTransitionDuration());
      transition_target_height =
          CalculateHeightByAnimationStep(prefetch_step, false);
    }
  }
  if (animator_.GetHeightTransitionDuration() > 0) {
    result_height = animator_.UpdateHeightTransition(transition_target_height);
  }
  return {.width_ = result_width,
          .height_ = result_height,
          .baseline_ = result_height};
}
void MarkdownView::Align(float x, float y) {
  measurer_.Align();
  if (custom_typewriter_cursor_ != nullptr &&
      custom_cursor_position_ != PointF{0, 0}) {
    custom_typewriter_cursor_->Align(custom_cursor_position_.x_,
                                     custom_cursor_position_.y_);
  }
}
void MarkdownView::Draw(tttext::ICanvasHelper* canvas, float x, float y) {
  const float left = x;
  const float top = y;
  SendDrawStart();
  if (document_updated_) {
    renderer_.SetDocument(document_);
    document_updated_ = false;
  }
  renderer_.SetTypewriterCursor(custom_typewriter_cursor_);
  renderer_.SetMarkdownAnimationStep(animator_.GetAnimationStep());
  renderer_.SetMarkdownAnimationType(animator_.GetAnimationType());
  renderer_.Draw(canvas, left, top);
  if (animator_.GetAnimationType() == MarkdownAnimationType::kNone) {
    SendDrawEnd();
  } else if (animator_.GetAnimationType() ==
                 MarkdownAnimationType::kTypewriter &&
             animator_.GetAnimationStep() >= animator_.GetMaxAnimationStep()) {
    SendDrawEnd();
  }
}
void MarkdownView::HideAllSubviews() {}
void MarkdownView::NeedsMeasure() {
  measurer_.NeedsMeasure();
  view_->RequestMeasure();
}
void MarkdownView::NeedsAlign() const {
  view_->RequestAlign();
}
void MarkdownView::NeedsDraw() const {
  view_->RequestDraw();
}
void MarkdownView::OnNextFrame(int64_t timestamp) {
  animator_.UpdateCurrentTime(timestamp);
  UpdateAnimationStep();
  UpdateTransitionHeight();
  UpdateExposure();
}
void MarkdownView::EnterSelection(PointF position) {
  is_in_selection_ = true;
  select_start_position_ = position;
  select_end_position_ = position;
  start_handle_position_ = end_handle_position_ = position;
  select_start_index_ = GetCharIndexByPosition(position);
  select_end_index_ = select_start_index_ + 1;
  is_adjust_start_pos_ = false;
  is_adjust_end_pos_ = false;
  handle_pan_before_motion_ = {0, 0};
  UpdateSelectionRects(SelectionState::kEnter);
}
void MarkdownView::ExitSelection() {
  is_in_selection_ = false;
  UpdateSelectionRects(SelectionState::kExit);
}
void MarkdownView::UpdateSelectionStart() {
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

void MarkdownView::UpdateSelectionEnd() {
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

void MarkdownView::SwapSelectionStartAndEnd() {
  std::swap(select_start_index_, select_end_index_);
  std::swap(select_start_position_, select_end_position_);
  std::swap(is_adjust_start_pos_, is_adjust_end_pos_);
}

void MarkdownView::RecalculateSelectionPosition() {
  is_adjust_start_pos_ = false;
  is_adjust_end_pos_ = false;
  handle_pan_before_motion_ = {0, 0};
  if (selection_highlight_rects_.empty())
    return;
  const auto& rect_before = selection_highlight_rects_.front();
  select_start_position_ =
      PointF{rect_before.GetLeft(),
             rect_before.GetTop() + rect_before.GetHeight() / 2};
  start_handle_position_ =
      PointF{rect_before.GetLeft(), rect_before.GetBottom()};
  const auto& rect_end = selection_highlight_rects_.back();
  select_end_position_ =
      PointF{rect_end.GetRight(), rect_end.GetTop() + rect_end.GetHeight() / 2};
  end_handle_position_ = PointF{rect_end.GetRight(), rect_end.GetBottom()};
}

void MarkdownView::UpdateSelectionViews() const {
  if (selection_highlight_ == nullptr || selection_handles_.left_ == nullptr ||
      selection_handles_.right_ == nullptr) {
    return;
  }
  const bool visible = is_in_selection_ && !selection_highlight_rects_.empty();
  selection_highlight_->SetVisibility(visible);
  selection_handles_.left_->SetVisibility(visible);
  selection_handles_.right_->SetVisibility(visible);
  if (!visible) {
    return;
  }
  GetSelectionHighlight(selection_highlight_)
      ->SetRects(selection_highlight_rects_);
  GetSelectionHighlight(selection_highlight_)
      ->UpdateViewRect(selection_highlight_);
  selection_highlight_->RequestMeasure();

  const auto rect_start = selection_highlight_rects_.front();
  auto* handle_start = GetSelectionHandle(selection_handles_.left_);
  handle_start->SetTextHeight(rect_start.GetHeight());
  handle_start->UpdateViewRect({rect_start.GetLeft(), rect_start.GetBottom()},
                               selection_handles_.left_);
  selection_highlight_->RequestMeasure();

  const auto& rect_end = selection_highlight_rects_.back();
  auto* handle_end = GetSelectionHandle(selection_handles_.right_);
  handle_end->SetTextHeight(selection_highlight_rects_.back().GetHeight());
  handle_end->UpdateViewRect({rect_end.GetRight(), rect_end.GetBottom()},
                             selection_handles_.right_);
  selection_highlight_->RequestMeasure();
}

void MarkdownView::UpdateSelectionRects(SelectionState state) {
  selection_highlight_rects_.clear();
  if (is_in_selection_ && document_ != nullptr) {
    auto page = document_->GetPage();
    if (page != nullptr) {
      selection_highlight_rects_ = MarkdownSelection::GetSelectionRectByCharPos(
          page.get(), select_start_index_, select_end_index_);
    }
  }
  UpdateSelectionViews();
  SendSelectionChanged(state);
}

int32_t MarkdownView::GetCharIndexByPosition(PointF position) {
  if (document_ == nullptr) {
    return -1;
  }
  auto page = document_->GetPage();
  if (!page)
    return -1;
  auto [start, end] = MarkdownSelection::GetCharRangeByPoint(
      page.get(), position, MarkdownSelection::CharRangeType::kChar);
  return start;
}

void MarkdownView::UpdateAnimationStep() {
  const auto result = animator_.UpdateTypewriterStep();
  if (result > 0) {
    if (animator_.GetTypewriterDynamicHeight()) {
      view_->RequestMeasure();
    } else {
      view_->RequestDraw();
    }
  }
}

void MarkdownView::UpdateTransitionHeight() const {
  if (animator_.IsHeightTransitionRunning()) {
    view_->RequestMeasure();
  }
}
void MarkdownView::SendImageClicked(const char* url) {
  if (event_listener_ == nullptr) {
    return;
  }
  event_listener_->OnImageClicked(url);
}
void MarkdownView::SendLinkClicked(const char* url, const char* content) {
  if (event_listener_ == nullptr) {
    return;
  }
  event_listener_->OnLinkClicked(url, content);
}
void MarkdownView::SendDrawStart() {
  if (!draw_start_sent_) {
    draw_start_sent_ = true;
    if (event_listener_ != nullptr) {
      event_listener_->OnDrawStart();
    }
  }
}
void MarkdownView::SendDrawEnd() {
  if (!draw_end_sent_) {
    draw_end_sent_ = true;
    if (event_listener_ != nullptr) {
      event_listener_->OnDrawEnd();
    }
  }
}
void MarkdownView::UpdateExposure() {
  if (exposure_listener_ == nullptr) {
    return;
  }
  if (document_ == nullptr) {
    return;
  }
  auto rect_in_screen = handle_->GetViewRectInScreen();
  auto images = document_->GetImageByViewRect(rect_in_screen);
  for (auto* image : images) {
    auto find = exposure_images_.find(image);
    if (find == exposure_images_.end()) {
      exposure_listener_->OnImageAppear(image->url_.c_str());
    } else {
      exposure_images_.erase(image);
    }
  }
  for (auto* image : exposure_images_) {
    exposure_listener_->OnImageDisappear(image->url_.c_str());
  }
  for (auto* image : images) {
    exposure_images_.emplace(image);
  }

  auto links = document_->GetLinksByViewRect(rect_in_screen);
  for (auto* item : links) {
    auto find = exposure_links_.find(item);
    if (find == exposure_links_.end()) {
      exposure_listener_->OnLinkAppear(item->url_.c_str(),
                                       item->content_.c_str());
    } else {
      exposure_links_.erase(item);
    }
  }
  for (auto* item : exposure_links_) {
    exposure_listener_->OnLinkDisappear(item->url_.c_str(),
                                        item->content_.c_str());
  }
  exposure_links_.clear();
  for (auto* item : links) {
    exposure_links_.emplace(item);
  }
}

void MarkdownView::ClearForParse() {
  exposure_images_.clear();
  exposure_links_.clear();
}

std::set<MarkdownPlatformView*> MarkdownView::GetInlineViews() {
  if (document_ == nullptr) {
    return {};
  }
  std::set<MarkdownPlatformView*> views;
  for (auto& image : document_->GetImages()) {
    if (image.image_ != nullptr) {
      views.emplace(static_cast<MarkdownPlatformView*>(image.image_));
    }
  }
  for (auto& inline_view : document_->GetInlineViews()) {
    if (inline_view.view_ != nullptr) {
      views.emplace(static_cast<MarkdownPlatformView*>(inline_view.view_));
    }
  }
  return views;
}

void MarkdownView::RemoveUnusedViews(
    const std::set<MarkdownPlatformView*>& before,
    const std::set<MarkdownPlatformView*>& after) const {
  std::vector<MarkdownPlatformView*> diff;
  std::set_difference(before.begin(), before.end(), after.begin(), after.end(),
                      std::inserter(diff, diff.begin()));
  for (const auto& view : diff) {
    handle_->RemoveSubView(view);
  }
}

void MarkdownView::RemoveInlineViews() {
  if (document_ == nullptr) {
    return;
  }
  for (auto& image : document_->GetImages()) {
    if (image.image_ != nullptr) {
      handle_->RemoveSubView(static_cast<MarkdownPlatformView*>(image.image_));
    }
  }
  for (auto& inline_view : document_->GetInlineViews()) {
    if (inline_view.view_ != nullptr) {
      handle_->RemoveSubView(
          static_cast<MarkdownPlatformView*>(inline_view.view_));
    }
  }
}

int32_t MarkdownView::GetCharCount() {
  if (document_ == nullptr) {
    return 0;
  }
  const auto page = document_->GetPage();
  if (page == nullptr) {
    return 0;
  }
  return MarkdownSelection::GetPageCharCount(page.get());
}

float MarkdownView::CalculateHeightByAnimationStep() {
  if (document_ == nullptr) {
    return measurer_.GetMeasuredSize().height_;
  }
  if (resource_loader_ == nullptr) {
    custom_typewriter_cursor_.reset();
    custom_cursor_position_ = {0, 0};
    return measurer_.GetMeasuredSize().height_;
  }
  EnsureTypewriterCursor();
  return CalculateHeightByAnimationStep(animator_.GetAnimationStep(), true);
}

void MarkdownView::EnsureTypewriterCursor() {
  auto& cursor_style = document_->GetStyle().typewriter_cursor_;
  const auto& selector = cursor_style.typewriter_cursor_.custom_cursor_;
  if (selector.empty()) {
    custom_typewriter_cursor_.reset();
    custom_typewriter_cursor_selector_.clear();
    return;
  }
  if (custom_typewriter_cursor_ != nullptr &&
      custom_typewriter_cursor_selector_ == selector) {
    return;
  }
  custom_typewriter_cursor_ = resource_loader_->LoadInlineView(
      selector.c_str(), document_->GetMaxWidth(), document_->GetMaxHeight());
  custom_typewriter_cursor_selector_ = selector;
}

float MarkdownView::CalculateHeightByAnimationStep(
    int32_t animation_step, bool update_cursor_position) {
  if (document_ == nullptr) {
    return measurer_.GetMeasuredSize().height_;
  }
  if (resource_loader_ == nullptr) {
    return measurer_.GetMeasuredSize().height_;
  }
  auto& cursor_style = document_->GetStyle().typewriter_cursor_;
  auto page = document_->GetPage();
  if (page == nullptr) {
    if (update_cursor_position) {
      custom_cursor_position_ = {0, 0};
    }
    return measurer_.GetMeasuredSize().height_;
  }
  MarkdownCharTypewriterDrawer drawer(
      nullptr, animation_step, document_->GetResourceLoader(), cursor_style,
      false, custom_typewriter_cursor_.get());
  drawer.CalculateCursorPosition(page.get());
  float typewriter_height = 0;
  if (animation_step < animator_.GetMaxAnimationStep()) {
    typewriter_height = drawer.GetMaxDrawHeight();
  } else {
    typewriter_height = measurer_.GetMeasuredSize().height_;
  }
  if (update_cursor_position) {
    custom_cursor_position_ = drawer.GetCursorPosition();
  }
  return typewriter_height;
}

void MarkdownView::SetTextSelection(const Range char_range) {
  select_start_index_ = char_range.start_;
  select_end_index_ = char_range.end_;
  if (char_range.start_ >= 0 && char_range.end_ >= 0) {
    is_in_selection_ = true;
    const auto draw_count =
        animator_.GetAnimationType() == MarkdownAnimationType::kNone
            ? GetCharCount()
            : animator_.GetAnimationStep();
    select_start_index_ = std::min(draw_count, select_start_index_);
    select_end_index_ = std::min(draw_count, select_end_index_);
    UpdateSelectionRects(SelectionState::kStop);
  } else {
    is_in_selection_ = false;
    UpdateSelectionRects(SelectionState::kExit);
  }
  RecalculateSelectionPosition();
}

void MarkdownView::SetTrimParagraphSpaces(bool trim_spaces) {
  trim_paragraph_spaces_ = trim_spaces;
}

Range MarkdownView::GetSelectedRange() const {
  return {select_start_index_, select_end_index_};
}

std::string MarkdownView::GetSelectedText() {
  if (document_ == nullptr) {
    return {};
  }
  return document_->GetContentByCharPos(select_start_index_, select_end_index_);
}

const std::vector<RectF>& MarkdownView::GetSelectedLineBoundingRect() {
  return selection_highlight_rects_;
}

Range MarkdownView::GetCharRangeByPosition(
    PointF position, MarkdownSelection::CharRangeType char_range_type) {
  if (document_ == nullptr) {
    return {};
  }
  auto page = document_->GetPage();
  if (page == nullptr)
    return {};
  return MarkdownSelection::GetCharRangeByPoint(page.get(), position,
                                                char_range_type);
}

RectF MarkdownView::GetTextBoundingRect(Range range) {
  if (document_ == nullptr) {
    return {};
  }
  auto page = document_->GetPage();
  if (page == nullptr)
    return {};
  return MarkdownSelection::GetSelectionClosedRectByCharPos(
      page.get(), range.start_, range.end_);
}

std::string MarkdownView::GetParsedContent(Range char_range) {
  if (document_ == nullptr) {
    return {};
  }
  return document_->GetContentByCharPos(char_range.start_, char_range.end_);
}

std::vector<RectF> MarkdownView::GetTextLineBoundingRect(Range range) {
  if (document_ == nullptr) {
    return {};
  }
  auto page = document_->GetPage();
  if (page == nullptr)
    return {};
  return MarkdownSelection::GetSelectionRectByCharPos(page.get(), range.start_,
                                                      range.end_);
}
void MarkdownView::SendSelectionChanged(SelectionState state) const {
  if (event_listener_ != nullptr) {
    const auto start = is_in_selection_ ? select_start_index_ : -1;
    const auto end = is_in_selection_ ? select_end_index_ : -1;
    const auto handle = is_adjust_start_pos_
                            ? SelectionHandleType::kLeftHandle
                            : SelectionHandleType::kRightHandle;
    event_listener_->OnSelectionChanged(start, end, handle, state);
  }
}

void MarkdownView::OnTap(PointF position, GestureEventType event) {
  if (event == GestureEventType::kDown) {
    if (is_in_selection_) {
      ExitSelection();
    }
    if (document_ == nullptr || event_listener_ == nullptr) {
      return;
    }
    const auto* link = document_->GetLinkByTouchPosition(position);
    if (link != nullptr) {
      event_listener_->OnLinkClicked(link->url_.c_str(),
                                     link->content_.c_str());
    }
    const auto image = document_->GetImageByTouchPosition(position);
    if (!image.empty()) {
      event_listener_->OnImageClicked(image.c_str());
    }
  }
}

void MarkdownView::OnLongPress(PointF position, GestureEventType event) {
  if (document_ == nullptr) {
    return;
  }
  if (event == GestureEventType::kDown && enable_selection_ &&
      !is_in_selection_) {
    EnterSelection(position);
  } else if (is_in_selection_) {
    SendSelectionChanged(SelectionState::kStop);
    RecalculateSelectionPosition();
  }
}

void MarkdownView::OnPan(PointF position, PointF motion,
                         GestureEventType event) {
  if (document_ == nullptr) {
    return;
  }
  auto state = MarkdownTouchState::kNone;
  if (event == GestureEventType::kDown) {
    state = document_->OnTouchEvent(MarkdownTouchEventType::kDown, position);
  } else if (event == GestureEventType::kMove) {
    state = document_->OnTouchEvent(MarkdownTouchEventType::kMove, position);
  } else {
    state = document_->OnTouchEvent(MarkdownTouchEventType::kUp, position);
  }
  if (state == MarkdownTouchState::kOnScroll) {
    NeedsAlign();
  }
}

void MarkdownView::OnStartHandleMove(PointF position, PointF motion,
                                     GestureEventType type) {
  if (!is_adjust_start_pos_ && !is_adjust_end_pos_) {
    is_adjust_start_pos_ = true;
  }
  OnHandleMove(position, motion, type);
}

void MarkdownView::OnEndHandleMove(PointF position, PointF motion,
                                   GestureEventType type) {
  if (!is_adjust_start_pos_ && !is_adjust_end_pos_) {
    is_adjust_end_pos_ = true;
  }
  OnHandleMove(position, motion, type);
}

void MarkdownView::OnHandleMove(PointF position, PointF motion,
                                GestureEventType type) {
  PointF delta = motion - handle_pan_before_motion_;
  handle_pan_before_motion_ = motion;
  if (is_adjust_start_pos_) {
    select_start_position_ += delta;
    UpdateSelectionStart();
  } else if (is_adjust_end_pos_) {
    select_end_position_ += delta;
    UpdateSelectionEnd();
  }
  if (type != GestureEventType::kMove) {
    RecalculateSelectionPosition();
    SendSelectionChanged(SelectionState::kStop);
  }
}

void MarkdownView::SetPadding(float padding) {
  SetPaddings(padding, padding, padding, padding);
}

void MarkdownView::SetPaddings(float left, float top, float right,
                               float bottom) {
  measurer_.SetPaddings({left, top, right, bottom});
}

void MarkdownView::SetNumberProp(MarkdownProps prop, double value) {
  switch (prop) {
    case MarkdownProps::kAnimationType:
      SetAnimationType(
          static_cast<MarkdownAnimationType>(static_cast<int32_t>(value)));
      break;
    case MarkdownProps::kAnimationVelocity:
      SetAnimationVelocity(static_cast<float>(value));
      break;
    case MarkdownProps::kTextMaxline:
      SetTextMaxLines(static_cast<int32_t>(value));
      break;
    case MarkdownProps::kContentComplete:
      SetContentComplete(static_cast<bool>(value));
      break;
    case MarkdownProps::kTypewriterDynamicHeight:
      SetTypewriterDynamicHeight(static_cast<bool>(value));
      break;
    case MarkdownProps::kInitialAnimationStep:
      SetInitialAnimationStep(static_cast<int32_t>(value));
      break;
    case MarkdownProps::kContentRangeStart:
      break;
    case MarkdownProps::kContentRangeEnd:
      break;
    case MarkdownProps::kTypewriterHeightTransitionDuration:
      SetHeightTransitionDuration(static_cast<float>(value));
      break;
    case MarkdownProps::kTypewriterHeightTransitionPrefetch:
      SetTypewriterHeightTransitionPrefetch(static_cast<bool>(value));
      break;
    case MarkdownProps::kAllowBreakAroundPunctuation:
      break;
    case MarkdownProps::kEnableTextSelection:
      SetEnableSelection(static_cast<bool>(value));
      break;
    case MarkdownProps::kSelectionHighlightColor:
      SetSelectionHighlightColor(static_cast<uint32_t>(value));
      break;
    case MarkdownProps::kSelectionHandleColor:
      SetSelectionHandleColor(static_cast<uint32_t>(value));
      break;
    case MarkdownProps::kSelectionHandleSize:
      SetSelectionHandleSize(static_cast<float>(value));
      break;
    case MarkdownProps::kSelectionHandleTouchMargin:
      SetSelectionHandleTouchMargin(static_cast<float>(value));
      break;
    default:
      break;
  }
}
void MarkdownView::SetStringProp(MarkdownProps prop, std::string_view value) {
  if (prop == MarkdownProps::kAnimationType) {
    if (value == "typewriter") {
      SetAnimationType(MarkdownAnimationType::kTypewriter);
    } else {
      SetAnimationType(MarkdownAnimationType::kNone);
    }
  }
}
void MarkdownView::SetArrayProp(MarkdownProps prop, const ValueArray& array) {}
void MarkdownView::SetMapProp(MarkdownProps prop, const ValueMap& map) {}
}  // namespace lynx::markdown
