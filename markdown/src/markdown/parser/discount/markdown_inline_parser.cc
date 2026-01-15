// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/parser/discount/markdown_inline_parser.h"

#include "markdown/parser/discount/markdown_inline_node.h"
namespace lynx::markdown {
enum class TokenType : uint8_t {
  kRawText = 0,
  kKeyword = 1,
  kKeywordStars,
  kKeywordUnderlines,
  kKeywordWavy,
  kKeywordBackticks,

  kKeywordExclamation,

  kKeywordLeftRoundBrackets,
  kKeywordRightRoundBrackets,
  kKeywordLeftSquareBrackets,
  kKeywordRightSquareBrackets,
  kKeywordLeftBraces,
  kKeywordRightBraces,

  kKeywordHtmlOpenStart,
  kKeywordHtmlOpen,
  kKeywordHtmlCloseStart,
  kKeywordHtmlClose,
  kKeywordHtmlEnd,

  kNode = 0x80
};

struct Token {
  TokenType type_;
  std::string_view content_;
  std::unique_ptr<MarkdownInlineNode> node_;
  std::string_view tag_;
  std::vector<MarkdownHtmlAttribute> attributes_;
};

int32_t SameCharCount(const char* current, const char* end, char c) {
  auto next = current + 1;
  while (next != end && *next == c) {
    next++;
  }
  return next - current;
}

bool IsValidHtmlChar(const char c) {
  if (static_cast<uint8_t>(c) > 0x80)
    return true;
  if (c >= 'a' && c <= 'z')
    return true;
  if (c >= 'A' && c <= 'Z')
    return true;
  if (c >= '0' && c <= '9')
    return true;
  if (c == '_')
    return true;
  if (c == '-')
    return true;
  return false;
}

class MarkdownHtmlParser {
 public:
  Token ParseHtmlTag(const char* start, const char* end) {
    start_ = start;
    processing_ = start + 1;
    end_ = end;
    SkipWhiteSpace();
    content_begin_ = processing_;
    while (processing_ < end) {
      if (*processing_ == '>') {
        TagEnd(false);
        break;
      } else if (*processing_ == '/' && processing_ + 1 != end &&
                 *(processing_ + 1) == '>') {
        TagEnd(true);
        break;
      } else if (*processing_ == ' ') {
        ParseStateEnd();
        // skip space
        SkipWhiteSpace();
        content_begin_ = processing_;
      } else if (*processing_ == '=') {
        if (!EqualMeet()) {
          break;
        }
      } else if (*processing_ == '"' || *processing_ == '\'') {
        if (!QuoteMeet()) {
          break;
        }
      } else if (IsValidHtmlChar(*processing_)) {
        WaitStateEnd();
        processing_++;
      } else {
        // invalid char
        state_ = State::kParseError;
        break;
      }
    }
    if (state_ == State::kParseEnd) {
      return MakeToken();
    } else {
      return {};
    }
  }

