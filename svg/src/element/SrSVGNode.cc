// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGNode.h"
#if defined(_WIN32) || defined(_WIN64)
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <cstdio>

#include "canvas/SrCanvas.h"
#include "element/SrSVGClipPath.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

const float SrSVGNode::s_stroke_miter_limit = 4.f;

void SrSVGNodeBase::Render(canvas::SrCanvas* const canvas,
                           SrSVGRenderContext& context) {
  // TODO(renzhongyue): directly return if prepare failed.
  canvas->Save();
  OnPrepareToRender(canvas, context);
  OnRender(canvas, context);
  canvas->Restore();
}

bool SrSVGNode::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "id") == 0) {
    id_ = value;
  } else if (strcmp(name, "fill") == 0) {
    release_serval_paint(fill_);
    fill_ = make_serval_paint(value);
  } else if (strcmp(name, "stroke") == 0) {
    release_serval_paint(stroke_);
    stroke_ = make_serval_paint(value);
  } else if (strcmp(name, "opacity") == 0) {
    opacity_ = Atof(value);
  } else if (strcmp(name, "stroke-width") == 0) {
    stroke_width_ = make_serval_length(value);
  } else if (strcmp(name, "stroke-dasharray") == 0) {
    ParseStrokeDashArray(value);
  } else if (strcmp(name, "stroke-dashoffset") == 0) {
    stroke_dash_offset_ = Atof(value);
  } else if (strcmp(name, "stroke-linecap") == 0) {
    stroke_cap_ = resolve_stroke_line_cap(value);
  } else if (strcmp(name, "stroke-linejoin") == 0) {
    stroke_join_ = resolve_stroke_line_join(value);
  } else if (strcmp(name, "stroke-miterlimit") == 0) {
    stoke_miter_limit_ = Atof(value);
  } else if (strcmp(name, "fill-opacity") == 0) {
    fill_opacity_ = Atof(value);
  } else if (strcmp(name, "stroke-opacity") == 0) {
    stroke_opacity_ = Atof(value);
  } else if (strcmp(name, "clip-path") == 0) {
    clip_path_ = make_serval_paint(value);
  } else if (strcmp(name, "transform") == 0) {
    ParseTransform(value, transform_);
  } else if (strcmp(name, "color") == 0) {
    color_ = make_serval_color(value);
  } else if (strcmp(name, "style") == 0) {
    ParseStyle(value);
  }
  return false;
}

SrSVGNode::~SrSVGNode() {
  release_serval_paint(fill_);
  release_serval_paint(stroke_);
  release_serval_paint(clip_path_);
}

bool SrPreparePattern(canvas::SrCanvas* canvas, SrSVGNodeBase* node,
                      SrSVGRenderContext& context) {
  switch (node->Tag()) {
    case SrSVGTag::kLinearGradient:
    case SrSVGTag::kRadialGradient:
    case SrSVGTag::kPattern:
      // paint will be generated on platform layer indexed by id.
      node->Render(canvas, context);
      return true;
    default:
      return false;
  }
}

bool SrSVGNode::OnPrepareToRender(canvas::SrCanvas* canvas,
                                  SrSVGRenderContext& context) const {
  SrSVGPaint* local_clip_path =
      clip_path_ != nullptr ? clip_path_ : inherit_clip_path_;
  if (local_clip_path && local_clip_path->type == SERVAL_PAINT_IRI) {
    std::string id(local_clip_path->content.iri + 1);
    IDMapper* nodes = static_cast<IDMapper*>(context.id_mapper);
    if (nodes) {
      SrSVGClipPath* clip_path_node =
          static_cast<SrSVGClipPath*>(nodes->at(id));
      if (clip_path_node) {
        std::unique_ptr<canvas::Path> path =
            clip_path_node->AsPath(canvas->PathFactory(), &context);
        if (clip_path_node->clip_path_units() ==
            SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
          SrSVGBox svg_box =
              this->AsPath(canvas->PathFactory(), &context)->GetBounds();
          float xform[6] = {svg_box.width, 0,          0, svg_box.height,
                            svg_box.left,  svg_box.top};
          path = path->CreateTransformCopy(xform);
        }
        canvas->ClipPath(path.get(), clip_path_node->clip_rule());
      }
    }
  }
  if (fill_ && fill_->type == SERVAL_PAINT_IRI) {
    std::string id{fill_->content.iri + 1};
    IDMapper* nodes = static_cast<IDMapper*>(context.id_mapper);
    SrSVGNodeBase* fill_content = (*nodes)[id];
    if (fill_content) {
      SrPreparePattern(canvas, fill_content, context);
    }
  }
  if (stroke_ && stroke_->type == SERVAL_PAINT_IRI) {
    std::string id{stroke_->content.iri + 1};
    IDMapper* nodes = static_cast<IDMapper*>(context.id_mapper);
    SrSVGNodeBase* stroke_content = (*nodes)[id];
    if (stroke_content) {
      SrPreparePattern(canvas, stroke_content, context);
    }
  }
  return false;
}

