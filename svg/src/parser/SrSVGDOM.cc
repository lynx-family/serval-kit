// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrSVGDOM.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

#include "element/SrSVGAnimation.h"
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
#include "parser/SrXMLParserError.h"
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

void ApplyAnimations(const std::vector<element::SrSVGNodeBase*>& nodes,
                     const element::IDMapper* id_mapper, double seconds) {
  for (auto* node : nodes) {
    if (node) {
      node->ApplyAnimations(seconds, id_mapper);
    }
  }
}

void RestoreAnimations(const std::vector<element::SrSVGNodeBase*>& nodes) {
  for (auto* node : nodes) {
    if (node) {
      node->RestoreAnimatedAttributes();
    }
  }
}

bool IsAnimationTag(element::SrSVGTag tag) {
  return tag == element::SrSVGTag::kAnimate ||
         tag == element::SrSVGTag::kAnimateColor ||
         tag == element::SrSVGTag::kAnimateMotion ||
         tag == element::SrSVGTag::kAnimateTransform ||
         tag == element::SrSVGTag::kSet;
}

std::string NormalizeHref(const std::string& href) {
  if (!href.empty() && href[0] == '#') {
    return href.substr(1);
  }
  return href;
}

void BindAnimation(element::SrSVGNodeBase* parent,
                   element::SrSVGAnimation* animation,
                   element::IDMapper* id_mapper) {
  if (!animation) {
    return;
  }
  const std::string& target_href = animation->TargetHref();
  if (!target_href.empty()) {
    if (!id_mapper) {
      return;
    }
    auto it = id_mapper->find(NormalizeHref(target_href));
    if (it != id_mapper->end() && it->second) {
      it->second->AddAnimation(animation);
    }
    return;
  }
  if (parent) {
    parent->AddAnimation(animation);
  }
}

void BindTargetAnimationsInNodes(
    const std::list<element::SrSVGNodeBase*>& nodes,
    element::IDMapper* id_mapper) {
  for (auto* node : nodes) {
    if (!node || !IsAnimationTag(node->Tag())) {
      continue;
    }
    auto* animation = static_cast<element::SrSVGAnimation*>(node);
    if (!animation->TargetHref().empty()) {
      BindAnimation(nullptr, animation, id_mapper);
    }
  }
}

}  // namespace

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
    case element::SrSVGTag::kAnimateColor:
    case element::SrSVGTag::kAnimateMotion:
    case element::SrSVGTag::kAnimateTransform:
    case element::SrSVGTag::kMPath:
    case element::SrSVGTag::kSet:
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
  if (IsAnimationTag(child->Tag())) {
    return false;
  }
  if (parent->Tag() == element::SrSVGTag::kAnimateMotion &&
      child->Tag() == element::SrSVGTag::kMPath) {
    return true;
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
  } else if (strcmp(el, "animateColor") == 0) {
    node = element::SrSVGAnimation::MakeAnimateColor();
  } else if (strcmp(el, "animateTransform") == 0) {
    node = element::SrSVGAnimation::MakeAnimateTransform();
  } else if (strcmp(el, "animateMotion") == 0) {
    node = element::SrSVGAnimation::MakeAnimateMotion();
  } else if (strcmp(el, "set") == 0) {
    node = element::SrSVGAnimation::MakeSet();
  } else if (strcmp(el, "mpath") == 0) {
    node = element::SrSVGAnimation::MakeMPath();
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
    if (childNode && IsAnimationTag(childNode->Tag())) {
      BindAnimation(node, static_cast<element::SrSVGAnimation*>(childNode),
                    id_mapper);
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
    svg_dom->BindTargetAnimations();
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
  const auto& animated_nodes = AnimatedNodes();
  ApplyAnimations(animated_nodes, id_mapper_, seconds);
  Render(canvas);
  RestoreAnimations(animated_nodes);
}

void SrSVGDOM::RenderAtTime(canvas::SrCanvas* canvas, SrSVGBox view_port,
                            double seconds) const {
  const auto& animated_nodes = AnimatedNodes();
  ApplyAnimations(animated_nodes, id_mapper_, seconds);
  Render(canvas, view_port);
  RestoreAnimations(animated_nodes);
}

bool SrSVGDOM::HasAnimations() const {
  return !AnimatedNodes().empty();
}

double SrSVGDOM::AnimationTimelineEndSeconds() const {
  double timeline_end = 0.0;
  for (auto* node : nodes_) {
    if (!node || !IsAnimationTag(node->Tag())) {
      continue;
    }
    const auto* animation = static_cast<const element::SrSVGAnimation*>(node);
    const double animation_end = animation->LastChangeSeconds(id_mapper_);
    if (std::isinf(animation_end)) {
      return std::numeric_limits<double>::infinity();
    }
    timeline_end = std::max(timeline_end, animation_end);
  }
  return timeline_end;
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

void SrSVGDOM::BindTargetAnimations() {
  BindTargetAnimationsInNodes(nodes_, id_mapper_);
  InvalidateAnimationCache();
}

const std::vector<element::SrSVGNodeBase*>& SrSVGDOM::AnimatedNodes() const {
  if (!animated_nodes_valid_) {
    animated_nodes_.clear();
    for (auto* node : nodes_) {
      if (node && node->HasAnimations()) {
        animated_nodes_.push_back(node);
      }
    }
    animated_nodes_valid_ = true;
  }
  return animated_nodes_;
}

void SrSVGDOM::InvalidateAnimationCache() const {
  animated_nodes_.clear();
  animated_nodes_valid_ = false;
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
