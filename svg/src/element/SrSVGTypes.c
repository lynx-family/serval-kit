// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGTypes.h"

#if defined(_WIN32) || defined(_WIN64)
#define strtok_r strtok_s
#define _USE_MATH_DEFINES
#endif

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SRSVGNamedColor sr_svg__colors[] = {
    {"red", NSVG_RGB(255, 0, 0)},
    {"green", NSVG_RGB(0, 128, 0)},
    {"blue", NSVG_RGB(0, 0, 255)},
    {"yellow", NSVG_RGB(255, 255, 0)},
    {"cyan", NSVG_RGB(0, 255, 255)},
    {"magenta", NSVG_RGB(255, 0, 255)},
    {"black", NSVG_RGB(0, 0, 0)},
    {"grey", NSVG_RGB(128, 128, 128)},
    {"gray", NSVG_RGB(128, 128, 128)},
    {"white", NSVG_RGB(255, 255, 255)},
    {"aliceblue", NSVG_RGB(240, 248, 255)},
    {"antiquewhite", NSVG_RGB(250, 235, 215)},
    {"aqua", NSVG_RGB(0, 255, 255)},
    {"aquamarine", NSVG_RGB(127, 255, 212)},
    {"azure", NSVG_RGB(240, 255, 255)},
    {"beige", NSVG_RGB(245, 245, 220)},
    {"bisque", NSVG_RGB(255, 228, 196)},
    {"blanchedalmond", NSVG_RGB(255, 235, 205)},
    {"blueviolet", NSVG_RGB(138, 43, 226)},
    {"brown", NSVG_RGB(165, 42, 42)},
    {"burlywood", NSVG_RGB(222, 184, 135)},
    {"cadetblue", NSVG_RGB(95, 158, 160)},
    {"chartreuse", NSVG_RGB(127, 255, 0)},
    {"chocolate", NSVG_RGB(210, 105, 30)},
    {"coral", NSVG_RGB(255, 127, 80)},
    {"cornflowerblue", NSVG_RGB(100, 149, 237)},
    {"cornsilk", NSVG_RGB(255, 248, 220)},
    {"crimson", NSVG_RGB(220, 20, 60)},
    {"darkblue", NSVG_RGB(0, 0, 139)},
    {"darkcyan", NSVG_RGB(0, 139, 139)},
    {"darkgoldenrod", NSVG_RGB(184, 134, 11)},
    {"darkgray", NSVG_RGB(169, 169, 169)},
    {"darkgreen", NSVG_RGB(0, 100, 0)},
    {"darkgrey", NSVG_RGB(169, 169, 169)},
    {"darkkhaki", NSVG_RGB(189, 183, 107)},
    {"darkmagenta", NSVG_RGB(139, 0, 139)},
    {"darkolivegreen", NSVG_RGB(85, 107, 47)},
    {"darkorange", NSVG_RGB(255, 140, 0)},
    {"darkorchid", NSVG_RGB(153, 50, 204)},
    {"darkred", NSVG_RGB(139, 0, 0)},
    {"darksalmon", NSVG_RGB(233, 150, 122)},
    {"darkseagreen", NSVG_RGB(143, 188, 143)},
    {"darkslateblue", NSVG_RGB(72, 61, 139)},
    {"darkslategray", NSVG_RGB(47, 79, 79)},
    {"darkslategrey", NSVG_RGB(47, 79, 79)},
    {"darkturquoise", NSVG_RGB(0, 206, 209)},
    {"darkviolet", NSVG_RGB(148, 0, 211)},
    {"deeppink", NSVG_RGB(255, 20, 147)},
    {"deepskyblue", NSVG_RGB(0, 191, 255)},
    {"dimgray", NSVG_RGB(105, 105, 105)},
    {"dimgrey", NSVG_RGB(105, 105, 105)},
    {"dodgerblue", NSVG_RGB(30, 144, 255)},
    {"firebrick", NSVG_RGB(178, 34, 34)},
    {"floralwhite", NSVG_RGB(255, 250, 240)},
    {"forestgreen", NSVG_RGB(34, 139, 34)},
    {"fuchsia", NSVG_RGB(255, 0, 255)},
    {"gainsboro", NSVG_RGB(220, 220, 220)},
    {"ghostwhite", NSVG_RGB(248, 248, 255)},
    {"gold", NSVG_RGB(255, 215, 0)},
    {"goldenrod", NSVG_RGB(218, 165, 32)},
    {"greenyellow", NSVG_RGB(173, 255, 47)},
    {"honeydew", NSVG_RGB(240, 255, 240)},
    {"hotpink", NSVG_RGB(255, 105, 180)},
    {"indianred", NSVG_RGB(205, 92, 92)},
    {"indigo", NSVG_RGB(75, 0, 130)},
    {"ivory", NSVG_RGB(255, 255, 240)},
    {"khaki", NSVG_RGB(240, 230, 140)},
    {"lavender", NSVG_RGB(230, 230, 250)},
    {"lavenderblush", NSVG_RGB(255, 240, 245)},
    {"lawngreen", NSVG_RGB(124, 252, 0)},
    {"lemonchiffon", NSVG_RGB(255, 250, 205)},
    {"lightblue", NSVG_RGB(173, 216, 230)},
    {"lightcoral", NSVG_RGB(240, 128, 128)},
    {"lightcyan", NSVG_RGB(224, 255, 255)},
    {"lightgoldenrodyellow", NSVG_RGB(250, 250, 210)},
    {"lightgray", NSVG_RGB(211, 211, 211)},
    {"lightgreen", NSVG_RGB(144, 238, 144)},
    {"lightgrey", NSVG_RGB(211, 211, 211)},
    {"lightpink", NSVG_RGB(255, 182, 193)},
    {"lightsalmon", NSVG_RGB(255, 160, 122)},
    {"lightseagreen", NSVG_RGB(32, 178, 170)},
    {"lightskyblue", NSVG_RGB(135, 206, 250)},
    {"lightslategray", NSVG_RGB(119, 136, 153)},
    {"lightslategrey", NSVG_RGB(119, 136, 153)},
    {"lightsteelblue", NSVG_RGB(176, 196, 222)},
    {"lightyellow", NSVG_RGB(255, 255, 224)},
    {"lime", NSVG_RGB(0, 255, 0)},
    {"limegreen", NSVG_RGB(50, 205, 50)},
    {"linen", NSVG_RGB(250, 240, 230)},
    {"maroon", NSVG_RGB(128, 0, 0)},
    {"mediumaquamarine", NSVG_RGB(102, 205, 170)},
    {"mediumblue", NSVG_RGB(0, 0, 205)},
    {"mediumorchid", NSVG_RGB(186, 85, 211)},
    {"mediumpurple", NSVG_RGB(147, 112, 219)},
    {"mediumseagreen", NSVG_RGB(60, 179, 113)},
    {"mediumslateblue", NSVG_RGB(123, 104, 238)},
    {"mediumspringgreen", NSVG_RGB(0, 250, 154)},
    {"mediumturquoise", NSVG_RGB(72, 209, 204)},
    {"mediumvioletred", NSVG_RGB(199, 21, 133)},
    {"midnightblue", NSVG_RGB(25, 25, 112)},
    {"mintcream", NSVG_RGB(245, 255, 250)},
    {"mistyrose", NSVG_RGB(255, 228, 225)},
    {"moccasin", NSVG_RGB(255, 228, 181)},
    {"navajowhite", NSVG_RGB(255, 222, 173)},
    {"navy", NSVG_RGB(0, 0, 128)},
    {"oldlace", NSVG_RGB(253, 245, 230)},
    {"olive", NSVG_RGB(128, 128, 0)},
    {"olivedrab", NSVG_RGB(107, 142, 35)},
    {"orange", NSVG_RGB(255, 165, 0)},
    {"orangered", NSVG_RGB(255, 69, 0)},
    {"orchid", NSVG_RGB(218, 112, 214)},
    {"palegoldenrod", NSVG_RGB(238, 232, 170)},
    {"palegreen", NSVG_RGB(152, 251, 152)},
    {"paleturquoise", NSVG_RGB(175, 238, 238)},
    {"palevioletred", NSVG_RGB(219, 112, 147)},
    {"papayawhip", NSVG_RGB(255, 239, 213)},
    {"peachpuff", NSVG_RGB(255, 218, 185)},
    {"peru", NSVG_RGB(205, 133, 63)},
    {"pink", NSVG_RGB(255, 192, 203)},
    {"plum", NSVG_RGB(221, 160, 221)},
    {"powderblue", NSVG_RGB(176, 224, 230)},
    {"purple", NSVG_RGB(128, 0, 128)},
    {"rosybrown", NSVG_RGB(188, 143, 143)},
    {"royalblue", NSVG_RGB(65, 105, 225)},
    {"saddlebrown", NSVG_RGB(139, 69, 19)},
    {"salmon", NSVG_RGB(250, 128, 114)},
    {"sandybrown", NSVG_RGB(244, 164, 96)},
    {"seagreen", NSVG_RGB(46, 139, 87)},
    {"seashell", NSVG_RGB(255, 245, 238)},
    {"sienna", NSVG_RGB(160, 82, 45)},
    {"silver", NSVG_RGB(192, 192, 192)},
    {"skyblue", NSVG_RGB(135, 206, 235)},
    {"slateblue", NSVG_RGB(106, 90, 205)},
    {"slategray", NSVG_RGB(112, 128, 144)},
    {"slategrey", NSVG_RGB(112, 128, 144)},
    {"snow", NSVG_RGB(255, 250, 250)},
    {"springgreen", NSVG_RGB(0, 255, 127)},
    {"steelblue", NSVG_RGB(70, 130, 180)},
    {"tan", NSVG_RGB(210, 180, 140)},
    {"teal", NSVG_RGB(0, 128, 128)},
    {"thistle", NSVG_RGB(216, 191, 216)},
    {"tomato", NSVG_RGB(255, 99, 71)},
    {"turquoise", NSVG_RGB(64, 224, 208)},
    {"violet", NSVG_RGB(238, 130, 238)},
    {"wheat", NSVG_RGB(245, 222, 179)},
    {"whitesmoke", NSVG_RGB(245, 245, 245)},
    {"yellowgreen", NSVG_RGB(154, 205, 50)},
};

