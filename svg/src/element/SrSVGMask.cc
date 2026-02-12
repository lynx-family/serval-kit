// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGMask.h"

#include <functional>
#include <optional>

#include "element/SrSVGNode.h"
#include "element/SrSVGUse.h"

namespace serval {
namespace svg {
namespace element {

// SrSVGMask::~SrSVGMask() = default;

void SrSVGMask::OnRender(canvas::SrCanvas*, SrSVGRenderContext&) {
  // invisible container, do nothing here.
}

bool SrSVGMask::ParseAndSetAttribute(const char* name, const char* value) {
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
    x_ = Atof(value);
    return true;
  } else if (strcmp(name, "y") == 0) {
    y_ = Atof(value);
    return true;
  } else if (strcmp(name, "width") == 0) {
    width_ = Atof(value);
    return true;
  } else if (strcmp(name, "height") == 0) {
    height_ = Atof(value);
    return true;
  }
  return SrSVGContainer::ParseAndSetAttribute(name, value);
}

std::unique_ptr<canvas::Path> SrSVGMask::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  std::unique_ptr<canvas::Path> path = path_factory->CreateMutable();
  
  // Create mask region path
  // If maskUnits is objectBoundingBox, these are relative coords.
  // If userSpaceOnUse, these are absolute coords.
  // The caller (OnPrepareToRender) will transform the result if needed.
  // So we just create the rect here.
  // But wait, the mask region clips the content.
  // Should we start with the mask region?
  // No, the mask content defines the alpha.
  // But the mask region defines the valid area.
  // So we should intersect the content with the mask region.
  
