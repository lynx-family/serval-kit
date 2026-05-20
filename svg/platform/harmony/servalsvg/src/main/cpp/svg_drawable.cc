// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "svg_drawable.h"
#include "element/SrSVGTypes.h"
#include "parser/SrSVGDOM.h"
#include "platform/harmony/sr_harmony_canvas.h"

#include <iterator>
#include <memory>
#include <multimedia/image_framework/image_pixel_map_mdk.h>
#include <native_drawing/drawing_pixel_map.h>
#include <native_drawing/drawing_types.h>
#include <utility>
#include <vector>

namespace serval {
namespace svg {
namespace harmony {

namespace {

napi_value CreateRenderResult(napi_env env, const SvgRenderResult &result) {
    napi_value js_result;
    napi_create_object(env, &js_result);

    napi_value has_error;
    napi_get_boolean(env, result.has_error, &has_error);
    napi_set_named_property(env, js_result, "hasError", has_error);

    if (!result.error_message.empty()) {
        napi_value error_message;
        napi_create_string_utf8(env, result.error_message.c_str(), NAPI_AUTO_LENGTH, &error_message);
        napi_set_named_property(env, js_result, "errorMessage", error_message);
    }
    return js_result;
}

SvgRenderResult MakeRenderResult(const std::vector<parser::SrSVGDiagnostic> &diagnostics) {
    SvgRenderResult result;
    result.has_error = !diagnostics.empty();
    if (!diagnostics.empty()) {
        result.error_message = diagnostics.front().message;
    }
    return result;
}

}  // namespace

napi_value SvgDrawable::Init(napi_env env, napi_value exports) {
    napi_value cons;
    constexpr napi_property_descriptor properties[] = {
        {"update", nullptr, Update, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"render", nullptr, Render, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setImageLoader", nullptr, SetImageLoader, nullptr, nullptr, nullptr, napi_default, nullptr}};
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
    napi_value js_this;
    size_t argc = 8;
    napi_value argv[8];
    napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
    SvgDrawable *svg;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&svg));
    double left{0};
    double top{0};
    double width{0};
    double height{0};
    double scale{1};
    bool anti_alias{true};
    bool has_color{false};
    std::string color;
    napi_get_value_double(env, argv[0], &width);
    napi_get_value_double(env, argv[1], &height);
    napi_get_value_double(env, argv[2], &left);
    napi_get_value_double(env, argv[3], &top);
    napi_get_value_double(env, argv[4], &scale);
    napi_get_value_bool(env, argv[6], &anti_alias);
    if (argc > 7) {
        napi_valuetype color_type = napi_undefined;
        napi_typeof(env, argv[7], &color_type);
        if (color_type == napi_string) {
            color = SvgDrawable::ConvertToString(env, argv[7]);
            has_color = true;
        }
    }
    const auto &str = SvgDrawable::ConvertToString(env, argv[5]);
    const auto result =
        svg->Update(str, left * scale, top * scale, width * scale, height * scale, anti_alias, has_color, color);
    return CreateRenderResult(env, result);
}

napi_value SvgDrawable::Constructor(napi_env env, napi_callback_info info) {
    size_t argc = 0;
    napi_value js_this;
    napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);

    SvgDrawable *obj = new SvgDrawable();
    obj->env_ = env;

    napi_wrap(
        env, js_this, obj,
        [](napi_env env, void *data, void *hint) {
            auto svg = static_cast<SvgDrawable *>(data);
            if (svg != nullptr) {
                svg->ClearImageCache();
                if (svg->image_loader_ref_ != nullptr) {
                    napi_delete_reference(env, svg->image_loader_ref_);
                    svg->image_loader_ref_ = nullptr;
                }
                delete svg;
            }
        },
        nullptr, nullptr);

    return js_this;
}

napi_value SvgDrawable::SetImageLoader(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_value js_this;
    napi_value argv[1];
    size_t argc = 1;
    napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);

