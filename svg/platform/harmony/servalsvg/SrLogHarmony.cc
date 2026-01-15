// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "utils/SrSVGLog.h"
#include <hilog/log.h>

namespace serval {
namespace svg {
namespace log {
const unsigned int LOG_PRINT_DOMAIN = 0xFFF0;
void Log(SrLogger *sr_logger, const SrSvgLogLevel &level) {
    LogLevel priority = LogLevel::LOG_DEBUG;
    switch (level) {
    case LOG_VERBOSE:
    case LOG_DEBUG:
        priority = LogLevel::LOG_DEBUG;
        break;
    case LOG_INFO:
        priority = LogLevel::LOG_INFO;
        break;
    case LOG_WARNING:
        priority = LogLevel::LOG_WARN;
        break;
    case LOG_ERROR:
        priority = LogLevel::LOG_ERROR;
        break;
    case LOG_FATAL:
        priority = LogLevel::LOG_FATAL;
        break;
    }
    OH_LOG_Print(LOG_APP, priority, LOG_PRINT_DOMAIN, "SrSVG", "%{public}s", sr_logger->info().c_str());
}
} // namespace log
} // namespace svg
} // namespace serval