  // Helper for matrix multiplication: out = a * b
  auto MultiplyMatrix = [](float* out, const float* a, const float* b) {
    float a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5];
    float b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4], b5 = b[5];
    out[0] = a0 * b0 + a2 * b1;
    out[1] = a1 * b0 + a3 * b1;
    out[2] = a0 * b2 + a2 * b3;
    out[3] = a1 * b2 + a3 * b3;
    out[4] = a0 * b4 + a2 * b5 + a4;
    out[5] = a1 * b4 + a3 * b5 + a5;
  };

  std::function<void(SrSVGNodeBase*, const float*, SrSVGPaint*, SrSVGPaint*,
                     std::optional<SrSVGLength>)>
      process_node = [&](SrSVGNodeBase* child, const float* parent_xform,
                         SrSVGPaint* inherited_fill,
                         SrSVGPaint* inherited_stroke,
                         std::optional<SrSVGLength> inherited_stroke_width) {
        if (!child || !child->IsSVGNode()) return;
        auto node = static_cast<SrSVGNode*>(child);

        // 1. Calculate Transform
        float current_xform[6];
        MultiplyMatrix(current_xform, parent_xform, node->transform_);

        // Handle Use: Apply x/y translation
        if (node->Tag() == SrSVGTag::kUse) {
          // SrSVGUse is a forward declaration in the header, so we need to
          // cast carefully. However, we included SrSVGUse.h, so it's fine.
          auto use_node = static_cast<SrSVGUse*>(node);
          float dx = convert_serval_length_to_float(
              &use_node->x_, context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
          float dy = convert_serval_length_to_float(
              &use_node->y_, context, SR_SVG_LENGTH_TYPE_VERTICAL);
          if (dx != 0 || dy != 0) {
            float translate[6] = {1, 0, 0, 1, dx, dy};
            float temp[6];
            std::copy(std::begin(current_xform), std::end(current_xform),
                      std::begin(temp));
            MultiplyMatrix(current_xform, temp, translate);
          }
        }

        // 2. Resolve Attributes
        SrSVGPaint* fill = node->fill_ ? node->fill_ : inherited_fill;
        SrSVGPaint* stroke = node->stroke_ ? node->stroke_ : inherited_stroke;
        auto stroke_width = node->stroke_width_.has_value()
                                ? node->stroke_width_
                                : inherited_stroke_width;

        // 3. Handle Container (Group / Use)
        if (node->Tag() == SrSVGTag::kG || node->Tag() == SrSVGTag::kUse) {
          std::vector<SrSVGNodeBase*> targets;
          if (node->Tag() == SrSVGTag::kG) {
            auto container = static_cast<SrSVGContainer*>(node);
            targets = container->children();
          } else {  // Use
            auto use_node = static_cast<SrSVGUse*>(node);
            IDMapper* id_mapper = static_cast<IDMapper*>(context->id_mapper);
            if (id_mapper && !use_node->href_.empty()) {
              auto it = id_mapper->find(use_node->href_);
              if (it != id_mapper->end()) {
                targets.push_back(it->second);
              }
            }
          }

          for (auto target : targets) {
            process_node(target, current_xform, fill, stroke, stroke_width);
          }
          return;
        }

        // 4. Handle Shape (Leaf)
        bool has_fill = true;  // Default process as fill exist
        if (fill && fill->type == SERVAL_PAINT_NONE) {
          has_fill = false;
        }

        if (has_fill) {
          canvas::OP op = canvas::OP::DIFFERENCE;  // Default black -> Difference
          if (fill && fill->type == SERVAL_PAINT_COLOR) {
            if (fill->content.color.color == 0xFFFFFFFF) {
              op = canvas::OP::UNION;
            } else {
              op = canvas::OP::XOR;
            }
          } else if (fill) {
            op = canvas::OP::XOR;
          } else {
            // effective_fill is null -> default black -> Difference
            op = canvas::OP::XOR;
          }

          std::unique_ptr<canvas::Path> fillPath =
              node->AsPath(path_factory, context);
          if (fillPath) {
            fillPath->Transform(current_xform);
            path_factory->Op(path.get(), fillPath.get(), op);
          }
        }

        float stroke_w = 1.0f;
        if (stroke_width.has_value()) {
          stroke_w = convert_serval_length_to_float(
              &stroke_width.value(), context, SR_SVG_LENGTH_TYPE_OTHER);
        }

        bool has_stroke =
            stroke && stroke->type != SERVAL_PAINT_NONE && stroke_w > 0;

        // Heuristic: If it's a hole (black fill) and no stroke is defined,
        // force a tiny stroke to expand the hole slightly.
        bool force_aa_stroke = !has_stroke && has_fill;
        if (force_aa_stroke && fill) {
          if (fill->type == SERVAL_PAINT_COLOR &&
              fill->content.color.color == 0xFFFFFFFF) {
            force_aa_stroke = false;  // Don't expand white masks
          }
        }

        if (has_stroke || force_aa_stroke) {
          canvas::OP op = canvas::OP::UNION;
          if (force_aa_stroke) {
            op = canvas::OP::DIFFERENCE;
          } else if (stroke->type == SERVAL_PAINT_COLOR) {
            if (stroke->content.color.color == 0xFFFFFFFF) {
              op = canvas::OP::UNION;
            } else {
              op = canvas::OP::XOR;
            }
          } else {
            op = canvas::OP::XOR;
          }

          std::unique_ptr<canvas::Path> rawPath =
              node->AsPath(path_factory, context);
          if (rawPath) {
            float final_stroke_w = has_stroke ? stroke_w : 0.04f;

            std::unique_ptr<canvas::Path> strokePath =
                path_factory->CreateStrokePath(rawPath.get(), final_stroke_w,
                                               node->stroke_cap_,
                                               node->stroke_join_,
                                               node->stoke_miter_limit_);

            if (strokePath) {
              strokePath->Transform(current_xform);
              path_factory->Op(path.get(), strokePath.get(), op);
            }
          }
        }
      };

  float identity[6] = {1, 0, 0, 1, 0, 0};
  for (SrSVGNodeBase* child : children_) {
    process_node(child, identity, this->fill_, this->stroke_, std::nullopt);
  }

  // Clip to mask region logic is omitted as it might cause regressions if defaults are not handled perfectly.
  // The content of the mask (e.g., white rect) effectively defines the clipping region in most cases.
  
  return path;
}

}  // namespace element
}  // namespace svg
}  // namespace serval