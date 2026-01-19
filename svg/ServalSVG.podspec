# Replace with the name of your library
Pod::Spec.new do |s|
  s.name             = 'ServalSVG'
  s.version = begin
    raw_version = ENV['POD_VERSION'] || File.read('SERVAL_KIT_VERSION').strip
    raw_version
  end
  s.summary          = 'A library for parsing and rendering SVG documents'
  s.homepage         = 'https://github.com/lynx-family/serval-kit'

  # Replace with the path to your library source directory
  s.source = { :git => 'https://github.com/lynx-family/serval-kit.git', }.tap do |source_hash|
    source_hash[:commit] = ENV['SERVAL_KIT_COMMIT_ID']
end
  # Replace with the path to your library header files
#   s.ios.public_header_files = 'path/to/headers/**/*.h'

  s.license          = { :type => 'Apache 2.0' }

  s.author           = 'Lynx'

#   s.platform         = :ios, '12.0'
  s.ios.deployment_target = "9.0"

  s.public_header_files = "svg/include/platform/iOS/SrSVG.h",
                          "svg/include/platform/iOS/SrSVGView.h"

  s.private_header_files = # element
                           "svg/include/element/SrSVGCircle.h",
                           "svg/include/element/SrSVGClipPath.h",
                           "svg/include/element/SrSVGContainer.h",
                           "svg/include/element/SrSVGDefs.h",
                           "svg/include/element/SrSVGEllipse.h",
                           "svg/include/element/SrSVGG.h",
                           "svg/include/element/SrSVGImage.h",
                           "svg/include/element/SrSVGLine.h",
                           "svg/include/element/SrSVGLinearGradient.h",
                           "svg/include/element/SrSVGNode.h",
                           "svg/include/element/SrSVGPath.h",
                           "svg/include/element/SrSVGPolyLine.h",
                           "svg/include/element/SrSVGPolygon.h",
                           "svg/include/element/SrSVGRadialGradient.h",
                           "svg/include/element/SrSVGRect.h",
                           "svg/include/element/SrSVGSVG.h",
                           "svg/include/element/SrSVGShape.h",
                           "svg/include/element/SrSVGStop.h",
                           "svg/include/element/SrSVGText.h",
                           "svg/include/element/SrSVGTypes.h",
                           "svg/include/element/SrSVGUse.h",
                           # parser
                           "svg/include/parser/SrDOM.h",
                           "svg/include/parser/SrDOMParser.h",
                           "svg/include/parser/SrSVGDOM.h",
                           "svg/include/parser/SrXMLExtractor.h",
                           "svg/include/parser/SrXMLParser.h",
                           "svg/include/parser/SrXMLParserError.h",
                           # canvas
                           "svg/include/canvas/SrCanvas.h",
                           "svg/include/canvas/SrParagraph.h",
                           "svg/include/platform/iOS/SrIOSCanvas.h",
                           "svg/include/platform/iOS/SrIOSParagraph.h",
                           # include
                           "svg/include/utils/SrFloatComparison.h",
                           "svg/include/utils/SrSVGLog.h"

  s.source_files =  # element
                    "svg/include/element/SrSVGCircle.h",
                    "svg/include/element/SrSVGClipPath.h",
                    "svg/include/element/SrSVGContainer.h",
                    "svg/include/element/SrSVGDefs.h",
                    "svg/include/element/SrSVGEllipse.h",
                    "svg/include/element/SrSVGG.h",
                    "svg/include/element/SrSVGImage.h",
                    "svg/include/element/SrSVGLine.h",
                    "svg/include/element/SrSVGLinearGradient.h",
                    "svg/include/element/SrSVGNode.h",
                    "svg/include/element/SrSVGPath.h",
                    "svg/include/element/SrSVGPolyLine.h",
                    "svg/include/element/SrSVGPolygon.h",
                    "svg/include/element/SrSVGRadialGradient.h",
                    "svg/include/element/SrSVGRect.h",
                    "svg/include/element/SrSVGSVG.h",
                    "svg/include/element/SrSVGShape.h",
                    "svg/include/element/SrSVGTypes.h",
                    "svg/include/element/SrSVGUse.h",
                    "svg/include/element/SrSVGStop.h",
                    "svg/include/element/SrSVGText.h",
                    "svg/src/element/SrSVGCircle.cc",
                    "svg/src/element/SrSVGClipPath.cc",
                    "svg/src/element/SrSVGContainer.cc",
                    "svg/src/element/SrSVGDefs.cc",
                    "svg/src/element/SrSVGEllipse.cc",
                    "svg/src/element/SrSVGImage.cc",
                    "svg/src/element/SrSVGLine.cc",
                    "svg/src/element/SrSVGLinearGradient.cc",
                    "svg/src/element/SrSVGNode.cc",
                    "svg/src/element/SrSVGPath.cc",
                    "svg/src/element/SrSVGPolyLine.cc",
                    "svg/src/element/SrSVGPolygon.cc",
                    "svg/src/element/SrSVGRadialGradient.cc",
                    "svg/src/element/SrSVGRect.cc",
                    "svg/src/element/SrSVGSVG.cc",
                    "svg/src/element/SrSVGShape.cc",
                    "svg/src/element/SrSVGStop.cc",
                    "svg/src/element/SrSVGText.cc",
                    "svg/src/element/SrSVGTypes.c",
                    "svg/src/element/SrSVGUse.cc",
                    # parser
                    "svg/include/parser/SrDOM.h",
                    "svg/include/parser/SrDOMParser.h",
                    "svg/include/parser/SrSVGDOM.h",
                    "svg/include/parser/SrXMLExtractor.h",
                    "svg/include/parser/SrXMLParser.h",
                    "svg/include/parser/SrXMLParserError.h",
                    "svg/src/parser/SrDOM.cc",
                    "svg/src/parser/SrDOMParser.cc",
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/parser/SrXMLExtractor.c",
                    "svg/src/parser/SrXMLParser.cc",
                    "svg/src/parser/SrXMLParserError.cc",
                    # canvas
                    "svg/include/platform/iOS/SrSVG.h",
                    "svg/include/platform/iOS/SrSVGView.h",
                    "svg/include/platform/iOS/SrIOSCanvas.h",
                    "svg/include/platform/iOS/SrIOSParagraph.h",
                    "svg/include/canvas/SrCanvas.h",
                    "svg/include/canvas/SrParagraph.h",
                    "svg/platform/iOS/SrIOSCanvas.mm",
                    "svg/platform/iOS/SrSVG.mm",
                    "svg/platform/iOS/SrSVGView.mm",
                    "svg/platform/iOS/SrIOSParagraph.mm",
                    # cpp
                    "svg/include/utils/SrFloatComparison.h",
                    "svg/include/utils/SrSVGLog.h",
                    "svg/platform/iOS/SrLogDarwin.mm"

  # s.exclude_files = "src/canvas/android/*.h",
  #                   "src/canvas/android/*.cc",
  #                   "src/canvas/harmony/*.h",
  #                   "src/canvas/harmony/*.cc",
  #                   "src/SrLogHarmony.cc",
  #                   "src/SrLogAndroid.cc",
  #                   "src/SVGRenderEngine.cc"
  s.description  = <<-DESC
    ServalSVG provides an SVG parsing and rendering engine for iOS.
    It includes DOM parsing, gradient support, shapes, paths and text rendering,
    with platform-specific canvas implementations and sample projects to help
    integrate into existing apps.
  DESC

  s.pod_target_xcconfig = {
    "GCC_PREPROCESSOR_DEFINITIONS" => "OS_IOS=1",
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'CLANG_CXX_LIBRARY' => 'libc++',
    'CLANG_ENABLE_MODULES' => 'YES',
    'CLANG_ENABLE_OBJC_ARC' => 'YES',
    'GCC_C_LANGUAGE_STANDARD' => 'c11'
  }

  s.xcconfig = {
    "USER_HEADER_SEARCH_PATHS" => "\"$(PODS_TARGET_SRCROOT)/svg/include/**\"",
    "OTHER_CPLUSPLUSFLAGS" => "-std=c++17 -stdlib=libc++"
  }
  s.libraries = 'stdc++', 'c++'

end