static void skip_sep(const char** s);
static uint32_t parse_color(const char* str);
static uint32_t parse_color_rgb(const char* str);
static uint32_t parse_color_rgba(const char* str, float* opacity);
static uint32_t parse_color_hex(const char* hex, float* opacity);

SrSVGUnits make_serval_length_unit(const char* c) {
  // todo(renzhongyue): support other units.
  if (strcmp(c, "px") == 0) {
    return SR_SVG_UNITS_PX;
  } else if (strcmp(c, "%") == 0) {
    return SR_SVG_UNITS_PERCENTAGE;
  }
  return SR_SVG_UNITS_NUMBER;
}

SrSVGLength make_serval_length(const char* value) {
  char* end;
  const double a = strtod(value, &end);
  if (*end) {
    return (SrSVGLength){.value = a, .unit = make_serval_length_unit(end)};
  }
  return (SrSVGLength){.value = a, .unit = SR_SVG_UNITS_NUMBER};
}

GradientSpread make_serval_spread_method(const char* value) {
  if (strcmp(value, "reflect") == 0) {
    return reflect;
  } else if (strcmp(value, "repeat") == 0) {
    return repeat;
  }
  return pad;
}

#define SERVAL_SVG_VIEW_BOX_DELIMITERS " ,;\t\n"
SrSVGBox make_serval_view_box(const char* value) {
  char* token;
  char* saved;
  int i = 0;
  float result[4] = {0};
  token = strtok_r((char*)value, SERVAL_SVG_VIEW_BOX_DELIMITERS, &saved);
  char* end;
  while (token != NULL && i < 4) {
    result[i] = (float)strtof(token, &end);
    token = strtok_r(NULL, SERVAL_SVG_VIEW_BOX_DELIMITERS, &saved);
    ++i;
  }
  return (SrSVGBox){.left = result[0],
                    .top = result[1],
                    .width = result[2],
                    .height = result[3]};
}
#undef SERVAL_SVG_VIEW_BOX_DELIMITERS

