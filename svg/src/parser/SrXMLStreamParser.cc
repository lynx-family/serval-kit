// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrXMLStreamParser.h"

#include "parser/SrXMLExtractor.h"
#include "parser/SrXMLParserError.h"

namespace serval {
namespace svg {
namespace parser {

namespace {

struct StreamParsingContext {
  explicit StreamParsingContext(SrXMLParser* parser) : parser(parser) {}
  SrXMLParser* parser;
};

bool StartElementHandler(void* data, const char* tag,
                         size_t tag_len,
                         const SrXMLAttrView* attributes,
                         size_t attr_count) {
  auto* ctx = static_cast<StreamParsingContext*>(data);
  if (ctx->parser->StartElement(tag, tag_len)) {
    return true;
  }
  for (size_t i = 0; i < attr_count; ++i) {
    if (ctx->parser->AddAttribute(attributes[i].name, attributes[i].name_len,
                                  attributes[i].value,
                                  attributes[i].value_len)) {
      return true;
    }
  }
  return false;
}

bool EndElementHandler(void* data, const char* tag, size_t tag_len) {
  auto* ctx = static_cast<StreamParsingContext*>(data);
  return ctx->parser->EndElement(tag, tag_len);
}

bool TextHandler(void* data, const char* text, size_t len) {
  auto* ctx = static_cast<StreamParsingContext*>(data);
  return ctx->parser->Text(text, len);
}

}  // namespace

SrXMLStreamParser::SrXMLStreamParser(SrXMLParser* parser) : parser_(parser) {}

SrXMLStreamParser::~SrXMLStreamParser() = default;

bool SrXMLStreamParser::Append(const char* data, size_t len) {
  if (has_error_) {
    return false;
  }
  if (len == 0) {
    return true;
  }
  if (!data || !parser_) {
    return SetMalformedError("invalid input");
  }

  for (size_t i = 0; i < len; ++i) {
    const char token = data[i];
    if (state_ == State::kContent) {
      if (token == '<') {
        if (!FlushText()) {
          return false;
        }
        tag_buffer_.clear();
        quote_ = '\0';
        state_ = State::kTag;
      } else {
        text_buffer_.push_back(token);
      }
      continue;
    }

    if (token == '>' && ShouldCloseTagHere(token)) {
      if (!ParseCurrentTag()) {
        return false;
      }
      tag_buffer_.clear();
      quote_ = '\0';
      state_ = State::kContent;
      continue;
    }

    if (token == '"' || token == '\'') {
      if (quote_ == '\0') {
        quote_ = token;
      } else if (quote_ == token) {
        quote_ = '\0';
      }
    }
    tag_buffer_.push_back(token);
  }
  return true;
}

bool SrXMLStreamParser::Finish() {
  if (has_error_) {
    return false;
  }
  if (state_ == State::kTag) {
    return SetMalformedError(tag_buffer_.empty() ? "unterminated tag"
                                                 : tag_buffer_.c_str());
  }
  return FlushText();
}

bool SrXMLStreamParser::FlushText() {
  if (text_buffer_.empty()) {
    return true;
  }
  StreamParsingContext ctx(parser_);
  const bool should_stop = SrXMLParseContent(
      text_buffer_.data(), text_buffer_.size(), TextHandler, &ctx);
  text_buffer_.clear();
  if (should_stop) {
    has_error_ = true;
    if (parser_ && parser_->fError && !parser_->fError->HasError()) {
      parser_->fError->SetCode(SrXMLParserError::kUnknownError);
      parser_->fError->SetNoun("text");
    }
    return false;
  }
  return true;
}

bool SrXMLStreamParser::ParseCurrentTag() {
  StreamParsingContext ctx(parser_);
  const SrXMLParseResult result = SrXMLParseElementResult(
      tag_buffer_.data(), tag_buffer_.size(), StartElementHandler,
      EndElementHandler, &ctx);
  if (result == SR_XML_PARSE_RESULT_CONTINUE) {
    return true;
  }
  has_error_ = true;
  if (parser_ && parser_->fError && !parser_->fError->HasError()) {
    parser_->fError->SetCode(SrXMLParserError::kUnknownError);
    parser_->fError->SetNoun(result == SR_XML_PARSE_RESULT_ERROR
                                 ? "malformed xml"
                                 : tag_buffer_.c_str());
  }
  return false;
}

bool SrXMLStreamParser::SetMalformedError(const char* noun) {
  has_error_ = true;
  if (parser_ && parser_->fError && !parser_->fError->HasError()) {
    parser_->fError->SetCode(SrXMLParserError::kUnknownError);
    parser_->fError->SetNoun(noun ? noun : "malformed xml");
  }
  return false;
}

bool SrXMLStreamParser::ShouldCloseTagHere(char token) const {
  if (token != '>' || quote_ != '\0') {
    return false;
  }
  if (tag_buffer_.empty()) {
    return true;
  }
  if (tag_buffer_.size() >= 3 && tag_buffer_[0] == '!' &&
      tag_buffer_[1] == '-' && tag_buffer_[2] == '-') {
    return tag_buffer_.size() >= 4 &&
           tag_buffer_[tag_buffer_.size() - 1] == '-' &&
           tag_buffer_[tag_buffer_.size() - 2] == '-';
  }
  if (tag_buffer_[0] == '?') {
    return tag_buffer_.back() == '?';
  }
  return true;
}

}  // namespace parser
}  // namespace svg
}  // namespace serval
