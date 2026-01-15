// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRXMLEXTRACTOR_H_
#define SVG_INCLUDE_PARSER_SRXMLEXTRACTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"

typedef void (*SrSVGStartElementCb)(void* ud, const char* el,
                                    const char** attr);
typedef void (*SrSVGEndElementCb)(void* ud, const char* el);
typedef void (*SrSVGContentCb)(void* ud, const char* s);

void SrXMLParseContent(char* s, SrSVGContentCb content, void* context);
void SrXMLParseElement(char* s, SrSVGStartElementCb start_element,
                       SrSVGEndElementCb end_element, void* context);
bool SrXMLParseXML(char* input, SrSVGStartElementCb start_element,
                   SrSVGEndElementCb end_element, SrSVGContentCb content,
                   void* context);

#ifdef __cplusplus
}
#endif

#endif  // SVG_INCLUDE_PARSER_SRXMLEXTRACTOR_H_