SrSVGPreserveAspectRatio make_preserve_aspect_radio(const char* value) {
  // xMidYMid is default value.
  SrSVGAlign align_x = SR_SVG_AlIGN_MID;
  SrSVGAlign align_y = SR_SVG_AlIGN_MID;
  // meet is default value.
  SrSVGScale scale = SR_SVG_SCALE_MEET;
  if (strstr(value, "none")) {
    scale = SR_SVG_SCALE_NONE;
  } else {
    // Parse X align
    if (strstr(value, "xMin")) {
      align_x = SR_SVG_AlIGN_MIN;
    } else if (strstr(value, "xMid")) {
      align_x = SR_SVG_AlIGN_MID;
    } else if (strstr(value, "xMax")) {
      align_x = SR_SVG_AlIGN_MAX;
    }
    // Parse X align
    if (strstr(value, "YMin")) {
      align_y = SR_SVG_AlIGN_MIN;
    } else if (strstr(value, "YMid")) {
      align_y = SR_SVG_AlIGN_MID;
    } else if (strstr(value, "YMax")) {
      align_y = SR_SVG_AlIGN_MAX;
    }
    // Parse meet(default) / slice
    scale = SR_SVG_SCALE_MEET;
    if (strstr(value, "slice")) {
      scale = SR_SVG_SCALE_SLICE;
    }
  }
  return (SrSVGPreserveAspectRatio){
      .align_x = align_x,
      .align_y = align_y,
      .scale = scale,
  };
}

SrSVGPreserveAspectRatio make_default_preserve_aspect_radio() {
  return make_preserve_aspect_radio("");
}

void calculate_view_box_transform(
    const SrSVGBox* view_port, const SrSVGBox* view_box,
    SrSVGPreserveAspectRatio preserve_aspect_ratio, float* xform) {
  xform_identity(xform);
  float x_scale = view_port->width / view_box->width;
  float y_scale = view_port->height / view_box->height;
  float x_offset = -view_box->left;
  float y_offset = -view_box->top;
  if (preserve_aspect_ratio.scale == SR_SVG_SCALE_NONE) {
    xform_pre_translate(xform, view_port->left, view_port->top);
    xform_pre_scale(xform, x_scale, y_scale);
    xform_pre_translate(xform, x_offset, y_offset);
    return;
  }
  float scale = 1.f;
  if (preserve_aspect_ratio.scale == SR_SVG_SCALE_SLICE) {
    scale = x_scale > y_scale ? x_scale : y_scale;
  } else if (preserve_aspect_ratio.scale == SR_SVG_SCALE_MEET) {
    scale = x_scale < y_scale ? x_scale : y_scale;
  }
  switch (preserve_aspect_ratio.align_x) {
    case SR_SVG_AlIGN_MID:
      x_offset -= (view_box->width - view_port->width / scale) / 2.f;
      break;
    case SR_SVG_AlIGN_MAX:
      x_offset -= (view_box->width - view_port->width / scale);
      break;
    default:
      break;
  }
  switch (preserve_aspect_ratio.align_y) {
    case SR_SVG_AlIGN_MID:
      y_offset -= (view_box->height - view_port->height / scale) / 2.f;
      break;
    case SR_SVG_AlIGN_MAX:
      y_offset -= (view_box->height - view_port->height / scale);
      break;
    default:
      break;
  }
  xform_pre_translate(xform, view_port->left, view_port->top);
  xform_pre_scale(xform, scale, scale);
  xform_pre_translate(xform, x_offset, y_offset);
}

void xform_pre_scale(float* xform, float x_scale, float y_scale) {
  float tmp_xform[6];
  xform_identity(tmp_xform);
  xform_set_scale(tmp_xform, x_scale, y_scale);
  xform_multiply(xform, tmp_xform);
}

void xform_pre_translate(float* xform, float x_trans, float y_trans) {
  float tmp_xform[6];
  xform_identity(tmp_xform);
  xform_set_translation(tmp_xform, x_trans, y_trans);
  xform_multiply(xform, tmp_xform);
}

/*
 *  | t0 t2 t4 |   | s0 s2 s4 |   | t0*s0+t2*s1 t0*s2+t2*s3 t0*s4+t2*s5+t4 |
 *  | t1 t3 t5 | * | s1 s3 s5 | = | t1*s0+t3*s1 t1*s2+t3*s3 t1*s4+t3*s5+t5 |
 *  | 0  0  1  |   | 0  0  1  |   | 0           0           1              |
 */
void xform_multiply(float* t, const float* s) {
  float tmp[6];
  tmp[0] = t[0] * s[0] + t[2] * s[1];
  tmp[2] = t[0] * s[2] + t[2] * s[3];
  tmp[1] = t[1] * s[0] + t[3] * s[1];
  tmp[3] = t[1] * s[2] + t[3] * s[3];
  tmp[4] = t[0] * s[4] + t[2] * s[5] + t[4];
  tmp[5] = t[1] * s[4] + t[3] * s[5] + t[5];
  memcpy(t, tmp, sizeof(float) * 6);
}

/*
 *  | s0 s2 s4 |   | t0 t2 t4 |   | s0*t0+s2*t1 s0*t2+s2*t3 s0*t4+s2*t5+s4 |
 *  | s1 s3 s5 | * | t1 t3 t5 | = | s1*t0+s3*t1 s1*t2+s3*t3 s1*t4+s3*t5+s5 |
 *  | 0  0  1  |   | 0  0  1  |   | 0           0           1              |
 */
void xform_pre_multiply(float* t, const float* s) {
  float tmp[6];
  tmp[0] = s[0] * t[0] + s[2] * t[1];
  tmp[2] = s[0] * t[2] + s[2] * t[3];
  tmp[1] = s[1] * t[0] + s[3] * t[1];
  tmp[3] = s[1] * t[2] + s[3] * t[3];
  tmp[4] = s[0] * t[4] + s[2] * t[5] + s[4];
  tmp[5] = s[1] * t[4] + s[3] * t[5] + s[5];
  memcpy(t, tmp, sizeof(float) * 6);
}

void xform_identity(float* xform) {
  xform[0] = 1.0f;
  xform[1] = 0.0f;
  xform[2] = 0.0f;
  xform[3] = 1.0f;
  xform[4] = 0.0f;
  xform[5] = 0.0f;
}

void xform_set_translation(float* xform, float tx, float ty) {
  xform[0] = 1.0f;
  xform[1] = 0.0f;
  xform[2] = 0.0f;
  xform[3] = 1.0f;
  xform[4] = tx;
  xform[5] = ty;
}

void xform_set_scale(float* xform, float sx, float sy) {
  xform[0] = sx;
  xform[1] = 0.0f;
  xform[2] = 0.0f;
  xform[3] = sy;
  xform[4] = 0.0f;
  xform[5] = 0.0f;
}

void xform_set_rotation(float* xform, float d) {
  float cos_value = cosf(d), sin_value = sinf(d);
  xform[0] = cos_value;
  xform[1] = sin_value;
  xform[2] = -sin_value;
  xform[3] = cos_value;
  xform[4] = 0.0f;
  xform[5] = 0.0f;
}

/*
 * | 1 tan(d) 0 |   |x|   | x + y * tan(d) |
 * | 0 1      0 | * |y| = | y              |
 * | 0 0      1 |   |z|   | z              |
 */
void xform_set_skewX(float* xform, float d) {
  xform[0] = 1.0f;
  xform[1] = 0.0f;
  xform[2] = tanf(d);
  xform[3] = 1.0f;
  xform[4] = 0.0f;
  xform[5] = 0.0f;
}

/*
 * | 1      0 0 |   |x|   | x              |
 * | tan(d) 1 0 | * |y| = | x * tan(d) + y |
 * | 0      0 1 |   |z|   | z              |
 */
void xform_set_skewY(float* xform, float d) {
  xform[0] = 1.0f;
  xform[1] = tanf(d);
  xform[2] = 0.0f;
  xform[3] = 1.0f;
  xform[4] = 0.0f;
  xform[5] = 0.0f;
}

#define SERVAL_SVG_LEFT_PAREN '('
#define SERVAL_SVG_RIGHT_PAREN ')'
static const char* extract_iri(const char* value) {
  char* start_ptr = strchr(value, SERVAL_SVG_LEFT_PAREN);
  if (!start_ptr) {
    // Start delimiter not found
    return NULL;
  }
  char* end_ptr = strchr(start_ptr + 1, SERVAL_SVG_RIGHT_PAREN);
  if (!end_ptr) {
    // End delimiter not found
    return NULL;
  }
  start_ptr++;  // move past '(' character
  end_ptr--;    // move before ')' character
  while (isspace(*start_ptr)) {
    start_ptr++;  // ignore leading whitespace
  }
  while (end_ptr > start_ptr && isspace(*end_ptr)) {
    end_ptr--;  // ignore trailing whitespace
  }

  const size_t result_size = (size_t)(end_ptr - start_ptr) + 1;
  char* result = malloc(result_size + 1);
  if (!result) {
    // Memory allocation failed
    return NULL;
  }
  memcpy(result, start_ptr, result_size);
  result[result_size] = '\0';
  return (const char*)result;
}
#undef SERVAL_SVG_LEFT_PAREN
#undef SERVAL_SVG_RIGHT_PAREN

SrSVGPaint* make_serval_paint(const char* value) {
  if (strcmp(value, "none") == 0) {
    SrSVGPaint* paint = malloc(sizeof(SrSVGPaint));
    *paint = (SrSVGPaint){.type = SERVAL_PAINT_NONE, .content = {0}};
    return paint;
  } else if (strncmp(value, "url(", 4) == 0) {
    SrSVGPaint* paint = malloc(sizeof(SrSVGPaint));
    const char* iri = extract_iri(value);
    *paint = (SrSVGPaint){.type = SERVAL_PAINT_IRI, .content.iri = iri};
    return paint;
  } else {
    SrSVGColor color = make_serval_color(value);
    SrSVGPaint* paint = malloc(sizeof(SrSVGPaint));
    *paint = (SrSVGPaint){.type = SERVAL_PAINT_COLOR, .content.color = color};
    return paint;
  }
}

void release_serval_paint(SrSVGPaint* paint) {
  if (paint) {
    if (paint->type == SERVAL_PAINT_IRI) {
      free((void*)paint->content.iri);
      paint->content.iri = NULL;
    }
  }
  free(paint);
}

SrSVGColor make_serval_color(const char* value) {
  if (strcmp(value, "currentColor") == 0) {
    SrSVGColor color = (SrSVGColor){.type = SERVAL_CURRENT_COLOR, .color = 0};
    return color;
  } else {
    uint32_t color_int = parse_color(value);
    SrSVGColor color = (SrSVGColor){.type = SERVAL_COLOR, .color = color_int};
    return color;
  }
}

static float length_size_for_type(SrSVGRenderContext* render_context,
                                  SrSVGLengthType type) {
  switch (type) {
    case SR_SVG_LENGTH_TYPE_HORIZONTAL:
      return render_context->view_box.width;
    case SR_SVG_LENGTH_TYPE_VERTICAL:
      return render_context->view_box.height;
    case SR_SVG_LENGTH_TYPE_OTHER:
      return M_SQRT1_2 * sqrt(render_context->view_box.width *
                                  render_context->view_box.width +
                              render_context->view_box.height *
                                  render_context->view_box.height);
    case SR_SVG_LENGTH_TYPE_NUMERIC:
      return 1.f;
  }
  return .0f;
}

float convert_serval_length_to_float(const SrSVGLength* length,
                                     SrSVGRenderContext* render_context,
                                     SrSVGLengthType type) {
  switch (length->unit) {
    case SR_SVG_UNITS_NUMBER:
    case SR_SVG_UNITS_UNKNOWN:
      return length->value;
    case SR_SVG_UNITS_PX:
      return length->value;
    case SR_SVG_UNITS_PT:
      return length->value / 72.0f * render_context->dpi;
    case SR_SVG_UNITS_PC:
      return length->value / 6.0f * render_context->dpi;
    case SR_SVG_UNITS_MM:
      return length->value / 25.4f * render_context->dpi;
    case SR_SVG_UNITS_CM:
      return length->value / 2.54f * render_context->dpi;
    case SR_SVG_UNITS_IN:
      return length->value * render_context->dpi;
    case SR_SVG_UNITS_EM:
      return length->value * render_context->font_size;
    case SR_SVG_UNITS_EX:
      return length->value * render_context->font_size * 0.52f;
    case SR_SVG_UNITS_PERCENTAGE:
      return length->value / 100.f * length_size_for_type(render_context, type);
  }
  return .0f;
}

static bool resize_polygon(SrPolygon* path) {  // resizing the points array.

  path->capacity = path->capacity ? path->capacity * 2 : 8;

  // prevent memory leak when realloc fails.

  float* new_points = realloc(path->points, path->capacity * 2 * sizeof(float));

  if (!new_points) {
    // realloc failed
    if (path->points) {
      free(path->points);
    }
    path->n_points = 0;
    path->capacity = 0;
    return false;
  }
  path->points = new_points;
  return true;
}

static bool add_point_to_polygon(SrPolygon* path, float x, float y) {
  if (path->n_points + 1 > path->capacity && !resize_polygon(path)) {
    return false;
  }
  path->points[2 * path->n_points] = x;
  path->points[2 * path->n_points + 1] = y;
  path->n_points++;
  return true;
}

