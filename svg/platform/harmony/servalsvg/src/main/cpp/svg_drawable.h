// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_PLATFORM_HARMONY_SERVALSVG_SRC_MAIN_CPP_SVG_DRAWABLE_H_
#define SVG_PLATFORM_HARMONY_SERVALSVG_SRC_MAIN_CPP_SVG_DRAWABLE_H_

#include "parser/SrSVGDOM.h"
#include "platform/harmony/sr_harmony_canvas.h"

#include <memory>
#include <native_drawing/drawing_types.h>
#include <node_api.h>
#include <string>
#include <sys/stat.h>


namespace serval {
namespace svg {
namespace harmony {
class SvgDrawable {

public:
    static napi_value Init(napi_env env, napi_value exports);

    void Render(OH_Drawing_Canvas *canvas);

    void Update(const std::string &content, float left, float top, float width, float height, bool anti_alias);


private:
    // JS methods
    static napi_value Update(napi_env env, napi_callback_info info);
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static napi_value Render(napi_env env, napi_callback_info info);

    static std::string ConvertToString(napi_env env, napi_value arg);

    float left_{0.f};
    float top_{0.f};
    float width_{0.f};
    float height_{0.f};
    bool anti_alias_{true};
    std::unique_ptr<SrHarmonyCanvas> sr_canvas_{nullptr};
    std::unique_ptr<parser::SrSVGDOM> svg_dom_{nullptr};
};
} // namespace harmony
} // namespace svg

} // namespace serval


#endif // SVG_PLATFORM_HARMONY_SERVALSVG_SRC_MAIN_CPP_SVG_DRAWABLE_H_