    SvgDrawable *svg;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&svg));
    if (svg->image_loader_ref_ != nullptr) {
        napi_delete_reference(env, svg->image_loader_ref_);
        svg->image_loader_ref_ = nullptr;
    }
    svg->ClearImageCache();
    if (argc > 0) {
        napi_valuetype loader_type = napi_undefined;
        napi_typeof(env, argv[0], &loader_type);
        if (loader_type == napi_function) {
            napi_create_reference(env, argv[0], 1, &svg->image_loader_ref_);
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

void SvgDrawable::ClearImageCache() {
    if (env_ == nullptr) {
        image_cache_.clear();
        return;
    }
    for (auto &entry : image_cache_) {
        if (entry.second.image_data.draw_pixel_map != nullptr) {
            OH_Drawing_PixelMapDissolve(entry.second.image_data.draw_pixel_map);
            entry.second.image_data.draw_pixel_map = nullptr;
        }
        if (entry.second.pixel_map_ref != nullptr) {
            napi_delete_reference(env_, entry.second.pixel_map_ref);
            entry.second.pixel_map_ref = nullptr;
        }
    }
    image_cache_.clear();
}

bool SvgDrawable::CacheImage(const std::string &url, napi_value pixel_map_value) {
    if (env_ == nullptr) {
        return false;
    }

    napi_ref pixel_map_ref = nullptr;
    if (napi_create_reference(env_, pixel_map_value, 1, &pixel_map_ref) != napi_ok) {
        return false;
    }

    NativePixelMap *native_pixel_map = OH_PixelMap_InitNativePixelMap(env_, pixel_map_value);
    if (native_pixel_map == nullptr) {
        napi_delete_reference(env_, pixel_map_ref);
        return false;
    }

    OhosPixelMapInfos image_info{};
    if (OH_PixelMap_GetImageInfo(native_pixel_map, &image_info) != 0) {
        napi_delete_reference(env_, pixel_map_ref);
        return false;
    }

    OH_Drawing_PixelMap *draw_pixel_map = OH_Drawing_PixelMapGetFromNativePixelMap(native_pixel_map);
    if (draw_pixel_map == nullptr) {
        napi_delete_reference(env_, pixel_map_ref);
        return false;
    }

    CachedImage cached_image;
    cached_image.pixel_map_ref = pixel_map_ref;
    cached_image.native_pixel_map = native_pixel_map;
    cached_image.image_data.draw_pixel_map = draw_pixel_map;
    cached_image.image_data.width = image_info.width;
    cached_image.image_data.height = image_info.height;
    image_cache_[url] = cached_image;
    return true;
}

const SvgDrawable::CachedImage *SvgDrawable::RequestImage(const std::string &url) {
    auto cached = image_cache_.find(url);
    if (cached != image_cache_.end()) {
        return &cached->second;
    }
    if (env_ == nullptr || image_loader_ref_ == nullptr) {
        return nullptr;
    }

    napi_value loader;
    napi_value global;
    napi_value argv[1];
    napi_value js_result;
    napi_get_reference_value(env_, image_loader_ref_, &loader);
    napi_get_global(env_, &global);
    napi_create_string_utf8(env_, url.c_str(), NAPI_AUTO_LENGTH, &argv[0]);
    if (napi_call_function(env_, global, loader, 1, argv, &js_result) != napi_ok) {
        return nullptr;
    }

    napi_valuetype result_type = napi_undefined;
    napi_typeof(env_, js_result, &result_type);
    if (result_type == napi_undefined || result_type == napi_null) {
        return nullptr;
    }
    if (!CacheImage(url, js_result)) {
        return nullptr;
    }
    return &image_cache_[url];
}

void SvgDrawable::Render(OH_Drawing_Canvas *canvas) {
    if (svg_dom_) {
        if (!sr_canvas_) {
            sr_canvas_ = std::make_unique<SrHarmonyCanvas>(canvas);
        } else {
            sr_canvas_->Reset(canvas);
        }
        sr_canvas_->SetAntiAlias(anti_alias_);
        sr_canvas_->SetImageProvider([this](const std::string &url) -> const SrHarmonyCanvas::ImageData * {
            const auto *image = this->RequestImage(url);
            return image == nullptr ? nullptr : &image->image_data;
        });
        if (has_color_) {
            uint32_t default_color = 0;
            if (parse_svg_color(color_.c_str(), &default_color)) {
                svg_dom_->SetDefaultColor(default_color);
            } else {
                svg_dom_->ResetDefaultColor();
            }
        } else {
            svg_dom_->ResetDefaultColor();
        }
        SrSVGBox box{left_, top_, width_, height_};
        svg_dom_->Render(sr_canvas_.get(), box);
        last_result_ = MakeRenderResult(svg_dom_->diagnostics());
    }
}

SvgRenderResult harmony::SvgDrawable::Update(const std::string &content, float left, float top, float width,
                                             float height, bool anti_alias, bool has_color, std::string color) {
    std::vector<parser::SrSVGDiagnostic> diagnostics;
    svg_dom_ = std::move(parser::SrSVGDOM::make(content.data(), content.size(), &diagnostics));
    left_ = left;
    top_ = top;
    width_ = width;
    height_ = height;
    anti_alias_ = anti_alias;
    has_color_ = has_color;
    color_ = std::move(color);
    last_result_ = MakeRenderResult(diagnostics);
    return last_result_;
}
}  // namespace harmony
}  // namespace svg
}  // namespace serval
