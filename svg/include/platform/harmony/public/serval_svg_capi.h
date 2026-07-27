// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_HARMONY_PUBLIC_SERVAL_SVG_CAPI_H_
#define SVG_INCLUDE_PLATFORM_HARMONY_PUBLIC_SERVAL_SVG_CAPI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <multimedia/image_framework/image/pixelmap_native.h>
#include <native_drawing/drawing_types.h>

#if defined(__GNUC__)
#define SERVAL_SVG_EXPORT __attribute__((visibility("default")))
#else
#define SERVAL_SVG_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ServalSvgHandle ServalSvgHandle;

typedef struct ServalSvgImageData {
  OH_PixelmapNative* pixel_map;
  uint32_t width;
  uint32_t height;
} ServalSvgImageData;

typedef const ServalSvgImageData* (*ServalSvgImageProvider)(void* user_data,
                                                            const char* source);

typedef enum ServalSvgStatus {
  SERVAL_SVG_STATUS_OK = 0,
  SERVAL_SVG_STATUS_INVALID_ARGUMENT = -1,
  SERVAL_SVG_STATUS_REENTRANT = -2,
  SERVAL_SVG_STATUS_NOT_READY = -3,
} ServalSvgStatus;

typedef enum ServalSvgUpdateResult {
  SERVAL_SVG_UPDATE_OK = 0,
  SERVAL_SVG_UPDATE_WITH_DIAGNOSTICS = 1,
  SERVAL_SVG_UPDATE_INVALID_ARGUMENT = -1,
  SERVAL_SVG_UPDATE_DOCUMENT_CREATION_FAILED = -2,
  SERVAL_SVG_UPDATE_REENTRANT = -3,
} ServalSvgUpdateResult;

// Operations on different handles may run concurrently. Operations on the
// same handle are serialized internally. The business owner must ensure that
// destruction is the final operation and does not run concurrently with any
// other operation on the handle.
SERVAL_SVG_EXPORT ServalSvgHandle* serval_svg_create(void);

SERVAL_SVG_EXPORT ServalSvgStatus serval_svg_destroy(ServalSvgHandle* handle);

// Updates the SVG document and render options. A result of
// SERVAL_SVG_UPDATE_WITH_DIAGNOSTICS means that the document was created and
// can still be rendered. On failure, the previously accepted document and
// render options remain unchanged.
SERVAL_SVG_EXPORT ServalSvgUpdateResult
serval_svg_update(ServalSvgHandle* handle, const char* content,
                  size_t content_length, float left, float top, float width,
                  float height, bool anti_alias, const char* current_color);

// Returns a snapshot owned by the calling thread. The returned pointer remains
// valid until the next serval_svg_get_last_error call on the same thread.
SERVAL_SVG_EXPORT const char* serval_svg_get_last_error(
    const ServalSvgHandle* handle);

// The provider is called synchronously from serval_svg_render. Mutating,
// rendering, or destroying the same handle from the provider is rejected with
// a reentrant status. serval_svg_get_last_error remains available. user_data
// and the returned pixel_map must remain valid until rendering finishes. The
// caller retains ownership of pixel_map.
SERVAL_SVG_EXPORT ServalSvgStatus serval_svg_set_image_provider(
    ServalSvgHandle* handle, ServalSvgImageProvider provider, void* user_data);

SERVAL_SVG_EXPORT ServalSvgStatus serval_svg_render(ServalSvgHandle* handle,
                                                    OH_Drawing_Canvas* canvas);

#ifdef __cplusplus
}
#endif

#endif  // SVG_INCLUDE_PLATFORM_HARMONY_PUBLIC_SERVAL_SVG_CAPI_H_
