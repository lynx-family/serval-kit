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
  s.license          = { :type => 'Apache 2.0', :file => "LICENSE" }
  s.author           = 'Lynx'

  s.ios.deployment_target    = "12.0"
  s.requires_arc             = true
  s.static_framework         = true

  s.public_header_files = "markdown/include/markdown/platform/ios/*.h"
  s.private_header_files = "markdown/include/markdown/platform/ios/internal/**/*.h"

  s.dependency 'LynxTextra'

  s.source_files =  "markdown/include/markdown/*.h",
                    "markdown/include/markdown/draw/**/*.h",
                    "markdown/include/markdown/element/**/*.h",
                    "markdown/include/markdown/layout/**/*.h",
                    "markdown/include/markdown/parser/**/*.h",
                    "markdown/include/markdown/style/**/*.h",
                    "markdown/include/markdown/utils/**/*.h",
                    "markdown/include/markdown/view/**/*.h",
                    "markdown/include/markdown/platform/ios/**/*.h",
                    "markdown/src/markdown/draw/**/*.{h,m,mm,cc}",
                    "markdown/src/markdown/element/**/*.{h,m,mm,cc}",
                    "markdown/src/markdown/layout/**/*.{h,m,mm,cc}",
                    "markdown/src/markdown/parser/**/*.{h,m,mm,cc}",
                    "markdown/src/markdown/style/**/*.{h,m,mm,cc}",
                    "markdown/src/markdown/utils/**/*.{h,m,mm,cc}",
                    "markdown/src/markdown/view/**/*.{h,m,mm,cc}",
                    "markdown/src/markdown/platform/ios/**/*.{h,m,mm,cc}",
                    "markdown/third_party/discount/discount_lite/*.{h,c}"

  s.pod_target_xcconfig = {
    "GCC_PREPROCESSOR_DEFINITIONS" => "OS_IOS=1",
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'CLANG_CXX_LIBRARY' => 'libc++',
    'CLANG_ENABLE_MODULES' => 'YES',
    'CLANG_ENABLE_OBJC_ARC' => 'YES',
    'GCC_C_LANGUAGE_STANDARD' => 'c11',
    'HEADER_SEARCH_PATHS' => '$(inherited) "$(PODS_TARGET_SRCROOT)" "$(PODS_TARGET_SRCROOT)/markdown/include" "$(PODS_TARGET_SRCROOT)/markdown/third_party" "$(PODS_ROOT)/Headers/Public" "$(PODS_ROOT)/Headers/Public/LynxTextra" "$(PODS_XCFRAMEWORKS_BUILD_DIR)/LynxTextra/LynxTextra.framework/Headers"'
  }

  s.xcconfig = {
    "USER_HEADER_SEARCH_PATHS" => '$(inherited) "$(PODS_TARGET_SRCROOT)/markdown/include" "$(PODS_ROOT)/Headers/Public" "$(PODS_ROOT)/Headers/Public/LynxTextra" "$(PODS_XCFRAMEWORKS_BUILD_DIR)/LynxTextra/LynxTextra.framework/Headers"',
    "OTHER_CPLUSPLUSFLAGS" => "-std=c++17 -stdlib=libc++"
  }
  s.libraries = 'stdc++', 'c++'
end
