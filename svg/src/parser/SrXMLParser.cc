// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrXMLParser.h"

#include <cstring>
#include <new>
#include <string>
#include "parser/SrSVGArray.h"

#include "parser/SrSVGParseLimits.h"
#include "parser/SrXMLExtractor.h"

namespace serval {
namespace svg {
namespace parser {

struct ParsingContext {
  explicit ParsingContext(SrXMLParser* parser) : fParser(parser) {}
  ~ParsingContext() = default;

  bool flushText() {
    if (!fBufferedText.empty()) {
      const bool should_stop =
          fParser->Text(fBufferedText.data(), fBufferedText.size());
      fBufferedText.clear();
      return should_stop;
    }
    return false;
  }

  bool appendText(const char* txt, size_t len) {
    if (len > kSVGMaxAttributeBytes - fBufferedText.size() ||
        !fBufferedText.reserve(fBufferedText.size() + len))
      return false;
    for (size_t i = 0; i < len; ++i)
      fBufferedText.push_back(txt[i]);
    return true;
  }

  SrXMLParser* fParser;

 private:
  SrSVGArray<char> fBufferedText;
};

bool start_element_handler(void* data, const char* tag, size_t tag_len,
                           const SrXMLAttrView* attributes, size_t attr_count) {
  auto* ctx = static_cast<ParsingContext*>(data);
  if (ctx->flushText()) {
    return true;
  }

  if (ctx->fParser->StartElement(tag, tag_len)) {
    return true;
  }

  for (size_t i = 0; i < attr_count; ++i) {
    if (ctx->fParser->AddAttribute(attributes[i].name, attributes[i].name_len,
                                   attributes[i].value,
                                   attributes[i].value_len)) {
      return true;
    }
  }
  return false;
}

bool end_element_handler(void* data, const char* tag, size_t tag_len) {
  auto* ctx = static_cast<ParsingContext*>(data);
  if (ctx->flushText()) {
    return true;
  }

  return ctx->fParser->EndElement(tag, tag_len);
}

bool text_handler(void* data, const char* txt, size_t len) {
  auto* ctx = static_cast<ParsingContext*>(data);
  return !ctx->appendText(txt, len);
}

SrXMLParser::SrXMLParser(SrXMLParserError* parserError)
    : fParser(nullptr), fError(parserError) {}

SrXMLParser::~SrXMLParser() = default;

bool SrXMLParser::parse(const char doc[], size_t len) {
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS)
  try {
#endif
    if (!doc || len == 0) {
      if (fError) {
        fError->SetCode(SrXMLParserError::kEmptyFile);
        fError->SetNoun("");
      }
      return false;
    }

    if (len > kSVGMaxDocumentBytes) {
      if (fError) {
        fError->SetCode(SrXMLParserError::kUnknownError);
        fError->SetNoun("XML document exceeds byte limit");
      }
      return false;
    }
    ParsingContext ctx(this);
    const bool ok = SrXMLParseXML(doc, len, start_element_handler,
                                  end_element_handler, text_handler, &ctx);
    if (!ok && fError && !fError->HasError()) {
      fError->SetCode(SrXMLParserError::kUnknownError);
      fError->SetNoun("malformed xml");
    }
    return ok;
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS)
  } catch (const std::bad_alloc&) {
    // Cleanup below this boundary owns all partially built XML data.
    if (fError)
      fError->SetCode(SrXMLParserError::kUnknownError);
    return false;
  }
#endif
}

void SrXMLParser::GetNativeErrorString(int error, std::string* str) {}

bool SrXMLParser::StartElement(const char elem[], size_t len) {
  return this->OnStartElement(elem, len);
}

bool SrXMLParser::StartElement(const char elem[]) {
  return this->StartElement(elem, std::strlen(elem));
}

bool SrXMLParser::AddAttribute(const char name[], size_t name_len,
                               const char value[], size_t value_len) {
  return this->OnAddAttribute(name, name_len, value, value_len);
}

bool SrXMLParser::AddAttribute(const char name[], const char value[]) {
  return this->AddAttribute(name, std::strlen(name), value, std::strlen(value));
}

bool SrXMLParser::EndElement(const char elem[], size_t len) {
  return this->OnEndElement(elem, len);
}

bool SrXMLParser::EndElement(const char elem[]) {
  return this->EndElement(elem, std::strlen(elem));
}

bool SrXMLParser::Text(const char text[], size_t len) {
  return this->OnText(text, len);
}

bool SrXMLParser::OnStartElement(const char elem[], size_t len) {
  return false;
}

bool SrXMLParser::OnAddAttribute(const char name[], size_t name_len,
                                 const char value[], size_t value_len) {
  return false;
}

bool SrXMLParser::OnEndElement(const char elem[], size_t len) {
  return false;
}

bool SrXMLParser::OnText(const char text[], size_t len) {
  return false;
}

}  // namespace parser
}  // namespace svg
}  // namespace serval
