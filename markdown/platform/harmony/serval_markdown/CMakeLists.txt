# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(serval_markdown_harmony)

set(CMAKE_CXX_STANDARD 17)
set(SOURCE_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../")
set(NATIVERENDER_ROOT_PATH ${SOURCE_ROOT_PATH}/src/markdown/platform/harmony)
set(SRC_ROOT_PATH "${SOURCE_ROOT_PATH}/src")
set(HEADERS_PATH "${SOURCE_ROOT_PATH}/include")
set(MARKDOWN_ROOT_PATH "${SOURCE_ROOT_PATH}/src/markdown")
set(DISCOUNT_ROOT_PATH "${SOURCE_ROOT_PATH}/third_party/discount")
set(BASE_ROOT_PATH "${SOURCE_ROOT_PATH}/third_party/base")
set(DEPENDENCY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/oh_modules)
set(IS_HARMONY true)
if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()


include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

include_directories(${SOURCE_ROOT_PATH}
                    ${MARKDOWN_ROOT_PATH}
                    ${DISCOUNT_ROOT_PATH}
                     ${SRC_ROOT_PATH}
                     ${SOURCE_ROOT_PATH}/third_party
                     ${BASE_ROOT_PATH}
                     ${HEADERS_PATH}
                     )

set(TEXTRA_PATH ${SOURCE_ROOT_PATH}/third_party/lynx-textra)
include_directories(${TEXTRA_PATH}/public)

add_subdirectory(${SRC_ROOT_PATH} markdown_dir)
add_subdirectory(${DISCOUNT_ROOT_PATH} discount_dir)
add_subdirectory(${BASE_ROOT_PATH} base_dir)

file(GLOB NATIVE_SOURCES
 ${NATIVERENDER_ROOT_PATH}/*.cpp)

add_library(serval_markdown SHARED ${NATIVE_SOURCES} src/main/cpp/napi_init.cpp)



add_library(lynxtextra SHARED IMPORTED)
set_target_properties(lynxtextra
    PROPERTIES
    IMPORTED_LOCATION ${DEPENDENCY_PATH}/@lynx/lynxtextra/build/default/intermediates/libs/default/${OHOS_ARCH}/liblynxtextra.so)
target_link_libraries(serval_markdown PUBLIC lynxtextra)

target_link_libraries(serval_markdown PUBLIC
    libace_napi.z.so libace_ndk.z.so libnative_display_manager.so libnative_vsync.so libnative_drawing.so uv
    -Wl,--whole-archive markdown -Wl,--no-whole-archive lynx_base lynx_trace libhitrace_ndk.z.so libhilog_ndk.z.so
)

target_compile_definitions(serval_markdown PUBLIC ENABLE_TRACE_SYSTRACE=1)

