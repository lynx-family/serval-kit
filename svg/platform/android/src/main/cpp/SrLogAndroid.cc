// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <android/log.h>

#include "SrSVGLog.h"

namespace serval {
namespace svg {
namespace log {

void Log(SrLogger* sr_logger, const SrSvgLogLevel& level) {
  __android_log_print(ANDROID_LOG_VERBOSE, "SrSVG", "%s",
                      sr_logger->info().c_str());
}

}  // namespace log
}  // namespace svg
}  // namespace serval
