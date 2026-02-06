project(markdown_tests)

file(GLOB MARKDOWN_TESTS_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
set(ROOT_DIR ../..)
include_directories(${ROOT_DIR}/third_party/lynx-textra/public)
include_directories(${ROOT_DIR}/third_party/lynx-textra)
include_directories(${ROOT_DIR}/third_party/googletest/googletest/include)
include_directories(${ROOT_DIR}/third_party/rapidjson/include)
add_compile_definitions(
        RAPIDJSON_HAS_STDSTRING=1
        RAPIDJSON_HAS_CXX11_RANGE_FOR
        RAPIDJSON_HAS_CXX11_RVALUE_REFS
        RAPIDJSON_HAS_CXX11_TYPETRAITS
        RAPIDJSON_HAS_CXX11_NOEXCEPT
)

add_subdirectory(mock_textra)
add_subdirectory(../../third_party/googletest googletest)
add_executable(markdown_tests ${MARKDOWN_TESTS_SOURCES})

target_link_libraries(markdown_tests PUBLIC markdown mock_textra gtest_main lynx_base)