void SrSVGNode::ParseTransform(const char* str, float* xform) {
  xform_identity(xform);
  float tmp_xform[6];
  while (*str) {
    if (strncmp(str, "matrix", 6) == 0) {
      str += ParseMatrix(tmp_xform, str);
    } else if (strncmp(str, "translate", 9) == 0) {
      str += ParseTranslate(tmp_xform, str);
    } else if (strncmp(str, "scale", 5) == 0) {
      str += ParseScale(tmp_xform, str);
    } else if (strncmp(str, "rotate", 6) == 0) {
      str += ParseRotate(tmp_xform, str);
    } else if (strncmp(str, "skewX", 5) == 0) {
      str += ParseSkewX(tmp_xform, str);
    } else if (strncmp(str, "skewY", 5) == 0) {
      str += ParseSkewY(tmp_xform, str);
    } else {
      ++str;
      continue;
    }
    xform_multiply(xform, tmp_xform);
  }
}

int SrSVGNode::ParseMatrix(float* xform, const char* str) {
  float args[6];
  int arg_index = 0;
  int length = ParseTransformArgs(str, args, 6, &arg_index);
  if (arg_index == 6) {
    memcpy(xform, args, sizeof(float) * 6);
  }
  return length;
}

int SrSVGNode::ParseTranslate(float* xform, const char* str) {
  float args[2];
  int arg_index = 0;
  float t[6];
  int length = ParseTransformArgs(str, args, 2, &arg_index);
  if (arg_index == 1) {
    args[1] = 0.f;
  }
  xform_set_translation(t, args[0], args[1]);
  memcpy(xform, t, sizeof(float) * 6);
  return length;
}

int SrSVGNode::ParseScale(float* xform, const char* str) {
  float args[2];
  int arg_index = 0;
  float t[6];
  int length = ParseTransformArgs(str, args, 2, &arg_index);
  if (arg_index == 1) {
    args[1] = args[0];
  }
  xform_set_scale(t, args[0], args[1]);
  memcpy(xform, t, sizeof(float) * 6);
  return length;
}

int SrSVGNode::ParseRotate(float* xform, const char* str) {
  float args[3];
  int arg_index = 0;
  int length = ParseTransformArgs(str, args, 3, &arg_index);
  if (arg_index == 1) {
    args[1] = 0.f;
    args[2] = 0.f;
  }
  float tmp_form[6];
  xform_identity(xform);
  if (arg_index > 1) {
    xform_set_translation(tmp_form, -args[1], -args[2]);
    xform_pre_multiply(xform, tmp_form);
  }
  xform_set_rotation(tmp_form, args[0] / 180.0f * M_PI);
  xform_pre_multiply(xform, tmp_form);
  if (arg_index > 1) {
    xform_set_translation(tmp_form, args[1], args[2]);
    xform_pre_multiply(xform, tmp_form);
  }
  return length;
}

int SrSVGNode::ParseSkewX(float* xform, const char* str) {
  float args[1];
  int arg_index = 0;
  int length = ParseTransformArgs(str, args, 3, &arg_index);
  xform_set_skewX(xform, args[0] / 180.0f * M_PI);
  return length;
}

int SrSVGNode::ParseSkewY(float* xform, const char* str) {
  float args[1];
  int arg_index = 0;
  int length = ParseTransformArgs(str, args, 3, &arg_index);
  xform_set_skewY(xform, args[0] / 180.0f * M_PI);
  return length;
}

int SrSVGNode::ParseTransformArgs(const char* str, float* args,
                                  int max_args_number, int* arg_index) {
  const char* end = nullptr;
  const char* ptr = nullptr;
  char it[64];

  *arg_index = 0;
  ptr = str;
  while (*ptr && *ptr != '(') {
    ++ptr;
  }
  if (*ptr == 0) {
    return 1;
  }
  end = ptr;
  while (*end && *end != ')') {
    ++end;
  }
  if (*end == 0) {
    return 1;
  }
  while (ptr < end) {
    if (*ptr == '-' || *ptr == '+' || *ptr == '.' || isdigit(*ptr)) {
      if (*arg_index >= max_args_number) {
        return 0;
      }
      ptr = ParseNumber(ptr, it, 64);
      args[(*arg_index)++] = (float)Atof(it);
    } else {
      ++ptr;
    }
  }
  return (int)(end - str);
}

void SrSVGNode::ParseStrokeDashArray(const char* value) {
  stroke_dash_array_.clear();
  char* input = const_cast<char*>(value);
  // replace the ','with ''
  for (int i = 0; input[i] != '\0'; ++i) {
    if (input[i] == ',')
      input[i] = ' ';
  }
  // adopt the '' to strtok the str
  char* token = strtok(input, " ");
  while (token != NULL) {
    // add the value to the vector
    stroke_dash_array_.push_back(static_cast<float>(Atof(token)));
    token = strtok(NULL, " ");
  }
}

