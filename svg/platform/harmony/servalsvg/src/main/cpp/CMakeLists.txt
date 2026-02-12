# the minimum version of CMake.
cmake_minimum_required(VERSION 3.4.1)

project("servalsvg")


set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(SVG_SRC_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..)
set(CMAKE_CXX_STANDARD 17)

include_directories(
        ${NATIVERENDER_ROOT_PATH}
        ${NATIVERENDER_ROOT_PATH}/include
        ${SVG_SRC_DIRECTORY}
        ${SVG_SRC_DIRECTORY}/include)

message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-D_DEBUG)
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  # lto is both compile and link option.
  set(CMAKE_CXX_FLAGS_DEBUG
          "${CMAKE_CXX_FLAGS_DEBUG} -Og -fvisibility=default -fno-exceptions -fno-rtti")
  set(CMAKE_SHARED_LINKER_FLAGS
          "${CMAKE_SHARED_LINKER_FLAGS}")
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Release")
  # lto is both compile and link option.
  set(CMAKE_CXX_FLAGS_RELEASE
          "${CMAKE_CXX_FLAGS_RELEASE} -g -Oz -fvisibility=hidden -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections")
  set(CMAKE_SHARED_LINKER_FLAGS
          "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--hash-style=sysv -Wl,--exclude-libs,ALL,")
endif()

add_definitions(-DOS_HARMONY=0)


add_library(
        servalsvg SHARED
        # log
        ${SVG_SRC_DIRECTORY}/include/utils/SrSVGLog.h
        ${SVG_SRC_DIRECTORY}/include/utils/SrFloatComparison.h
        # parser
        ${SVG_SRC_DIRECTORY}/include/parser/SrDOM.h
        ${SVG_SRC_DIRECTORY}/include/parser/SrDOMParser.h
        ${SVG_SRC_DIRECTORY}/include/parser/SrXMLExtractor.h
        ${SVG_SRC_DIRECTORY}/include/parser/SrXMLParser.h
        ${SVG_SRC_DIRECTORY}/include/parser/SrXMLParserError.h
        ${SVG_SRC_DIRECTORY}/include/parser/SrSVGDOM.h
        ${SVG_SRC_DIRECTORY}/src/parser/SrDOM.cc
        ${SVG_SRC_DIRECTORY}/src/parser/SrDOMParser.cc
        ${SVG_SRC_DIRECTORY}/src/parser/SrXMLExtractor.c
        ${SVG_SRC_DIRECTORY}/src/parser/SrXMLParser.cc
        ${SVG_SRC_DIRECTORY}/src/parser/SrXMLParserError.cc
        ${SVG_SRC_DIRECTORY}/src/parser/SrSVGDOM.cc
        # canvas
        ${SVG_SRC_DIRECTORY}/include/canvas/SrCanvas.h
        ${SVG_SRC_DIRECTORY}/include/canvas/SrParagraph.h
        # element
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGCircle.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGContainer.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGEllipse.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGLine.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGNode.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGPath.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGPolygon.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGText.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGRect.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGShape.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGSVG.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGTypes.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGPath.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGPolyLine.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGStop.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGDefs.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGUse.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGImage.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGLinearGradient.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGRadialGradient.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGClipPath.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGMask.h
        # source files
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGStop.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGCircle.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGContainer.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGEllipse.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGLine.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGNode.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGPath.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGPolygon.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGPolyLine.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGText.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGRect.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGShape.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGSVG.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGPath.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGTypes.c
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGLinearGradient.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGRadialGradient.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGDefs.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGUse.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGImage.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGClipPath.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGMask.cc
        ${SVG_SRC_DIRECTORY}/platform/harmony/servalsvg/SrLogHarmony.cc
        #harmony
        ${NATIVERENDER_ROOT_PATH}/entry.cc
        ${NATIVERENDER_ROOT_PATH}/svg_drawable.h
        ${NATIVERENDER_ROOT_PATH}/svg_drawable.cc
        ${SVG_SRC_DIRECTORY}/include/platform/harmony/sr_harmony_canvas.h
        ${SVG_SRC_DIRECTORY}/include/platform/harmony/sr_harmony_paragraph.h
        ${SVG_SRC_DIRECTORY}/platform/harmony/servalsvg/sr_harmony_canvas.cc
        ${SVG_SRC_DIRECTORY}/platform/harmony/servalsvg/sr_harmony_paragraph.cc
        ${SVG_SRC_DIRECTORY}/include/platform/harmony/path_harmony_impl.h
        ${SVG_SRC_DIRECTORY}/platform/harmony/servalsvg/path_harmony_impl.cc
        ${SVG_SRC_DIRECTORY}/include/platform/harmony/path_factory_harmony_impl.h
        ${SVG_SRC_DIRECTORY}/platform/harmony/servalsvg/path_factory_harmony_impl.cc
)
target_link_libraries(servalsvg PUBLIC libace_napi.z.so
                                 libhilog_ndk.z.so
                                 librawfile.z.so
                                 libace_ndk.z.so
                                 libnative_drawing.so)