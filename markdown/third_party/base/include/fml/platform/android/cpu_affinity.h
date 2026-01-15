// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_THIRD_PARTY_BASE_INCLUDE_FML_PLATFORM_ANDROID_CPU_AFFINITY_H_
#define MARKDOWN_THIRD_PARTY_BASE_INCLUDE_FML_PLATFORM_ANDROID_CPU_AFFINITY_H_

#include "base/include/fml/cpu_affinity.h"

namespace lynx {
namespace fml {

/// @brief Android specific implementation of EfficiencyCoreCount.
std::optional<size_t> AndroidEfficiencyCoreCount();

/// @brief Android specific implementation of RequestAffinity.
bool AndroidRequestAffinity(CpuAffinity affinity);

}  // namespace fml
}  // namespace lynx

#endif  // MARKDOWN_THIRD_PARTY_BASE_INCLUDE_FML_PLATFORM_ANDROID_CPU_AFFINITY_H_