 private:
  enum class State : uint8_t {
    kParseEnd,
    kWaitForTag,
    kParseTag,
    kWaitForAttributeName,
    kParseAttributeName,
    kWaitForEqual,
    kWaitForAttributeValue,
    kParseAttributeValue,
    kParseError,
  };
  void SkipWhiteSpace() {
    if (*processing_ == ' ') {
      processing_ += SameCharCount(processing_, end_, ' ');
    }
  }
  void ParseStateEnd() {
    if (state_ == State::kParseTag) {
      tag_ = std::string_view(content_begin_, processing_ - content_begin_);
      state_ = State::kWaitForAttributeName;
    } else if (state_ == State::kParseAttributeName) {
      name_ = std::string_view(content_begin_, processing_ - content_begin_);
      state_ = State::kWaitForEqual;
    } else if (state_ == State::kParseAttributeValue) {
      auto value =
          std::string_view(content_begin_, processing_ - content_begin_);
      attributes_.emplace_back(
          MarkdownHtmlAttribute{.name_ = name_, .value_ = value});
      state_ = State::kWaitForAttributeName;
    }
  }
  void WaitStateEnd() {
    if (state_ == State::kWaitForTag) {
      state_ = State::kParseTag;
    } else if (state_ == State::kWaitForAttributeName) {
      state_ = State::kParseAttributeName;
    } else if (state_ == State::kWaitForAttributeValue) {
      state_ = State::kParseAttributeValue;
    }
  }
  void TagEnd(const bool force_self_close) {
    ParseStateEnd();
    if (tag_.empty()) {
      state_ = State::kParseError;
    } else {
      // open tag
      state_ = State::kParseEnd;
      processing_++;
      if (force_self_close) {
        processing_++;
        self_close_ = true;
      }
      if (tag_ == "br") {
        self_close_ = true;
      }
    }
  }
  bool EqualMeet() {
    if (state_ == State::kWaitForEqual) {
      state_ = State::kWaitForAttributeValue;
      processing_++;
    } else if (state_ == State::kParseAttributeName) {
      name_ = std::string_view(content_begin_, processing_ - content_begin_);
      state_ = State::kWaitForAttributeValue;
      processing_++;
    } else {
      state_ = State::kParseError;
      return false;
    }
    return true;
  }
  bool QuoteMeet() {
    if (state_ == State::kWaitForAttributeValue) {
      // start a quoted attribute value
      std::string_view rest(processing_, end_ - processing_);
      auto quote_end = rest.find(*processing_, 1);
      if (quote_end != std::string_view::npos) {
        auto value = rest.substr(1, quote_end - 1);
        attributes_.emplace_back(
            MarkdownHtmlAttribute{.name_ = name_, .value_ = value});
        state_ = State::kWaitForAttributeName;
        processing_ += quote_end + 1;
        content_begin_ = processing_;
        return true;
      }
    }
    state_ = State::kParseError;
    return false;
  }
  Token MakeToken() {
    if (self_close_) {
      auto self_close_node = std::make_unique<MarkdownInlineHtmlTag>();
      self_close_node->SetText(std::string_view(start_, processing_ - start_));
      self_close_node->SetTag(tag_);
      self_close_node->SetAttributes(std::move(attributes_));
      return Token{
          .type_ = TokenType::kNode,
          .content_ = std::string_view(start_, processing_ - start_),
          .node_ = std::move(self_close_node),
      };
    } else {
      return Token{.type_ = TokenType::kKeywordHtmlOpen,
                   .content_ = std::string_view(start_, processing_ - start_),
                   .tag_ = tag_,
                   .attributes_ = std::move(attributes_)};
    }
  }

  const char* start_{nullptr};
  const char* processing_{nullptr};
  const char* end_{nullptr};
  std::string_view tag_;
  const char* content_begin_{nullptr};
  std::string_view name_;
  std::vector<MarkdownHtmlAttribute> attributes_;
  State state_ = State::kWaitForTag;
  bool self_close_ = false;
};

class MarkdownInlineSyntaxParserImpl {
 public:
  std::unique_ptr<MarkdownInlineNode> Parse(const std::string_view content) {
    auto* processing = content.data();
    auto* end = content.data() + content.length();
    while (processing != end) {
      if (*processing == '*') {
        const int32_t count = SameCharCount(processing, end, '*');
        PushStars(std::string_view(processing, count));
        processing += count;
      } else if (*processing == '_') {
        const int32_t count = SameCharCount(processing, end, '_');
        PushUnderlines(std::string_view(processing, count));
        processing += count;
      } else if (*processing == '`') {
        const int32_t count = SameCharCount(processing, end, '`');
        const auto inline_code_end = FindInlineCodeEnd(processing, end, count);
        if (inline_code_end > 0) {
          PushInlineCode(std::string_view(processing, inline_code_end + count),
                         count);
          processing += inline_code_end + count;
        } else {
          PushRawText(std::string_view(processing, count));
          processing += count;
        }
      } else if (*processing == '\\') {
        if (processing + 1 != end) {
          PushBackSlash(std::string_view(processing, 2));
          processing += 2;
        } else {
          PushRawText(std::string_view(processing, 1));
          processing += 1;
        }
      } else if (*processing == '!') {
        PushExclamation(std::string_view(processing, 1));
        processing += 1;
      } else if (*processing == '[') {
        const int32_t count = SameCharCount(processing, end, '[');
        PushSquareBracketsStart(std::string_view(processing, count));
        processing += count;
      } else if (*processing == '(') {
        PushRoundBracketStart(std::string_view(processing, 1));
        processing++;
      } else if (*processing == ')') {
        PushRoundBracketEnd(std::string_view(processing, 1));
        processing++;
      } else if (*processing == '{') {
        if (processing + 1 != end && *(processing + 1) == '{') {
          PushBracesStart(std::string_view(processing, 2));
          processing += 2;
        } else {
          PushRawText(std::string_view(processing, 1));
          processing++;
        }
      } else if (*processing == '}') {
        if (processing + 1 != end && *(processing + 1) == '}') {
          PushBracesEnd(std::string_view(processing, 2));
          processing += 2;
        } else {
          PushRawText(std::string_view(processing, 1));
          processing++;
        }
      } else if (*processing == ']') {
        const int32_t count = SameCharCount(processing, end, ']');
        PushSquareBracketsEnd(std::string_view(processing, count));
        processing += count;
      } else if (*processing == '~') {
        const int32_t count = SameCharCount(processing, end, '~');
        PushWavy(std::string_view(processing, count));
        processing += count;
      } else if (*processing == '<') {
        if (*(processing + 1) == '/') {
          auto* tag_end = processing + 2;
          while (*tag_end != '>' && tag_end != end &&
                 IsValidHtmlChar(*tag_end)) {
            tag_end++;
          }
          if (*tag_end == '>') {
            PushHtmlTagClose(
                std::string_view(processing, tag_end - processing + 1),
                std::string_view(processing + 2, tag_end - processing - 2));
            processing = tag_end + 1;
          } else {
            PushRawText(std::string_view{processing, 1});
            processing++;
          }
        } else {
          MarkdownHtmlParser html_parser;
          auto html_tag_open = html_parser.ParseHtmlTag(processing, end);
          if (!html_tag_open.content_.empty()) {
            processing += html_tag_open.content_.length();
            PushHtmlTagOpen(std::move(html_tag_open));
          } else {
            PushRawText(std::string_view(processing, 1));
            processing++;
          }
        }
      } else {
        PushRawText(std::string_view(processing, 1));
        processing++;
      }
    }
    auto result = std::make_unique<MarkdownInlineNode>();
    result->SetText(content);
    MergePiecesToNode(0, tokens_.size(), result.get());
    return result;
  }

