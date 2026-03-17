// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGMask.h"

#include <functional>
#include <string>

#include "canvas/SrCanvas.h"
#include "element/SrSVGNode.h"

namespace serval {
namespace svg {
namespace element {

static SrSVGMask* ResolveMaskNode(const SrSVGPaint* mask_paint,
                                  SrSVGRenderContext& context) {
  if (!mask_paint || mask_paint->type != SERVAL_PAINT_IRI ||
      !mask_paint->content.iri || strlen(mask_paint->content.iri) == 0) {
    return nullptr;
  }
  std::string id(mask_paint->content.iri + 1);
  IDMapper* nodes = static_cast<IDMapper*>(context.id_mapper);
  if (!nodes) {
    return nullptr;
  }
  auto it = nodes->find(id);
  if (it == nodes->end()) {
    return nullptr;
  }
  SrSVGNodeBase* node_base = it->second;
  if (!node_base || node_base->Tag() != SrSVGTag::kMask) {
    return nullptr;
  }
  return static_cast<SrSVGMask*>(node_base);
}

static void DrawWithMask(canvas::SrCanvas* canvas, canvas::MaskType mask_type,
                         const std::function<void()>& draw_content,
                         const std::function<void()>& draw_mask) {
  canvas->SaveLayer();
  canvas->SetBlendMode(canvas::BlendMode::kSrcOver);
  draw_content();
  canvas->SetBlendMode(canvas::BlendMode::kDstIn);
  canvas->BeginMaskMode(mask_type);
  draw_mask();
  canvas->EndMaskMode();
  canvas->RestoreLayer();
  canvas->SetBlendMode(canvas::BlendMode::kSrcOver);
}

static float ResolveLengthForBox(const SrSVGLength& len, float box_size) {
  if (len.unit == SR_SVG_UNITS_PERCENTAGE) {
    return len.value / 100.f * box_size;
  }
  return len.value;
}

static float ResolveCoordForMaskUnits(const SrSVGLength& len, float box_origin,
                                      float box_size, bool object_bbox_units) {
  if (object_bbox_units) {
    return box_origin + ResolveLengthForBox(len, box_size);
  }
  if (len.unit == SR_SVG_UNITS_PERCENTAGE) {
    return box_origin + ResolveLengthForBox(len, box_size);
  }
  return len.value;
}

static float ResolveSizeForMaskUnits(const SrSVGLength& len, float box_size,
                                     bool object_bbox_units) {
  if (object_bbox_units) {
    return ResolveLengthForBox(len, box_size);
  }
  if (len.unit == SR_SVG_UNITS_PERCENTAGE) {
    return ResolveLengthForBox(len, box_size);
  }
  return len.value;
}

bool SrSVGMask::ApplyToTargetIfPresent(
    canvas::SrCanvas* canvas, SrSVGRenderContext& context,
    const SrSVGNodeBase* target, const SrSVGPaint* mask_paint,
    const std::function<void()>& draw_content) {
  if (!canvas || !target || !canvas->SupportsMaskLayer()) {
    return false;
  }
  SrSVGMask* mask_node = ResolveMaskNode(mask_paint, context);
  if (!mask_node) {
    return false;
  }

  std::unique_ptr<canvas::Path> target_path =
      target->AsPath(canvas->PathFactory(), &context);
  SrSVGBox target_box = context.view_port;
  if (target_path) {
    target_box = target_path->GetBounds();
  }

  const bool object_bbox_units =
      mask_node->mask_units_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
  float region_x = ResolveCoordForMaskUnits(
      mask_node->x_, target_box.left, target_box.width, object_bbox_units);
  float region_y = ResolveCoordForMaskUnits(
      mask_node->y_, target_box.top, target_box.height, object_bbox_units);
  float region_w = ResolveSizeForMaskUnits(mask_node->width_, target_box.width,
                                           object_bbox_units);
  float region_h = ResolveSizeForMaskUnits(
      mask_node->height_, target_box.height, object_bbox_units);
  if (region_w < 0) {
    region_w = 0;
  }
  if (region_h < 0) {
    region_h = 0;
  }

  if (region_w == 0 || region_h == 0) {
    DrawWithMask(canvas, mask_node->mask_type_, draw_content, []() {});
    return true;
  }

  DrawWithMask(canvas, mask_node->mask_type_, draw_content, [&]() {
    canvas->Save();
    std::unique_ptr<canvas::Path> region_path =
        canvas->PathFactory()->CreateRect(region_x, region_y, 0, 0, region_w,
                                          region_h);
    if (region_path) {
      canvas->ClipPath(region_path.get(), SR_SVG_FILL);
    }
    if (mask_node->mask_content_units_ ==
        SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
      if (target_box.width != 0 && target_box.height != 0) {
        float xform[6] = {
            target_box.width, 0, 0, target_box.height, target_box.left,
            target_box.top};
        canvas->Transform(xform);
      }
    }
    mask_node->RenderMaskContent(canvas, context);
    canvas->Restore();
  });
  return true;
}

// SrSVGMask::~SrSVGMask() = default;

void SrSVGMask::OnRender(canvas::SrCanvas*, SrSVGRenderContext&) {
  // invisible container, do nothing here.
}

void SrSVGMask::RenderMaskContent(canvas::SrCanvas* canvas,
                                  SrSVGRenderContext& context) {
  SrSVGContainer::OnRender(canvas, context);
}

bool SrSVGMask::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "mask-type") == 0) {
    if (strcmp(value, "alpha") == 0) {
      mask_type_ = canvas::MaskType::kAlpha;
    } else {
      mask_type_ = canvas::MaskType::kLuminance;
    }
    return true;
  }
  if (strcmp(name, "maskUnits") == 0) {
    if (strcmp(value, "objectBoundingBox") == 0) {
      mask_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
      // Re-apply defaults if switching to OBB?
      // Usually defaults are applied if attribute is missing.
      // But if user sets maskUnits="userSpaceOnUse", the default values are still -0.1, -0.1, 1.2, 1.2?
      // No, for userSpaceOnUse, defaults are different.
      // But let's assume the user provides x/y/w/h if needed.
    } else {
      mask_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    }
    return true;
  } else if (strcmp(name, "maskContentUnits") == 0) {
    if (strcmp(value, "objectBoundingBox") == 0) {
      mask_content_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    } else {
      mask_content_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    }
    return true;
  } else if (strcmp(name, "x") == 0) {
    x_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "y") == 0) {
    y_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "width") == 0) {
    width_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "height") == 0) {
    height_ = make_serval_length(value);
    return true;
  }
  return SrSVGContainer::ParseAndSetAttribute(name, value);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
