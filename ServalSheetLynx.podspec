Pod::Spec.new do |s|
  s.name = 'ServalSheetLynx'
  s.version = ENV.fetch('POD_VERSION', '0.1.0')
  s.summary = 'Shared Lynx content hosting for native Sheet elements'
  s.homepage = 'https://github.com/lynx-family/serval-kit'
  s.license = { :type => 'Apache 2.0', :file => 'LICENSE' }
  s.preserve_paths = 'sheet/LICENSE.screens'
  s.author = 'Lynx'
  s.source = { :git => 'https://github.com/lynx-family/serval-kit.git', :tag => s.version.to_s }
  s.ios.deployment_target = '15.0'
  s.requires_arc = true
  s.static_framework = true
  s.source_files = 'sheet/lynx/ios/*.{h,m,mm}'
  s.public_header_files = 'sheet/lynx/ios/*.h'
  s.dependency 'ServalSheet', s.version.to_s
  s.dependency 'Lynx'
end