 private:
  static int32_t FindInlineCodeEnd(const char* start, const char* end,
                                   int32_t count) {
    auto p = start + count;
    while (p != end) {
      if (*p == '`') {
        const auto c = SameCharCount(p, end, '`');
        if (c == count) {
          return p - start;
        } else {
          p += c;
        }
      } else {
        ++p;
      }
    }
    return -1;
  }

  static std::string_view MergeRawText(std::string_view left,
                                       std::string_view right) {
    return std::string_view(left.data(),
                            right.data() + right.length() - left.data());
  }

  void MergePiecesToNode(const int32_t start, const int32_t end,
                         MarkdownInlineNode* node) {
    std::string_view merged_text;
    for (int32_t i = start; i < end; i++) {
      if (tokens_[i].type_ == TokenType::kRawText ||
          (tokens_[i].type_ > TokenType::kKeyword &&
           tokens_[i].type_ < TokenType::kNode)) {
        if (merged_text.empty()) {
          merged_text = tokens_[i].content_;
        } else {
          merged_text = MergeRawText(merged_text, tokens_[i].content_);
        }
      } else if (tokens_[i].type_ == TokenType::kNode) {
        if (!merged_text.empty()) {
          node->AppendChild(std::make_unique<MarkdownRawTextNode>(merged_text));
          merged_text = "";
        }
        node->AppendChild(std::move(tokens_[i].node_));
      }
    }
    if (!merged_text.empty()) {
      node->AppendChild(std::make_unique<MarkdownRawTextNode>(merged_text));
    }
  }

  static std::pair<MarkdownInlineSyntax, int32_t> StarCountToNode(
      int32_t count) {
    switch (count) {
      case 0:
        return {MarkdownInlineSyntax::kNone, 0};
      case 1:
        return {MarkdownInlineSyntax::kItalic, 1};
      case 2:
        return {MarkdownInlineSyntax::kBold, 2};
      default:
        return {MarkdownInlineSyntax::kBoldItalic, 3};
    }
  }

  static std::pair<MarkdownInlineSyntax, int32_t> SquareBracketToNode(
      int32_t count) {
    if (count >= 2) {
      return {MarkdownInlineSyntax::kDoubleSquareBrackets, 2};
    } else {
      return {MarkdownInlineSyntax::kNone, 0};
    }
  }

  static std::pair<MarkdownInlineSyntax, int32_t> BracesToNode(int32_t count) {
    if (count >= 2) {
      return {MarkdownInlineSyntax::kDoubleBraces, 2};
    }
    return {MarkdownInlineSyntax::kNone, 0};
  }

  static std::pair<MarkdownInlineSyntax, int32_t> WavyToNode(int32_t count) {
    if (count >= 2) {
      return {MarkdownInlineSyntax::kDelete, 2};
    }
    return {MarkdownInlineSyntax::kNone, 0};
  }

