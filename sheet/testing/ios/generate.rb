#!/usr/bin/env ruby
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

require 'xcodeproj'
require 'fileutils'

root = File.expand_path('../..', __dir__)
output = File.join(root, 'build', 'ios-tests')
FileUtils.mkdir_p(output)
project = Xcodeproj::Project.new(File.join(output, 'SheetTests.xcodeproj'))
app = project.new_target(:application, 'SheetTestHost', :ios, '15.1')
tests = project.new_target(:unit_test_bundle, 'SheetTests', :ios, '15.1')
tests.add_dependency(app)
app.add_file_references([project.main_group.new_file(File.join(__dir__, 'main.m'))])
sources = Dir[File.join(root, 'platform/ios/*.{m,mm}')] + [File.join(__dir__, 'SheetSessionTests.m')]
tests.add_file_references(sources.map { |path| project.main_group.new_file(path) })
[app, tests].each do |target|
  target.build_configurations.each do |config|
    config.build_settings['PRODUCT_BUNDLE_IDENTIFIER'] = "org.lynxsdk.#{target.name}"
    config.build_settings['GENERATE_INFOPLIST_FILE'] = 'YES'
    config.build_settings['CLANG_ENABLE_OBJC_ARC'] = 'YES'
    config.build_settings['CLANG_CXX_LANGUAGE_STANDARD'] = 'c++17'
    config.build_settings['CODE_SIGNING_ALLOWED'] = 'NO'
    config.build_settings['HEADER_SEARCH_PATHS'] = File.join(root, 'platform/ios')
    config.build_settings['TARGETED_DEVICE_FAMILY'] = '1,2'
  end
end
tests.build_configurations.each do |config|
  config.build_settings['TEST_HOST'] = '$(BUILT_PRODUCTS_DIR)/SheetTestHost.app/SheetTestHost'
  config.build_settings['BUNDLE_LOADER'] = '$(TEST_HOST)'
end
project.save
scheme = Xcodeproj::XCScheme.new
scheme.add_build_target(app)
scheme.add_build_target(tests)
scheme.add_test_target(tests)
scheme.set_launch_target(app)
scheme.save_as(project.path, 'SheetTests')
puts project.path
