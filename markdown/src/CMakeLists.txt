project(markdown_source)


set(MARKDOWN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/markdown)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)

file(GLOB MARKDOWN_SOURCES
        ${MARKDOWN_DIR}/*.cc
        ${MARKDOWN_DIR}/draw/*.cc
        ${MARKDOWN_DIR}/element/*.cc
        ${MARKDOWN_DIR}/layout/*.cc
        ${MARKDOWN_DIR}/parser/*.cc
        ${MARKDOWN_DIR}/parser/embed/*.cc
        ${MARKDOWN_DIR}/style/*.cc
        ${MARKDOWN_DIR}/utils/*.cc
        ${MARKDOWN_DIR}/view/*.cc
)

add_library(markdown STATIC ${MARKDOWN_SOURCES})

target_include_directories(markdown PUBLIC
        ${MARKDOWN_DIR}
        ${INCLUDE_DIR}
)

target_link_libraries(markdown PUBLIC discount lynx_base)
target_compile_definitions(markdown PUBLIC ENABLE_TRACE_SYSTRACE=1)
