// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gtest/gtest.h"
#include "markdown/markdown_event_listener.h"
#include "markdown/markdown_exposure_listener.h"
#include "testing/markdown/mock_platform/markdown_tests_platform.h"
#include "testing/markdown/mock_platform/mock_markdown_resource_loader.h"

namespace serval::markdown {
namespace {
MeasureSpec ParityMeasureSpec() {
  return {.width_ = 500,
          .width_mode_ = tttext::LayoutMode::kAtMost,
          .height_ = MeasureSpec::LAYOUT_MAX_SIZE,
          .height_mode_ = tttext::LayoutMode::kIndefinite};
}
class ParityViewTest : public ::testing::Test {
 protected:
  ParityViewTest() : main_view_(testing::CreateTestMarkdownSharedContext()) {
    loader_.SetMainView(&main_view_);
    view_ = main_view_.GetMarkdownView();
    view_->SetResourceLoader(&loader_);
  }
  void Render(const std::string& content) {
    view_->SetContent(content);
    main_view_.Measure(ParityMeasureSpec());
    view_->OnRendererFrame(0);
  }
  testing::MockMarkdownResourceLoader loader_;
  testing::MockMarkdownMainView main_view_;
  MarkdownView* view_;
};
class TextClickListener : public MarkdownEventListener {
 public:
  void OnParseEnd() override {}
  void OnTextOverflow(MarkdownTextOverflow) override {}
  void OnDrawStart() override {}
  void OnDrawEnd() override {}
  void OnAnimationStep(int32_t, int32_t) override {}
  void OnLinkClicked(const char*, const char*) override {}
  void OnImageClicked(const char*) override {}
  void OnSelectionChanged(int32_t, int32_t, SelectionHandleType,
                          SelectionState) override {}
  void OnTextClicked(const char* id) override { clicked_id_ = id; }
  std::string clicked_id_;
};
class ExposureListener : public MarkdownExposureListener {
 public:
  void OnLinkAppear(const char*, const char*) override { ++links_; }
  void OnLinkDisappear(const char*, const char*) override { --links_; }
  void OnImageAppear(const char*) override { ++images_; }
  void OnImageDisappear(const char*) override { --images_; }
  int links_{0};
  int images_{0};
};
}  // namespace

TEST_F(ParityViewTest, UnorderedListCopyIncludesMarkerAndImageAltText) {
  Render("- 111\n- 222\n");
  EXPECT_EQ(view_->GetParsedContent({0, 100}), "- 111\n- 222\n");
  EXPECT_EQ(view_->GetParsedContent({0, 1}), "- ");
  EXPECT_EQ(view_->GetParsedContent({1, 3}), "11");
  Render("- ![img](url)text\n");
  EXPECT_EQ(view_->GetParsedContent({0, 100}), "- imgtext\n");
}

TEST_F(ParityViewTest, ContentRangeDoesNotLeaveDiscardedListMarkerAltText) {
  view_->SetContentRange({13, 19});
  Render("- 111\n- 222\n\nplain\n");
  EXPECT_EQ(view_->GetParsedContent({0, 100}), "plain\n");
}

TEST_F(ParityViewTest, ImagesAndInlineViewsMapWholeSourceToOneCharacter) {
  for (const std::string& url :
       {"url", "inlineview://view", "blockview://view"}) {
    const std::string source = "a![img](" + url + ")b";
    const int end = static_cast<int>(source.size()) - 1;
    Render(source);
    EXPECT_EQ(view_->SourceOffsetToCharOffset(1), 1) << source;
    EXPECT_EQ(view_->SourceOffsetToCharOffset(5), 2) << source;
    EXPECT_EQ(view_->SourceOffsetToCharOffset(end), 2) << source;
    EXPECT_EQ(view_->CharOffsetToSourceOffset(1), 1) << source;
    EXPECT_EQ(view_->CharOffsetToSourceOffset(2), end) << source;
  }
}

TEST_F(ParityViewTest, ImageSourceMappingUsesUnicodeCharacterOffsets) {
  Render("中😀![img](url)b");
  EXPECT_EQ(view_->SourceOffsetToCharOffset(2), 2);
  EXPECT_EQ(view_->SourceOffsetToCharOffset(7), 3);
  EXPECT_EQ(view_->SourceOffsetToCharOffset(13), 3);
  EXPECT_EQ(view_->CharOffsetToSourceOffset(2), 2);
  EXPECT_EQ(view_->CharOffsetToSourceOffset(3), 13);
}

TEST_F(ParityViewTest, MaxHeightPropConstrainsLayoutAndCanBeRemoved) {
  view_->SetContent("first\n\nsecond\n\nthird\n\nfourth\n\nfifth");
  auto full = main_view_.Measure(ParityMeasureSpec());
  view_->SetNumberProp(MarkdownProps::kMarkdownMaxHeight, 60);
  auto limited = main_view_.Measure(ParityMeasureSpec());
  EXPECT_GT(full.height_, 60);
  EXPECT_LE(limited.height_, 60);
  EXPECT_GT(limited.height_, 0);
  view_->SetNumberProp(MarkdownProps::kMarkdownMaxHeight, 0);
  auto restored = main_view_.Measure(ParityMeasureSpec());
  EXPECT_FLOAT_EQ(restored.height_, full.height_);
  auto spec = ParityMeasureSpec();
  spec.height_ = 40;
  spec.height_mode_ = tttext::LayoutMode::kAtMost;
  view_->SetNumberProp(MarkdownProps::kMarkdownMaxHeight, 60);
  EXPECT_LE(main_view_.Measure(spec).height_, 40);
}

TEST_F(ParityViewTest, TextAttachmentTapReturnsIdentifier) {
  TextClickListener listener;
  view_->SetEventListener(&listener);
  ValueMap style;
  style.emplace("color", Value::MakeString("#ff0000"));
  ValueMap attachment;
  attachment.emplace("id", Value::MakeString("note-😀"));
  attachment.emplace("startIndex", Value::MakeInt(0));
  attachment.emplace("endIndex", Value::MakeInt(3));
  attachment.emplace("style", Value::MakeMap(std::move(style)));
  ValueArray attachments;
  attachments.emplace_back(Value::MakeMap(std::move(attachment)));
  view_->SetArrayProp(MarkdownProps::kTextMarkAttachments, attachments);
  Render("hello world");
  auto rects = view_->GetTextLineBoundingRect({1, 2});
  ASSERT_FALSE(rects.empty());
  const auto& rect = rects.front();
  EXPECT_TRUE(view_->OnTap({(rect.GetLeft() + rect.GetRight()) / 2,
                            (rect.GetTop() + rect.GetBottom()) / 2},
                           GestureEventType::kDown));
  EXPECT_EQ(listener.clicked_id_, "note-😀");
  view_->SetEventListener(nullptr);
}

TEST_F(ParityViewTest, ExposureTagsFilterAndUpdateExistingExposure) {
  ExposureListener listener;
  view_->SetExposureListener(&listener);
  Render("[link](url) ![image](image)");
  EXPECT_EQ(listener.links_, 0);
  EXPECT_EQ(listener.images_, 0);
  ValueArray tags;
  tags.emplace_back(Value::MakeString("link"));
  view_->SetArrayProp(MarkdownProps::kExposureTags, tags);
  view_->OnRendererFrame(1);
  EXPECT_EQ(listener.links_, 1);
  EXPECT_EQ(listener.images_, 0);
  tags.clear();
  tags.emplace_back(Value::MakeString("image"));
  view_->SetArrayProp(MarkdownProps::kExposureTags, tags);
  view_->OnRendererFrame(2);
  EXPECT_EQ(listener.links_, 0);
  EXPECT_EQ(listener.images_, 1);
  tags.clear();
  view_->SetArrayProp(MarkdownProps::kExposureTags, tags);
  view_->OnRendererFrame(3);
  EXPECT_EQ(listener.links_, 0);
  EXPECT_EQ(listener.images_, 0);
  view_->SetExposureListener(nullptr);
}
}  // namespace serval::markdown
