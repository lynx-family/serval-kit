# Set the minimum version of CMAKE that is required
cmake_minimum_required(VERSION 3.4.1)
project(LynxMarkdown)

set(SOURCE_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(SRC_ROOT_PATH "${SOURCE_ROOT_PATH}/src")
set(HEADERS_PATH "${SOURCE_ROOT_PATH}/include")
set(DISCOUNT_ROOT_PATH "${SOURCE_ROOT_PATH}/third_party/discount")
set(BASE_ROOT_PATH "${SOURCE_ROOT_PATH}/third_party/base")
set(IS_ANDROID true)
include_directories(
                  ${SOURCE_ROOT_PATH}
                    ${DISCOUNT_ROOT_PATH}
                     ${SRC_ROOT_PATH}
                     ${SOURCE_ROOT_PATH}/third_party
                     ${BASE_ROOT_PATH}
                     ${HEADERS_PATH}
                     )

add_library(
    serval_markdown
    SHARED
    ${SOURCE_ROOT_PATH}/src/markdown/platform/android/android_serval_markdown_view.cc
    ${SOURCE_ROOT_PATH}/src/markdown/platform/android/markdown_class_cache.cc
    ${SOURCE_ROOT_PATH}/src/markdown/platform/android/markdown_buffer_reader.cc
    ${SOURCE_ROOT_PATH}/src/markdown/platform/android/markdown_native.cc
    ${SOURCE_ROOT_PATH}/src/markdown/platform/android/markdown_platform_android.cc
    ${SOURCE_ROOT_PATH}/src/markdown/platform/android/buffer_input_stream.cc
    ${SOURCE_ROOT_PATH}/src/markdown/platform/android/markdown_java_canvas_helper.cc
    ${SOURCE_ROOT_PATH}/src/markdown/platform/android/tttext_run_delegate.cc
)

set(LYNX_TEXTRA_ARTIFACT lynxtextra)
find_package(${LYNX_TEXTRA_ARTIFACT} CONFIG)
if (${${LYNX_TEXTRA_ARTIFACT}_FOUND})
  get_target_property(LYNX_TEXTRA_INCLUDE_DIR ${LYNX_TEXTRA_ARTIFACT}::lynxtextra INTERFACE_INCLUDE_DIRECTORIES)
  include_directories(${LYNX_TEXTRA_INCLUDE_DIR})
  target_link_libraries(serval_markdown ${LYNX_TEXTRA_ARTIFACT}::lynxtextra)
endif ()

find_library(
    log-lib
    log
)

add_subdirectory(${SRC_ROOT_PATH} markdown_dir)
add_subdirectory(${DISCOUNT_ROOT_PATH} discount_dir)
add_subdirectory(${BASE_ROOT_PATH} base_dir)

target_link_libraries(serval_markdown
    ${log-lib}
    markdown lynx_base lynx_trace
)

target_compile_definitions(serval_markdown PUBLIC ENABLE_TRACE_SYSTRACE=1)