  template <TokenType match_type, TokenType current_type, auto Generate>
  void MatchSymmetryKeyword(std::string_view stars) {
    int32_t rest_stars_count = stars.size();
    for (int32_t i = tokens_.size() - 1; i >= 0; i--) {
      if (tokens_[i].type_ == match_type) {
        auto piece_start_content = tokens_[i].content_;
        int32_t piece_star_count = tokens_[i].content_.size();
        int32_t keyword_count = std::min(piece_star_count, rest_stars_count);
        auto [node_syntax, eat] = Generate(keyword_count);
        if (node_syntax == MarkdownInlineSyntax::kNone)
          continue;
        auto new_node = std::make_unique<MarkdownInlineNode>();
        new_node->SetSyntax(node_syntax);
        keyword_count = eat;
        new_node->SetText(MergeRawText(
            piece_start_content.substr(piece_star_count - keyword_count),
            stars.substr(0, keyword_count)));
        MergePiecesToNode(i + 1, tokens_.size(), new_node.get());
        if (eat == piece_star_count) {
          tokens_.resize(i);
        } else {
          tokens_.resize(i + 1);
          tokens_[i].content_ =
              tokens_[i].content_.substr(0, piece_star_count - keyword_count);
        }
        tokens_.emplace_back(
            Token{.type_ = TokenType::kNode, .node_ = std::move(new_node)});
        if (eat == rest_stars_count) {
          rest_stars_count = 0;
          break;
        }
        stars = stars.substr(keyword_count);
        rest_stars_count = stars.size();
      }
    }
    if (rest_stars_count > 0) {
      if (current_type == TokenType::kRawText) {
        PushRawText(stars);
      } else {
        tokens_.emplace_back(
            Token{.type_ = current_type, .content_ = std::move(stars)});
      }
    }
  }

  void PushStars(std::string_view stars) {
    MatchSymmetryKeyword<TokenType::kKeywordStars, TokenType::kKeywordStars,
                         StarCountToNode>(stars);
  }

  void PushUnderlines(std::string_view underlines) {
    MatchSymmetryKeyword<TokenType::kKeywordUnderlines,
                         TokenType::kKeywordUnderlines, StarCountToNode>(
        underlines);
  }

  void PushWavy(std::string_view wavy) {
    MatchSymmetryKeyword<TokenType::kKeywordWavy, TokenType::kKeywordWavy,
                         WavyToNode>(wavy);
  }

  void PushBackSlash(std::string_view backslash) {
    auto node = std::make_unique<MarkdownInlineNode>(
        MarkdownInlineSyntax::kEscape, backslash);
    node->AppendChild(
        std::make_unique<MarkdownRawTextNode>(backslash.substr(1)));
    tokens_.emplace_back(
        Token{.type_ = TokenType::kNode, .node_ = std::move(node)});
  }

  void PushInlineCode(std::string_view inline_code, int32_t backtick_count) {
    auto node = std::make_unique<MarkdownInlineNode>(
        MarkdownInlineSyntax::kInlineCode, inline_code);
    auto raw = std::make_unique<MarkdownRawTextNode>(inline_code.substr(
        backtick_count, inline_code.size() - 2 * backtick_count));
    node->AppendChild(std::move(raw));
    tokens_.emplace_back(
        Token{.type_ = TokenType::kNode, .node_ = std::move(node)});
  }

  void PushRawText(std::string_view raw_text) {
    if (!tokens_.empty() && tokens_.back().type_ == TokenType::kRawText) {
      tokens_.back().content_ = MergeRawText(tokens_.back().content_, raw_text);
    } else {
      tokens_.emplace_back(
          Token{.type_ = TokenType::kRawText, .content_ = raw_text});
    }
  }

  void PushExclamation(std::string_view image_start) {
    tokens_.emplace_back(Token{.type_ = TokenType::kKeywordExclamation,
                               .content_ = image_start});
  }

  void PushSquareBracketsStart(std::string_view brackets) {
    tokens_.emplace_back(Token{.type_ = TokenType::kKeywordLeftSquareBrackets,
                               .content_ = brackets});
  }

  void PushSquareBracketsEnd(std::string_view brackets) {
    if (brackets.size() >= 2) {
      MatchSymmetryKeyword<TokenType::kKeywordLeftSquareBrackets,
                           TokenType::kKeywordRightSquareBrackets,
                           SquareBracketToNode>(brackets);
    } else {
      tokens_.emplace_back(
          Token{.type_ = TokenType::kKeywordRightSquareBrackets,
                .content_ = brackets});
    }
  }

  void PushRoundBracketStart(std::string_view brackets) {
    tokens_.emplace_back(Token{.type_ = TokenType::kKeywordLeftRoundBrackets,
                               .content_ = brackets});
  }

