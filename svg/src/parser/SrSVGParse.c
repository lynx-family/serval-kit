// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrSVGParse.h"
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "parser/SrSVGMemory.h"

#define SR_SVG_MAX_VALUE_BYTES (1024u * 1024u)

static void reset_error(SrSVGParseError* error) {
  if (error)
    *error = (SrSVGParseError){SR_PARSE_OK, 0};
}
static void set_error(SrSVGParseError* error, SrSVGParseCode code,
                      const SrSVGParseCursor* cursor) {
  if (error)
    *error = (SrSVGParseError){code, cursor->begin && cursor->pos
                                         ? (size_t)(cursor->pos - cursor->begin)
                                         : 0};
}
SrSVGParseCursor SrSVGCursor(const char* data, size_t size) {
  return (SrSVGParseCursor){data, data, data ? data + size : NULL};
}
bool SrSVGCursorEnd(const SrSVGParseCursor* cursor) {
  return !cursor->pos ||
         (cursor->end ? cursor->pos >= cursor->end : !*cursor->pos);
}
static char peek(const SrSVGParseCursor* cursor) {
  return SrSVGCursorEnd(cursor) ? '\0' : *cursor->pos;
}
void SrSVGCursorSpaces(SrSVGParseCursor* cursor) {
  while (!SrSVGCursorEnd(cursor) && isspace((unsigned char)peek(cursor)))
    ++cursor->pos;
}
bool SrSVGCursorConsume(SrSVGParseCursor* cursor, char token) {
  if (!SrSVGCursorEnd(cursor) && peek(cursor) == token) {
    ++cursor->pos;
    return true;
  }
  return false;
}
bool SrSVGCursorMatch(SrSVGParseCursor* cursor, const char* token) {
  SrSVGParseCursor local = *cursor;
  while (*token)
    if (!SrSVGCursorConsume(&local, *token++))
      return false;
  *cursor = local;
  return true;
}
bool SrSVGCursorSeparator(SrSVGParseCursor* cursor) {
  const char* start = cursor->pos;
  SrSVGCursorSpaces(cursor);
  if (SrSVGCursorConsume(cursor, ','))
    SrSVGCursorSpaces(cursor);
  return cursor->pos != start;
}

