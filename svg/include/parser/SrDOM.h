// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRDOM_H_
#define SVG_INCLUDE_PARSER_SRDOM_H_

#include <memory>
#include <vector>

namespace serval {
namespace svg {
namespace parser {

struct SrDOMAttr {
  const char* fName;
  const char* fValue;
};

struct SrDOMNode {
  const char* fName;
  SrDOMNode* fFirstChild;
  SrDOMNode* fNextSibling;
  SrDOMAttr* fAttrs;
  uint16_t fAttrCount;
  uint8_t fType;
  uint8_t fPad;
  const SrDOMAttr* attrs() const { return fAttrs; }
  SrDOMAttr* attrs() { return fAttrs; }
};

class SrDOMParser;
class SrXMLParser;

class SrDOM {
 public:
  SrDOM();
  ~SrDOM();

  typedef SrDOMNode Node;
  typedef SrDOMAttr Attr;

  /** Returns null on failure
   */
  const SrDOM::Node* build(const char* data, size_t len);
  const Node* Copy(const SrDOM& dom, const Node* node);

  [[nodiscard]] const Node* GetRootNode() const;

  SrXMLParser* BeginParsing();
  const Node* FinishParsing();

  enum Type { kElement_Type, kText_Type };
  Type GetType(const Node*) const;

  const char* GetName(const Node*) const;
  const Node* GetFirstChild(const Node*, const char elem[] = nullptr) const;
  const Node* GetNextSibling(const Node*, const char elem[] = nullptr) const;

  const char* FindAttr(const Node*, const char attrName[]) const;

  // helpers for walking children
  int FindList(const Node*, const char name[], const char list[]) const;

  class AttrIter {
   public:
    AttrIter(const Node*);
    const char* Next(const char** value);

   private:
    const Attr* fAttr;
    const Attr* fStop;
  };

 private:
  Node* fRoot;
  std::unique_ptr<SrDOMParser> fParser;
};

}  // namespace parser
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PARSER_SRDOM_H_
