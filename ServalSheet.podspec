Pod::Spec.new do |s|
  s.name = 'ServalSheet'
  s.version = ENV.fetch('POD_VERSION', '0.1.0')
  s.summary = 'Shared native Sheet presentation and interaction'
  s.homepage = 'https://github.com/lynx-family/serval-kit'
  s.license = { :type => 'Apache 2.0', :file => 'LICENSE' }
  s.preserve_paths = 'sheet/LICENSE.screens'
  s.author = 'Lynx'
  s.source = { :git => 'https://github.com/lynx-family/serval-kit.git', :tag => s.version.to_s }
  s.ios.deployment_target = '15.0'
  s.requires_arc = true
  s.static_framework = true
  s.source_files = 'sheet/platform/ios/*.{h,m,mm}'
  s.public_header_files = 'sheet/platform/ios/*.h'
  s.frameworks = 'UIKit', 'QuartzCore'
  s.pod_target_xcconfig = { 'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17' }
end
