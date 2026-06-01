// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRXMLSTREAMPARSER_H_
#define SVG_INCLUDE_PARSER_SRXMLSTREAMPARSER_H_

#include <cstddef>
#include <string>
#include <vector>

#include "parser/SrXMLParser.h"

namespace serval {
namespace svg {
namespace parser {

class SrXMLStreamParser {
 public:
  explicit SrXMLStreamParser(SrXMLParser* parser);
  ~SrXMLStreamParser();

  bool Append(const char* data, size_t len);
  bool Finish();
  bool HasError() const { return has_error_; }

 private:
  enum class State { kContent, kTag };

  bool FlushText();
  bool ParseCurrentTag();
  bool SetMalformedError(const char* noun);
  bool ShouldCloseTagHere(char token) const;

  SrXMLParser* parser_{nullptr};
  State state_{State::kContent};
  char quote_{'\0'};
  bool has_error_{false};
  std::vector<char> text_buffer_;
  std::string tag_buffer_;
};

}  // namespace parser
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PARSER_SRXMLSTREAMPARSER_H_
