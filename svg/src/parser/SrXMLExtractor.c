// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrXMLExtractor.h"

#include "ctype.h"

#define SR_XML_PARSING_STATE_TAG 1
#define SR_XML_PARSING_STATE_CONTENT 2
#define SR_XML_MAX_ATTR 256

typedef enum SrXMLParseResult {
  SR_XML_PARSE_RESULT_CONTINUE = 0,
  SR_XML_PARSE_RESULT_STOP = 1,
  SR_XML_PARSE_RESULT_ERROR = 2,
} SrXMLParseResult;

static const char* SrXMLSkipSpaces(const char* cursor, const char* end) {
  while (cursor < end && isspace((unsigned char)*cursor)) {
    cursor++;
  }
  return cursor;
}

static bool SrXMLShouldCloseTagHere(const char* mark, const char* cursor,
                                    char quote) {
  if (quote != '\0') {
    return false;
  }
  if (cursor <= mark) {
    return true;
  }
  if (cursor - mark >= 3 && mark[0] == '!' && mark[1] == '-' &&
      mark[2] == '-') {
    return cursor - mark >= 5 && cursor[-1] == '-' && cursor[-2] == '-';
  }
  if (mark[0] == '?') {
    return cursor[-1] == '?';
  }
  return true;
}

bool SrXMLParseContent(const char* s, size_t len, SrSVGContentCb content,
                       void* context) {
  const char* end = s + len;
  s = SrXMLSkipSpaces(s, end);
  if (s == end || !content) {
    return false;
  }
  return (*content)(context, s, (size_t)(end - s));
}

static SrXMLParseResult SrXMLParseElementInternal(
    const char* s, size_t len, SrSVGStartElementCb start_element,
    SrSVGEndElementCb end_element, void* context) {
  const char* cursor = s;
  const char* end = s + len;
  const char* name = NULL;
  size_t name_len = 0;
  SrXMLAttrView attr[SR_XML_MAX_ATTR];
  size_t n_attr = 0;
  bool start = false;
  bool end_tag = false;

  cursor = SrXMLSkipSpaces(cursor, end);
  if (cursor == end) {
    return SR_XML_PARSE_RESULT_CONTINUE;
  }

  if (*cursor == '/') {
    end_tag = true;
    cursor = SrXMLSkipSpaces(cursor + 1, end);
  } else {
    start = true;
  }

  if (cursor == end) {
    return SR_XML_PARSE_RESULT_ERROR;
  }

  if (*cursor == '?' || *cursor == '!') {
    return SR_XML_PARSE_RESULT_CONTINUE;
  }

  name = cursor;
  while (cursor < end && !isspace((unsigned char)*cursor) && *cursor != '/') {
    if (*cursor == '=') {
      return SR_XML_PARSE_RESULT_ERROR;
    }
    cursor++;
  }
  name_len = (size_t)(cursor - name);
  if (name_len == 0) {
    return SR_XML_PARSE_RESULT_ERROR;
  }

  cursor = SrXMLSkipSpaces(cursor, end);
  if (end_tag) {
    if (cursor != end) {
      return SR_XML_PARSE_RESULT_ERROR;
    }
    if (end_element && (*end_element)(context, name, name_len)) {
      return SR_XML_PARSE_RESULT_STOP;
    }
    return SR_XML_PARSE_RESULT_CONTINUE;
  }

  while (cursor < end) {
    const char* attr_name = NULL;
    const char* attr_value = NULL;
    size_t attr_name_len = 0;
    size_t attr_value_len = 0;
    char quote = '\0';

    if (*cursor == '/') {
      cursor = SrXMLSkipSpaces(cursor + 1, end);
      if (cursor != end) {
        return SR_XML_PARSE_RESULT_ERROR;
      }
      end_tag = true;
      break;
    }

    if (n_attr >= SR_XML_MAX_ATTR) {
      return SR_XML_PARSE_RESULT_ERROR;
    }

    attr_name = cursor;
    while (cursor < end && !isspace((unsigned char)*cursor) && *cursor != '=' &&
           *cursor != '/') {
      cursor++;
    }
    attr_name_len = (size_t)(cursor - attr_name);
    if (attr_name_len == 0) {
      return SR_XML_PARSE_RESULT_ERROR;
    }

    cursor = SrXMLSkipSpaces(cursor, end);
    if (cursor == end || *cursor != '=') {
      return SR_XML_PARSE_RESULT_ERROR;
    }

    cursor = SrXMLSkipSpaces(cursor + 1, end);
    if (cursor == end || (*cursor != '"' && *cursor != '\'')) {
      return SR_XML_PARSE_RESULT_ERROR;
    }

    quote = *cursor++;
    attr_value = cursor;
    while (cursor < end && *cursor != quote) {
      cursor++;
    }
    if (cursor == end) {
      return SR_XML_PARSE_RESULT_ERROR;
    }

    attr_value_len = (size_t)(cursor - attr_value);
    attr[n_attr].name = attr_name;
    attr[n_attr].name_len = attr_name_len;
    attr[n_attr].value = attr_value;
    attr[n_attr].value_len = attr_value_len;
    n_attr++;

    cursor = SrXMLSkipSpaces(cursor + 1, end);
  }

  if (start_element &&
      (*start_element)(context, name, name_len, attr, n_attr)) {
    return SR_XML_PARSE_RESULT_STOP;
  }
  if (end_tag && end_element && (*end_element)(context, name, name_len)) {
    return SR_XML_PARSE_RESULT_STOP;
  }
  return SR_XML_PARSE_RESULT_CONTINUE;
}

bool SrXMLParseElement(const char* s, size_t len,
                       SrSVGStartElementCb start_element,
                       SrSVGEndElementCb end_element, void* context) {
  return SrXMLParseElementInternal(s, len, start_element, end_element,
                                   context) == SR_XML_PARSE_RESULT_STOP;
}

bool SrXMLParseXML(const char* input, size_t len,
                   SrSVGStartElementCb startElement,
                   SrSVGEndElementCb endElement, SrSVGContentCb content,
                   void* context) {
  const char* cursor = input;
  const char* mark = input;
  const char* end = input + len;
  int state = SR_XML_PARSING_STATE_CONTENT;
  char quote = '\0';

  while (cursor < end) {
    if (*cursor == '<' && state == SR_XML_PARSING_STATE_CONTENT) {
      if (SrXMLParseContent(mark, (size_t)(cursor - mark), content, context)) {
        return false;
      }
      cursor++;
      mark = cursor;
      state = SR_XML_PARSING_STATE_TAG;
      continue;
    }

    if (state == SR_XML_PARSING_STATE_TAG &&
        (*cursor == '"' || *cursor == '\'')) {
      if (quote == '\0') {
        quote = *cursor;
      } else if (quote == *cursor) {
        quote = '\0';
      }
      cursor++;
      continue;
    }

    if (*cursor == '>' && state == SR_XML_PARSING_STATE_TAG &&
        SrXMLShouldCloseTagHere(mark, cursor, quote)) {
      SrXMLParseResult result = SrXMLParseElementInternal(
          mark, (size_t)(cursor - mark), startElement, endElement, context);
      if (result != SR_XML_PARSE_RESULT_CONTINUE) {
        return false;
      }
      cursor++;
      mark = cursor;
      state = SR_XML_PARSING_STATE_CONTENT;
      quote = '\0';
      continue;
    }

    cursor++;
  }

  if (state == SR_XML_PARSING_STATE_CONTENT) {
    return !SrXMLParseContent(mark, (size_t)(end - mark), content, context);
  }

  return false;
}
