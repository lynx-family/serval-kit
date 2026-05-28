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
  explicit SrDOMParser(const SrSVGDiagnosticSink* diagnostic_sink = nullptr);
  ~SrDOMParser() override;

  bool Finish();
  SrDOM::Node* getRoot() const { return fRoot; }
  SrDOM::Node* releaseRoot() {
    auto* root = fRoot;
    fRoot = nullptr;
    return root;
  }
  SrXMLParserError fParserError;

 protected:
  bool flushAttributes();
  bool OnStartElement(const char elem[], size_t len) override;
  bool OnAddAttribute(const char name[], size_t name_len, const char value[],
                      size_t value_len) override;
  bool OnEndElement(const char elem[], size_t len) override;
  bool OnText(const char text[], size_t len) override;

 private:
  bool startCommon(const char elem[], size_t elemSize, SrDOM::Type type);

 private:
  std::vector<SrDOM::Node*> fParentStack;
  SrDOM::Node* fRoot;
  bool fNeedToFlush;
  // state needed for flushAttributes()
  std::vector<SrDOM::Attr> fAttrs;
  char* fElemName;
  SrDOM::Type fElemType;
  int fLevel;
  const SrSVGDiagnosticSink* fDiagnosticSink;
};

}  // namespace parser
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PARSER_SRDOMPARSER_H_
