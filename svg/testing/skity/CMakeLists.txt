# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

cmake_minimum_required(VERSION 3.15)
project(ServalPatternSkityTests LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD 17)
get_filename_component(SERVAL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(SKITY_SOURCE_DIR "" CACHE PATH "Skity source checkout containing include/skity")
set(SKITY_BUILD_DIR "" CACHE PATH "Configured Skity build containing libskity")
if(NOT EXISTS "${SKITY_SOURCE_DIR}/include/skity/skity.hpp")
  message(FATAL_ERROR "Set SKITY_SOURCE_DIR to the Skity source directory")
endif()
find_library(SKITY_LIBRARY NAMES skity PATHS "${SKITY_BUILD_DIR}" "${SKITY_BUILD_DIR}/lib" NO_DEFAULT_PATH)
if(NOT SKITY_LIBRARY)
  message(FATAL_ERROR "Build Skity first and set SKITY_BUILD_DIR")
endif()

file(GLOB SERVAL_SOURCES CONFIGURE_DEPENDS
  "${SERVAL_ROOT}/svg/src/parser/*.cc"
  "${SERVAL_ROOT}/svg/src/parser/*.c"
  "${SERVAL_ROOT}/svg/src/element/*.cc"
  "${SERVAL_ROOT}/svg/src/element/*.c"
  "${SERVAL_ROOT}/svg/src/utils/*.cc")
add_executable(pattern_percent_runner pattern_percent_runner.mm ${SERVAL_SOURCES}
  "${SERVAL_ROOT}/svg/platform/skity/SrSkityCanvas.cc"
  "${SERVAL_ROOT}/svg/platform/skity/SrSkityParagraph.cc")
target_include_directories(pattern_percent_runner PRIVATE
  "${SERVAL_ROOT}/svg/include" "${SKITY_SOURCE_DIR}/include" "${SKITY_BUILD_DIR}/include")
target_compile_options(pattern_percent_runner PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
target_compile_definitions(pattern_percent_runner PRIVATE SR_SVG_MIN_LOG_LEVEL=3)
target_link_libraries(pattern_percent_runner PRIVATE "${SKITY_LIBRARY}"
  "-framework Foundation" "-framework CoreGraphics" "-framework CoreText" "-framework ImageIO")
