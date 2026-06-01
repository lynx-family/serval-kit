// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrSVGDOM.h"

#include <cstring>
#include <iterator>
#include <string>
#include <vector>

#include "element/SrSVGAnimation.h"
#include "element/SrSVGCircle.h"
#include "element/SrSVGClipPath.h"
#include "element/SrSVGContainer.h"
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
#include "element/SrSVGPattern.h"
#include "element/SrSVGPolyLine.h"
#include "element/SrSVGPolygon.h"
#include "element/SrSVGRadialGradient.h"
#include "element/SrSVGRect.h"
#include "element/SrSVGStop.h"
#include "element/SrSVGText.h"
#include "element/SrSVGUse.h"
#include "parser/SrDOM.h"
#include "parser/SrSVGTraversalState.h"
#include "parser/SrXMLParser.h"
#include "parser/SrXMLParserError.h"
#include "parser/SrXMLStreamParser.h"
#include "utils/SrFloatComparison.h"
#include "utils/SrSVGLog.h"

namespace serval {
namespace svg {
namespace parser {

namespace {

SrSVGDiagnostic MakeParserDiagnostic(const SrXMLParserError& error) {
  std::string message;
  error.GetErrorString(message);
  if (message.empty()) {
    message = "Failed to build SVG DOM.";
  }

  SrSVGDiagnostic diagnostic;
  diagnostic.code = error.HasError() && error.GetErrorCode() ==
                                            SrXMLParserError::kUnknownError
                        ? SR_SVG_DIAGNOSTIC_XML_UNEXPECTED_CLOSE_TAG
                        : SR_SVG_DIAGNOSTIC_XML_BUILD_FAILED;
  diagnostic.message = message;
  diagnostic.subject = error.HasNoun() ? error.GetNoun() : "";
  diagnostic.fatal = true;
  return diagnostic;
}

void ReportDiagnosticToTraversalState(void* ctx, ::SrSVGDiagnosticCode code,
                                      const char* message, const char* subject,
                                      uint8_t fatal) {
  if (!ctx || code == SR_SVG_DIAGNOSTIC_NONE) {
    return;
  }
  static_cast<SrSVGTraversalState*>(ctx)->Report(code, message, subject,
                                                 fatal != 0);
}

SrSVGDiagnosticSink MakeDiagnosticSink(SrSVGTraversalState* state) {
  SrSVGDiagnosticSink sink{};
  sink.ctx = state;
  sink.report = ReportDiagnosticToTraversalState;
  return sink;
}

bool IsContainerTag(element::SrSVGTag tag) {
  switch (tag) {
    case element::SrSVGTag::kSvg:
    case element::SrSVGTag::kG:
    case element::SrSVGTag::kDefs:
    case element::SrSVGTag::kClipPath:
    case element::SrSVGTag::kMask:
    case element::SrSVGTag::kFilter:
    case element::SrSVGTag::kPattern:
      return true;
    default:
      return false;
  }
}

bool TagNamesMatch(const char* lhs, const char* rhs, size_t rhs_len) {
  return lhs && rhs && std::strlen(lhs) == rhs_len &&
         std::memcmp(lhs, rhs, rhs_len) == 0;
}

std::string ToString(const char* data, size_t len) {
  return data ? std::string(data, len) : std::string();
}

bool BoundsContains(const SrSVGBox& bounds, float x, float y) {
  return x >= bounds.left && x <= bounds.left + bounds.width &&
         y >= bounds.top && y <= bounds.top + bounds.height;
}

void CopyTransform(const float (&src)[6], float (&dst)[6]) {
  std::memcpy(dst, src, sizeof(float) * 6);
}

void SetHitResult(const element::SrSVGNodeBase* node,
                  SrSVGHitTestResult* result) {
  result->hit = true;
  result->id = node->Id();
  result->action = node->ClickEvent();
}

bool HitTestNode(const element::SrSVGNodeBase* node,
                 canvas::PathFactory* path_factory,
                 SrSVGRenderContext* context,
                 const float (&ancestor_transform)[6], float x, float y,
                 SrSVGHitTestResult* result) {
  if (!node || !path_factory || !context) {
    return false;
  }

  bool geometry_hit = false;

  if (node->IsSVGNode() && IsContainerTag(node->Tag())) {
    const auto* container = static_cast<const element::SrSVGContainer*>(node);
    const auto* svg_node = static_cast<const element::SrSVGNode*>(node);
    float child_transform[6];
    CopyTransform(ancestor_transform, child_transform);
    xform_multiply(child_transform, svg_node->transform_);

    const auto& children = container->children();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
      const bool child_hit =
          HitTestNode(*it, path_factory, context, child_transform, x, y, result);
      if (result->hit) {
        return true;
      }
      geometry_hit = geometry_hit || child_hit;
    }

    if (geometry_hit && node->HasClickEvent()) {
      SetHitResult(node, result);
      return true;
    }
    return geometry_hit;
  }

