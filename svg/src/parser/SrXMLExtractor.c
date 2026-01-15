// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrXMLExtractor.h"

#include "ctype.h"
#include "stdio.h"

#define SR_XML_PARSING_STATE_TAG 1
#define SR_XML_PARSING_STATE_CONTENT 2
#define SR_XML_PARSING_STATE_ERROR 4
#define SR_XML_MAX_ATTR 256

void SrXMLParseContent(char* s, SrSVGContentCb content, void* context) {
  // Trim start white spaces
  while (*s && isspace(*s)) {
    s++;
  }
  if (!*s) {
    return;
  }
  if (content) {
    (*content)(context, s);
  }
}

void SrXMLParseElement(char* s, SrSVGStartElementCb start_element,
                       SrSVGEndElementCb end_element, void* context) {
  const char* attr[SR_XML_MAX_ATTR] = {NULL};
  int n_attr = 0;
  char* name;
  bool start = false;
  bool end = false;
  char quote = '\0';

  // Skip leading whitespaces
  while (isspace(*s)) {
    s++;
  }

  if (*s == '/') {  // end of the element
    s++;
    end = true;
  } else {
    start = true;
  }

  // Skip comments <!-- some comments --> and preprocessing <?xml
  // version="1.0"?>
  if (!*s || *s == '?' || *s == '!')
    return;

  // Parse tag name
  name = s;
  while (*s && !isspace(*s)) {
    s++;
  }
  if (*s) {
    *s++ = '\0';
  }

  // Parse attrs
  while (!end && *s && n_attr < SR_XML_MAX_ATTR) {
    char* attr_name = NULL;
    char* attr_value = NULL;

    while (*s && isspace(*s)) {
      s++;
    }
    if (!*s) {
      break;
    }

    if (*s == '/') {  // <tagName attrName="value" />
      end = true;
      break;
    }

    // extract attr name
    attr_name = s;
    while (*s && !isspace(*s) && *s != '=') {
      s++;
    }
    if (*s) {
      *s++ = '\0';
    }

    // extract attr value
    while (*s && *s != '\"' && *s != '\'') {
      s++;
    }
    if (!*s) {
      break;
    }
    // quote may be \" or \'
    quote = *s;
    // attr value start from s to the next " or '
    attr_value = ++s;
    while (*s && *s != quote) {
      s++;
    }
    if (*s) {
      *s++ = '\0';
    }

    // save attr name and value
    if (attr_name && attr_value) {
      attr[n_attr++] = attr_name;
      attr[n_attr++] = attr_value;
    }
  }
  attr[n_attr++] = NULL;
  attr[n_attr++] = NULL;

  if (start && start_element) {
    (*start_element)(context, name, attr);
  }
  if (end && end_element) {
    (*end_element)(context, name);
  }
}

bool SrXMLParseXML(char* input, SrSVGStartElementCb startElement,
                   SrSVGEndElementCb endElement, SrSVGContentCb content,
                   void* context) {
  char* s = input;
  char* mark = s;
  int state = SR_XML_PARSING_STATE_CONTENT;
  // TODO(renzhongyue): add parsing error state handling.
  while (*s) {
    if (*s == '<' && state == SR_XML_PARSING_STATE_CONTENT) {
      *s++ = '\0';
      SrXMLParseContent(mark, content, context);
      mark = s;
      state = SR_XML_PARSING_STATE_TAG;
    } else if (*s == '>' && state == SR_XML_PARSING_STATE_TAG) {
      *s++ = '\0';
      SrXMLParseElement(mark, startElement, endElement, context);
      mark = s;
      state = SR_XML_PARSING_STATE_CONTENT;
    } else {
      s++;
    }
  }
  return true;
}
