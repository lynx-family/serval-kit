// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_EXAMPLE_MACOS_APP_MAC_APP_H_
#define MARKDOWN_EXAMPLE_MACOS_APP_MAC_APP_H_

#include <cstdint>
#include <memory>

#include "skity_demo/skity_demo.h"

struct GLFWwindow;

#ifdef __OBJC__
@class CAMetalLayer;
@class NSWindow;
#else
class CAMetalLayer;
class NSWindow;
#endif

namespace skity {
class Canvas;
class GPUContext;
class GPUSurface;
}  // namespace skity

namespace serval::markdown::example {

class MacApp {
 public:
  explicit MacApp(SkityDemoConfig config);
  ~MacApp();

  void Run();

 private:
  void InitWindow();
  void InitMetalLayer();
  void InitGPUContext();
  void CreateSurfaceIfNeeded();
  void CreateSurface();
  void RunLoop();
  void Destroy();
  void DispatchPendingTitle();
  void UpdateMetalLayerFrame();
  int32_t DeviceSampleCount();
  skity::Canvas* GetCanvas();

  static void KeyCallback(GLFWwindow* window, int key, int scancode, int action,
                          int mods);
  static void ScrollCallback(GLFWwindow* window, double offset_x,
                             double offset_y);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action,
                                  int mods);
  static void CursorPosCallback(GLFWwindow* window, double x, double y);

 private:
  SkityDemo demo_;
  int32_t width_{0};
  int32_t height_{0};
  GLFWwindow* window_{nullptr};
  NSWindow* ns_window_{nullptr};
  CAMetalLayer* metal_layer_{nullptr};
  std::unique_ptr<skity::GPUContext> ctx_;
  std::unique_ptr<skity::GPUSurface> surface_;
  skity::Canvas* canvas_{nullptr};
  bool glfw_initialized_{false};
};

}  // namespace serval::markdown::example

#endif  // MARKDOWN_EXAMPLE_MACOS_APP_MAC_APP_H_
