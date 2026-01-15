// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGTYPES_H_
#define SVG_INCLUDE_ELEMENT_SRSVGTYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdlib.h"
#include "string.h"

#define NSVG_RGB(r, g, b)                                     \
  (((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b) | \
   ((uint32_t)0xff << 24))

#define NSVG_RGBA(r, g, b, a)                                 \
  (((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b) | \
   ((uint32_t)a << 24))

typedef enum SrSVGUnits {
  SR_SVG_UNITS_UNKNOWN,
  SR_SVG_UNITS_NUMBER,
  SR_SVG_UNITS_PERCENTAGE,
  SR_SVG_UNITS_EM,
  SR_SVG_UNITS_EX,
  SR_SVG_UNITS_PX,
  SR_SVG_UNITS_CM,
  SR_SVG_UNITS_MM,
  SR_SVG_UNITS_IN,
  SR_SVG_UNITS_PT,
  SR_SVG_UNITS_PC,
} SrSVGUnits;

typedef enum SrTextAnchor {
  SR_SVG_TEXT_ANCHOR_START,
  SR_SVG_TEXT_ANCHOR_MIDDLE,
  SR_SVG_TEXT_ANCHOR_END,
} SrTextAnchor;

typedef enum GradientSpread { pad, reflect, repeat } GradientSpread;

typedef enum SrSVGFillRule {
  SR_SVG_FILL = 0,
  SR_SVG_EO_FILL = 1
} SrSVGFillRule;

typedef enum SrSVGStrokeJoin {
  SR_SVG_STROKE_JOIN_MITER = 0,
  SR_SVG_STROKE_JOIN_ROUND,
  SR_SVG_STROKE_JOIN_BEVEL,
} SrSVGStrokeJoin;

typedef enum SrSVGStrokeCap {
  SR_SVG_STROKE_CAP_BUTT = 0,
  SR_SVG_STROKE_CAP_ROUND,
  SR_SVG_STROKE_CAP_SQUARE,
} SrSVGStrokeCap;

typedef enum SrPathOps : uint8_t {
  SPO_MOVE_TO = 0,
  SPO_LINE_TO = 1,
  SPO_CUBIC_BEZ = 2,
  SPO_QUAD_ARC = 3,
  SPO_ELLIPTICAL_ARC = 4,
  SPO_CLOSE = 5,
} SrPathOps;

typedef struct SrSVGBox {
  float left;
  float top;
  float width;
  float height;
} SrSVGBox;

typedef struct SrTextStyle {
  uint32_t color;
  float font_size;
} SrTextStyle;

typedef struct SrParagraphStyle {
  SrTextAnchor text_anchor;
} SrParagraphStyle;

typedef struct SrSVGRenderContext {
  float width;
  float height;
  float dpi;
  float font_size;
  void* id_mapper;
  SrSVGBox view_port;
  SrSVGBox view_box;
} SrSVGRenderContext;

typedef enum SrSVGLengthType {
  SR_SVG_LENGTH_TYPE_HORIZONTAL,
  SR_SVG_LENGTH_TYPE_VERTICAL,
  SR_SVG_LENGTH_TYPE_OTHER,
  SR_SVG_LENGTH_TYPE_NUMERIC
} SrSVGLengthType;

typedef enum SrSVGObjectBoundingBoxUnitType {
  SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE = 0,
  SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX = 1
} SrSVGObjectBoundingBoxUnitType;

typedef struct SrSVGLength {
  float value;
  SrSVGUnits unit;
} SrSVGLength;

typedef struct SrPolygon {
  float* points;
  uint32_t n_points;
  uint32_t capacity;
} SrPolygon;

typedef struct SrPath {
  uint8_t* ops;
  uint32_t n_ops;
  uint64_t c_ops;
  float* args;
  uint32_t n_args;
  uint64_t c_args;
} SrPathData;

typedef enum SrSVGPaintType {
  SERVAL_PAINT_NONE,
  SERVAL_PAINT_COLOR,
  SERVAL_PAINT_IRI
} SrSVGPaintType;

typedef enum SrSVGColorType {
  SERVAL_CURRENT_COLOR,
  SERVAL_COLOR
} SrSVGColorType;

typedef struct SrSVGColor {
  SrSVGColorType type;
  uint32_t color;
} SrSVGColor;

typedef struct SRSVGNamedColor {
  const char* name;
  unsigned int color;
} SRSVGNamedColor;

typedef struct SrStop {
  SrSVGLength offset;
  SrSVGLength stopOpacity;
  SrSVGColor stopColor;
} SrStop;

typedef struct SrSVGPaint {
  SrSVGPaintType type;
  union {
    const char* iri;
    SrSVGColor color;
  } content;
} SrSVGPaint;

typedef struct SRSVGStrokeState {
  SrSVGStrokeJoin stroke_line_join;
  SrSVGStrokeCap stroke_line_cap;
  float stroke_miter_limit;
  float stroke_dash_offset;
  float* dash_array;
  size_t dash_array_length;
} SRSVGStrokeState;

typedef struct SrSVGRenderState {
  SrSVGPaint* stroke;
  SrSVGPaint* fill;
  float opacity;
  float stroke_width;
  float stroke_opacity;
  float fill_opacity;
  SrSVGFillRule fill_rule;
  SRSVGStrokeState* stroke_state;
} SrSVGRenderState;

typedef enum SrSVGAlign {
  SR_SVG_AlIGN_NONE = 0,
  SR_SVG_AlIGN_MIN,
  SR_SVG_AlIGN_MID,
  SR_SVG_AlIGN_MAX,
} SrSVGAlign;

typedef enum SrSVGScale {
  SR_SVG_SCALE_NONE = 0,
  SR_SVG_SCALE_MEET,
  SR_SVG_SCALE_SLICE,
} SrSVGScale;

typedef struct SrSVGPreserveAspectRatio {
  SrSVGAlign align_x;
  SrSVGAlign align_y;
  SrSVGScale scale;
} SrSVGPreserveAspectRatio;

SrSVGUnits make_serval_length_unit(const char* c);
SrSVGLength make_serval_length(const char* value);
SrSVGBox make_serval_view_box(const char* value);
SrSVGPreserveAspectRatio make_preserve_aspect_radio(const char* value);
SrSVGPreserveAspectRatio make_default_preserve_aspect_radio();
void calculate_view_box_transform(
    const SrSVGBox* view_port, const SrSVGBox* view_box,
    SrSVGPreserveAspectRatio preserve_aspect_ratio, float* xform);
SrSVGPaint* make_serval_paint(const char* value);
void release_serval_paint(SrSVGPaint* paint);
SrSVGColor make_serval_color(const char* value);
float convert_serval_length_to_float(const SrSVGLength* length,
                                     SrSVGRenderContext* render_context,
                                     SrSVGLengthType type);
SrPolygon* make_serval_polygon(const char* value);
GradientSpread make_serval_spread_method(const char* value);
void release_serval_polygon_path(SrPolygon*);

#pragma region path
SrPathData* make_serval_path(const char*);
void release_serval_path(SrPathData*);
void add_circle_to_path(SrPathData* path, float cx, float cy, float r);
#pragma endregion

SrSVGStrokeCap resolve_stroke_line_cap(const char* value);
SrSVGStrokeJoin resolve_stroke_line_join(const char* value);

void xform_pre_scale(float* xform, float x_scale, float y_scale);
void xform_pre_translate(float* xform, float x_trans, float y_trans);
void xform_identity(float* xform);
void xform_set_translation(float* xform, float tx, float ty);
void xform_set_scale(float* xform, float sx, float sy);
void xform_set_rotation(float* xform, float d);
void xform_set_skewX(float* xform, float d);
void xform_set_skewY(float* xform, float d);
void xform_multiply(float* t, const float* s);
void xform_pre_multiply(float* t, const float* s);

#ifdef __cplusplus
}
#endif

#endif  // SVG_INCLUDE_ELEMENT_SRSVGTYPES_H_