  void PushRoundBracketEnd(std::string_view brackets) {
    for (int32_t i = tokens_.size() - 1; i > 0; i--) {
      // find: ](...)
      if (tokens_[i].type_ == TokenType::kKeywordLeftRoundBrackets &&
          tokens_[i - 1].type_ == TokenType::kKeywordRightSquareBrackets) {
        for (int32_t j = i; j >= 0; j--) {
          // find: [...](...)
          if (tokens_[j].type_ == TokenType::kKeywordLeftSquareBrackets) {
            // erase syntax in round bracket
            auto url = std::string_view(
                tokens_[i].content_.data() + 1,
                brackets.data() - tokens_[i].content_.data() - 1);
            std::string_view extra;
            auto space_index = url.find(' ');
            if (space_index != std::string_view::npos) {
              extra = url.substr(space_index);
              url = url.substr(0, space_index);
            }
            if (j > 0 && tokens_[j].content_.size() == 1 &&
                tokens_[j - 1].type_ == TokenType::kKeywordExclamation) {
              // img: ![...](...)
              auto img = std::make_unique<MarkdownImageNode>(
                  MergeRawText(tokens_[j - 1].content_, brackets), url);
              if (i > j + 2) {
                img->SetAltText(MergeRawText(tokens_[j + 1].content_,
                                             tokens_[i - 2].content_));
                MergePiecesToNode(j + 1, i - 1, img.get());
              }
              if (!extra.empty()) {
                const auto [width, height] = ParseImageSize(extra);
                if (width > 0) {
                  img->SetWidth(width);
                  img->SetHeight(height);
                }
              }
              tokens_.resize(j - 1);
              tokens_.emplace_back(
                  Token{.type_ = TokenType::kNode, .node_ = std::move(img)});
            } else {
              // link: [...](...)
              auto node = std::make_unique<MarkdownLinkNode>(
                  MergeRawText(tokens_[j].content_, brackets), url);
              MergePiecesToNode(j + 1, i - 1, node.get());
              tokens_.resize(j);
              tokens_.emplace_back(
                  Token{.type_ = TokenType::kNode, .node_ = std::move(node)});
            }
            return;
          }
        }
      }
    }
    PushRawText(brackets);
  }

  void PushBracesStart(std::string_view braces) {
    tokens_.emplace_back(
        Token{.type_ = TokenType::kKeywordLeftBraces, .content_ = braces});
  }

  void PushBracesEnd(std::string_view braces) {
    MatchSymmetryKeyword<TokenType::kKeywordLeftBraces, TokenType::kRawText,
                         BracesToNode>(braces);
  }

  void PushHtmlTagOpen(Token token) { tokens_.emplace_back(std::move(token)); }

  void PushHtmlTagClose(std::string_view content, std::string_view tag) {
    tokens_.emplace_back(Token{.type_ = TokenType::kKeywordHtmlClose,
                               .content_ = content,
                               .tag_ = tag});
    MatchHtmlTag(tokens_.size() - 1, tag);
  }

  void MatchHtmlTag(int32_t end_index, std::string_view tag) {
    for (int32_t j = end_index - 1; j >= 0; j--) {
      if (tokens_[j].type_ == TokenType::kKeywordHtmlOpen &&
          tokens_[j].tag_ == tag) {
        auto html_node = std::make_unique<MarkdownInlineHtmlTag>();
        html_node->SetText(
            MergeRawText(tokens_[j].content_, tokens_[end_index].content_));
        html_node->SetTag(tokens_[j].tag_);
        html_node->SetAttributes(std::move(tokens_[j].attributes_));
        MergePiecesToNode(j + 1, end_index, html_node.get());
        tokens_.resize(j);
        tokens_.emplace_back(
            Token{.type_ = TokenType::kNode, .node_ = std::move(html_node)});
        break;
      }
    }
  }

  static std::pair<float, float> ParseImageSize(std::string_view extra) {
    float width = -1, height = -1;
    auto pos = extra.find(" width=", 0);
    if (pos != std::string_view::npos) {
      char* end = nullptr;
      width = strtod(extra.data() + pos + 7, &end);
      if (end == extra.data() + pos + 7 || width == 0) {
        return {-1, -1};
      }
    }
    pos = extra.find(" height=", 0);
    if (pos != std::string_view::npos) {
      char* end = nullptr;
      height = strtod(extra.data() + pos + 8, &end);
      if (end == extra.data() + pos + 8 || height == 0) {
        return {-1, -1};
      }
    }
    return {width, height};
  }

  std::vector<Token> tokens_;
};

std::unique_ptr<MarkdownInlineNode> MarkdownInlineSyntaxParser::Parse(
    const std::string_view content) {
  MarkdownInlineSyntaxParserImpl parser;
  return parser.Parse(content);
}
}  // namespace lynx::markdown
