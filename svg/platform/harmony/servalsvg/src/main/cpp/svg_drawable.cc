// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "svg_drawable.h"
#include "element/SrSVGTypes.h"
#include "parser/SrSVGDOM.h"
#include "platform/harmony/sr_harmony_canvas.h"

#include <cstdint>
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

void ReleasePixelMapValue(napi_env env, napi_value pixel_map_value) {
    if (env == nullptr || pixel_map_value == nullptr) {
        return;
    }
    napi_value release = nullptr;
    napi_valuetype release_type = napi_undefined;
    if (napi_get_named_property(env, pixel_map_value, "release", &release) != napi_ok) {
        return;
    }
    if (napi_typeof(env, release, &release_type) != napi_ok || release_type != napi_function) {
        return;
    }
    napi_value result = nullptr;
    napi_call_function(env, pixel_map_value, release, 0, nullptr, &result);
}

void ReleasePixelMapReference(napi_env env, napi_ref &pixel_map_ref, bool release_pixel_map) {
    if (env == nullptr || pixel_map_ref == nullptr) {
        pixel_map_ref = nullptr;
        return;
    }
    if (release_pixel_map) {
        napi_value pixel_map_value = nullptr;
        if (napi_get_reference_value(env, pixel_map_ref, &pixel_map_value) == napi_ok) {
            ReleasePixelMapValue(env, pixel_map_value);
        }
    }
    napi_delete_reference(env, pixel_map_ref);
    pixel_map_ref = nullptr;
}

void ReleaseCachedImage(napi_env env, SvgDrawable::CachedImage &cached_image, bool release_pixel_map) {
    if (cached_image.image_data.draw_pixel_map != nullptr) {
        OH_Drawing_PixelMapDissolve(cached_image.image_data.draw_pixel_map);
        cached_image.image_data.draw_pixel_map = nullptr;
    }
    ReleasePixelMapReference(env, cached_image.pixel_map_ref, release_pixel_map);
    cached_image.state = ImageLoadState::kIdle;
    cached_image.image_data.width = 0;
    cached_image.image_data.height = 0;
}

}  // namespace

