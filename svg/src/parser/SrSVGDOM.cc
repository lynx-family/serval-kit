// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrSVGDOM.h"

#include <cstring>
#include <string>

#include "element/SrSVGCircle.h"
#include "element/SrSVGClipPath.h"
#include "element/SrSVGDefs.h"
#include "element/SrSVGEllipse.h"
#include "element/SrSVGFilter.h"
#include "element/SrSVGFilterPrimitives.h"
#include "element/SrSVGG.h"
#include "element/SrSVGImage.h"
#include "element/SrSVGLine.h"
#include "element/SrSVGLinearGradient.h"
#include "element/SrSVGMask.h"
#include "element/SrSVGPath.h"
#include "element/SrSVGPolyLine.h"
#include "element/SrSVGPolygon.h"
#include "element/SrSVGRadialGradient.h"
#include "element/SrSVGRect.h"
#include "element/SrSVGStop.h"
#include "element/SrSVGText.h"
#include "element/SrSVGUse.h"
#include "parser/SrDOM.h"
#include "utils/SrSVGLog.h"

namespace serval {
namespace svg {
namespace parser {

static bool gEnableDumpDom = false;

static void DumpDomTree(const SrDOM& dom, const SrDOM::Node* node, int depth) {
  if (!node) {
    return;
  }
  std::string indent(depth * 2, ' ');
  const char* name = dom.GetName(node);
  LOGI("%s[%d] Node: %s", indent.c_str(), depth, name);

  SrDOM::AttrIter attr_iter(node);
  const char* attr_name;
  const char* attr_value;
  while ((attr_name = attr_iter.Next(&attr_value))) {
    LOGI("%s  Attr: %s = %s", indent.c_str(), attr_name, attr_value);
  }

  for (auto* child = dom.GetFirstChild(node, nullptr); child;
       child = dom.GetNextSibling(child)) {
    DumpDomTree(dom, child, depth + 1);
  }
}

static bool IsNonRenderingTag(element::SrSVGTag tag) {
  switch (tag) {
    case element::SrSVGTag::kDefs:
    case element::SrSVGTag::kStop:
    case element::SrSVGTag::kLinearGradient:
    case element::SrSVGTag::kRadialGradient:
    case element::SrSVGTag::kClipPath:
    case element::SrSVGTag::kMask:
    case element::SrSVGTag::kFilter:
    case element::SrSVGTag::kFeBlend:
    case element::SrSVGTag::kFeColorMatrix:
    case element::SrSVGTag::kFeComposite:
    case element::SrSVGTag::kFeFlood:
    case element::SrSVGTag::kFeGaussianBlur:
    case element::SrSVGTag::kFeOffset:
      return true;
    default:
      return false;
  }
}

// Prevents non-rendering nodes (defs/mask/filter/gradient/fe*) from being
// added as renderable children of normal containers (g/svg). These nodes are
// still registered in IDMapper for IRI-based lookup. Without this filter,
// Container::OnRender() would cascade inherited attributes onto these nodes,
// polluting their state when later referenced via url(#id).
static bool ShouldAppendChild(const element::SrSVGNodeBase* parent,
                              const element::SrSVGNodeBase* child) {
  if (!parent || !child) {
    return false;
  }
  if (!IsNonRenderingTag(child->Tag())) {
    return true;
  }

  switch (parent->Tag()) {
    case element::SrSVGTag::kDefs:
    case element::SrSVGTag::kFilter:
    case element::SrSVGTag::kLinearGradient:
    case element::SrSVGTag::kRadialGradient:
      return true;
    default:
      return false;
  }
}

bool set_string_attribute(element::SrSVGNodeBase* node, const char* name,
                          const char* value) {
  return node->ParseAndSetAttribute(name, value);
}

void parse_node_attribute(const SrDOM& dom, const SrDOM::Node* xmlNode,
                          element::SrSVGNodeBase* svgNode,
                          element::IDMapper* id_mapper) {
  const char *name, *value;
  SrDOM::AttrIter attr_iter(xmlNode);
  while ((name = attr_iter.Next(&value))) {
    if (!std::strcmp(name, "id")) {
      std::string key{value};
      (*id_mapper)[key] = svgNode;
    }
    set_string_attribute(svgNode, name, value);
  }
}

void pre_parse_inherit_attribute(const element::SrSVGNode* parent_node,
                                 element::SrSVGNode* node) {
  node->inherit_fill_paint_ = parent_node->fill_;
  node->inherit_stroke_paint_ = parent_node->stroke_;
  node->inherit_clip_path_ = parent_node->clip_path_;
  node->inherit_mask_ = parent_node->mask_;
  node->inherit_opacity_ = parent_node->opacity_;
  node->inherit_fill_opacity_ = parent_node->fill_opacity_;
  node->inherit_stroke_opacity_ = parent_node->stroke_opacity_;
  node->inherit_stroke_width_ = parent_node->stroke_width_;
}

void pre_parse_inherit_color(const element::SrSVGNodeBase* parent_node,
                             element::SrSVGNodeBase* child_node) {
  if (parent_node->color_) {
    child_node->inherit_color_ = parent_node->color_;
  } else {
    child_node->inherit_color_ = parent_node->inherit_color_;
  }
}

element::SrSVGNodeBase* construct_svg_node(
    const SrDOM& dom, const element::SrSVGNodeBase* parentNode,
    const SrDOM::Node* curNode, element::IDMapper* id_mapper,
    std::list<element::SrSVGNodeBase*>& holder) {
  const char* el = dom.GetName(curNode);
  const auto type = dom.GetType(curNode);

  if (type == SrDOM::Type::kText_Type) {
    auto* text_el = element::SrSVGRawText::Make();
    text_el->SetText(el);
    holder.push_back(text_el);
    return text_el;
  }

  element::SrSVGNodeBase* node = nullptr;
  if (strcmp(el, "svg") == 0) {
    node = element::SrSVGSVG::Make();
  } else if (strcmp(el, "rect") == 0) {
    node = element::SrSVGRect::Make();
  } else if (strcmp(el, "circle") == 0) {
    node = element::SrSVGCircle::Make();
  } else if (strcmp(el, "line") == 0) {
    node = element::SrSVGLine::Make();
  } else if (strcmp(el, "polygon") == 0) {
    node = element::SrSVGPolygon::Make();
  } else if (strcmp(el, "polyline") == 0) {
    node = element::SrSVGPolyLine::Make();
  } else if (strcmp(el, "path") == 0) {
    node = element::SrSVGPath::Make();
  } else if (strcmp(el, "ellipse") == 0) {
    node = element::SrSVGEllipse::Make();
  } else if (strcmp(el, "defs") == 0) {
    node = element::SrSVGDefs::Make();
  } else if (strcmp(el, "stop") == 0) {
    node = element::SrSVGStop::Make();
  } else if (strcmp(el, "linearGradient") == 0) {
    node = element::SrSVGLinearGradient::Make();
  } else if (strcmp(el, "radialGradient") == 0) {
    node = element::SrSVGRadialGradient::Make();
  } else if (strcmp(el, "mask") == 0) {
    node = element::SrSVGMask::Make();
  } else if (strcmp(el, "use") == 0) {
    node = element::SrSVGUse::Make();
  } else if (strcmp(el, "image") == 0) {
    node = element::SrSVGImage::Make();
  } else if (strcmp(el, "clipPath") == 0) {
    node = element::SrSVGClipPath::Make();
  } else if (strcmp(el, "filter") == 0) {
    node = element::SrSVGFilter::Make();
  } else if (strcmp(el, "feGaussianBlur") == 0) {
    node = element::SrSVGFeGaussianBlur::Make();
  } else if (strcmp(el, "feOffset") == 0) {
    node = element::SrSVGFeOffset::Make();
  } else if (strcmp(el, "feColorMatrix") == 0) {
    node = element::SrSVGFeColorMatrix::Make();
  } else if (strcmp(el, "feComposite") == 0) {
    node = element::SrSVGFeComposite::Make();
  } else if (strcmp(el, "feBlend") == 0) {
    node = element::SrSVGFeBlend::Make();
  } else if (strcmp(el, "feFlood") == 0) {
    node = element::SrSVGFeFlood::Make();
  } else if (strcmp(el, "g") == 0) {
    node = element::SrSVGG::Make();
  } else if (strcmp(el, "text") == 0) {
    node = element::SrSVGText::Make();
  } else if (strcmp(el, "tspan") == 0) {
    node = element::SrSVGTextSpan::Make();
  }
  if (!node) {
    return nullptr;
  }
  holder.push_back(node);
  if (parentNode) {
    if (parentNode->IsSVGNode() && node->IsSVGNode()) {
      pre_parse_inherit_attribute(
          static_cast<const element::SrSVGNode*>(parentNode),
          static_cast<element::SrSVGNode*>(node));
    }
    pre_parse_inherit_color(parentNode, node);
  }
  parse_node_attribute(dom, curNode, node, id_mapper);
  for (auto* child = dom.GetFirstChild(curNode, nullptr); child;
       child = dom.GetNextSibling(child)) {
    element::SrSVGNodeBase* childNode =
        construct_svg_node(dom, node, child, id_mapper, holder);
    if (ShouldAppendChild(node, childNode)) {
      node->AppendChild(childNode);
    }
  }
  return node;
}

std::unique_ptr<SrSVGDOM> SrSVGDOM::make(const char* doc, size_t len) {
  auto xml_dom = std::make_shared<SrDOM>();
  if (!xml_dom->build(doc, len)) {
    return nullptr;
  }
  if (gEnableDumpDom) {
    DumpDomTree(*xml_dom, xml_dom->GetRootNode(), 0);
  }
  auto id_mapper = std::make_unique<element::IDMapper>();
  std::list<element::SrSVGNodeBase*> holder;
  auto* root_node = xml_dom->GetRootNode();
  if (!root_node) {
    return nullptr;
  }
  auto* root =
      construct_svg_node(*xml_dom, nullptr, root_node, id_mapper.get(), holder);
  if (!root) {
    for (auto* node : holder) {
      delete node;
    }
    return nullptr;
  }
  if (root->Tag() == element::SrSVGTag::kSvg) {
    return std::make_unique<SrSVGDOM>(static_cast<element::SrSVGSVG*>(root),
                                      id_mapper.release(), std::move(holder),
                                      xml_dom);
  }
  for (auto* node : holder) {
    delete node;
  }
  return nullptr;
}

void SrSVGDOM::Render(canvas::SrCanvas* canvas) const {
  if (root_) {
    SrSVGBox view_box = root_->viewBox();
    SrSVGRenderContext context = (SrSVGRenderContext){
        this->dpi_, view_box.width, view_box.height, 0, id_mapper_};
    root_->Render(canvas, context);
  }
}

void SrSVGDOM::Render(canvas::SrCanvas* canvas, SrSVGBox view_port) const {
  if (root_) {
    SrSVGBox view_box = root_->viewBox();
    SrSVGRenderContext context{
        .dpi = dpi_,
        .id_mapper = id_mapper_,
        .view_port = view_port,
        .view_box = view_box,
    };
    root_->Render(canvas, context);
  }
}

// Id mapper should only ref to an svg node, but should never copy or delete
// them. They will be released within the svg dom deconstruction process while
// svg node delete their children iteratively
SrSVGDOM::~SrSVGDOM() {
  if (id_mapper_) {
    delete id_mapper_;
    id_mapper_ = nullptr;
  }

  for (auto* node : nodes_) {
    delete node;
  }
}

}  // namespace parser
}  // namespace svg
}  // namespace serval
