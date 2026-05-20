// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/view/markdown_view.h"

#include "markdown/draw/markdown_typewriter_drawer.h"
#include "markdown/layout/markdown_layout.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/markdown_event_listener.h"
#include "markdown/markdown_exposure_listener.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/view/markdown_platform_view.h"
#include "markdown/view/markdown_view_animator.h"
namespace serval::markdown {
namespace {
std::unique_ptr<Value> ConvertEffectToAttachment(ValueMap& effect) {
  const auto color = effect.find("color");
  const auto start_iter = effect.find("rangeStart");
  const auto end_iter = effect.find("rangeEnd");
  const int32_t start =
      start_iter != effect.end() ? start_iter->second->GetInt() : 0;
  const int32_t end = end_iter != effect.end() ? end_iter->second->GetInt() : 0;
  if (color == effect.end() || color->second->GetString().empty() ||
      (start == 0 && end == 0)) {
    return nullptr;
  }

  ValueMap style;
  style.emplace("color", Value::MakeString(color->second->GetString()));

  ValueMap attachment;
  attachment.emplace("startIndex", Value::MakeInt(start));
  attachment.emplace("endIndex", Value::MakeInt(end));
  attachment.emplace("layer", Value::MakeString("foreground"));
  attachment.emplace("style", Value::MakeMap(std::move(style)));

  ValueArray attachments;
  attachments.emplace_back(Value::MakeMap(std::move(attachment)));
  return Value::MakeArray(std::move(attachments));
}
}  // namespace

MarkdownView::MarkdownView(MarkdownPlatformView* view,
                           std::shared_ptr<MarkdownContext> context)
    : view_(view),
      handle_(view->GetViewContainerHandle()),
      context_(std::move(context)),
      measurer_(context_),
      gesture_(handle_, context_.get(), &renderer_) {
  renderer_.SetViewContainerHandle(handle_);
}
MarkdownView::~MarkdownView() {
  gesture_.SetRenderer(nullptr);
}
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
  gesture_.SetEventListener(listener);
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
  NeedsMeasure();
}
void MarkdownView::SetMarkdownEffect(std::unique_ptr<Value> effect) {
  effect_ = std::move(effect);
  NeedsMeasure();
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
  gesture_.SetEnableSelection(enable_selection);
}
void MarkdownView::SetSelectionHandleSize(float size) {
  gesture_.SetSelectionHandleSize(size);
}
void MarkdownView::SetSelectionHandleTouchMargin(float margin) {
  gesture_.SetSelectionHandleTouchMargin(margin);
}
void MarkdownView::SetSelectionHandleColor(uint32_t color) {
  gesture_.SetSelectionHandleColor(color);
}
void MarkdownView::SetSelectionHighlightColor(uint32_t color) {
  gesture_.SetSelectionHighlightColor(color);
}
MeasureResult MarkdownView::OnMeasure(MeasureSpec spec) {
  measurer_.Measure(spec);
  if (measurer_.DidLayoutInLastMeasure()) {
    auto before_views = GetInlineViews();
    layout_data_.document_ = measurer_.GetDocument();
    UpdateTextAttachments();
    draw_end_sent_ = false;
    auto after_views = GetInlineViews();
    RemoveUnusedViews(before_views, after_views);
    animator_.SetMaxAnimationStep(GetCharCount());
    animator_.SetLineExpandLineEndSteps(GetLineExpandAnimationSteps());
  }

  const auto measured = measurer_.GetMeasuredSize();
  float result_width = measured.width_;
  float result_height = measured.height_;
  float transition_target_height = result_height;
  if (layout_data_.document_ != nullptr && IsStepAnimatedHeightEnabled()) {
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
void MarkdownView::MarkDirty() {
  NeedsMeasure();
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
void MarkdownView::UpdateTextAttachments() {
  if (layout_data_.document_ == nullptr) {
    return;
  }
  const auto page = layout_data_.document_->GetPage();
  if (page == nullptr) {
    return;
  }
  page->ClearAttachments();
  if (attachments_ != nullptr) {
    auto attachments = MarkdownStyleReader::ReadTextAttachments(
        attachments_.get(), layout_data_.document_.get());
    page->AddTextAttachments(std::move(attachments));
  }
  if (effect_ != nullptr) {
    auto effect = MarkdownStyleReader::ReadTextAttachments(
        effect_.get(), layout_data_.document_.get());
    page->AddTextAttachments(std::move(effect));
  }
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
int32_t MarkdownView::GetCharIndexByPosition(PointF position) const {
  return gesture_.GetCharIndexByPosition(position);
}

void MarkdownView::UpdateAnimationStep() {
  const auto result = animator_.UpdateAnimationStep();
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
  if (animator_.GetAnimationType() == MarkdownAnimationType::kNone ||
      (IsAnimationComplete() && layout_data_.content_complete_)) {
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
    gesture_.SetDocument(bundle->document_);
    UpdateExposure();
  }
  renderer_.SetMarkdownAnimationType(bundle->animation_type_);
  renderer_.SetMarkdownAnimationStep(bundle->animation_step_);
  renderer_.SetContentComplete(bundle->content_complete_);
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
      layout_data_.document_->GetContextPtr(), nullptr, animation_step,
      layout_data_.document_->GetResourceLoader(), cursor_style,
      !layout_data_.content_complete_,
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

std::vector<int32_t> MarkdownView::GetLineExpandAnimationSteps() const {
  if (layout_data_.document_ == nullptr) {
    return {};
  }
  return layout_data_.document_->GetLineEndCharIndices();
}

bool MarkdownView::IsStepAnimatedHeightEnabled() const {
  const auto animation_type = animator_.GetAnimationType();
  return animator_.GetTypewriterDynamicHeight() &&
         (animation_type == MarkdownAnimationType::kTypewriter ||
          animation_type == MarkdownAnimationType::kLineExpand);
}

bool MarkdownView::IsAnimationComplete() const {
  if (animator_.GetAnimationType() == MarkdownAnimationType::kLineExpand) {
    return animator_.IsLineExpandComplete() ||
           animator_.GetAnimationStep() >= animator_.GetMaxAnimationStep();
  }
  return animator_.GetAnimationStep() >= animator_.GetMaxAnimationStep();
}

void MarkdownView::SetTextSelection(const Range char_range) {
  const auto draw_count =
      animator_.GetAnimationType() == MarkdownAnimationType::kNone
          ? GetCharCount()
          : animator_.GetAnimationStep();
  gesture_.SetTextSelection(char_range, draw_count);
}

void MarkdownView::SetTrimParagraphSpaces(bool trim_spaces) {
  trim_paragraph_spaces_ = trim_spaces;
}

Range MarkdownView::GetSelectedRange() const {
  return gesture_.GetSelectedRange();
}

std::string MarkdownView::GetSelectedText() const {
  return gesture_.GetSelectedText();
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
  return gesture_.GetSelectedLineBoundingRect();
}

PointF MarkdownView::GetSelectionHandlePosition() const {
  return gesture_.GetSelectionHandlePosition();
}

float MarkdownView::GetSelectionHandleRadius() const {
  return gesture_.GetSelectionHandleRadius();
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
  return gesture_.GetCharRangeByPosition(position, char_range_type);
}

RectF MarkdownView::GetTextBoundingRect(Range range) const {
  return gesture_.GetTextBoundingRect(range);
}

std::string MarkdownView::GetParsedContent(Range char_range) const {
  return gesture_.GetParsedContent(char_range);
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
  return gesture_.GetTextLineBoundingRect(range);
}

bool MarkdownView::OnTap(PointF position, GestureEventType event) {
  return gesture_.OnTap(position, event);
}

bool MarkdownView::ShouldBeginPan(PointF position, PointF motion) {
  return gesture_.ShouldBeginPan(position, motion);
}

bool MarkdownView::OnPan(PointF position, PointF motion,
                         GestureEventType event) {
  return gesture_.OnPan(position, motion, event);
}

bool MarkdownView::OnLongPress(PointF position, GestureEventType event) {
  return gesture_.OnLongPress(position, event);
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
    } else if (value == "line-expand") {
      SetAnimationType(MarkdownAnimationType::kLineExpand);
    } else {
      SetAnimationType(MarkdownAnimationType::kNone);
    }
  }
}
void MarkdownView::SetArrayProp(MarkdownProps prop, ValueArray& array) {
  if (prop == MarkdownProps::kTextMarkAttachments) {
    SetTextAttachments(Value::MakeArray(std::move(array)));
  }
}
void MarkdownView::SetMapProp(MarkdownProps prop, ValueMap& map) {
  if (prop == MarkdownProps::kMarkdownEffect) {
    SetMarkdownEffect(ConvertEffectToAttachment(map));
  }
}

void MarkdownView::OnFontLoaded(std::string_view family, int weight,
                                int style) {
  measurer_.NeedsMeasure();
}
void MarkdownView::OnImageLoaded(std::string_view url) {
  measurer_.NeedsMeasure();
}
}  // namespace serval::markdown
