// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRSVGPARSELIMITS_H_
#define SVG_INCLUDE_PARSER_SRSVGPARSELIMITS_H_

#include <cstddef>

namespace serval {
namespace svg {
namespace parser {

// Bound input, copied DOM storage, and the later recursive SVG build/render.
constexpr size_t kSVGMaxDocumentBytes = 16 * 1024 * 1024;
constexpr size_t kSVGMaxDOMBytes = 32 * 1024 * 1024;
constexpr size_t kSVGMaxAttributeBytes = 1024 * 1024;
constexpr size_t kSVGMaxNodes = 65536;
constexpr int kSVGMaxDepth = 128;

}  // namespace parser
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PARSER_SRSVGPARSELIMITS_H_
