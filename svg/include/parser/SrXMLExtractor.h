// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRXMLEXTRACTOR_H_
#define SVG_INCLUDE_PARSER_SRXMLEXTRACTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "stdbool.h"

typedef struct SrXMLAttrView {
  const char* name;
  size_t name_len;
  const char* value;
  size_t value_len;
} SrXMLAttrView;

typedef bool (*SrSVGStartElementCb)(void* ud, const char* el, size_t el_len,
                                    const SrXMLAttrView* attr,
                                    size_t attr_count);
typedef bool (*SrSVGEndElementCb)(void* ud, const char* el, size_t el_len);
typedef bool (*SrSVGContentCb)(void* ud, const char* s, size_t len);

typedef enum SrXMLParseResult {
  SR_XML_PARSE_RESULT_CONTINUE = 0,
  SR_XML_PARSE_RESULT_STOP = 1,
  SR_XML_PARSE_RESULT_ERROR = 2,
} SrXMLParseResult;

bool SrXMLParseContent(const char* s, size_t len, SrSVGContentCb content,
                       void* context);
SrXMLParseResult SrXMLParseElementResult(const char* s, size_t len,
                                         SrSVGStartElementCb start_element,
                                         SrSVGEndElementCb end_element,
                                         void* context);
bool SrXMLParseElement(const char* s, size_t len,
                       SrSVGStartElementCb start_element,
                       SrSVGEndElementCb end_element, void* context);
bool SrXMLParseXML(const char* input, size_t len,
                   SrSVGStartElementCb start_element,
                   SrSVGEndElementCb end_element, SrSVGContentCb content,
                   void* context);

#ifdef __cplusplus
}
#endif

#endif  // SVG_INCLUDE_PARSER_SRXMLEXTRACTOR_H_
