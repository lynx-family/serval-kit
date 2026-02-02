// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <optional>
#include <string_view>

#include "gtest/gtest.h"
#include "markdown/parser/discount/markdown_inline_node.h"
#include "markdown/parser/discount/markdown_inline_parser.h"
namespace lynx::markdown::testing {
struct InlineNodeDescriptor {
  std::string content_;
  MarkdownInlineSyntax syntax_;
  std::optional<std::string> url_;
  std::optional<float> width_;
  std::optional<float> height_;
  std::optional<std::string> tag_;
  std::optional<std::vector<MarkdownHtmlAttribute>> attributes_;
  std::vector<InlineNodeDescriptor> children_;
};

std::string MergeContent(const std::vector<InlineNodeDescriptor>& children) {
  std::string text = "";
  for (auto& child : children) {
    text += child.content_;
  }
  return text;
}

InlineNodeDescriptor Root(std::vector<InlineNodeDescriptor> children) {
  return InlineNodeDescriptor{.content_ = MergeContent(children),
                              .syntax_ = MarkdownInlineSyntax::kNone,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor RawText(std::string text) {
  return InlineNodeDescriptor{
      .content_ = text,
      .syntax_ = MarkdownInlineSyntax::kRawText,
  };
}

InlineNodeDescriptor Italic(std::string symbol,
                            std::vector<InlineNodeDescriptor> children) {
  auto text = symbol + MergeContent(children) + symbol;
  return InlineNodeDescriptor{.content_ = text,
                              .syntax_ = MarkdownInlineSyntax::kItalic,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor Bold(std::string symbol,
                          std::vector<InlineNodeDescriptor> children) {
  auto text = symbol + symbol + MergeContent(children) + symbol + symbol;
  return InlineNodeDescriptor{.content_ = text,
                              .syntax_ = MarkdownInlineSyntax::kBold,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor BoldItalic(std::string symbol,
                                std::vector<InlineNodeDescriptor> children) {
  auto text = symbol + symbol + symbol + MergeContent(children) + symbol +
              symbol + symbol;
  return InlineNodeDescriptor{.content_ = text,
                              .syntax_ = MarkdownInlineSyntax::kBoldItalic,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor InlineCode(int32_t count, std::string content) {
  std::string text;
  for (int i = 0; i < count; i++) {
    text += '`';
  }
  text += content;
  for (int i = 0; i < count; i++) {
    text += '`';
  }
  return InlineNodeDescriptor{.content_ = text,
                              .syntax_ = MarkdownInlineSyntax::kInlineCode,
                              .children_ = {RawText(content)}};
}

InlineNodeDescriptor Image(std::string url,
                           std::vector<InlineNodeDescriptor> children) {
  auto text = "![" + MergeContent(children) + "](" + url + ")";
  return InlineNodeDescriptor{.content_ = text,
                              .syntax_ = MarkdownInlineSyntax::kImg,
                              .url_ = url,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor Image(std::string url, std::string extra,
                           std::vector<InlineNodeDescriptor> children) {
  auto text = "![" + MergeContent(children) + "](" + url + extra + ")";
  return InlineNodeDescriptor{.content_ = text,
                              .syntax_ = MarkdownInlineSyntax::kImg,
                              .url_ = url,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor Image(std::string url, float width, float height,
                           std::vector<InlineNodeDescriptor> children) {
  auto text = "![" + MergeContent(children) + "](" + url +
              " width=" + std::to_string(width) +
              " height=" + std::to_string(height) + ")";
  return InlineNodeDescriptor{.content_ = text,
                              .syntax_ = MarkdownInlineSyntax::kImg,
                              .url_ = url,
                              .width_ = width,
                              .height_ = height,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor Image(std::string url, std::string extra, float width,
                           float height,
                           std::vector<InlineNodeDescriptor> children) {
  auto text = "![" + MergeContent(children) + "](" + url + extra + ")";
  return InlineNodeDescriptor{.content_ = text,
                              .syntax_ = MarkdownInlineSyntax::kImg,
                              .url_ = url,
                              .width_ = width,
                              .height_ = height,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor Link(std::string link,
                          std::vector<InlineNodeDescriptor> children) {
  auto text = "[" + MergeContent(children) + "](" + link + ")";
  return InlineNodeDescriptor{.content_ = text,
                              .syntax_ = MarkdownInlineSyntax::kLink,
                              .url_ = link,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor Link(std::string link, std::string extra,
                          std::vector<InlineNodeDescriptor> children) {
  auto text = "![" + MergeContent(children) + "](" + link + extra + ")";
  return InlineNodeDescriptor{.content_ = text,
                              .syntax_ = MarkdownInlineSyntax::kLink,
                              .url_ = link,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor DoubleSquareBracket(
    std::vector<InlineNodeDescriptor> children) {
  return InlineNodeDescriptor{
      .content_ = "[[" + MergeContent(children) + "]]",
      .syntax_ = MarkdownInlineSyntax::kDoubleSquareBrackets,
      .children_ = std::move(children)};
}

InlineNodeDescriptor DoubleBraces(std::vector<InlineNodeDescriptor> children) {
  return InlineNodeDescriptor{.content_ = "{{" + MergeContent(children) + "}}",
                              .syntax_ = MarkdownInlineSyntax::kDoubleBraces,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor Escape(std::string c) {
  return InlineNodeDescriptor{.content_ = "\\" + c,
                              .syntax_ = MarkdownInlineSyntax::kEscape,
                              .children_ = {RawText(c)}};
}

InlineNodeDescriptor Delete(std::vector<InlineNodeDescriptor> children) {
  return InlineNodeDescriptor{.content_ = "~~" + MergeContent(children) + "~~",
                              .syntax_ = MarkdownInlineSyntax::kDelete,
                              .children_ = std::move(children)};
}

InlineNodeDescriptor HtmlTag(std::string tag,
                             std::vector<InlineNodeDescriptor> children) {
  std::string text =
      "<" + tag + ">" + MergeContent(children) + "</" + tag + ">";
  return InlineNodeDescriptor{
      .content_ = text,
      .syntax_ = MarkdownInlineSyntax::kInlineHtml,
      .tag_ = tag,
      .children_ = std::move(children),
  };
}

InlineNodeDescriptor HtmlTag(std::string tag,
                             std::vector<MarkdownHtmlAttribute> attrs,
                             std::vector<InlineNodeDescriptor> children) {
  std::string text = "<" + tag;
  for (auto& attr : attrs) {
    text += " " + std::string(attr.name_) + "=" + '"' +
            std::string(attr.value_) + '"';
  }
  text += ">" + MergeContent(children) + "</" + tag + ">";
  return InlineNodeDescriptor{
      .content_ = text,
      .syntax_ = MarkdownInlineSyntax::kInlineHtml,
      .tag_ = tag,
      .attributes_ = std::move(attrs),
      .children_ = std::move(children),
  };
}

InlineNodeDescriptor HtmlTagSelfClose(std::string tag) {
  std::string text = "<" + tag + " />";
  return InlineNodeDescriptor{
      .content_ = text,
      .syntax_ = MarkdownInlineSyntax::kInlineHtml,
      .tag_ = tag,
  };
}

void Expect(std::string_view text, const InlineNodeDescriptor& node);
TEST(MarkdownInlineParserTest, Stars) {
  Expect("*italic*normal text",
         Root({Italic("*", {RawText("italic")}), RawText("normal text")}));
  Expect("normal text*italic*",
         Root({RawText("normal text"), Italic("*", {RawText("italic")})}));
  Expect("***bold italic***",
         Root({BoldItalic("*", {RawText("bold italic")})}));
  Expect("normal text*italic*normal text", Root({
                                               RawText("normal text"),
                                               Italic("*",
                                                      {
                                                          RawText("italic"),
                                                      }),
                                               RawText("normal text"),
                                           }));
  Expect("normal text**bold**normal text", Root({
                                               RawText("normal text"),
                                               Bold("*", {RawText("bold")}),
                                               RawText("normal text"),
                                           }));
  Expect("normal text**italic*normal text",
         Root({RawText("normal text*"), Italic("*", {RawText("italic")}),
               RawText("normal text")}));
  Expect("normal text*italic**normal text",
         Root({RawText("normal text"), Italic("*", {RawText("italic")}),
               RawText("*normal text")}));
  Expect(
      "normal text***bold italic*bold**normal text",
      Root({RawText("normal text"),
            Bold("*", {Italic("*", {RawText("bold italic")}), RawText("bold")}),
            RawText("normal text")}));
}

TEST(MarkdownInlineParserTest, Underlines) {
  Expect("___bold italic___",
         Root({BoldItalic("_", {RawText("bold italic")})}));
  Expect("__bold__", Root({Bold("_", {RawText("bold")})}));
  Expect("_italic_", Root({Italic("_", {RawText("italic")})}));
}

TEST(MarkdownInlineParserTest, InlineCode) {
  Expect("`code block`", Root({InlineCode(1, "code block")}));
  Expect("``code block`", Root({RawText("``code block`")}));
  Expect("``code block``", Root({InlineCode(2, "code block")}));
  Expect("``code block```", Root({RawText("``code block```")}));
  Expect("```code block```", Root({InlineCode(3, "code block")}));
  Expect("```code`block````123```", Root({InlineCode(3, "code`block````123")}));
}

TEST(MarkdownInlineParserTest, Image) {
  Expect("test![test](url)",
         Root({RawText("test"), Image("url", {RawText("test")})}));
  Expect("![***1234**56*78](url)",
         Root({Image(
             "url", {Italic("*", {Bold("*", {RawText("1234")}), RawText("56")}),
                     RawText("78")})}));
  Expect(
      "![***1234**56*78](url 1234567)",
      Root({Image("url", " 1234567",
                  {Italic("*", {Bold("*", {RawText("1234")}), RawText("56")}),
                   RawText("78")})}));
  Expect(
      "![***1234**56*78](url width=30 height=40)",
      Root({Image("url", " width=30 height=40", 30, 40,
                  {Italic("*", {Bold("*", {RawText("1234")}), RawText("56")}),
                   RawText("78")})}));
}

TEST(MarkdownInlineParserTest, Link) {
  Expect("[link](https://test.cc?aaa=b&ccc=d)",
         Root({Link("https://test.cc?aaa=b&ccc=d", {RawText("link")})}));
  Expect("[**link**](link)",
         Root({Link("link", {Bold("*", {RawText("link")})})}));
  Expect("[![](image)](link)", Root({Link("link", {Image("image", {})})}));
}

TEST(MarkdownInlineParserTest, DoubleSquareBracket) {
  Expect("[12345]", Root({RawText("[12345]")}));
  Expect("[[12345]]", Root({DoubleSquareBracket({RawText("12345")})}));
  Expect("[[[12345]]]",
         Root({RawText("["), DoubleSquareBracket({RawText("12345")}),
               RawText("]")}));
  Expect("*[[12345*]]",
         Root({Italic("*", {RawText("[[12345")}), RawText("]]")}));
  Expect("[[*12*345]]", Root({DoubleSquareBracket(
                            {Italic("*", {RawText("12")}), RawText("345")})}));
}

TEST(MarkdownInlineParserTest, DoubleSquareBraces) {
  Expect("{12345}", Root({RawText("{12345}")}));
  Expect("{{12345}}", Root({DoubleBraces({RawText("12345")})}));
}

TEST(MarkdownInlineParserTest, Delete) {
  Expect("~~12345~~", Root({Delete({RawText("12345")})}));
}

TEST(MarkdownInlineParserTest, Escape) {
  Expect("**123\\**",
         Root({RawText("*"), Italic("*", {RawText("123"), Escape("*")})}));
  Expect("**123\\\\**", Root({Bold("*", {RawText("123"), Escape("\\")})}));
}

TEST(MarkdownInlineParserTest, InlineHtml) {
  Expect("<tag>1234</tag>", Root({HtmlTag("tag", {RawText("1234")})}));
  Expect("<tag>**1234**</tag>",
         Root({HtmlTag("tag", {Bold("*", {RawText("1234")})})}));
  Expect("1234<br />5678",
         Root({RawText("1234"), HtmlTagSelfClose("br"), RawText("5678")}));
  Expect(R"(<span class="cls">1234</span>)",
         Root({HtmlTag("span", {{"class", "cls"}}, {RawText("1234")})}));
  Expect(R"(<span class="red">123<span class="bold">456</span>789</span>)",
         Root({HtmlTag("span", {{"class", "red"}},
                       {RawText("123"),
                        HtmlTag("span", {{"class", "bold"}}, {RawText("456")}),
                        RawText("789")})}));
}

void ExpectNode(MarkdownInlineNode* node, const InlineNodeDescriptor& desc) {
  EXPECT_EQ(node->GetText(), desc.content_);
  EXPECT_EQ(node->GetSyntax(), desc.syntax_);
  if (node->GetSyntax() == MarkdownInlineSyntax::kImg) {
    auto* img = reinterpret_cast<MarkdownImageNode*>(node);
    if (desc.url_.has_value()) {
      EXPECT_EQ(img->GetUrl(), desc.url_.value());
    }
    if (desc.width_.has_value()) {
      EXPECT_EQ(img->GetWidth(), desc.width_.value());
    }
    if (desc.height_.has_value()) {
      EXPECT_EQ(img->GetHeight(), desc.height_.value());
    }
  } else if (node->GetSyntax() == MarkdownInlineSyntax::kLink) {
    auto* link = reinterpret_cast<MarkdownLinkNode*>(node);
    if (desc.url_.has_value()) {
      EXPECT_EQ(link->GetLink(), desc.url_);
    }
  } else if (node->GetSyntax() == MarkdownInlineSyntax::kInlineHtml) {
    auto* tag = reinterpret_cast<MarkdownInlineHtmlTag*>(node);
    if (desc.tag_.has_value()) {
      EXPECT_EQ(tag->GetTag(), desc.tag_.value());
    }
    if (desc.attributes_.has_value()) {
      auto& attrs = desc.attributes_.value();
      EXPECT_EQ(attrs.size(), tag->GetAttributes().size());
      for (uint32_t i = 0; i < attrs.size(); i++) {
        EXPECT_EQ(attrs[i].name_, tag->GetAttributes()[i].name_);
        EXPECT_EQ(attrs[i].value_, tag->GetAttributes()[i].value_);
      }
    }
  }
  EXPECT_EQ(node->Children().size(), desc.children_.size());
  for (size_t i = 0; i < desc.children_.size(); ++i) {
    ExpectNode(node->Children()[i].get(), desc.children_[i]);
  }
}

void Expect(std::string_view text, const InlineNodeDescriptor& node) {
  auto result = MarkdownInlineSyntaxParser::Parse(text);
  ExpectNode(result.get(), node);
}
}  // namespace lynx::markdown::testing
