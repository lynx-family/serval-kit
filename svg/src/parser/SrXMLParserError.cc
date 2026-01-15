// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrXMLParserError.h"
#include "parser/SrXMLParser.h"

namespace serval {
namespace svg {
namespace parser {

static char const* const gErrorStrings[] = {
    "empty or missing file ",    "unknown element ", "unknown attribute name ",
    "error in attribute value ", "duplicate ID ",    "unknown error "};

SrXMLParserError::SrXMLParserError()
    : fCode(kNoError), fLineNumber(-1), fNativeCode(-1) {
  Reset();
}

SrXMLParserError::~SrXMLParserError() {
  // need a virtual destructor for our subclasses
}

void SrXMLParserError::GetErrorString(std::string& str) const {
  std::string temp;
  if (fCode != kNoError) {
    if ((unsigned)fCode < sizeof(gErrorStrings) / sizeof(char*)) {
      temp = gErrorStrings[fCode - 1];
    }
    temp.append(fNoun);
  } else {
    SrXMLParser::GetNativeErrorString(fNativeCode, &temp);
  }
  str.append(temp);
}

void SrXMLParserError::Reset() {
  fCode = kNoError;
  fLineNumber = -1;
  fNativeCode = -1;
}

}  // namespace parser
}  // namespace svg
}  // namespace serval
