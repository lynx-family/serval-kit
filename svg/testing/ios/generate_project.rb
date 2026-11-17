# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

require 'fileutils'
require 'json'
require 'xcodeproj'

root = File.expand_path('../../..', __dir__)
build_dir = File.expand_path(ARGV.fetch(0))
manifest = JSON.parse(File.read(File.join(root, 'svg/testing/pattern_percent_cases.json')))
raise 'Unsupported pattern reference schema' unless manifest.fetch('schema_version') == 1
if ENV['SERVAL_IOS_PATTERN_CASE_FILTER']
  pattern = Regexp.new(ENV.fetch('SERVAL_IOS_PATTERN_CASE_FILTER'))
  manifest['cases'] = manifest.fetch('cases').select { |test_case| pattern.match?(test_case.fetch('file')) }
  raise 'No pattern fixtures matched SERVAL_IOS_PATTERN_CASE_FILTER' if manifest['cases'].empty?
end
manifest.fetch('cases').each do |test_case|
  test_case.fetch('samples').each do |sample|
    rgba = sample.fetch('rgba')
    unless rgba.is_a?(Array) && rgba.length == 4 && rgba.all? { |value| value.is_a?(Numeric) }
      raise "Missing Chrome RGBA reference for #{test_case.fetch('file')}"
    end
  end
end

FileUtils.mkdir_p(build_dir)
manifest_path = File.join(build_dir, 'pattern_percent_cases.json')
File.write(manifest_path, JSON.pretty_generate(manifest) + "\n")
project_path = File.join(build_dir, 'PatternTests.xcodeproj')
project = Xcodeproj::Project.new(project_path)
target = project.new_target(:unit_test_bundle, 'PatternTests', :ios, '16.4')
target.build_configurations.each do |configuration|
  configuration.build_settings['PRODUCT_BUNDLE_IDENTIFIER'] = 'org.lynx.serval.PatternTests'
  configuration.build_settings['GENERATE_INFOPLIST_FILE'] = 'YES'
  configuration.build_settings['CODE_SIGNING_ALLOWED'] = 'NO'
  configuration.build_settings['CLANG_ENABLE_OBJC_ARC'] = 'YES'
  configuration.build_settings['HEADER_SEARCH_PATHS'] = [
    '$(inherited)', File.join(root, 'svg/examples/iOS/ServalPark/ServalPark')
  ]
end
sources = [
  File.join(__dir__, 'PatternImageTests.m'),
  File.join(__dir__, 'ImageLoaderTests.m'),
  File.join(root, 'svg/examples/iOS/ServalPark/ServalPark/SvgImageLoader.m'),
  File.join(root, 'svg/examples/iOS/ServalPark/ServalPark/ViewController.m')
]
sources.each do |path|
  target.add_file_references([project.main_group.new_file(path)])
end

resources = [manifest_path] + manifest.fetch('cases').map do |test_case|
  File.join(root, 'svg/test_cases', test_case.fetch('file'))
end
resources.each do |path|
  raise "Missing fixture #{path}" unless File.file?(path)
  reference = project.main_group.new_file(path)
  target.resources_build_phase.add_file_reference(reference)
end
project.save

scheme = Xcodeproj::XCScheme.new
scheme.add_build_target(target)
scheme.add_test_target(target)
scheme.test_action.build_configuration = 'Debug'
scheme.save_as(project_path, 'PatternTests', true)

File.write(File.join(build_dir, 'Podfile'), <<~POD)
  project './PatternTests.xcodeproj'
  platform :ios, '16.4'
  target 'PatternTests' do
    pod 'ServalSVG', :path => #{root.dump}
  end
POD
