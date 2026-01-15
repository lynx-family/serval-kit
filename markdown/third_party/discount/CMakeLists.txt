# third_party/discount/CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(discount_project)

set(DISCOUNT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/discount_lite/cstring.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/discount_lite/cstring_private.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/discount_lite/markdown.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/discount_lite/markdown.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/discount_lite/resource.c"
)

add_library(discount STATIC ${DISCOUNT_SOURCES})

target_include_directories(discount PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})