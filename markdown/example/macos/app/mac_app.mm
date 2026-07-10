// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define GLFW_INCLUDE_NONE
#import <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3native.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "app/mac_app.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "skity/gpu/gpu_context_mtl.h"

namespace {

void ErrorCallback(int, const char* description) {
  fputs(description, stderr);
}

int64_t NowMilliseconds() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

serval::markdown::example::SkityDemoKey ToDemoKey(int key) {
  using serval::markdown::example::SkityDemoKey;
  switch (key) {
    case GLFW_KEY_SPACE:
      return SkityDemoKey::kSpace;
    case GLFW_KEY_R:
      return SkityDemoKey::kR;
    case GLFW_KEY_N:
      return SkityDemoKey::kN;
    case GLFW_KEY_P:
      return SkityDemoKey::kP;
    case GLFW_KEY_LEFT:
    case GLFW_KEY_LEFT_BRACKET:
      return SkityDemoKey::kLeft;
    case GLFW_KEY_RIGHT:
    case GLFW_KEY_RIGHT_BRACKET:
      return SkityDemoKey::kRight;
    case GLFW_KEY_UP:
      return SkityDemoKey::kUp;
    case GLFW_KEY_DOWN:
      return SkityDemoKey::kDown;
    case GLFW_KEY_PAGE_UP:
      return SkityDemoKey::kPageUp;
    case GLFW_KEY_PAGE_DOWN:
      return SkityDemoKey::kPageDown;
    case GLFW_KEY_HOME:
      return SkityDemoKey::kHome;
    case GLFW_KEY_END:
      return SkityDemoKey::kEnd;
    default:
      return SkityDemoKey::kUnknown;
  }
}

serval::markdown::example::SkityDemoKeyAction ToDemoKeyAction(int action) {
  using serval::markdown::example::SkityDemoKeyAction;
  return action == GLFW_RELEASE ? SkityDemoKeyAction::kRelease
                                : SkityDemoKeyAction::kPress;
}

serval::markdown::example::SkityDemoPointerButton ToDemoPointerButton(
    int button) {
  using serval::markdown::example::SkityDemoPointerButton;
  return button == GLFW_MOUSE_BUTTON_LEFT ? SkityDemoPointerButton::kPrimary
                                          : SkityDemoPointerButton::kOther;
}

}  // namespace

