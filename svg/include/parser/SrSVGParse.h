// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRSVGPARSE_H_
#define SVG_INCLUDE_PARSER_SRSVGPARSE_H_

#include <stddef.h>
#include "element/SrSVGTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SrSVGParseCode {
  SR_PARSE_OK,
  SR_PARSE_INVALID,
  SR_PARSE_RANGE,
  SR_PARSE_LIMIT,
  SR_PARSE_OOM
} SrSVGParseCode;

// Errors never allocate. Output values and cursor position commit on success.
typedef struct SrSVGParseError {
  SrSVGParseCode code;
  size_t offset;
} SrSVGParseError;

typedef struct SrSVGParseCursor {
  const char* begin;
  const char* pos;
  const char* end;  // null only for the legacy NUL-terminated token adapters
} SrSVGParseCursor;

SrSVGParseCursor SrSVGCursor(const char* data, size_t size);
bool SrSVGCursorEnd(const SrSVGParseCursor* cursor);
void SrSVGCursorSpaces(SrSVGParseCursor* cursor);
bool SrSVGCursorSeparator(SrSVGParseCursor* cursor);
bool SrSVGCursorConsume(SrSVGParseCursor* cursor, char token);
bool SrSVGCursorMatch(SrSVGParseCursor* cursor, const char* token);
bool SrSVGParseScalar(SrSVGParseCursor* cursor, double* out,
                      SrSVGParseError* error);
bool SrSVGParseCoordinate(SrSVGParseCursor* cursor, float* out,
                          SrSVGParseError* error);

// Reads an animation scalar and returns a canonical unit string, or NULL on
// failure. Equal units share the same pointer. The optional raw suffix points
// into the input immediately after the number, including any whitespace.
const char* SrSVGParseNumberWithSuffix(const char* value, double* out,
                                       const char** raw_suffix);
bool SrSVGParseLength(const char* data, size_t size, SrSVGLength* out,
                      SrSVGParseError* error);
bool SrSVGParseViewBox(const char* data, size_t size, SrSVGBox* out,
                       SrSVGParseError* error);
bool SrSVGParseTransform(const char* data, size_t size, float out[6],
                         SrSVGParseError* error);
bool SrSVGParseTransformArgs(SrSVGParseCursor* cursor, float* out,
                             size_t max_count, size_t* count,
                             SrSVGParseError* error);
bool SrSVGParseTransformFunction(SrSVGParseCursor* cursor, float out[6],
                                 SrSVGParseError* error);
// Appends a complete normalized command. On failure, the stored command and
// argument counts remain valid even if one buffer was already enlarged.
SrSVGParseCode SrSVGAppendPathCommand(SrPathData* path, SrPathOps op,
                                      const float* args, size_t count,
                                      size_t initial_ops_capacity);
bool SrSVGParsePath(const char* data, size_t size, SrPathData** out,
                    SrSVGParseError* error);
bool SrSVGParsePolygon(const char* data, size_t size, SrPolygon** out,
                       SrSVGParseError* error);

#ifdef __cplusplus
}
#endif
#endif  // SVG_INCLUDE_PARSER_SRSVGPARSE_H_
