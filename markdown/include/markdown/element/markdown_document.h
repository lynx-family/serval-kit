// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_DOCUMENT_H_
#define MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_DOCUMENT_H_
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "markdown/element/markdown_page.h"
#include "markdown/element/markdown_paragraph.h"
#include "markdown/markdown_event_listener.h"
#include "markdown/markdown_resource_loader.h"
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"

namespace lynx {
namespace markdown {
enum class MarkdownTouchEventType : uint8_t {
  kUnknown = 0,
  kDown = 1,
  kMove = 2,
  kUp = 3,
  kCancel = 4,
};
enum class MarkdownTouchState : uint8_t {
  kNone = 0,
  kOnScroll = 1,
};
struct MarkdownLink {
  std::string url_;
  std::string content_;
  uint32_t char_start_;
  uint32_t char_count_;
  tttext::Paragraph* attached_paragraph_;
  uint32_t char_start_in_paragraph_;
};
struct MarkdownImage {
  std::string url_;
  int32_t char_index_;
  MarkdownPlatformView* view_;
};
struct MarkdownInlineView {
  std::string id_;
  int32_t char_index_;
  bool is_block_view_;
  MarkdownPlatformView* view_;
};
class MarkdownDocument {
 public:
  MarkdownDocument() : MarkdownDocument(nullptr) {}
  explicit MarkdownDocument(MarkdownResourceLoader* loader)
      : loader_(loader), touch_state_(MarkdownTouchState::kNone) {}
  const std::string& GetMarkdownContent() const { return markdown_content_; }
  void SetMarkdownContent(std::string_view content) {
    markdown_content_ = content;
  }
  void SetMarkdownContentRange(Range range) { content_range_ = range; }
  Range GetMarkdownContentRange() const { return content_range_; }
  void SetMaxSize(float width, float height) {
    max_width_ = width;
    max_height_ = height;
  }
  float GetMaxWidth() const { return max_width_; }
  float GetMaxHeight() const { return max_height_; }
  void SetMaxLines(int32_t max_lines) { max_lines_ = max_lines; }
  int32_t GetMaxLines() const { return max_lines_; }
  std::shared_ptr<MarkdownPage> GetPage() {
    std::lock_guard<std::mutex> guard(page_lock_);
    return page_;
  }
  void SetPage(std::shared_ptr<MarkdownPage> page) {
    std::lock_guard<std::mutex> guard(page_lock_);
    page_ = std::move(page);
  }
  void SetResourceLoader(MarkdownResourceLoader* loader) { loader_ = loader; }
  MarkdownResourceLoader* GetResourceLoader() const { return loader_; }
  void SetMarkdownEventListener(MarkdownEventListener* event) {
    event_ = event;
  }
  MarkdownEventListener* GetMarkdownEventListener() const { return event_; }
  void SetStyle(const MarkdownStyle& style) { style_ = style; }
  const MarkdownStyle& GetStyle() const { return style_; }
  std::pair<float, float> GetInlineViewOrigin(const char* idSelector);
  std::vector<std::pair<std::string, PointF>> GetAllInlineViewOrigin();
  std::vector<std::string> GetAllInlineViewId();

  const MarkdownLink* GetLinkByTouchPosition(PointF point);
  std::vector<MarkdownLink*> GetLinksByViewRect(RectF view_rect);
  std::string GetContentByCharPos(int32_t char_pos_start, int32_t char_pos_end);
  const std::vector<MarkdownLink>& GetLinks() { return links_; }
  const std::vector<std::shared_ptr<MarkdownElement>>& GetParagraphs() const {
    return para_vec_;
  }
  void AddParagraph(std::shared_ptr<MarkdownElement> element) {
    para_vec_.emplace_back(std::move(element));
  }

  std::vector<Range> GetSyntaxSourceRanges(std::string_view tag);
  static MarkdownSyntaxType TagToSyntaxType(std::string_view tag);
  std::vector<std::string> GetAllImageUrl();
  std::string GetImageByTouchPosition(PointF point);
  std::vector<MarkdownImage*> GetImageByViewRect(RectF view_rect);

