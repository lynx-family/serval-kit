// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_SKITY_SRSKITYMATRIXCANVAS_H_
#define SVG_INCLUDE_PLATFORM_SKITY_SRSKITYMATRIXCANVAS_H_

#include <memory>
#include <vector>
#include "skity/render/canvas.hpp"

namespace serval {
namespace svg {
namespace skity {

// Preserve the caller's absolute matrix semantics while recording in another
// coordinate space, or replaying under an offset/filter. This also preserves
// ResetMatrix used by non-scaling strokes; a plain display-list replay would
// otherwise discard the filter input's transform.
class SrSkityMatrixCanvas final : public ::skity::Canvas {
 public:
  SrSkityMatrixCanvas(::skity::Canvas* target, const ::skity::Matrix& prefix,
                      const ::skity::Matrix& initial,
                      const ::skity::Matrix& target_layer)
      : target_(target),
        prefix_(prefix),
        save_offset_(target->GetSaveCount() - 1) {
    PushLayerInverse(target_layer);
    SetMatrix(initial);
  }

 protected:
  void OnClipRect(const ::skity::Rect& rect, ClipOp op) override {
    target_->ClipRect(rect, op);
  }
  void OnClipRRect(const ::skity::RRect& rect, ClipOp op) override {
    target_->ClipRRect(rect, op);
  }
  void OnClipPath(const ::skity::Path& path, ClipOp op) override {
    target_->ClipPath(path, op);
  }
  void OnDrawPath(const ::skity::Path& path,
                  const ::skity::Paint& paint) override {
    target_->DrawPath(path, paint);
  }
  void OnSaveLayer(const ::skity::Rect& bounds,
                   const ::skity::Paint& paint) override {
    const auto layer = target_->GetTotalMatrix();
    target_->SaveLayer(bounds, paint);
    PushLayerInverse(layer);
  }
  void OnDrawBlob(const ::skity::TextBlob* blob, float x, float y,
                  const ::skity::Paint& paint) override {
    target_->DrawTextBlob(blob, x, y, paint);
  }
  void OnDrawImageRect(std::shared_ptr<::skity::Image> image,
                       const ::skity::Rect& src, const ::skity::Rect& dst,
                       const ::skity::SamplingOptions& sampling,
                       const ::skity::Paint* paint) override {
    target_->DrawImageRect(image, src, dst, sampling, paint);
  }
  void OnDrawGlyphs(uint32_t count, const ::skity::GlyphID glyphs[],
                    const float xs[], const float ys[],
                    const ::skity::Font& font,
                    const ::skity::Paint& paint) override {
    target_->DrawGlyphs(count, glyphs, xs, ys, font, paint);
  }
  void OnDrawPaint(const ::skity::Paint& paint) override {
    target_->DrawPaint(paint);
  }
  void OnSave() override {
    target_->Save();
    layer_inverses_.push_back(layer_inverses_.back());
  }
  void OnRestore() override {
    target_->Restore();
    layer_inverses_.pop_back();
  }
  void OnRestoreToCount(int count) override {
    target_->RestoreToCount(save_offset_ + count);
  }
  void OnTranslate(float x, float y) override { target_->Translate(x, y); }
  void OnScale(float x, float y) override { target_->Scale(x, y); }
  void OnRotate(float angle) override { target_->Rotate(angle); }
  void OnRotate(float angle, float x, float y) override {
    target_->Rotate(angle, x, y);
  }
  void OnSkew(float x, float y) override { target_->Skew(x, y); }
  void OnConcat(const ::skity::Matrix& matrix) override {
    target_->Concat(matrix);
  }
  void OnSetMatrix(const ::skity::Matrix&) override {
    // SetMatrix in Skity is relative to the current SaveLayer, whereas
    // GetTotalMatrix includes the layer's world transform. Convert through
    // both layer origins instead of applying the replay prefix twice.
    target_->SetMatrix(layer_inverses_.back() * prefix_ * GetTotalMatrix());
  }
  void OnResetMatrix() override { OnSetMatrix(::skity::Matrix()); }
  void OnFlush() override { target_->Flush(); }
  uint32_t OnGetWidth() const override { return target_->Width(); }
  uint32_t OnGetHeight() const override { return target_->Height(); }

 private:
  void PushLayerInverse(const ::skity::Matrix& layer) {
    ::skity::Matrix inverse;
    if (!layer.Invert(&inverse))
      inverse = ::skity::Matrix::Scale(0, 0);
    layer_inverses_.push_back(inverse);
  }
  ::skity::Canvas* target_;
  ::skity::Matrix prefix_;
  int save_offset_;
  std::vector<::skity::Matrix> layer_inverses_;
};

}  // namespace skity
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_SKITY_SRSKITYMATRIXCANVAS_H_
