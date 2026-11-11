// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/internal/harmony_gesture_registry.h"

#include <memory>
#include <thread>

#include "gtest/gtest.h"

namespace serval::markdown {
namespace {

class HarmonyGestureRegistryTest : public ::testing::Test {
 protected:
  // The registry treats both views and recognizers as opaque handles.
  static std::shared_ptr<NativeServalMarkdownView> MakeView() {
    auto storage = std::make_shared<uintptr_t>();
    return {storage,
            reinterpret_cast<NativeServalMarkdownView*>(storage.get())};
  }
  uintptr_t handles_[2]{};
  ArkUI_GestureRecognizer* tap_ =
      reinterpret_cast<ArkUI_GestureRecognizer*>(&handles_[0]);
  ArkUI_GestureRecognizer* pan_ =
      reinterpret_cast<ArkUI_GestureRecognizer*>(&handles_[1]);
  HarmonyGestureRegistry registry_;
};

TEST_F(HarmonyGestureRegistryTest, RegistrationDoesNotRetainView) {
  auto host = MakeView();
  auto* token = registry_.Register(host, {tap_, pan_});
  std::weak_ptr<NativeServalMarkdownView> lifetime = host;
  ASSERT_EQ(registry_.GetView(token), host);
  ASSERT_EQ(registry_.GetView(tap_), host);
  ASSERT_EQ(registry_.GetView(pan_), host);

  host.reset();
  EXPECT_TRUE(lifetime.expired());
  EXPECT_FALSE(registry_.GetView(token));
  EXPECT_FALSE(registry_.GetView(tap_));
  EXPECT_FALSE(registry_.GetView(pan_));
  registry_.Unregister(token, {tap_, pan_});
}

TEST_F(HarmonyGestureRegistryTest, ActiveCallbacksRetainViewAfterHostRelease) {
  auto host = MakeView();
  auto* token = registry_.Register(host, {pan_});
  auto dispatch = registry_.GetView(token);
  auto interrupt = registry_.GetView(pan_);
  ASSERT_EQ(dispatch, host);
  ASSERT_EQ(interrupt, host);
  std::weak_ptr<NativeServalMarkdownView> lifetime = dispatch;

  std::thread teardown([&host] { host.reset(); });
  teardown.join();
  dispatch.reset();
  EXPECT_EQ(registry_.GetView(token), interrupt);
  interrupt.reset();
  EXPECT_TRUE(lifetime.expired());
  EXPECT_FALSE(registry_.GetView(pan_));
  registry_.Unregister(token, {pan_});
}

TEST_F(HarmonyGestureRegistryTest, DiscardsOldTokenAfterRecognizerReuse) {
  auto host = MakeView();
  auto* old_token = registry_.Register(host, {tap_, pan_});
  registry_.Unregister(old_token, {tap_, pan_});
  EXPECT_FALSE(registry_.GetView(tap_));
  EXPECT_FALSE(registry_.GetView(pan_));

  auto* new_token = registry_.Register(host, {pan_});
  EXPECT_FALSE(registry_.GetView(old_token));
  EXPECT_EQ(registry_.GetView(pan_), host);
  EXPECT_EQ(registry_.GetView(new_token), host);
  registry_.Unregister(new_token, {pan_});
}

}  // namespace
}  // namespace serval::markdown
