// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrXMLParser.h"

#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <string_view>
#include <utility>

#include "parser/SrSVGArray.h"
#include "parser/SrSVGParseLimits.h"
#include "parser/SrXMLExtractor.h"

namespace serval {
namespace svg {
namespace parser {

static bool IsTextElement(const char* tag, size_t len) {
  return (len == 4 && std::memcmp(tag, "text", len) == 0) ||
         (len == 5 && std::memcmp(tag, "tspan", len) == 0);
}

static bool IsXmlCharacter(uint32_t code_point) {
  return code_point == 0x9 || code_point == 0xA || code_point == 0xD ||
         (code_point >= 0x20 && code_point <= 0xD7FF) ||
         (code_point >= 0xE000 && code_point <= 0xFFFD) ||
         (code_point >= 0x10000 && code_point <= 0x10FFFF);
}

// Read the reference after '&'. Leave unrecognized or invalid references
// literal, as the parser has historically done.
static size_t DecodeTextReference(std::string_view reference,
                                  uint32_t& code_point) {
  if (reference.empty() || reference.front() != '#') {
    static constexpr std::pair<std::string_view, char> kNamedReferences[] = {
        {"amp;", '&'},  {"lt;", '<'},    {"gt;", '>'},
        {"quot;", '"'}, {"apos;", '\''},
    };
    for (const auto& [name, value] : kNamedReferences) {
      if (reference.substr(0, name.size()) == name) {
        code_point = value;
        return name.size();
      }
    }
    return 0;
  }

  auto digits = reference.substr(1);
  int base = 10;
  if (!digits.empty() && digits.front() == 'x') {
    digits.remove_prefix(1);
    base = 16;
  }
  const char* end = digits.data() + digits.size();
  const auto [next, error] =
      std::from_chars(digits.data(), end, code_point, base);
  if (error != std::errc{} || next == end || *next != ';' ||
      !IsXmlCharacter(code_point)) {
    return 0;
  }
  return static_cast<size_t>(next - reference.data()) + 1;
}

static void AppendUtf8(SrSVGArray<char>& text, uint32_t code_point) {
  if (code_point < 0x80) {
    text.push_back(static_cast<char>(code_point));
    return;
  }
  if (code_point < 0x800) {
    text.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
  } else if (code_point < 0x10000) {
    text.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
    text.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
  } else {
    text.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
    text.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
    text.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
  }
  text.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
}

struct ParsingContext {
  explicit ParsingContext(SrXMLParser* parser) : fParser(parser) {}
  ~ParsingContext() = default;

  bool flushText() {
    if (!fBufferedText.empty()) {
      const bool should_stop =
          fParser->Text(fBufferedText.data(), fBufferedText.size());
      fBufferedText.clear();
      buffered_source_size_ = 0;
      return should_stop;
    }
    return false;
  }

  bool appendText(const char* txt, size_t len) {
    if (text_depth == 0) {
      while (len != 0 && std::isspace(static_cast<unsigned char>(*txt))) {
        ++txt;
        --len;
      }
    }
    if (len == 0) {
      return true;
    }
    // Both line-ending normalization and reference decoding can only shrink
    // the input, so reserving the undecoded length covers every append.
    if (len > kSVGMaxAttributeBytes - buffered_source_size_ ||
        !fBufferedText.reserve(fBufferedText.size() + len))
      return false;
    buffered_source_size_ += len;
    for (size_t i = 0; i < len; ++i) {
      const char c = txt[i];
      if (text_depth != 0 && c == '&') {
        uint32_t code_point;
        const size_t consumed = DecodeTextReference(
            std::string_view(txt + i + 1, len - i - 1), code_point);
        if (consumed != 0) {
          AppendUtf8(fBufferedText, code_point);
          i += consumed;
          continue;
        }
      }
      if (text_depth != 0 && c == '\r') {
        // XML normalizes source line endings before processing references.
        fBufferedText.push_back('\n');
        if (i + 1 < len && txt[i + 1] == '\n') {
          ++i;
        }
      } else {
        fBufferedText.push_back(c);
      }
    }
    return true;
  }

  SrXMLParser* fParser;
  size_t text_depth{0};

 private:
  SrSVGArray<char> fBufferedText;
  size_t buffered_source_size_{0};
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
  if (IsTextElement(tag, tag_len)) {
    ++ctx->text_depth;
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

  if (ctx->fParser->EndElement(tag, tag_len)) {
    return true;
  }
  if (IsTextElement(tag, tag_len) && ctx->text_depth != 0) {
    --ctx->text_depth;
  }
  return false;
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
    const bool ok =
        SrXMLParseXMLRawContent(doc, len, start_element_handler,
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
