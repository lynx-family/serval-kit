// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPARAGRAPHFACTORY_H_
#define SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPARAGRAPHFACTORY_H_

#include <jni.h>
#include <deque>
#include <memory>
#include <string>
#include "canvas/SrParagraph.h"
#include "platform/android/SrJNIUtils.h"
namespace serval {
namespace svg {
namespace canvas {
class SrAndroidParagraphFactory : public ParagraphFactory {
 private:
  std::deque<SrTextStyle> style_stack_;
  SrParagraphStyle paragraph_style_;
  android::JavaGlobalRef<jobject> j_paragraph_ref_{};
  const SrCanvas* srCanvas_{nullptr};

 public:
  SrAndroidParagraphFactory(const SrCanvas* srCanvas);
  ~SrAndroidParagraphFactory() = default;

  std::unique_ptr<Paragraph> CreateParagraph() override;

  void PushTextStyle(const SrTextStyle& style) override;

  void PopTextStyle() override;

  void SetParagraphStyle(SrParagraphStyle&& style) override;

  void AddText(const std::string& text) override;

  void Reset() override;
};

}  // namespace canvas
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPARAGRAPHFACTORY_H_
