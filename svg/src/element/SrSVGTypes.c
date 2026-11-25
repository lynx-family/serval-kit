// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGTypes.h"
#include "parser/SrSVGMemory.h"
#include "parser/SrSVGParse.h"
#include "utils/SrSVGMath.h"
#include "utils/SrSVGNumber.h"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SR_SVG_INVALID_COLOR_FALLBACK NSVG_RGB(128, 128, 128)

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
    {"transparent", NSVG_RGBA(0, 0, 0, 0)},
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
static bool parse_color_hex(const char* str, uint32_t* out_color);
static bool parse_color_rgb(const char* str, uint32_t* out_color);
static bool parse_color_rgba(const char* str, uint32_t* out_color);
static bool parse_color_display_p3(const char* str, uint32_t* out_color);

SrSVGUnits make_serval_length_unit(const char* c) {
  if (strcmp(c, "px") == 0) {
    return SR_SVG_UNITS_PX;
  } else if (strcmp(c, "pt") == 0) {
    return SR_SVG_UNITS_PT;
  } else if (strcmp(c, "pc") == 0) {
    return SR_SVG_UNITS_PC;
  } else if (strcmp(c, "mm") == 0) {
    return SR_SVG_UNITS_MM;
  } else if (strcmp(c, "cm") == 0) {
    return SR_SVG_UNITS_CM;
  } else if (strcmp(c, "in") == 0) {
    return SR_SVG_UNITS_IN;
  } else if (strcmp(c, "em") == 0) {
    return SR_SVG_UNITS_EM;
  } else if (strcmp(c, "ex") == 0) {
    return SR_SVG_UNITS_EX;
  } else if (strcmp(c, "%") == 0) {
    return SR_SVG_UNITS_PERCENTAGE;
  }
  return SR_SVG_UNITS_NUMBER;
}

bool parse_serval_length(const char* value, SrSVGLength* out) {
  return value && SrSVGParseLength(value, strlen(value), out, NULL);
}

SrSVGLength make_serval_length(const char* value) {
  SrSVGLength result = {0.f, SR_SVG_UNITS_NUMBER};
  parse_serval_length(value, &result);
  return result;
}

GradientSpread make_serval_spread_method(const char* value) {
  if (strcmp(value, "reflect") == 0) {
    return reflect;
  } else if (strcmp(value, "repeat") == 0) {
    return repeat;
  }
  return pad;
}

bool parse_serval_view_box(const char* value, SrSVGBox* out) {
  return value && SrSVGParseViewBox(value, strlen(value), out, NULL);
}

SrSVGBox make_serval_view_box(const char* value) {
  SrSVGBox result = {0};
  parse_serval_view_box(value, &result);
  return result;
}

SrSVGPreserveAspectRatio make_default_preserve_aspect_radio() {
  return (SrSVGPreserveAspectRatio){.align_x = SR_SVG_AlIGN_MID,
                                    .align_y = SR_SVG_AlIGN_MID,
                                    .scale = SR_SVG_SCALE_MEET};
}

static bool parse_align(SrSVGParseCursor* cursor, SrSVGAlign* align) {
  if (SrSVGCursorMatch(cursor, "Min"))
    *align = SR_SVG_AlIGN_MIN;
  else if (SrSVGCursorMatch(cursor, "Mid"))
    *align = SR_SVG_AlIGN_MID;
  else if (SrSVGCursorMatch(cursor, "Max"))
    *align = SR_SVG_AlIGN_MAX;
  else
    return false;
  return true;
}

