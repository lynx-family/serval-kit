// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define GLFW_INCLUDE_NONE
#import <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3native.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "mtl_app.h"

#include "skity/gpu/gpu_context_mtl.h"

namespace {
void ErrorCallback(int, const char* description) {
  fputs(description, stderr);
}
}  // namespace

namespace serval::markdown::example {

MTLApp::MTLApp(int32_t width, int32_t height, std::string name,
               const skity::Vec4& clear_color)
    : width_(width),
      height_(height),
      name_(std::move(name)),
      clear_color_(clear_color) {}

MTLApp::~MTLApp() = default;

void MTLApp::Run() {
  Init();
  RunLoop();
  Destroy();
}

void MTLApp::OnHandleKey(int key, int, int action, int) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
  }
}

void MTLApp::SetWindowTitle(const std::string& title) {
  if (window_ != nullptr) {
    glfwSetWindowTitle(window_, title.c_str());
  }
}

void MTLApp::KeyCallback(GLFWwindow* window, int key, int scancode, int action,
                         int mods) {
  auto* app = static_cast<MTLApp*>(glfwGetWindowUserPointer(window));
  if (app != nullptr && action == GLFW_RELEASE) {
    app->OnHandleKey(key, scancode, action, mods);
  }
}

void MTLApp::ScrollCallback(GLFWwindow* window, double offset_x,
                            double offset_y) {
  auto* app = static_cast<MTLApp*>(glfwGetWindowUserPointer(window));
  if (app != nullptr) {
    app->OnScroll(offset_x, offset_y);
  }
}

void MTLApp::Init() {
  glfwSetErrorCallback(ErrorCallback);

  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(width_, height_, name_.c_str(), nullptr, nullptr);
  if (window_ == nullptr) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetWindowUserPointer(window_, this);
  glfwSetKeyCallback(window_, KeyCallback);
  glfwSetScrollCallback(window_, ScrollCallback);

  InitMetalLayer();

  id<MTLDevice> device = metal_layer_.device;
  ctx_ = skity::MTLContextCreate(device, [device newCommandQueue]);

  OnStart();
}

void MTLApp::InitMetalLayer() {
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

void MTLApp::UpdateMetalLayerFrame() {
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

void MTLApp::CreateSurface() {
  UpdateMetalLayerFrame();
  if (metal_layer_.bounds.size.width <= 0 || metal_layer_.bounds.size.height <= 0) {
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

int32_t MTLApp::DeviceSampleCount() {
  int32_t sample_count = 8;
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  while (sample_count > 1 &&
         ![device supportsTextureSampleCount:sample_count]) {
    sample_count /= 2;
  }
  return sample_count;
}

void MTLApp::RunLoop() {
  while (!glfwWindowShouldClose(window_)) {
    @autoreleasepool {
      UpdateMetalLayerFrame();
      if (surface_ == nullptr) {
        CreateSurface();
      }
      if (surface_ == nullptr) {
        glfwPollEvents();
        continue;
      }

      auto* canvas = GetCanvas();
      canvas->RestoreToCount(1);
      canvas->ResetMatrix();
      canvas->DrawColor(clear_color_, skity::BlendMode::kSrc);
      OnUpdate(0.f);
      canvas->RestoreToCount(1);
      canvas->ResetMatrix();
      canvas->Flush();
      surface_->Flush();
      canvas_ = nullptr;
      glfwPollEvents();
    }
  }
}

void MTLApp::Destroy() {
  OnDestroy();
  glfwDestroyWindow(window_);
  glfwTerminate();
}

void MTLApp::GetCursorPos(double& x, double& y) {
  glfwGetCursorPos(window_, &x, &y);
}

skity::Canvas* MTLApp::GetCanvas() {
  if (canvas_ == nullptr && surface_ != nullptr) {
    canvas_ = surface_->LockCanvas();
  }
  return canvas_;
}

}  // namespace serval::markdown::example
