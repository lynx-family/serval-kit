// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrDOMParser.h"

#include <cstdlib>
#include <cstring>
#include <string>

#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace parser {

static char* dupStr(const char src[], size_t srcLen) {
  char* dst = (char*)malloc(srcLen + 1);
  memcpy(dst, src, srcLen);
  dst[srcLen] = '\0';
  return dst;
}

static void DestroyNodeTree(SrDOMNode* node) {
  if (!node) {
    return;
  }
  std::vector<SrDOMNode*> stack;
  stack.push_back(node);
  while (!stack.empty()) {
    SrDOMNode* current = stack.back();
    stack.pop_back();
    for (SrDOMNode* child = current->fFirstChild; child != nullptr;
         child = child->fNextSibling) {
      stack.push_back(child);
    }
    for (int i = 0; i < current->fAttrCount; i++) {
      SrDOMAttr* attr = current->fAttrs + i;
      free((void*)attr->fName);
      free((void*)attr->fValue);
    }
    free(current->fAttrs);
    free((void*)current->fName);
    free(current);
  }
}

static void FreeAttrList(std::vector<SrDOM::Attr>* attrs) {
  for (auto& attr : *attrs) {
    free((void*)attr.fName);
    free((void*)attr.fValue);
  }
  attrs->clear();
}

static bool TagNamesMatch(const char* lhs, const char* rhs, size_t rhs_len) {
  return lhs && rhs && std::strlen(lhs) == rhs_len &&
         std::memcmp(lhs, rhs, rhs_len) == 0;
}

static const char* CurrentElementName(
    const std::vector<SrDOM::Node*>& parent_stack) {
  if (parent_stack.empty()) {
    return "";
  }
  const auto* current = parent_stack.back();
  return current && current->fName ? current->fName : "";
}

static void SetMalformedXMLError(SrXMLParserError* error, const char* noun,
                                 size_t noun_len) {
  if (error) {
    error->SetCode(SrXMLParserError::kUnknownError);
    error->SetNoun(noun ? noun : "", noun ? noun_len : 0);
  }
}

SrDOMParser::SrDOMParser(const SrSVGDiagnosticSink* diagnostic_sink)
    : SrXMLParser(&fParserError), fDiagnosticSink(diagnostic_sink) {
  fRoot = nullptr;
  fElemName = nullptr;
  fLevel = 0;
  fNeedToFlush = true;
}

SrDOMParser::~SrDOMParser() {
  if (fNeedToFlush) {
    FreeAttrList(&fAttrs);
    free(fElemName);
  }
  DestroyNodeTree(fRoot);
}

bool SrDOMParser::Finish() {
  if (fLevel == 0 && fParentStack.empty() && !fNeedToFlush) {
    return true;
  }
  if (fLevel == 0 && fParentStack.empty() && fRoot == nullptr) {
    if (fError && !fError->HasError()) {
      fError->SetCode(SrXMLParserError::kEmptyFile);
      fError->SetNoun("");
    }
    SrSVGReportDiagnostic(fDiagnosticSink, SR_SVG_DIAGNOSTIC_XML_BUILD_FAILED,
                          "Encountered empty XML document.", "", 1);
    return false;
  }
  if (fError && !fError->HasError()) {
    fError->SetCode(SrXMLParserError::kUnknownError);
    fError->SetNoun(CurrentElementName(fParentStack));
  }
  SrSVGReportDiagnostic(fDiagnosticSink, SR_SVG_DIAGNOSTIC_XML_BUILD_FAILED,
                        "Encountered unterminated XML element at end of file.",
                        CurrentElementName(fParentStack), 1);
  return false;
}

bool SrDOMParser::flushAttributes() {
  if (fRoot != nullptr && fParentStack.empty()) {
    SetMalformedXMLError(fError, fElemName,
                         fElemName ? std::strlen(fElemName) : 0);
    SrSVGReportDiagnostic(fDiagnosticSink, SR_SVG_DIAGNOSTIC_XML_BUILD_FAILED,
                          "Encountered content after the root XML element.",
                          fElemName, 1);
    FreeAttrList(&fAttrs);
    free(fElemName);
    fElemName = nullptr;
    fNeedToFlush = false;
    return true;
  }

  int attrCount = (int)fAttrs.size();
  auto* node = (SrDOM::Node*)malloc(sizeof(SrDOM::Node));

  node->fAttrs = nullptr;

  if (attrCount > 0) {
    auto* attrs = (SrDOMAttr*)malloc(attrCount * sizeof(SrDOMAttr));
    node->fAttrs = attrs;
    memcpy(node->attrs(), &fAttrs.front(), attrCount * sizeof(SrDOM::Attr));
  }

  node->fName = fElemName;
  fElemName = nullptr;
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
  return false;
}

bool SrDOMParser::OnStartElement(const char elem[], size_t len) {
  return this->startCommon(elem, len, SrDOM::kElement_Type);
}

bool SrDOMParser::OnAddAttribute(const char name[], size_t name_len,
                                 const char value[], size_t value_len) {
  fAttrs.emplace_back(SrDOM::Attr{.fName = dupStr(name, name_len),
                                  .fValue = dupStr(value, value_len)});
  return false;
}

bool SrDOMParser::OnEndElement(const char elem[], size_t len) {
  if (fNeedToFlush) {
    if (this->flushAttributes()) {
      return true;
    }
  }
  fNeedToFlush = false;
  if (fLevel <= 0 || fParentStack.empty()) {
    if (fError) {
      fError->SetCode(SrXMLParserError::kUnknownError);
      fError->SetNoun(elem, len);
    }
    SrSVGReportDiagnostic(
        fDiagnosticSink, SR_SVG_DIAGNOSTIC_XML_UNEXPECTED_CLOSE_TAG,
        "Encountered unexpected closing tag while XML stack was empty.",
        std::string(elem, len).c_str(), 1);
    return true;
  }

  const SrDOM::Node* current = fParentStack.back();
  if (!TagNamesMatch(current->fName, elem, len)) {
    if (fError) {
      fError->SetCode(SrXMLParserError::kUnknownError);
      fError->SetNoun(elem, len);
    }
    SrSVGReportDiagnostic(fDiagnosticSink,
                          SR_SVG_DIAGNOSTIC_XML_UNEXPECTED_CLOSE_TAG,
                          "Encountered mismatched closing tag.",
                          std::string(elem, len).c_str(), 1);
    return true;
  }

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

bool SrDOMParser::OnText(const char text[], size_t len) {
  if (len == 0) {
    return false;
  }
  if (this->startCommon(text, len, SrDOM::kText_Type)) {
    return true;
  }
  return this->SrDOMParser::OnEndElement(fElemName, len);
}

bool SrDOMParser::startCommon(const char elem[], size_t elemSize,
                              SrDOM::Type type) {
  if (fLevel > 0 && fNeedToFlush) {
    if (this->flushAttributes()) {
      return true;
    }
  }
  if (fLevel == 0 && fRoot != nullptr) {
    SetMalformedXMLError(fError, elem, elemSize);
    SrSVGReportDiagnostic(fDiagnosticSink, SR_SVG_DIAGNOSTIC_XML_BUILD_FAILED,
                          "Encountered content after the root XML element.",
                          std::string(elem, elemSize).c_str(), 1);
    return true;
  }
  fNeedToFlush = true;
  fElemName = dupStr(elem, elemSize);
  fElemType = type;
  ++fLevel;
  return false;
}

}  // namespace parser
}  // namespace svg
}  // namespace serval
