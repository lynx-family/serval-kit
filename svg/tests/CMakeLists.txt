# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
cmake_minimum_required(VERSION 3.20)
project(serval_svg_skity_tests LANGUAGES C CXX OBJCXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD 17)
if(NOT APPLE)
  message(FATAL_ERROR "These rendering tests require the Metal backend")
endif()
set(SKITY_ROOT "" CACHE PATH "Skity source directory")
set(SKITY_BUILD "" CACHE PATH "Matching compiled Skity directory")
set(GTEST_ROOT "${SKITY_ROOT}/third_party/gtest" CACHE PATH "GoogleTest source directory")
find_library(SKITY_LIBRARY NAMES skity PATHS "${SKITY_BUILD}" NO_DEFAULT_PATH)
if(NOT SKITY_LIBRARY)
  message(FATAL_ERROR "Set SKITY_BUILD to a matching compiled Skity library directory")
endif()
if(NOT EXISTS "${SKITY_ROOT}/include/skity/skity.hpp")
  message(FATAL_ERROR "Set SKITY_ROOT to the sources matching the Skity library")
endif()
if(NOT EXISTS "${GTEST_ROOT}/CMakeLists.txt")
  message(FATAL_ERROR "Set GTEST_ROOT to a GoogleTest source checkout")
endif()
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
add_subdirectory("${GTEST_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/gtest" EXCLUDE_FROM_ALL)
file(GLOB_RECURSE SVG_SOURCES CONFIGURE_DEPENDS "../src/*.cc" "../src/*.c")
add_library(serval_svg_test_subject STATIC ${SVG_SOURCES}
  ../platform/skity/SrSkityCanvas.cc
  ../platform/skity/SrSkityFilterGraph.cc
  ../platform/skity/SrSkityParagraph.cc)
target_compile_options(serval_svg_test_subject PRIVATE -fno-rtti)
target_include_directories(serval_svg_test_subject PUBLIC ../include
  "${SKITY_ROOT}/include" "${SKITY_ROOT}/third_party/glm")
target_link_libraries(serval_svg_test_subject PUBLIC "${SKITY_LIBRARY}")
add_executable(serval_svg_filter_tests filter_graph_test.mm)
target_compile_options(serval_svg_filter_tests PRIVATE -fobjc-arc -fno-rtti)
target_link_libraries(serval_svg_filter_tests PRIVATE serval_svg_test_subject GTest::gtest
  "-framework AppKit" "-framework Foundation" "-framework Metal" "-framework QuartzCore")
enable_testing()
add_test(NAME serval_svg_filter_tests COMMAND serval_svg_filter_tests)