bool parse_serval_preserve_aspect_ratio(const char* value,
                                        SrSVGPreserveAspectRatio* out) {
  if (!value || !out)
    return false;
  SrSVGParseCursor cursor = SrSVGCursor(value, strlen(value));
  SrSVGCursorSpaces(&cursor);
  if (SrSVGCursorMatch(&cursor, "defer")) {
    const char* end = cursor.pos;
    SrSVGCursorSpaces(&cursor);
    if (cursor.pos == end)
      return false;
  }
  SrSVGPreserveAspectRatio result = make_default_preserve_aspect_radio();
  bool none = SrSVGCursorMatch(&cursor, "none");
  if (!none && (!SrSVGCursorMatch(&cursor, "x") ||
                !parse_align(&cursor, &result.align_x) ||
                !SrSVGCursorMatch(&cursor, "Y") ||
                !parse_align(&cursor, &result.align_y)))
    return false;
  const char* end = cursor.pos;
  SrSVGCursorSpaces(&cursor);
  if (!SrSVGCursorEnd(&cursor)) {
    if (cursor.pos == end)
      return false;
    if (SrSVGCursorMatch(&cursor, "slice"))
      result.scale = SR_SVG_SCALE_SLICE;
    else if (!SrSVGCursorMatch(&cursor, "meet"))
      return false;
    SrSVGCursorSpaces(&cursor);
    if (!SrSVGCursorEnd(&cursor))
      return false;
  }
  if (none)
    result.scale = SR_SVG_SCALE_NONE;
  *out = result;
  return true;
}

SrSVGPreserveAspectRatio make_preserve_aspect_radio(const char* value) {
  SrSVGPreserveAspectRatio result = make_default_preserve_aspect_radio();
  parse_serval_preserve_aspect_ratio(value, &result);
  return result;
}

bool serval_svg_valid_dash_array(const char* value) {
  if (!value)
    return false;
  SrSVGParseCursor cursor = SrSVGCursor(value, strlen(value));
  SrSVGCursorSpaces(&cursor);
  if (SrSVGCursorMatch(&cursor, "none")) {
    SrSVGCursorSpaces(&cursor);
    return SrSVGCursorEnd(&cursor);
  }
  double sum = 0;
  size_t count = 0;
  while (!SrSVGCursorEnd(&cursor)) {
    float entry;
    if (!SrSVGParseCoordinate(&cursor, &entry, NULL) || entry < 0.f)
      return false;
    sum += entry;
    if (sum > FLT_MAX / 2 || ++count > 16384)
      return false;  // Odd lists may be doubled.
    const char* end = cursor.pos;
    SrSVGCursorSpaces(&cursor);
    if (SrSVGCursorConsume(&cursor, ',')) {
      SrSVGCursorSpaces(&cursor);
      if (SrSVGCursorEnd(&cursor))
        return false;
    } else if (!SrSVGCursorEnd(&cursor) && cursor.pos == end) {
      return false;
    }
  }
  return count != 0;
}

bool serval_svg_valid_box(const SrSVGBox* box) {
  return box && isfinite(box->left) && isfinite(box->top) &&
         isfinite(box->width) && isfinite(box->height) &&
         isfinite(box->left + box->width) && isfinite(box->top + box->height) &&
         box->width > 0.f && box->height > 0.f;
}

