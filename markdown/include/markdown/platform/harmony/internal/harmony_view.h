// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_VIEW_H_
#include <cstdint>
#include <list>
#include <memory>
#include <utility>

#include "arkui/native_type.h"
#include "markdown/platform/harmony/internal/harmony_utils.h"
#include "markdown/view/markdown_platform_view.h"
namespace lynx::markdown {
struct MarginPadding {
  float left_{0};
  float top_{0};
  float right_{0};
  float bottom_{0};
};
class HarmonyView : public MarkdownPlatformView {
 public:
  HarmonyView();
  explicit HarmonyView(ArkUI_NodeHandle handle);
  ~HarmonyView() override;

  SizeF Measure(MeasureSpec spec) override;
  void Align(float left, float top) override;
  void Draw(tttext::ICanvasHelper* canvas) override;
  SizeF GetMeasuredSize() final;
  PointF GetAlignedPosition() final;
  void SetMeasuredSize(SizeF size) final {
    SetMeasuredSize(static_cast<int32_t>(size.width_),
                    static_cast<int32_t>(size.height_));
  }
  void SetAlignPosition(PointF position) final {
    SetLayoutPosition(static_cast<int32_t>(position.x_),
                      static_cast<int32_t>(position.y_));
  }
  void SetVisibility(bool visible) final;
  void SetPadding(float padding);
  void SetPaddings(float left, float top, float right, float bottom);
  void SetMargin(float margin);
  void SetMargins(float left, float top, float right, float bottom);
  MarginPadding GetPaddings();
  MarginPadding GetMargins();

  ArkUI_IntSize GetMeasuredIntSize() { return api_->getMeasuredSize(handle_); }
  ArkUI_IntOffset GetAlignedIntPos() {
    return api_->getLayoutPosition(handle_);
  }
  void Measure(ArkUI_LayoutConstraint* constraint) {
    api_->measureNode(handle_, constraint);
  }
  void SetMeasuredSize(int32_t width, int32_t height) {
    api_->setMeasuredSize(handle_, width, height);
  }
  void Layout(float x_offset, float y_offset) {
    api_->layoutNode(handle_, static_cast<int32_t>(x_offset),
                     static_cast<int32_t>(y_offset));
  }
  void SetLayoutPosition(int32_t x_offset, int32_t y_offset) {
    api_->setLayoutPosition(handle_, x_offset, y_offset);
  }
  void RequestMeasure() override { MarkNeedsMeasure(); }
  void RequestAlign() override { MarkNeedsLayout(); }
  void RequestDraw() override { MarkNeedsRender(); }

  void EnableTapEvent(bool enable, ArkUI_GesturePriority priority);
  void EnableLongPressEvent(bool enable, ArkUI_GesturePriority priority);
  void EnablePanEvent(bool enable, ArkUI_GestureDirectionMask direction,
                      ArkUI_GesturePriority priority);

  void SetWidth(float width) {
    ArkUI_NumberValue value[] = {{.f32 = width}};
    ArkUI_AttributeItem item = {value, 1};
    api_->setAttribute(handle_, NODE_WIDTH, &item);
  }
  void SetPercentWidth(float percent) {
    ArkUI_NumberValue value[] = {{.f32 = percent}};
    ArkUI_AttributeItem item = {value, 1};
    api_->setAttribute(handle_, NODE_WIDTH_PERCENT, &item);
  }
  void SetHeight(float height) {
    ArkUI_NumberValue value[] = {{.f32 = height}};
    ArkUI_AttributeItem item = {value, 1};
    api_->setAttribute(handle_, NODE_HEIGHT, &item);
  }
  void SetPercentHeight(float percent) {
    ArkUI_NumberValue value[] = {{.f32 = percent}};
    ArkUI_AttributeItem item = {value, 1};
    api_->setAttribute(handle_, NODE_HEIGHT_PERCENT, &item);
  }
  void SetBackgroundColor(uint32_t color) {
    ArkUI_NumberValue value[] = {{.u32 = color}};
    ArkUI_AttributeItem item = {value, 1};
    api_->setAttribute(handle_, NODE_BACKGROUND_COLOR, &item);
  }
  void MarkNeedsMeasure() { api_->markDirty(handle_, NODE_NEED_MEASURE); }
  void MarkNeedsLayout() { api_->markDirty(handle_, NODE_NEED_LAYOUT); }
  void MarkNeedsRender() { api_->markDirty(handle_, NODE_NEED_RENDER); }