bool SrSVGParseScalar(SrSVGParseCursor* cursor, double* out,
                      SrSVGParseError* error) {
  SrSVGParseCursor local = *cursor;
  reset_error(error);
  if (!out || SrSVGCursorEnd(&local))
    goto invalid;
  if (peek(&local) == '+' || peek(&local) == '-')
    ++local.pos;
  bool digits = false;
  while (isdigit((unsigned char)peek(&local))) {
    digits = true;
    ++local.pos;
  }
  if (SrSVGCursorConsume(&local, '.'))
    while (isdigit((unsigned char)peek(&local))) {
      digits = true;
      ++local.pos;
    }
  if (!digits)
    goto invalid;
  if (peek(&local) == 'e' || peek(&local) == 'E') {
    SrSVGParseCursor exponent = local;
    ++exponent.pos;
    // The e in em/ex belongs to a length suffix.
    if (peek(&exponent) != 'm' && peek(&exponent) != 'x') {
      if (peek(&exponent) == '+' || peek(&exponent) == '-')
        ++exponent.pos;
      const char* start = exponent.pos;
      while (isdigit((unsigned char)peek(&exponent)))
        ++exponent.pos;
      if (exponent.pos == start) {
        local = exponent;
        goto invalid;
      }
      local = exponent;
    }
  }
  size_t length = (size_t)(local.pos - cursor->pos);
  if (length > SR_SVG_MAX_VALUE_BYTES) {
    set_error(error, SR_PARSE_LIMIT, &local);
    return false;
  }
  char stack[128];
  char* token = length < sizeof(stack) ? stack : SrSVGAlloc(length + 1);
  if (!token) {
    set_error(error, SR_PARSE_OOM, cursor);
    return false;
  }
  memcpy(token, cursor->pos, length);
  token[length] = '\0';
  char* end;
  double value = strtod(token, &end);
  bool complete = end == token + length;
  if (token != stack)
    SrSVGFree(token);
  if (!complete)
    goto invalid;
  if (!isfinite(value)) {
    set_error(error, SR_PARSE_RANGE, &local);
    return false;
  }
  *cursor = local;
  *out = value;
  return true;
invalid:
  set_error(error, SR_PARSE_INVALID, &local);
  return false;
}
bool SrSVGParseCoordinate(SrSVGParseCursor* cursor, float* out,
                          SrSVGParseError* error) {
  SrSVGParseCursor local = *cursor;
  double value;
  if (!out) {
    set_error(error, SR_PARSE_INVALID, &local);
    return false;
  }
  if (!SrSVGParseScalar(&local, &value, error))
    return false;
  if (fabs(value) > FLT_MAX) {
    set_error(error, SR_PARSE_RANGE, &local);
    return false;
  }
  *cursor = local;
  *out = (float)value;
  return true;
}
static bool value_cursor(const char* data, size_t size, const void* out,
                         SrSVGParseCursor* cursor, SrSVGParseError* error) {
  reset_error(error);
  *cursor = (SrSVGParseCursor){data, data, data};
  if (!data || !out || size > SR_SVG_MAX_VALUE_BYTES) {
    set_error(error,
              size > SR_SVG_MAX_VALUE_BYTES ? SR_PARSE_LIMIT : SR_PARSE_INVALID,
              cursor);
    return false;
  }
  *cursor = SrSVGCursor(data, size);
  SrSVGCursorSpaces(cursor);
  return true;
}
static const char* parse_length_suffix(SrSVGParseCursor* cursor,
                                       SrSVGUnits* unit) {
  static const struct {
    const char* name;
    SrSVGUnits unit;
  } units[] = {{"px", SR_SVG_UNITS_PX},       {"pt", SR_SVG_UNITS_PT},
               {"pc", SR_SVG_UNITS_PC},       {"cm", SR_SVG_UNITS_CM},
               {"mm", SR_SVG_UNITS_MM},       {"in", SR_SVG_UNITS_IN},
               {"em", SR_SVG_UNITS_EM},       {"ex", SR_SVG_UNITS_EX},
               {"%", SR_SVG_UNITS_PERCENTAGE}};
  SrSVGCursorSpaces(cursor);
  if (SrSVGCursorEnd(cursor)) {
    *unit = SR_SVG_UNITS_NUMBER;
    return "";
  }
  for (size_t i = 0; i < sizeof(units) / sizeof(units[0]); ++i) {
    if (SrSVGCursorMatch(cursor, units[i].name)) {
      SrSVGCursorSpaces(cursor);
      if (!SrSVGCursorEnd(cursor))
        return NULL;
      *unit = units[i].unit;
      return units[i].name;
    }
  }
  return NULL;
}

const char* SrSVGParseNumberWithSuffix(const char* value, double* out,
                                       const char** raw_suffix) {
  if (!value || !out)
    return NULL;
  SrSVGParseCursor cursor = {value, value, NULL};
  SrSVGCursorSpaces(&cursor);
  double parsed;
  if (!SrSVGParseScalar(&cursor, &parsed, NULL) || fabs(parsed) > FLT_MAX)
    return NULL;
  const char* raw = cursor.pos;
  SrSVGUnits unit;
  const char* suffix = parse_length_suffix(&cursor, &unit);
  if (!suffix)
    return NULL;
  if (raw_suffix)
    *raw_suffix = raw;
  *out = parsed;
  return suffix;
}

