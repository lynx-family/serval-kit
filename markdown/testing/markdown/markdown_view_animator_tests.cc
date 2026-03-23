// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gtest/gtest.h"

#include "markdown/view/markdown_view_animator.h"

namespace serval::markdown {

TEST(MarkdownViewAnimatorTest, UpdateTypewriterStepAdvancesByVelocity) {
  MarkdownViewAnimator animator;
  animator.SetAnimationType(MarkdownAnimationType::kTypewriter);
  animator.SetAnimationVelocity(2.0f);
  animator.SetMaxAnimationStep(10);
  animator.SetAnimationStep(0);

  animator.UpdateCurrentTime(100);
  auto r1 = animator.UpdateTypewriterStep();
  EXPECT_EQ(r1, 1);
  EXPECT_EQ(animator.GetAnimationStep(), 1);

  animator.UpdateCurrentTime(300);
  auto r2 = animator.UpdateTypewriterStep();
  EXPECT_EQ(r2, 0);
  EXPECT_EQ(animator.GetAnimationStep(), 1);

  animator.UpdateCurrentTime(600);
  auto r3 = animator.UpdateTypewriterStep();
  EXPECT_EQ(r3, 1);
  EXPECT_EQ(animator.GetAnimationStep(), 2);

  animator.UpdateCurrentTime(1600);
  auto r4 = animator.UpdateTypewriterStep();
  EXPECT_EQ(r4, 2);
  EXPECT_EQ(animator.GetAnimationStep(), 4);
}

TEST(MarkdownViewAnimatorTest, UpdateTypewriterStepClampsToMax) {
  MarkdownViewAnimator animator;
  animator.SetAnimationType(MarkdownAnimationType::kTypewriter);
  animator.SetAnimationVelocity(2.0f);
  animator.SetMaxAnimationStep(10);
  animator.SetAnimationStep(9);

  animator.UpdateCurrentTime(3000);
  auto r = animator.UpdateTypewriterStep();
  EXPECT_GT(r, 0);
  EXPECT_EQ(animator.GetAnimationStep(), 10);
}

TEST(MarkdownViewAnimatorTest, UpdateTypewriterStepIgnoresNonMonotonicTime) {
  MarkdownViewAnimator animator;
  animator.SetAnimationType(MarkdownAnimationType::kTypewriter);
  animator.SetAnimationVelocity(2.0f);
  animator.SetMaxAnimationStep(10);
  animator.SetAnimationStep(0);

  animator.UpdateCurrentTime(1000);
  auto r1 = animator.UpdateTypewriterStep();
  EXPECT_EQ(r1, 1);
  EXPECT_EQ(animator.GetAnimationStep(), 1);

  animator.UpdateCurrentTime(900);
  auto r2 = animator.UpdateTypewriterStep();
  EXPECT_EQ(r2, 0);
  EXPECT_EQ(animator.GetAnimationStep(), 1);
}

TEST(MarkdownViewAnimatorTest, GetAnimationStepAfterUsesVelocityAndClamps) {
  MarkdownViewAnimator animator;
  animator.SetAnimationType(MarkdownAnimationType::kTypewriter);
  animator.SetAnimationVelocity(100.0f);
  animator.SetMaxAnimationStep(25);
  animator.SetAnimationStep(10);

  EXPECT_EQ(animator.GetAnimationStepAfter(0.1f), 20);
  EXPECT_EQ(animator.GetAnimationStepAfter(1.0f), 25);
  EXPECT_EQ(animator.GetAnimationStepAfter(0.0f), 10);
}

TEST(MarkdownViewAnimatorTest, UpdateHeightTransitionInterpolatesLinearly) {
  MarkdownViewAnimator animator;
  animator.SetHeightTransitionDuration(1.0f);

  animator.UpdateCurrentTime(1000);
  float h0 = animator.UpdateHeightTransition(100);
  EXPECT_EQ(h0, 100);

  animator.UpdateCurrentTime(1100);
  float h1 = animator.UpdateHeightTransition(200);
  EXPECT_EQ(h1, 100);

  animator.UpdateCurrentTime(1600);
  float h2 = animator.UpdateHeightTransition(200);
  EXPECT_NEAR(h2, 150, 0.01);

  animator.UpdateCurrentTime(2100);
  float h3 = animator.UpdateHeightTransition(200);
  EXPECT_EQ(h3, 200);
}

TEST(MarkdownViewAnimatorTest, UpdateHeightTransitionClampsDurationMsToOne) {
  MarkdownViewAnimator animator;
  animator.SetHeightTransitionDuration(0.0001f);

  animator.UpdateCurrentTime(1000);
  float h0 = animator.UpdateHeightTransition(100);
  EXPECT_EQ(h0, 100);

  animator.UpdateCurrentTime(1000);
  float h1 = animator.UpdateHeightTransition(200);
  EXPECT_EQ(h1, 100);

  animator.UpdateCurrentTime(1001);
  float h2 = animator.UpdateHeightTransition(200);
  EXPECT_EQ(h2, 200);
}

}  // namespace serval::markdown
