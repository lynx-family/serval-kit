// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_INLINE_NODE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_INLINE_NODE_H_
#include <cstdint>
#include <string_view>
#include <vector>
namespace lynx::markdown {
enum class MarkdownInlineSyntax : uint8_t {
  kNone,
  kBold,
  kItalic,
  kBoldItalic,
  kDelete,
  kInlineCode,
  kImg,
  kLink,
  kDoubleSquareBrackets,
  kDoubleBraces,
  kInlineHtml,
  kEscape,
  kRawText,
  kHtmlEntity,
  kBreakLine,
};
class MarkdownInlineNode {
 public:
  MarkdownInlineNode() : syntax_(MarkdownInlineSyntax::kNone) {}
  MarkdownInlineNode(MarkdownInlineSyntax syntax, std::string_view text)
      : syntax_(syntax), raw_text_(text) {}
  virtual ~MarkdownInlineNode() = default;
  MarkdownInlineSyntax GetSyntax() const { return syntax_; }
  std::string_view GetText() const { return raw_text_; }
  void SetSyntax(MarkdownInlineSyntax syntax) { syntax_ = syntax; }
  void SetText(std::string_view text) { raw_text_ = text; }
  void AppendChild(std::unique_ptr<MarkdownInlineNode> child) {
    children_.emplace_back(std::move(child));
  }
  const std::vector<std::unique_ptr<MarkdownInlineNode>>& Children() {
    return children_;
  }

 private:
  MarkdownInlineSyntax syntax_;
  std::string_view raw_text_;
  std::vector<std::unique_ptr<MarkdownInlineNode>> children_;
};
class MarkdownRawTextNode : public MarkdownInlineNode {
 public:
  explicit MarkdownRawTextNode(std::string_view text)
      : MarkdownInlineNode(MarkdownInlineSyntax::kRawText, text) {}
  ~MarkdownRawTextNode() override = default;
};
class MarkdownHtmlEntityNode : public MarkdownInlineNode {
 public:
  explicit MarkdownHtmlEntityNode(std::string_view text, std::string&& entity)
      : MarkdownInlineNode(MarkdownInlineSyntax::kHtmlEntity, text),
        entity_(entity) {}
  ~MarkdownHtmlEntityNode() override = default;
  const std::string& GetEntity() const { return entity_; }

 private:
  std::string entity_;
};
class MarkdownBreakLineNode : public MarkdownInlineNode {
 public:
  explicit MarkdownBreakLineNode(std::string_view text)
      : MarkdownInlineNode(MarkdownInlineSyntax::kBreakLine, text) {}
  ~MarkdownBreakLineNode() override = default;
};
class MarkdownLinkNode : public MarkdownInlineNode {
 public:
  MarkdownLinkNode() { SetSyntax(MarkdownInlineSyntax::kLink); }
  MarkdownLinkNode(std::string_view text, std::string_view link)
      : MarkdownInlineNode(MarkdownInlineSyntax::kLink, text), link_(link) {}
  ~MarkdownLinkNode() override = default;
  std::string_view GetLink() const { return link_; }
  void SetLink(std::string_view link) { link_ = link; }
  std::string_view GetDescription() const { return description_; }
  void SetDescription(std::string_view description) {
    description_ = description;
  }

 private:
  std::string_view link_;
  std::string_view description_;
};
class MarkdownImageNode : public MarkdownInlineNode {
 public:
  MarkdownImageNode() { SetSyntax(MarkdownInlineSyntax::kImg); }
  MarkdownImageNode(std::string_view text, std::string_view url)
      : MarkdownInlineNode(MarkdownInlineSyntax::kImg, text), url_(url) {}
  ~MarkdownImageNode() override = default;
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

 private:
  std::string_view url_;
  std::string_view alt_text_;
  std::string_view caption_;
  float width_{-1};
  float height_{-1};
};
struct MarkdownHtmlAttribute {
  std::string_view name_;
  std::string_view value_;
};
class MarkdownInlineHtmlTag : public MarkdownInlineNode {
 public:
  MarkdownInlineHtmlTag() { SetSyntax(MarkdownInlineSyntax::kInlineHtml); }
  std::string_view GetTag() const { return tag_; }
  void SetTag(const std::string_view tag) { tag_ = tag; }
  void AddAttribute(std::string_view name, std::string_view value) {
    attributes_.emplace_back(MarkdownHtmlAttribute{name, value});
  }
  const std::vector<MarkdownHtmlAttribute>& GetAttributes() const {
    return attributes_;
  }
  void SetAttributes(std::vector<MarkdownHtmlAttribute>&& attributes) {
    attributes_ = attributes;
  }
  std::string_view GetClass() const {
    for (auto& attr : attributes_) {
      if (attr.name_ == "class") {
        return attr.value_;
      }
    }
    return "";
  }

 private:
  std::string_view tag_;
  std::vector<MarkdownHtmlAttribute> attributes_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_INLINE_NODE_H_
