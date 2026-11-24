cmake_minimum_required(VERSION 3.16)
project(serval_svg_text_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(../../platform/harmony/servalsvg/src/main/cpp servalsvg)
add_executable(text_tests text_tests.cc)
target_include_directories(text_tests PRIVATE ../../include)
target_link_libraries(text_tests PRIVATE servalsvg libnative_drawing.so)

# An app can load the same tests when the device disallows shell executables.
add_library(text_tests_napi SHARED text_tests.cc text_tests_napi.cc)
set_target_properties(text_tests_napi PROPERTIES OUTPUT_NAME text_tests)
target_compile_definitions(text_tests_napi PRIVATE SERVAL_TEXT_TESTS_EMBEDDED)
target_include_directories(text_tests_napi PRIVATE ../../include)
target_link_libraries(text_tests_napi PRIVATE servalsvg libnative_drawing.so
                                            libace_napi.z.so libhilog_ndk.z.so)