static int IsSpace(char c) {
  if (c == 0)
    return 0;
  return strchr(" \t\n\v\f\r", c) != 0;
}

bool SrSVGNodeBase::ParseNameValue(const char* start, const char* end) {
  const char* str;
  const char* val;
  char name[512];
  char value[512];
  int n;

  str = start;
  while (str < end && *str != ':')
    ++str;

  val = str;

  // Right Trim
  while (str > start && (str == end || *str == ':' || IsSpace(*str)))
    --str;
  ++str;

  n = (int)(str - start);
  if (n > 511)
    n = 511;
  if (n)
    memcpy(name, start, n);
  name[n] = 0;

  while (val < end && (*val == ':' || IsSpace(*val)))
    ++val;

  n = (int)(end - val);
  if (n > 511)
    n = 511;
  if (n)
    memcpy(value, val, n);
  value[n] = 0;

  return ParseAndSetAttribute(name, value);
}

void SrSVGNodeBase::ParseStyle(const char* str) {
  const char* start;
  const char* end;
  while (*str) {
    // Left Trim
    while (*str && IsSpace(*str))
      ++str;
    start = str;
    int parenthesis_depth = 0;
    int quote_status = 0; // 0: no quote, 1: single quote, 2: double quote
    while (*str) {
        if (*str == ';' && parenthesis_depth == 0 && quote_status == 0) {
            break;
        }
        if (*str == '(') {
            parenthesis_depth++;
        } else if (*str == ')') {
            if (parenthesis_depth > 0) parenthesis_depth--;
        } else if (*str == '\'') {
            if (quote_status == 0) quote_status = 1;
            else if (quote_status == 1) quote_status = 0;
        } else if (*str == '"') {
            if (quote_status == 0) quote_status = 2;
            else if (quote_status == 2) quote_status = 0;
        }
        ++str;
    }
    end = str;

    // Right Trim
    while (end > start && (*end == 0 || *end == ';' || IsSpace(*end)))
      --end;
    ++end;

    ParseNameValue(start, end);
    if (*str)
      ++str;
  }
}

const char* SrSVGNode::ParseNumber(const char* s, char* it, const int size) {
  const int last = size - 1;
  int i = 0;

  // sign
  if (*s == '-' || *s == '+') {
    if (i < last)
      it[i++] = *s;
    s++;
  }
  // integer part
  while (*s && isdigit(*s)) {
    if (i < last)
      it[i++] = *s;
    s++;
  }
  if (*s == '.') {
    // decimal point
    if (i < last)
      it[i++] = *s;
    s++;
    // fraction part
    while (*s && isdigit(*s)) {
      if (i < last)
        it[i++] = *s;
      s++;
    }
  }
  // exponent
  if ((*s == 'e' || *s == 'E') && (s[1] != 'm' && s[1] != 'x')) {
    if (i < last)
      it[i++] = *s;
    s++;
    if (*s == '-' || *s == '+') {
      if (i < last)
        it[i++] = *s;
      s++;
    }
    while (*s && isdigit(*s)) {
      if (i < last)
        it[i++] = *s;
      s++;
    }
  }
  it[i] = '\0';

  return s;
}

double SrSVGNode::Atof(const char* s) {
  char* cur = (char*)s;
  char* end = NULL;
  double res = 0.0, sign = 1.0;
  long long intPart = 0, fracPart = 0;
  char hasIntPart = 0, hasFracPart = 0;

  // Parse optional sign
  if (*cur == '+') {
    cur++;
  } else if (*cur == '-') {
    sign = -1;
    cur++;
  }

  // Parse integer part
  if (isdigit(*cur)) {
    // Parse digit sequence
    intPart = strtoll(cur, &end, 10);
    if (cur != end) {
      res = (double)intPart;
      hasIntPart = 1;
      cur = end;
    }
  }

  // Parse fractional part.
  if (*cur == '.') {
    cur++;  // Skip '.'
    if (isdigit(*cur)) {
      // Parse digit sequence
      fracPart = strtoll(cur, &end, 10);
      if (cur != end) {
        res += (double)fracPart / pow(10.0, (double)(end - cur));
        hasFracPart = 1;
        cur = end;
      }
    }
  }

  // A valid number should have integer or fractional part.
  if (!hasIntPart && !hasFracPart)
    return 0.0;

  // Parse optional exponent
  if (*cur == 'e' || *cur == 'E') {
    long expPart = 0;
    cur++;                            // skip 'E'
    expPart = strtol(cur, &end, 10);  // Parse digit sequence with sign
    if (cur != end) {
      res *= pow(10.0, (double)expPart);
    }
  }

  return res * sign;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
