// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/skity/SrSkityParagraph.h"
#include "canvas/SrParagraph.h"
namespace serval {
namespace svg {
namespace canvas {

std::unique_ptr<canvas::ParagraphFactory> CreateParagraphFactoryFactory(
    const SrCanvas* srCanvas) {
  return std::unique_ptr<canvas::ParagraphFactory>(
      new canvas::SrSkityParagraphFactory(srCanvas));
}

}  // namespace canvas
}  // namespace svg
}  // namespace serval
