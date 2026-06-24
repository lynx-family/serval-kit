// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_PLATFORM_HARMONY_SERVALSVG_SRC_MAIN_CPP_SVG_DRAWABLE_H_
#define SVG_PLATFORM_HARMONY_SERVALSVG_SRC_MAIN_CPP_SVG_DRAWABLE_H_

#include "parser/SrSVGDOM.h"
#include "platform/harmony/sr_harmony_canvas.h"
#include "renderer/SrSVGAnimationState.h"

#include <cstdint>
#include <memory>
#include <multimedia/image_framework/image_pixel_map_mdk.h>
#include <native_drawing/drawing_types.h>
#include <node_api.h>
#include <string>
#include <sys/stat.h>
#include <unordered_map>

namespace serval {
namespace svg {
namespace harmony {

struct SvgRenderResult {
    bool has_error{false};
    std::string error_message;
};

enum class ImageLoadState {
    kIdle,
    kLoading,
    kReady,
    kFailed,
};

class SvgDrawable {
public:
    struct CachedImage {
        ImageLoadState state{ImageLoadState::kIdle};
        napi_ref pixel_map_ref{nullptr};
        SrHarmonyCanvas::ImageData image_data{};
    };

    static napi_value Init(napi_env env, napi_value exports);

    void Render(OH_Drawing_Canvas *canvas);
    void RenderAtTime(OH_Drawing_Canvas *canvas, double seconds);
    bool HasAnimations() const;
    double AnimationTimelineEndSeconds() const;
    void StartAnimation();
    void StopAnimation();
    void ResetAnimationClock();
    bool NeedsAnimationFrame() const;
    bool OnFrameTimeNanos(int64_t frame_time_nanos);
    double CurrentAnimationSeconds() const;

    SvgRenderResult Update(const std::string &content, float left, float top, float width, float height,
                           bool anti_alias, bool has_color, std::string color);

private:
    struct ImageFetchContext {
        SvgDrawable *svg{nullptr};
        std::string url;
        uint64_t epoch{0};
    };

    // JS methods
    static napi_value Update(napi_env env, napi_callback_info info);
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static napi_value Render(napi_env env, napi_callback_info info);
    static napi_value HasAnimations(napi_env env, napi_callback_info info);
    static napi_value AnimationTimelineEndSeconds(napi_env env, napi_callback_info info);
    static napi_value StartAnimation(napi_env env, napi_callback_info info);
    static napi_value StopAnimation(napi_env env, napi_callback_info info);
    static napi_value ResetAnimationClock(napi_env env, napi_callback_info info);
    static napi_value NeedsAnimationFrame(napi_env env, napi_callback_info info);
    static napi_value OnFrameTimeNanos(napi_env env, napi_callback_info info);
    static napi_value CurrentAnimationSeconds(napi_env env, napi_callback_info info);
    static napi_value Dispose(napi_env env, napi_callback_info info);
    static napi_value SetImageFetcher(napi_env env, napi_callback_info info);
    static napi_value SetInvalidateCallback(napi_env env, napi_callback_info info);
    static napi_value RetryFailedImages(napi_env env, napi_callback_info info);
    static napi_value OnFetchFulfilled(napi_env env, napi_callback_info info);
    static napi_value OnFetchRejected(napi_env env, napi_callback_info info);

    static std::string ConvertToString(napi_env env, napi_value arg);
    void ReleaseResources();
    void ClearImageCache();
    void RetryFailedImages();
    const CachedImage *RequestImage(const std::string &url);
    void StartFetchImage(const std::string &url);
    void MarkImageFailed(const std::string &url);
    void NotifyInvalidate();
    bool CacheImage(const std::string &url, napi_value pixel_map_value);
    void RetainSelf();
    void ReleaseSelf();
    void DeleteReference(napi_ref &ref);

    napi_env env_{nullptr};
    napi_ref self_ref_{nullptr};
    napi_ref image_fetcher_ref_{nullptr};
    napi_ref invalidate_ref_{nullptr};
    bool disposed_{false};
    size_t inflight_count_{0};
    uint64_t image_request_epoch_{0};
    float left_{0.f};
    float top_{0.f};
    float width_{0.f};
    float height_{0.f};
    bool anti_alias_{true};
    bool has_color_{false};
    std::string color_;
    SvgRenderResult last_result_{};
    std::unique_ptr<SrHarmonyCanvas> sr_canvas_{nullptr};
    std::unique_ptr<parser::SrSVGDOM> svg_dom_{nullptr};
    renderer::SrSVGAnimationState animation_state_{};
    std::unordered_map<std::string, CachedImage> image_cache_{};
};
}  // namespace harmony
}  // namespace svg

}  // namespace serval

#endif  // SVG_PLATFORM_HARMONY_SERVALSVG_SRC_MAIN_CPP_SVG_DRAWABLE_H_