static float next_number(const char** s) {
  const char* start = *s;
  uint32_t len = 0;
  // sign
  if (**s == '-' || **s == '+') {
    len++;
    (*s)++;
  }

  // integer
  while (**s && isdigit(**s)) {
    (*s)++;
    len++;
  }

  // decimal
  if (**s == '.') {
    len++;
    (*s)++;
    while (**s && isdigit(**s)) {
      (*s)++;
      len++;
    }
  }

  // exponent
  if ((**s == 'e' || **s == 'E') &&
      (isdigit((*s)[1]) || (*s)[1] == '+' || (*s)[1] == '-')) {
    (*s)++;
    len++;
    if (**s == '-' || **s == '+') {
      (*s)++;
      len++;
    }
  }
  while (**s && isdigit(**s)) {
    (*s)++;
    len++;
  }
  char number[len + 1];
  strncpy(number, start, len);
  number[len] = 0;
  char* error;
  return strtof(number, &error);
}

static void extract_path_args(const char** s, float args[], int n_args) {
  for (int i = 0; i < n_args; i++) {
    skip_sep(s);
    args[i] = next_number(s);
  }
}

static void add_command_to_path(SrPathData* path, SrPathOps ops) {
  if (path->n_ops + 1 > path->c_ops) {
    path->c_ops = path->c_ops ? path->c_ops * 2 : 8;
    SrPathOps* new_ops = realloc(path->ops, path->c_ops * sizeof(SrPathOps));
    if (!new_ops) {
      return;
    }
    path->ops = new_ops;
  }
  path->ops[path->n_ops++] = ops;
}

static void add_args_to_path(SrPathData* path, float args[], size_t len) {
  bool need_realloc = false;
  while (path->n_args + len > path->c_args) {
    need_realloc = true;
    path->c_args = path->c_args ? path->c_args * 2 : 8;
  }
  if (need_realloc) {
    float* new_args = realloc(path->args, path->c_args * sizeof(float));
    if (!new_args) {
      return;
    }
    path->args = new_args;
  }
  memcpy(path->args + path->n_args, args, len * sizeof(float));
  path->n_args += len;
}

SrPathData* make_serval_path(const char* value) {
  SrPathData* path = malloc(sizeof(SrPathData));
  memset(path, 0, sizeof(SrPathData));
  const char* s = value;
  float args[6] = {0};
  skip_sep(&s);
  if (s) {
    char cmd = 0, last_cmd = 0;
    float current_point_x = 0, current_point_y = 0, control_point_x = 0,
          control_point_y = 0;
    float sub_path_start_x = 0, sub_path_start_y = 0;
    while (*s) {
      if (isalpha(*s)) {
        cmd = last_cmd = *s++;
      } else {
        cmd = last_cmd;
      }
      switch (cmd) {
        case 'm':
        case 'M': {
          extract_path_args(&s, args, 2);
          if (cmd == 'm') {
            args[0] += current_point_x;
            args[1] += current_point_y;
            last_cmd = 'l';
          } else {
            last_cmd = 'L';
          }
          add_command_to_path(path, SPO_MOVE_TO);
          add_args_to_path(path, args, 2);
          sub_path_start_x = current_point_x = control_point_x = args[0];
          sub_path_start_y = current_point_y = control_point_y = args[1];
          break;
        }
        case 'l':
        case 'L':
          extract_path_args(&s, args, 2);
          if (cmd == 'l') {
            args[0] += current_point_x;
            args[1] += current_point_y;
          }
          add_command_to_path(path, SPO_LINE_TO);
          add_args_to_path(path, args, 2);
          current_point_x = control_point_x = args[0];
          current_point_y = control_point_y = args[1];
          last_cmd = cmd;
          break;
        case 'h':
        case 'H':
          extract_path_args(&s, args, 1);
          if (cmd == 'h') {
            args[0] += current_point_x;
          }
          args[1] = current_point_y;
          add_command_to_path(path, SPO_LINE_TO);
          add_args_to_path(path, args, 2);
          current_point_x = control_point_x = args[0];
          last_cmd = cmd;
          break;
        case 'v':
        case 'V':
          extract_path_args(&s, args + 1, 1);
          if (cmd == 'v') {
            args[1] += current_point_y;
          }
          args[0] = current_point_x;
          add_command_to_path(path, SPO_LINE_TO);
          add_args_to_path(path, args, 2);
          current_point_y = control_point_y = args[1];
          last_cmd = cmd;
          break;
        case 'c':
        case 'C':
          extract_path_args(&s, args, 6);
          if (cmd == 'c') {
            for (int i = 0; i < 3; i++) {
              args[2 * i] += current_point_x;
              args[2 * i + 1] += current_point_y;
            }
          }
          add_command_to_path(path, SPO_CUBIC_BEZ);
          add_args_to_path(path, args, 6);
          current_point_x = args[4];
          current_point_y = args[5];
          control_point_x = args[2];
          control_point_y = args[3];
          last_cmd = cmd;
          break;
        case 's':
        case 'S':
          extract_path_args(&s, args + 2, 4);
          if ('s' == cmd) {
            for (int i = 1; i < 3; i++) {
              args[2 * i] += current_point_x;
              args[2 * i + 1] += current_point_y;
            }
          }

          // If there is no previous command or if the previous command was not
          // an C, c, S or s, assume the first control point is coincident with
          // the current point.
          if (strchr("cCsS", last_cmd)) {
            args[0] = 2 * current_point_x - control_point_x;
            args[1] = 2 * current_point_y - control_point_y;
          } else {
            args[0] = current_point_x;
            args[1] = current_point_y;
          }

          add_command_to_path(path, SPO_CUBIC_BEZ);
          add_args_to_path(path, args, 6);
          control_point_x = args[2];
          control_point_y = args[3];
          current_point_x = args[4];
          current_point_y = args[5];
          last_cmd = cmd;
          break;
        case 'q':
        case 'Q':
          extract_path_args(&s, args, 4);
          if (cmd == 'q') {
            for (int i = 0; i < 2; i++) {
              args[2 * i] += current_point_x;
              args[2 * i + 1] += current_point_y;
            }
          }
          control_point_x = args[0];
          control_point_y = args[1];
          add_command_to_path(path, SPO_QUAD_ARC);
          add_args_to_path(path, args, 4);
          current_point_x = args[2];
          current_point_y = args[3];
          last_cmd = cmd;
          break;
        case 't':
        case 'T':
          extract_path_args(&s, args + 2, 2);
          if (cmd == 't') {
            args[2] += current_point_x;
            args[3] += current_point_y;
          }

          // The control point is assumed to be the reflection of the control
          // point on the previous command relative to the current point. (If
          // there is no previous command or if the previous command was not a
          // Q, q, T or t, assume the control point is coincident with the
          // current point.)
          if (strchr("tTqQ", last_cmd)) {
            control_point_x = 2 * current_point_x - control_point_x;
            control_point_y = 2 * current_point_y - control_point_y;
          } else {
            control_point_x = current_point_x;
            control_point_y = current_point_y;
          }
          args[0] = control_point_x;
          args[1] = control_point_y;
          add_command_to_path(path, SPO_QUAD_ARC);
          add_args_to_path(path, args, 4);
          current_point_x = args[2];
          current_point_y = args[3];
          last_cmd = cmd;
          break;
        case 'a':
        case 'A':
          extract_path_args(&s, args, 3);
          skip_sep(&s);
          float f_large_flag = *s++ == '1' ? 1.f : 0.f;
          skip_sep(&s);
          float f_sweep_flag = *s++ == '1' ? 1.f : 0.f;
          extract_path_args(&s, args + 3, 2);
          if (cmd == 'a') {
            args[3] += current_point_x;
            args[4] += current_point_y;
          }
          add_command_to_path(path, SPO_ELLIPTICAL_ARC);
          float cp[2] = {current_point_x, current_point_y};
          add_args_to_path(path, cp, 2);
          add_args_to_path(path, args, 3);
          add_args_to_path(path, &f_large_flag, 1);
          add_args_to_path(path, &f_sweep_flag, 1);
          float cur_point[2] = {args[3], args[4]};
          add_args_to_path(path, cur_point, 2);
          control_point_x = current_point_x = args[3];
          control_point_y = current_point_y = args[4];
          last_cmd = cmd;
          break;
        case 'z':
        case 'Z':
          current_point_x = control_point_x = sub_path_start_x;
          current_point_y = control_point_y = sub_path_start_y;
          add_command_to_path(path, SPO_CLOSE);
          break;
        default:
          break;
      }
      // next command
      skip_sep(&s);
    }
  }
  return path;
}

