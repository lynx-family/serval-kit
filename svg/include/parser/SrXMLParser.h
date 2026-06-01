// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRXMLPARSER_H_
#define SVG_INCLUDE_PARSER_SRXMLPARSER_H_

#include <string>

#include "parser/SrXMLParserError.h"

class SrDOM;
struct SrDOMNode;

namespace serval {
namespace svg {
namespace parser {

class SrXMLStreamParser;

class SrXMLParser {
 public:
  SrXMLParser(SrXMLParserError* parserError = nullptr);
  virtual ~SrXMLParser();

  /** Returns true for success
   */
  bool parse(const char doc[], size_t len);
  // bool parse(const SrDOM&, const SrDOMNode*);

  static void GetNativeErrorString(int nativeErrorCode, std::string* str);

 protected:
  // override in subclasses; return true to stop parsing
  virtual bool OnStartElement(const char elem[], size_t len);
  virtual bool OnAddAttribute(const char name[], size_t name_len,
                              const char value[], size_t value_len);
  virtual bool OnEndElement(const char elem[], size_t len);
  virtual bool OnText(const char text[], size_t len);

 public:
  // public for ported implementation, not meant for clients to call
  bool StartElement(const char elem[], size_t len);
  bool StartElement(const char elem[]);
  bool AddAttribute(const char name[], size_t name_len, const char value[],
                    size_t value_len);
  bool AddAttribute(const char name[], const char value[]);
  bool EndElement(const char elem[], size_t len);
  bool EndElement(const char elem[]);
  bool Text(const char text[], size_t len);
  void* fParser;

 protected:
  SrXMLParserError* fError;
  friend class SrXMLStreamParser;

 private:
  void ReportError(void* parser);
};

}  // namespace parser
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PARSER_SRXMLPARSER_H_
