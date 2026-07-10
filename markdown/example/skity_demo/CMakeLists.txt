set(SKITY_DEMO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

if (NOT DEFINED MARKDOWN_ROOT)
    set(MARKDOWN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
endif ()

if (NOT DEFINED TEXTRA_ROOT)
    set(TEXTRA_ROOT "${MARKDOWN_ROOT}/third_party/lynx-textra")
endif ()

add_library(serval_markdown_skity_demo STATIC
        skity_demo.cc
        skity_demo.h
        skity_demo_config.h
        skity_demo_font_manager.h
        skity_demo_input.h
        skity_demo_platform_view.cc
        skity_demo_platform_view.h
        skity_demo_resource_loader.cc
        skity_demo_resource_loader.h
        skity_markdown_canvas.h
        skity_markdown_platform.cc
        skity_markdown_platform.h
        "${MARKDOWN_ROOT}/testing/markdown/frame_driven_tests/markdown_case_builder.h"
        "${MARKDOWN_ROOT}/testing/markdown/frame_driven_tests/markdown_case_builder.cc"
)

target_include_directories(serval_markdown_skity_demo PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${MARKDOWN_ROOT}"
        "${MARKDOWN_ROOT}/include"
        "${MARKDOWN_ROOT}/third_party"
        "${MARKDOWN_ROOT}/third_party/rapidjson/include"
        "${TEXTRA_ROOT}"
        "${TEXTRA_ROOT}/public"
        "${TEXTRA_ROOT}/src/ports"
        "${TEXTRA_ROOT}/third_party/skity/include"
)

target_compile_options(serval_markdown_skity_demo PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>
)

target_link_libraries(serval_markdown_skity_demo PUBLIC
        markdown
        lynxtextra
        skity
)
