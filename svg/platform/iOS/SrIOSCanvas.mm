// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <CoreImage/CoreImage.h>

#include <algorithm>
#include <cfloat>
#include <cmath>

#include "element/SrSVGTypes.h"
#include "parser/SrSVGArray.h"
#include "platform/iOS/SrIOSCanvas.h"
#include "platform/iOS/SrIOSColorUtils.h"
#include "utils/SrFloatComparison.h"
#include "utils/SrSVGArc.h"
#include "utils/SrSVGPatternUtils.h"

#ifdef __cplusplus
extern "C" {
#endif

static void SrSVGDrawArc(CGMutablePathRef path, float x, float y, float x1,
                         float y1, float a, float b, float theta,
                         bool large_arc, bool sweep) {
  const float args[9] = {x,  y, a, b, theta, (float)large_arc, (float)sweep,
                         x1, y1};
  SrSVGArc arc;
  if (!SrSVGArcToCubics(args, &arc))
    return;
  if (arc.line)
    CGPathAddLineToPoint(path, nullptr, x1, y1);
  for (size_t i = 0; i < arc.count; ++i) {
    const double* c = arc.cubics[i];
    CGPathAddCurveToPoint(path, nullptr, c[0], c[1], c[2], c[3], c[4], c[5]);
  }
}

#ifdef __cplusplus
}
#endif

