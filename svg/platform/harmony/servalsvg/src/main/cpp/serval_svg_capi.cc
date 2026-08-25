// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/public/serval_svg_capi.h"

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "element/SrSVGTypes.h"
#include "parser/SrSVGDOM.h"
#include "platform/harmony/sr_harmony_canvas.h"

struct ServalSvgHandle {
    mutable std::recursive_mutex mutex;
    bool in_provider_callback{false};
    std::unique_ptr<serval::svg::parser::SrSVGDOM> dom;
    std::unique_ptr<serval::svg::harmony::SrHarmonyCanvas> canvas;
    ServalSvgImageProvider image_provider{nullptr};
    void *image_provider_user_data{nullptr};
    serval::svg::harmony::SrHarmonyCanvas::ImageData image_data;
    std::string last_error;
    std::string current_color;
    float left{0.f};
    float top{0.f};
    float width{0.f};
    float height{0.f};
    bool anti_alias{true};
    bool has_current_color{false};
};

namespace {

class ScopedProviderCallback {
public:
    explicit ScopedProviderCallback(ServalSvgHandle *handle)
        : handle_(handle), previous_(handle->in_provider_callback) {
        handle_->in_provider_callback = true;
    }
    ~ScopedProviderCallback() { handle_->in_provider_callback = previous_; }

private:
    ServalSvgHandle *handle_;
    bool previous_;
};

void UpdateLastError(ServalSvgHandle *handle, const std::vector<serval::svg::parser::SrSVGDiagnostic> &diagnostics) {
    handle->last_error = diagnostics.empty() ? std::string() : diagnostics.front().message;
}

bool UpdateImageData(ServalSvgHandle *handle, OH_PixelmapNative *pixel_map) {
    OH_Pixelmap_ImageInfo *image_info = nullptr;
    if (OH_PixelmapImageInfo_Create(&image_info) != IMAGE_SUCCESS || image_info == nullptr) {
        return false;
    }

    uint32_t width = 0;
    uint32_t height = 0;
    const bool success = OH_PixelmapNative_GetImageInfo(pixel_map, image_info) == IMAGE_SUCCESS &&
                         OH_PixelmapImageInfo_GetWidth(image_info, &width) == IMAGE_SUCCESS &&
                         OH_PixelmapImageInfo_GetHeight(image_info, &height) == IMAGE_SUCCESS && width > 0 &&
                         height > 0;
    OH_PixelmapImageInfo_Release(image_info);
    if (!success) {
        return false;
    }

    handle->image_data.pixel_map = pixel_map;
    handle->image_data.width = width;
    handle->image_data.height = height;
    return true;
}

}  // namespace

ServalSvgHandle *serval_svg_create(ServalSvgImageProvider provider, void *user_data) {
    auto *handle = new ServalSvgHandle();
    handle->image_provider = provider;
    handle->image_provider_user_data = user_data;
    return handle;
}

ServalSvgStatus serval_svg_destroy(ServalSvgHandle *handle) {
    if (handle == nullptr) {
        return SERVAL_SVG_STATUS_INVALID_ARGUMENT;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(handle->mutex);
        if (handle->in_provider_callback) {
            return SERVAL_SVG_STATUS_REENTRANT;
        }
    }
    delete handle;
    return SERVAL_SVG_STATUS_OK;
}

ServalSvgUpdateResult serval_svg_update(ServalSvgHandle *handle, const char *content, size_t content_length, float left,
                                        float top, float width, float height, bool anti_alias,
                                        const char *current_color) {
    if (handle == nullptr) {
        return SERVAL_SVG_UPDATE_INVALID_ARGUMENT;
    }
    std::lock_guard<std::recursive_mutex> lock(handle->mutex);
    if (handle->in_provider_callback) {
        return SERVAL_SVG_UPDATE_REENTRANT;
    }
    if (content == nullptr && content_length != 0) {
        handle->last_error = "SVG content is null but content_length is non-zero.";
        return SERVAL_SVG_UPDATE_INVALID_ARGUMENT;
    }

    std::vector<serval::svg::parser::SrSVGDiagnostic> diagnostics;
    const char *svg_content = content == nullptr ? "" : content;
    auto dom = serval::svg::parser::SrSVGDOM::make(svg_content, content_length, &diagnostics);
    UpdateLastError(handle, diagnostics);
    if (dom == nullptr) {
        if (handle->last_error.empty()) {
            handle->last_error = "Failed to create SVG document.";
        }
        return SERVAL_SVG_UPDATE_DOCUMENT_CREATION_FAILED;
    }

    handle->dom = std::move(dom);
    handle->left = left;
    handle->top = top;
    handle->width = width;
    handle->height = height;
    handle->anti_alias = anti_alias;
    handle->has_current_color = current_color != nullptr;
    handle->current_color = current_color == nullptr ? std::string() : std::string(current_color);
    return diagnostics.empty() ? SERVAL_SVG_UPDATE_OK : SERVAL_SVG_UPDATE_WITH_DIAGNOSTICS;
}

const char *serval_svg_get_last_error(const ServalSvgHandle *handle) {
    static thread_local std::string last_error_snapshot;
    if (handle == nullptr) {
        last_error_snapshot.clear();
        return last_error_snapshot.c_str();
    }
    std::lock_guard<std::recursive_mutex> lock(handle->mutex);
    last_error_snapshot = handle->last_error;
    return last_error_snapshot.c_str();
}

ServalSvgStatus serval_svg_render(ServalSvgHandle *handle, OH_Drawing_Canvas *canvas) {
    if (handle == nullptr || canvas == nullptr) {
        return SERVAL_SVG_STATUS_INVALID_ARGUMENT;
    }
    std::lock_guard<std::recursive_mutex> lock(handle->mutex);
    if (handle->in_provider_callback) {
        return SERVAL_SVG_STATUS_REENTRANT;
    }
    if (handle->dom == nullptr) {
        return SERVAL_SVG_STATUS_NOT_READY;
    }

    if (handle->canvas == nullptr) {
        handle->canvas = std::make_unique<serval::svg::harmony::SrHarmonyCanvas>(canvas);
    } else {
        handle->canvas->Reset(canvas);
    }
    handle->canvas->SetAntiAlias(handle->anti_alias);
    handle->canvas->SetImageProvider(
        [handle](const std::string &source) -> const serval::svg::harmony::SrHarmonyCanvas::ImageData * {
            if (handle->image_provider == nullptr) {
                return nullptr;
            }
            ScopedProviderCallback callback_scope(handle);
            OH_PixelmapNative *pixel_map = handle->image_provider(handle->image_provider_user_data, source.c_str());
            if (pixel_map == nullptr || !UpdateImageData(handle, pixel_map)) {
                return nullptr;
            }
            return &handle->image_data;
        });

    if (handle->has_current_color) {
        uint32_t color = 0;
        if (parse_svg_color(handle->current_color.c_str(), &color)) {
            handle->dom->SetDefaultColor(color);
        } else {
            handle->dom->ResetDefaultColor();
        }
    } else {
        handle->dom->ResetDefaultColor();
    }

    SrSVGBox viewport{handle->left, handle->top, handle->width, handle->height};
    handle->dom->Render(handle->canvas.get(), viewport);
    UpdateLastError(handle, handle->dom->diagnostics());
    return SERVAL_SVG_STATUS_OK;
}
