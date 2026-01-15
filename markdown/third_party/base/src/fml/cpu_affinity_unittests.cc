// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/cpu_affinity.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace fml {
namespace testing {

TEST(CpuAffinity, NonAndroidPlatformDefaults) {
  ASSERT_FALSE(fml::EfficiencyCoreCount().has_value());
  ASSERT_TRUE(fml::RequestAffinity(fml::CpuAffinity::kEfficiency));
}

TEST(CpuAffinity, NormalSlowMedFastCores) {
  auto speeds = {CpuIndexAndSpeed{.index = 0, .speed = 1},
                 CpuIndexAndSpeed{.index = 1, .speed = 2},
                 CpuIndexAndSpeed{.index = 2, .speed = 3}};
  auto tracker = CPUSpeedTracker(speeds);

  ASSERT_TRUE(tracker.IsValid());
  ASSERT_EQ(tracker.GetIndices(CpuAffinity::kEfficiency)[0], 0u);
  ASSERT_EQ(tracker.GetIndices(CpuAffinity::kPerformance)[0], 2u);
  ASSERT_EQ(tracker.GetIndices(CpuAffinity::kNotPerformance).size(), 2u);
  ASSERT_EQ(tracker.GetIndices(CpuAffinity::kNotPerformance)[0], 0u);
  ASSERT_EQ(tracker.GetIndices(CpuAffinity::kNotPerformance)[1], 1u);
  ASSERT_EQ(tracker.GetIndices(CpuAffinity::kNotEfficiency).size(), 2u);
  ASSERT_EQ(tracker.GetIndices(CpuAffinity::kNotEfficiency)[0], 1u);
  ASSERT_EQ(tracker.GetIndices(CpuAffinity::kNotEfficiency)[1], 2u);
}

TEST(CpuAffinity, NoCpuData) {
  auto tracker = CPUSpeedTracker({});

  ASSERT_FALSE(tracker.IsValid());
}

TEST(CpuAffinity, AllSameSpeed) {
  auto speeds = {CpuIndexAndSpeed{.index = 0, .speed = 1},
                 CpuIndexAndSpeed{.index = 1, .speed = 1},
                 CpuIndexAndSpeed{.index = 2, .speed = 1}};
  auto tracker = CPUSpeedTracker(speeds);

  ASSERT_FALSE(tracker.IsValid());
}

TEST(CpuAffinity, SingleCore) {
  auto speeds = {CpuIndexAndSpeed{.index = 0, .speed = 1}};
  auto tracker = CPUSpeedTracker(speeds);

  ASSERT_FALSE(tracker.IsValid());
}

TEST(CpuAffinity, MissingFileParsing) {
  auto result = ReadIntFromFile("/does_not_exist");
  ASSERT_FALSE(result.has_value());
}

}  // namespace testing
}  // namespace fml
}  // namespace lynx