  auto path = node->AsPath(path_factory, context);
  if (path) {
    path->Transform(ancestor_transform);
    geometry_hit = BoundsContains(path->GetBounds(), x, y);
    if (geometry_hit && node->HasClickEvent()) {
      SetHitResult(node, result);
      return true;
    }
  }
  return geometry_hit;
}

void ApplyAnimations(const std::list<element::SrSVGNodeBase*>& nodes,
                     double seconds) {
  for (auto* node : nodes) {
    if (node && node->IsSVGNode()) {
      static_cast<element::SrSVGNode*>(node)->ApplyAnimations(seconds);
    }
  }
}

void RestoreAnimations(const std::list<element::SrSVGNodeBase*>& nodes) {
  for (auto* node : nodes) {
    if (node && node->IsSVGNode()) {
      static_cast<element::SrSVGNode*>(node)->RestoreAnimatedAttributes();
    }
  }
}

bool HasAnimationsRecursive(const element::SrSVGNodeBase* node) {
  if (!node || !node->IsSVGNode()) {
    return false;
  }
  const auto* svg_node = static_cast<const element::SrSVGNode*>(node);
  if (svg_node->HasAnimations()) {
    return true;
  }
  if (IsContainerTag(node->Tag())) {
    const auto* container = static_cast<const element::SrSVGContainer*>(node);
    for (auto* child : container->children()) {
      if (HasAnimationsRecursive(child)) {
        return true;
      }
    }
  }
  return false;
}

element::SrSVGNodeBase* make_svg_node(const char* el) {
  element::SrSVGNodeBase* node = nullptr;
  if (strcmp(el, "animate") == 0) {
    node = element::SrSVGAnimation::MakeAnimate();
  } else if (strcmp(el, "animateTransform") == 0) {
    node = element::SrSVGAnimation::MakeAnimateTransform();
  } else if (strcmp(el, "svg") == 0) {
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
  } else if (strcmp(el, "pattern") == 0) {
    node = element::SrSVGPattern::Make();
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
  return node;
}

}  // namespace

struct SrSVGDOMStreamBuilder::Impl {
  Impl()
      : xml_dom(std::make_shared<SrDOM>()),
        sink(MakeDiagnosticSink(&build_state)),
        xml_parser(xml_dom->BeginParsing(&sink)),
        stream_parser(xml_parser) {}

