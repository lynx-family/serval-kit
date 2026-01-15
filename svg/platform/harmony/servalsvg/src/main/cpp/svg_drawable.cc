// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "svg_drawable.h"
#include "parser/SrSVGDOM.h"
#include "platform/harmony/sr_harmony_canvas.h"

#include <iterator>
#include <memory>
#include <native_drawing/drawing_types.h>
#include <utility>

namespace serval {
namespace svg {
namespace harmony {

napi_value SvgDrawable::Init(napi_env env, napi_value exports) {
    napi_value cons;
    constexpr napi_property_descriptor properties[] = {
        {"update", nullptr, Update, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"render", nullptr, Render, nullptr, nullptr, nullptr, napi_default, nullptr}};
    constexpr size_t prop_size = std::size(properties);
    napi_define_class(env, "SvgDrawable", NAPI_AUTO_LENGTH, Constructor, nullptr, prop_size, properties, &cons);
    napi_set_named_property(env, exports, "SvgDrawable", cons);
    return exports;
}

napi_value SvgDrawable::Render(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_value js_this;
    napi_value argv[1];
    size_t argc = 1;
    napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
    OH_Drawing_Canvas *canvas;
    napi_unwrap(env, argv[0], reinterpret_cast<void **>(&canvas));
    SvgDrawable *svg;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&svg));
    svg->Render(canvas);
    napi_get_undefined(env, &result);
    return result;
}

std::string SvgDrawable::ConvertToString(napi_env env, napi_value arg) {
    size_t str_size;
    napi_status status = napi_get_value_string_utf8(env, arg, nullptr, 0, &str_size);
    if (status != napi_ok) {
        return "";
    }
    auto buf = std::make_unique<char[]>(str_size + 1);
    status = napi_get_value_string_utf8(env, arg, buf.get(), str_size + 1, &str_size);
    if (status != napi_ok) {
        return "";
    }
    return std::string(buf.get(), str_size);
}

napi_value SvgDrawable::Update(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_get_undefined(env, &result);
    napi_value js_this;
    size_t argc = 7;
    napi_value argv[7];
    napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
    SvgDrawable *svg;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&svg));
    double left{0};
    double top{0};
    double width{0};
    double height{0};
    double scale{1};
    bool anti_alias{true};
    napi_get_value_double(env, argv[0], &width);
    napi_get_value_double(env, argv[1], &height);
    napi_get_value_double(env, argv[2], &left);
    napi_get_value_double(env, argv[3], &top);
    napi_get_value_double(env, argv[4], &scale);
    napi_get_value_bool(env, argv[6], &anti_alias);
    const auto &str = SvgDrawable::ConvertToString(env, argv[5]);
    svg->Update(str, left * scale, top * scale, width * scale, height * scale, anti_alias);
    return result;
}

napi_value SvgDrawable::Constructor(napi_env env, napi_callback_info info) {
    size_t argc = 0;
    napi_value js_this;
    napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);

    SvgDrawable *obj = new SvgDrawable();

    napi_wrap(
        env, js_this, obj,
        [](napi_env env, void *data, void *hint) {
            auto svg = static_cast<SvgDrawable *>(data);
            if (svg != nullptr) {
                delete svg;
            }
        },
        nullptr, nullptr);

    return js_this;
}

void SvgDrawable::Render(OH_Drawing_Canvas *canvas) {
    if (svg_dom_) {
        if (!sr_canvas_) {
            sr_canvas_ = std::make_unique<SrHarmonyCanvas>(canvas);
        } else {
            sr_canvas_->Reset(canvas);
        }
        sr_canvas_->SetAntiAlias(anti_alias_);
        SrSVGBox box{left_, top_, width_, height_};
        svg_dom_->Render(sr_canvas_.get(), box);
    }
}

void harmony::SvgDrawable::Update(const std::string &content, float left, float top, float width, float height,
                                  bool anti_alias) {
    svg_dom_ = std::move(parser::SrSVGDOM::make(content.data(), content.size()));
    left_ = left;
    top_ = top;
    width_ = width;
    height_ = height;
    anti_alias_ = anti_alias;
}
} // namespace harmony
} // namespace svg
} // namespace serval
