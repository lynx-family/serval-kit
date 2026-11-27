// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/harmony_markdown_measurer.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "markdown/element/markdown_context.h"
#include "markdown/platform/harmony/internal/harmony_utils.h"
#include "markdown/platform/harmony/internal/harmony_view.h"
#include "markdown/platform/harmony/markdown_platform_harmony.h"
#include "markdown/platform/harmony/serval_markdown_view.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/view/markdown_view.h"

namespace serval::markdown {
namespace {

std::unique_ptr<Value> CopyValue(Value& value) {
  switch (value.GetType()) {
    case ValueType::kNull:
      return Value::MakeNull();
    case ValueType::kBool:
      return Value::MakeBool(value.AsBool());
    case ValueType::kInt:
      return Value::MakeInt(value.AsInt());
    case ValueType::kLong:
      return Value::MakeLong(value.AsLong());
    case ValueType::kDouble:
      return Value::MakeDouble(value.AsDouble());
    case ValueType::kString:
      return Value::MakeString(value.GetString());
    case ValueType::kArray: {
      ValueArray array;
      for (const auto& item : value.AsArray()) {
        array.emplace_back(CopyValue(*item));
      }
      return Value::MakeArray(std::move(array));
    }
    case ValueType::kMap: {
      ValueMap map;
      for (const auto& [key, item] : value.AsMap()) {
        map.emplace(key, CopyValue(*item));
      }
      return Value::MakeMap(std::move(map));
    }
  }
}

int32_t ToFontWeight(MarkdownFontWeight weight) {
  switch (weight) {
    case MarkdownFontWeight::kBold:
      return 700;
    case MarkdownFontWeight::k100:
      return 100;
    case MarkdownFontWeight::k200:
      return 200;
    case MarkdownFontWeight::k300:
      return 300;
    case MarkdownFontWeight::k400:
      return 400;
    case MarkdownFontWeight::k500:
      return 500;
    case MarkdownFontWeight::k600:
      return 600;
    case MarkdownFontWeight::k700:
      return 700;
    case MarkdownFontWeight::k800:
      return 800;
    case MarkdownFontWeight::k900:
      return 900;
    case MarkdownFontWeight::kNormal:
      return 400;
  }
}

}  // namespace

NativeMarkdownMeasurer::NativeMarkdownMeasurer() {
  view_ = std::make_shared<MarkdownView>(
      nullptr, this,
      std::make_shared<MarkdownContext>(CreateHarmonyMarkdownPlatform()));
  GetMarkdownView()->SetEnableRegionView(true);
  GetMarkdownView()->SetResourceLoader(this);
}

NativeMarkdownMeasurer::~NativeMarkdownMeasurer() {
  request_measure_callback_ = nullptr;
  if (bound_view_ != nullptr) {
    DetachView(bound_view_);
  }
  auto* markdown_view = GetMarkdownView();
  markdown_view->SetExposureListener(nullptr);
  markdown_view->SetEventListener(nullptr);
  markdown_view->SetResourceLoader(nullptr);
}

void NativeMarkdownMeasurer::SetContent(const std::string& content) const {
  GetMarkdownView()->SetContent(content);
}

void NativeMarkdownMeasurer::SetStyle(const ValueMap& style) const {
  GetMarkdownView()->SetStyle(style);
}

void NativeMarkdownMeasurer::MarkDirty() const {
  GetMarkdownView()->MarkDirty();
}

void NativeMarkdownMeasurer::SetConfig(const ValueMap& config) const {
  static const std::unordered_map<std::string, MarkdownProps> props = {
      {"animationType", MarkdownProps::kAnimationType},
      {"animationVelocity", MarkdownProps::kAnimationVelocity},
      {"initialAnimationStep", MarkdownProps::kInitialAnimationStep},
      {"animationFrameRate", MarkdownProps::kAnimationFrameRate},
      {"textMaxline", MarkdownProps::kTextMaxline},
      {"markdownMaxHeight", MarkdownProps::kMarkdownMaxHeight},
      {"contentComplete", MarkdownProps::kContentComplete},
      {"contentRangeStart", MarkdownProps::kContentRangeStart},
      {"contentRangeEnd", MarkdownProps::kContentRangeEnd},
      {"typewriterDynamicHeight", MarkdownProps::kTypewriterDynamicHeight},
      {"typewriterHeightTransitionDuration",
       MarkdownProps::kTypewriterHeightTransitionDuration},
      {"typewriterHeightTransitionPrefetch",
       MarkdownProps::kTypewriterHeightTransitionPrefetch},
      {"enableSyntaxCompletion", MarkdownProps::kEnableSyntaxCompletion},
      {"allowBreakAroundPunctuation",
       MarkdownProps::kAllowBreakAroundPunctuation},
      {"enableSelection", MarkdownProps::kEnableTextSelection},
      {"selectionHandleColor", MarkdownProps::kSelectionHandleColor},
      {"selectionHighlightColor", MarkdownProps::kSelectionHighlightColor},
      {"selectionHandleSize", MarkdownProps::kSelectionHandleSize},
      {"selectionHandleTouchMargin",
       MarkdownProps::kSelectionHandleTouchMargin},
      {"exposureTags", MarkdownProps::kExposureTags},
      {"textMarkAttachments", MarkdownProps::kTextMarkAttachments},
      {"markdownEffect", MarkdownProps::kMarkdownEffect},
  };
  for (const auto& [name, value] : config) {
    if (const auto prop = props.find(name); prop != props.end()) {
      switch (value->GetType()) {
        case ValueType::kBool:
        case ValueType::kInt:
        case ValueType::kLong:
        case ValueType::kDouble:
          SetNumberProp(prop->second, value->GetDouble());
          break;
        case ValueType::kString:
          if (prop->second == MarkdownProps::kSelectionHandleColor ||
              prop->second == MarkdownProps::kSelectionHighlightColor) {
            SetNumberProp(prop->second,
                          MarkdownStyleReader::ReadColor(value->AsString()));
          } else {
            SetStringProp(prop->second, value->AsString());
          }
          break;
        case ValueType::kArray: {
          // Attachment setters take ownership; SetConfig leaves its input intact.
          auto copy = CopyValue(*value);
          SetArrayProp(prop->second, copy->AsArray());
          break;
        }
        case ValueType::kMap:
          SetMapProp(prop->second, value->AsMap());
          break;
        case ValueType::kNull:
          break;
      }
    } else if (name == "contentID" && value->GetType() == ValueType::kString) {
      view_->SetContentID(value->AsString());
    } else if (name == "sourceType" && value->GetType() == ValueType::kString) {
      view_->SetSourceType(value->AsString() == "plainText"
                               ? SourceType::kPlainText
                               : SourceType::kMarkdown);
    } else if (name == "parser" && value->GetType() == ValueType::kString) {
      view_->SetParserType(value->AsString());
    } else if (name == "trimParagraphSpaces" &&
               value->GetType() == ValueType::kBool) {
      view_->SetTrimParagraphSpaces(value->AsBool());
    }
  }
  // An explicit step takes precedence over the initial step in the same update.
  if (const auto step = config.find("animationStep"); step != config.end()) {
    const auto type = step->second->GetType();
    if (type == ValueType::kDouble || type == ValueType::kInt ||
        type == ValueType::kLong) {
      SetAnimationStep(step->second->GetInt());
    }
  }
}

void NativeMarkdownMeasurer::SetNumberProp(MarkdownProps prop,
                                           double value) const {
  view_->SetNumberProp(prop, value);
}
void NativeMarkdownMeasurer::SetStringProp(MarkdownProps prop,
                                           const std::string& value) const {
  view_->SetStringProp(prop, value);
}
void NativeMarkdownMeasurer::SetArrayProp(MarkdownProps prop,
                                          ValueArray& value) const {
  view_->SetArrayProp(prop, value);
}
void NativeMarkdownMeasurer::SetMapProp(MarkdownProps prop,
                                        ValueMap& value) const {
  view_->SetMapProp(prop, value);
}
void NativeMarkdownMeasurer::SetEventListener(
    MarkdownEventListener* listener) const {
  view_->SetEventListener(listener);
}
void NativeMarkdownMeasurer::SetExposureListener(
    MarkdownExposureListener* listener) const {
  view_->SetExposureListener(listener);
}
void NativeMarkdownMeasurer::OnLayoutFrame(int64_t frame_time_nanos) {
  animation_clock_.OnFrame(frame_time_nanos);
  if (!animation_clock_.IsPaused()) {
    view_->OnLayoutFrame(animation_clock_.GetAnimationTimeMs());
  }
}
void NativeMarkdownMeasurer::PauseAnimation() {
  animation_clock_.Pause();
}
void NativeMarkdownMeasurer::ResumeAnimation(int32_t animation_step) {
  if (animation_step != -1) {
    SetAnimationStep(animation_step);
  }
  animation_clock_.Resume();
}
int32_t NativeMarkdownMeasurer::GetAnimationStep() const {
  return view_->GetAnimationStep();
}
void NativeMarkdownMeasurer::SetAnimationStep(int32_t animation_step) const {
  view_->SetAnimationStep(animation_step);
}
void NativeMarkdownMeasurer::OnImageLoaded(const std::string& url) {
  view_->OnImageLoaded(url);
  RequestMeasure();
}
void NativeMarkdownMeasurer::OnFontLoaded(const std::string& family,
                                          int32_t weight, int32_t style) {
  view_->OnFontLoaded(family, weight, style);
  RequestMeasure();
}

MeasureResult NativeMarkdownMeasurer::Measure(MeasureSpec spec) {
  return view_->Measure(spec);
}

void NativeMarkdownMeasurer::Align(float left, float top) {
  view_->Align(left, top);
}

bool NativeMarkdownMeasurer::BindView(NativeServalMarkdownView* view) {
  if (view == nullptr || bound_view_ != nullptr) {
    return false;
  }
  bound_view_ = view;
  for (const auto& [handle, platform_view] : view_cache_) {
    bound_view_->InsertEtsView(handle, platform_view);
  }
  view_cache_.clear();
  bound_view_->AttachDrawable(view_);
  view_->SetView(bound_view_);
  return true;
}

void NativeMarkdownMeasurer::DetachView(NativeServalMarkdownView* view) {
  if (view == nullptr || view != bound_view_) {
    return;
  }
  view_cache_ = std::move(bound_view_->view_cache_);
  view_->DetachView();
  bound_view_->RemoveAllSubViews();
  bound_view_->AttachDrawable(nullptr);
  bound_view_->measurer_ = nullptr;
  bound_view_ = nullptr;
}

void NativeMarkdownMeasurer::SetRequestMeasureCallback(
    std::function<void()> callback) {
  request_measure_callback_ = std::move(callback);
}

void NativeMarkdownMeasurer::RequestMeasure() {
  if (request_measure_callback_) {
    request_measure_callback_();
  } else if (bound_view_ != nullptr) {
    HarmonyUIThread::RunOnUIThread(
        [view = bound_view_]() { view->RequestMeasure(); });
  }
}

void NativeMarkdownMeasurer::SetResourceLoader(IHarmonyResourceLoader* loader) {
  loader_ = loader;
}

void* NativeMarkdownMeasurer::LoadFont(const char* family,
                                       MarkdownFontWeight weight) {
  return loader_ == nullptr
             ? nullptr
             : loader_->LoadFont(family, ToFontWeight(weight), 0);
}

std::shared_ptr<MarkdownDrawable> NativeMarkdownMeasurer::LoadInlineView(
    const char* id_selector, float max_width, float max_height) {
  if (loader_ == nullptr) {
    return nullptr;
  }
  return std::static_pointer_cast<MarkdownDrawable>(
      WrapEtsView(loader_->LoadInlineView(id_selector, max_width, max_height)));
}

std::shared_ptr<MarkdownDrawable> NativeMarkdownMeasurer::LoadImage(
    const char* src, float desire_width, float desire_height, float max_width,
    float max_height, float border_radius) {
  if (loader_ == nullptr) {
    return nullptr;
  }
  return std::static_pointer_cast<MarkdownDrawable>(WrapEtsView(
      loader_->LoadImageView(src, desire_width, desire_height, max_width,
                             max_height, border_radius)));
}

MarkdownReplacementView NativeMarkdownMeasurer::LoadReplacementView(
    void* ud, int32_t id, float max_width, float max_height) {
  if (loader_ == nullptr) {
    return {};
  }
  auto replacement =
      loader_->LoadReplacementView(ud, id, max_width, max_height);
  return {
      .view_ = WrapEtsView(replacement.view_),
      .alt_text_ = std::move(replacement.alt_text_),
  };
}

std::shared_ptr<MarkdownPlatformView> NativeMarkdownMeasurer::WrapEtsView(
    ArkUI_NodeHandle handle) {
  if (handle == nullptr) {
    return nullptr;
  }
  if (bound_view_ != nullptr) {
    return bound_view_->InsertEtsView(handle);
  }
  auto iter = view_cache_.find(handle);
  if (iter == view_cache_.end()) {
    auto view = std::static_pointer_cast<MarkdownPlatformView>(
        std::make_shared<EtsViewHolder>(handle));
    iter = view_cache_.emplace(handle, std::move(view)).first;
  }
  return iter->second;
}

}  // namespace serval::markdown
