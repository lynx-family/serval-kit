// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrXMLParser.h"

#include <cstring>
#include <string>
#include <vector>

#include "parser/SrXMLExtractor.h"

namespace serval {
namespace svg {
namespace parser {

struct ParsingContext {
  explicit ParsingContext(SrXMLParser* parser) : fParser(parser) {}
  ~ParsingContext() = default;

  void flushText() {
    if (!fBufferedText.empty()) {
      fParser->Text(fBufferedText.data(),
                    static_cast<int>(fBufferedText.size()));
      fBufferedText.clear();
    }
  }

  void appendText(const char* txt, size_t len) {
    fBufferedText.insert(fBufferedText.end(), txt, &txt[len]);
  }

  SrXMLParser* fParser;

 private:
  std::vector<char> fBufferedText;
};

#define HANDLER_CONTEXT(arg, name) \
  ParsingContext* name = static_cast<ParsingContext*>(arg)

void start_element_handler(void* data, const char* tag,
                           const char** attributes) {
  HANDLER_CONTEXT(data, ctx);
  ctx->flushText();

  ctx->fParser->StartElement(tag);

  for (size_t i = 0; attributes[i]; i += 2) {
    ctx->fParser->AddAttribute(attributes[i], attributes[i + 1]);
  }
}

void end_element_handler(void* data, const char* tag) {
  HANDLER_CONTEXT(data, ctx);
  ctx->flushText();

  ctx->fParser->EndElement(tag);
}

void text_handler(void* data, const char* txt) {
  HANDLER_CONTEXT(data, ctx);

  ctx->appendText(txt, strlen(txt));
}

SrXMLParser::SrXMLParser(SrXMLParserError* parserError)
    : fParser(nullptr), fError(parserError) {}

SrXMLParser::~SrXMLParser() = default;

bool SrXMLParser::parse(const char doc[], size_t len) {
  ParsingContext ctx(this);
  return SrXMLParseXML((char*)doc, start_element_handler, end_element_handler,
                       text_handler, &ctx);
}

void SrXMLParser::GetNativeErrorString(int error, std::string* str) {}

bool SrXMLParser::StartElement(const char elem[]) {
  return this->OnStartElement(elem);
}

bool SrXMLParser::AddAttribute(const char name[], const char value[]) {
  return this->OnAddAttribute(name, value);
}

bool SrXMLParser::EndElement(const char elem[]) {
  return this->OnEndElement(elem);
}

bool SrXMLParser::Text(const char text[], int len) {
  return this->OnText(text, len);
}

bool SrXMLParser::OnStartElement(const char elem[]) {
  return false;
}

bool SrXMLParser::OnAddAttribute(const char name[], const char value[]) {
  return false;
}

bool SrXMLParser::OnEndElement(const char elem[]) {
  return false;
}

bool SrXMLParser::OnText(const char text[], int len) {
  return false;
}

}  // namespace parser
}  // namespace svg
}  // namespace serval
