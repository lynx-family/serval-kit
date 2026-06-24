// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGPath.h"

#include <cstdlib>
#include <cstring>

#include "canvas/SrCanvas.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

namespace {

SrPathData* ClonePathData(const SrPathData* source) {
  if (!source) {
    return nullptr;
  }
  auto* clone = static_cast<SrPathData*>(std::calloc(1, sizeof(SrPathData)));
  if (!clone) {
    return nullptr;
  }
  clone->n_ops = source->n_ops;
  clone->c_ops = source->n_ops;
  clone->n_args = source->n_args;
  clone->c_args = source->n_args;
  if (source->n_ops > 0) {
    if (!source->ops) {
      release_serval_path(clone);
      return nullptr;
    }
    clone->ops =
        static_cast<uint8_t*>(std::malloc(source->n_ops * sizeof(uint8_t)));
    if (!clone->ops) {
      release_serval_path(clone);
      return nullptr;
    }
    std::memcpy(clone->ops, source->ops, source->n_ops * sizeof(uint8_t));
  }
  if (source->n_args > 0) {
    if (!source->args) {
      release_serval_path(clone);
      return nullptr;
    }
    clone->args =
        static_cast<float*>(std::malloc(source->n_args * sizeof(float)));
    if (!clone->args) {
      release_serval_path(clone);
      return nullptr;
    }
    std::memcpy(clone->args, source->args, source->n_args * sizeof(float));
  }
  return clone;
}

}  // namespace

void SrSVGPath::onDraw(canvas::SrCanvas* canvas,
                       SrSVGRenderContext& context) const {
  if (path_) {
    canvas->DrawPath(id_.c_str(), path_->ops, path_->n_ops, path_->args,
                     path_->n_args, render_state_);
  }
}

bool SrSVGPath::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "d") == 0) {
    release_serval_path(path_);
    path_ =
        value && value[0] ? make_serval_path(value, diagnostic_sink_) : nullptr;
    return true;
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

bool SrSVGPath::SetAnimatedPathData(const SrPathData* path_data) {
  SrPathData* cloned = ClonePathData(path_data);
  if (!cloned && path_data) {
    return false;
  }
  release_serval_path(path_);
  path_ = cloned;
  return true;
}

std::unique_ptr<canvas::Path> SrSVGPath::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context,
    bool include_transform) const {
  if (!path_) {
    return nullptr;
  }
  auto path = path_factory->CreatePath(path_->ops, path_->n_ops, path_->args,
                                       path_->n_args);
  if (path) {
    path->SetFillType(fill_rule_);
    if (include_transform) {
      float xform[6];
      ResolvedTransform(xform, *context, path_factory);
      path->Transform(xform);
    }
  }
  return path;
}

SrSVGPath::~SrSVGPath() {
  release_serval_path(path_);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
