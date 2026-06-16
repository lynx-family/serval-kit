cmake_minimum_required(VERSION 3.15)

project(serval_markdown_macos_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  message(FATAL_ERROR "The macOS markdown demo can only be built on Darwin.")
endif ()

set(MARKDOWN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(TEXTRA_ROOT "${MARKDOWN_ROOT}/third_party/lynx-textra")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  set(TEXTRA_BUILD_TYPE "is_debug=false")
else ()
  set(TEXTRA_BUILD_TYPE "is_debug=true")
endif ()

execute_process(
  COMMAND python3 "${TEXTRA_ROOT}/tools/build_cmake_environment.py"
          --gn-args "${TEXTRA_BUILD_TYPE} use_flutter_cxx=false use_xcode=true"
  WORKING_DIRECTORY "${TEXTRA_ROOT}"
  RESULT_VARIABLE TEXTRA_GN_RESULT
)
if (NOT TEXTRA_GN_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to generate Textra GN project for CMake.")
endif ()

execute_process(
  COMMAND python3 "${TEXTRA_ROOT}/tools/build_cmake_environment.py"
          --target //platform/mac:lynxtextra
  WORKING_DIRECTORY "${TEXTRA_ROOT}"
  RESULT_VARIABLE TEXTRA_CMAKE_RESULT
)
if (NOT TEXTRA_CMAKE_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to generate Textra CMake target.")
endif ()

set(SKITY_MTL_BACKEND ON CACHE BOOL "" FORCE)
set(SKITY_SW_RENDERER ON CACHE BOOL "" FORCE)
set(SKITY_GL_BACKEND OFF CACHE BOOL "" FORCE)
set(SKITY_VK_BACKEND OFF CACHE BOOL "" FORCE)
set(SKITY_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SKITY_TEST OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory("${TEXTRA_ROOT}/third_party/skity" skity.out)
add_subdirectory("${TEXTRA_ROOT}/third_party/glfw" glfw.out)
add_subdirectory("${TEXTRA_ROOT}/platform/mac" lynxtextra.out)
add_subdirectory("${MARKDOWN_ROOT}/third_party/discount" discount.out)
add_subdirectory("${MARKDOWN_ROOT}/third_party/base" lynx_base.out)
target_include_directories(lynx_base PUBLIC "${MARKDOWN_ROOT}/third_party")
target_include_directories(lynx_trace PUBLIC "${MARKDOWN_ROOT}/third_party")
add_subdirectory("${MARKDOWN_ROOT}/src" markdown.out)
target_include_directories(markdown PUBLIC
  "${TEXTRA_ROOT}"
  "${TEXTRA_ROOT}/public"
  "${TEXTRA_ROOT}/src/ports"
)

add_executable(serval_markdown_macos_demo
  app/markdown_macos_demo.mm
  app/mtl_app.mm
  "${MARKDOWN_ROOT}/testing/markdown/markdown_case_builder.cc"
)

target_include_directories(serval_markdown_macos_demo PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/app"
  "${MARKDOWN_ROOT}"
  "${MARKDOWN_ROOT}/include"
  "${MARKDOWN_ROOT}/third_party"
  "${MARKDOWN_ROOT}/third_party/rapidjson/include"
  "${TEXTRA_ROOT}"
  "${TEXTRA_ROOT}/public"
  "${TEXTRA_ROOT}/src/ports"
  "${TEXTRA_ROOT}/third_party/glfw/include"
  "${TEXTRA_ROOT}/third_party/skity/include"
)

target_compile_definitions(serval_markdown_macos_demo PRIVATE
  FONT_ROOT="${TEXTRA_ROOT}/fonts/"
  MARKDOWN_CASES_ROOT="${MARKDOWN_ROOT}/testing/markdown/cases"
)

target_compile_options(serval_markdown_macos_demo PRIVATE
  $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc -Wno-unused-parameter>
  $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>
)

target_link_libraries(serval_markdown_macos_demo PRIVATE
  markdown
  lynxtextra
  skity
  glfw
  "-framework AppKit"
  "-framework CoreText"
  "-framework QuartzCore"
  "-framework MetalKit"
  "-framework Metal"
  "-framework IOKit"
)
