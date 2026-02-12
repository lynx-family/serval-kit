# Replace with the name of your library
Pod::Spec.new do |s|
  s.name             = 'ServalSVG'
  s.version = begin
    version_path = File.expand_path('SERVAL_KIT_VERSION', __dir__)
    raw_version = ENV['POD_VERSION'] || File.read(version_path).strip
    raw_version
  end
  s.summary          = 'A library for parsing and rendering SVG documents'
  s.homepage         = 'https://github.com/lynx-family/serval-kit'

  s.source = { :git => 'https://github.com/lynx-family/serval-kit.git', }.tap do |source_hash|
    source_hash[:commit] = ENV['SERVAL_KIT_COMMIT_ID']
  end

  s.license          = { :type => 'Apache 2.0' }

  s.author           = 'Lynx'

#   s.platform         = :ios, '12.0'
  s.ios.deployment_target = "9.0"

  s.public_header_files = "include/platform/iOS/SrSVG.h",
                          "include/platform/iOS/SrSVGView.h"

  s.source_files = "include/element/**/*.h",
                   "include/parser/**/*.h",
                   "include/canvas/**/*.h",
                   "include/platform/iOS/**/*.h",
                   "include/utils/**/*.h",
                   "src/element/**/*.{c,cc,cpp}",
                   "src/parser/**/*.{c,cc,cpp}",
                   "platform/iOS/**/*.{mm,m,cc,cpp,c}"

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
    'HEADER_SEARCH_PATHS' => '$(inherited) "$(PODS_TARGET_SRCROOT)/include"',
    'GCC_C_LANGUAGE_STANDARD' => 'c11'
  }

  s.xcconfig = { "OTHER_CPLUSPLUSFLAGS" => "-std=c++17 -stdlib=libc++" }
  s.libraries = 'stdc++', 'c++'

end
