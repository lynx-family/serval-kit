// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrDOMParser.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

namespace serval {
namespace svg {
namespace parser {

static char* dupStr(const char src[], size_t srcLen) {
  char* dst = (char*)malloc(srcLen + 1);
  memcpy(dst, src, srcLen);
  dst[srcLen] = '\0';
  return dst;
}

static std::string DecodeXmlEntities(const char* src, size_t len) {
  std::string out;
  out.reserve(len);
  size_t i = 0;
  while (i < len) {
    char c = src[i];
    if (c != '&') {
      out.push_back(c);
      i++;
      continue;
    }

    size_t semi = i + 1;
    while (semi < len && src[semi] != ';' && semi - i <= 16) {
      semi++;
    }
    if (semi >= len || src[semi] != ';') {
      out.push_back('&');
      i++;
      continue;
    }

    const size_t entity_start = i + 1;
    const size_t entity_len = semi - entity_start;
    if (entity_len == 0) {
      out.push_back('&');
      i++;
      continue;
    }

    auto append_utf8 = [&](uint32_t codepoint) {
      if (codepoint <= 0x7F) {
        out.push_back(static_cast<char>(codepoint));
      } else if (codepoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
      } else if (codepoint <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
      } else if (codepoint <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
      }
    };

    bool decoded = false;
    const char* e = src + entity_start;
    if (entity_len == 3 && std::memcmp(e, "amp", 3) == 0) {
      out.push_back('&');
      decoded = true;
    } else if (entity_len == 2 && std::memcmp(e, "lt", 2) == 0) {
      out.push_back('<');
      decoded = true;
    } else if (entity_len == 2 && std::memcmp(e, "gt", 2) == 0) {
      out.push_back('>');
      decoded = true;
    } else if (entity_len == 4 && std::memcmp(e, "quot", 4) == 0) {
      out.push_back('"');
      decoded = true;
    } else if (entity_len == 4 && std::memcmp(e, "apos", 4) == 0) {
      out.push_back('\'');
      decoded = true;
    } else if (entity_len >= 2 && e[0] == '#') {
      uint32_t codepoint = 0;
      bool ok = false;
      if (entity_len >= 3 && (e[1] == 'x' || e[1] == 'X')) {
        ok = true;
        for (size_t k = 2; k < entity_len; k++) {
          const char ch = e[k];
          uint32_t v = 0;
          if (ch >= '0' && ch <= '9') {
            v = static_cast<uint32_t>(ch - '0');
          } else if (ch >= 'a' && ch <= 'f') {
            v = static_cast<uint32_t>(ch - 'a' + 10);
          } else if (ch >= 'A' && ch <= 'F') {
            v = static_cast<uint32_t>(ch - 'A' + 10);
          } else {
            ok = false;
            break;
          }
          codepoint = (codepoint << 4) + v;
        }
      } else {
        ok = true;
        for (size_t k = 1; k < entity_len; k++) {
          const char ch = e[k];
          if (ch < '0' || ch > '9') {
            ok = false;
            break;
          }
          codepoint = codepoint * 10 + static_cast<uint32_t>(ch - '0');
        }
      }
      if (ok && codepoint <= 0x10FFFF) {
        append_utf8(codepoint);
        decoded = true;
      }
    }

    if (!decoded) {
      out.append(src + i, semi - i + 1);
    }
    i = semi + 1;
  }
  return out;
}

SrDOMParser::SrDOMParser() : SrXMLParser(&fParserError) {
  fRoot = nullptr;
  fLevel = 0;
  fNeedToFlush = true;
}

void SrDOMParser::flushAttributes() {
  int attrCount = (int)fAttrs.size();
  auto* node = (SrDOM::Node*)malloc(sizeof(SrDOM::Node));

  node->fAttrs = nullptr;

  if (attrCount > 0) {
    auto* attrs = (SrDOMAttr*)malloc(attrCount * sizeof(SrDOMAttr));
    node->fAttrs = attrs;
    memcpy(node->attrs(), &fAttrs.front(), attrCount * sizeof(SrDOM::Attr));
  }

  node->fName = fElemName;
  node->fFirstChild = nullptr;
  node->fAttrCount = static_cast<int16_t>(attrCount);
  node->fType = fElemType;

  if (fRoot == nullptr) {
    node->fNextSibling = nullptr;
    fRoot = node;
  } else {  // this adds siblings in reverse order. gets corrected in
            // onEndElement()
    SrDOM::Node* parent = fParentStack.back();
    node->fNextSibling = parent->fFirstChild;
    parent->fFirstChild = node;
  }
  fParentStack.emplace_back();
  fParentStack.back() = node;

  fAttrs.clear();
}

bool SrDOMParser::OnStartElement(const char elem[]) {
  this->startCommon(elem, strlen(elem), SrDOM::kElement_Type);
  return false;
}

bool SrDOMParser::OnAddAttribute(const char name[], const char value[]) {
  std::string decoded = DecodeXmlEntities(value, strlen(value));
  fAttrs.emplace_back(
      SrDOM::Attr{.fName = dupStr(name, strlen(name)),
                  .fValue = dupStr(decoded.c_str(), decoded.size())});
  return false;
}

bool SrDOMParser::OnEndElement(const char elem[]) {
  if (fNeedToFlush) {
    this->flushAttributes();
  }
  fNeedToFlush = false;
  --fLevel;
  SrDOM::Node* parent = fParentStack.back();
  fParentStack.pop_back();
  SrDOM::Node* child = parent->fFirstChild;
  SrDOM::Node* prev = nullptr;
  while (child) {  // A3(First) -> A2 -> A1 -> A0
    SrDOM::Node* next = child->fNextSibling;
    child->fNextSibling = prev;
    prev = child;
    child = next;
  }  // A0(First) -> A1 -> A2 -> A3
  parent->fFirstChild = prev;
  return false;
}

bool SrDOMParser::OnText(const char text[], int len) {
  if (len == 0) {
    return false;
  }
  std::string decoded = DecodeXmlEntities(text, static_cast<size_t>(len));
  this->startCommon(decoded.c_str(), decoded.size(), SrDOM::kText_Type);
  this->SrDOMParser::OnEndElement(fElemName);
  return true;
}

void SrDOMParser::startCommon(const char elem[], size_t elemSize,
                              SrDOM::Type type) {
  if (fLevel > 0 && fNeedToFlush) {
    this->flushAttributes();
  }
  fNeedToFlush = true;
  fElemName = dupStr(elem, elemSize);
  fElemType = type;
  ++fLevel;
}

}  // namespace parser
}  // namespace svg
}  // namespace serval
