// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/view/markdown_view_measurer.h"

#include "markdown/layout/markdown_layout.h"
#include "markdown/parser/impl/markdown_parser_impl.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/utils/markdown_float_comparison.h"
#include "markdown/view/markdown_platform_view.h"

namespace lynx::markdown {

MarkdownViewMeasurer::MarkdownViewMeasurer(
    MarkdownResourceLoader* resource_loader)
    : resource_loader_(resource_loader) {
  style_ = MarkdownStyleReader::ReadStyle(ValueMap{}, resource_loader_);
}

void MarkdownViewMeasurer::SetResourceLoader(
    MarkdownResourceLoader* resource_loader) {
  resource_loader_ = resource_loader;
  NeedsMeasure();
}
void MarkdownViewMeasurer::SetEventListener(
    MarkdownEventListener* event_listener) {
  event_listener_ = event_listener;
}

void MarkdownViewMeasurer::SetContent(std::string_view content) {
  content_ = content;
  if (document_ != nullptr) {
    document_->SetMarkdownContent(content_);
  }
  NeedsMeasure();
}

void MarkdownViewMeasurer::SetContentID(std::string_view id) {
  content_id_ = id;
}

void MarkdownViewMeasurer::SetContentComplete(bool complete) {
  content_complete_ = complete;
}

void MarkdownViewMeasurer::SetContentRange(Range range) {
  content_start_ = range.start_;
  content_end_ = range.end_;
  if (document_ != nullptr) {
    document_->SetMarkdownContentRange(range);
  }
  NeedsMeasure();
}

void MarkdownViewMeasurer::SetParserType(std::string_view parser_type,
                                         void* parser_ud) {
  parser_type_ = parser_type;
  parser_ud_ = parser_ud;
  NeedsMeasure();
}

void MarkdownViewMeasurer::SetSourceType(SourceType type) {
  source_type_ = type;
  NeedsMeasure();
}

void MarkdownViewMeasurer::SetStyle(const ValueMap& style_map) {
  style_ = MarkdownStyleReader::ReadStyle(style_map, resource_loader_);
  NeedsMeasure();
}

void MarkdownViewMeasurer::SetStyle(const MarkdownStyle& style) {
  style_ = style;
  NeedsMeasure();
}

void MarkdownViewMeasurer::ApplyStyleInRange(const ValueMap& style_map,
                                             int32_t char_start,
                                             int32_t char_end) {
  if (document_ == nullptr) {
    return;
  }
  const auto base_style =
      MarkdownStyleReader::ReadBaseStyle(style_map, resource_loader_);
  document_->ApplyStyleInRange(base_style, {char_start, char_end});
}

void MarkdownViewMeasurer::SetTextMaxLines(int32_t max_lines) {
  text_max_lines_ = max_lines;
  NeedsMeasure();
}

void MarkdownViewMeasurer::SetEnableBreakAroundPunctuation(bool allow) {
  break_around_punctuation_ = allow;
  NeedsMeasure();
}
void MarkdownViewMeasurer::SetTrimLastParagraphSpace(bool trim) {
  trim_last_paragraph_space_ = trim;
  NeedsMeasure();
}

void MarkdownViewMeasurer::SetPaddings(Paddings paddings) {
  paddings_ = paddings;
  NeedsMeasure();
}

void MarkdownViewMeasurer::InitialDocument() {
  auto new_document = std::make_shared<MarkdownDocument>();
  new_document->InheritState(document_.get());
  document_ = std::move(new_document);
  document_->SetMarkdownContent(content_);
  document_->SetMarkdownContentRange({content_start_, content_end_});
  document_->SetMaxLines(text_max_lines_);
  document_->SetStyle(style_);
  document_->AllowBreakAroundPunctuation(break_around_punctuation_);
  document_->SetResourceLoader(resource_loader_);
  document_->SetMarkdownEventListener(event_listener_);
}

SizeF MarkdownViewMeasurer::Measure(MeasureSpec spec) {
  did_layout_in_last_measure_ = false;

  if (spec.width_mode_ == tttext::LayoutMode::kIndefinite) {
    spec.width_ = MeasureSpec::LAYOUT_MAX_SIZE;
  }
  if (spec.height_mode_ == tttext::LayoutMode::kIndefinite) {
    spec.height_ = MeasureSpec::LAYOUT_MAX_SIZE;
  }

  if (FloatsNotEqual(spec.width_, last_measure_spec_.width_)) {
    needs_measure_ = true;
  }
  if (FloatsNotEqual(spec.height_, last_measure_spec_.height_)) {
    needs_measure_ = true;
  }
  last_measure_spec_ = spec;

  if (needs_measure_) {
    InitialDocument();
    document_->SetMaxSize(spec.width_, spec.height_);
    document_->ClearForParse();
    if (source_type_ == SourceType::kMarkdown) {
      MarkdownParserImpl::ParseMarkdown(parser_type_, document_.get(),
                                        parser_ud_);
    } else {
      MarkdownParserImpl::ParsePlainText(document_.get());
    }
    if (trim_last_paragraph_space_) {
      document_->TrimParagraphSpaces();
    }
    if (event_listener_) {
      event_listener_->OnParseEnd();
    }
    MarkdownLayout layout(document_.get());
    layout.SetPaddings(paddings_);
    layout.Layout(spec.width_, spec.height_,
                  text_max_lines_ > 0 ? text_max_lines_ : -1);
    auto page = document_->GetPage();
    if (page != nullptr) {
      measured_width_ = page->GetLayoutWidth();
      measured_height_ = page->GetLayoutHeight();
    } else {
      measured_width_ = 0;
      measured_height_ = 0;
    }
    needs_measure_ = false;
    did_layout_in_last_measure_ = true;
  }

  return {measured_width_, measured_height_};
}

void MarkdownViewMeasurer::Align() {
  if (document_ == nullptr) {
    return;
  }
  for (auto& inline_view : document_->GetInlineViews()) {
    if (inline_view.view_ == nullptr) {
      continue;
    }
    auto pos = document_->GetElementOrigin(inline_view.char_index_,
                                           inline_view.is_block_view_);
    static_cast<MarkdownDrawable*>(inline_view.view_)->Align(pos.x_, pos.y_);
  }
  for (auto& image : document_->GetImages()) {
    if (image.image_ == nullptr) {
      continue;
    }
    auto pos = document_->GetElementOrigin(image.char_index_);
    static_cast<MarkdownDrawable*>(image.image_)->Align(pos.x_, pos.y_);
  }
}

std::shared_ptr<MarkdownDocument> MarkdownViewMeasurer::GetDocument() {
  return document_;
}
void MarkdownViewMeasurer::NeedsMeasure() {
  needs_measure_ = true;
}

}  // namespace lynx::markdown
