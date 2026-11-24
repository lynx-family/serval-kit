// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <node_api.h>

int RunHarmonyTextTests();
const char* HarmonyTextTestReport();

static napi_value Run(napi_env env, napi_callback_info info) {
  napi_value result;
  RunHarmonyTextTests();
  napi_create_string_utf8(env, HarmonyTextTestReport(), NAPI_AUTO_LENGTH,
                          &result);
  return result;
}

static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor descriptor{"run",   nullptr, Run,          nullptr,
                                      nullptr, nullptr, napi_default, nullptr};
  napi_define_properties(env, exports, 1, &descriptor);
  return exports;
}

static napi_module module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "text_tests",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterTextTests() {
  napi_module_register(&module);
}
