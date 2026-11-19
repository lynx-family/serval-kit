// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrDOM.h"
#include "parser/SrDOMParser.h"

#include <cstdlib>
#include <new>
#include "parser/SrSVGMemory.h"

namespace serval {
namespace svg {
namespace parser {

SrDOM::SrDOM() : fRoot(nullptr) {}

static void Destroy_node(SrDOMNode* node);

SrDOM::~SrDOM() {
  Destroy_node(fRoot);
}

const SrDOM::Node* SrDOM::GetRootNode() const {
  return fRoot;
}

const SrDOM::Node* SrDOM::GetFirstChild(const Node* node,
                                        const char name[]) const {
  const Node* child = node->fFirstChild;

  if (name) {
    for (; child != nullptr; child = child->fNextSibling) {
      if (!strcmp(name, child->fName)) {
        break;
      }
    }
  }
  return child;
}

const SrDOM::Node* SrDOM::GetNextSibling(const Node* node,
                                         const char name[]) const {
  const Node* sibling = node->fNextSibling;
  if (name) {
    for (; sibling != nullptr; sibling = sibling->fNextSibling) {
      if (!strcmp(name, sibling->fName)) {
        break;
      }
    }
  }
  return sibling;
}

SrDOM::Type SrDOM::GetType(const Node* node) const {
  return (Type)node->fType;
}

const char* SrDOM::GetName(const Node* node) const {
  return node->fName;
}

const char* SrDOM::FindAttr(const Node* node, const char name[]) const {
  const Attr* attr = node->attrs();
  const Attr* stop = attr ? attr + node->fAttrCount : attr;

  while (attr < stop) {
    if (!strcmp(attr->fName, name)) {
      return attr->fValue;
    }
    attr += 1;
  }
  return nullptr;
}

SrDOM::AttrIter::AttrIter(const SrDOM::Node* node) {
  fAttr = node->attrs();
  fStop = fAttr ? fAttr + node->fAttrCount : fAttr;
}

const char* SrDOM::AttrIter::Next(const char** value) {
  const char* name = nullptr;

  if (fAttr < fStop) {
    name = fAttr->fName;
    if (value) {
      *value = fAttr->fValue;
    }
    fAttr += 1;
  }
  return name;
}

const SrDOM::Node* SrDOM::build(const char* data, size_t len,
                                SrXMLParserError* error,
                                const SrSVGDiagnosticSink* diagnostic_sink) {
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS)
  try {
#endif
    SrDOMParser parser(diagnostic_sink);
    const bool success = parser.parse(data, len) && parser.Finish();
    if (error)
      *error = parser.fParserError;
    if (!success)
      return nullptr;
    auto* root = parser.releaseRoot();
    Destroy_node(fRoot);
    fRoot = root;
    return fRoot;
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS)
  } catch (const std::bad_alloc&) {
    // Reporting OOM must not allocate another error string.
    if (error)
      error->SetCode(SrXMLParserError::kUnknownError);
    return nullptr;
  }
#endif
}

static bool Walk_dom(const SrDOM& dom, const SrDOM::Node* node,
                     SrXMLParser* parser) {
  const char* elem = dom.GetName(node);
  if (dom.GetType(node) == SrDOM::kText_Type)
    return !parser->Text(elem, strlen(elem));
  if (parser->StartElement(elem))
    return false;
  SrDOM::AttrIter iter(node);
  const char* name;
  const char* value;
  while ((name = iter.Next(&value)) != nullptr)
    if (parser->AddAttribute(name, value))
      return false;
  for (auto* child = dom.GetFirstChild(node, nullptr); child;
       child = dom.GetNextSibling(child, nullptr))
    if (!Walk_dom(dom, child, parser))
      return false;
  return !parser->EndElement(elem);
}

const SrDOM::Node* SrDOM::Copy(const SrDOM& dom, const SrDOM::Node* node) {
  SrDOMParser parser{};
  if (!node || !Walk_dom(dom, node, &parser) || !parser.Finish())
    return nullptr;
  auto* root = parser.releaseRoot();
  Destroy_node(fRoot);
  fRoot = root;
  return fRoot;
}

void SrDOM::ParserDeleter::operator()(SrDOMParser* parser) const {
  if (!parser)
    return;
  parser->~SrDOMParser();
  SrSVGFree(parser);
}

SrXMLParser* SrDOM::BeginParsing() {
  fParser.reset();
  void* storage = SrSVGAlloc(sizeof(SrDOMParser));
  if (!storage)
    return nullptr;
  fParser.reset(new (storage) SrDOMParser());
  return fParser.get();
}

const SrDOM::Node* SrDOM::FinishParsing() {
  if (!fParser || !fParser->Finish()) {
    fParser.reset();
    return nullptr;
  }
  Destroy_node(fRoot);
  fRoot = fParser->releaseRoot();
  fParser.reset();

  return fRoot;
}

static int findList(const char target[], const char list[]) {
  size_t len = strlen(target);
  int index = 0;

  for (;;) {
    const char* end = strchr(list, ',');
    size_t entryLen;

    if (end == nullptr)  // last entry
      entryLen = strlen(list);
    else
      entryLen = end - list;

    if (entryLen == len && memcmp(target, list, len) == 0)
      return index;
    if (end == nullptr)
      break;

    list = end + 1;  // skip the ','
    index += 1;
  }
  return -1;
}

int SrDOM::FindList(const Node* node, const char name[],
                    const char list[]) const {
  const char* vstr = this->FindAttr(node, name);

  return vstr ? findList(vstr, list) : -1;
}

static void Destroy_node(SrDOMNode* node) {
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

}  // namespace parser
}  // namespace svg
}  // namespace serval