  void AllowBreakAroundPunctuation(const bool allow) {
    allow_break_around_punctuation = allow;
  }
  bool GetAllowBreakAroundPunctuation() const {
    return allow_break_around_punctuation;
  }
  std::u16string_view GetTruncationText() const { return truncation_text_; }
  std::shared_ptr<tttext::RunDelegate> GetTruncationDelegate() const {
    return truncation_delegate_;
  }
  MarkdownTouchState OnTouchEvent(MarkdownTouchEventType type, PointF point);
  bool TouchPointCanScroll(PointF point, float safe_offset);
  void AddInlineBorder(std::unique_ptr<MarkdownTextAttachment> inline_border) {
    border_attachments_.emplace_back(std::move(inline_border));
  }
  void AddInlineView(MarkdownInlineView inline_view) {
    inline_views_.emplace_back(std::move(inline_view));
  }
  const std::vector<MarkdownInlineView>& GetInlineViews() {
    return inline_views_;
  }
  void AddLink(MarkdownLink&& link) { links_.emplace_back(link); }
  void AddImage(MarkdownImage image) { images_.emplace_back(std::move(image)); }
  const std::vector<MarkdownImage>& GetImages() { return images_; }
  void AddQuoteRange(Range quote) { quote_range_.emplace_back(quote); }

  void ClearForParse();
  void UpdateTruncation(float width);

  void ApplyStyleInRange(const MarkdownBaseStylePart& style, Range range);

  PointF GetElementOrigin(int32_t char_index, bool is_block = false);
  void TrimParagraphSpaces() const;

  int32_t MarkdownOffsetToCharOffset(int32_t markdown_offset);

  const MarkdownTextAttachment* GetTextClickRangeByTouchPosition(
      PointF position);
  int32_t GetCharIndexByTouchPosition(PointF point);
  int32_t GetRegionIndexByCharIndex(int32_t char_index);
  std::vector<std::string> GetVisibleInlineViews(int32_t animation_step,
                                                 bool content_complete);
  Range GetChangedRegionsWhenAnimationUpdated(int32_t from_step,
                                              int32_t to_step);
  Range GetShowedRegions(float top, float bottom);
  Range GetShowedExtraContents(float top, float bottom);
  void InheritState(MarkdownDocument* old_document);

 private:
  void SetShapeRunAltString(uint32_t char_offset, std::string_view content);
  PointF GetTruncationOrigin();

  Range GetCharRangeByViewRect(RectF view_rect);

 private:
  std::string markdown_content_;
  Range content_range_{0, std::numeric_limits<int32_t>::max()};
  float max_width_{std::numeric_limits<float>::max()};
  float max_height_{std::numeric_limits<float>::max()};
  int32_t max_lines_{-1};
  std::vector<std::shared_ptr<MarkdownElement>> para_vec_;
  std::vector<MarkdownLink> links_;
  std::vector<MarkdownImage> images_;
  std::mutex page_lock_;
  std::shared_ptr<MarkdownPage> page_;
  std::vector<MarkdownInlineView> inline_views_;

  std::vector<std::unique_ptr<MarkdownTextAttachment>> border_attachments_;
  std::vector<std::pair<uint32_t, std::string>> shape_run_alt_strings_;

  // TODO(zhouchaoying): temporarily fix quote border, will be removed next
  // commit
  std::vector<Range> quote_range_;
  MarkdownStyle style_{};

  MarkdownResourceLoader* loader_{nullptr};
  MarkdownEventListener* event_{nullptr};

  std::u16string truncation_text_;
  std::shared_ptr<tttext::RunDelegate> truncation_delegate_;

  bool allow_break_around_punctuation = false;

  PointF touch_down_point_{};
  bool touch_down_{false};
  // using index instead of pointer because region may be deleted after relayout
  int32_t touch_down_region_index_{};
  float touch_down_region_origin_scroll_offset_{0};
  MarkdownTouchState touch_state_;

  std::vector<std::pair<Range, Range>> markdown_index_to_char_index_;

  std::vector<ScrollState> inherited_scroll_state_;

  friend class MarkdownLayout;
  friend class MarkdownParserDiscountImpl;
};

}  // namespace markdown
}  // namespace lynx

#endif  // MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_DOCUMENT_H_
