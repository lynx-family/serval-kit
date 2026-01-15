// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRXMLPARSERERROR_H_
#define SVG_INCLUDE_PARSER_SRXMLPARSERERROR_H_

#include <string>

namespace serval {
namespace svg {
namespace parser {

class SrXMLParserError {
 public:
  enum ErrorCode {
    kNoError,
    kEmptyFile,
    kUnknownElement,
    kUnknownAttributeName,
    kErrorInAttributeValue,
    kDuplicateIDs,
    kUnknownError
  };

  SrXMLParserError();
  virtual ~SrXMLParserError();

  ErrorCode GetErrorCode() const { return fCode; }
  virtual void GetErrorString(std::string& str) const;
  int GetLineNumber() const { return fLineNumber; }
  int GetNativeCode() const { return fNativeCode; }
  bool HasError() const { return fCode != kNoError || fNativeCode != -1; }
  bool HasNoun() const { return fNoun.size() > 0; }
  void Reset();
  void SetCode(ErrorCode code) { fCode = code; }
  void SetNoun(const std::string& str) { fNoun = str; }
  void SetNoun(const char* ch) { fNoun = ch; }
  void SetNoun(const char* ch, size_t len) { fNoun = ch; }

 protected:
  ErrorCode fCode;

 private:
  int fLineNumber;
  int fNativeCode;
  std::string fNoun;
  friend class SkXMLParser;
};

}  // namespace parser
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PARSER_SRXMLPARSERERROR_H_
