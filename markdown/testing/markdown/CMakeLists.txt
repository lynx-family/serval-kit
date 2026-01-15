project(markdown_tests)

file(GLOB MARKDOWN_TESTS_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
add_compile_definitions(
        RAPIDJSON_HAS_STDSTRING=1
        RAPIDJSON_HAS_CXX11_RANGE_FOR
        RAPIDJSON_HAS_CXX11_RVALUE_REFS
        RAPIDJSON_HAS_CXX11_TYPETRAITS
        RAPIDJSON_HAS_CXX11_NOEXCEPT
)

add_executable(markdown_tests ${MARKDOWN_TESTS_SOURCES})

target_link_libraries(markdown_tests PUBLIC markdown tttext_static gtest gtest_main lynx_base
        "-framework AppKit"
        "-framework CoreData"
        "-framework CoreTelephony"
        "-framework GLUT"
        "-framework OpenGL"
)
