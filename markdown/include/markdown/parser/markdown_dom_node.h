// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_DOM_NODE_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_DOM_NODE_
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "markdown/utils/markdown_node.h"

#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_definition.h"
namespace lynx::markdown {
enum class MarkdownDomType : uint8_t {
  kUndefined,
  kParagraph,
  kHeader,
  kSource,
  kCodeBlock,
  kQuote,
  kOrderedList,
  kUnorderedList,
  kListItem,
  kSplit,
  kTable,
  kTableRow,
  kTableCell,
  kPlaceHolder,
  kRawText,
  kBold,
  kItalic,
  kBoldItalic,
  kDelete,
  kInlineCode,
  kImage,
  kLink,
  kDoubleBracket,
  kDoubleBraces,
  kInlineHtml,
  kEscape,
  kHtmlEntity,
  kBreakLine,
};
class MarkdownDomNode : public MarkdownNode {
 public:
  explicit MarkdownDomNode(const MarkdownDomType type) : type_(type) {}
  ~MarkdownDomNode() override = default;
  Range GetSourceRange() const { return source_range_; }
  void SetSourceRange(Range range) { source_range_ = range; }

 protected:
  MarkdownDomType type_;
  Range source_range_;
};
class MarkdownHeader final : public MarkdownDomNode {
 public:
  explicit MarkdownHeader() : MarkdownDomNode(MarkdownDomType::kHeader) {}
  ~MarkdownHeader() override = default;
  int32_t GetHN() const { return hn_; }
  void SetHN(const int32_t hn) { hn_ = hn; }

 protected:
  int32_t hn_{0};
};
class MarkdownCodeBlock final : public MarkdownDomNode {
 public:
  explicit MarkdownCodeBlock() : MarkdownDomNode(MarkdownDomType::kCodeBlock) {}
  ~MarkdownCodeBlock() override = default;
  const std::string& GetLanguage() { return language_; }
  void SetLanguage(std::string_view language) { language_ = language; }

 protected:
  std::string language_;
};
class MarkdownList final : public MarkdownDomNode {
 public:
  explicit MarkdownList(MarkdownDomType type) : MarkdownDomNode(type) {}
  ~MarkdownList() override = default;
  int32_t GetStart() const { return start_; }
  void SetStart(const int32_t start) { start_ = start; }
  char GetDelimiter() const { return delimiter_; }
  void SetDelimiter(char delimiter) { delimiter_ = delimiter; }
  void SetExtraLevel(int32_t extra_level) { extra_level_ = extra_level; }
  int32_t GetExtraLevel() const { return extra_level_; }
  bool IsChecked() const { return checked_; }
  void SetChecked(bool checked) { checked_ = checked; }

 protected:
  int32_t start_{0};
  char delimiter_{'-'};
  int32_t extra_level_{0};
  bool checked_{false};
};
class MarkdownTable final : public MarkdownDomNode {
 public:
  explicit MarkdownTable() : MarkdownDomNode(MarkdownDomType::kTable) {}
  ~MarkdownTable() override = default;
  const std::vector<MarkdownTextAlign>& GetAligns() const { return aligns_; }
  void SetAligns(std::vector<MarkdownTextAlign> aligns) {
    aligns_ = std::move(aligns);
  }

 protected:
  std::vector<MarkdownTextAlign> aligns_;
};
class MarkdownLink final : public MarkdownDomNode {
 public:
  explicit MarkdownLink() : MarkdownDomNode(MarkdownDomType::kLink) {}
  ~MarkdownLink() override = default;
  const std::string& GetUrl() const { return url_; }
  void SetUrl(std::string_view url) { url_ = url; }
  const std::string& GetTitle() const { return title_; }
  void SetTitle(std::string_view title) { title_ = title; }

 protected:
  std::string url_;
  std::string title_;
};
class MarkdownImage final : public MarkdownDomNode {
 public:
  explicit MarkdownImage() : MarkdownDomNode(MarkdownDomType::kImage) {}
  ~MarkdownImage() override = default;
  std::string_view GetUrl() const { return url_; }
  void SetUrl(std::string_view url) { url_ = url; }
  float GetWidth() const { return width_; }
  void SetWidth(float width) { width_ = width; }
  float GetHeight() const { return height_; }
  void SetHeight(float height) { height_ = height; }
  void SetAltText(std::string_view text) { alt_text_ = text; }
  std::string_view GetAltText() const { return alt_text_; }
  void SetCaption(std::string_view caption) { caption_ = caption; }
  std::string_view GetCaption() const { return caption_; }

 protected:
  std::string url_;
  std::string alt_text_;
  std::string caption_;
  float width_{0};
  float height_{0};
};
class MarkdownRawText final : public MarkdownDomNode {
 public:
  explicit MarkdownRawText(MarkdownDomType type) : MarkdownDomNode(type) {}
  ~MarkdownRawText() override = default;
  const std::string& GetText() const { return text_; }
  void SetText(std::string_view text) { text_ = text; }

 protected:
  std::string text_;
};
struct MarkdownHtmlAttribute {
  std::string name;
  std::string value;
};
class MarkdownHtmlNode final : public MarkdownDomNode {
 public:
  MarkdownHtmlNode() : MarkdownDomNode(MarkdownDomType::kInlineHtml) {}
  ~MarkdownHtmlNode() override = default;
  const std::string& GetTag() const { return tag_; }
  void SetTag(const std::string_view tag) { tag_ = tag; }
  void AddAttribute(std::string_view name, std::string_view value) {
    attributes_.emplace_back(
        MarkdownHtmlAttribute{std::string(name), std::string(value)});
  }
  const std::vector<MarkdownHtmlAttribute>& GetAttributes() const {
    return attributes_;
  }
  void SetAttributes(std::vector<MarkdownHtmlAttribute>&& attributes) {
    attributes_ = attributes;
  }
  const std::string& GetClass() const {
    for (const auto& [name, value] : attributes_) {
      if (name == "class") {
        return value;
      }
    }
    return "";
  }

 protected:
  std::string tag_;
  std::vector<MarkdownHtmlAttribute> attributes_;
};
class MarkdownPlaceHolder final : public MarkdownDomNode {
 public:
  MarkdownPlaceHolder() : MarkdownDomNode(MarkdownDomType::kPlaceHolder) {}
  ~MarkdownPlaceHolder() override = default;
  void SetData(void* data) { ud_ = data; }
  void* GetData() const { return ud_; }

 protected:
  void* ud_{nullptr};
};
}  // namespace lynx::markdown
#endif  //MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_DOM_NODE_
