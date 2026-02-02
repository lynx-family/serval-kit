// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/internal/harmony_view.h"

#include <arkui/ui_input_event.h>
#include <hilog/log.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include "arkui/native_gesture.h"
#include "markdown/platform/harmony/internal/harmony_markdown_canvas.h"
#include "markdown/utils/markdown_screen_metrics.h"
#include "markdown/view/markdown_gesture.h"
#include "native_drawing/drawing_canvas.h"

namespace lynx::markdown {
HarmonyView::HarmonyView()
    : HarmonyView(ArkUINativeAPI::GetNodeApi()->createNode(ARKUI_NODE_CUSTOM)) {
}
HarmonyView::HarmonyView(ArkUI_NodeHandle handle)
    : handle_(handle), api_(ArkUINativeAPI::GetNodeApi()) {
  api_->addNodeCustomEventReceiver(handle_, CustomEventDispatcher);
  api_->addNodeEventReceiver(handle_, NodeEventDispatcher);
}
HarmonyView::~HarmonyView() {
  api_->removeNodeCustomEventReceiver(handle_, CustomEventDispatcher);
  api_->removeNodeEventReceiver(handle_, NodeEventDispatcher);
  // Wrap destructor, implement child node removal function.
  if (!children_.empty()) {
    RemoveAllChildren();
  }
  // Wrap destructor, unify node resource recovery.
  api_->disposeNode(handle_);
}
void HarmonyView::CustomEventDispatcher(ArkUI_NodeCustomEvent* event) {
  // Get the component instance object and call the related instance method.
  auto* view = reinterpret_cast<HarmonyView*>(
      OH_ArkUI_NodeCustomEvent_GetUserData(event));
  auto type = OH_ArkUI_NodeCustomEvent_GetEventType(event);
  switch (type) {
    case ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE: {
      auto layoutConstrain =
          OH_ArkUI_NodeCustomEvent_GetLayoutConstraintInMeasure(event);
      view->OnMeasure(layoutConstrain);
    } break;
    case ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT: {
      auto position = OH_ArkUI_NodeCustomEvent_GetPositionInLayout(event);
      view->OnLayout(position.x, position.y);
    } break;
    case ARKUI_NODE_CUSTOM_EVENT_ON_DRAW: {
      auto drawContext = OH_ArkUI_NodeCustomEvent_GetDrawContextInDraw(event);
      view->OnDraw(drawContext);
    } break;
    default:
      break;
  }
}
void HarmonyView::NodeEventDispatcher(ArkUI_NodeEvent* event) {
  auto* view =
      reinterpret_cast<HarmonyView*>(OH_ArkUI_NodeEvent_GetUserData(event));
  auto type = OH_ArkUI_NodeEvent_GetEventType(event);
  switch (type) {
    case NODE_TOUCH_EVENT: {
      auto* input = OH_ArkUI_NodeEvent_GetInputEvent(event);
      int32_t action = OH_ArkUI_UIInputEvent_GetAction(input);
      float x =
          MarkdownScreenMetrics::DPToPx(OH_ArkUI_PointerEvent_GetX(input));
      float y =
          MarkdownScreenMetrics::DPToPx(OH_ArkUI_PointerEvent_GetY(input));
      if (!view->OnTouchEvent(action, x, y)) {
        OH_ArkUI_PointerEvent_SetStopPropagation(input, false);
      }
    } break;
    default:
      break;
  }
}

GestureEventType HarmonyView::ConvertAction(int32_t action) {
  GestureEventType type;
  switch (action) {
    case UI_TOUCH_EVENT_ACTION_DOWN:
      type = GestureEventType::kDown;
      break;
    case UI_TOUCH_EVENT_ACTION_UP:
      type = GestureEventType::kUp;
      break;
    case UI_TOUCH_EVENT_ACTION_MOVE:
      type = GestureEventType::kMove;
      break;
    case UI_TOUCH_EVENT_ACTION_CANCEL:
      type = GestureEventType::kCancel;
      break;
    default:
      type = GestureEventType::kUnknown;
      break;
  }
  return type;
}

GestureEventType ConvertGestureType(int32_t action) {
  GestureEventType type;
  switch (action) {
    case UI_TOUCH_EVENT_ACTION_DOWN:
      type = GestureEventType::kDown;
      break;
    case UI_TOUCH_EVENT_ACTION_UP:
      type = GestureEventType::kUp;
      break;
    case UI_TOUCH_EVENT_ACTION_MOVE:
      type = GestureEventType::kMove;
      break;
    case UI_TOUCH_EVENT_ACTION_CANCEL:
      type = GestureEventType::kCancel;
      break;
    default:
      type = GestureEventType::kUnknown;
      break;
  }
  return type;
}

void HarmonyView::EnableTapEvent(bool enable, ArkUI_GesturePriority priority) {
  auto api = ArkUINativeAPI::GetGestureApi();
  if (enable) {
    if (tap_ == nullptr) {
      tap_ = api->createTapGestureWithDistanceThreshold(1, 1, 1);
      auto tap_callback = [](ArkUI_GestureEvent* event, void* ud) {
        auto* view = reinterpret_cast<HarmonyView*>(ud);
        if (view->tap_gesture_listener_ == nullptr)
          return;
        auto* input_event = OH_ArkUI_GestureEvent_GetRawInputEvent(event);
        auto x = OH_ArkUI_PointerEvent_GetX(input_event);
        auto y = OH_ArkUI_PointerEvent_GetY(input_event);
        auto type = OH_ArkUI_GestureEvent_GetActionType(event);
        if (type & GESTURE_EVENT_ACTION_ACCEPT) {
          view->tap_gesture_listener_({x, y}, GestureEventType::kDown);
        }
        if (type & GESTURE_EVENT_ACTION_END) {
          view->tap_gesture_listener_({x, y}, GestureEventType::kUp);
        }
      };
      api->setGestureEventTarget(tap_,
                                 GESTURE_EVENT_ACTION_ACCEPT |
                                     GESTURE_EVENT_ACTION_UPDATE |
                                     GESTURE_EVENT_ACTION_END,
                                 this, tap_callback);
    }
    api->addGestureToNode(handle_, tap_, priority, NORMAL_GESTURE_MASK);
  } else if (tap_ != nullptr) {
    api->removeGestureFromNode(handle_, tap_);
  }
}

void HarmonyView::EnableLongPressEvent(bool enable,
                                       ArkUI_GesturePriority priority) {
  auto api = ArkUINativeAPI::GetGestureApi();
  if (enable) {
    if (long_press_ == nullptr) {
      long_press_ = api->createLongPressGesture(1, false, 500);
      auto long_press_callback = [](ArkUI_GestureEvent* event, void* ud) {
        auto* view = reinterpret_cast<HarmonyView*>(ud);
        if (view->long_press_gesture_listener_ == nullptr)
          return;
        auto* input_event = OH_ArkUI_GestureEvent_GetRawInputEvent(event);
        auto x = OH_ArkUI_PointerEvent_GetX(input_event);
        auto y = OH_ArkUI_PointerEvent_GetY(input_event);
        auto type = OH_ArkUI_GestureEvent_GetActionType(event);
        if (type & GESTURE_EVENT_ACTION_ACCEPT) {
          view->long_press_gesture_listener_({x, y}, GestureEventType::kDown);
        }
        if (type & GESTURE_EVENT_ACTION_END) {
          view->long_press_gesture_listener_({x, y}, GestureEventType::kUp);
        }
      };
      api->setGestureEventTarget(long_press_,
                                 GESTURE_EVENT_ACTION_ACCEPT |
                                     GESTURE_EVENT_ACTION_UPDATE |
                                     GESTURE_EVENT_ACTION_END,
                                 this, long_press_callback);
    }
    api->addGestureToNode(handle_, long_press_, priority, NORMAL_GESTURE_MASK);
  } else if (long_press_ != nullptr) {
    api->removeGestureFromNode(handle_, long_press_);
  }
}
void HarmonyView::EnablePanEvent(bool enable,
                                 ArkUI_GestureDirectionMask direction,
                                 ArkUI_GesturePriority priority) {
  auto api = ArkUINativeAPI::GetGestureApi();
  if (enable) {
    if (pan_ == nullptr) {
      pan_ = api->createPanGesture(1, direction, 5);
      auto pan_callback = [](ArkUI_GestureEvent* event, void* ud) {
        auto* view = reinterpret_cast<HarmonyView*>(ud);
        if (view->pan_gesture_listener_ == nullptr)
          return;
        auto* input_event = OH_ArkUI_GestureEvent_GetRawInputEvent(event);
        auto x = OH_ArkUI_PointerEvent_GetX(input_event);
        auto y = OH_ArkUI_PointerEvent_GetY(input_event);
        auto ox = OH_ArkUI_PanGesture_GetOffsetX(event);
        auto oy = OH_ArkUI_PanGesture_GetOffsetY(event);
        auto type = OH_ArkUI_GestureEvent_GetActionType(event);
        if (type & GESTURE_EVENT_ACTION_ACCEPT) {
          view->pan_gesture_listener_({x, y}, {ox, oy},
                                      GestureEventType::kDown);
        }
        if (type & GESTURE_EVENT_ACTION_UPDATE) {
          view->pan_gesture_listener_({x, y}, {ox, oy},
                                      GestureEventType::kMove);
        }
        if (type & GESTURE_EVENT_ACTION_END) {
          view->pan_gesture_listener_({x, y}, {ox, oy}, GestureEventType::kUp);
        }
      };
      api->setGestureEventTarget(pan_,
                                 GESTURE_EVENT_ACTION_ACCEPT |
                                     GESTURE_EVENT_ACTION_UPDATE |
                                     GESTURE_EVENT_ACTION_END,
                                 this, pan_callback);
    }
    api->addGestureToNode(handle_, pan_, priority, NORMAL_GESTURE_MASK);
  } else if (pan_ != nullptr) {
    api->removeGestureFromNode(handle_, pan_);
  }
}

void HarmonyView::RequestCustomMeasure() {
  api_->registerNodeCustomEvent(handle_, ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE, 0,
                                this);
}
void HarmonyView::RequestCustomLayout() {
  api_->registerNodeCustomEvent(handle_, ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT, 0,
                                this);
}
void HarmonyView::RequestCustomDraw() {
  api_->registerNodeCustomEvent(handle_, ARKUI_NODE_CUSTOM_EVENT_ON_DRAW, 0,
                                this);
}
void HarmonyView::RequestTouchEvent() {
  api_->registerNodeEvent(handle_, NODE_TOUCH_EVENT, 0, this);
}

void HarmonyView::SetVisibility(ArkUI_Visibility visible) {
  SetIntAttribute(NODE_VISIBILITY, visible);
}

void HarmonyView::SetClipByParent(bool clip) {
  SetIntAttribute(NODE_CLIP, clip ? 1 : 0);
}

void HarmonyView::SetOpacity(float opacity) {
  SetFloatAttribute(NODE_OPACITY, opacity);
}

void HarmonyView::SetPadding(float padding) {
  SetFloatAttribute(NODE_PADDING, padding);
}

void HarmonyView::SetPaddings(float l, float t, float r, float b) {
  float data[]{t, r, b, l};
  SetFloatsAttribute(NODE_PADDING, data, 4);
}

MarginPadding HarmonyView::GetPaddings() {
  auto items = api_->getAttribute(handle_, NODE_PADDING);
  if (items == nullptr || items->size != 4) {
    return {};
  }
  MarginPadding result;
  result.top_ = items->value[0].f32;
  result.right_ = items->value[1].f32;
  result.bottom_ = items->value[2].f32;
  result.left_ = items->value[3].f32;
  return result;
}

void HarmonyView::SetMargin(float margin) {
  SetFloatAttribute(NODE_MARGIN, margin);
}
void HarmonyView::SetMargins(float l, float t, float r, float b) {
  float data[]{t, r, b, l};
  SetFloatsAttribute(NODE_MARGIN, data, 4);
}

MarginPadding HarmonyView::GetMargins() {
  auto items = api_->getAttribute(handle_, NODE_MARGIN);
  if (items == nullptr || items->size != 4) {
    return {};
  }
  MarginPadding result;
  result.top_ = items->value[0].f32;
  result.right_ = items->value[1].f32;
  result.bottom_ = items->value[2].f32;
  result.left_ = items->value[3].f32;
  return result;
}

void HarmonyView::SetIntAttribute(ArkUI_NodeAttributeType type, int32_t v) {
  ArkUI_NumberValue value;
  value.i32 = v;
  ArkUI_AttributeItem attribute;
  attribute.value = &value;
  attribute.size = 1;
  api_->setAttribute(handle_, type, &attribute);
}
void HarmonyView::SetFloatAttribute(ArkUI_NodeAttributeType type, float v) {
  ArkUI_NumberValue value;
  value.f32 = v;
  ArkUI_AttributeItem attribute;
  attribute.value = &value;
  attribute.size = 1;
  api_->setAttribute(handle_, type, &attribute);
}

void HarmonyView::SetFloatsAttribute(ArkUI_NodeAttributeType type, float* v,
                                     int32_t size) {
  std::vector<ArkUI_NumberValue> values;
  values.resize(size);
  for (int i = 0; i < size; i++) {
    values[i].f32 = v[i];
  }
  ArkUI_AttributeItem attribute;
  attribute.value = values.data();
  attribute.size = size;
  api_->setAttribute(handle_, type, &attribute);
}

SizeF HarmonyView::Measure(MeasureSpec spec) {
  auto* constraint = OH_ArkUI_LayoutConstraint_Create();
  OH_ArkUI_LayoutConstraint_SetPercentReferenceWidth(
      constraint, static_cast<int32_t>(std::min(1e5f, spec.width_)));
  OH_ArkUI_LayoutConstraint_SetPercentReferenceHeight(
      constraint, static_cast<int32_t>(std::min(1e5f, spec.height_)));
  OH_ArkUI_LayoutConstraint_SetMaxWidth(
      constraint, static_cast<int32_t>(std::min(1e5f, spec.width_)));
  OH_ArkUI_LayoutConstraint_SetMaxHeight(constraint,
                                         static_cast<int32_t>(1e5f));
  OH_ArkUI_LayoutConstraint_SetMinWidth(constraint, 0);
  OH_ArkUI_LayoutConstraint_SetMinHeight(constraint, 0);
  Measure(constraint);
  OH_ArkUI_LayoutConstraint_Dispose(constraint);
  return GetMeasuredSize();
}
void HarmonyView::Align(float left, float top) {
  Layout(left, top);
}
void HarmonyView::Draw(tttext::ICanvasHelper* canvas) {}
SizeF HarmonyView::GetMeasuredSize() {
  auto size = GetMeasuredIntSize();
  return SizeF{static_cast<float>(size.width), static_cast<float>(size.height)};
}
PointF HarmonyView::GetAlignedPosition() {
  auto pos = GetAlignedIntPos();
  return PointF{static_cast<float>(pos.x), static_cast<float>(pos.y)};
}
void HarmonyView::SetVisibility(bool visible) {
  SetVisibility(visible ? ARKUI_VISIBILITY_VISIBLE : ARKUI_VISIBILITY_HIDDEN);
}

EtsViewHolder::EtsViewHolder(ArkUI_NodeHandle child) : child_(child) {
  api_->addChild(handle_, child_);
  RequestCustomLayout();
  RequestCustomMeasure();
}
EtsViewHolder::~EtsViewHolder() {
  api_->removeChild(handle_, child_);
}
void EtsViewHolder::OnMeasure(ArkUI_LayoutConstraint* constraint) {
  api_->measureNode(child_, constraint);
  auto child_size = api_->getMeasuredSize(child_);
  if (child_size.width > 1e5 || child_size.height > 1e5) {
    OH_LOG_Print(LOG_APP, LOG_ERROR, 100, "NativeServalMarkdown",
                 "ets view measure too large, width:%d, height:%d",
                 child_size.width, child_size.height);
  }
  SetMeasuredSize(std::min(100000, child_size.width),
                  std::min(100000, child_size.height));
}
void EtsViewHolder::OnLayout(int32_t offset_x, int32_t offset_y) {
  SetLayoutPosition(offset_x, offset_y);
  api_->layoutNode(child_, 0, 0);
}

HarmonyCustomView::HarmonyCustomView() {
  RequestCustomMeasure();
  RequestCustomLayout();
  RequestCustomDraw();
}
void HarmonyCustomView::OnMeasure(ArkUI_LayoutConstraint* constraint) {
  if (drawable_ == nullptr) {
    return;
  }
  auto max_width = OH_ArkUI_LayoutConstraint_GetMaxWidth(constraint);
  auto max_height = OH_ArkUI_LayoutConstraint_GetMaxHeight(constraint);
  MeasureSpec spec{.width_ = static_cast<float>(max_width),
                   .height_ = static_cast<float>(max_height)};
  auto size = drawable_->Measure(spec);
  if (size.width_ > 1e5f || size.height_ > 1e5f) {
    OH_LOG_Print(LOG_APP, LOG_ERROR, 100, "NativeServalMarkdown",
                 "custom view measure too large, width:%f, height:%f",
                 size.width_, size.height_);
  }
  SetMeasuredSize(
      static_cast<int32_t>(std::ceil(std::min(1e5f, size.width_))),
      static_cast<int32_t>(std::ceil(std::min(1e5f, size.height_))));
}
void HarmonyCustomView::OnLayout(int32_t offset_x, int32_t offset_y) {
  SetLayoutPosition(offset_x, offset_y);
}
void HarmonyCustomView::OnDraw(ArkUI_DrawContext* context) {
  if (drawable_ == nullptr) {
    return;
  }
  auto draw_canvas = reinterpret_cast<OH_Drawing_Canvas*>(
      OH_ArkUI_DrawContext_GetCanvas(context));
  auto canvas = tttext::PlatformHelper::CreateCanvasHelper(
      tttext::PlatformType::kSystem, draw_canvas);
  auto size = GetMeasuredIntSize();
  canvas->ClipRect(0, 0, static_cast<float>(size.width),
                   static_cast<float>(size.height), true);
  HarmonyMarkdownCanvas markdown_canvas(canvas.get(), draw_canvas);
  drawable_->Draw(&markdown_canvas, 0, 0, static_cast<float>(size.width),
                  static_cast<float>(size.height));
}
}  // namespace lynx::markdown
