// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_H_
#define MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "markdown/markdown_event_listener.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/view/markdown_view_measurer.h"
#include "skity/skity.hpp"
#include "skity_demo/skity_demo_config.h"
#include "skity_demo/skity_demo_input.h"
#include "skity_demo/skity_demo_platform_view.h"
#include "testing/markdown/frame_driven_tests/markdown_case_builder.h"

namespace serval::markdown {
class MarkdownView;
}

namespace serval::markdown::example {

class SkityDemoResourceLoader;

class SkityDemo final : public MarkdownEventListener {
 public:
  explicit SkityDemo(SkityDemoConfig config);
  ~SkityDemo() override;

  void Start();
  void Stop();
  void Resize(int32_t width, int32_t height);
  void Render(skity::Canvas* canvas, int64_t now_ms);

  bool HandleKey(SkityDemoKey key, SkityDemoKeyAction action);
  void HandleScroll(double offset_x, double offset_y);
  void HandlePointer(SkityDemoPointerAction action,
                     SkityDemoPointerButton button, float x, float y);

  const SkityDemoConfig& GetConfig() const;
  std::optional<std::string> TakePendingWindowTitle();

  void OnParseEnd() override;
  void OnTextOverflow(MarkdownTextOverflow overflow) override;
  void OnDrawStart() override;
  void OnDrawEnd() override;
  void OnAnimationStep(int32_t animation_step,
                       int32_t max_animation_step) override;
  void OnLinkClicked(const char* url, const char* content) override;
  void OnImageClicked(const char* url) override;
  void OnSelectionChanged(int32_t start_index, int32_t end_index,
                          SelectionHandleType handle,
                          SelectionState state) override;

 private:
  PointF WindowPointToContent(float x, float y) const;
  bool IsWindowPointInContent(float x, float y) const;
  void BeginPointerGesture(float x, float y, int64_t timestamp_ms);
  void EndPointerGesture(float x, float y);
  void MovePointerGesture(float x, float y);
  void ResetPointerGesture();
  void HandlePendingLongPress(int64_t now);

  void LoadCases();
  void ApplyCase(size_t index);
  void SelectCase(int direction);

  bool IsAnimationEnabled() const;
  void SetAnimationEnabled(bool enabled);

  float InitialScrollY(
      const serval::markdown::testing::MarkdownAttributes& attributes) const;
  void MeasureAndAlignIfNeeded();
  void SyncViewportState();
  void ScrollBy(float delta_y);
  void ScrollTo(float scroll_y);
  void ClampScrollY();

  float ContentTop() const;
  float ContentLeft() const;
  float ViewportHeight() const;
  float LayoutWidth() const;
  void SetPendingWindowTitle(std::string title);

 private:
  SkityDemoConfig config_;
  int32_t screen_width_{0};
  int32_t screen_height_{0};
  SkityDemoMainView root_view_;
  std::unique_ptr<SkityDemoResourceLoader> resource_loader_;
  std::shared_ptr<MarkdownView> markdown_view_;
  std::vector<serval::markdown::testing::MarkdownCaseEntry> cases_;
  size_t case_index_{0};
  bool animation_enabled_{true};
  float content_width_{kSkityDemoWidth};
  float viewport_height_{kSkityDemoHeight};
  float content_height_{0.f};
  float scroll_y_{0.f};
  bool pointer_down_{false};
  bool content_gesture_active_{false};
  bool long_press_sent_{false};
  bool markdown_pan_active_{false};
  bool drag_scroll_active_{false};
  int64_t pointer_down_time_ms_{0};
  float pointer_down_scroll_y_{0.f};
  PointF pointer_down_window_;
  PointF pointer_down_content_;
  PointF last_cursor_window_;
  std::optional<std::string> pending_window_title_;
};

}  // namespace serval::markdown::example

#endif  // MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_H_
