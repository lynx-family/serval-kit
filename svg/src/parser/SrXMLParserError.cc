// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrXMLParserError.h"
#include <cstring>
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
  SrSVGBufferRelease(&fNounBuffer);
}

SrXMLParserError::SrXMLParserError(const SrXMLParserError& other)
    : SrXMLParserError() {
  *this = other;
}
SrXMLParserError& SrXMLParserError::operator=(const SrXMLParserError& other) {
  if (this != &other) {
    fCode = other.fCode;
    fLineNumber = other.fLineNumber;
    fNativeCode = other.fNativeCode;
    SetNoun(static_cast<const char*>(other.fNounBuffer.data),
            other.HasNoun() ? other.fNounBuffer.size - 1 : 0);
  }
  return *this;
}
void SrXMLParserError::SetNoun(const char* ch) {
  SetNoun(ch, ch ? strlen(ch) : 0);
}
void SrXMLParserError::SetNoun(const char* ch, size_t len) {
  // Copy before clearing the lazily materialized string: ch may be GetNoun().data().
  fNounBuffer.size = 0;
  if (!ch || !len || len > 1024 * 1024 ||
      !SrSVGBufferReserve(&fNounBuffer, len + 1)) {
    fNoun.clear();
    return;
  }
  memcpy(fNounBuffer.data, ch, len);
  static_cast<char*>(fNounBuffer.data)[len] = '\0';
  fNounBuffer.size = len + 1;
  fNoun.clear();
}
const std::string& SrXMLParserError::GetNoun() const {
  if (HasNoun() && fNoun.empty())
    fNoun.assign(static_cast<const char*>(fNounBuffer.data),
                 fNounBuffer.size - 1);
  return fNoun;
}

void SrXMLParserError::GetErrorString(std::string& str) const {
  std::string temp;
  if (fCode != kNoError) {
    if ((unsigned)fCode < sizeof(gErrorStrings) / sizeof(char*)) {
      temp = gErrorStrings[fCode - 1];
    }
    if (HasNoun())
      temp.append(static_cast<const char*>(fNounBuffer.data),
                  fNounBuffer.size - 1);
  } else {
    SrXMLParser::GetNativeErrorString(fNativeCode, &temp);
  }
  str.append(temp);
}

void SrXMLParserError::Reset() {
  fCode = kNoError;
  fLineNumber = -1;
  fNativeCode = -1;
  fNounBuffer.size = 0;
  fNoun.clear();
}

}  // namespace parser
}  // namespace svg
}  // namespace serval