namespace serval {
namespace svg {
namespace ios {

static bool SetLineDash(CGContextRef context, const float* values, size_t count,
                        CGFloat offset) {
  CGFloat local[32];
  parser::SrSVGArray<CGFloat> storage;
  CGFloat* dash = local;
  if (count > 32) {
    if (!storage.reserve(count))
      return false;
    dash = storage.data();
  }
  for (size_t i = 0; i < count; ++i)
    dash[i] = values[i];
  CGContextSetLineDash(context, offset, dash, count);
  return true;
}

static CGFloat SrMapVectorLength(CGAffineTransform transform, CGFloat x,
                                 CGFloat y) {
  CGFloat mapped_x = transform.a * x + transform.c * y;
  CGFloat mapped_y = transform.b * x + transform.d * y;
  return hypot(mapped_x, mapped_y);
}

static CIVector* SrColorVector(const std::vector<float>& values,
                               size_t offset) {
  return [CIVector vectorWithX:values[offset]
                             Y:values[offset + 1]
                             Z:values[offset + 2]
                             W:values[offset + 3]];
}

static CIImage* SrApplyColorMatrix(CIImage* image,
                                   const std::vector<float>& values) {
  CIFilter* filter = [CIFilter filterWithName:@"CIColorMatrix"];
  [filter setValue:image forKey:kCIInputImageKey];
  [filter setValue:SrColorVector(values, 0) forKey:@"inputRVector"];
  [filter setValue:SrColorVector(values, 5) forKey:@"inputGVector"];
  [filter setValue:SrColorVector(values, 10) forKey:@"inputBVector"];
  [filter setValue:SrColorVector(values, 15) forKey:@"inputAVector"];
  [filter setValue:[CIVector vectorWithX:values[4]
                                       Y:values[9]
                                       Z:values[14]
                                       W:values[19]]
            forKey:@"inputBiasVector"];
  return [filter valueForKey:kCIOutputImageKey];
}

static CIImage* SrApplyLuminanceToAlpha(CIImage* image) {
  CIFilter* filter = [CIFilter filterWithName:@"CIColorMatrix"];
  [filter setValue:image forKey:kCIInputImageKey];
  [filter setValue:[CIVector vectorWithX:0 Y:0 Z:0 W:0] forKey:@"inputRVector"];
  [filter setValue:[CIVector vectorWithX:0 Y:0 Z:0 W:0] forKey:@"inputGVector"];
  [filter setValue:[CIVector vectorWithX:0 Y:0 Z:0 W:0] forKey:@"inputBVector"];
  [filter setValue:[CIVector vectorWithX:0.2126 Y:0.7152 Z:0.0722 W:0]
            forKey:@"inputAVector"];
  [filter setValue:[CIVector vectorWithX:0 Y:0 Z:0 W:0]
            forKey:@"inputBiasVector"];
  return [filter valueForKey:kCIOutputImageKey];
}

static CIImage* SrApplyGaussianBlur(CIImage* image, CGFloat sigma_x,
                                    CGFloat sigma_y, CGRect extent) {
  if (sigma_x <= 0 && sigma_y <= 0) {
    return image;
  }
  CGFloat radius = fmax(sigma_x, sigma_y);
  if (radius <= 0) {
    return image;
  }

  CGFloat scale_x = sigma_x > 0 ? radius / sigma_x : 1;
  CGFloat scale_y = sigma_y > 0 ? radius / sigma_y : 1;
  CIImage* scaled = image;
  if (fabs(scale_x - 1) > 1e-5 || fabs(scale_y - 1) > 1e-5) {
    scaled = [scaled
        imageByApplyingTransform:CGAffineTransformMakeScale(scale_x, scale_y)];
  }

  CIFilter* filter = [CIFilter filterWithName:@"CIGaussianBlur"];
  [filter setValue:scaled forKey:kCIInputImageKey];
  [filter setValue:@(radius) forKey:kCIInputRadiusKey];
  CIImage* output = [filter valueForKey:kCIOutputImageKey];
  if (fabs(scale_x - 1) > 1e-5 || fabs(scale_y - 1) > 1e-5) {
    output = [output imageByApplyingTransform:CGAffineTransformMakeScale(
                                                  1 / scale_x, 1 / scale_y)];
  }
  return [output imageByCroppingToRect:extent];
}

static CGImageRef SrCreateFilteredImage(CGImageRef source_image,
                                        const canvas::SrFilterModel& filter,
                                        CGAffineTransform ctm) {
  if (!source_image) {
    return nullptr;
  }

  @autoreleasepool {
    CIImage* image = [CIImage imageWithCGImage:source_image];
    CGRect extent = [image extent];
    for (const auto& primitive : filter.primitives) {
      switch (primitive.type) {
        case canvas::SrFilterPrimitiveType::kGaussianBlur: {
          CGFloat sigma_x =
              SrMapVectorLength(ctm, primitive.std_deviation_x, 0);
          CGFloat sigma_y =
              SrMapVectorLength(ctm, 0, primitive.std_deviation_y);
          image = SrApplyGaussianBlur(image, sigma_x, sigma_y, extent);
          break;
        }
        case canvas::SrFilterPrimitiveType::kOffset: {
          CGFloat dx = ctm.a * primitive.dx + ctm.c * primitive.dy;
          CGFloat dy = ctm.b * primitive.dx + ctm.d * primitive.dy;
          image =
              [image imageByApplyingTransform:CGAffineTransformMakeTranslation(
                                                  dx, dy)];
          image = [image imageByCroppingToRect:extent];
          break;
        }
        case canvas::SrFilterPrimitiveType::kColorMatrix:
          if (primitive.color_matrix_type == "luminanceToAlpha") {
            image = SrApplyLuminanceToAlpha(image);
          } else {
            image = SrApplyColorMatrix(image, primitive.color_matrix_values);
          }
          break;
        case canvas::SrFilterPrimitiveType::kComposite:
        case canvas::SrFilterPrimitiveType::kBlend:
        case canvas::SrFilterPrimitiveType::kFlood:
          return CGImageRetain(source_image);
      }
    }

    CIContext* context = [CIContext contextWithOptions:nil];
    CGImageRef filtered_image = [context createCGImage:image fromRect:extent];
    return filtered_image ? filtered_image : CGImageRetain(source_image);
  }
}

static void MakeGradientColorsAndOffsets(const canvas::GradientModel& model,
                                         std::vector<CGFloat>& offsets,
                                         std::vector<CGFloat>& colors) {
  size_t stopSize = model.stops_.size();
  if (!stopSize) {
    // If there are no stops defined, we are to treat it as paint = 'none'
    return;
  }
  CGFloat lastOffset = -1.f;
  for (size_t i = 0; i < stopSize; ++i) {
    const SrStop& stop = model.stops_[i];
    // prepare position
    CGFloat offset = stop.offset.value;
    if (i == 0 || FloatsLargerOrEqual(offset, lastOffset)) {
      offsets.push_back(offset);
      lastOffset = offset;
    } else {
      // Each offset must be equal or greater than the last one.
      // If it doesn't we need to replace it with the previous value.
      offsets.push_back(lastOffset);
    }
    colors.push_back(SrIOSColorUtils::GetRedFromARGB(stop.stopColor.color));
    colors.push_back(SrIOSColorUtils::GetGreenFromARGB(stop.stopColor.color));
    colors.push_back(SrIOSColorUtils::GetBlueFromARGB(stop.stopColor.color));
    colors.push_back(SrIOSColorUtils::GetAlphaFromARGB(stop.stopColor.color,
                                                       stop.stopOpacity.value));
  }
  return;
}

static bool PrepareNonScalingStrokePath(CGContextRef cgContext,
                                        CGPathRef cgPath,
                                        CGPathRef* transformed_path,
                                        CGAffineTransform* ctm) {
  if (!cgContext || !cgPath || !transformed_path || !ctm) {
    return false;
  }
  *ctm = CGContextGetCTM(cgContext);
  CGFloat determinant = ctm->a * ctm->d - ctm->b * ctm->c;
  if (fabs(determinant) <= 1e-6) {
    return false;
  }
  *transformed_path = CGPathCreateCopyByTransformingPath(cgPath, ctm);
  if (!*transformed_path) {
    return false;
  }
  CGContextConcatCTM(cgContext, CGAffineTransformInvert(*ctm));
  return true;
}

static void DrawLinearGradient(
    CGContextRef cgContext, const canvas::LinearGradientModel& lgModel,
    CGPathRef geometryPath, SrSVGFillRule fillRule, bool isStroke,
    CGPathRef drawPath = nullptr,
    const CGAffineTransform* extraTransform = nullptr) {
  if (!cgContext || !geometryPath || lgModel.stop_size() == 0) {
    // If there are no stops defined, we are to treat it as paint = 'none'
    return;
  }
  CGContextSaveGState(cgContext);
  std::vector<CGFloat> offsets;
  std::vector<CGFloat> colors;
  MakeGradientColorsAndOffsets(lgModel, offsets, colors);
  CGColorSpaceRef cgColorSpace = CGColorSpaceCreateDeviceRGB();
  CGGradientRef gradientRef = CGGradientCreateWithColorComponents(
      cgColorSpace, colors.data(), offsets.data(), lgModel.stop_size());
  CGRect boundingBox = CGPathGetBoundingBox(geometryPath);
  auto form = lgModel.gradient_transformer_;
  CGAffineTransform gradient_transform = CGAffineTransformMake(
      form[0], form[1], form[2], form[3], form[4], form[5]);
  if (lgModel.obb_type_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    CGFloat width = CGRectGetWidth(boundingBox);
    CGFloat height = CGRectGetHeight(boundingBox);
    if (width != 0 && height != 0) {
      CGFloat minX = CGRectGetMinX(boundingBox);
      CGFloat minY = CGRectGetMinY(boundingBox);
      CGAffineTransform bboxToUser =
          CGAffineTransformMake(width, 0, 0, height, minX, minY);
      gradient_transform = CGAffineTransformConcat(
          CGAffineTransformConcat(CGAffineTransformInvert(bboxToUser),
                                  gradient_transform),
          bboxToUser);
    }
  }

  CGFloat x1 = lgModel.x1_;
  CGFloat y1 = lgModel.y1_;
  CGFloat x2 = lgModel.x2_;
  CGFloat y2 = lgModel.y2_;
  if (lgModel.obb_type_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    CGFloat width = CGRectGetWidth(boundingBox);
    CGFloat height = CGRectGetHeight(boundingBox);
    CGFloat minX = CGRectGetMinX(boundingBox);
    CGFloat minY = CGRectGetMinY(boundingBox);
    x1 = minX + x1 * width;
    y1 = minY + y1 * height;
    x2 = minX + x2 * width;
    y2 = minY + y2 * height;
  }
  CGPoint startPoint = CGPointMake(x1, y1);
  CGPoint endPoint = CGPointMake(x2, y2);
  CGContextAddPath(cgContext, drawPath ? drawPath : geometryPath);
  if (isStroke) {
    CGContextReplacePathWithStrokedPath(cgContext);
  }
  if (fillRule == SR_SVG_EO_FILL) {
    CGContextEOClip(cgContext);
  } else {
    CGContextClip(cgContext);
  }
  if (extraTransform) {
    CGContextConcatCTM(cgContext, *extraTransform);
  }
  CGContextConcatCTM(cgContext, gradient_transform);
  CGContextDrawLinearGradient(
      cgContext, gradientRef, startPoint, endPoint,
      kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
  CGGradientRelease(gradientRef);
  CGColorSpaceRelease(cgColorSpace);
  CGContextRestoreGState(cgContext);
}

static void DrawRadialGradient(
    CGContextRef cgContext, const canvas::RadialGradientModel& rgModel,
    CGPathRef geometryPath, SrSVGFillRule fillRule, bool isStroke,
    CGPathRef drawPath = nullptr,
    const CGAffineTransform* extraTransform = nullptr) {
  if (!cgContext || !geometryPath || rgModel.stop_size() == 0) {
    // If there are no stops defined, we are to treat it as paint = 'none'
    return;
  }
  CGContextSaveGState(cgContext);
  std::vector<CGFloat> offsets;
  std::vector<CGFloat> colors;
  MakeGradientColorsAndOffsets(rgModel, offsets, colors);
  CGColorSpaceRef cgColorSpace = CGColorSpaceCreateDeviceRGB();
  CGGradientRef gradientRef = CGGradientCreateWithColorComponents(
      cgColorSpace, colors.data(), offsets.data(), rgModel.stop_size());
  CGRect boundingBox = CGPathGetBoundingBox(geometryPath);

  auto form = rgModel.gradient_transformer_;
  CGAffineTransform gradient_transform = CGAffineTransformMake(
      form[0], form[1], form[2], form[3], form[4], form[5]);
  if (rgModel.obb_type_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    CGFloat width = CGRectGetWidth(boundingBox);
    CGFloat height = CGRectGetHeight(boundingBox);
    CGFloat maxSize = MAX(width, height);
    if (maxSize != 0) {
      CGFloat minX = CGRectGetMinX(boundingBox);
      CGFloat minY = CGRectGetMinY(boundingBox);
      CGAffineTransform bboxToUser =
          CGAffineTransformMake(maxSize, 0, 0, maxSize, minX, minY);
      gradient_transform = CGAffineTransformConcat(
          CGAffineTransformConcat(CGAffineTransformInvert(bboxToUser),
                                  gradient_transform),
          bboxToUser);
    }
  }

  CGFloat minX = CGRectGetMinX(boundingBox);
  CGFloat minY = CGRectGetMinY(boundingBox);
  CGFloat width = CGRectGetWidth(boundingBox);
  CGFloat height = CGRectGetHeight(boundingBox);
  CGFloat fx = rgModel.fx_;
  CGFloat fy = rgModel.fy_;
  CGFloat cx = rgModel.cx_;
  CGFloat cy = rgModel.cy_;
  CGFloat r = rgModel.r_;
  CGFloat startRadius = 0.0;
  CGFloat endRadius = r;
  CGPoint startCenter = CGPointMake(fx, fy);
  CGPoint endCenter = CGPointMake(cx, cy);
  if (rgModel.obb_type_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    CGFloat maxSize = MAX(width, height);
    endRadius = r * maxSize;
    startCenter = CGPointMake(minX + fx * maxSize, minY + fy * maxSize);
    endCenter = CGPointMake(minX + cx * maxSize, minY + cy * maxSize);
  }
  CGContextAddPath(cgContext, drawPath ? drawPath : geometryPath);
  if (isStroke) {
    CGContextReplacePathWithStrokedPath(cgContext);
  }
  if (fillRule == SR_SVG_EO_FILL) {
    CGContextEOClip(cgContext);
  } else {
    CGContextClip(cgContext);
  }
  if (extraTransform) {
    CGContextConcatCTM(cgContext, *extraTransform);
  }
  CGContextConcatCTM(cgContext, gradient_transform);
  CGContextTranslateCTM(cgContext, minX, minY);
  if (width > height) {
    CGContextScaleCTM(cgContext, 1.f, height / width);
  } else {
    CGContextScaleCTM(cgContext, width / height, 1.f);
  }
  CGContextTranslateCTM(cgContext, -minX, -minY);
  CGContextDrawRadialGradient(
      cgContext, gradientRef, startCenter, startRadius, endCenter, endRadius,
      kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
  CGGradientRelease(gradientRef);
  CGColorSpaceRelease(cgColorSpace);
  CGContextRestoreGState(cgContext);
}

SrIOSCanvas::SrIOSCanvas(CGFloat width, CGFloat height)
    : path_factory_(std::make_unique<PathFactoryQuartz2D>()) {
  if (width < 1 || height < 1 ||
      !parser::SrSVGTraversalState::OffscreenPixels(width, height))
    return;
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  if (!colorSpace)
    return;
  _context = root_context_ = CGBitmapContextCreate(
      NULL, width, height, 8, static_cast<size_t>(width) * 4, colorSpace,
      kCGBitmapByteOrderDefault | kCGImageAlphaPremultipliedLast);
  CGColorSpaceRelease(colorSpace);
  if (!_context)
    return;
  // initialize the rendering context
  CGContextSetStrokeColorWithColor(_context, [UIColor clearColor].CGColor);
  CGContextSetFillColorWithColor(_context, [UIColor clearColor].CGColor);
}

SrIOSCanvas::SrIOSCanvas(CGContextRef context)
    : path_factory_(std::make_unique<PathFactoryQuartz2D>()) {
  CGContextRetain(context);
  _context = root_context_ = context;
  // initialize the fill color
  CGContextSetFillColorWithColor(_context, [UIColor clearColor].CGColor);
  CGContextSetStrokeColorWithColor(_context, [UIColor clearColor].CGColor);
}

SrIOSCanvas::SrIOSCanvas(CGContextRef context, ImageCallback image_callback)
    : path_factory_(std::make_unique<PathFactoryQuartz2D>()),
      image_callback_(image_callback) {
  CGContextRetain(context);
  _context = root_context_ = context;
  // initialize the fill color
  CGContextSetFillColorWithColor(_context, [UIColor clearColor].CGColor);
  CGContextSetStrokeColorWithColor(_context, [UIColor clearColor].CGColor);
}

SrIOSCanvas::~SrIOSCanvas() {
  for (size_t i = 0; i < filter_depth_; ++i) {
    auto& layer = filter_layers_[i];
    CGContextRelease(layer.layer_context);
    CGContextRelease(layer.saved_context);
  }
  for (size_t i = 0; i < mask_depth_; ++i) {
    if (mask_layers_[i].context)
      CGContextRelease(mask_layers_[i].context);
  }
  if (root_context_)
    CGContextRelease(root_context_);
}

void SrIOSCanvas::FillPath(CGMutablePathRef cgPath,
                           const SrSVGRenderState& renderState) {
  CGContextSaveGState(_context);
  CGContextSetAlpha(_context, renderState.fill_opacity);
  if (!renderState.fill) {
    // if fill is null, we should set fill color to black and apply fill opacity
    CGContextSetFillColorWithColor(_context, UIColor.blackColor.CGColor);
    CGContextAddPath(_context, cgPath);
    if (renderState.fill_rule == SR_SVG_FILL) {
      CGContextFillPath(_context);
    } else {
      CGContextEOFillPath(_context);
    }
  } else if (renderState.fill && renderState.fill->type == SERVAL_PAINT_COLOR) {
    UIColor* fill_color =
        SrIOSColorUtils::UIColorFromARGB(renderState.fill->content.color.color);
    CGContextSetFillColorWithColor(_context, fill_color.CGColor);
    CGContextAddPath(_context, cgPath);
    if (renderState.fill_rule == SR_SVG_FILL) {
      CGContextFillPath(_context);
    } else {
      CGContextEOFillPath(_context);
    }
  } else if (renderState.fill && renderState.fill->type == SERVAL_PAINT_IRI) {
    const char* iri = renderState.fill->content.iri;
    if (!iri || iri[0] == '\0') {
      CGContextRestoreGState(_context);
      return;
    }
    if (RenderPatternFill(cgPath, renderState, iri)) {
      CGContextRestoreGState(_context);
      return;
    }
    auto it1 = lg_models_.find(iri);
    if (it1 != lg_models_.end()) {
      const canvas::LinearGradientModel& lgModel = it1->second;
      DrawLinearGradient(_context, lgModel, cgPath, renderState.fill_rule,
                         false);
    }
    auto it2 = rg_models_.find(iri);
    if (it2 != rg_models_.end()) {
      const canvas::RadialGradientModel& rgModel = it2->second;
      DrawRadialGradient(_context, rgModel, cgPath, renderState.fill_rule,
                         false);
    }
  }
  CGContextRestoreGState(_context);
}

bool SrIOSCanvas::CalculatePathBounds(CGPathRef cgPath, SrSVGBox* bounds) {
  if (!cgPath || !bounds) {
    return false;
  }
  CGRect box = CGPathGetBoundingBox(cgPath);
  bounds->left = static_cast<float>(CGRectGetMinX(box));
  bounds->top = static_cast<float>(CGRectGetMinY(box));
  bounds->width = static_cast<float>(CGRectGetWidth(box));
  bounds->height = static_cast<float>(CGRectGetHeight(box));
  return true;
}

void SrIOSCanvas::RenderPatternTiles(
    const element::ResolvedPattern& resolved_pattern,
    const SrSVGBox& target_bounds) {

  SrSVGBox pattern_area = target_bounds;
  if (!element::IsIdentityTransform(resolved_pattern.pattern_transform)) {
    float inverse[6];
    if (!element::InvertAffineTransform(resolved_pattern.pattern_transform,
                                        inverse) ||
        !element::TryMapBounds(target_bounds, inverse, &pattern_area))
      return;
    Transform(resolved_pattern.pattern_transform);
  }

  float origin_x =
      resolved_pattern.x + std::floor((pattern_area.left - resolved_pattern.x) /
                                      resolved_pattern.width) *
                               resolved_pattern.width;
  float origin_y =
      resolved_pattern.y + std::floor((pattern_area.top - resolved_pattern.y) /
                                      resolved_pattern.height) *
                               resolved_pattern.height;
  float right = pattern_area.left + pattern_area.width;
  float bottom = pattern_area.top + pattern_area.height;
  element::PatternTileBudget budget(
      *current_render_context_, origin_x, origin_y, right, bottom,
      resolved_pattern.width, resolved_pattern.height);
  if (!budget.HasRemaining() ||
      !active_pattern_ids_.insert(resolved_pattern.id).second)
    return;
  const bool has_resolved_view_box =
      resolved_pattern.has_view_box &&
      FloatsLarger(resolved_pattern.view_box.width, 0.f) &&
      FloatsLarger(resolved_pattern.view_box.height, 0.f);
  const bool uses_object_bounding_box_content_units =
      resolved_pattern.pattern_content_units ==
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
  SrSVGRenderContext base_tile_context = *current_render_context_;
  // A pattern changes the content transform, but does not establish an SVG
  // viewport. Keep the caller's viewport/viewBox for percentage lengths.

  for (float step_y = origin_y; step_y < bottom && budget.HasRemaining();
       step_y += resolved_pattern.height) {
    for (float step_x = origin_x; step_x < right && budget.HasRemaining();
         step_x += resolved_pattern.width) {
      budget.Consume();
      Save();
      CGContextClipToRect(_context,
                          CGRectMake(step_x, step_y, resolved_pattern.width,
                                     resolved_pattern.height));

      SrSVGRenderContext tile_context = base_tile_context;
      if (has_resolved_view_box) {
        SrSVGBox tile_view_port{step_x, step_y, resolved_pattern.width,
                                resolved_pattern.height};
        float view_box_xform[6];
        calculate_view_box_transform(
            &tile_view_port, &resolved_pattern.view_box,
            resolved_pattern.preserve_aspect_ratio, view_box_xform);
        Transform(view_box_xform);
      } else {
        if (uses_object_bounding_box_content_units) {
          Translate(step_x, step_y);
          float scale_xform[6];
          xform_set_scale(scale_xform, target_bounds.width,
                          target_bounds.height);
          Transform(scale_xform);
        } else {
          Translate(step_x, step_y);
        }
      }

      auto* previous_render_context = current_render_context_;
      resolved_pattern.content_pattern->RenderContent(this, tile_context);
      current_render_context_ = previous_render_context;
      Restore();
    }
  }

  active_pattern_ids_.erase(resolved_pattern.id);
}

bool SrIOSCanvas::RenderPatternFill(CGPathRef cgPath,
                                    const SrSVGRenderState& render_state,
                                    const char* iri) {
  if (!current_render_context_ || !cgPath || !iri || iri[0] != '#' ||
      !element::IsPatternIri(iri, *current_render_context_)) {
    return false;
  }

  SrSVGBox bounds{0.f, 0.f, 0.f, 0.f};
  if (!CalculatePathBounds(cgPath, &bounds)) {
    return false;
  }
  element::ResolvedPattern resolved_pattern;
  if (!element::ResolvePatternFromIri(iri, *current_render_context_, bounds,
                                      active_pattern_ids_, &resolved_pattern)) {
    return false;
  }

  Save();
  CGContextAddPath(_context, cgPath);
  if (render_state.fill_rule == SR_SVG_EO_FILL) {
    CGContextEOClip(_context);
  } else {
    CGContextClip(_context);
  }
  RenderPatternTiles(resolved_pattern, bounds);
  Restore();
  return true;
}

CGPathRef SrIOSCanvas::CreateStrokeClipPath(
    CGPathRef cgPath, const SrSVGRenderState& render_state) {
  if (!cgPath || !FloatsLarger(render_state.stroke_width, 0.f)) {
    return nullptr;
  }

  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  CGContextRef temp_context = CGBitmapContextCreate(
      NULL, 1, 1, 8, 4, color_space,
      kCGBitmapByteOrderDefault | kCGImageAlphaPremultipliedLast);
  CGColorSpaceRelease(color_space);
  if (!temp_context) {
    return nullptr;
  }

  CGContextAddPath(temp_context, cgPath);
  CGContextSetLineWidth(temp_context, render_state.stroke_width);
  if (render_state.stroke_state) {
    switch (render_state.stroke_state->stroke_line_cap) {
      case SR_SVG_STROKE_CAP_ROUND:
        CGContextSetLineCap(temp_context, kCGLineCapRound);
        break;
      case SR_SVG_STROKE_CAP_SQUARE:
        CGContextSetLineCap(temp_context, kCGLineCapSquare);
        break;
      default:
        CGContextSetLineCap(temp_context, kCGLineCapButt);
        break;
    }
    switch (render_state.stroke_state->stroke_line_join) {
      case SR_SVG_STROKE_JOIN_ROUND:
        CGContextSetLineJoin(temp_context, kCGLineJoinRound);
        break;
      case SR_SVG_STROKE_JOIN_BEVEL:
        CGContextSetLineJoin(temp_context, kCGLineJoinBevel);
        break;
      default:
        CGContextSetLineJoin(temp_context, kCGLineJoinMiter);
        break;
    }
    CGContextSetMiterLimit(temp_context,
                           render_state.stroke_state->stroke_miter_limit);
    const float* stroke_dash_array = render_state.stroke_state->dash_array;
    size_t dash_count = render_state.stroke_state->dash_array_length;
    if (stroke_dash_array && dash_count > 0) {
      if (!SetLineDash(temp_context, stroke_dash_array, dash_count,
                       render_state.stroke_state->stroke_dash_offset)) {
        CGContextRelease(temp_context);
        return nullptr;
      }
    }
  }

  CGContextReplacePathWithStrokedPath(temp_context);
  CGPathRef stroked_path = CGContextCopyPath(temp_context);
  CGContextRelease(temp_context);
  return stroked_path;
}

bool SrIOSCanvas::RenderPatternStroke(CGPathRef cgPath,
                                      const SrSVGRenderState& render_state,
                                      const char* iri) {
  if (!current_render_context_ || !cgPath || !iri || iri[0] != '#' ||
      !render_state.stroke || !FloatsLarger(render_state.stroke_width, 0.f) ||
      !element::IsPatternIri(iri, *current_render_context_)) {
    return false;
  }

  SrSVGBox object_bounds{0.f, 0.f, 0.f, 0.f};
  if (!CalculatePathBounds(cgPath, &object_bounds)) {
    return false;
  }
  element::ResolvedPattern resolved_pattern;
  if (!element::ResolvePatternFromIri(iri, *current_render_context_,
                                      object_bounds, active_pattern_ids_,
                                      &resolved_pattern)) {
    return false;
  }

  CGAffineTransform ctm = CGContextGetCTM(_context);
  CGFloat determinant = ctm.a * ctm.d - ctm.b * ctm.c;
  const bool can_use_non_scaling_stroke =
      render_state.vector_effect == SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE &&
      fabs(determinant) > 1e-6;
  bool use_non_scaling_stroke = false;
  CGPathRef transformed_path = nullptr;
  CGPathRef stroke_source_path = cgPath;
  if (can_use_non_scaling_stroke) {
    transformed_path = CGPathCreateCopyByTransformingPath(cgPath, &ctm);
    if (transformed_path) {
      stroke_source_path = transformed_path;
      use_non_scaling_stroke = true;
    }
  }

  CGPathRef stroked_path =
      CreateStrokeClipPath(stroke_source_path, render_state);
  if (transformed_path) {
    CGPathRelease(transformed_path);
  }
  if (!stroked_path) {
    return false;
  }

  CGRect stroke_box = CGPathGetBoundingBox(stroked_path);
  SrSVGBox pattern_bounds{static_cast<float>(CGRectGetMinX(stroke_box)),
                          static_cast<float>(CGRectGetMinY(stroke_box)),
                          static_cast<float>(CGRectGetWidth(stroke_box)),
                          static_cast<float>(CGRectGetHeight(stroke_box))};
  if (use_non_scaling_stroke) {
    CGAffineTransform inverse_ctm = CGAffineTransformInvert(ctm);
    float inverse[6] = {
        static_cast<float>(inverse_ctm.a),  static_cast<float>(inverse_ctm.b),
        static_cast<float>(inverse_ctm.c),  static_cast<float>(inverse_ctm.d),
        static_cast<float>(inverse_ctm.tx), static_cast<float>(inverse_ctm.ty)};
    pattern_bounds = element::MapBounds(pattern_bounds, inverse);
  }

  Save();
  if (use_non_scaling_stroke) {
    CGContextConcatCTM(_context, CGAffineTransformInvert(ctm));
  }
  CGContextAddPath(_context, stroked_path);
  CGContextClip(_context);
  if (use_non_scaling_stroke) {
    CGContextConcatCTM(_context, ctm);
  }
  RenderPatternTiles(resolved_pattern, pattern_bounds);
  Restore();
  CGPathRelease(stroked_path);
  return true;
}

void SrIOSCanvas::StrokePath(CGMutablePathRef cgPath,
                             const SrSVGRenderState& renderState) {
  if (!renderState.stroke || renderState.stroke->type == SERVAL_PAINT_NONE) {
    return;
  }
  CGContextSaveGState(_context);
  CGContextSetAlpha(_context, renderState.stroke_opacity);
  if (renderState.stroke_width > 0) {
    CGContextSetLineWidth(_context, renderState.stroke_width);
  }
  if (renderState.stroke_state) {
    switch (renderState.stroke_state->stroke_line_cap) {
      case SR_SVG_STROKE_CAP_BUTT:
        CGContextSetLineCap(_context, kCGLineCapButt);
        break;
      case SR_SVG_STROKE_CAP_ROUND:
        CGContextSetLineCap(_context, kCGLineCapRound);
        break;
      case SR_SVG_STROKE_CAP_SQUARE:
        CGContextSetLineCap(_context, kCGLineCapSquare);
        break;
      default:
        break;
    }
    switch (renderState.stroke_state->stroke_line_join) {
      case SR_SVG_STROKE_JOIN_MITER:
        CGContextSetLineJoin(_context, kCGLineJoinMiter);
        break;
      case SR_SVG_STROKE_JOIN_ROUND:
        CGContextSetLineJoin(_context, kCGLineJoinRound);
        break;
      case SR_SVG_STROKE_JOIN_BEVEL:
        CGContextSetLineJoin(_context, kCGLineJoinBevel);
        break;
      default:
        break;
    }
    CGContextSetMiterLimit(_context,
                           renderState.stroke_state->stroke_miter_limit);
    const float* stroke_dash_array = renderState.stroke_state->dash_array;
    size_t dash_count = renderState.stroke_state->dash_array_length;
    if (stroke_dash_array && dash_count > 0) {
      if (!SetLineDash(_context, stroke_dash_array, dash_count,
                       renderState.stroke_state->stroke_dash_offset)) {
        CGContextRestoreGState(_context);
        return;
      }
    }
  }
  if (renderState.stroke->type == SERVAL_PAINT_COLOR) {
    CGPathRef stroke_path = cgPath;
    CGPathRef transformed_path = nullptr;
    if (renderState.vector_effect == SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE) {
      CGAffineTransform ctm = CGAffineTransformIdentity;
      if (PrepareNonScalingStrokePath(_context, cgPath, &transformed_path,
                                      &ctm)) {
        stroke_path = transformed_path;
      }
    }
    UIColor* stroke_color = SrIOSColorUtils::UIColorFromARGB(
        renderState.stroke->content.color.color);
    CGContextSetStrokeColorWithColor(_context, stroke_color.CGColor);
    CGContextAddPath(_context, stroke_path);
    CGContextStrokePath(_context);
    if (transformed_path) {
      CGPathRelease(transformed_path);
    }
  } else if (renderState.stroke->type == SERVAL_PAINT_IRI) {
    const char* iri = renderState.stroke->content.iri;
    if (!iri || iri[0] == '\0') {
      CGContextRestoreGState(_context);
      return;
    }
    if (RenderPatternStroke(cgPath, renderState, iri)) {
      CGContextRestoreGState(_context);
      return;
    }
    CGPathRef stroke_path = cgPath;
    CGPathRef transformed_path = nullptr;
    CGAffineTransform ctm = CGAffineTransformIdentity;
    const bool use_non_scaling_stroke =
        renderState.vector_effect == SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE &&
        PrepareNonScalingStrokePath(_context, cgPath, &transformed_path, &ctm);
    if (use_non_scaling_stroke) {
      stroke_path = transformed_path;
    }
    auto it1 = lg_models_.find(iri);
    if (it1 != lg_models_.end()) {
      const canvas::LinearGradientModel& lgModel = it1->second;
      const bool user_space_gradient =
          lgModel.obb_type_ != SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
      DrawLinearGradient(
          _context, lgModel, use_non_scaling_stroke ? stroke_path : cgPath,
          renderState.fill_rule, true, nullptr,
          (use_non_scaling_stroke && user_space_gradient) ? &ctm : nullptr);
    }
    auto it2 = rg_models_.find(iri);
    if (it2 != rg_models_.end()) {
      const canvas::RadialGradientModel& rgModel = it2->second;
      DrawRadialGradient(_context, rgModel, cgPath, renderState.fill_rule, true,
                         use_non_scaling_stroke ? stroke_path : nullptr,
                         use_non_scaling_stroke ? &ctm : nullptr);
    }
    if (transformed_path) {
      CGPathRelease(transformed_path);
    }
  }
  CGContextRestoreGState(_context);
}

void SrIOSCanvas::SetViewBox(float x, float y, float width, float height) {
  CGFloat colors[4] = {0.f, 0.f, 0.f, 0.f};
  CGColorSpaceRef colorSpaceRef = CGColorSpaceCreateDeviceRGB();
  CGColorRef transparentColorRef = CGColorCreate(colorSpaceRef, colors);
  CGContextSetFillColorWithColor(_context, transparentColorRef);
  CGContextClearRect(_context, CGRectMake(x, y, width, height));
  CGColorRelease(transparentColorRef);
  CGColorSpaceRelease(colorSpaceRef);
}

void SrIOSCanvas::DrawLine(const char* id, float x1, float y1, float x2,
                           float y2, const SrSVGRenderState& render_state) {
  CGContextSaveGState(_context);
  CGMutablePathRef cgLine = CGPathCreateMutable();
  CGPathMoveToPoint(cgLine, NULL, x1, y1);
  CGPathAddLineToPoint(cgLine, NULL, x2, y2);
  StrokePath(cgLine, render_state);
  CGPathRelease(cgLine);
  CGContextRestoreGState(_context);
}

void SrIOSCanvas::DrawRect(const char* id, float x, float y, float rx, float ry,
                           float width, float height,
                           const SrSVGRenderState& render_state) {
  CGMutablePathRef path = CGPathCreateMutable();
  if (rx < 0.00001f || ry < 0.0001f) {
    CGRect rect = (CGRect){x, y, width, height};
    CGPathAddRect(path, NULL, rect);
  } else {
    constexpr float kKappa90 = 0.5522847493f;
    CGPathMoveToPoint(path, NULL, x + rx, y);
    CGPathAddLineToPoint(path, NULL, x + width - rx, y);
    CGPathAddCurveToPoint(path, NULL, x + width - rx * (1 - kKappa90), y,
                          x + width, y + ry * (1 - kKappa90), x + width,
                          y + ry);
    CGPathAddLineToPoint(path, NULL, x + width, y + height - ry);
    CGPathAddCurveToPoint(path, NULL, x + width,
                          y + height - ry * (1 - kKappa90),
                          x + width - rx * (1 - kKappa90), y + height,
                          x + width - rx, y + height);
    CGPathAddLineToPoint(path, NULL, x + rx, y + height);
    CGPathAddCurveToPoint(path, NULL, x + rx * (1 - kKappa90), y + height, x,
                          y + height - ry * (1 - kKappa90), x, y + height - ry);
    CGPathAddLineToPoint(path, NULL, x, y + ry);
    CGPathAddCurveToPoint(path, NULL, x, y + ry * (1 - kKappa90),
                          x + rx * (1 - kKappa90), y, x + rx, y);
    CGPathCloseSubpath(path);
  }
  FillPath(path, render_state);
  StrokePath(path, render_state);
  CGPathRelease(path);
}

void SrIOSCanvas::DrawCircle(const char* id, float cx, float cy, float r,
                             const SrSVGRenderState& render_state) {
  CGContextSaveGState(_context);
  CGMutablePathRef cgCircle = CGPathCreateMutable();
  CGRect rect = CGRectMake(cx - r, cy - r, 2 * r, 2 * r);
  CGPathAddEllipseInRect(cgCircle, NULL, rect);
  FillPath(cgCircle, render_state);
  StrokePath(cgCircle, render_state);
  CGPathRelease(cgCircle);
  CGContextRestoreGState(_context);
}

void SrIOSCanvas::DrawEllipse(const char* id, float center_x, float center_y,
                              float radius_x, float radius_y,
                              const SrSVGRenderState& render_state) {
  CGContextSaveGState(_context);
  CGMutablePathRef cgEllipse = CGPathCreateMutable();
  CGRect rect = CGRectMake(center_x - radius_x, center_y - radius_y,
                           2 * radius_x, 2 * radius_y);
  CGPathAddEllipseInRect(cgEllipse, NULL, rect);
  FillPath(cgEllipse, render_state);
  StrokePath(cgEllipse, render_state);
  CGPathRelease(cgEllipse);
  CGContextRestoreGState(_context);
}

void SrIOSCanvas::DrawPolygon(const char* id, float* points, uint32_t n_points,
                              const SrSVGRenderState& render_state) {
  if (n_points > 1) {
    CGContextSaveGState(_context);
    CGMutablePathRef cgPolygon = CGPathCreateMutable();
    CGPathMoveToPoint(cgPolygon, NULL, points[0], points[1]);
    for (int i = 1; i < n_points; i++) {
      CGPathAddLineToPoint(cgPolygon, NULL, points[2 * i], points[2 * i + 1]);
    }
    CGPathCloseSubpath(cgPolygon);
    FillPath(cgPolygon, render_state);
    StrokePath(cgPolygon, render_state);
    CGPathRelease(cgPolygon);
    CGContextRestoreGState(_context);
  }
}

void SrIOSCanvas::DrawPolyline(const char*, float* points, uint32_t n_points,
                               const SrSVGRenderState& render_state) {
  if (n_points >= 1) {
    CGContextSaveGState(_context);
    CGMutablePathRef cgPolyline = CGPathCreateMutable();
    CGPathMoveToPoint(cgPolyline, NULL, points[0], points[1]);
    for (int i = 1; i < n_points; i++) {
      CGPathAddLineToPoint(cgPolyline, NULL, points[2 * i], points[2 * i + 1]);
    }
    FillPath(cgPolyline, render_state);
    StrokePath(cgPolyline, render_state);
    CGPathRelease(cgPolyline);
    CGContextRestoreGState(_context);
  }
}

void SrIOSCanvas::DrawPath(const char*, uint8_t ops[], uint32_t n_ops,
                           float args[], uint32_t n_args,
                           const SrSVGRenderState& render_state) {
  CGContextSaveGState(_context);
  CGMutablePathRef cgPath = CGPathCreateMutable();
  uint64_t iArg = 0;
  float x = .0f, y = .0f;
  float cp1x = .0f, cp1y = .0f, cp2x = .0f, cp2y = .0f;
  for (uint64_t i = 0; i < n_ops; i++) {
    switch (ops[i]) {
      case SPO_MOVE_TO:
        x = args[iArg++];
        y = args[iArg++];
        CGPathMoveToPoint(cgPath, NULL, x, y);
        break;
      case SPO_LINE_TO:
        x = args[iArg++];
        y = args[iArg++];
        CGPathAddLineToPoint(cgPath, NULL, x, y);
        break;
      case SPO_CUBIC_BEZ:
        cp1x = args[iArg++];
        cp1y = args[iArg++];
        cp2x = args[iArg++];
        cp2y = args[iArg++];
        x = args[iArg++];
        y = args[iArg++];
        CGPathAddCurveToPoint(cgPath, NULL, cp1x, cp1y, cp2x, cp2y, x, y);
        break;
      case SPO_QUAD_ARC:
        cp1x = args[iArg++];
        cp1y = args[iArg++];
        x = args[iArg++];
        y = args[iArg++];
        CGPathAddQuadCurveToPoint(cgPath, NULL, cp1x, cp1y, x, y);
        break;
      case SPO_ELLIPTICAL_ARC: {
        float c1x = args[iArg++], c1y = args[iArg++];
        float rx = args[iArg++];
        float ry = args[iArg++];
        float angle = args[iArg++];
        bool largeArc = fabs(args[iArg++]) > 1e-6 ? YES : NO;
        bool sweep = fabs(args[iArg++]) > 1e-6 ? YES : NO;
        float x = args[iArg++];
        float y = args[iArg++];
        SrSVGDrawArc(cgPath, c1x, c1y, x, y, rx, ry, angle, largeArc, sweep);
        break;
      }
      case SPO_CLOSE:
        CGPathCloseSubpath(cgPath);
        break;
      default:
        break;
    }
  }
  FillPath(cgPath, render_state);
  StrokePath(cgPath, render_state);
  CGPathRelease(cgPath);
  CGContextRestoreGState(_context);
}

void SrIOSCanvas::Save() {
  CGContextSaveGState(_context);
}

void SrIOSCanvas::Restore() {
  CGContextRestoreGState(_context);
}

void SrIOSCanvas::UpdateLinearGradient(
    const char* id, const float (&gradient_transform)[6],
    const GradientSpread spread, float x1, float x2, float y1, float y2,
    const std::vector<SrStop>& stops, SrSVGObjectBoundingBoxUnitType obb_type) {
  if (strlen(id)) {
    lg_models_[std::string("#") + id] = canvas::LinearGradientModel(
        spread, x1, x2, y1, y2, gradient_transform, stops, obb_type);
  }
}

void SrIOSCanvas::UpdateRadialGradient(
    const char* id, const float (&gradient_transform)[6],
    const GradientSpread spread, float cx, float cy, float r, float fx,
    float fy, const std::vector<SrStop>& stops,
    SrSVGObjectBoundingBoxUnitType obb_type) {
  if (strlen(id)) {
    rg_models_[std::string("#") + id] = canvas::RadialGradientModel(
        spread, cx, cy, r, fx, fy, gradient_transform, stops, obb_type);
  }
}

void SrIOSCanvas::DrawUse(const char* href, float x, float y, float width,
                          float height) {}

void SrIOSCanvas::DrawImage(
    const char* url, float x, float y, float width, float height,
    const SrSVGPreserveAspectRatio& preserve_aspect_radio, float opacity) {
  if (url) {
    NSString* href = [NSString stringWithUTF8String:url];
    UIImage* image = nil;
    if (!image && image_callback_) {
      image = image_callback_(href);
    }
    if (image) {
      const float image_width = static_cast<float>(image.size.width);
      const float image_height = static_cast<float>(image.size.height);
      if (!FloatsLarger(image_width, 0.f) || !FloatsLarger(image_height, 0.f)) {
        return;
      }
      if (!FloatsLarger(width, 0.f) || !FloatsLarger(height, 0.f)) {
        return;
      }
      float form[6];
      SrSVGBox view_port{x, y, width, height};
      SrSVGBox view_box{0, 0, image_width, image_height};
      calculate_view_box_transform(&view_port, &view_box, preserve_aspect_radio,
                                   form);
      CGContextSaveGState(_context);
      CGContextSetAlpha(_context,
                        static_cast<CGFloat>(ClampUnitFloat(opacity)));
      CGContextBeginTransparencyLayer(_context, NULL);
      CGAffineTransform box_transform = CGAffineTransformMake(
          form[0], form[1], form[2], form[3], form[4], form[5]);
      CGRect rect = CGRectMake(0, 0, static_cast<float>(image.size.width),
                               static_cast<float>(image.size.height));
      rect = CGRectApplyAffineTransform(rect, box_transform);
      rect =
          CGRectApplyAffineTransform(rect, CGAffineTransformMakeScale(1, -1));
      CGContextScaleCTM(_context, 1, -1);
      CGContextDrawImage(_context, rect, image.CGImage);
      CGContextEndTransparencyLayer(_context);
      CGContextRestoreGState(_context);
    }
  }
}

void SrIOSCanvas::Translate(float x, float y) {
  float translation[6];
  xform_set_translation(translation, x, y);
  Transform(translation);
}

void SrIOSCanvas::Transform(const float (&form)[6]) {
  CGAffineTransform transform = CGAffineTransformMake(
      form[0], form[1], form[2], form[3], form[4], form[5]);
  const auto combined =
      CGAffineTransformConcat(transform, CGContextGetCTM(_context));
  for (CGFloat value : {combined.a, combined.b, combined.c, combined.d,
                        combined.tx, combined.ty}) {
    if (!std::isfinite(value) || std::fabs(value) > FLT_MAX)
      return;
  }
  CGContextConcatCTM(_context, transform);
}

void SrIOSCanvas::ClipPath(canvas::Path* path, SrSVGFillRule clip_rule) {
  PathQuartz2D* path_q2d = static_cast<PathQuartz2D*>(path);
  if (path_q2d) {
    CGContextAddPath(_context, path_q2d->GetPath());
    if (clip_rule == SR_SVG_EO_FILL) {
      CGContextEOClip(_context);
    } else {
      CGContextClip(_context);
    }
  }
}

void SrIOSCanvas::SaveLayer(const SrSVGBox* bounds) {
  (void)bounds;
  // CGContextBeginTransparencyLayer creates an off-screen buffer for compositing.
  // All subsequent drawing is redirected to this buffer until RestoreLayer().
  // The transparency layer starts fully transparent by default.
  CGContextBeginTransparencyLayer(_context, NULL);
}

void SrIOSCanvas::RestoreLayer() {
  CGContextEndTransparencyLayer(_context);
}

void SrIOSCanvas::BeginOpacityLayer(const SrSVGBox* bounds, float opacity) {
  (void)bounds;
  CGContextSetAlpha(_context,
                    static_cast<CGFloat>(std::clamp(opacity, 0.f, 1.f)));
  CGContextBeginTransparencyLayer(_context, NULL);
}

void SrIOSCanvas::EndOpacityLayer() {
  RestoreLayer();
}

bool SrIOSCanvas::SupportsFilterModel(
    const canvas::SrFilterModel& filter) const {
  return canvas::SrSupportsLinearSourceGraphicFilterModel(filter);
}

static bool GetLayerSize(CGContextRef context, size_t* width, size_t* height) {
  constexpr size_t kMaxLayerPixels =
      parser::SrSVGTraversalState::kMaxOffscreenPixels;
  *width = CGBitmapContextGetWidth(context);
  *height = CGBitmapContextGetHeight(context);
  if (!*width || !*height) {
    CGRect bounds = CGContextConvertRectToDeviceSpace(
        context, CGContextGetClipBoundingBox(context));
    const CGFloat w = ceil(CGRectGetMaxX(bounds)),
                  h = ceil(CGRectGetMaxY(bounds));
    if (!std::isfinite(w) || !std::isfinite(h) || w < 1 || h < 1 ||
        w > kMaxLayerPixels || h > kMaxLayerPixels)
      return false;
    *width = static_cast<size_t>(w);
    *height = static_cast<size_t>(h);
  }
  return *width && *height && *width <= kMaxLayerPixels / *height;
}

bool SrIOSCanvas::GetOffscreenPixelCount(size_t* out) const {
  size_t width, height;
  *out = GetLayerSize(_context, &width, &height) ? width * height : 0;
  return true;
}

static bool GetInverseLayerTransform(CGContextRef context,
                                     CGAffineTransform* inverse) {
  const auto ctm = CGContextGetCTM(context);
  const double det = ctm.a * ctm.d - ctm.b * ctm.c;
  if (det == 0 || !std::isfinite(det))
    return false;
  const auto candidate = CGAffineTransformInvert(ctm);
  for (CGFloat value : {candidate.a, candidate.b, candidate.c, candidate.d,
                        candidate.tx, candidate.ty}) {
    if (!std::isfinite(value))
      return false;
  }
  *inverse = candidate;
  return true;
}

void SrIOSCanvas::BeginFilterLayer(const SrSVGBox* bounds,
                                   const canvas::SrFilterModel& filter) {
  if (filter_fallback_depth_ == 0 && TryBeginFilterLayer(bounds, filter)) {
    auto& layer = filter_layers_[filter_depth_ - 1];
    layer.owned_filter = std::make_unique<canvas::SrFilterModel>(filter);
    layer.filter = layer.owned_filter.get();
  } else {
    SaveLayer(bounds);
    ++filter_fallback_depth_;
  }
}

bool SrIOSCanvas::TryBeginFilterLayer(const SrSVGBox* bounds,
                                      const canvas::SrFilterModel& filter) {
  (void)bounds;
  if (filter_fallback_depth_ || filter_depth_ == filter_layers_.size())
    return false;
  FilterLayerState state;
  if (!GetLayerSize(_context, &state.width, &state.height) ||
      !GetInverseLayerTransform(_context, &state.inverse_ctm))
    return false;
  state.ctm = CGContextGetCTM(_context);
  state.filter = &filter;
  for (const auto& primitive : filter.primitives) {
    if (primitive.type != canvas::SrFilterPrimitiveType::kGaussianBlur)
      continue;
    const CGFloat x =
        SrMapVectorLength(state.ctm, primitive.std_deviation_x, 0);
    const CGFloat y =
        SrMapVectorLength(state.ctm, 0, primitive.std_deviation_y);
    const CGFloat sigma = fmax(x, y);
    const CGFloat expanded_width =
        std::ceil(state.width * (x > 0 ? sigma / x : 1) + 6 * sigma);
    const CGFloat expanded_height =
        std::ceil(state.height * (y > 0 ? sigma / y : 1) + 6 * sigma);
    if (!(sigma <= parser::SrSVGTraversalState::kMaxBlurSigma) ||
        !parser::SrSVGTraversalState::OffscreenPixels(expanded_width,
                                                      expanded_height))
      return false;
  }
  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  if (!color_space)
    return false;
  state.layer_context = CGBitmapContextCreate(
      NULL, state.width, state.height, 8, state.width * 4, color_space,
      kCGBitmapByteOrderDefault | kCGImageAlphaPremultipliedLast);
  CGColorSpaceRelease(color_space);
  if (!state.layer_context)
    return false;
  CGContextConcatCTM(state.layer_context, state.ctm);
  state.saved_context = CGContextRetain(_context);
  _context = state.layer_context;
  filter_layers_[filter_depth_++] = std::move(state);
  return true;
}

void SrIOSCanvas::EndFilterLayer() {
  if (filter_fallback_depth_) {
    --filter_fallback_depth_;
    RestoreLayer();
    return;
  }
  if (!filter_depth_)
    return;
  FilterLayerState state = std::move(filter_layers_[--filter_depth_]);
  _context = state.saved_context;
  CGImageRef layer_image = CGBitmapContextCreateImage(state.layer_context);
  CGImageRef filtered_image =
      SrCreateFilteredImage(layer_image, *state.filter, state.ctm);
  if (filtered_image) {
    CGContextSaveGState(_context);
    CGContextConcatCTM(_context, state.inverse_ctm);
    CGContextDrawImage(_context, CGRectMake(0, 0, state.width, state.height),
                       filtered_image);
    CGContextRestoreGState(_context);
    CGImageRelease(filtered_image);
  }
  if (layer_image)
    CGImageRelease(layer_image);
  CGContextRelease(state.layer_context);
  CGContextRelease(state.saved_context);
}

void SrIOSCanvas::BeginMaskLayer(const SrSVGBox* bounds, bool is_luminance) {
  if (!TryBeginMaskLayer(bounds, is_luminance)) {
    ++mask_fallback_depth_;
    SaveLayer(bounds);
  }
}

bool SrIOSCanvas::TryBeginMaskLayer(const SrSVGBox* bounds, bool is_luminance) {
  if (mask_fallback_depth_ || mask_depth_ == mask_layers_.size())
    return false;
  mask_layers_[mask_depth_++] = {nullptr, nullptr, is_luminance};
  SaveLayer(bounds);
  return true;
}

void SrIOSCanvas::BeginMaskContentLayer() {
  TryBeginMaskContentLayer();
}

bool SrIOSCanvas::TryBeginMaskContentLayer() {
  if (mask_fallback_depth_ || !mask_depth_)
    return false;
  auto& layer = mask_layers_[mask_depth_ - 1];
  if (layer.context)
    return false;
  size_t w, h;
  CGAffineTransform inverse;
  if (!GetLayerSize(_context, &w, &h) ||
      !GetInverseLayerTransform(_context, &inverse))
    return false;
  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  if (!cs)
    return false;
  layer.context = CGBitmapContextCreate(
      NULL, w, h, 8, w * 4, cs,
      kCGBitmapByteOrderDefault | kCGImageAlphaPremultipliedLast);
  CGColorSpaceRelease(cs);
  if (!layer.context)
    return false;
  CGContextConcatCTM(layer.context, CGContextGetCTM(_context));
  layer.parent = _context;
  _context = layer.context;
  return true;
}

void SrIOSCanvas::EndMaskContentLayer() {
  if (mask_fallback_depth_ || !mask_depth_)
    return;
  auto& layer = mask_layers_[mask_depth_ - 1];
  if (!layer.context)
    return;
  if (layer.luminance)
    ApplyLuminanceMaskToAlpha();
  _context = layer.parent;
  CGAffineTransform inverse;
  if (GetInverseLayerTransform(_context, &inverse)) {
    if (CGImageRef image = CGBitmapContextCreateImage(layer.context)) {
      CGContextSaveGState(_context);
      CGContextConcatCTM(_context, inverse);
      CGContextSetBlendMode(_context, kCGBlendModeDestinationIn);
      CGContextDrawImage(
          _context,
          CGRectMake(0, 0, CGImageGetWidth(image), CGImageGetHeight(image)),
          image);
      CGContextRestoreGState(_context);
      CGImageRelease(image);
    }
  }
  CGContextRelease(layer.context);
  layer.context = nullptr;
  layer.parent = nullptr;
}

void SrIOSCanvas::EndMaskLayer() {
  if (mask_fallback_depth_) {
    --mask_fallback_depth_;
  } else {
    if (!mask_depth_)
      return;
    EndMaskContentLayer();
    --mask_depth_;
  }
  RestoreLayer();
}

void SrIOSCanvas::ApplyLuminanceMaskToAlpha() {
  CGContextRef mask = mask_layers_[mask_depth_ - 1].context;
  if (!mask) {
    return;
  }
  size_t w = CGBitmapContextGetWidth(mask);
  size_t h = CGBitmapContextGetHeight(mask);
  uint8_t* data = static_cast<uint8_t*>(CGBitmapContextGetData(mask));
  if (!data || w == 0 || h == 0) {
    return;
  }

  // Pixel-level luminance-to-alpha conversion (premultiplied RGBA).
  // For premultiplied pixels: new_alpha = 0.2126*R + 0.7152*G + 0.0722*B
  // Set RGB to premultiplied white so later DestinationIn compositing only
  // depends on the derived alpha channel.
  size_t totalPixels = w * h;
  for (size_t i = 0; i < totalPixels; ++i) {
    size_t idx = i * 4;
    uint8_t r = data[idx + 0];
    uint8_t g = data[idx + 1];
    uint8_t b = data[idx + 2];
    float newAlpha = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    uint8_t na = static_cast<uint8_t>(newAlpha > 255.f ? 255 : newAlpha);
    data[idx + 0] = na;
    data[idx + 1] = na;
    data[idx + 2] = na;
    data[idx + 3] = na;
  }
}

canvas::PathFactory* SrIOSCanvas::PathFactory() {
  return path_factory_.get();
}

PathQuartz2D::~PathQuartz2D() {
  CGPathRelease(path_);
}

void PathQuartz2D::AddPath(Path* path) {
  PathQuartz2D* path_q2d = static_cast<PathQuartz2D*>(path);
  if (path_q2d) {
    CGPathAddPath(path_, NULL, path_q2d->path_);
  }
}

std::unique_ptr<canvas::Path> PathQuartz2D::CreateTransformCopy(
    const float (&xform)[6]) const {
  CGAffineTransform transform = CGAffineTransformMake(
      xform[0], xform[1], xform[2], xform[3], xform[4], xform[5]);
  CGMutablePathRef path =
      CGPathCreateMutableCopyByTransformingPath(path_, &transform);
  auto ret = std::make_unique<PathQuartz2D>(path);
  CGPathRelease(path);
  return std::move(ret);
}

void PathQuartz2D::Transform(const float (&xform)[6]) {
  CGAffineTransform transform = CGAffineTransformMake(
      xform[0], xform[1], xform[2], xform[3], xform[4], xform[5]);
  CGMutablePathRef path =
      CGPathCreateMutableCopyByTransformingPath(path_, &transform);
  CGPathRelease(path_);
  const_cast<PathQuartz2D*>(this)->path_ = path;
}

SrSVGBox PathQuartz2D::GetBounds() const {
  CGRect bounds = CGPathGetBoundingBox(path_);
  return (SrSVGBox){.left = (float)bounds.origin.x,
                    .top = (float)bounds.origin.y,
                    .width = (float)bounds.size.width,
                    .height = (float)bounds.size.height};
}

CGPathRef PathQuartz2D::GetPath() const {
  return path_;
}

std::unique_ptr<canvas::Path> PathFactoryQuartz2D::CreateCircle(float cx,
                                                                float cy,
                                                                float r) {
  CGMutablePathRef path = CGPathCreateMutable();
  CGRect rect = CGRectMake(cx - r, cy - r, 2 * r, 2 * r);
  CGPathAddEllipseInRect(path, NULL, rect);
  auto ret = std::make_unique<PathQuartz2D>(path);
  CGPathRelease(path);
  return ret;
}

std::unique_ptr<canvas::Path> PathFactoryQuartz2D::CreateRect(
    float x, float y, float rx, float ry, float width, float height) {
  CGMutablePathRef path = CGPathCreateMutable();
  CGRect rect = CGRectMake(x, y, width, height);
  CGPathAddRoundedRect(path, NULL, rect, rx, ry);
  auto ret = std::make_unique<PathQuartz2D>(path);
  CGPathRelease(path);
  return ret;
}

std::unique_ptr<canvas::Path> PathFactoryQuartz2D::CreateMutable() {
  CGMutablePathRef path = CGPathCreateMutable();
  auto ret = std::make_unique<PathQuartz2D>(path);
  CGPathRelease(path);
  return ret;
}

void PathFactoryQuartz2D::Op(canvas::Path* path1, canvas::Path* path2,
                             canvas::OP type) {
  PathQuartz2D* path_q2d_1 = static_cast<PathQuartz2D*>(path1);
  PathQuartz2D* path_q2d_2 = static_cast<PathQuartz2D*>(path2);
  if (!path_q2d_1 || !path_q2d_2) {
    return;
  }
  switch (type) {
    case canvas::DIFFERENCE:
      // CoreGraphics does not support path difference natively.
      // Fallback to adding path, relying on Even-Odd fill rule for "holes".
      path_q2d_1->AddPath(path_q2d_2);
      break;
    case canvas::INTERSECT:
      // Intersection is hard to simulate with just AddPath.
      // But for Masking, usually we don't use Intersect.
      break;
    case canvas::UNION:
      path_q2d_1->AddPath(path_q2d_2);
      break;
    case canvas::XOR:
      // XOR behaves like Difference for "hole inside shape".
      // Fallback to adding path, relying on Even-Odd fill rule.
      path_q2d_1->AddPath(path_q2d_2);
      break;
    case canvas::REVERSE_DIFFERENCE:
      break;
    default:
      break;
  }
}

std::unique_ptr<canvas::Path> PathFactoryQuartz2D::CreatePath(uint8_t* ops,
                                                              uint64_t n_ops,
                                                              float* args,
                                                              uint64_t n_args) {
  CGMutablePathRef path = CGPathCreateMutable();
  uint64_t iArg = 0;
  float x = .0f, y = .0f;
  float cp1x = .0f, cp1y = .0f, cp2x = .0f, cp2y = .0f;
  for (uint64_t i = 0; i < n_ops; i++) {
    switch (ops[i]) {
      case SPO_MOVE_TO:
        x = args[iArg++];
        y = args[iArg++];
        CGPathMoveToPoint(path, NULL, x, y);
        break;
      case SPO_LINE_TO:
        x = args[iArg++];
        y = args[iArg++];
        CGPathAddLineToPoint(path, NULL, x, y);
        break;
      case SPO_CUBIC_BEZ:
        cp1x = args[iArg++];
        cp1y = args[iArg++];
        cp2x = args[iArg++];
        cp2y = args[iArg++];
        x = args[iArg++];
        y = args[iArg++];
        CGPathAddCurveToPoint(path, NULL, cp1x, cp1y, cp2x, cp2y, x, y);
        break;
      case SPO_QUAD_ARC:
        cp1x = args[iArg++];
        cp1y = args[iArg++];
        x = args[iArg++];
        y = args[iArg++];
        CGPathAddQuadCurveToPoint(path, NULL, cp1x, cp1y, x, y);
        break;
      case SPO_ELLIPTICAL_ARC: {
        float c1x = args[iArg++], c1y = args[iArg++];
        float rx = args[iArg++];
        float ry = args[iArg++];
        float angle = args[iArg++];
        bool largeArc = fabs(args[iArg++]) > 1e-6 ? YES : NO;
        bool sweep = fabs(args[iArg++]) > 1e-6 ? YES : NO;
        float x = args[iArg++];
        float y = args[iArg++];

        SrSVGDrawArc(path, c1x, c1y, x, y, rx, ry, angle, largeArc, sweep);
        break;
      }
      case SPO_CLOSE:
        CGPathCloseSubpath(path);
        break;
      default:
        break;
    }
  }
  std::unique_ptr<PathQuartz2D> ret = std::make_unique<PathQuartz2D>(path);
  CGPathRelease(path);
  return std::move(ret);
}

std::unique_ptr<canvas::Path> PathFactoryQuartz2D::CreateStrokePath(
    const canvas::Path* path, float width, SrSVGStrokeCap cap,
    SrSVGStrokeJoin join, float miter_limit) {
  const PathQuartz2D* path_q2d = static_cast<const PathQuartz2D*>(path);
  if (!path_q2d || !path_q2d->GetPath()) {
    return nullptr;
  }

  CGLineCap cgCap = kCGLineCapButt;
  if (cap == SR_SVG_STROKE_CAP_ROUND)
    cgCap = kCGLineCapRound;
  else if (cap == SR_SVG_STROKE_CAP_SQUARE)
    cgCap = kCGLineCapSquare;

  CGLineJoin cgJoin = kCGLineJoinMiter;
  if (join == SR_SVG_STROKE_JOIN_ROUND)
    cgJoin = kCGLineJoinRound;
  else if (join == SR_SVG_STROKE_JOIN_BEVEL)
    cgJoin = kCGLineJoinBevel;

  CGPathRef strokedPath = CGPathCreateCopyByStrokingPath(
      path_q2d->GetPath(), NULL, width, cgCap, cgJoin, miter_limit);

  if (!strokedPath) {
    return nullptr;
  }

  CGMutablePathRef mutable_path = CGPathCreateMutableCopy(strokedPath);
  CGPathRelease(strokedPath);
  if (!mutable_path) {
    return nullptr;
  }

  auto ret = std::make_unique<PathQuartz2D>(mutable_path);
  CGPathRelease(mutable_path);
  return ret;
}

}  // namespace ios
}  // namespace svg
}  // namespace serval
