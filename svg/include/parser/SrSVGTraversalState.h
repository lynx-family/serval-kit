// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRSVGTRAVERSALSTATE_H_
#define SVG_INCLUDE_PARSER_SRSVGTRAVERSALSTATE_H_

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "parser/SrSVGDOM.h"

namespace serval {
namespace svg {
namespace parser {

struct SrSVGTraversalState {
  std::unordered_set<std::string> active_use_ids;
  std::vector<SrSVGDiagnostic> diagnostics;

  void Report(::SrSVGDiagnosticCode code, const char* message,
              const char* subject, bool fatal) {
    SrSVGDiagnostic diagnostic;
    diagnostic.code = code;
    diagnostic.message = message ? message : "";
    diagnostic.subject = subject ? subject : "";
    diagnostic.fatal = fatal;
    if (!diagnostics.empty()) {
      const auto& last = diagnostics.back();
      if (last.code == diagnostic.code && last.message == diagnostic.message &&
          last.subject == diagnostic.subject &&
          last.fatal == diagnostic.fatal) {
        return;
      }
    }
    diagnostics.push_back(std::move(diagnostic));
  }
};

}  // namespace parser
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PARSER_SRSVGTRAVERSALSTATE_H_
