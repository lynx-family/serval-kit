// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRSVGTRAVERSALSTATE_H_
#define SVG_INCLUDE_PARSER_SRSVGTRAVERSALSTATE_H_

#include <array>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "canvas/SrCanvas.h"
#include "parser/SrSVGDOM.h"

namespace serval {
namespace svg {
namespace parser {

struct SrSVGTraversalState {
  static constexpr size_t kMaxRenderDepth = 256;
  // XML node limits do not bound repeated expansion of referenced nodes.
  static constexpr size_t kMaxRenderVisits = 262144;
  static constexpr size_t kMaxPatternTiles = 16384;
  static constexpr size_t kMaxOffscreenPixels = 32 * 1024 * 1024;
  static constexpr size_t kMaxLiveOffscreenBytes = 128 * 1024 * 1024;
  static constexpr size_t kMaxFilterPixelPasses = 128 * 1024 * 1024;
  static constexpr size_t kMaxFilterPrimitives = 64;
  static constexpr double kMaxBlurSigma = 4096.0 / 3.0;
  size_t render_depth{0};
  size_t render_visits{0};
  size_t pattern_tiles{0};
  std::array<const void*, kMaxRenderDepth> active_masks{};
  size_t mask_depth{0};

  bool HasRenderBudget() const { return render_visits < kMaxRenderVisits; }
  bool TakePatternTile() {
    if (!HasRenderBudget() || pattern_tiles >= kMaxPatternTiles)
      return false;
    ++pattern_tiles;
    return true;
  }

  std::array<const std::string*, kMaxRenderDepth> active_use_ids{};
  size_t use_depth{0};
  size_t offscreen_pixels{0};
  size_t live_offscreen_bytes{0};
  size_t filter_pixel_passes{0};

  static size_t OffscreenPixels(double width, double height) {
    const double pixels = std::ceil(width) * std::ceil(height);
    if (!std::isfinite(pixels) || width <= 0 || height <= 0 ||
        pixels > kMaxOffscreenPixels)
      return 0;
    return static_cast<size_t>(pixels);
  }
  std::vector<SrSVGDiagnostic> diagnostics;

  void Report(::SrSVGDiagnosticCode code, const char* message,
              const char* subject, bool fatal) {
    SrSVGDiagnostic diagnostic;
    diagnostic.code = code;
    diagnostic.message = message ? message : "";
    diagnostic.subject = subject ? subject : "";
    diagnostic.fatal = fatal;
    if (!diagnostics.empty()) {
      const auto& last = diagnostics.back();
      if (last.code == diagnostic.code && last.message == diagnostic.message &&
          last.subject == diagnostic.subject &&
          last.fatal == diagnostic.fatal) {
        return;
      }
    }
    diagnostics.push_back(std::move(diagnostic));
  }
};

// The canvas holds the context by address. Scope the viewport's user coordinates
// in place so leaving an SVG subtree also restores the box seen by the canvas.
class SrSVGViewBoxScope {
 public:
  explicit SrSVGViewBoxScope(SrSVGRenderContext& context)
      : box_(context.view_box), previous_(box_) {}
  SrSVGViewBoxScope(const SrSVGViewBoxScope&) = delete;
  SrSVGViewBoxScope& operator=(const SrSVGViewBoxScope&) = delete;
  ~SrSVGViewBoxScope() { box_ = previous_; }

 private:
  SrSVGBox& box_;
  SrSVGBox previous_;
};

class SrSVGRenderScope {
 public:
  explicit SrSVGRenderScope(const SrSVGRenderContext* context)
      : state_(context
                   ? static_cast<SrSVGTraversalState*>(context->traversal_state)
                   : nullptr) {
    if (state_) {
      entered_ = state_->render_depth < SrSVGTraversalState::kMaxRenderDepth &&
                 state_->HasRenderBudget();
      if (entered_) {
        if (state_->render_visits == 0)
          state_->offscreen_pixels = SrSVGTraversalState::OffscreenPixels(
              context->view_port.width, context->view_port.height);
        ++state_->render_depth;
        ++state_->render_visits;
      }
    }
  }
  SrSVGRenderScope(const SrSVGRenderScope&) = delete;
  SrSVGRenderScope& operator=(const SrSVGRenderScope&) = delete;
  ~SrSVGRenderScope() {
    if (state_ && entered_)
      --state_->render_depth;
  }
  explicit operator bool() const { return entered_; }

 private:
  SrSVGTraversalState* state_;
  bool entered_{true};
};

// Reserve RGBA surface bytes while the layer is alive, and debit the
// estimated filter passes for the whole render so sibling work is bounded too.
class SrSVGOffscreenScope {
 public:
  SrSVGOffscreenScope(const SrSVGOffscreenScope&) = delete;
  SrSVGOffscreenScope& operator=(const SrSVGOffscreenScope&) = delete;
  SrSVGOffscreenScope(const SrSVGRenderContext& context)
      : state_(static_cast<SrSVGTraversalState*>(context.traversal_state)),
        viewport_(context.view_port) {}
  ~SrSVGOffscreenScope() {
    if (reserved_ && state_)
      state_->live_offscreen_bytes -= bytes_;
  }

  bool Reserve(const canvas::SrCanvas& canvas, size_t surfaces,
               size_t passes = 0) {
    if (reserved_)
      return false;
    size_t pixels;
    if (!canvas.GetOffscreenPixelCount(&pixels))
      pixels = state_ ? state_->offscreen_pixels
                      : SrSVGTraversalState::OffscreenPixels(viewport_.width,
                                                             viewport_.height);
    if (!pixels || pixels > SrSVGTraversalState::kMaxOffscreenPixels ||
        surfaces > SrSVGTraversalState::kMaxLiveOffscreenBytes / 4 / pixels)
      return false;
    const size_t bytes = pixels * surfaces * 4;
    if (state_) {
      if (bytes > SrSVGTraversalState::kMaxLiveOffscreenBytes -
                      state_->live_offscreen_bytes ||
          passes > (SrSVGTraversalState::kMaxFilterPixelPasses -
                    state_->filter_pixel_passes) /
                       pixels)
        return false;
      state_->live_offscreen_bytes += bytes;
      state_->filter_pixel_passes += pixels * passes;
    }
    bytes_ = bytes;
    reserved_ = true;
    return true;
  }

 private:
  SrSVGTraversalState* state_;
  SrSVGBox viewport_;
  size_t bytes_{0};
  bool reserved_{false};
};

class SrSVGMaskScope {
 public:
  SrSVGMaskScope(SrSVGTraversalState* state, const void* mask)
      : state_(state), mask_(mask) {
    if (!state_ || state_->mask_depth == state_->active_masks.size())
      return;
    for (size_t i = 0; i < state_->mask_depth; ++i) {
      if (state_->active_masks[i] == mask)
        return;
    }
    can_enter_ = true;
  }
  SrSVGMaskScope(const SrSVGMaskScope&) = delete;
  SrSVGMaskScope& operator=(const SrSVGMaskScope&) = delete;
  ~SrSVGMaskScope() {
    if (entered_)
      --state_->mask_depth;
  }
  explicit operator bool() const { return can_enter_; }
  void EnterContent() {
    // Only resolving mask content expands the reference. Preserve the
    // existing application of the same mask to descendants of its source.
    state_->active_masks[state_->mask_depth++] = mask_;
    entered_ = true;
  }

 private:
  SrSVGTraversalState* state_;
  const void* mask_;
  bool can_enter_{false};
  bool entered_{false};
};

}  // namespace parser
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PARSER_SRSVGTRAVERSALSTATE_H_
