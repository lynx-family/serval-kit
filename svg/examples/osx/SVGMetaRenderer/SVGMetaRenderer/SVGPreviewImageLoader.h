// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#include <memory>
#include <string>

namespace skity {
class Image;
}

// One loader per preview document. Lookups and invalidation run on the main thread.
@interface SVGPreviewImageLoader : NSObject
- (instancetype)initWithRedraw:(dispatch_block_t)redraw;
- (std::shared_ptr<skity::Image>)imageForHref:(const std::string&)href;
- (void)invalidate;
@end
