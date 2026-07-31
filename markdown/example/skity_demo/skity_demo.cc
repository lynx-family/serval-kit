// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "skity_demo/skity_demo.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

#include "markdown/element/markdown_context.h"
#include "markdown/view/markdown_view.h"
#include "skity_demo/skity_demo_resource_loader.h"
#include "skity_demo/skity_markdown_canvas.h"
#include "skity_demo/skity_markdown_platform.h"
#include "testing/markdown/frame_driven_tests/markdown_case_builder.h"

namespace serval::markdown::example {
namespace {

using MarkdownAttributes = serval::markdown::testing::MarkdownAttributes;
using MarkdownCaseBuilder = serval::markdown::testing::MarkdownCaseBuilder;
using MarkdownCaseEntry = serval::markdown::testing::MarkdownCaseEntry;

constexpr float kScrollStep = 48.f;
constexpr float kGesturePanSlop = 4.f;
constexpr float kWindowHorizontalInset = 32.f;
constexpr float kDocumentTopGap = 18.f;
constexpr float kDocumentBottomGap = 18.f;
constexpr float kDocumentFramePadding = 14.f;
constexpr int64_t kLongPressDelayMs = 450;

struct SkityDemoCases {
  std::vector<MarkdownCaseEntry> cases;
  size_t default_case_index{0};
};

int64_t NowMilliseconds() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

serval::markdown::testing::MarkdownCaseValuePtr CloneOptionalValue(
    Value* value) {
  return value == nullptr ? nullptr : MarkdownCaseBuilder::CloneValue(value);
}

MarkdownAttributes CloneAttributes(const MarkdownAttributes& source) {
  MarkdownAttributes attributes;
  attributes.generate_ground_truth = source.generate_ground_truth;
  attributes.ground_truth = CloneOptionalValue(source.ground_truth.get());
  attributes.width = source.width;
  attributes.height = source.height;
  attributes.visible_rect = source.visible_rect;
  attributes.markdown = source.markdown;
  attributes.style = source.style == nullptr
                         ? Value::MakeMap()
                         : MarkdownCaseBuilder::CloneValue(source.style.get());
  attributes.animation_type = source.animation_type;
  attributes.animation_velocity = source.animation_velocity;
  attributes.initial_animation_step = source.initial_animation_step;
  attributes.content_complete = source.content_complete;
  attributes.max_lines = source.max_lines;
  attributes.attachments = CloneOptionalValue(source.attachments.get());
  attributes.effects = CloneOptionalValue(source.effects.get());
  attributes.source_type = source.source_type;
  attributes.content_range = source.content_range;
  attributes.paddings = source.paddings;
  attributes.allow_break_around_punctuation =
      source.allow_break_around_punctuation;
  attributes.typewriter_dynamic_height = source.typewriter_dynamic_height;
  attributes.typewriter_height_transition_duration =
      source.typewriter_height_transition_duration;
  attributes.typewriter_height_transition_prefetch =
      source.typewriter_height_transition_prefetch;
  attributes.enable_selection = source.enable_selection;
  return attributes;
}

SkityDemoCases LoadSkityDemoCases(const std::filesystem::path& cases_root) {
  SkityDemoCases result;
  result.cases = MarkdownCaseBuilder::LoadCases(cases_root);
  const auto default_case = std::find_if(
      result.cases.begin(), result.cases.end(),
      [](const auto& item) { return item.name == "demo_all_features"; });
  if (default_case != result.cases.end()) {
    result.default_case_index =
        static_cast<size_t>(std::distance(result.cases.begin(), default_case));
  }
  std::fprintf(stderr, "loaded %zu markdown demo cases from %s\n",
               result.cases.size(), cases_root.string().c_str());
  return result;
}

skity::Rect ToSkityRect(RectF rect) {
  return skity::Rect::MakeLTRB(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                               rect.GetBottom());
}

skity::Paint MakePaint(uint32_t color,
                       skity::Paint::Style style = skity::Paint::kFill_Style) {
  skity::Paint paint;
  paint.SetAntiAlias(true);
  paint.SetStyle(style);
  paint.SetColor(color);
  return paint;
}

}  // namespace

SkityDemo::SkityDemo(SkityDemoConfig config)
    : config_(std::move(config)),
      screen_width_(config_.initial_width),
      screen_height_(config_.initial_height) {}

SkityDemo::~SkityDemo() {
  Stop();
}

void SkityDemo::Start() {
  if (markdown_view_ != nullptr) {
    return;
  }
  auto platform = std::make_unique<SkityMarkdownPlatform>(config_.font_root);
  auto* font_manager = platform->GetFontManager();
  resource_loader_ = std::make_unique<SkityDemoResourceLoader>(font_manager);
  auto context = std::make_shared<MarkdownContext>(std::move(platform));
  root_view_.SetMarkdownContext(context.get());

  markdown_view_ =
      std::make_shared<MarkdownView>(&root_view_, &root_view_, context);
  root_view_.AttachDrawable(markdown_view_);
  markdown_view_->SetResourceLoader(resource_loader_.get());
  markdown_view_->SetEventListener(this);
  markdown_view_->SetTypewriterDynamicHeight(false);
  markdown_view_->SetPaddings(0, 0, 0, 0);
  LoadCases();
  if (!cases_.empty()) {
    ApplyCase(case_index_);
    return;
  }
  SetAnimationEnabled(false);
  SetPendingWindowTitle(config_.window_title);
}

void SkityDemo::Stop() {
  ResetPointerGesture();
  root_view_.AttachDrawable(nullptr);
  root_view_.RemoveAllSubViews();
  markdown_view_.reset();
  resource_loader_.reset();
}

void SkityDemo::Resize(int32_t width, int32_t height) {
  width = std::max(1, width);
  height = std::max(1, height);
  if (screen_width_ == width && screen_height_ == height) {
    return;
  }
  screen_width_ = width;
  screen_height_ = height;
  root_view_.RequestMeasure();
}

void SkityDemo::Render(skity::Canvas* canvas, int64_t now_ms) {
  if (canvas == nullptr || markdown_view_ == nullptr) {
    return;
  }
  HandlePendingLongPress(now_ms);

  if (IsAnimationEnabled()) {
    markdown_view_->OnLayoutFrame(now_ms);
  }
  markdown_view_->OnRendererFrame(now_ms);

  MeasureAndAlignIfNeeded();
  SyncViewportState();

  SkityMarkdownCanvas markdown_canvas(canvas);
  const auto content_left = ContentLeft();
  const auto content_top = ContentTop();
  const auto frame = RectF::MakeLTRB(
      content_left - kDocumentFramePadding, content_top - kDocumentFramePadding,
      content_left + LayoutWidth() + kDocumentFramePadding,
      content_top + ViewportHeight() + kDocumentFramePadding);
  canvas->DrawRoundRect(ToSkityRect(frame), 8.f, 8.f, MakePaint(0xffffffff));
  auto border = MakePaint(0xffdbe4ef, skity::Paint::kStroke_Style);
  border.SetStrokeWidth(1.f);
  canvas->DrawRoundRect(ToSkityRect(frame), 8.f, 8.f, border);

  markdown_canvas.Save();
  markdown_canvas.ClipRect(content_left, content_top,
                           content_left + LayoutWidth(),
                           content_top + ViewportHeight(), true);
  markdown_canvas.Translate(content_left, content_top - scroll_y_);
  root_view_.Draw(&markdown_canvas, 0, 0);
  markdown_canvas.Restore();
}

bool SkityDemo::HandleKey(SkityDemoKey key, SkityDemoKeyAction action) {
  if (action != SkityDemoKeyAction::kRelease) {
    return false;
  }
  switch (key) {
    case SkityDemoKey::kSpace:
      SetAnimationEnabled(!IsAnimationEnabled());
      return true;
    case SkityDemoKey::kR: {
      if (markdown_view_ != nullptr) {
        markdown_view_->SetAnimationStep(0);
      }
      SetAnimationEnabled(true);
      return true;
    }
    case SkityDemoKey::kN:
    case SkityDemoKey::kRight:
      SelectCase(1);
      return true;
    case SkityDemoKey::kP:
    case SkityDemoKey::kLeft:
      SelectCase(-1);
      return true;
    case SkityDemoKey::kDown:
      ScrollBy(kScrollStep);
      return true;
    case SkityDemoKey::kUp:
      ScrollBy(-kScrollStep);
      return true;
    case SkityDemoKey::kPageDown:
      ScrollBy(ViewportHeight() * 0.85f);
      return true;
    case SkityDemoKey::kPageUp:
      ScrollBy(-ViewportHeight() * 0.85f);
      return true;
    case SkityDemoKey::kHome:
      ScrollTo(0.f);
      return true;
    case SkityDemoKey::kEnd:
      ScrollTo(content_height_);
      return true;
    case SkityDemoKey::kUnknown:
      return false;
  }
  return false;
}

void SkityDemo::HandleScroll(double offset_x, double offset_y) {
  (void)offset_x;
  ScrollBy(-static_cast<float>(offset_y) * kScrollStep);
}

void SkityDemo::HandlePointer(SkityDemoPointerAction action,
                              SkityDemoPointerButton button, float x, float y) {
  switch (action) {
    case SkityDemoPointerAction::kDown:
      if (button == SkityDemoPointerButton::kPrimary) {
        BeginPointerGesture(x, y, NowMilliseconds());
      }
      break;
    case SkityDemoPointerAction::kUp:
      if (button == SkityDemoPointerButton::kPrimary) {
        EndPointerGesture(x, y);
      }
      break;
    case SkityDemoPointerAction::kMove:
      MovePointerGesture(x, y);
      break;
  }
}

const SkityDemoConfig& SkityDemo::GetConfig() const {
  return config_;
}

std::optional<std::string> SkityDemo::TakePendingWindowTitle() {
  auto title = std::move(pending_window_title_);
  pending_window_title_.reset();
  return title;
}

void SkityDemo::OnParseEnd() {}

void SkityDemo::OnTextOverflow(MarkdownTextOverflow overflow) {
  (void)overflow;
}

void SkityDemo::OnDrawStart() {}

void SkityDemo::OnDrawEnd() {}

void SkityDemo::OnAnimationStep(int32_t animation_step,
                                int32_t max_animation_step) {
  (void)animation_step;
  (void)max_animation_step;
}

void SkityDemo::OnLinkClicked(const char* url, const char* content) {
  (void)url;
  (void)content;
}

void SkityDemo::OnImageClicked(const char* url) {
  (void)url;
}

void SkityDemo::OnSelectionChanged(int32_t start_index, int32_t end_index,
                                   SelectionHandleType handle,
                                   SelectionState state) {
  (void)start_index;
  (void)end_index;
  (void)handle;
  (void)state;
}

PointF SkityDemo::WindowPointToContent(float x, float y) const {
  return {x - ContentLeft(), y - ContentTop() + scroll_y_};
}

bool SkityDemo::IsWindowPointInContent(float x, float y) const {
  const auto left = ContentLeft();
  const auto top = ContentTop();
  return x >= left && x <= left + LayoutWidth() && y >= top &&
         y <= top + ViewportHeight();
}

void SkityDemo::BeginPointerGesture(float x, float y, int64_t timestamp_ms) {
  ResetPointerGesture();
  pointer_down_ = true;
  pointer_down_time_ms_ = timestamp_ms;
  pointer_down_window_ = {x, y};
  last_cursor_window_ = pointer_down_window_;
  pointer_down_scroll_y_ = scroll_y_;
  if (!IsWindowPointInContent(x, y)) {
    return;
  }
  content_gesture_active_ = true;
  pointer_down_content_ = WindowPointToContent(x, y);
}

void SkityDemo::EndPointerGesture(float x, float y) {
  if (content_gesture_active_) {
    const auto content_position = WindowPointToContent(x, y);
    const auto motion = content_position - pointer_down_content_;
    if (markdown_pan_active_) {
      if (markdown_view_ != nullptr) {
        markdown_view_->OnPan(content_position, motion, GestureEventType::kUp);
      }
    } else if (!long_press_sent_ && !drag_scroll_active_ &&
               motion.LengthToZero() <= kGesturePanSlop) {
      if (markdown_view_ != nullptr) {
        markdown_view_->OnTap(content_position, GestureEventType::kDown);
      }
    }
    ResetPointerGesture();
  }
}

void SkityDemo::MovePointerGesture(float x, float y) {
  last_cursor_window_ = {x, y};
  if (!pointer_down_ || !content_gesture_active_) {
    return;
  }
  const auto content_position = WindowPointToContent(x, y);
  const auto motion = content_position - pointer_down_content_;
  const auto window_motion = PointF{x, y} - pointer_down_window_;
  if (markdown_pan_active_) {
    if (markdown_view_ != nullptr) {
      markdown_view_->OnPan(content_position, motion, GestureEventType::kMove);
    }
    return;
  }
  if (drag_scroll_active_) {
    ScrollTo(pointer_down_scroll_y_ - window_motion.y_);
    return;
  }
  if (motion.LengthToZero() <= kGesturePanSlop) {
    return;
  }
  {
    if (markdown_view_ != nullptr &&
        markdown_view_->ShouldBeginPan(content_position, motion)) {
      markdown_pan_active_ = markdown_view_->OnPan(content_position, motion,
                                                   GestureEventType::kDown);
    }
    if (markdown_view_ != nullptr && markdown_pan_active_) {
      markdown_view_->OnPan(content_position, motion, GestureEventType::kMove);
    }
  }
  if (markdown_pan_active_) {
    return;
  }
  if (std::abs(motion.y_) > std::abs(motion.x_)) {
    drag_scroll_active_ = true;
    ScrollTo(pointer_down_scroll_y_ - window_motion.y_);
  }
}

void SkityDemo::ResetPointerGesture() {
  pointer_down_ = false;
  content_gesture_active_ = false;
  long_press_sent_ = false;
  markdown_pan_active_ = false;
  drag_scroll_active_ = false;
  pointer_down_time_ms_ = 0;
}

void SkityDemo::HandlePendingLongPress(int64_t now) {
  if (!pointer_down_ || !content_gesture_active_ || long_press_sent_ ||
      markdown_pan_active_ || drag_scroll_active_) {
    return;
  }
  if (now - pointer_down_time_ms_ < kLongPressDelayMs) {
    return;
  }
  const auto current_content =
      WindowPointToContent(last_cursor_window_.x_, last_cursor_window_.y_);
  const auto motion = current_content - pointer_down_content_;
  if (motion.LengthToZero() > kGesturePanSlop) {
    return;
  }
  if (markdown_view_ != nullptr) {
    long_press_sent_ = markdown_view_->OnLongPress(pointer_down_content_,
                                                   GestureEventType::kDown);
  }
}

void SkityDemo::LoadCases() {
  auto loaded_cases = LoadSkityDemoCases(config_.cases_root);
  cases_ = std::move(loaded_cases.cases);
  case_index_ = loaded_cases.default_case_index;
}

void SkityDemo::ApplyCase(size_t index) {
  if (cases_.empty()) {
    return;
  }
  case_index_ = index % cases_.size();
  const auto& demo_case = cases_[case_index_];

  auto attributes = CloneAttributes(demo_case.attributes);

  content_width_ = std::max(1.f, attributes.width);
  viewport_height_ = std::max(1.f, attributes.height);
  if (markdown_view_ == nullptr) {
    return;
  }
  MarkdownCaseBuilder::ApplyAttributes(attributes, markdown_view_.get());
  markdown_view_->SetContentRange(attributes.content_range);
  markdown_view_->SetInitialAnimationStep(attributes.initial_animation_step);
  markdown_view_->SetTextSelection({-1, -1});
  SetAnimationEnabled(attributes.animation_type !=
                      MarkdownAnimationType::kNone);
  scroll_y_ = InitialScrollY(attributes);
  content_height_ = 0.f;
  root_view_.RequestMeasure();

  const auto title = config_.window_title + " - " + demo_case.name + " (" +
                     std::to_string(case_index_ + 1) + "/" +
                     std::to_string(cases_.size()) + ")";
  SetPendingWindowTitle(title);
  std::fprintf(stderr, "markdown demo case: %s\n", demo_case.name.c_str());
}

void SkityDemo::SelectCase(int direction) {
  if (cases_.empty()) {
    return;
  }
  const auto size = static_cast<int>(cases_.size());
  const auto next = (static_cast<int>(case_index_) + direction + size) % size;
  ApplyCase(static_cast<size_t>(next));
}

bool SkityDemo::IsAnimationEnabled() const {
  return animation_enabled_;
}

void SkityDemo::SetAnimationEnabled(bool enabled) {
  if (enabled && !animation_enabled_ && markdown_view_ != nullptr) {
    markdown_view_->SetAnimationStep(markdown_view_->GetAnimationStep());
  }
  animation_enabled_ = enabled;
}

float SkityDemo::InitialScrollY(const MarkdownAttributes& attributes) const {
  return std::max(0.f, attributes.visible_rect.GetTop());
}

void SkityDemo::MeasureAndAlignIfNeeded() {
  if (markdown_view_ == nullptr) {
    return;
  }
  if (root_view_.TakeNeedsMeasure()) {
    const auto layout_width = LayoutWidth();
    const auto result =
        markdown_view_->Measure({.width_ = layout_width,
                                 .width_mode_ = tttext::LayoutMode::kDefinite,
                                 .height_ = kSkityDemoHeight,
                                 .height_mode_ = tttext::LayoutMode::kAtMost});
    content_height_ = std::max(result.height_, ViewportHeight());
    root_view_.SetMeasuredSize({layout_width, content_height_});
    ClampScrollY();
    root_view_.Align(0, 0);
  }
}

void SkityDemo::SyncViewportState() {
  ClampScrollY();
  root_view_.SetViewRectInScreen(RectF::MakeLTRB(0, scroll_y_, LayoutWidth(),
                                                 scroll_y_ + ViewportHeight()));
}

void SkityDemo::ScrollBy(float delta_y) {
  ScrollTo(scroll_y_ + delta_y);
}

void SkityDemo::ScrollTo(float scroll_y) {
  scroll_y_ = scroll_y;
  SyncViewportState();
}

void SkityDemo::ClampScrollY() {
  const float max_scroll = std::max(0.f, content_height_ - ViewportHeight());
  scroll_y_ = std::clamp(scroll_y_, 0.f, max_scroll);
}

float SkityDemo::ContentTop() const {
  return kDocumentTopGap;
}

float SkityDemo::ContentLeft() const {
  const auto available_width = static_cast<float>(screen_width_);
  const auto layout_width = LayoutWidth();
  const auto centered_left = (available_width - layout_width) * 0.5f;
  return std::max(kWindowHorizontalInset, centered_left);
}

float SkityDemo::ViewportHeight() const {
  const auto available_height =
      std::max(0.f, static_cast<float>(screen_height_) - ContentTop() -
                        kDocumentBottomGap);
  return std::min(viewport_height_, available_height);
}

float SkityDemo::LayoutWidth() const {
  const auto available_width = std::max(
      1.f, static_cast<float>(screen_width_) - kWindowHorizontalInset * 2.f);
  return std::clamp(content_width_, 1.f, available_width);
}

void SkityDemo::SetPendingWindowTitle(std::string title) {
  pending_window_title_ = std::move(title);
}

}  // namespace serval::markdown::example