void release_serval_polygon_path(SrPolygon* path) {
  if (!path) {
    return;
  }
  if (path->points) {
    free(path->points);
  }
  free(path);
}

SrPolygon* make_serval_polygon(const char* value) {
  SrPolygon* polygon = malloc(sizeof(SrPolygon));
  *polygon = (SrPolygon){0};
  const char* s = value;
  while (*s) {
    float x = strtof(s, (char**)&s);
    skip_sep(&s);
    float y = strtof(s, (char**)&s);
    // resize failed, return an empty path;
    if (!add_point_to_polygon(polygon, x, y))
      return polygon;
    skip_sep(&s);
  }
  return polygon;
}

void release_serval_path(SrPathData* path) {
  if (path->args) {
    free(path->args);
    path->args = NULL;
  }
  if (path->ops) {
    free(path->ops);
    path->ops = NULL;
  }
  free(path);
}

#define CP_FACTOR 0.5519150244935105
// reference to https://spencermortensen.com/articles/bezier-circle/
void add_circle_to_path(SrPathData* path, float cx, float cy, float r) {
  add_command_to_path(path, SPO_MOVE_TO);
  add_args_to_path(path, (float[2]){cx + r, cy}, 2);
  add_command_to_path(path, SPO_CUBIC_BEZ);
  add_args_to_path(path,
                   (float[6]){cx - r * CP_FACTOR, cy + r, cx - r,
                              cy + r * CP_FACTOR, cx - r, cy},
                   6);
  add_command_to_path(path, SPO_CUBIC_BEZ);
  add_args_to_path(path,
                   (float[6]){cx - r, cy - r * CP_FACTOR, cx - r * CP_FACTOR,
                              cy - r, cx, cy - r},
                   6);
  add_command_to_path(path, SPO_CUBIC_BEZ);
  add_args_to_path(path,
                   (float[6]){cx + r * CP_FACTOR, cy - r, cx + r,
                              cy - r * CP_FACTOR, cx + r, cy},
                   6);
}