bool SrSVGParseLength(const char* data, size_t size, SrSVGLength* out,
                      SrSVGParseError* error) {
  SrSVGParseCursor cursor;
  SrSVGLength result;
  if (!value_cursor(data, size, out, &cursor, error) ||
      !SrSVGParseCoordinate(&cursor, &result.value, error))
    return false;
  if (!parse_length_suffix(&cursor, &result.unit)) {
    set_error(error, SR_PARSE_INVALID, &cursor);
    return false;
  }
  *out = result;
  return true;
}

bool SrSVGParseViewBox(const char* data, size_t size, SrSVGBox* out,
                       SrSVGParseError* error) {
  SrSVGParseCursor cursor;
  float args[4];
  if (!value_cursor(data, size, out, &cursor, error))
    return false;
  for (int i = 0; i < 4; ++i) {
    if (!SrSVGParseCoordinate(&cursor, &args[i], error))
      return false;
    if (i < 3 && !SrSVGCursorSeparator(&cursor)) {
      set_error(error, SR_PARSE_INVALID, &cursor);
      return false;
    }
  }
  SrSVGCursorSpaces(&cursor);
  if (!SrSVGCursorEnd(&cursor) || args[2] < 0 || args[3] < 0) {
    set_error(error, SR_PARSE_INVALID, &cursor);
    return false;
  }
  *out = (SrSVGBox){args[0], args[1], args[2], args[3]};
  return true;
}
bool SrSVGParseTransformArgs(SrSVGParseCursor* cursor, float* out,
                             size_t max_count, size_t* count,
                             SrSVGParseError* error) {
  SrSVGParseCursor local = *cursor;
  float args[6];
  size_t n = 0;
  if (!out || !count || max_count > 6)
    goto invalid;
  // Legacy ParseTransformArgs includes the function name.
  while (isalpha((unsigned char)peek(&local)))
    ++local.pos;
  SrSVGCursorSpaces(&local);
  if (!SrSVGCursorConsume(&local, '('))
    goto invalid;
  SrSVGCursorSpaces(&local);
  while (!SrSVGCursorEnd(&local) && peek(&local) != ')') {
    if (n == max_count)
      goto invalid;
    if (!SrSVGParseCoordinate(&local, &args[n], error))
      return false;
    ++n;
    bool separator = SrSVGCursorSeparator(&local);
    if (peek(&local) != ')' && !separator)
      goto invalid;
    if (separator && peek(&local) == ')') {
      const char* p = local.pos;
      while (p > cursor->pos && isspace((unsigned char)p[-1]))
        --p;
      if (p > cursor->pos && p[-1] == ',')
        goto invalid;
    }
  }
  if (!SrSVGCursorConsume(&local, ')'))
    goto invalid;
  memcpy(out, args, n * sizeof(float));
  *cursor = local;
  *count = n;
  return true;
invalid:
  set_error(error, SR_PARSE_INVALID, &local);
  return false;
}

static bool transform_name_equals(const char* name, size_t len,
                                  const char* expected) {
  const size_t expected_len = strlen(expected);
  return len == expected_len && memcmp(name, expected, len) == 0;
}