napi_value SvgDrawable::Init(napi_env env, napi_value exports) {
    napi_value cons;
    constexpr napi_property_descriptor properties[] = {
        {"update", nullptr, Update, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"render", nullptr, Render, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"hasAnimations", nullptr, HasAnimations, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"animationTimelineEndSeconds", nullptr, AnimationTimelineEndSeconds, nullptr, nullptr, nullptr, napi_default,
         nullptr},
        {"startAnimation", nullptr, StartAnimation, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"stopAnimation", nullptr, StopAnimation, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"resetAnimationClock", nullptr, ResetAnimationClock, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"needsAnimationFrame", nullptr, NeedsAnimationFrame, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onFrameTimeNanos", nullptr, OnFrameTimeNanos, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"currentAnimationSeconds", nullptr, CurrentAnimationSeconds, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"dispose", nullptr, Dispose, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setImageFetcher", nullptr, SetImageFetcher, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setInvalidateCallback", nullptr, SetInvalidateCallback, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"retryFailedImages", nullptr, RetryFailedImages, nullptr, nullptr, nullptr, napi_default, nullptr}};
    constexpr size_t prop_size = std::size(properties);
    napi_define_class(env, "SvgDrawable", NAPI_AUTO_LENGTH, Constructor, nullptr, prop_size, properties, &cons);
    napi_set_named_property(env, exports, "SvgDrawable", cons);
    return exports;
}

napi_value SvgDrawable::Render(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_value js_this;
    napi_value argv[2];
    size_t argc = 2;
    napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
    OH_Drawing_Canvas *canvas;
    napi_unwrap(env, argv[0], reinterpret_cast<void **>(&canvas));
    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    double seconds{0};
    if (argc > 1) {
        napi_get_value_double(env, argv[1], &seconds);
        drawable->RenderAtTime(canvas, seconds);
    } else {
        drawable->Render(canvas);
    }
    napi_get_undefined(env, &result);
    return result;
}

napi_value SvgDrawable::HasAnimations(napi_env env, napi_callback_info info) {
    napi_value js_this;
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    napi_value result;
    napi_get_boolean(env, drawable != nullptr && drawable->HasAnimations(), &result);
    return result;
}

napi_value SvgDrawable::AnimationTimelineEndSeconds(napi_env env, napi_callback_info info) {
    napi_value js_this;
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    napi_value result;
    napi_create_double(env, drawable != nullptr ? drawable->AnimationTimelineEndSeconds() : 0.0, &result);
    return result;
}

napi_value SvgDrawable::StartAnimation(napi_env env, napi_callback_info info) {
    napi_value js_this;
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    if (drawable != nullptr) {
        drawable->StartAnimation();
    }
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

napi_value SvgDrawable::StopAnimation(napi_env env, napi_callback_info info) {
    napi_value js_this;
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    if (drawable != nullptr) {
        drawable->StopAnimation();
    }
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

napi_value SvgDrawable::ResetAnimationClock(napi_env env, napi_callback_info info) {
    napi_value js_this;
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    if (drawable != nullptr) {
        drawable->ResetAnimationClock();
    }
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

napi_value SvgDrawable::NeedsAnimationFrame(napi_env env, napi_callback_info info) {
    napi_value js_this;
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    napi_value result;
    napi_get_boolean(env, drawable != nullptr && drawable->NeedsAnimationFrame(), &result);
    return result;
}

napi_value SvgDrawable::OnFrameTimeNanos(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_value argv[1];
    size_t argc = 1;
    napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    double frame_time_nanos = 0.0;
    if (argc > 0) {
        napi_get_value_double(env, argv[0], &frame_time_nanos);
    }
    napi_value result;
    napi_get_boolean(env, drawable != nullptr && drawable->OnFrameTimeNanos(static_cast<int64_t>(frame_time_nanos)),
                     &result);
    return result;
}

napi_value SvgDrawable::CurrentAnimationSeconds(napi_env env, napi_callback_info info) {
    napi_value js_this;
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    napi_value result;
    napi_create_double(env, drawable != nullptr ? drawable->CurrentAnimationSeconds() : 0.0, &result);
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
    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
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
        drawable->Update(str, left * scale, top * scale, width * scale, height * scale, anti_alias, has_color, color);
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
            auto *drawable = static_cast<SvgDrawable *>(data);
            if (drawable != nullptr) {
                drawable->ReleaseResources();
                drawable->DeleteReference(drawable->self_ref_);
                delete drawable;
            }
        },
        nullptr, nullptr);
    napi_create_reference(env, js_this, 0, &obj->self_ref_);

    return js_this;
}

napi_value SvgDrawable::Dispose(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_value js_this;
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);

    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    drawable->ReleaseResources();

    napi_get_undefined(env, &result);
    return result;
}

napi_value SvgDrawable::SetImageFetcher(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_value js_this;
    napi_value argv[1];
    size_t argc = 1;
    napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);

    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    if (drawable == nullptr || drawable->disposed_) {
        napi_get_undefined(env, &result);
        return result;
    }
    drawable->image_request_epoch_++;
    drawable->DeleteReference(drawable->image_fetcher_ref_);
    drawable->ClearImageCache();
    if (argc > 0) {
        napi_valuetype loader_type = napi_undefined;
        napi_typeof(env, argv[0], &loader_type);
        if (loader_type == napi_function) {
            napi_create_reference(env, argv[0], 1, &drawable->image_fetcher_ref_);
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value SvgDrawable::SetInvalidateCallback(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_value js_this;
    napi_value argv[1];
    size_t argc = 1;
    napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);

    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    if (drawable == nullptr || drawable->disposed_) {
        napi_get_undefined(env, &result);
        return result;
    }
    drawable->DeleteReference(drawable->invalidate_ref_);
    if (argc > 0) {
        napi_valuetype callback_type = napi_undefined;
        napi_typeof(env, argv[0], &callback_type);
        if (callback_type == napi_function) {
            napi_create_reference(env, argv[0], 1, &drawable->invalidate_ref_);
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value SvgDrawable::RetryFailedImages(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_value js_this;
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);

    SvgDrawable *drawable;
    napi_unwrap(env, js_this, reinterpret_cast<void **>(&drawable));
    if (drawable != nullptr && !drawable->disposed_) {
        drawable->RetryFailedImages();
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value SvgDrawable::OnFetchFulfilled(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_value argv[1];
    size_t argc = 1;
    void *data = nullptr;
    napi_get_cb_info(env, info, &argc, argv, nullptr, &data);

    std::unique_ptr<ImageFetchContext> context(static_cast<ImageFetchContext *>(data));
    SvgDrawable *drawable = context != nullptr ? context->svg : nullptr;
    if (drawable != nullptr) {
        const bool is_outdated =
            drawable->disposed_ || context->epoch != drawable->image_request_epoch_ || drawable->env_ == nullptr;
        if (!is_outdated) {
            napi_valuetype type = napi_undefined;
            if (argc > 0) {
                napi_typeof(env, argv[0], &type);
            }
            if (type == napi_undefined || type == napi_null || !drawable->CacheImage(context->url, argv[0])) {
                drawable->MarkImageFailed(context->url);
            } else {
                drawable->NotifyInvalidate();
            }
        } else if (argc > 0) {
            napi_valuetype type = napi_undefined;
            napi_typeof(env, argv[0], &type);
            if (type != napi_undefined && type != napi_null) {
                ReleasePixelMapValue(env, argv[0]);
            }
        }
        drawable->ReleaseSelf();
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value SvgDrawable::OnFetchRejected(napi_env env, napi_callback_info info) {
    napi_value result;
    void *data = nullptr;
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data);

    std::unique_ptr<ImageFetchContext> context(static_cast<ImageFetchContext *>(data));
    SvgDrawable *drawable = context != nullptr ? context->svg : nullptr;
    if (drawable != nullptr) {
        if (!drawable->disposed_ && context->epoch == drawable->image_request_epoch_) {
            drawable->MarkImageFailed(context->url);
        }
        drawable->ReleaseSelf();
    }

    napi_get_undefined(env, &result);
    return result;
}

void SvgDrawable::ReleaseResources() {
    if (disposed_) {
        return;
    }
    disposed_ = true;
    image_request_epoch_++;
    ClearImageCache();
    DeleteReference(image_fetcher_ref_);
    DeleteReference(invalidate_ref_);
    sr_canvas_.reset();
    svg_dom_.reset();
    animation_state_.SetHasAnimations(false);
    animation_state_.SetAnimationTimelineEndSeconds(0.0);
}

void SvgDrawable::ClearImageCache() {
    for (auto &entry : image_cache_) {
        ReleaseCachedImage(env_, entry.second, true);
    }
    image_cache_.clear();
}

void SvgDrawable::RetryFailedImages() {
    for (auto &entry : image_cache_) {
        if (entry.second.state == ImageLoadState::kFailed) {
            ReleaseCachedImage(env_, entry.second, true);
        }
    }
}

void SvgDrawable::RetainSelf() {
    if (env_ == nullptr || self_ref_ == nullptr) {
        return;
    }
    uint32_t ref_count = 0;
    napi_reference_ref(env_, self_ref_, &ref_count);
    ++inflight_count_;
}

void SvgDrawable::ReleaseSelf() {
    if (env_ == nullptr || self_ref_ == nullptr) {
        return;
    }
    if (inflight_count_ > 0) {
        --inflight_count_;
    }
    uint32_t ref_count = 0;
    napi_reference_unref(env_, self_ref_, &ref_count);
}

void SvgDrawable::DeleteReference(napi_ref &ref) {
    if (env_ != nullptr && ref != nullptr) {
        napi_delete_reference(env_, ref);
    }
    ref = nullptr;
}

bool SvgDrawable::CacheImage(const std::string &url, napi_value pixel_map_value) {
    if (env_ == nullptr || disposed_) {
        return false;
    }

    CachedImage cached_image;
    NativePixelMap *native_pixel_map = OH_PixelMap_InitNativePixelMap(env_, pixel_map_value);
    if (native_pixel_map == nullptr) {
        return false;
    }

    OhosPixelMapInfos image_info{};
    if (OH_PixelMap_GetImageInfo(native_pixel_map, &image_info) != 0) {
        return false;
    }

    OH_Drawing_PixelMap *draw_pixel_map = OH_Drawing_PixelMapGetFromNativePixelMap(native_pixel_map);
    if (draw_pixel_map == nullptr) {
        return false;
    }

    cached_image.image_data.draw_pixel_map = draw_pixel_map;
    cached_image.image_data.width = image_info.width;
    cached_image.image_data.height = image_info.height;
    cached_image.state = ImageLoadState::kReady;
    if (napi_create_reference(env_, pixel_map_value, 1, &cached_image.pixel_map_ref) != napi_ok) {
        ReleaseCachedImage(env_, cached_image, false);
        return false;
    }
    auto &entry = image_cache_[url];
    ReleaseCachedImage(env_, entry, true);
    entry = std::move(cached_image);
    return true;
}

const SvgDrawable::CachedImage *SvgDrawable::RequestImage(const std::string &url) {
    auto &entry = image_cache_[url];
    switch (entry.state) {
    case ImageLoadState::kReady:
        return &entry;
    case ImageLoadState::kLoading:
    case ImageLoadState::kFailed:
        return nullptr;
    case ImageLoadState::kIdle:
        break;
    }
    if (env_ == nullptr || image_fetcher_ref_ == nullptr || disposed_) {
        return nullptr;
    }
    StartFetchImage(url);
    return nullptr;
}

void SvgDrawable::StartFetchImage(const std::string &url) {
    auto cache_it = image_cache_.find(url);
    if (cache_it == image_cache_.end() || cache_it->second.state != ImageLoadState::kIdle || env_ == nullptr ||
        image_fetcher_ref_ == nullptr || disposed_) {
        return;
    }
    cache_it->second.state = ImageLoadState::kLoading;

    napi_value fetcher;
    napi_value global;
    napi_value argv[1];
    napi_value promise;
    napi_get_reference_value(env_, image_fetcher_ref_, &fetcher);
    napi_get_global(env_, &global);
    napi_create_string_utf8(env_, url.c_str(), NAPI_AUTO_LENGTH, &argv[0]);
    if (napi_call_function(env_, global, fetcher, 1, argv, &promise) != napi_ok) {
        MarkImageFailed(url);
        return;
    }

    napi_value then_func = nullptr;
    napi_valuetype then_type = napi_undefined;
    if (napi_get_named_property(env_, promise, "then", &then_func) != napi_ok ||
        napi_typeof(env_, then_func, &then_type) != napi_ok || then_type != napi_function) {
        MarkImageFailed(url);
        return;
    }

    auto *context = new ImageFetchContext{this, url, image_request_epoch_};
    RetainSelf();

    napi_value resolve_cb = nullptr;
    napi_value reject_cb = nullptr;
    napi_create_function(env_, "onFetchFulfilled", NAPI_AUTO_LENGTH, OnFetchFulfilled, context, &resolve_cb);
    napi_create_function(env_, "onFetchRejected", NAPI_AUTO_LENGTH, OnFetchRejected, context, &reject_cb);

    napi_value then_argv[2] = {resolve_cb, reject_cb};
    napi_value chained_promise = nullptr;
    if (napi_call_function(env_, promise, then_func, 2, then_argv, &chained_promise) != napi_ok) {
        MarkImageFailed(url);
        delete context;
        ReleaseSelf();
    }
}

void SvgDrawable::MarkImageFailed(const std::string &url) {
    auto &entry = image_cache_[url];
    ReleaseCachedImage(env_, entry, true);
    entry.state = ImageLoadState::kFailed;
}

void SvgDrawable::NotifyInvalidate() {
    if (disposed_ || env_ == nullptr || invalidate_ref_ == nullptr) {
        return;
    }
    napi_value callback = nullptr;
    napi_value global = nullptr;
    napi_value result = nullptr;
    if (napi_get_reference_value(env_, invalidate_ref_, &callback) != napi_ok) {
        return;
    }
    napi_get_global(env_, &global);
    napi_call_function(env_, global, callback, 0, nullptr, &result);
}

void SvgDrawable::Render(OH_Drawing_Canvas *canvas) { RenderAtTime(canvas, animation_state_.CurrentSeconds()); }

void SvgDrawable::RenderAtTime(OH_Drawing_Canvas *canvas, double seconds) {
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
        if (svg_dom_->HasAnimations()) {
            svg_dom_->RenderAtTime(sr_canvas_.get(), box, seconds);
        } else {
            svg_dom_->Render(sr_canvas_.get(), box);
        }
        last_result_ = MakeRenderResult(svg_dom_->diagnostics());
    }
}

bool SvgDrawable::HasAnimations() const { return animation_state_.HasAnimations(); }

double SvgDrawable::AnimationTimelineEndSeconds() const { return animation_state_.AnimationTimelineEndSeconds(); }

void SvgDrawable::StartAnimation() { animation_state_.Start(); }

void SvgDrawable::StopAnimation() { animation_state_.Stop(); }

void SvgDrawable::ResetAnimationClock() { animation_state_.ResetClock(); }

bool SvgDrawable::NeedsAnimationFrame() const { return animation_state_.NeedsAnimationFrame(); }

bool SvgDrawable::OnFrameTimeNanos(int64_t frame_time_nanos) {
    return animation_state_.OnFrameTimeNanos(frame_time_nanos);
}

double SvgDrawable::CurrentAnimationSeconds() const { return animation_state_.CurrentSeconds(); }

SvgRenderResult harmony::SvgDrawable::Update(const std::string &content, float left, float top, float width,
                                             float height, bool anti_alias, bool has_color, std::string color) {
    std::vector<parser::SrSVGDiagnostic> diagnostics;
    RetryFailedImages();
    svg_dom_ = std::move(parser::SrSVGDOM::make(content.data(), content.size(), &diagnostics));
    left_ = left;
    top_ = top;
    width_ = width;
    height_ = height;
    anti_alias_ = anti_alias;
    has_color_ = has_color;
    color_ = std::move(color);
    animation_state_.SetHasAnimations(svg_dom_ && svg_dom_->HasAnimations());
    animation_state_.SetAnimationTimelineEndSeconds(svg_dom_ ? svg_dom_->AnimationTimelineEndSeconds() : 0.0);
    last_result_ = MakeRenderResult(diagnostics);
    return last_result_;
}
}  // namespace harmony
}  // namespace svg
}  // namespace serval