  void AddChild(std::unique_ptr<HarmonyView> child) {
    api_->addChild(handle_, child->GetHandle());
    children_.emplace_back(std::move(child));
  }
  std::unique_ptr<HarmonyView> RemoveChild(HarmonyView* child) {
    auto pred = [child](const auto& ptr) { return ptr.get() == child; };
    auto find = std::find_if(children_.begin(), children_.end(), pred);
    if (find == children_.end()) return nullptr;
    auto content = std::move(*find);
    children_.erase(find);
    api_->removeChild(handle_, child->GetHandle());
    return content;
  }
  void RemoveAllChildren() {
    for (const auto& child : children_) {
      api_->removeChild(handle_, child->GetHandle());
    }
    children_.clear();
  }
  void InsertChild(std::unique_ptr<HarmonyView> child, int32_t index) {
    if (index >= children_.size()) {
      AddChild(std::move(child));
    } else {
      auto iter = children_.begin();
      std::advance(iter, index);
      api_->insertChildAt(handle_, child->GetHandle(), index);
      children_.insert(iter, std::move(child));
    }
  }
  ArkUI_NodeHandle GetHandle() const { return handle_; }

  void SetVisibility(ArkUI_Visibility visible);
  void SetClipByParent(bool clip);
  void SetOpacity(float opacity);

 protected:
  static void CustomEventDispatcher(ArkUI_NodeCustomEvent* event);
  static void NodeEventDispatcher(ArkUI_NodeEvent* event);
  void RequestCustomMeasure();
  void RequestCustomLayout();
  void RequestCustomDraw();
  void RequestTouchEvent();
  void SetIntAttribute(ArkUI_NodeAttributeType type, int32_t v);
  void SetFloatAttribute(ArkUI_NodeAttributeType type, float v);
  void SetFloatsAttribute(ArkUI_NodeAttributeType type, float* v, int32_t size);
  virtual void OnMeasure(ArkUI_LayoutConstraint* constraint) {}
  virtual void OnLayout(int32_t offset_x, int32_t offset_y) {}
  virtual void OnDraw(ArkUI_DrawContext* context) {}
  virtual bool OnTouchEvent(int32_t action, float x, float y) { return false; }
  static GestureEventType ConvertAction(int32_t action);

 protected:
  ArkUI_NodeHandle handle_{nullptr};
  ArkUI_NativeNodeAPI_1* api_{nullptr};
  std::list<std::unique_ptr<HarmonyView>> children_;
  ArkUI_GestureRecognizer* long_press_{nullptr};
  ArkUI_GestureRecognizer* tap_{nullptr};
  ArkUI_GestureRecognizer* pan_{nullptr};
};

// visibility can't set to ets view, should using a custom view wrap it.
class EtsViewHolder : public HarmonyView {
 public:
  explicit EtsViewHolder(ArkUI_NodeHandle child);
  ~EtsViewHolder() override;
  void OnMeasure(ArkUI_LayoutConstraint* constraint) override;
  void OnLayout(int32_t offset_x, int32_t offset_y) override;

 private:
  ArkUI_NodeHandle child_;
};

class HarmonyCustomView : public HarmonyView, public MarkdownCustomViewHandle {
 public:
  HarmonyCustomView();
  ~HarmonyCustomView() override = default;

  MarkdownCustomViewHandle* GetCustomViewHandle() final { return this; }

  void OnMeasure(ArkUI_LayoutConstraint* constraint) final;
  void OnLayout(int32_t offset_x, int32_t offset_y) override;
  void OnDraw(ArkUI_DrawContext* context) final;
};

}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_VIEW_H_