bool SrSVGParseTransformFunction(SrSVGParseCursor* cursor, float out[6],
                                 SrSVGParseError* error) {
  SrSVGParseCursor local = *cursor;
  if (!out) {
    set_error(error, SR_PARSE_INVALID, &local);
    return false;
  }
  const char* name = local.pos;
  while (isalpha((unsigned char)peek(&local)))
    ++local.pos;
  size_t len = (size_t)(local.pos - name), n = 0;
  float args[6], result[6];
  local = *cursor;
  if (!SrSVGParseTransformArgs(&local, args, 6, &n, error))
    return false;
  if (transform_name_equals(name, len, "matrix") && n == 6)
    memcpy(result, args, sizeof(result));
  else if (transform_name_equals(name, len, "translate") && (n == 1 || n == 2))
    xform_set_translation(result, args[0], n == 1 ? 0 : args[1]);
  else if (transform_name_equals(name, len, "scale") && (n == 1 || n == 2))
    xform_set_scale(result, args[0], n == 1 ? args[0] : args[1]);
  else if (transform_name_equals(name, len, "rotate") && (n == 1 || n == 3)) {
    float temp[6];
    xform_identity(result);
    if (n == 3) {
      xform_set_translation(temp, -args[1], -args[2]);
      xform_pre_multiply(result, temp);
    }
    xform_set_rotation(temp, args[0] / 180.0f * M_PI);
    xform_pre_multiply(result, temp);
    if (n == 3) {
      xform_set_translation(temp, args[1], args[2]);
      xform_pre_multiply(result, temp);
    }
  } else if (transform_name_equals(name, len, "skewX") && n == 1)
    xform_set_skewX(result, args[0] / 180.0f * M_PI);
  else if (transform_name_equals(name, len, "skewY") && n == 1)
    xform_set_skewY(result, args[0] / 180.0f * M_PI);
  else {
    set_error(error, SR_PARSE_INVALID, &local);
    return false;
  }
  for (int i = 0; i < 6; ++i)
    if (!isfinite(result[i])) {
      set_error(error, SR_PARSE_RANGE, &local);
      return false;
    }
  memcpy(out, result, sizeof(result));
  *cursor = local;
  return true;
}
bool SrSVGParseTransform(const char* data, size_t size, float out[6],
                         SrSVGParseError* error) {
  SrSVGParseCursor cursor;
  float result[6];
  if (!value_cursor(data, size, out, &cursor, error))
    return false;
  xform_identity(result);
  while (!SrSVGCursorEnd(&cursor)) {
    float transform[6];
    if (!SrSVGParseTransformFunction(&cursor, transform, error))
      return false;
    xform_multiply(result, transform);
    for (int i = 0; i < 6; ++i)
      if (!isfinite(result[i])) {
        set_error(error, SR_PARSE_RANGE, &cursor);
        return false;
      }
    SrSVGCursorSpaces(&cursor);
    if (SrSVGCursorConsume(&cursor, ',')) {
      SrSVGCursorSpaces(&cursor);
      if (SrSVGCursorEnd(&cursor)) {
        set_error(error, SR_PARSE_INVALID, &cursor);
        return false;
      }
    }
  }
  memcpy(out, result, sizeof(result));
  return true;
}

static void* grow_path_buffer(void* data, uint64_t* capacity, uint64_t required,
                              size_t element_size, size_t initial_capacity) {
  const uint64_t limit = SIZE_MAX / element_size;
  if (required > limit)
    return NULL;
  uint64_t next = *capacity ? *capacity : initial_capacity;
  while (next < required) {
    if (next > limit / 2) {
      next = required;
      break;
    }
    next *= 2;
  }
  void* resized = SrSVGRealloc(data, next * element_size);
  if (resized)
    *capacity = next;
  return resized;
}

SrSVGParseCode SrSVGAppendPathCommand(SrPathData* path, SrPathOps op,
                                      const float* args, size_t count,
                                      size_t initial_ops_capacity) {
  if (!path || !initial_ops_capacity || (count && !args))
    return SR_PARSE_INVALID;
  for (size_t i = 0; i < count; ++i)
    if (!isfinite(args[i]))
      return SR_PARSE_RANGE;
  if (path->n_ops == UINT32_MAX || count > UINT32_MAX - path->n_args)
    return SR_PARSE_LIMIT;

  const uint64_t required_ops = (uint64_t)path->n_ops + 1;
  const uint64_t required_args = (uint64_t)path->n_args + count;
  if (required_ops > path->c_ops) {
    uint8_t* ops = grow_path_buffer(path->ops, &path->c_ops, required_ops,
                                    sizeof(*path->ops), initial_ops_capacity);
    if (!ops)
      return SR_PARSE_OOM;
    path->ops = ops;
  }
  if (required_args > path->c_args) {
    float* storage = grow_path_buffer(path->args, &path->c_args, required_args,
                                      sizeof(*path->args), 8);
    if (!storage)
      return SR_PARSE_OOM;
    path->args = storage;
  }
  // Publish the command only after both buffers have enough storage.
  path->ops[path->n_ops++] = op;
  if (count) {
    memcpy(path->args + path->n_args, args, count * sizeof(*path->args));
    path->n_args += (uint32_t)count;
  }
  return SR_PARSE_OK;
}

