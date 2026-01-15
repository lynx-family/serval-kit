// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRSVGDOM_H_
#define SVG_INCLUDE_PARSER_SRSVGDOM_H_

#include <list>
#include <memory>
#include <unordered_map>
#include <utility>

#include "canvas/SrCanvas.h"
#include "element/SrSVGSVG.h"
#include "parser/SrDOM.h"

namespace serval {
namespace svg {
namespace parser {

class SrSVGDOM {
 public:
  static std::unique_ptr<SrSVGDOM> make(const char*, size_t);
  ~SrSVGDOM();
  explicit SrSVGDOM(element::SrSVGSVG* root, element::IDMapper* id_mapper,
                    std::list<element::SrSVGNodeBase*>&& holder,
                    std::shared_ptr<SrDOM> xml_dom)
      : root_(root),
        id_mapper_(id_mapper),
        nodes_(std::move(holder)),
        xml_dom_(xml_dom) {}

  float dpi_{0.f};
  void Render(canvas::SrCanvas* canvas) const;
  void Render(canvas::SrCanvas* canvas, SrSVGBox view_port) const;

 private:
  element::SrSVGSVG* root_;
  element::IDMapper* id_mapper_;
  std::list<element::SrSVGNodeBase*> nodes_;
  //release SrDOM after rendering is complete
  std::shared_ptr<SrDOM> xml_dom_;
};

}  // namespace parser
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PARSER_SRSVGDOM_H_
