// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRSVGDOM_H_
#define SVG_INCLUDE_PARSER_SRSVGDOM_H_

#include <cstddef>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "canvas/SrCanvas.h"
#include "element/SrSVGSVG.h"
#include "parser/SrDOM.h"

namespace serval {
namespace svg {
namespace parser {

struct SrSVGDiagnostic {
  SrSVGDiagnosticCode code{SR_SVG_DIAGNOSTIC_NONE};
  std::string message;
  std::string subject;
  bool fatal{false};
};

class SrSVGDOM {
 public:
  static std::unique_ptr<SrSVGDOM> make(const char*, size_t,
                                        std::vector<SrSVGDiagnostic>*);
  ~SrSVGDOM();
  explicit SrSVGDOM(element::SrSVGSVG* root, element::IDMapper* id_mapper,
                    std::list<element::SrSVGNodeBase*>&& holder,
                    std::shared_ptr<SrDOM> xml_dom)
      : root_(root),
        id_mapper_(id_mapper),
        nodes_(std::move(holder)),
        xml_dom_(xml_dom) {}

  float dpi_{0.f};
  std::optional<uint32_t> default_color_;
  void SetDefaultColor(uint32_t color);
  void ResetDefaultColor();
  void Render(canvas::SrCanvas* canvas) const;
  void Render(canvas::SrCanvas* canvas, SrSVGBox view_port) const;
  void RenderAtTime(canvas::SrCanvas* canvas, double seconds) const;
  void RenderAtTime(canvas::SrCanvas* canvas, SrSVGBox view_port,
                    double seconds) const;
  bool HasAnimations() const;
  double AnimationTimelineEndSeconds() const;
  const std::vector<SrSVGDiagnostic>& diagnostics() const {
    return diagnostics_;
  }
  const SrSVGDiagnostic* last_diagnostic() const;
  void SetBuildDiagnostics(std::vector<SrSVGDiagnostic> diagnostics);
  void ReplaceRuntimeDiagnostics(
      std::vector<SrSVGDiagnostic> diagnostics) const;
  void BindTargetAnimations();

 private:
  const std::vector<element::SrSVGNodeBase*>& AnimatedNodes() const;
  void InvalidateAnimationCache() const;

  element::SrSVGSVG* root_;
  element::IDMapper* id_mapper_;
  std::list<element::SrSVGNodeBase*> nodes_;
  //release SrDOM after rendering is complete
  std::shared_ptr<SrDOM> xml_dom_;
  mutable std::vector<SrSVGDiagnostic> diagnostics_;
  mutable size_t static_diagnostic_count_{0};
  mutable bool animated_nodes_valid_{false};
  mutable std::vector<element::SrSVGNodeBase*> animated_nodes_;
};

}  // namespace parser
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PARSER_SRSVGDOM_H_