static void path_separator(SrSVGParseCursor* cursor) {
  SrSVGCursorSpaces(cursor);
  // A comma separates parameters, never a command from its first parameter.
  const char* previous = cursor->pos;
  while (previous > cursor->begin && isspace((unsigned char)previous[-1]))
    --previous;
  if (previous > cursor->begin && !isalpha((unsigned char)previous[-1]) &&
      SrSVGCursorConsume(cursor, ','))
    SrSVGCursorSpaces(cursor);
}
static bool path_args(SrSVGParseCursor* cursor, float* args, int n,
                      SrSVGParseError* error) {
  for (int i = 0; i < n; ++i) {
    path_separator(cursor);
    if (!SrSVGParseCoordinate(cursor, &args[i], error))
      return false;
  }
  return true;
}
bool SrSVGParsePath(const char* data, size_t size, SrPathData** out,
                    SrSVGParseError* error) {
  SrSVGParseCursor cursor;
  SrPathData builder = {0};
  if (!value_cursor(data, size, out, &cursor, error))
    return false;
  // The parameter count and placement refer to the normalized SrPathData
  // layout. A commands read the two flags and end point separately.
  static const struct {
    uint8_t count;
    uint8_t offset;
  } parameters[26] = {
      ['A' - 'A'] = {3, 2}, ['C' - 'A'] = {6, 0}, ['H' - 'A'] = {1, 0},
      ['L' - 'A'] = {2, 0}, ['M' - 'A'] = {2, 0}, ['Q' - 'A'] = {4, 0},
      ['S' - 'A'] = {4, 2}, ['T' - 'A'] = {2, 2}, ['V' - 'A'] = {1, 1},
  };
  float args[9];
  if (!SrSVGCursorEnd(&cursor) && peek(&cursor) != 'M' && peek(&cursor) != 'm')
    goto invalid_path;

  char last_cmd = 0;
  float x = 0, y = 0, control_x = 0, control_y = 0;
  float start_x = 0, start_y = 0;
  while (!SrSVGCursorEnd(&cursor)) {
    const char* loop_start = cursor.pos;
    const char cmd =
        isalpha((unsigned char)peek(&cursor)) ? *cursor.pos++ : last_cmd;
    const bool relative = cmd >= 'a' && cmd <= 'z';
    const char letter = relative ? cmd - 'a' + 'A' : cmd;
    if (letter < 'A' || letter > 'Z')
      goto invalid_path;

    const size_t input_count = parameters[letter - 'A'].count;
    const size_t offset = parameters[letter - 'A'].offset;
    if (letter != 'Z' && (!input_count || !path_args(&cursor, args + offset,
                                                     input_count, error)))
      goto invalid_path;

    char next_cmd = cmd;
    SrPathOps op;
    size_t count;
    switch (letter) {
      case 'M':
      case 'L':
        if (relative) {
          args[0] += x;
          args[1] += y;
        }
        x = control_x = args[0];
        y = control_y = args[1];
        op = SPO_LINE_TO;
        count = 2;
        if (letter == 'M') {
          start_x = x;
          start_y = y;
          next_cmd = relative ? 'l' : 'L';
          op = SPO_MOVE_TO;
        }
        break;
      case 'H':
        if (relative)
          args[0] += x;
        args[1] = y;
        x = control_x = args[0];
        op = SPO_LINE_TO;
        count = 2;
        break;
      case 'V':
        if (relative)
          args[1] += y;
        args[0] = x;
        y = control_y = args[1];
        op = SPO_LINE_TO;
        count = 2;
        break;
      case 'C':
      case 'S':
      case 'Q':
      case 'T':
        if (relative) {
          for (size_t i = offset; i < offset + input_count; i += 2) {
            args[i] += x;
            args[i + 1] += y;
          }
        }
        if (letter == 'S' || letter == 'T') {
          const char* curve_commands = letter == 'S' ? "cCsS" : "tTqQ";
          const bool reflect = strchr(curve_commands, last_cmd) != NULL;
          args[0] = reflect ? 2 * x - control_x : x;
          args[1] = reflect ? 2 * y - control_y : y;
        }
        count = letter == 'C' || letter == 'S' ? 6 : 4;
        control_x = args[count - 4];
        control_y = args[count - 3];
        x = args[count - 2];
        y = args[count - 1];
        op = count == 6 ? SPO_CUBIC_BEZ : SPO_QUAD_ARC;
        break;
      case 'A':
        for (int i = 5; i < 7; ++i) {
          path_separator(&cursor);
          const char flag = peek(&cursor);
          if (flag != '0' && flag != '1')
            goto invalid_path;
          ++cursor.pos;
          args[i] = flag == '1' ? 1.f : 0.f;
        }
        if (!path_args(&cursor, args + 7, 2, error))
          goto invalid_path;
        if (relative) {
          args[7] += x;
          args[8] += y;
        }
        args[0] = x;
        args[1] = y;
        x = control_x = args[7];
        y = control_y = args[8];
        op = SPO_ELLIPTICAL_ARC;
        count = 9;
        break;
      case 'Z':
        next_cmd = 0;
        x = control_x = start_x;
        y = control_y = start_y;
        op = SPO_CLOSE;
        count = 0;
        break;
      default:
        goto invalid_path;
    }
    SrSVGParseCode code = SrSVGAppendPathCommand(&builder, op, args, count, 32);
    if (code != SR_PARSE_OK) {
      if (error)
        error->code = code;
      goto failed;
    }
    last_cmd = next_cmd;

    // A comma here belongs to an implicit parameter group, parsed next.
    SrSVGCursorSpaces(&cursor);
    if (cursor.pos == loop_start)
      goto invalid_path;
  }
  SrPathData* path = SrSVGCalloc(1, sizeof(*path));
  if (!path) {
    set_error(error, SR_PARSE_OOM, &cursor);
    goto failed;
  }
  *path = builder;
  *out = path;
  return true;

invalid_path:
  if (!error || error->code == SR_PARSE_OK)
    set_error(error, SR_PARSE_INVALID, &cursor);
failed:
  SrSVGFree(builder.ops);
  SrSVGFree(builder.args);
  return false;
}

bool SrSVGParsePolygon(const char* data, size_t size, SrPolygon** out,
                       SrSVGParseError* error) {
  SrSVGParseCursor cursor;
  SrSVGBuffer points = {0};
  if (!value_cursor(data, size, out, &cursor, error))
    return false;
  while (!SrSVGCursorEnd(&cursor)) {
    float pair[2];
    if (!path_args(&cursor, pair, 2, error))
      goto failed;
    if (!SrSVGBufferAppend(&points, pair, sizeof(pair))) {
      set_error(error, SR_PARSE_OOM, &cursor);
      goto failed;
    }
    SrSVGCursorSpaces(&cursor);
  }
  SrPolygon* polygon = SrSVGCalloc(1, sizeof(*polygon));
  if (!polygon) {
    set_error(error, SR_PARSE_OOM, &cursor);
    goto failed;
  }
  polygon->n_points = (uint32_t)(points.size / (2 * sizeof(float)));
  polygon->capacity = (uint32_t)(points.capacity / (2 * sizeof(float)));
  polygon->points = SrSVGBufferDetach(&points);
  *out = polygon;
  return true;
failed:
  SrSVGBufferRelease(&points);
  return false;
}
