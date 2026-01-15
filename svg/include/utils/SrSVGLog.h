// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_UTILS_SRSVGLOG_H_
#define SVG_INCLUDE_UTILS_SRSVGLOG_H_

#include <cstdarg>
#include <cstdio>
#include <string>

namespace serval {
namespace svg {
namespace log {

enum SrSvgLogLevel {
  LOG_VERBOSE = 0,
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_FATAL
};

class SrLogger;
void Log(serval::svg::log::SrLogger* sr_logger, const SrSvgLogLevel& level);

#define SR_SVG_LOG_LEVEL_VERBOSE 0
#define SR_SVG_LOG_LEVEL_DEBUG 1
#define SR_SVG_LOG_LEVEL_INFO 2
#define SR_SVG_LOG_LEVEL_WARNING 3
#define SR_SVG_LOG_LEVEL_ERROR 4
#define SR_SVG_LOG_LEVEL_FATAL 5
#define SR_SVG_LOG_LEVEL_NUM 6

#ifndef SR_SVG_MIN_LOG_LEVEL
#define SR_SVG_MIN_LOG_LEVEL SR_SVG_LOG_LEVEL_VERBOSE
#endif

/**
 * @brief Get last path form macro __FILE__ at compile time. This function running at compile time will be enable only when passed const expression as parameters.
 */
static constexpr const char* GetLastPath(const char* filename, int32_t length) {
  for (int32_t i = length; i > 0; --i) {
    if (filename[i] == '/' || filename[i] == '\\') {
      return filename + i + 1;
    }
  }
  return filename;
}

#define SR_SVG_LOG_IS_ON(level) \
  ((serval::svg::log::LOG_##level) >= serval::svg::log::LOG_VERBOSE)

#define SR_SVG_LOGGER(level)                                                   \
  serval::svg::log::SrLogger(                                                  \
      serval::svg::log::GetLastPath(__FILE__, sizeof(__FILE__) - 1), __LINE__, \
      serval::svg::log::LOG_##level)

#define SR_SVG_LAZY_LOGGER(logger, condition) !(condition) ? (void)0 : (logger)

#if SR_SVG_MIN_LOG_LEVEL <= SR_SVG_LOG_LEVEL_VERBOSE
#define LOGV(format, ...)                                                 \
  {                                                                       \
    SR_SVG_LAZY_LOGGER(SR_SVG_LOGGER(VERBOSE), SR_SVG_LOG_IS_ON(VERBOSE)) \
    (format, ##__VA_ARGS__);                                              \
  }
#else
#define LOGV(msg, ...)
#endif

#if SR_SVG_MIN_LOG_LEVEL <= SR_SVG_LOG_LEVEL_DEBUG
#define LOGD(format, ...)                                             \
  {                                                                   \
    SR_SVG_LAZY_LOGGER(SR_SVG_LOGGER(DEBUG), SR_SVG_LOG_IS_ON(DEBUG)) \
    (format, ##__VA_ARGS__);                                          \
  }
#else
#define LOGD(msg, ...)
#endif

#if SR_SVG_MIN_LOG_LEVEL <= SR_SVG_LOG_LEVEL_INFO
#define LOGI(format, ...)                                           \
  {                                                                 \
    SR_SVG_LAZY_LOGGER(SR_SVG_LOGGER(INFO), SR_SVG_LOG_IS_ON(INFO)) \
    (format, ##__VA_ARGS__);                                        \
  }
#else
#define LOGI(msg, ...)
#endif

#if SR_SVG_MIN_LOG_LEVEL <= SR_SVG_LOG_LEVEL_WARNING
#define LOGW(format, ...)                                                 \
  {                                                                       \
    SR_SVG_LAZY_LOGGER(SR_SVG_LOGGER(WARNING), SR_SVG_LOG_IS_ON(WARNING)) \
    (format, ##__VA_ARGS__);                                              \
  }
#else
#define LOGW(msg, ...)
#endif

#if SR_SVG_MIN_LOG_LEVEL <= SR_SVG_LOG_LEVEL_ERROR
#define LOGE(format, ...)                                             \
  {                                                                   \
    SR_SVG_LAZY_LOGGER(SR_SVG_LOGGER(ERROR), SR_SVG_LOG_IS_ON(ERROR)) \
    (format, ##__VA_ARGS__);                                          \
  }
#else
#define LOGE(msg, ...)
#endif

#if SR_SVG_MIN_LOG_LEVEL <= SR_SVG_LOG_LEVEL_FATAL
#define LOGF(format, ...)                                             \
  {                                                                   \
    SR_SVG_LAZY_LOGGER(SR_SVG_LOGGER(FATAL), SR_SVG_LOG_IS_ON(FATAL)) \
    (format, ##__VA_ARGS__);                                          \
  }
#else
#define LOGF(msg, ...)
#endif

class SrLogger {
 public:
  SrLogger(const char* file, int line, SrSvgLogLevel level)
      : file_(file), line_(line), level_(level) {
    info_ = FormatString("[%s:%s(%d)]: ", SrLogger::GetLogLevelName(level_),
                         file_.c_str(), line_);
  }

  void operator()(const char* format, ...) {
    va_list args;
    va_start(args, format);
    info_ += FormatStringWithVaList(format, args);
    va_end(args);
  }

  ~SrLogger() {
#if defined(OS_ANDROID) || defined(OS_IOS) || defined(OS_HARMONY)
    serval::svg::log::Log(this, level_);
#else
    std::string new_line(info_.c_str());
    printf("%s\n", new_line.c_str());
#endif
    if (level_ == LOG_FATAL) {
      abort();
    }
  }

  const std::string& info() const { return info_; }

  static const char* GetLogLevelName(int32_t level) {
    static const char* const kSrSvgLogLevelName[SR_SVG_LOG_LEVEL_NUM] = {
        "VERBOSE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};
    if (level >= 0 && level < SR_SVG_LOG_LEVEL_NUM) {
      return kSrSvgLogLevelName[level];
    }
    return "UNKNOWN";
  }

 private:
  static std::string FormatString(const char* format, ...) {
    std::string msg;
    va_list args;
    va_start(args, format);
    msg = FormatStringWithVaList(format, args);
    va_end(args);
    return msg;
  }

  static std::string FormatStringWithVaList(const char* format, va_list args) {
    int length, size = 100;
    char* mes = nullptr;
    if ((mes = static_cast<char*>(malloc(size * sizeof(char)))) == nullptr) {
      return "";
    }
    while (true) {
      va_list copy_args;
      va_copy(copy_args, args);
      length = vsnprintf(mes, size, format, copy_args);
      va_end(copy_args);
      if (length > -1 && length < size) {
        break;
      }
      size *= 2;
      char* clone = static_cast<char*>(realloc(mes, size * sizeof(char)));
      if (clone == nullptr) {
        break;
      } else {
        mes = clone;
        clone = nullptr;
      }
    }
    std::string message = mes;
    free(mes);
    mes = nullptr;
    return message;
  }

 private:
  SrLogger(const SrLogger&) = delete;
  SrLogger& operator=(const SrLogger&) = delete;

 private:
  std::string info_;
  std::string file_;
  int line_;
  SrSvgLogLevel level_;
};

}  // namespace log
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_UTILS_SRSVGLOG_H_
