// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrDOMParser.h"

#include <cstdlib>
#include <cstring>

namespace serval {
namespace svg {
namespace parser {

static char* dupStr(const char src[], size_t srcLen) {
  char* dst = (char*)malloc(srcLen + 1);
  memcpy(dst, src, srcLen);
  dst[srcLen] = '\0';
  return dst;
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
  fAttrs.emplace_back(SrDOM::Attr{.fName = dupStr(name, strlen(name)),
                                  .fValue = dupStr(value, strlen(value))});
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
  this->startCommon(text, len, SrDOM::kText_Type);
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
