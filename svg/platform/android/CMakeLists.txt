# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html. For
# more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.10.2)
# cmake_minimum_required(VERSION 3.18.1)

# Declares the project name. The project name can be accessed via ${
# PROJECT_NAME}, Since this is the top level CMakeLists.txt, the project name is
# also accessible with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync
# within the top level build script scope).
project("serval_svg")

# Creates and names a library, sets it as either STATIC or SHARED, and provides
# the relative paths to its source code. You can define multiple libraries, and
# CMake builds them for you. Gradle automatically packages shared libraries with
# your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define the
# target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME} is
# preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here; for
# GameActivity/NativeActivity derived applications, the same library name must
# be used in the AndroidManifest.xml file.
set(SVG_SRC_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(SVG_PLATFORM_DIRECTORY ${SVG_SRC_DIRECTORY}/platform)
include_directories(
        AFTER
        ${SVG_SRC_DIRECTORY}/include/
        ${SVG_SRC_DIRECTORY}/include/utils/
        ${SVG_PLATFORM_DIRECTORY}/android/)

message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")

# Android 15  16KB
if (${ANDROID_ABI} STREQUAL "arm64-v8a" OR ${ANDROID_ABI} STREQUAL "x86_64")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384")
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
          "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fvisibility=hidden -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections")
  set(CMAKE_SHARED_LINKER_FLAGS
          "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--hash-style=sysv -Wl,--exclude-libs,ALL,")

  # To generate output.map to analyse so size, we should set -fuse-ld=lld -Wl,-Map=output.map and should not set -flto=full
  # set(CMAKE_SHARED_LINKER_FLAGS
  #       "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld -Wl,-Map=output.map")
endif()

add_definitions(-DOS_ANDROID=1)

add_library(
        ${CMAKE_PROJECT_NAME} SHARED
        # engine
        ${SVG_PLATFORM_DIRECTORY}/android/src/main/cpp/SVGRenderEngine.cc
        # log
        ${SVG_SRC_DIRECTORY}/include/utils/SrSVGLog.h
        ${SVG_SRC_DIRECTORY}/include/utils/SrFloatComparison.h
        ${SVG_PLATFORM_DIRECTORY}/android/src/main/cpp/SrLogAndroid.cc
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
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGG.h
        ${SVG_SRC_DIRECTORY}/include/element/SrSVGText.h
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
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGRect.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGShape.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGSVG.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGTypes.c
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGLinearGradient.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGRadialGradient.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGDefs.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGUse.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGImage.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGClipPath.cc
        ${SVG_SRC_DIRECTORY}/src/element/SrSVGText.cc
        # Android platform
        ${SVG_SRC_DIRECTORY}/include/platform/android/SrAndroidCanvas.h
        ${SVG_SRC_DIRECTORY}/include/platform/android/SrAndroidPathFactory.h
        ${SVG_SRC_DIRECTORY}/include/platform/android/SrAndroidPath.h
        ${SVG_SRC_DIRECTORY}/include/platform/android/SrJNIUtils.h
        ${SVG_SRC_DIRECTORY}/include/platform/android/SrScopedJavaRef.h
        ${SVG_SRC_DIRECTORY}/include/platform/android/SrAndroidParagraphFactory.h
        ${SVG_SRC_DIRECTORY}/include/platform/android/SrAndroidParagraph.h

        ${SVG_PLATFORM_DIRECTORY}/android/src/main/cpp/SrAndroidCanvas.cc
        ${SVG_PLATFORM_DIRECTORY}/android/src/main/cpp/SrAndroidPathFactory.cc
        ${SVG_PLATFORM_DIRECTORY}/android/src/main/cpp/SrAndroidPath.cc
        ${SVG_PLATFORM_DIRECTORY}/android/src/main/cpp/SrJNIUtils.cc
        ${SVG_PLATFORM_DIRECTORY}/android/src/main/cpp/SrScopedJavaRef.cc
        ${SVG_PLATFORM_DIRECTORY}/android/src/main/cpp/SrAndroidParagraphFactory.cc
        ${SVG_PLATFORM_DIRECTORY}/android/src/main/cpp/SrAndroidParagraph.cc



)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:SR_SVG_MIN_LOG_LEVEL=0>"
        "$<$<CONFIG:Release>:SR_SVG_MIN_LOG_LEVEL=3>"
        )


# Specifies libraries CMake should link to your target library. You can link
# libraries from various origins, such as libraries defined in this build
# script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(
        ${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        android
        log
)

target_include_directories(
        ${CMAKE_PROJECT_NAME}
        PUBLIC
        ${SVG_SRC_DIRECTORY}/include

)