namespace serval::markdown::example {

MacApp::MacApp(SkityDemoConfig config)
    : demo_(std::move(config)),
      width_(demo_.GetConfig().initial_width),
      height_(demo_.GetConfig().initial_height) {}

MacApp::~MacApp() {
  Destroy();
}

void MacApp::Run() {
  InitWindow();
  InitMetalLayer();
  InitGPUContext();
  demo_.Start();
  DispatchPendingTitle();
  RunLoop();
  Destroy();
}

void MacApp::InitWindow() {
  glfwSetErrorCallback(ErrorCallback);

  if (!glfwInit()) {
    std::exit(EXIT_FAILURE);
  }
  glfw_initialized_ = true;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ =
      glfwCreateWindow(width_, height_, demo_.GetConfig().window_title.c_str(),
                       nullptr, nullptr);
  if (window_ == nullptr) {
    Destroy();
    std::exit(EXIT_FAILURE);
  }

  glfwSetWindowUserPointer(window_, this);
  glfwSetKeyCallback(window_, KeyCallback);
  glfwSetScrollCallback(window_, ScrollCallback);
  glfwSetMouseButtonCallback(window_, MouseButtonCallback);
  glfwSetCursorPosCallback(window_, CursorPosCallback);
}

void MacApp::InitMetalLayer() {
  metal_layer_ = [CAMetalLayer layer];
  metal_layer_.device = MTLCreateSystemDefaultDevice();
  metal_layer_.opaque = YES;
  metal_layer_.pixelFormat = MTLPixelFormatBGRA8Unorm;
  metal_layer_.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
  metal_layer_.colorspace = CGColorSpaceCreateDeviceRGB();

  ns_window_ = glfwGetCocoaWindow(window_);
  ns_window_.contentView.wantsLayer = YES;
  ns_window_.contentView.layer = metal_layer_;
  UpdateMetalLayerFrame();
}

void MacApp::InitGPUContext() {
  id<MTLDevice> device = metal_layer_.device;
  ctx_ = skity::MTLContextCreate(device, [device newCommandQueue]);
}

void MacApp::CreateSurfaceIfNeeded() {
  UpdateMetalLayerFrame();
  if (surface_ == nullptr) {
    CreateSurface();
  }
}

void MacApp::CreateSurface() {
  if (ctx_ == nullptr || metal_layer_.bounds.size.width <= 0 ||
      metal_layer_.bounds.size.height <= 0) {
    return;
  }
  skity::GPUSurfaceDescriptorMTL desc{};
  desc.backend = skity::GPUBackendType::kMetal;
  desc.width = metal_layer_.bounds.size.width;
  desc.height = metal_layer_.bounds.size.height;
  desc.content_scale = metal_layer_.contentsScale;
  desc.sample_count = DeviceSampleCount();
  desc.surface_type = skity::MTLSurfaceType::kLayer;
  desc.layer = metal_layer_;

  surface_ = ctx_->CreateSurface(&desc);
}

void MacApp::RunLoop() {
  while (window_ != nullptr && !glfwWindowShouldClose(window_)) {
    @autoreleasepool {
      CreateSurfaceIfNeeded();
      if (surface_ == nullptr) {
        glfwPollEvents();
        continue;
      }

      auto* canvas = GetCanvas();
      canvas->RestoreToCount(1);
      canvas->ResetMatrix();
      canvas->DrawColor(demo_.GetConfig().clear_color, skity::BlendMode::kSrc);
      demo_.Resize(width_, height_);
      demo_.Render(canvas, NowMilliseconds());
      DispatchPendingTitle();
      canvas->RestoreToCount(1);
      canvas->ResetMatrix();
      canvas->Flush();
      surface_->Flush();
      canvas_ = nullptr;
      glfwPollEvents();
    }
  }
}

void MacApp::Destroy() {
  demo_.Stop();
  canvas_ = nullptr;
  surface_.reset();
  ctx_.reset();
  metal_layer_ = nil;
  ns_window_ = nil;
  if (window_ != nullptr) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
  if (glfw_initialized_) {
    glfwTerminate();
    glfw_initialized_ = false;
  }
}

void MacApp::DispatchPendingTitle() {
  if (window_ == nullptr) {
    return;
  }
  auto title = demo_.TakePendingWindowTitle();
  if (title.has_value()) {
    glfwSetWindowTitle(window_, title->c_str());
  }
}

void MacApp::UpdateMetalLayerFrame() {
  if (ns_window_ == nullptr || metal_layer_ == nullptr) {
    return;
  }
  [ns_window_.contentView layoutSubtreeIfNeeded];
  const CGRect bounds = ns_window_.contentView.bounds;
  const CGFloat scale = ns_window_.backingScaleFactor > 0
                            ? ns_window_.backingScaleFactor
                            : [[NSScreen mainScreen] backingScaleFactor];
  const CGSize drawable_size =
      CGSizeMake(bounds.size.width * scale, bounds.size.height * scale);
  const bool size_changed =
      !CGSizeEqualToSize(metal_layer_.bounds.size, bounds.size) ||
      !CGSizeEqualToSize(metal_layer_.drawableSize, drawable_size) ||
      metal_layer_.contentsScale != scale;

  metal_layer_.contentsScale = scale;
  metal_layer_.frame = bounds;
  metal_layer_.drawableSize = drawable_size;
  width_ = static_cast<int32_t>(bounds.size.width);
  height_ = static_cast<int32_t>(bounds.size.height);

  if (size_changed) {
    surface_.reset();
    canvas_ = nullptr;
  }
}

int32_t MacApp::DeviceSampleCount() {
  int32_t sample_count = 8;
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  while (sample_count > 1 &&
         ![device supportsTextureSampleCount:sample_count]) {
    sample_count /= 2;
  }
  return sample_count;
}

skity::Canvas* MacApp::GetCanvas() {
  if (canvas_ == nullptr && surface_ != nullptr) {
    canvas_ = surface_->LockCanvas();
  }
  return canvas_;
}

void MacApp::KeyCallback(GLFWwindow* window, int key, int scancode, int action,
                         int mods) {
  (void)scancode;
  (void)mods;
  auto* app = static_cast<MacApp*>(glfwGetWindowUserPointer(window));
  if (app == nullptr) {
    return;
  }
  if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }
  app->demo_.HandleKey(ToDemoKey(key), ToDemoKeyAction(action));
}

void MacApp::ScrollCallback(GLFWwindow* window, double offset_x,
                            double offset_y) {
  auto* app = static_cast<MacApp*>(glfwGetWindowUserPointer(window));
  if (app != nullptr) {
    app->demo_.HandleScroll(offset_x, offset_y);
  }
}

void MacApp::MouseButtonCallback(GLFWwindow* window, int button, int action,
                                 int mods) {
  (void)mods;
  auto* app = static_cast<MacApp*>(glfwGetWindowUserPointer(window));
  if (app == nullptr) {
    return;
  }
  double cursor_x = 0;
  double cursor_y = 0;
  glfwGetCursorPos(window, &cursor_x, &cursor_y);
  if (action == GLFW_PRESS) {
    app->demo_.HandlePointer(
        SkityDemoPointerAction::kDown, ToDemoPointerButton(button),
        static_cast<float>(cursor_x), static_cast<float>(cursor_y));
    return;
  }
  if (action == GLFW_RELEASE) {
    app->demo_.HandlePointer(
        SkityDemoPointerAction::kUp, ToDemoPointerButton(button),
        static_cast<float>(cursor_x), static_cast<float>(cursor_y));
  }
}

void MacApp::CursorPosCallback(GLFWwindow* window, double x, double y) {
  auto* app = static_cast<MacApp*>(glfwGetWindowUserPointer(window));
  if (app != nullptr) {
    app->demo_.HandlePointer(SkityDemoPointerAction::kMove,
                             SkityDemoPointerButton::kPrimary,
                             static_cast<float>(x), static_cast<float>(y));
  }
}

}  // namespace serval::markdown::example
