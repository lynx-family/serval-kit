Pod::Spec.new do |s|
  s.name             = 'ServalMarkdown'
  s.version          = begin
    raw_version = ENV['POD_VERSION'] || "0.0.1"
    raw_version
  end
  s.summary          = 'A library for parsing and rendering Markdown documents'
  s.description      = <<-DESC
    ServalMarkdown is an multiplatform library for parsing and rendering Markdown documents. 
    It provides parsing, layout and styling for markdown rendering, with iOS, Android and Harmony platform support.
  DESC
  s.homepage         = 'https://github.com/lynx-family/serval-kit'
  s.source           = { :git => 'https://github.com/lynx-family/serval-kit.git', }.tap do |source_hash|
    source_hash[:commit] = ENV['SERVAL_KIT_COMMIT_ID']
  end
  s.license          = { :type => 'Apache 2.0', :file => "../LICENSE" }
  s.author           = 'Lynx'

  s.ios.deployment_target    = "12.0"
  s.requires_arc             = true
  s.static_framework         = true

  s.dependency 'LynxTextra', '~> 0.1.1-alpha'

  s.public_header_files = "include/markdown/platform/ios/**/*.h"

  s.source_files =  "include/markdown/draw/**/*.h",
                    "include/markdown/element/**/*.h",
                    "include/markdown/layout/**/*.h",
                    "include/markdown/parser/**/*.h",
                    "include/markdown/style/**/*.h",
                    "include/markdown/utils/**/*.h",
                    "include/markdown/view/**/*.h",
                    "include/markdown/platform/ios/**/*.h",
                    "src/markdown/draw/**/*.{h,m,mm,cc}",
                    "src/markdown/element/**/*.{h,m,mm,cc}",
                    "src/markdown/layout/**/*.{h,m,mm,cc}",
                    "src/markdown/parser/**/*.{h,m,mm,cc}",
                    "src/markdown/style/**/*.{h,m,mm,cc}",
                    "src/markdown/utils/**/*.{h,m,mm,cc}",
                    "src/markdown/view/**/*.{h,m,mm,cc}",
                    "src/markdown/platform/ios/**/*.{h,m,mm,cc}",
                    "third_party/discount/discount_lite/*.{h,c}",
                    "third_party/base/src/string/string_utils.cc",
                    "third_party/base/include/string/string_utils.h",
                    "third_party/base/src/string/string_number_convert.cc",
                    "third_party/base/include/string/string_number_convert.h"

  s.pod_target_xcconfig = {
    "GCC_PREPROCESSOR_DEFINITIONS" => "OS_IOS=1",
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'CLANG_CXX_LIBRARY' => 'libc++',
    'CLANG_ENABLE_MODULES' => 'YES',
    'CLANG_ENABLE_OBJC_ARC' => 'YES',
    'GCC_C_LANGUAGE_STANDARD' => 'c11',
    'HEADER_SEARCH_PATHS' => '"$(PODS_TARGET_SRCROOT)" "$(PODS_TARGET_SRCROOT)/include" "$(PODS_TARGET_SRCROOT)/third_party" "$(PODS_TARGET_SRCROOT)/third_party/lynx-textra/public"'
  }

  s.xcconfig = {
    "USER_HEADER_SEARCH_PATHS" => '"$(PODS_TARGET_SRCROOT)/include" "$(PODS_TARGET_SRCROOT)/third_party/base" "$(PODS_TARGET_SRCROOT)/third_party/lynx-textra/public"',
    "OTHER_CPLUSPLUSFLAGS" => "-std=c++17 -stdlib=libc++"
  }
  s.libraries = 'stdc++', 'c++'
end
