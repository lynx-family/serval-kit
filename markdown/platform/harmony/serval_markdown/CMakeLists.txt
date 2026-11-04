# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(serval_markdown_harmony)

# The OHOS SDK uses Clang, so its LLVM directory is not a GCC toolchain.
set(CMAKE_C_COMPILER_EXTERNAL_TOOLCHAIN "")
set(CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN "")

set(CMAKE_CXX_STANDARD 17)
set(SOURCE_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../")
set(NATIVERENDER_ROOT_PATH ${SOURCE_ROOT_PATH}/src/markdown/platform/harmony)
set(SRC_ROOT_PATH "${SOURCE_ROOT_PATH}/src")
set(HEADERS_PATH "${SOURCE_ROOT_PATH}/include")
set(MARKDOWN_ROOT_PATH "${SOURCE_ROOT_PATH}/src/markdown")
set(DISCOUNT_ROOT_PATH "${SOURCE_ROOT_PATH}/third_party/discount")
set(DEPENDENCY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/oh_modules)
if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

set(LYNX_TEXTRA_ARTIFACT lynxtextra)
find_package(${LYNX_TEXTRA_ARTIFACT} CONFIG)
if (${${LYNX_TEXTRA_ARTIFACT}_FOUND})
    get_target_property(LYNX_TEXTRA_INCLUDE_DIR
        ${LYNX_TEXTRA_ARTIFACT}::lynxtextra
        INTERFACE_INCLUDE_DIRECTORIES)
    include_directories(${LYNX_TEXTRA_INCLUDE_DIR})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

include_directories(${SOURCE_ROOT_PATH}
                    ${MARKDOWN_ROOT_PATH}
                    ${DISCOUNT_ROOT_PATH}
                     ${SRC_ROOT_PATH}
                     ${SOURCE_ROOT_PATH}/third_party
                     ${HEADERS_PATH}
                     )

add_subdirectory(${SRC_ROOT_PATH} markdown_dir)
add_subdirectory(${DISCOUNT_ROOT_PATH} discount_dir)

target_compile_definitions(markdown PRIVATE
    MARKDOWN_ENABLE_PARSER_PROVIDER=1
)

file(GLOB NATIVE_SOURCES
 ${NATIVERENDER_ROOT_PATH}/*.cpp)

add_library(serval_markdown SHARED ${NATIVE_SOURCES} src/main/cpp/napi_init.cpp)

target_compile_options(serval_markdown PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Werror -fno-exceptions -fno-rtti -fvisibility=hidden -ffunction-sections -fdata-sections>
)
set_property(TARGET serval_markdown APPEND_STRING PROPERTY LINK_FLAGS
             " -Wl,--gc-sections")

if (${${LYNX_TEXTRA_ARTIFACT}_FOUND})
    target_link_libraries(serval_markdown PUBLIC ${LYNX_TEXTRA_ARTIFACT}::lynxtextra)
endif()

target_link_libraries(serval_markdown PUBLIC
    libace_napi.z.so libace_ndk.z.so libnative_display_manager.so libnative_vsync.so libnative_drawing.so uv
    -Wl,--whole-archive markdown -Wl,--no-whole-archive libhilog_ndk.z.so
)


set(MARKDOWN_RELEASE_COMPILE_OPTIONS -Oz -flto=full)
set(MARKDOWN_RELEASE_LINK_FLAGS " -flto=full")

foreach (target serval_markdown markdown discount)
  target_compile_options(${target} PRIVATE
      "$<$<CONFIG:Release>:${MARKDOWN_RELEASE_COMPILE_OPTIONS}>")
endforeach ()

set_property(TARGET serval_markdown APPEND_STRING PROPERTY LINK_FLAGS_RELEASE
             "${MARKDOWN_RELEASE_LINK_FLAGS}")
