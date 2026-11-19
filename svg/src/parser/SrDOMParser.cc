// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrDOMParser.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include "parser/SrSVGMemory.h"

#include "element/SrSVGTypes.h"
#include "parser/SrSVGParseLimits.h"

namespace serval {
namespace svg {
namespace parser {

static char* dupStr(const char src[], size_t srcLen) {
  char* dst = (char*)SrSVGAlloc(srcLen + 1);
  if (!dst)
    return nullptr;
  memcpy(dst, src, srcLen);
  dst[srcLen] = '\0';
  return dst;
}

static void DestroyNodeTree(SrDOMNode* node) {
  // Thread children into the work list. Cleanup must not allocate or throw.
  while (node) {
    if (node->fFirstChild) {
      SrDOMNode* child = node->fFirstChild;
      node->fFirstChild = child->fNextSibling;
      child->fNextSibling = node;
      node = child;
      continue;
    }
    SrDOMNode* next = node->fNextSibling;
    for (int i = 0; i < node->fAttrCount; ++i) {
      SrSVGFree((void*)node->fAttrs[i].fName);
      SrSVGFree((void*)node->fAttrs[i].fValue);
    }
    SrSVGFree(node->fAttrs);
    SrSVGFree((void*)node->fName);
    SrSVGFree(node);
    node = next;
  }
}

static void FreeAttrList(SrSVGArray<SrDOM::Attr>* attrs) {
  for (auto& attr : *attrs) {
    SrSVGFree((void*)attr.fName);
    SrSVGFree((void*)attr.fValue);
  }
  attrs->clear();
}

static bool TagNamesMatch(const char* lhs, const char* rhs, size_t rhs_len) {
  return lhs && rhs && std::strlen(lhs) == rhs_len &&
         std::memcmp(lhs, rhs, rhs_len) == 0;
}

static const char* CurrentElementName(
    const SrSVGArray<SrDOM::Node*>& parent_stack) {
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
    SrSVGFree(fElemName);
  }
  DestroyNodeTree(fRoot);
}

bool SrDOMParser::fail(const char* reason) {
  fFailed = true;
  SetMalformedXMLError(fError, reason, std::strlen(reason));
  SrSVGReportDiagnostic(fDiagnosticSink, SR_SVG_DIAGNOSTIC_XML_BUILD_FAILED,
                        reason, "", 1);
  return true;
}

bool SrDOMParser::reserveBytes(size_t bytes) {
  if (bytes > kSVGMaxDOMBytes - fDOMBytes)
    return false;
  fDOMBytes += bytes;
  return true;
}

bool SrDOMParser::Finish() {
  if (fFailed || fParserError.HasError())
    return false;
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
  if (fFailed)
    return true;
  if (!fElemName)
    return fail("Closing tag has no pending XML element");
  if (fRoot != nullptr && fParentStack.empty()) {
    SetMalformedXMLError(fError, fElemName,
                         fElemName ? std::strlen(fElemName) : 0);
    SrSVGReportDiagnostic(fDiagnosticSink, SR_SVG_DIAGNOSTIC_XML_BUILD_FAILED,
                          "Encountered content after the root XML element.",
                          fElemName, 1);
    FreeAttrList(&fAttrs);
    SrSVGFree(fElemName);
    fElemName = nullptr;
    fNeedToFlush = false;
    return true;
  }

  if (fFailed)
    return true;
  // Complete fallible growth before transferring any string ownership.
  if (!fParentStack.reserve(fParentStack.size() + 1))
    return fail("Cannot allocate XML parent stack");
  const int attrCount = (int)fAttrs.size();
  if (!reserveBytes(sizeof(SrDOM::Node) + attrCount * sizeof(SrDOMAttr)))
    return fail("XML DOM exceeds byte limit");
  auto* node = (SrDOM::Node*)SrSVGAlloc(sizeof(SrDOM::Node));
  if (!node)
    return fail("Cannot allocate XML node");
  node->fAttrs = nullptr;

  if (attrCount > 0) {
    auto* attrs = (SrDOMAttr*)SrSVGAlloc(attrCount * sizeof(SrDOMAttr));
    if (!attrs) {
      SrSVGFree(node);
      return fail("Cannot allocate XML attributes");
    }
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
  // Capacity is already reserved above; this cannot allocate.
  fParentStack.push_back(node);

  fAttrs.clear();
  return false;
}

bool SrDOMParser::OnStartElement(const char elem[], size_t len) {
  return this->startCommon(elem, len, SrDOM::kElement_Type);
}

bool SrDOMParser::OnAddAttribute(const char name[], size_t name_len,
                                 const char value[], size_t value_len) {
  if (fFailed)
    return true;
  if (name_len > kSVGMaxAttributeBytes || value_len > kSVGMaxAttributeBytes ||
      fAttrs.size() >= 256 || !reserveBytes(name_len + value_len + 2))
    return fail("XML attribute exceeds resource limit");
  if (!fAttrs.reserve(fAttrs.size() + 1))
    return fail("Cannot allocate XML attribute list");
  char* copied_name = dupStr(name, name_len);
  char* copied_value = dupStr(value, value_len);
  if (!copied_name || !copied_value) {
    SrSVGFree(copied_name);
    SrSVGFree(copied_value);
    return fail("Cannot allocate XML attribute strings");
  }
  fAttrs.push_back(SrDOM::Attr{copied_name, copied_value});
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
        "Encountered unexpected closing tag while XML stack was empty.", "", 1);
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
                          "Encountered mismatched closing tag.", "", 1);
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
  if (fFailed)
    return true;
  if (fLevel >= kSVGMaxDepth || fNodeCount >= kSVGMaxNodes ||
      elemSize > kSVGMaxAttributeBytes || !reserveBytes(elemSize + 1))
    return fail("XML tree exceeds resource limit");
  ++fNodeCount;
  if (fLevel > 0 && fNeedToFlush) {
    if (this->flushAttributes()) {
      return true;
    }
  }
  if (fLevel == 0 && fRoot != nullptr) {
    SetMalformedXMLError(fError, elem, elemSize);
    SrSVGReportDiagnostic(fDiagnosticSink, SR_SVG_DIAGNOSTIC_XML_BUILD_FAILED,
                          "Encountered content after the root XML element.", "",
                          1);
    return true;
  }
  fNeedToFlush = true;
  fElemName = dupStr(elem, elemSize);
  if (!fElemName)
    return fail("Cannot allocate XML element name");
  fElemType = type;
  ++fLevel;
  return false;
}

}  // namespace parser
}  // namespace svg
}  // namespace serval
