# Replace with the name of your library
Pod::Spec.new do |s|
  s.name             = 'ServalSVG'
  s.version          = '0.0.16'
  s.summary          = 'A library for parsing and rendering SVG documents'
  s.homepage         = 'https://github.com/lynx-family/serval-kit'

  # Replace with the path to your library source directory
  s.source                = { :git => 'git@github.com:lynx-family/serval-kit.git', :tag => s.version }

  # Replace with the path to your library header files
#   s.ios.public_header_files = 'path/to/headers/**/*.h'

  s.license          = 'MIT'

  s.author           = { 'thendlessky' => 'fncdong@gmail.com' }

#   s.platform         = :ios, '12.0'
  s.ios.deployment_target = "9.0"

  s.public_header_files = "include/platform/iOS/SrSVG.h",
                          "include/platform/iOS/SrSVGView.h"

  s.private_header_files = # element
                           "include/element/SrSVGCircle.h",
                           "include/element/SrSVGClipPath.h",
                           "include/element/SrSVGContainer.h",
                           "include/element/SrSVGDefs.h",
                           "include/element/SrSVGEllipse.h",
                           "include/element/SrSVGG.h",
                           "include/element/SrSVGImage.h",
                           "include/element/SrSVGLine.h",
                           "include/element/SrSVGLinearGradient.h",
                           "include/element/SrSVGNode.h",
                           "include/element/SrSVGPath.h",
                           "include/element/SrSVGPolyLine.h",
                           "include/element/SrSVGPolygon.h",
                           "include/element/SrSVGRadialGradient.h",
                           "include/element/SrSVGRect.h",
                           "include/element/SrSVGSVG.h",
                           "include/element/SrSVGShape.h",
                           "include/element/SrSVGStop.h",
                           "include/element/SrSVGText.h",
                           "include/element/SrSVGTypes.h",
                           "include/element/SrSVGUse.h",
                           # parser
                           "include/parser/SrDOM.h",
                           "include/parser/SrDOMParser.h",
                           "include/parser/SrSVGDOM.h",
                           "include/parser/SrXMLExtractor.h",
                           "include/parser/SrXMLParser.h",
                           "include/parser/SrXMLParserError.h",
                           # canvas
                           "include/canvas/SrCanvas.h",
                           "include/canvas/SrParagraph.h",
                           "include/platform/iOS/SrIOSCanvas.h",
                           "include/platform/iOS/SrIOSParagraph.h",
                           # include
                           "include/utils/SrFloatComparison.h",
                           "include/utils/SrSVGLog.h"

  s.source_files =  # element
                    "include/element/SrSVGCircle.h",
                    "include/element/SrSVGClipPath.h",
                    "include/element/SrSVGContainer.h",
                    "include/element/SrSVGDefs.h",
                    "include/element/SrSVGEllipse.h",
                    "include/element/SrSVGG.h",
                    "include/element/SrSVGImage.h",
                    "include/element/SrSVGLine.h",
                    "include/element/SrSVGLinearGradient.h",
                    "include/element/SrSVGNode.h",
                    "include/element/SrSVGPath.h",
                    "include/element/SrSVGPolyLine.h",
                    "include/element/SrSVGPolygon.h",
                    "include/element/SrSVGRadialGradient.h",
                    "include/element/SrSVGRect.h",
                    "include/element/SrSVGSVG.h",
                    "include/element/SrSVGShape.h",
                    "include/element/SrSVGStop.h",
                    "include/element/SrSVGTypes.h",
                    "include/element/SrSVGUse.h",
                    "include/element/SrSVGText.h",
                    "src/element/SrSVGCircle.cc",
                    "src/element/SrSVGClipPath.cc",
                    "src/element/SrSVGContainer.cc",
                    "src/element/SrSVGDefs.cc",
                    "src/element/SrSVGEllipse.cc",
                    "src/element/SrSVGImage.cc",
                    "src/element/SrSVGLine.cc",
                    "src/element/SrSVGLinearGradient.cc",
                    "src/element/SrSVGNode.cc",
                    "src/element/SrSVGPath.cc",
                    "src/element/SrSVGPolyLine.cc",
                    "src/element/SrSVGPolygon.cc",
                    "src/element/SrSVGRadialGradient.cc",
                    "src/element/SrSVGRect.cc",
                    "src/element/SrSVGSVG.cc",
                    "src/element/SrSVGShape.cc",
                    "src/element/SrSVGStop.cc",
                    "src/element/SrSVGText.cc",
                    "src/element/SrSVGTypes.c",
                    "src/element/SrSVGUse.cc",
                    # parser
                    "include/parser/SrDOM.h",
                    "include/parser/SrDOMParser.h",
                    "include/parser/SrSVGDOM.h",
                    "include/parser/SrXMLExtractor.h",
                    "include/parser/SrXMLParser.h",
                    "include/parser/SrXMLParserError.h",
                    "src/parser/SrDOM.cc",
                    "src/parser/SrDOMParser.cc",
                    "src/parser/SrSVGDOM.cc",
                    "src/parser/SrXMLExtractor.c",
                    "src/parser/SrXMLParser.cc",
                    "src/parser/SrXMLParserError.cc",
                    # canvas
                    "include/platform/iOS/SrSVG.h",
                    "include/platform/iOS/SrSVGView.h",
                    "include/platform/iOS/SrIOSCanvas.h",
                    "include/platform/iOS/SrIOSParagraph.h",
                    "include/canvas/SrCanvas.h",
                    "include/canvas/SrParagraph.h",
                    "platform/iOS/SrIOSCanvas.mm",
                    "platform/iOS/SrSVG.mm",
                    "platform/iOS/SrSVGView.mm",
                    "platform/iOS/SrIOSParagraph.mm",
                    # cpp
                    "include/utils/SrFloatComparison.h",
                    "include/utils/SrSVGLog.h",
                    "platform/iOS/SrLogDarwin.mm"

  # s.exclude_files = "src/canvas/android/*.h",
  #                   "src/canvas/android/*.cc",
  #                   "src/canvas/harmony/*.h",
  #                   "src/canvas/harmony/*.cc",
  #                   "src/SrLogHarmony.cc",
  #                   "src/SrLogAndroid.cc",
  #                   "src/SVGRenderEngine.cc"

  s.description  = <<-DESC
          SVG parser and render
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
    "USER_HEADER_SEARCH_PATHS" => "\"$(PODS_TARGET_SRCROOT)/include\"",
    "OTHER_CPLUSPLUSFLAGS" => "-std=c++17 -stdlib=libc++"
  }
  s.libraries = 'stdc++', 'c++'

end