  std::shared_ptr<SrDOM> xml_dom;
  SrSVGTraversalState build_state;
  SrSVGDiagnosticSink sink;
  SrXMLParser* xml_parser{nullptr};
  SrXMLStreamParser stream_parser;
  SrXMLParserError parser_error;
  bool append_failed{false};
  bool finished{false};
  std::vector<SrSVGDiagnostic> diagnostics;
};

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
    case element::SrSVGTag::kAnimate:
    case element::SrSVGTag::kAnimateTransform:
    case element::SrSVGTag::kStop:
    case element::SrSVGTag::kLinearGradient:
    case element::SrSVGTag::kRadialGradient:
    case element::SrSVGTag::kClipPath:
    case element::SrSVGTag::kMask:
    case element::SrSVGTag::kFilter:
    case element::SrSVGTag::kPattern:
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
  if (child->Tag() == element::SrSVGTag::kAnimate ||
      child->Tag() == element::SrSVGTag::kAnimateTransform) {
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
    case element::SrSVGTag::kPattern:
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
                          element::IDMapper* id_mapper,
                          const SrSVGDiagnosticSink* diagnostic_sink) {
  svgNode->SetDiagnosticSink(diagnostic_sink);
  const char *name, *value;
  SrDOM::AttrIter attr_iter(xmlNode);
  while ((name = attr_iter.Next(&value))) {
    svgNode->StoreAttribute(name, value);
    if (!std::strcmp(name, "id")) {
      std::string key{value};
      (*id_mapper)[key] = svgNode;
    }
    set_string_attribute(svgNode, name, value);
  }
}

void pre_parse_inherit_attribute(const element::SrSVGNode* parent_node,
                                 element::SrSVGNode* node) {
  node->inherit_fill_paint_ = parent_node->fill_
                                  ? parent_node->fill_
                                  : parent_node->inherit_fill_paint_;
  node->inherit_stroke_paint_ = parent_node->stroke_
                                    ? parent_node->stroke_
                                    : parent_node->inherit_stroke_paint_;
  node->inherit_clip_path_ = parent_node->clip_path_
                                 ? parent_node->clip_path_
                                 : parent_node->inherit_clip_path_;
  node->inherit_mask_ =
      parent_node->mask_ ? parent_node->mask_ : parent_node->inherit_mask_;
  node->inherit_opacity_ = parent_node->opacity_
                               ? parent_node->opacity_
                               : parent_node->inherit_opacity_;
  node->inherit_fill_opacity_ = parent_node->fill_opacity_
                                    ? parent_node->fill_opacity_
                                    : parent_node->inherit_fill_opacity_;
  node->inherit_stroke_opacity_ = parent_node->stroke_opacity_
                                      ? parent_node->stroke_opacity_
                                      : parent_node->inherit_stroke_opacity_;
  node->inherit_stroke_width_ = parent_node->stroke_width_
                                    ? parent_node->stroke_width_
                                    : parent_node->inherit_stroke_width_;
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
    std::list<element::SrSVGNodeBase*>& holder,
    const SrSVGDiagnosticSink* diagnostic_sink) {
  const char* el = dom.GetName(curNode);
  const auto type = dom.GetType(curNode);

  if (type == SrDOM::Type::kText_Type) {
    auto* text_el = element::SrSVGRawText::Make();
    text_el->SetText(el);
    holder.push_back(text_el);
    return text_el;
  }

  element::SrSVGNodeBase* node = nullptr;
  if (strcmp(el, "animate") == 0) {
    node = element::SrSVGAnimation::MakeAnimate();
  } else if (strcmp(el, "animateTransform") == 0) {
    node = element::SrSVGAnimation::MakeAnimateTransform();
  } else if (strcmp(el, "svg") == 0) {
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
  } else if (strcmp(el, "pattern") == 0) {
    node = element::SrSVGPattern::Make();
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
  parse_node_attribute(dom, curNode, node, id_mapper, diagnostic_sink);
  for (auto* child = dom.GetFirstChild(curNode, nullptr); child;
       child = dom.GetNextSibling(child)) {
    element::SrSVGNodeBase* childNode = construct_svg_node(
        dom, node, child, id_mapper, holder, diagnostic_sink);
    if (childNode &&
        (childNode->Tag() == element::SrSVGTag::kAnimate ||
         childNode->Tag() == element::SrSVGTag::kAnimateTransform) &&
        node->IsSVGNode()) {
      static_cast<element::SrSVGNode*>(node)->AddAnimation(
          static_cast<element::SrSVGAnimation*>(childNode));
    }
    if (ShouldAppendChild(node, childNode)) {
      node->AppendChild(childNode);
    }
  }
  return node;
}

std::unique_ptr<SrSVGDOM> SrSVGDOM::make(
    const char* doc, size_t len, std::vector<SrSVGDiagnostic>* diagnostics) {
  SrSVGTraversalState build_state;
  SrSVGDiagnosticSink build_sink = MakeDiagnosticSink(&build_state);
  auto xml_dom = std::make_shared<SrDOM>();
  SrXMLParserError parser_error;
  if (!xml_dom->build(doc, len, &parser_error, &build_sink)) {
    if (diagnostics && parser_error.HasError()) {
      diagnostics->push_back(MakeParserDiagnostic(parser_error));
    }
    return nullptr;
  }
  if (parser_error.HasError()) {
    build_state.diagnostics.push_back(MakeParserDiagnostic(parser_error));
  }
  if (gEnableDumpDom) {
    DumpDomTree(*xml_dom, xml_dom->GetRootNode(), 0);
  }
  auto id_mapper = std::make_unique<element::IDMapper>();
  std::list<element::SrSVGNodeBase*> holder;
  auto* root_node = xml_dom->GetRootNode();
  if (!root_node) {
    if (diagnostics && !build_state.diagnostics.empty()) {
      *diagnostics = build_state.diagnostics;
    }
    return nullptr;
  }
  auto* root = construct_svg_node(*xml_dom, nullptr, root_node, id_mapper.get(),
                                  holder, &build_sink);
  if (!root) {
    for (auto* node : holder) {
      delete node;
    }
    if (diagnostics && !build_state.diagnostics.empty()) {
      *diagnostics = build_state.diagnostics;
    }
    return nullptr;
  }
  if (root->Tag() == element::SrSVGTag::kSvg) {
    auto svg_dom = std::make_unique<SrSVGDOM>(
        static_cast<element::SrSVGSVG*>(root), id_mapper.release(),
        std::move(holder), xml_dom);
    svg_dom->SetBuildDiagnostics(std::move(build_state.diagnostics));
    if (diagnostics) {
      *diagnostics = svg_dom->diagnostics();
    }
    return svg_dom;
  }
  for (auto* node : holder) {
    delete node;
  }
  if (diagnostics && !build_state.diagnostics.empty()) {
    *diagnostics = build_state.diagnostics;
  }
  return nullptr;
}

SrSVGDOMStreamBuilder::SrSVGDOMStreamBuilder()
    : impl_(std::make_unique<Impl>()) {}

SrSVGDOMStreamBuilder::~SrSVGDOMStreamBuilder() = default;

bool SrSVGDOMStreamBuilder::Append(const char* data, size_t len) {
  if (!impl_ || impl_->finished || impl_->append_failed) {
    return false;
  }
  if (!impl_->stream_parser.Append(data, len)) {
    impl_->append_failed = true;
    return false;
  }
  return true;
}

std::unique_ptr<SrSVGDOM> SrSVGDOMStreamBuilder::Finish() {
  if (!impl_ || impl_->finished) {
    return nullptr;
  }
  impl_->finished = true;

  const bool stream_ok =
      !impl_->append_failed && impl_->stream_parser.Finish();
  const bool dom_ok = impl_->xml_dom->FinishParsing(&impl_->parser_error);
  if (!stream_ok || !dom_ok) {
    if (impl_->parser_error.HasError()) {
      impl_->diagnostics.push_back(MakeParserDiagnostic(impl_->parser_error));
    }
    return nullptr;
  }

  if (impl_->parser_error.HasError()) {
    impl_->build_state.diagnostics.push_back(
        MakeParserDiagnostic(impl_->parser_error));
  }
  if (gEnableDumpDom) {
    DumpDomTree(*impl_->xml_dom, impl_->xml_dom->GetRootNode(), 0);
  }

  auto id_mapper = std::make_unique<element::IDMapper>();
  std::list<element::SrSVGNodeBase*> holder;
  auto* root_node = impl_->xml_dom->GetRootNode();
  if (!root_node) {
    impl_->diagnostics = impl_->build_state.diagnostics;
    return nullptr;
  }
  auto* root =
      construct_svg_node(*impl_->xml_dom, nullptr, root_node, id_mapper.get(),
                         holder, &impl_->sink);
  if (!root) {
    for (auto* node : holder) {
      delete node;
    }
    impl_->diagnostics = impl_->build_state.diagnostics;
    return nullptr;
  }
  if (root->Tag() == element::SrSVGTag::kSvg) {
    auto svg_dom = std::make_unique<SrSVGDOM>(
        static_cast<element::SrSVGSVG*>(root), id_mapper.release(),
        std::move(holder), impl_->xml_dom);
    svg_dom->SetBuildDiagnostics(std::move(impl_->build_state.diagnostics));
    impl_->diagnostics = svg_dom->diagnostics();
    return svg_dom;
  }
  for (auto* node : holder) {
    delete node;
  }
  impl_->diagnostics = impl_->build_state.diagnostics;
  return nullptr;
}

const std::vector<SrSVGDiagnostic>& SrSVGDOMStreamBuilder::diagnostics()
    const {
  return impl_->diagnostics;
}

class IncrementalSVGParser : public SrXMLParser {
 public:
  explicit IncrementalSVGParser(SrSVGTraversalState* build_state)
      : SrXMLParser(&parser_error_),
        build_state_(build_state),
        sink_(MakeDiagnosticSink(build_state)) {}

  bool Finish() {
    if (!frames_.empty()) {
      if (!parser_error_.HasError()) {
        parser_error_.SetCode(SrXMLParserError::kUnknownError);
        parser_error_.SetNoun(frames_.back().name.c_str());
      }
      SrSVGReportDiagnostic(&sink_, SR_SVG_DIAGNOSTIC_XML_BUILD_FAILED,
                            "Encountered unterminated XML element at end of "
                            "stream.",
                            frames_.back().name.c_str(), 1);
      return false;
    }
    if (!dom_) {
      if (!parser_error_.HasError()) {
        parser_error_.SetCode(SrXMLParserError::kEmptyFile);
        parser_error_.SetNoun("");
      }
      return false;
    }
    finished_ = true;
    SyncBuildDiagnostics();
    return true;
  }

  SrSVGDOM* Preview() {
    SyncBuildDiagnostics();
    return dom_.get();
  }

  SrSVGDOM* Final() {
    return finished_ ? dom_.get() : nullptr;
  }

  SrXMLParserError& parser_error() { return parser_error_; }

 protected:
  bool OnStartElement(const char elem[], size_t len) override {
    if (root_closed_) {
      SetError(elem, len);
      SrSVGReportDiagnostic(&sink_, SR_SVG_DIAGNOSTIC_XML_BUILD_FAILED,
                            "Encountered content after the root XML element.",
                            ToString(elem, len).c_str(), 1);
      return true;
    }

    element::SrSVGNodeBase* parent =
        frames_.empty() ? nullptr : frames_.back().node;
    element::SrSVGNodeBase* node = nullptr;
    if (frames_.empty() || parent != nullptr) {
      const std::string name = ToString(elem, len);
      node = make_svg_node(name.c_str());
      if (node && parent) {
        if (parent->IsSVGNode() && node->IsSVGNode()) {
          pre_parse_inherit_attribute(
              static_cast<const element::SrSVGNode*>(parent),
              static_cast<element::SrSVGNode*>(node));
        }
        pre_parse_inherit_color(parent, node);
      }
      if (node) {
        node->SetDiagnosticSink(&sink_);
      }
    }

    if (frames_.empty() && node) {
      if (node->Tag() != element::SrSVGTag::kSvg) {
        delete node;
        SetError(elem, len);
        return true;
      }
      auto id_mapper = std::make_unique<element::IDMapper>();
      id_mapper_ = id_mapper.get();
      std::list<element::SrSVGNodeBase*> holder;
      holder.push_back(node);
      dom_ = std::make_unique<SrSVGDOM>(
          static_cast<element::SrSVGSVG*>(node), id_mapper.release(),
          std::move(holder), std::make_shared<SrDOM>());
    } else if (node && dom_) {
      dom_->AdoptNode(node);
    }

    bool appended_to_parent = false;
    if (node && parent && ShouldAppendChild(parent, node)) {
      parent->AppendChild(node);
      appended_to_parent = true;
    }

    frames_.push_back(Frame{ToString(elem, len), node, appended_to_parent});
    return false;
  }

  bool OnAddAttribute(const char name[], size_t name_len, const char value[],
                      size_t value_len) override {
    if (frames_.empty() || !frames_.back().node) {
      return false;
    }
    auto* node = frames_.back().node;
    const std::string attr_name = ToString(name, name_len);
    const std::string attr_value = ToString(value, value_len);
    node->StoreAttribute(attr_name.c_str(), attr_value.c_str());
    if (attr_name == "id" && id_mapper_) {
      (*id_mapper_)[attr_value] = node;
    }
    node->ParseAndSetAttribute(attr_name.c_str(), attr_value.c_str());
    return false;
  }

  bool OnEndElement(const char elem[], size_t len) override {
    if (frames_.empty()) {
      SetError(elem, len);
      SrSVGReportDiagnostic(
          &sink_, SR_SVG_DIAGNOSTIC_XML_UNEXPECTED_CLOSE_TAG,
          "Encountered unexpected closing tag while XML stack was empty.",
          ToString(elem, len).c_str(), 1);
      return true;
    }
    if (!TagNamesMatch(frames_.back().name.c_str(), elem, len)) {
      SetError(elem, len);
      SrSVGReportDiagnostic(&sink_,
                            SR_SVG_DIAGNOSTIC_XML_UNEXPECTED_CLOSE_TAG,
                            "Encountered mismatched closing tag.",
                            ToString(elem, len).c_str(), 1);
      return true;
    }

    Frame frame = frames_.back();
    frames_.pop_back();
    element::SrSVGNodeBase* parent =
        frames_.empty() ? nullptr : frames_.back().node;
    if (frame.node && parent) {
      if ((frame.node->Tag() == element::SrSVGTag::kAnimate ||
           frame.node->Tag() == element::SrSVGTag::kAnimateTransform) &&
          parent->IsSVGNode()) {
        static_cast<element::SrSVGNode*>(parent)->AddAnimation(
            static_cast<element::SrSVGAnimation*>(frame.node));
      }
      if (!frame.appended_to_parent && ShouldAppendChild(parent, frame.node)) {
        parent->AppendChild(frame.node);
      }
    }
    if (!parent && frame.node && frame.node->Tag() == element::SrSVGTag::kSvg) {
      root_closed_ = true;
    }
    SyncBuildDiagnostics();
    return false;
  }

  bool OnText(const char text[], size_t len) override {
    if (frames_.empty() || !frames_.back().node || len == 0 || !dom_) {
      return false;
    }
    auto* text_el = element::SrSVGRawText::Make();
    text_el->SetText(ToString(text, len).c_str());
    text_el->SetDiagnosticSink(&sink_);
    dom_->AdoptNode(text_el);
    if (ShouldAppendChild(frames_.back().node, text_el)) {
      frames_.back().node->AppendChild(text_el);
    }
    return false;
  }

 private:
  struct Frame {
    std::string name;
    element::SrSVGNodeBase* node{nullptr};
    bool appended_to_parent{false};
  };

  void SetError(const char* noun, size_t noun_len) {
    if (!parser_error_.HasError()) {
      parser_error_.SetCode(SrXMLParserError::kUnknownError);
      parser_error_.SetNoun(noun, noun_len);
    }
  }

  void SyncBuildDiagnostics() {
    if (dom_) {
      dom_->SetBuildDiagnostics(build_state_->diagnostics);
    }
  }

  SrXMLParserError parser_error_;
  SrSVGTraversalState* build_state_;
  SrSVGDiagnosticSink sink_;
  std::unique_ptr<SrSVGDOM> dom_;
  element::IDMapper* id_mapper_{nullptr};
  std::vector<Frame> frames_;
  bool root_closed_{false};
  bool finished_{false};
};

struct SrSVGDOMIncrementalBuilder::Impl {
  Impl() : parser(&build_state), stream_parser(&parser) {}

  SrSVGTraversalState build_state;
  IncrementalSVGParser parser;
  SrXMLStreamParser stream_parser;
  bool append_failed{false};
  bool finished{false};
  std::vector<SrSVGDiagnostic> diagnostics;

  void RefreshDiagnostics() {
    diagnostics = build_state.diagnostics;
    if (parser.parser_error().HasError()) {
      diagnostics.push_back(MakeParserDiagnostic(parser.parser_error()));
    }
  }
};

SrSVGDOMIncrementalBuilder::SrSVGDOMIncrementalBuilder()
    : impl_(std::make_unique<Impl>()) {}

SrSVGDOMIncrementalBuilder::~SrSVGDOMIncrementalBuilder() = default;

bool SrSVGDOMIncrementalBuilder::Append(const char* data, size_t len) {
  if (!impl_ || impl_->finished || impl_->append_failed) {
    return false;
  }
  if (!impl_->stream_parser.Append(data, len)) {
    impl_->append_failed = true;
    impl_->RefreshDiagnostics();
    return false;
  }
  impl_->RefreshDiagnostics();
  return true;
}

bool SrSVGDOMIncrementalBuilder::Finish() {
  if (!impl_ || impl_->finished) {
    return false;
  }
  impl_->finished = true;
  const bool ok =
      !impl_->append_failed && impl_->stream_parser.Finish() &&
      impl_->parser.Finish();
  impl_->RefreshDiagnostics();
  return ok;
}

SrSVGDOM* SrSVGDOMIncrementalBuilder::Preview() {
  return impl_ ? impl_->parser.Preview() : nullptr;
}

SrSVGDOM* SrSVGDOMIncrementalBuilder::Final() {
  return impl_ ? impl_->parser.Final() : nullptr;
}

const std::vector<SrSVGDiagnostic>& SrSVGDOMIncrementalBuilder::diagnostics()
    const {
  return impl_->diagnostics;
}

void SrSVGDOM::SetDefaultColor(uint32_t color) {
  default_color_ = color;
}

void SrSVGDOM::ResetDefaultColor() {
  default_color_.reset();
}

void SrSVGDOM::Render(canvas::SrCanvas* canvas) const {
  if (root_) {
    SrSVGBox view_box = root_->viewBox();
    float local_dpi = FloatsLarger(dpi_, 0.f) ? dpi_ : 96.f;
    SrSVGTraversalState render_state;
    SrSVGRenderContext context{
        .width = view_box.width,
        .height = view_box.height,
        .dpi = local_dpi,
        .font_size = 0.f,
        .id_mapper = id_mapper_,
        .traversal_state = &render_state,
        .view_port = view_box,
        .view_box = view_box,
        .has_default_color = static_cast<uint8_t>(default_color_.has_value()),
        .default_color = default_color_.value_or(0),
    };
    root_->Render(canvas, context);
    ReplaceRuntimeDiagnostics(std::move(render_state.diagnostics));
  }
}

void SrSVGDOM::Render(canvas::SrCanvas* canvas, SrSVGBox view_port) const {
  if (root_) {
    SrSVGBox view_box = root_->viewBox();
    float local_dpi = FloatsLarger(dpi_, 0.f) ? dpi_ : 96.f;
    SrSVGTraversalState render_state;
    SrSVGRenderContext context{
        .width = view_port.width,
        .height = view_port.height,
        .dpi = local_dpi,
        .font_size = 0.f,
        .id_mapper = id_mapper_,
        .traversal_state = &render_state,
        .view_port = view_port,
        .view_box = view_box,
        .has_default_color = static_cast<uint8_t>(default_color_.has_value()),
        .default_color = default_color_.value_or(0),
    };
    root_->Render(canvas, context);
    ReplaceRuntimeDiagnostics(std::move(render_state.diagnostics));
  }
}

void SrSVGDOM::RenderAtTime(canvas::SrCanvas* canvas, double seconds) const {
  ApplyAnimations(nodes_, seconds);
  Render(canvas);
  RestoreAnimations(nodes_);
}

void SrSVGDOM::RenderAtTime(canvas::SrCanvas* canvas, SrSVGBox view_port,
                            double seconds) const {
  ApplyAnimations(nodes_, seconds);
  Render(canvas, view_port);
  RestoreAnimations(nodes_);
}

size_t SrSVGDOM::LayerCount() const {
  return root_ ? root_->ChildCount() : 0;
}

bool SrSVGDOM::LayerHasAnimations(size_t index) const {
  if (!root_ || index >= root_->children().size()) {
    return false;
  }
  return HasAnimationsRecursive(root_->children()[index]);
}

void SrSVGDOM::RenderLayerAtTime(canvas::SrCanvas* canvas, SrSVGBox view_port,
                                 size_t index, double seconds) const {
  if (!root_ || !canvas || index >= root_->children().size()) {
    return;
  }
  ApplyAnimations(nodes_, seconds);
  SrSVGBox view_box = root_->viewBox();
  float local_dpi = FloatsLarger(dpi_, 0.f) ? dpi_ : 96.f;
  SrSVGTraversalState render_state;
  SrSVGRenderContext context{
      .width = view_port.width,
      .height = view_port.height,
      .dpi = local_dpi,
      .font_size = 0.f,
      .id_mapper = id_mapper_,
      .traversal_state = &render_state,
      .view_port = view_port,
      .view_box = view_box,
      .has_default_color = static_cast<uint8_t>(default_color_.has_value()),
      .default_color = default_color_.value_or(0),
  };
  root_->RenderChildAt(canvas, context, index);
  RestoreAnimations(nodes_);
  ReplaceRuntimeDiagnostics(std::move(render_state.diagnostics));
}

SrSVGHitTestResult SrSVGDOM::HitTest(canvas::PathFactory* path_factory, float x,
                                     float y) const {
  if (!root_) {
    return {};
  }
  SrSVGBox view_box = root_->viewBox();
  return HitTest(path_factory, view_box, x, y);
}

SrSVGHitTestResult SrSVGDOM::HitTest(canvas::PathFactory* path_factory,
                                     SrSVGBox view_port, float x,
                                     float y) const {
  SrSVGHitTestResult result;
  if (!root_ || !path_factory) {
    return result;
  }

  SrSVGBox view_box = root_->viewBox();
  if (IsZero(view_box.width) || IsZero(view_box.height)) {
    view_box = SrSVGBox{0.f, 0.f, view_port.width, view_port.height};
  }

  SrSVGTraversalState traversal_state;
  SrSVGRenderContext context{
      .width = view_port.width,
      .height = view_port.height,
      .dpi = FloatsLarger(dpi_, 0.f) ? dpi_ : 96.f,
      .font_size = 0.f,
      .id_mapper = id_mapper_,
      .traversal_state = &traversal_state,
      .view_port = view_port,
      .view_box = view_box,
      .has_default_color = static_cast<uint8_t>(default_color_.has_value()),
      .default_color = default_color_.value_or(0),
  };

  float root_transform[6];
  calculate_view_box_transform(&view_port, &view_box,
                               root_->preserveAspectRatio(), root_transform);
  HitTestNode(root_, path_factory, &context, root_transform, x, y, &result);
  ReplaceRuntimeDiagnostics(std::move(traversal_state.diagnostics));
  return result;
}

const SrSVGDiagnostic* SrSVGDOM::last_diagnostic() const {
  return diagnostics_.empty() ? nullptr : &diagnostics_.back();
}

void SrSVGDOM::SetBuildDiagnostics(std::vector<SrSVGDiagnostic> diagnostics) {
  diagnostics_ = std::move(diagnostics);
  static_diagnostic_count_ = diagnostics_.size();
}

void SrSVGDOM::ReplaceRuntimeDiagnostics(
    std::vector<SrSVGDiagnostic> diagnostics) const {
  diagnostics_.resize(static_diagnostic_count_);
  diagnostics_.insert(diagnostics_.end(),
                      std::make_move_iterator(diagnostics.begin()),
                      std::make_move_iterator(diagnostics.end()));
}

void SrSVGDOM::AdoptNode(element::SrSVGNodeBase* node) {
  if (node) {
    nodes_.push_back(node);
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
