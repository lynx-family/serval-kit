// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrDOM.h"
#include "parser/SrDOMParser.h"

#include <cstdlib>

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
  const Attr* stop = attr + node->fAttrCount;

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
  fStop = fAttr + node->fAttrCount;
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
  SrDOMParser parser(diagnostic_sink);
  if (error) {
    *error = parser.fParserError;
  }
  if (!parser.parse(data, len)) {
    if (error) {
      *error = parser.fParserError;
    }
    return nullptr;
  }
  if (!parser.Finish()) {
    if (error) {
      *error = parser.fParserError;
    }
    return nullptr;
  }
  if (error) {
    *error = parser.fParserError;
  }
  fRoot = parser.releaseRoot();
  return fRoot;
}

static void Walk_dom(const SrDOM& dom, const SrDOM::Node* node,
                     SrXMLParser* parser) {
  const char* elem = dom.GetName(node);
  if (dom.GetType(node) == SrDOM::kText_Type) {
    parser->Text(elem, static_cast<int>(strlen(elem)));
    return;
  }

  parser->StartElement(elem);

  SrDOM::AttrIter iter(node);
  const char* name;
  const char* value;
  while ((name = iter.Next(&value)) != nullptr)
    parser->AddAttribute(name, value);

  node = dom.GetFirstChild(node, nullptr);
  while (node) {
    Walk_dom(dom, node, parser);
    node = dom.GetNextSibling(node, nullptr);
  }

  parser->EndElement(elem);
}

const SrDOM::Node* SrDOM::Copy(const SrDOM& dom, const SrDOM::Node* node) {
  SrDOMParser parser{};

  Walk_dom(dom, node, &parser);

  fRoot = parser.releaseRoot();
  return fRoot;
}

SrXMLParser* SrDOM::BeginParsing() {
  fParser = std::make_unique<SrDOMParser>();

  return fParser.get();
}

const SrDOM::Node* SrDOM::FinishParsing() {
  if (!fParser->Finish()) {
    fRoot = nullptr;
    fParser.reset();
    return nullptr;
  }
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

}  // namespace parser
}  // namespace svg
}  // namespace serval