static void skip_sep(const char** s) {
  if (!*s) {
    return;
  }
  while (**s && (isspace(**s) || **s == ',')) {
    (*s)++;
  }
}

static uint32_t parse_color(const char* str) {
  uint32_t color = 0;
  const char* hex = str;
  skip_sep(&hex);
  size_t len = strlen(hex);
  if (len >= 1 && hex[0] == '#') {
    return parse_color_hex(hex, NULL);
  } else if (len >= 4 && hex[0] == 'r' && hex[1] == 'g' && hex[2] == 'b' &&
             hex[3] == '(') {
    return parse_color_rgb(hex);
  } else if (len >= 5 && hex[0] == 'r' && hex[1] == 'g' && hex[2] == 'b' &&
             hex[3] == 'a' && hex[4] == '(') {
    return parse_color_rgba(hex, NULL);
  } else if (len >= 1) {
    int i, colors = sizeof(sr_svg__colors) / sizeof(SRSVGNamedColor);
    for (i = 0; i < colors; i++) {
      if (strcmp(sr_svg__colors[i].name, hex) == 0) {
        return sr_svg__colors[i].color;
      }
    }
    return NSVG_RGB(128, 128, 128);
  } else {
    return 0;
  }
  return color;
}

static uint32_t parse_color_rgb(const char* str) {
  int r = -1, g = -1, b = -1;
  char s1[32] = "", s2[32] = "";
  sscanf(str + 4, "%d%[%%, \t]%d%[%%, \t]%d", &r, s1, &g, s2, &b);
  if (strchr(s1, '%')) {
    return NSVG_RGB((r * 255) / 100, (g * 255) / 100, (b * 255) / 100);
  } else {
    return NSVG_RGB(r, g, b);
  }
}

static uint32_t parse_color_rgba(const char* str, float* opacity) {
  int r = -1, g = -1, b = -1;
  float a = 1.0;
  char s1[32] = "", s2[32] = "", s3[32] = "";
  sscanf(str + 5, "%d%[%%, \t]%d%[%%, \t]%d%[%%, \t]%f", &r, s1, &g, s2, &b, s3,
         &a);
  if (opacity) {
    *opacity = a;
  }
  if (strchr(s1, '%')) {
    return NSVG_RGBA((r * 255) / 100, (g * 255) / 100, (b * 255) / 100, a);
  } else {
    return NSVG_RGBA(r, g, b, (a * 255));
  }
}

static uint32_t parse_color_hex(const char* hex, float* opacity) {
  uint32_t len = 0;

  // calculate the length of hex number, stop by whitespace.
  while (hex[len] && !isspace(hex[len])) {
    len++;
  }

  // Lookup table for hex number to decimal.
  // The ASCII of 'f' is 102
  static int lookupTable[103] = {
      -1, ['0'] = 0, 1,  2,  3,  4,  5,          6,  7,  8,  9,  ['A'] = 10,
      11, 12,        13, 14, 15, -1, ['a'] = 10, 11, 12, 13, 14, 15};

  if (len < 3 || len > 9 || hex[0] != '#') {
    // If the length is invalid or the hex string does not start with '#',
    // return a transparent color.
    return 0;
  }
  uint32_t color = 0;

  char* cPos = (char*)(hex + 1);
  uint32_t colors[4] = {0, [3] = 0xff};  // number for r,g,b,a

  // when we got only 3 or 4 hex digit, we need to repeat it for each channel.
  char needRepeat = len < 7;

  // Mapping hex length to color channels.
  uint32_t numIteration = needRepeat ? (len - 1) : (len - 1) / 2;

  if (len == 4 || len == 5 || len == 7 || len == 9) {
    for (uint32_t i = 0; i < numIteration; i++) {
      char c = *cPos++;
      int value = lookupTable[(int)c];
      if (value == -1) {
        return 0;
      }
      // set value to color. When this is alpha channel, the
      // initial 0xff value will be shift out when compose to
      // a uint32_t value.
      colors[i] = (colors[i] << 4) | value;
      if (!needRepeat) {
        c = *cPos++;
        value = lookupTable[(int)c];  // repeat the value of color
      }
      colors[i] = (colors[i] << 4) | value;
    }
  } else {
    // If the length does not match any valid cases, return 0 (transparent
    // color).
    return 0;
  }
  color = NSVG_RGBA(colors[0], colors[1], colors[2], colors[3]);
  if (opacity != NULL) {
    *opacity = colors[3] / 255.0f;
  }
  // Return the final color value.
  return color;
}

SrSVGStrokeCap resolve_stroke_line_cap(const char* value) {
  if (strcmp(value, "round") == 0) {
    return SR_SVG_STROKE_CAP_ROUND;
  } else if (strcmp(value, "square") == 0) {
    return SR_SVG_STROKE_CAP_SQUARE;
  }
  return SR_SVG_STROKE_CAP_BUTT;
}

SrSVGStrokeJoin resolve_stroke_line_join(const char* value) {
  if (strcmp(value, "round") == 0) {
    return SR_SVG_STROKE_JOIN_ROUND;
  } else if (strcmp(value, "bevel") == 0) {
    return SR_SVG_STROKE_JOIN_BEVEL;
  }
  return SR_SVG_STROKE_JOIN_MITER;
}
