// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "svg_drawable.h"
#include <mutex>
#include <node_api.h>

EXTERN_C_START static napi_value Init(napi_env env, napi_value exports) {
    static std::once_flag once_flag;
    std::call_once(once_flag, [&env, &exports]() { serval::svg::harmony::SvgDrawable::Init(env, exports); });
    return exports;
}

EXTERN_C_END

static napi_module serval_svg_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "servalsvg",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) { napi_module_register(&serval_svg_module); }
