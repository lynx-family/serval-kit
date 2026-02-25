// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/view/markdown_view.h"

#include "markdown/draw/markdown_drawer.h"
#include "markdown/draw/markdown_typewriter_drawer.h"
#include "markdown/layout/markdown_layout.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/parser/impl/markdown_parser_impl.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/utils/markdown_float_comparison.h"
#include "markdown/utils/markdown_trace.h"
#include "markdown/view/markdown_platform_view.h"
#include "markdown/view/markdown_selection_view.h"
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
void MarkdownView::SetPlatformLoader(MarkdownPlatformLoader* loader) {
  document_.SetResourceLoader(this);
  platform_loader_ = loader;
  NeedsParse();
}
MarkdownPlatformLoader* MarkdownView::GetPlatformLoader() const {
  return platform_loader_;
}
void MarkdownView::SetEventListener(MarkdownEventListener* listener) {
  event_listener_ = listener;
  document_.SetMarkdownEventListener(listener);
}
void MarkdownView::SetExposureListener(MarkdownExposureListener* listener) {
  exposure_listener_ = listener;
}
void MarkdownView::SetContent(const std::string_view content) {
  content_ = content;
  document_.SetMarkdownContent(content);
  NeedsParse();
}
void MarkdownView::SetContentID(std::string_view id) {
  content_id_ = id;
}
void MarkdownView::SetContentComplete(bool complete) {
  content_complete_ = complete;
}
void MarkdownView::SetContentRange(Range range) {
  content_range_ = range;
}
void MarkdownView::SetStyle(const ValueMap& style_map) {
  const auto style =
      MarkdownStyleReader::ReadStyle(style_map, document_.GetResourceLoader());
  style_map_ = style;
  document_.SetStyle(style);
  NeedsParse();
}
void MarkdownView::ApplyStyleInRange(const ValueMap& style_map,
                                     int32_t char_start, int32_t char_end) {
  const auto style = MarkdownStyleReader::ReadBaseStyle(
      style_map, document_.GetResourceLoader());
  document_.ApplyStyleInRange(style, {char_start, char_end});
  NeedsMeasure();
}
void MarkdownView::SetTextMaxLines(int32_t max_lines) {
  text_max_lines_ = max_lines;
  document_.SetMaxLines(max_lines);
  NeedsMeasure();
}
void MarkdownView::SetEnableBreakAroundPunctuation(bool allow) {
  allow_break_around_punctuation_ = allow;
}
void MarkdownView::SetTextAttachments(std::unique_ptr<Value> attachments) {
  attachments_ = std::move(attachments);
}
void MarkdownView::SetAnimationType(const MarkdownAnimationType type) {
  animation_type_ = type;
  NeedsMeasure();
}
void MarkdownView::SetAnimationStep(int32_t animation_step) {
  current_animation_step_ = animation_step;
  NeedsMeasure();
}
void MarkdownView::SetTypewriterDynamicHeight(bool enable) {
  typewriter_dynamic_height_ = enable;
  NeedsMeasure();
}
void MarkdownView::SetHeightTransitionDuration(float duration) {
  height_transition_duration_ = duration;
}
void MarkdownView::SetAnimationVelocity(float velocity) {
  animation_velocity_ = velocity;
}
void MarkdownView::SetInitialAnimationStep(int32_t step) {
  initial_animation_step_ = step;
}
void MarkdownView::SetParserType(std::string_view parser_type,
                                 void* parser_ud) {
  parser_type_ = parser_type;
  parser_ud_ = parser_ud;
  NeedsParse();
}
void MarkdownView::SetSourceType(SourceType type) {
  source_type_ = type;
  NeedsParse();
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
SizeF MarkdownView::Measure(MeasureSpec spec) {
  TraceEventBegin("MarkdownView::Measure");
  if (FloatsNotEqual(spec.width_, last_measure_spec_.width_)) {
    needs_measure_ = true;
    needs_parse_ = true;
  }
  if (FloatsNotEqual(spec.height_, last_measure_spec_.height_)) {
    needs_measure_ = true;
  }
  document_.SetMaxSize(spec.width_, spec.height_);
  last_measure_spec_ = spec;
  float result_width = measured_width_;
  float result_height = measured_height_;
  if (needs_parse_) {
    TraceEventBegin("MarkdownView::Parse");
    auto before_views = GetInlineViews();
    ClearForParse();
    if (source_type_ == SourceType::kMarkdown) {
      MarkdownParserImpl::ParseMarkdown(parser_type_, &document_, parser_ud_);
    } else if (source_type_ == SourceType::kPlainText) {
      MarkdownParserImpl::ParsePlainText(&document_);
    }
    auto after_views = GetInlineViews();
    RemoveUnusedViews(before_views, after_views);
    if (trim_paragraph_spaces_) {
      document_.TrimParagraphSpaces();
    }
    needs_parse_ = false;
    SendParseEnd();
    TraceEventEnd();
  }
  if (needs_measure_) {
    TraceEventBegin("MarkdownView::Layout");
    MarkdownLayout layout(&document_);
    layout.SetPaddings(paddings_);
    layout.Layout(spec.width_, spec.height_, -1);
    auto page = document_.GetPage();
    if (page != nullptr) {
      max_animation_step_ = MarkdownSelection::GetPageCharCount(page.get());
      measured_width_ = page->GetLayoutWidth();
      measured_height_ = page->GetLayoutHeight();
      result_width = measured_width_;
      result_height = measured_height_;
    }
    needs_measure_ = false;
    draw_end_sent_ = false;
    TraceEventEnd();
  }
  if (animation_type_ == MarkdownAnimationType::kTypewriter &&
      typewriter_dynamic_height_) {
    TraceEventBegin("MarkdownView::TypewriterUpdate");
    auto& cursor_style = document_.GetStyle().typewriter_cursor_;
    if (!cursor_style.typewriter_cursor_.custom_cursor_.empty()) {
      custom_typewriter_cursor_ = platform_loader_->LoadInlineView(
          cursor_style.typewriter_cursor_.custom_cursor_.c_str(),
          document_.GetMaxWidth(), document_.GetMaxHeight());
    } else {
      custom_typewriter_cursor_ = nullptr;
    }
    auto page = document_.GetPage();
    MarkdownViewDelegate cursor(custom_typewriter_cursor_,
                                document_.GetMaxWidth(),
                                document_.GetMaxHeight());
    MarkdownCharTypewriterDrawer drawer(
        nullptr, current_animation_step_, document_.GetResourceLoader(),
        cursor_style, false,
        (custom_typewriter_cursor_ == nullptr) ? nullptr : &cursor);
    drawer.CalculateCursorPosition(page.get());
    if (current_animation_step_ < max_animation_step_) {
      current_typewriter_height_ = drawer.GetMaxDrawHeight();
    } else {
      current_typewriter_height_ = measured_height_;
    }
    result_height = current_typewriter_height_;
    custom_cursor_position_ = drawer.GetCursorPosition();
    TraceEventEnd();
  }
  if (height_transition_duration_ > 0) {
    if (transition_start_time_ == 0) {
      transition_start_height_ = result_height;
      transition_end_height_ = result_height;
      transition_start_time_ = current_frame_time_;
      current_transition_height_ = result_height;
      current_transition_time_ = current_frame_time_;
    } else {
      if (FloatsNotEqual(transition_end_height_, result_height)) {
        if (FloatsEqual(transition_end_height_, current_transition_height_)) {
          transition_start_time_ = current_frame_time_;
        } else {
          transition_start_time_ = current_transition_time_;
        }
        transition_end_height_ = result_height;
        transition_start_height_ = current_transition_height_;
      }
      if (current_frame_time_ >=
          transition_start_time_ +
              static_cast<int64_t>(height_transition_duration_ * 1000)) {
        current_transition_height_ = transition_end_height_;
      } else {
        current_transition_height_ =
            transition_start_height_ +
            (transition_end_height_ - transition_start_height_) *
                static_cast<float>(current_frame_time_ -
                                   transition_start_time_) /
                (height_transition_duration_ * 1000);
      }
    }
    result_height = current_transition_height_;
  }
  TraceEventEnd();
  return {result_width, result_height};
}
void MarkdownView::Align(float x, float y) {
  TraceEventBegin("MarkdownView::Align");
  for (auto& inline_view : document_.GetInlineViews()) {
    auto pos = document_.GetElementOrigin(inline_view.char_index_,
                                          inline_view.is_block_view_);
    static_cast<MarkdownViewDelegate*>(inline_view.view_)
        ->GetPlatformView()
        ->Align(pos.x_, pos.y_);
  }
  for (auto& image : document_.GetImages()) {
    auto pos = document_.GetElementOrigin(image.char_index_);
    static_cast<MarkdownViewDelegate*>(image.image_)
        ->GetPlatformView()
        ->Align(pos.x_, pos.y_);
  }
  if (custom_typewriter_cursor_ != nullptr &&
      custom_cursor_position_ != PointF{0, 0}) {
    custom_typewriter_cursor_->Align(custom_cursor_position_.x_,
                                     custom_cursor_position_.y_);
  }
  TraceEventEnd();
}
void MarkdownView::Draw(tttext::ICanvasHelper* canvas, float left, float top,
                        float right, float bottom) {
  TraceEventBegin("MarkdownView::Draw");
  SendDrawStart();
  if (animation_type_ == MarkdownAnimationType::kNone) {
    MarkdownDrawer drawer(canvas);
    auto page = document_.GetPage();
    if (page != nullptr) {
      drawer.DrawPage(*page);
    }
    SendDrawEnd();
  } else if (animation_type_ == MarkdownAnimationType::kTypewriter) {
    MarkdownViewDelegate cursor(custom_typewriter_cursor_,
                                document_.GetMaxWidth(),
                                document_.GetMaxHeight());
    MarkdownCharTypewriterDrawer drawer(
        canvas, current_animation_step_, document_.GetResourceLoader(),
        document_.GetStyle().typewriter_cursor_, false,
        custom_typewriter_cursor_ == nullptr ? nullptr : &cursor);
    auto page = document_.GetPage();
    if (page != nullptr) {
      drawer.DrawPage(*page);
    }
    if (current_animation_step_ >= max_animation_step_) {
      SendDrawEnd();
    }
  }
  TraceEventEnd();
}

float MarkdownView::GetWidth() const {
  return measured_width_;
}

float MarkdownView::GetHeight() const {
  return measured_height_;
}

void MarkdownView::HideAllSubviews() {}

void MarkdownView::NeedsParse() {
  needs_parse_ = true;
  needs_measure_ = true;
  view_->RequestMeasure();
}
void MarkdownView::NeedsMeasure() {
  needs_measure_ = true;
  view_->RequestMeasure();
}
void MarkdownView::NeedsAlign() const {
  view_->RequestAlign();
}
void MarkdownView::NeedsDraw() const {
  view_->RequestDraw();
}
void MarkdownView::OnNextFrame(int64_t timestamp) {
  current_frame_time_ = timestamp;
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
  if (is_in_selection_) {
    auto page = document_.GetPage();
    if (page != nullptr) {
      selection_highlight_rects_ = MarkdownSelection::GetSelectionRectByCharPos(
          page.get(), select_start_index_, select_end_index_);
    }
  }
  UpdateSelectionViews();
  SendSelectionChanged(state);
}

int32_t MarkdownView::GetCharIndexByPosition(PointF position) {
  auto page = document_.GetPage();
  if (!page)
    return -1;
  auto [start, end] = MarkdownSelection::GetCharRangeByPoint(
      page.get(), position, MarkdownSelection::CharRangeType::kChar);
  return start;
}

void MarkdownView::UpdateAnimationStep() {
  if (animation_type_ != MarkdownAnimationType::kTypewriter ||
      animation_velocity_ <= 0 ||
      current_animation_step_ >= max_animation_step_) {
    return;
  }
  TraceEventBegin("MarkdownView::AnimationUpdate");
  int32_t step_count = 0;
  float interval = 1000.0f / animation_velocity_;
  if (current_animation_step_time_ == 0) {
    step_count = 1;
    current_animation_step_time_ = current_frame_time_;
  } else {
    auto duration = current_frame_time_ - current_animation_step_time_;
    step_count = static_cast<int32_t>(static_cast<float>(duration) / interval);
    current_animation_step_time_ +=
        static_cast<int64_t>(static_cast<float>(step_count) * interval);
  }
  if (step_count > 0) {
    current_animation_step_ += step_count;
    if (current_animation_step_ >= max_animation_step_) {
      current_animation_step_ = max_animation_step_;
      current_animation_step_time_ = 0;
    }
    SendAnimationStep(current_animation_step_, max_animation_step_);
    if (typewriter_dynamic_height_) {
      view_->RequestMeasure();
    } else {
      view_->RequestDraw();
    }
  }
  TraceEventEnd();
}

void MarkdownView::UpdateTransitionHeight() const {
  if (height_transition_duration_ > 0 &&
      FloatsNotEqual(transition_end_height_, current_transition_height_)) {
    view_->RequestMeasure();
  }
}

void MarkdownView::SendParseEnd() const {
  if (document_.GetMarkdownEventListener() == nullptr) {
    return;
  }
  document_.GetMarkdownEventListener()->OnParseEnd();
}
void MarkdownView::SendDrawStart() {
  if (!draw_start_sent_) {
    draw_start_sent_ = true;
    if (document_.GetMarkdownEventListener() != nullptr) {
      document_.GetMarkdownEventListener()->OnDrawStart();
    }
  }
}
void MarkdownView::SendDrawEnd() {
  if (!draw_end_sent_) {
    draw_end_sent_ = true;
    if (document_.GetMarkdownEventListener() != nullptr) {
      document_.GetMarkdownEventListener()->OnDrawEnd();
    }
  }
}
void MarkdownView::SendAnimationStep(int32_t animation_step,
                                     int32_t max_animation_step) {
  if (document_.GetMarkdownEventListener() == nullptr) {
    return;
  }
  document_.GetMarkdownEventListener()->OnAnimationStep(animation_step,
                                                        max_animation_step);
}
void MarkdownView::SendImageClicked(const char* url) {
  if (document_.GetMarkdownEventListener() == nullptr) {
    return;
  }
  document_.GetMarkdownEventListener()->OnImageClicked(url);
}
void MarkdownView::SendLinkClicked(const char* url, const char* content) {
  if (document_.GetMarkdownEventListener() == nullptr) {
    return;
  }
  document_.GetMarkdownEventListener()->OnLinkClicked(url, content);
}

void MarkdownView::UpdateExposure() {
  if (exposure_listener_ == nullptr) {
    return;
  }
  TraceEventBegin("MarkdownView::Exposure");
  auto rect_in_screen = handle_->GetViewRectInScreen();
  auto images = document_.GetImageByViewRect(rect_in_screen);
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

  auto links = document_.GetLinksByViewRect(rect_in_screen);
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
  TraceEventEnd();
}

void MarkdownView::ClearForParse() {
  exposure_images_.clear();
  exposure_links_.clear();
}

std::set<MarkdownPlatformView*> MarkdownView::GetInlineViews() {
  std::set<MarkdownPlatformView*> views;
  for (auto& image : document_.GetImages()) {
    views.emplace(
        static_cast<MarkdownViewDelegate*>(image.image_)->GetPlatformView());
  }
  for (auto& inline_view : document_.GetInlineViews()) {
    views.emplace(static_cast<MarkdownViewDelegate*>(inline_view.view_)
                      ->GetPlatformView());
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
  for (auto& image : document_.GetImages()) {
    handle_->RemoveSubView(
        static_cast<MarkdownViewDelegate*>(image.image_)->GetPlatformView());
  }
  for (auto& inline_view : document_.GetInlineViews()) {
    handle_->RemoveSubView(static_cast<MarkdownViewDelegate*>(inline_view.view_)
                               ->GetPlatformView());
  }
}

int32_t MarkdownView::GetCharCount() {
  const auto page = document_.GetPage();
  if (page == nullptr) {
    return 0;
  }
  return MarkdownSelection::GetPageCharCount(page.get());
}

void MarkdownView::SetTextSelection(const Range char_range) {
  select_start_index_ = char_range.start_;
  select_end_index_ = char_range.end_;
  if (char_range.start_ >= 0 && char_range.end_ >= 0) {
    is_in_selection_ = true;
    const auto draw_count = animation_type_ == MarkdownAnimationType::kNone
                                ? GetCharCount()
                                : current_animation_step_;
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
  return document_.GetContentByCharPos(select_start_index_, select_end_index_);
}

const std::vector<RectF>& MarkdownView::GetSelectedLineBoundingRect() {
  return selection_highlight_rects_;
}

Range MarkdownView::GetCharRangeByPosition(
    PointF position, MarkdownSelection::CharRangeType char_range_type) {
  auto page = document_.GetPage();
  if (page == nullptr)
    return {};
  return MarkdownSelection::GetCharRangeByPoint(page.get(), position,
                                                char_range_type);
}

RectF MarkdownView::GetTextBoundingRect(Range range) {
  auto page = document_.GetPage();
  if (page == nullptr)
    return {};
  return MarkdownSelection::GetSelectionClosedRectByCharPos(
      page.get(), range.start_, range.end_);
}

std::string MarkdownView::GetParsedContent(Range char_range) {
  return document_.GetContentByCharPos(char_range.start_, char_range.end_);
}

std::vector<RectF> MarkdownView::GetTextLineBoundingRect(Range range) {
  auto page = document_.GetPage();
  if (page == nullptr)
    return {};
  return MarkdownSelection::GetSelectionRectByCharPos(page.get(), range.start_,
                                                      range.end_);
}
void MarkdownView::SendSelectionChanged(SelectionState state) const {
  if (document_.GetMarkdownEventListener() != nullptr) {
    const auto start = is_in_selection_ ? select_start_index_ : -1;
    const auto end = is_in_selection_ ? select_end_index_ : -1;
    const auto handle = is_adjust_start_pos_
                            ? SelectionHandleType::kLeftHandle
                            : SelectionHandleType::kRightHandle;
    document_.GetMarkdownEventListener()->OnSelectionChanged(start, end, handle,
                                                             state);
  }
}

void MarkdownView::OnTap(PointF position, GestureEventType event) {
  if (event == GestureEventType::kDown) {
    if (is_in_selection_) {
      ExitSelection();
    }
    auto* listener = document_.GetMarkdownEventListener();
    const auto* link = document_.GetLinkByTouchPosition(position);
    if (link != nullptr) {
      listener->OnLinkClicked(link->url_.c_str(), link->content_.c_str());
    }
    const auto image = document_.GetImageByTouchPosition(position);
    if (!image.empty()) {
      listener->OnImageClicked(image.c_str());
    }
  }
}

void MarkdownView::OnLongPress(PointF position, GestureEventType event) {
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
  auto state = MarkdownTouchState::kNone;
  if (event == GestureEventType::kDown) {
    state = document_.OnTouchEvent(MarkdownTouchEventType::kDown, position);
  } else if (event == GestureEventType::kMove) {
    state = document_.OnTouchEvent(MarkdownTouchEventType::kMove, position);
  } else {
    state = document_.OnTouchEvent(MarkdownTouchEventType::kUp, position);
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
  paddings_ = {left, top, right, bottom};
}

void* MarkdownView::LoadFont(const char* family, MarkdownFontWeight weight) {
  if (platform_loader_ == nullptr) {
    return nullptr;
  }
  return platform_loader_->LoadFont(family, weight);
}

std::unique_ptr<tttext::RunDelegate> MarkdownView::LoadImage(
    const char* src, float desire_width, float desire_height, float max_width,
    float max_height, float border_radius) {
  if (platform_loader_ == nullptr) {
    return nullptr;
  }
  auto handle = platform_loader_->LoadImageView(
      src, desire_width, desire_height, max_width, max_height, border_radius);
  if (handle == nullptr) {
    return nullptr;
  }
  return std::make_unique<MarkdownViewDelegate>(handle, max_width, max_height);
}

std::unique_ptr<tttext::RunDelegate> MarkdownView::LoadGradient(
    const char* gradient, float font_size, float root_font_size) {
  return nullptr;
}
std::unique_ptr<tttext::RunDelegate> MarkdownView::LoadInlineView(
    const char* id_selector, float max_width, float max_height) {
  if (platform_loader_ == nullptr) {
    return nullptr;
  }
  auto handle =
      platform_loader_->LoadInlineView(id_selector, max_width, max_height);
  if (handle == nullptr) {
    return nullptr;
  }
  return std::make_unique<MarkdownViewDelegate>(handle, max_width, max_height);
}
std::unique_ptr<tttext::RunDelegate> MarkdownView::LoadReplacementView(
    void* ud, int32_t id, float max_width, float max_height) {
  if (platform_loader_ == nullptr) {
    return nullptr;
  }
  auto handle =
      platform_loader_->LoadReplacementView(ud, id, max_width, max_height);
  if (handle == nullptr) {
    return nullptr;
  }
  return std::make_unique<MarkdownViewDelegate>(handle, max_width, max_height);
}

void MarkdownView::SetNumberProp(MarkdownProps prop, double value) {
  switch (prop) {
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
