// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_EXAMPLE_MACOS_APP_MTL_APP_H_
#define MARKDOWN_EXAMPLE_MACOS_APP_MTL_APP_H_

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <memory>
#include <string>

#include "skity/skity.hpp"

namespace skity {
class GPUSurface;
}  // namespace skity

@class CAMetalLayer;
@class NSWindow;

namespace serval::markdown::example {

class MTLApp {
 public:
  MTLApp(int32_t width, int32_t height, std::string name,
         const skity::Vec4& clear_color = {1.f, 1.f, 1.f, 1.f});
  virtual ~MTLApp();

  void Run();

  int32_t ScreenWidth() const { return width_; }
  int32_t ScreenHeight() const { return height_; }
  skity::GPUContext* GetGPUContext() const { return ctx_.get(); }

 protected:
  virtual void OnStart() {}
  virtual void OnUpdate(float elapsed_time) {}
  virtual void OnDestroy() {}
  virtual void OnHandleKey(int key, int scancode, int action, int mods);
  virtual void OnScroll(double offset_x, double offset_y) {}

  void GetCursorPos(double& x, double& y);
  skity::Canvas* GetCanvas();
  void SetWindowTitle(const std::string& title);

 private:
  void Init();
  void RunLoop();
  void Destroy();
  void InitMetalLayer();
  void UpdateMetalLayerFrame();
  void CreateSurface();
  int32_t DeviceSampleCount();

  static void KeyCallback(GLFWwindow* window, int key, int scancode, int action,
                          int mods);
  static void ScrollCallback(GLFWwindow* window, double offset_x,
                             double offset_y);

 private:
  int32_t width_{0};
  int32_t height_{0};
  std::string name_;
  GLFWwindow* window_{nullptr};
  NSWindow* ns_window_{nullptr};
  skity::Vec4 clear_color_;
  CAMetalLayer* metal_layer_{nullptr};
  std::unique_ptr<skity::GPUContext> ctx_;
  std::unique_ptr<skity::GPUSurface> surface_;
  skity::Canvas* canvas_{nullptr};
};

}  // namespace serval::markdown::example

#endif  // MARKDOWN_EXAMPLE_MACOS_APP_MTL_APP_H_
