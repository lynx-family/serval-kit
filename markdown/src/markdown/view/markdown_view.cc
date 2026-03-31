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
namespace serval::markdown {
MarkdownView::MarkdownView(MarkdownPlatformView* view)
    : view_(view), handle_(view->GetViewContainerHandle()) {
  renderer_.SetViewContainerHandle(handle_);
  view_->SetTapListener([this](PointF position, GestureEventType event) {
    return OnTap(position, event);
  });
  view_->SetLongPressListener([this](PointF position, GestureEventType event) {
    return OnLongPress(position, event);
  });
  view_->SetPanGestureListener(
      [this](PointF position, PointF motion, GestureEventType event) {
        return OnPan(position, motion, event);
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
  layout_data_.content_complete_ = complete;
  NeedsMeasure();
}
void MarkdownView::SetContentRange(Range range) {
  measurer_.SetContentRange(range);
  NeedsMeasure();
}
void MarkdownView::SetContentRangeStart(int32_t start) {
  SetContentRange({start, measurer_.GetContentEnd()});
}
void MarkdownView::SetContentRangeEnd(int32_t end) {
  SetContentRange({measurer_.GetContentStart(), end});
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
  if (animator_.GetTypewriterDynamicHeight()) {
    view_->RequestMeasure();
  } else {
    view_->RequestDraw();
    PublishRendererBundle();
  }
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
  if (selection_highlight_ != nullptr || handle_ == nullptr) {
    return;
  }
  selection_highlight_ =
      handle_->CreateSelectionHighlightSubView(selection_highlight_color_);
  selection_highlight_->SetVisibility(false);
  selection_handles_.left_ = handle_->CreateSelectionHandleSubView(
      SelectionHandleType::kLeftHandle, selection_handle_size_,
      selection_handle_touch_margin_, selection_handle_color_);
  selection_handles_.left_->Measure(MeasureSpec{});
  selection_handles_.left_->SetPanGestureListener(
      [this](PointF position, PointF motion, GestureEventType event) {
        return OnStartHandleMove(position, motion, event);
      });
  selection_handles_.left_->SetVisibility(false);
  selection_handles_.right_ = handle_->CreateSelectionHandleSubView(
      SelectionHandleType::kRightHandle, selection_handle_size_,
      selection_handle_touch_margin_, selection_handle_color_);
  selection_handles_.right_->Measure(MeasureSpec{});
  selection_handles_.right_->SetPanGestureListener(
      [this](PointF position, PointF motion, GestureEventType event) {
        return OnEndHandleMove(position, motion, event);
      });
  selection_handles_.right_->SetVisibility(false);
}
MeasureResult MarkdownView::OnMeasure(MeasureSpec spec) {
  measurer_.Measure(spec);
  if (measurer_.DidLayoutInLastMeasure()) {
    auto before_views = GetInlineViews();
    layout_data_.document_ = measurer_.GetDocument();
    draw_end_sent_ = false;
    auto after_views = GetInlineViews();
    RemoveUnusedViews(before_views, after_views);
    animator_.SetMaxAnimationStep(GetCharCount());
  }

  const auto measured = measurer_.GetMeasuredSize();
  float result_width = measured.width_;
  float result_height = measured.height_;
  float transition_target_height = result_height;
  if (layout_data_.document_ != nullptr &&
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
  PublishRendererBundle();
  return {.width_ = result_width,
          .height_ = result_height,
          .baseline_ = result_height};
}
void MarkdownView::Align(float x, float y) {
  measurer_.Align();
  if (layout_data_.document_ == nullptr ||
      layout_data_.custom_cursor_position_ == PointF{0, 0}) {
    return;
  }
  const auto page = layout_data_.document_->GetPage();
  if (page == nullptr) {
    return;
  }
  const auto cursor = page->GetCustomTypewriterCursor();
  if (cursor != nullptr) {
    cursor->Align(layout_data_.custom_cursor_position_.x_,
                  layout_data_.custom_cursor_position_.y_);
  }
}
void MarkdownView::Draw(tttext::ICanvasHelper* canvas, float x, float y) {
  ConsumeRendererBundleIfNeeded();
  renderer_.Draw(canvas, x, y);
}
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
void MarkdownView::OnLayoutFrame(int64_t timestamp) {
  animator_.UpdateCurrentTime(timestamp);
  UpdateAnimationStep();
  UpdateTransitionHeight();
  UpdateDrawEventsByAnimation();
}
void MarkdownView::OnRendererFrame(int64_t /*timestamp*/) {
  ConsumeRendererBundleIfNeeded();
  UpdateExposure();
  renderer_.OnNextFrame();
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
      ->UpdateViewRect(selection_highlight_.get());
  selection_highlight_->RequestMeasure();

  const auto rect_start = selection_highlight_rects_.front();
  auto* handle_start = GetSelectionHandle(selection_handles_.left_);
  handle_start->SetTextHeight(rect_start.GetHeight());
  handle_start->UpdateViewRect({rect_start.GetLeft(), rect_start.GetBottom()},
                               selection_handles_.left_.get());
  selection_highlight_->RequestMeasure();

  const auto& rect_end = selection_highlight_rects_.back();
  auto* handle_end = GetSelectionHandle(selection_handles_.right_);
  handle_end->SetTextHeight(selection_highlight_rects_.back().GetHeight());
  handle_end->UpdateViewRect({rect_end.GetRight(), rect_end.GetBottom()},
                             selection_handles_.right_.get());
  selection_highlight_->RequestMeasure();
}

void MarkdownView::UpdateSelectionRects(SelectionState state) {
  selection_highlight_rects_.clear();
  if (is_in_selection_ && renderer_data_.document_ != nullptr) {
    auto page = renderer_data_.document_->GetPage();
    if (page != nullptr) {
      selection_highlight_rects_ = MarkdownSelection::GetSelectionRectByCharPos(
          page.get(), select_start_index_, select_end_index_);
    }
  }
  UpdateSelectionViews();
  SendSelectionChanged(state);
}

int32_t MarkdownView::GetCharIndexByPosition(PointF position) const {
  if (renderer_data_.document_ == nullptr) {
    return -1;
  }
  auto page = renderer_data_.document_->GetPage();
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
      PublishRendererBundle();
    }
  }
}

void MarkdownView::UpdateTransitionHeight() const {
  if (animator_.IsHeightTransitionRunning()) {
    view_->RequestMeasure();
  }
}

void MarkdownView::UpdateDrawEventsByAnimation() {
  SendDrawStart();
  const int32_t animation_step = animator_.GetAnimationStep();
  const int32_t max_animation_step = animator_.GetMaxAnimationStep();
  if (animator_.GetAnimationType() != MarkdownAnimationType::kTypewriter ||
      (animation_step >= max_animation_step &&
       layout_data_.content_complete_)) {
    SendDrawEnd();
  }
}

void MarkdownView::PublishRendererBundle() {
  auto bundle = std::make_unique<RendererBundle>();
  bundle->document_ = layout_data_.document_;
  bundle->animation_type_ = animator_.GetAnimationType();
  bundle->animation_step_ = animator_.GetAnimationStep();
  bundle->content_complete_ = layout_data_.content_complete_;
  std::lock_guard<std::mutex> guard(renderer_bundle_mutex_);
  renderer_bundle_ = std::move(bundle);
}

void MarkdownView::ConsumeRendererBundleIfNeeded() {
  std::unique_ptr<RendererBundle> bundle;
  {
    std::lock_guard<std::mutex> guard(renderer_bundle_mutex_);
    if (renderer_bundle_ == nullptr) {
      return;
    }
    bundle = std::move(renderer_bundle_);
  }
  if (renderer_data_.document_.get() != bundle->document_.get()) {
    renderer_.SetDocument(bundle->document_);
    renderer_data_.document_ = bundle->document_;
    UpdateExposure();
    if (is_in_selection_) {
      UpdateSelectionRects(SelectionState::kMove);
    }
  }
  renderer_.SetMarkdownAnimationType(bundle->animation_type_);
  renderer_.SetMarkdownAnimationStep(bundle->animation_step_);
  renderer_.SetContentComplete(bundle->content_complete_);
}

void MarkdownView::SendImageClicked(const char* url) const {
  if (event_listener_ == nullptr) {
    return;
  }
  event_listener_->OnImageClicked(url);
}
void MarkdownView::SendLinkClicked(const char* url, const char* content) const {
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
  if (renderer_data_.document_ == nullptr) {
    return;
  }
  auto rect_in_screen = handle_->GetViewRectInScreen();
  auto images = renderer_data_.document_->GetImageByViewRect(rect_in_screen);
  std::unordered_set<ExposureKey, ExposureKey::Hash> current_images;
  for (auto* image : images) {
    if (image == nullptr) {
      continue;
    }
    ExposureKey key;
    key.url_ = image->url_;
    key.char_index_ = image->char_index_;
    current_images.emplace(key);
    if (renderer_data_.exposure_images_.find(key) ==
        renderer_data_.exposure_images_.end()) {
      exposure_listener_->OnImageAppear(key.url_.c_str());
    }
  }
  for (const auto& image_key : renderer_data_.exposure_images_) {
    if (current_images.find(image_key) == current_images.end()) {
      exposure_listener_->OnImageDisappear(image_key.url_.c_str());
    }
  }
  renderer_data_.exposure_images_ = std::move(current_images);

  auto links = renderer_data_.document_->GetLinksByViewRect(rect_in_screen);
  std::unordered_set<ExposureKey, ExposureKey::Hash> current_links;
  for (auto* item : links) {
    if (item == nullptr) {
      continue;
    }
    ExposureKey key;
    key.url_ = item->url_;
    key.char_index_ = static_cast<int32_t>(item->char_start_);
    key.content_ = item->content_;
    current_links.emplace(key);
    if (renderer_data_.exposure_links_.find(key) ==
        renderer_data_.exposure_links_.end()) {
      exposure_listener_->OnLinkAppear(key.url_.c_str(), key.content_.c_str());
    }
  }
  for (const auto& link_key : renderer_data_.exposure_links_) {
    if (current_links.find(link_key) == current_links.end()) {
      exposure_listener_->OnLinkDisappear(link_key.url_.c_str(),
                                          link_key.content_.c_str());
    }
  }
  renderer_data_.exposure_links_ = std::move(current_links);
}
std::set<MarkdownPlatformView*> MarkdownView::GetInlineViews() const {
  if (layout_data_.document_ == nullptr) {
    return {};
  }
  std::set<MarkdownPlatformView*> views;
  for (auto& image : layout_data_.document_->GetImages()) {
    if (image.image_ != nullptr) {
      views.emplace(static_cast<MarkdownPlatformView*>(image.image_));
    }
  }
  for (auto& inline_view : layout_data_.document_->GetInlineViews()) {
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

int32_t MarkdownView::GetCharCount() const {
  if (layout_data_.document_ == nullptr) {
    return 0;
  }
  const auto page = layout_data_.document_->GetPage();
  if (page == nullptr) {
    return 0;
  }
  return MarkdownSelection::GetPageCharCount(page.get());
}

float MarkdownView::CalculateHeightByAnimationStep() {
  if (layout_data_.document_ == nullptr) {
    return measurer_.GetMeasuredSize().height_;
  }
  return CalculateHeightByAnimationStep(animator_.GetAnimationStep(), true);
}

float MarkdownView::CalculateHeightByAnimationStep(
    int32_t animation_step, bool update_cursor_position) {
  if (layout_data_.document_ == nullptr) {
    return measurer_.GetMeasuredSize().height_;
  }
  auto& cursor_style = layout_data_.document_->GetStyle().typewriter_cursor_;
  auto page = layout_data_.document_->GetPage();
  if (page == nullptr) {
    if (update_cursor_position) {
      layout_data_.custom_cursor_position_ = {0, 0};
    }
    return measurer_.GetMeasuredSize().height_;
  }
  const auto cursor = page->GetCustomTypewriterCursor();
  MarkdownCharTypewriterDrawer drawer(
      nullptr, animation_step, layout_data_.document_->GetResourceLoader(),
      cursor_style, !layout_data_.content_complete_,
      cursor == nullptr ? nullptr : cursor.get());
  drawer.CalculateCursorPosition(page.get());
  float typewriter_height = 0;
  if (animation_step < animator_.GetMaxAnimationStep()) {
    typewriter_height = drawer.GetMaxDrawHeight();
  } else {
    typewriter_height = measurer_.GetMeasuredSize().height_;
  }
  if (update_cursor_position) {
    layout_data_.custom_cursor_position_ = drawer.GetCursorPosition();
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

std::string MarkdownView::GetSelectedText() const {
  if (renderer_data_.document_ == nullptr) {
    return {};
  }
  return renderer_data_.document_->GetContentByCharPos(select_start_index_,
                                                       select_end_index_);
}

std::string MarkdownView::GetContent() const {
  if (renderer_data_.document_ == nullptr) {
    return {};
  }
  return renderer_data_.document_->GetMarkdownContent();
}

std::string MarkdownView::GetContentID() const {
  return measurer_.GetContentID();
}

const std::vector<RectF>& MarkdownView::GetSelectedLineBoundingRect() {
  return selection_highlight_rects_;
}

PointF MarkdownView::GetSelectionHandlePosition() const {
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

float MarkdownView::GetSelectionHandleRadius() const {
  return selection_handle_size_ * 0.5f;
}

std::vector<std::string> MarkdownView::GetAllImageUrl() const {
  if (renderer_data_.document_ == nullptr) {
    return {};
  }
  return renderer_data_.document_->GetAllImageUrl();
}

std::vector<std::string> MarkdownView::GetLinkUrl() const {
  if (renderer_data_.document_ == nullptr) {
    return {};
  }
  const auto& links = renderer_data_.document_->GetLinks();
  std::vector<std::string> urls;
  urls.reserve(links.size());
  for (const auto& link : links) {
    urls.emplace_back(link.url_);
  }
  return urls;
}

std::vector<std::string> MarkdownView::GetLinkContent() const {
  if (renderer_data_.document_ == nullptr) {
    return {};
  }
  const auto& links = renderer_data_.document_->GetLinks();
  std::vector<std::string> contents;
  contents.reserve(links.size());
  for (const auto& link : links) {
    contents.emplace_back(link.content_);
  }
  return contents;
}

std::vector<RectF> MarkdownView::GetLinkBoundingRect() const {
  if (renderer_data_.document_ == nullptr) {
    return {};
  }
  const auto& links = renderer_data_.document_->GetLinks();
  std::vector<RectF> rects;
  rects.reserve(links.size());
  for (const auto& link : links) {
    const auto start = static_cast<int32_t>(link.char_start_);
    const auto end = static_cast<int32_t>(link.char_start_ + link.char_count_);
    rects.emplace_back(GetTextBoundingRect({start, end}));
  }
  return rects;
}

std::vector<Range> MarkdownView::GetSyntaxSourceRanges(
    std::string_view tag) const {
  if (renderer_data_.document_ == nullptr) {
    return {};
  }
  return renderer_data_.document_->GetSyntaxSourceRanges(tag);
}

Range MarkdownView::GetCharRangeByPosition(
    PointF position, MarkdownSelection::CharRangeType char_range_type) const {
  if (renderer_data_.document_ == nullptr) {
    return {};
  }
  auto page = renderer_data_.document_->GetPage();
  if (page == nullptr)
    return {};
  return MarkdownSelection::GetCharRangeByPoint(page.get(), position,
                                                char_range_type);
}

RectF MarkdownView::GetTextBoundingRect(Range range) const {
  if (renderer_data_.document_ == nullptr) {
    return {};
  }
  auto page = renderer_data_.document_->GetPage();
  if (page == nullptr)
    return {};
  return MarkdownSelection::GetSelectionClosedRectByCharPos(
      page.get(), range.start_, range.end_);
}

std::string MarkdownView::GetParsedContent(Range char_range) const {
  if (renderer_data_.document_ == nullptr) {
    return {};
  }
  return renderer_data_.document_->GetContentByCharPos(char_range.start_,
                                                       char_range.end_);
}

int32_t MarkdownView::CharOffsetToSourceOffset(int32_t char_offset) const {
  if (renderer_data_.document_ == nullptr) {
    return 0;
  }
  return renderer_data_.document_->CharOffsetToMarkdownOffset(char_offset);
}

int32_t MarkdownView::SourceOffsetToCharOffset(int32_t source_offset) const {
  if (renderer_data_.document_ == nullptr) {
    return 0;
  }
  return renderer_data_.document_->MarkdownOffsetToCharOffset(source_offset);
}

std::vector<RectF> MarkdownView::GetTextLineBoundingRect(Range range) const {
  if (renderer_data_.document_ == nullptr) {
    return {};
  }
  auto page = renderer_data_.document_->GetPage();
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

bool MarkdownView::OnTap(PointF position, GestureEventType event) {
  if (event == GestureEventType::kDown) {
    if (is_in_selection_) {
      ExitSelection();
      return true;
    }
    if (renderer_data_.document_ == nullptr || event_listener_ == nullptr) {
      return false;
    }
    const auto* link =
        renderer_data_.document_->GetLinkByTouchPosition(position);
    if (link != nullptr) {
      event_listener_->OnLinkClicked(link->url_.c_str(),
                                     link->content_.c_str());
      return true;
    }
    const auto image =
        renderer_data_.document_->GetImageByTouchPosition(position);
    if (!image.empty()) {
      event_listener_->OnImageClicked(image.c_str());
      return true;
    }
  }
  return false;
}

bool MarkdownView::OnLongPress(PointF position, GestureEventType event) {
  if (renderer_data_.document_ == nullptr) {
    return false;
  }
  if (event == GestureEventType::kDown && enable_selection_ &&
      !is_in_selection_) {
    EnterSelection(position);
    return true;
  } else if (is_in_selection_) {
    SendSelectionChanged(SelectionState::kStop);
    RecalculateSelectionPosition();
    return true;
  }
  return false;
}

bool MarkdownView::OnPan(PointF position, PointF motion,
                         GestureEventType event) const {
  if (renderer_data_.document_ == nullptr) {
    return false;
  }
  auto state = MarkdownTouchState::kNone;
  if (event == GestureEventType::kDown) {
    state = renderer_data_.document_->OnTouchEvent(
        MarkdownTouchEventType::kDown, position);
  } else if (event == GestureEventType::kMove) {
    state = renderer_data_.document_->OnTouchEvent(
        MarkdownTouchEventType::kMove, position);
  } else {
    state = renderer_data_.document_->OnTouchEvent(MarkdownTouchEventType::kUp,
                                                   position);
  }
  if (state == MarkdownTouchState::kOnScroll) {
    NeedsAlign();
    return true;
  }
  return false;
}

bool MarkdownView::OnStartHandleMove(PointF position, PointF motion,
                                     GestureEventType type) {
  if (!is_in_selection_) {
    return false;
  }
  if (!is_adjust_start_pos_ && !is_adjust_end_pos_) {
    is_adjust_start_pos_ = true;
  }
  return OnHandleMove(position, motion, type);
}

bool MarkdownView::OnEndHandleMove(PointF position, PointF motion,
                                   GestureEventType type) {
  if (!is_in_selection_) {
    return false;
  }
  if (!is_adjust_start_pos_ && !is_adjust_end_pos_) {
    is_adjust_end_pos_ = true;
  }
  return OnHandleMove(position, motion, type);
}

bool MarkdownView::OnHandleMove(PointF position, PointF motion,
                                GestureEventType type) {
  if (!is_in_selection_) {
    return false;
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
  if (type != GestureEventType::kMove) {
    RecalculateSelectionPosition();
    SendSelectionChanged(SelectionState::kStop);
  }
  return true;
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
      SetContentRangeStart(static_cast<int32_t>(value));
      break;
    case MarkdownProps::kContentRangeEnd:
      SetContentRangeEnd(static_cast<int32_t>(value));
      break;
    case MarkdownProps::kTypewriterHeightTransitionDuration:
      SetHeightTransitionDuration(static_cast<float>(value));
      break;
    case MarkdownProps::kTypewriterHeightTransitionPrefetch:
      SetTypewriterHeightTransitionPrefetch(static_cast<bool>(value));
      break;
    case MarkdownProps::kAllowBreakAroundPunctuation:
      SetEnableBreakAroundPunctuation(static_cast<bool>(value));
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
}  // namespace serval::markdown
