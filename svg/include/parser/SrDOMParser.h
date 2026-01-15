// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRDOMPARSER_H_
#define SVG_INCLUDE_PARSER_SRDOMPARSER_H_

#include "parser/SrDOM.h"
#include "parser/SrXMLParser.h"

#include <vector>

namespace serval {
namespace svg {
namespace parser {

class SrDOMParser : public SrXMLParser {
 public:
  SrDOMParser();

  SrDOM::Node* getRoot() const { return fRoot; }
  SrXMLParserError fParserError;

 protected:
  void flushAttributes();
  bool OnStartElement(const char elem[]) override;
  bool OnAddAttribute(const char name[], const char value[]) override;
  bool OnEndElement(const char elem[]) override;
  bool OnText(const char text[], int len) override;

 private:
  void startCommon(const char elem[], size_t elemSize, SrDOM::Type type);

 private:
  std::vector<SrDOM::Node*> fParentStack;
  SrDOM::Node* fRoot;
  bool fNeedToFlush;
  // state needed for flushAttributes()
  std::vector<SrDOM::Attr> fAttrs;
  char* fElemName;
  SrDOM::Type fElemType;
  int fLevel;
};

}  // namespace parser
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PARSER_SRDOMPARSER_H_