bool resolve_serval_view_box_transform(
    const SrSVGBox* view_port, const SrSVGBox* view_box,
    SrSVGPreserveAspectRatio preserve_aspect_ratio, float* out) {
  float xform[6];
  xform_identity(xform);
  if (!serval_svg_valid_box(view_port) || !serval_svg_valid_box(view_box))
    return false;
  float x_scale = view_port->width / view_box->width;
  float y_scale = view_port->height / view_box->height;
  if (!isfinite(x_scale) || !isfinite(y_scale) || x_scale <= 0.f ||
      y_scale <= 0.f)
    return false;
  float x_offset = -view_box->left;
  float y_offset = -view_box->top;
  if (preserve_aspect_ratio.scale == SR_SVG_SCALE_NONE) {
    xform_pre_translate(xform, view_port->left, view_port->top);
    xform_pre_scale(xform, x_scale, y_scale);
    xform_pre_translate(xform, x_offset, y_offset);
    goto validate;
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
validate:
  for (int i = 0; i < 6; ++i) {
    if (!isfinite(xform[i]))
      return false;
  }
  memcpy(out, xform, sizeof(xform));
  return true;
}

void calculate_view_box_transform(
    const SrSVGBox* view_port, const SrSVGBox* view_box,
    SrSVGPreserveAspectRatio preserve_aspect_ratio, float* xform) {
  if (!resolve_serval_view_box_transform(view_port, view_box,
                                         preserve_aspect_ratio, xform))
    xform_identity(xform);
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
static const char* extract_iri(const char* value, bool* allocation_failed) {
  *allocation_failed = false;
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
  while (isspace((unsigned char)*start_ptr)) {
    start_ptr++;  // ignore leading whitespace
  }
  while (end_ptr > start_ptr && isspace((unsigned char)*end_ptr)) {
    end_ptr--;  // ignore trailing whitespace
  }
  if (end_ptr < start_ptr) {
    return NULL;
  }

  const size_t result_size = (size_t)(end_ptr - start_ptr) + 1;
  char* result = SrSVGAlloc(result_size + 1);
  if (!result) {
    *allocation_failed = true;
    return NULL;
  }
  memcpy(result, start_ptr, result_size);
  result[result_size] = '\0';
  return (const char*)result;
}
#undef SERVAL_SVG_LEFT_PAREN
#undef SERVAL_SVG_RIGHT_PAREN

SrSVGPaint* make_serval_paint(const char* value) {
  if (!value)
    return NULL;
  SrSVGPaint* paint = SrSVGAlloc(sizeof(SrSVGPaint));
  if (!paint)
    return NULL;
  *paint = (SrSVGPaint){.type = SERVAL_PAINT_NONE, .content = {0}};
  if (strcmp(value, "context-fill") == 0 ||
      strcmp(value, "context-stroke") == 0) {
    *paint = (SrSVGPaint){.type = strcmp(value, "context-fill") == 0
                                      ? SERVAL_PAINT_CONTEXT_FILL
                                      : SERVAL_PAINT_CONTEXT_STROKE,
                          .content = {0}};
    return paint;
  }
  if (strcmp(value, "none") == 0)
    return paint;
  if (strncmp(value, "url(", 4) == 0) {
    bool allocation_failed;
    const char* iri = extract_iri(value, &allocation_failed);
    if (allocation_failed) {
      SrSVGFree(paint);
      return NULL;
    }
    if (!iri)
      return paint;
    if (!*iri) {
      SrSVGFree((void*)iri);
    } else {
      paint->type = SERVAL_PAINT_IRI;
      paint->content.iri = iri;
    }
  } else {
    paint->type = SERVAL_PAINT_COLOR;
    paint->content.color = make_serval_color(value);
  }
  return paint;
}

void release_serval_paint(SrSVGPaint* paint) {
  if (paint) {
    if (paint->type == SERVAL_PAINT_IRI) {
      SrSVGFree((void*)paint->content.iri);
      paint->content.iri = NULL;
    }
  }
  SrSVGFree(paint);
}

SrSVGColor make_serval_color(const char* value) {
  if (strcmp(value, "currentColor") == 0) {
    SrSVGColor color = (SrSVGColor){.type = SERVAL_CURRENT_COLOR, .color = 0};
    return color;
  } else {
    uint32_t color_int = SR_SVG_INVALID_COLOR_FALLBACK;
    parse_svg_color(value, &color_int);
    SrSVGColor color = (SrSVGColor){.type = SERVAL_COLOR, .color = color_int};
    return color;
  }
}

static float length_size_for_type(const SrSVGRenderContext* render_context,
                                  SrSVGLengthType type) {
  switch (type) {
    case SR_SVG_LENGTH_TYPE_HORIZONTAL:
      return render_context->view_box.width;
    case SR_SVG_LENGTH_TYPE_VERTICAL:
      return render_context->view_box.height;
    case SR_SVG_LENGTH_TYPE_OTHER: {
      const float width = render_context->view_box.width;
      const float height = render_context->view_box.height;
      const float squares = width * width + height * height;
      return kSrSVGInvSqrt2 *
             (isfinite(squares) ? sqrt(squares) : hypot(width, height));
    }
    case SR_SVG_LENGTH_TYPE_NUMERIC:
      return 1.f;
  }
  return .0f;
}

bool resolve_serval_length(const SrSVGLength* length,
                           const SrSVGRenderContext* context,
                           SrSVGLengthType type, float* out) {
  if (!length || !context || !out || !isfinite(length->value))
    return false;
  const float value = length->value;
  float resolved;
  switch (length->unit) {
    case SR_SVG_UNITS_NUMBER:
    case SR_SVG_UNITS_UNKNOWN:
    case SR_SVG_UNITS_PX:
      resolved = value;
      break;
    case SR_SVG_UNITS_PT:
      resolved = value / 72.f * context->dpi;
      break;
    case SR_SVG_UNITS_PC:
      resolved = value / 6.f * context->dpi;
      break;
    case SR_SVG_UNITS_MM:
      resolved = value / 25.4f * context->dpi;
      break;
    case SR_SVG_UNITS_CM:
      resolved = value / 2.54f * context->dpi;
      break;
    case SR_SVG_UNITS_IN:
      resolved = value * context->dpi;
      break;
    case SR_SVG_UNITS_EM:
      resolved = value * context->font_size;
      break;
    case SR_SVG_UNITS_EX: {
      const float em = value * context->font_size;
      if (isfinite(em)) {
        resolved = em * .52f;
      } else {
        const double ex = (double)value * context->font_size * .52f;
        if (!isfinite(ex) || fabs(ex) > FLT_MAX)
          return false;
        resolved = (float)ex;
      }
      break;
    }
    case SR_SVG_UNITS_PERCENTAGE:
      resolved = value / 100.f * length_size_for_type(context, type);
      break;
    default:
      return false;
  }
  if (!isfinite(resolved))
    return false;
  *out = resolved;
  return true;
}

float convert_serval_length_to_float(const SrSVGLength* length,
                                     SrSVGRenderContext* render_context,
                                     SrSVGLengthType type) {
  float result = 0.f;
  resolve_serval_length(length, render_context, type, &result);
  return result;
}

static bool append_path_command(SrPathData* path, SrPathOps op,
                                const float* args, size_t len) {
  return SrSVGAppendPathCommand(path, op, args, len, 8) == SR_PARSE_OK;
}

SrPathData* make_serval_path(const char* value,
                             const SrSVGDiagnosticSink* sink) {
  SrPathData* path = NULL;
  SrSVGParseError error;
  if (value && SrSVGParsePath(value, strlen(value), &path, &error))
    return path;
  if (value && error.code != SR_PARSE_OOM)
    SrSVGReportDiagnostic(sink, SR_SVG_DIAGNOSTIC_PATH_PARSER_STALLED,
                          "Stopped parsing invalid path data.",
                          value + error.offset, 0);
  return NULL;
}

void release_serval_polygon_path(SrPolygon* path) {
  if (!path) {
    return;
  }
  if (path->points) {
    SrSVGFree(path->points);
  }
  SrSVGFree(path);
}

SrPolygon* make_serval_polygon(const char* value,
                               const SrSVGDiagnosticSink* sink) {
  SrPolygon* polygon = NULL;
  SrSVGParseError error;
  if (value && SrSVGParsePolygon(value, strlen(value), &polygon, &error))
    return polygon;
  if (value && error.code != SR_PARSE_OOM)
    SrSVGReportDiagnostic(sink, SR_SVG_DIAGNOSTIC_POLYGON_PARSER_STALLED,
                          "Stopped parsing invalid polygon/polyline points.",
                          value + error.offset, 0);
  return NULL;
}

void release_serval_path(SrPathData* path) {
  if (!path) {
    return;
  }
  if (path->args) {
    SrSVGFree(path->args);
    path->args = NULL;
  }
  if (path->ops) {
    SrSVGFree(path->ops);
    path->ops = NULL;
  }
  SrSVGFree(path);
}

// reference to https://spencermortensen.com/articles/bezier-circle/
void add_circle_to_path(SrPathData* path, float cx, float cy, float r) {
  const double control_point_factor = 0.5519150244935105;
  if (!append_path_command(path, SPO_MOVE_TO, (float[2]){cx + r, cy}, 2) ||
      !append_path_command(
          path, SPO_CUBIC_BEZ,
          (float[6]){cx - r * control_point_factor, cy + r, cx - r,
                     cy + r * control_point_factor, cx - r, cy},
          6) ||
      !append_path_command(
          path, SPO_CUBIC_BEZ,
          (float[6]){cx - r, cy - r * control_point_factor,
                     cx - r * control_point_factor, cy - r, cx, cy - r},
          6)) {
    return;
  }
  append_path_command(path, SPO_CUBIC_BEZ,
                      (float[6]){cx + r * control_point_factor, cy - r, cx + r,
                                 cy - r * control_point_factor, cx + r, cy},
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

static float clamp01(float v) {
  if (v < 0.0f)
    return 0.0f;
  if (v > 1.0f)
    return 1.0f;
  return v;
}

static void skip_spaces_only(const char** s) {
  if (!*s) {
    return;
  }
  while (**s && isspace(**s)) {
    (*s)++;
  }
}

static bool consume_required_separator(const char** s) {
  const char* start = *s;
  skip_sep(s);
  return *s != start;
}

static bool parse_float_token(const char** s, float* out, bool* is_percent) {
  float value;
  if (!SrSVGParseFloat(s, &value))
    return false;
  *is_percent = false;
  if (**s == '%') {
    *is_percent = true;
    (*s)++;
  }
  *out = value;
  return true;
}

static bool consume_closing_paren(const char** s) {
  skip_spaces_only(s);
  if (**s != ')') {
    return false;
  }
  (*s)++;
  skip_spaces_only(s);
  return **s == '\0';
}

static uint32_t clamp_color_byte(float value, bool is_percent) {
  if (is_percent) {
    value = value * 255.0f / 100.0f;
  }
  if (value < 0.0f) {
    value = 0.0f;
  }
  if (value > 255.0f) {
    value = 255.0f;
  }
  return (uint32_t)lrintf(value);
}

static uint32_t clamp_alpha_byte(float value, bool is_percent) {
  if (is_percent) {
    value = value / 100.0f;
  }
  value = clamp01(value);
  return (uint32_t)lrintf(value * 255.0f);
}

static bool parse_color_hex(const char* str, uint32_t* out_color) {
  size_t len = 0;
  while (str[len] && !isspace(str[len])) {
    len++;
  }
  if (!(len == 4 || len == 5 || len == 7 || len == 9) || str[0] != '#') {
    return false;
  }
  for (size_t i = 1; i < len; ++i) {
    if (!isxdigit(str[i])) {
      return false;
    }
  }
  const char* tail = str + len;
  skip_spaces_only(&tail);
  if (*tail != '\0') {
    return false;
  }
  uint32_t colors[4] = {0, [3] = 0xff};
  char need_repeat = len < 7;
  uint32_t num_iteration = need_repeat ? (len - 1) : (len - 1) / 2;
  static int lookup_table[103] = {
      -1, ['0'] = 0, 1,  2,  3,  4,  5,          6,  7,  8,  9,  ['A'] = 10,
      11, 12,        13, 14, 15, -1, ['a'] = 10, 11, 12, 13, 14, 15};
  char* c_pos = (char*)(str + 1);
  for (uint32_t i = 0; i < num_iteration; i++) {
    char c = *c_pos++;
    int value = lookup_table[(int)c];
    if (value == -1) {
      return false;
    }
    colors[i] = (colors[i] << 4) | value;
    if (!need_repeat) {
      c = *c_pos++;
      value = lookup_table[(int)c];
      if (value == -1) {
        return false;
      }
    }
    colors[i] = (colors[i] << 4) | value;
  }
  *out_color = NSVG_RGBA(colors[0], colors[1], colors[2], colors[3]);
  return true;
}

static bool parse_color_rgb(const char* str, uint32_t* out_color) {
  const char* p = str + 4;
  float r = 0.0f, g = 0.0f, b = 0.0f;
  bool r_percent = false, g_percent = false, b_percent = false;
  if (!parse_float_token(&p, &r, &r_percent) ||
      !consume_required_separator(&p) ||
      !parse_float_token(&p, &g, &g_percent) ||
      !consume_required_separator(&p) ||
      !parse_float_token(&p, &b, &b_percent) || !consume_closing_paren(&p)) {
    return false;
  }
  *out_color =
      NSVG_RGB(clamp_color_byte(r, r_percent), clamp_color_byte(g, g_percent),
               clamp_color_byte(b, b_percent));
  return true;
}

static bool parse_color_rgba(const char* str, uint32_t* out_color) {
  const char* p = str + 5;
  float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
  bool r_percent = false, g_percent = false, b_percent = false;
  bool a_percent = false;
  if (!parse_float_token(&p, &r, &r_percent) ||
      !consume_required_separator(&p) ||
      !parse_float_token(&p, &g, &g_percent) ||
      !consume_required_separator(&p) ||
      !parse_float_token(&p, &b, &b_percent) ||
      !consume_required_separator(&p) ||
      !parse_float_token(&p, &a, &a_percent) || !consume_closing_paren(&p)) {
    return false;
  }
  *out_color =
      NSVG_RGBA(clamp_color_byte(r, r_percent), clamp_color_byte(g, g_percent),
                clamp_color_byte(b, b_percent), clamp_alpha_byte(a, a_percent));
  return true;
}

static bool parse_color_display_p3(const char* str, uint32_t* out_color) {
  const char* p = str;
  if (strncmp(p, "color(", 6) != 0) {
    return false;
  }
  p += 6;
  skip_spaces_only(&p);
  if (strncmp(p, "display-p3", 10) != 0) {
    return false;
  }
  p += 10;
  if (!consume_required_separator(&p)) {
    return false;
  }
  float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
  bool r_percent = false, g_percent = false, b_percent = false;
  bool a_percent = false;
  if (!parse_float_token(&p, &r, &r_percent) ||
      !consume_required_separator(&p) ||
      !parse_float_token(&p, &g, &g_percent) ||
      !consume_required_separator(&p) ||
      !parse_float_token(&p, &b, &b_percent)) {
    return false;
  }
  skip_spaces_only(&p);
  if (*p == '/') {
    p++;
    skip_spaces_only(&p);
    if (!parse_float_token(&p, &a, &a_percent)) {
      return false;
    }
  }
  if (!consume_closing_paren(&p)) {
    return false;
  }
  r = clamp01(r_percent ? r / 100.0f : r);
  g = clamp01(g_percent ? g / 100.0f : g);
  b = clamp01(b_percent ? b / 100.0f : b);
  a = clamp01(a_percent ? a / 100.0f : a);
  *out_color =
      NSVG_RGBA((uint32_t)lrintf(r * 255.0f), (uint32_t)lrintf(g * 255.0f),
                (uint32_t)lrintf(b * 255.0f), (uint32_t)lrintf(a * 255.0f));
  return true;
}

bool parse_svg_color(const char* str, uint32_t* out_color) {
  if (str == NULL || out_color == NULL) {
    return false;
  }
  const char* value = str;
  skip_sep(&value);
  if (*value == '\0') {
    return false;
  }
  if (value[0] == '#') {
    if (parse_color_hex(value, out_color)) {
      return true;
    }
  } else if (strncmp(value, "rgb(", 4) == 0) {
    if (parse_color_rgb(value, out_color)) {
      return true;
    }
  } else if (strncmp(value, "rgba(", 5) == 0) {
    if (parse_color_rgba(value, out_color)) {
      return true;
    }
  } else if (strncmp(value, "color(", 6) == 0) {
    if (parse_color_display_p3(value, out_color)) {
      return true;
    }
  }
  size_t value_len = strlen(value);
  while (value_len > 0 && isspace(value[value_len - 1])) {
    value_len--;
  }
  int colors = sizeof(sr_svg__colors) / sizeof(SRSVGNamedColor);
  for (int i = 0; i < colors; i++) {
    size_t name_len = strlen(sr_svg__colors[i].name);
    if (name_len == value_len &&
        strncmp(sr_svg__colors[i].name, value, value_len) == 0) {
      *out_color = sr_svg__colors[i].color;
      return true;
    }
  }
  return false;
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
