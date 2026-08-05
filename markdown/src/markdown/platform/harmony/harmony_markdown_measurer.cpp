// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/harmony_markdown_measurer.h"

#include <memory>
#include <string>
#include <utility>

#include "markdown/element/markdown_context.h"
#include "markdown/platform/harmony/internal/harmony_view.h"
#include "markdown/platform/harmony/markdown_platform_harmony.h"
#include "markdown/platform/harmony/serval_markdown_view.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/view/markdown_view.h"

namespace serval::markdown {
namespace {

std::string GetStringValue(const ValueMap& config, const std::string& key,
                           const std::string& default_value = "") {
  if (auto iter = config.find(key);
      iter != config.end() && iter->second->GetType() == ValueType::kString) {
    return iter->second->GetString();
  }
  return default_value;
}

double GetNumberValue(const ValueMap& config, const std::string& key,
                      double default_value = 0) {
  if (auto iter = config.find(key);
      iter != config.end() && iter->second->GetType() == ValueType::kDouble) {
    return iter->second->GetDouble();
  }
  return default_value;
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
  auto* view = GetMarkdownView();
  if (auto animation_type = GetStringValue(config, "animationType");
      !animation_type.empty()) {
    if (animation_type == "typewriter") {
      view->SetAnimationType(MarkdownAnimationType::kTypewriter);
    } else if (animation_type == "line-expand") {
      view->SetAnimationType(MarkdownAnimationType::kLineExpand);
    } else {
      view->SetAnimationType(MarkdownAnimationType::kNone);
    }
  }
  if (auto velocity = GetNumberValue(config, "animationVelocity", -1);
      velocity >= 0) {
    view->SetAnimationVelocity(static_cast<float>(velocity));
  }
  if (auto iter = config.find("typewriterDynamicHeight");
      iter != config.end() && iter->second->GetType() == ValueType::kBool) {
    view->SetTypewriterDynamicHeight(iter->second->AsBool());
  }
  bool enable_selection = false;
  if (auto iter = config.find("enableSelection");
      iter != config.end() && iter->second->GetType() == ValueType::kBool) {
    enable_selection = iter->second->GetBool();
  }
  view->SetEnableSelection(enable_selection);
  if (GetStringValue(config, "sourceType") == "plainText") {
    view->SetSourceType(SourceType::kPlainText);
  }
  if (const auto parser = GetStringValue(config, "parser"); !parser.empty()) {
    view->SetParserType(parser, nullptr);
  }
  if (auto color = GetStringValue(config, "selectionHandleColor");
      !color.empty()) {
    view->SetSelectionHandleColor(MarkdownStyleReader::ReadColor(color));
  }
  if (auto color = GetStringValue(config, "selectionHighlightColor");
      !color.empty()) {
    view->SetSelectionHighlightColor(MarkdownStyleReader::ReadColor(color));
  }
  if (const auto size = GetNumberValue(config, "selectionHandleSize");
      size > 0) {
    view->SetSelectionHandleSize(static_cast<float>(size));
  }
  if (const auto margin = GetNumberValue(config, "selectionHandleTouchMargin");
      margin > 0) {
    view->SetSelectionHandleTouchMargin(static_cast<float>(margin));
  }
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
    bound_view_->RequestMeasure();
  }
}

void NativeMarkdownMeasurer::SetResourceLoader(IHarmonyResourceLoader* loader) {
  loader_ = loader;
}

void* NativeMarkdownMeasurer::LoadFont(const char* family,
                                       MarkdownFontWeight weight) {
  return loader_ == nullptr ? nullptr : loader_->LoadFont(family);
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
