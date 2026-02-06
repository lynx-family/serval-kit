// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/parser/embed/markdown_inline_parser.h"

#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "markdown/parser/embed/markdown_inline_node.h"
#include "markdown/utils/markdown_definition.h"
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
  bool can_start_{false};
  bool can_end_{false};
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
      attributes_.emplace_back(MarkdownHtmlAttribute{
          .name = std::string(name_), .value = std::string(value)});
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
        attributes_.emplace_back(MarkdownHtmlAttribute{
            .name = std::string(name_), .value = std::string(value)});
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

struct ImageExtra {
  float width_{};
  float height_{};
  std::string_view caption_;
};

class MarkdownInlineSyntaxParserImpl {
 public:
  std::unique_ptr<MarkdownInlineNode> Parse(const std::string_view content) {
    auto* processing = content.data();
    auto* begin = processing;
    auto* end = content.data() + content.length();
    while (processing != end) {
      if (*processing == '*') {
        const int32_t count = SameCharCount(processing, end, '*');
        PushDelimiter(std::string_view(processing, count),
                      TokenType::kKeywordStars, processing, begin, end);
        processing += count;
      } else if (*processing == '_') {
        const int32_t count = SameCharCount(processing, end, '_');
        PushDelimiter(std::string_view(processing, count),
                      TokenType::kKeywordUnderlines, processing, begin, end);
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
        PushDelimiter(std::string_view(processing, count),
                      TokenType::kKeywordWavy, processing, begin, end);
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
      } else if (*processing == '&') {
        // &#x00000000; &dagger;
        constexpr size_t HTML_ENTITY_MAX_LENGTH = 12;
        auto substr = std::string_view(
            processing, std::min(HTML_ENTITY_MAX_LENGTH,
                                 static_cast<size_t>(end - processing)));
        const auto entity_end_token = substr.find(';');
        bool success = false;
        if (entity_end_token != std::string_view::npos &&
            entity_end_token > 1) {
          const auto entity_raw_text = substr.substr(0, entity_end_token + 1);
          if (auto entity = DecodeHtmlEntity(entity_raw_text);
              !entity.empty()) {
            PushHtmlEntity(entity_raw_text, std::move(entity));
            success = true;
            processing += entity_end_token + 1;
          }
        }
        if (!success) {
          PushRawText(std::string_view(processing, 1));
          processing++;
        }
      } else if (*processing == '\n') {
        auto text = std::string_view(processing, 1);
        tokens_.emplace_back(Token{
            .type_ = TokenType::kNode,
            .content_ = text,
            .node_ = std::make_unique<MarkdownBreakLineNode>(text),
        });
        processing++;
      } else {
        PushRawText(std::string_view(processing, 1));
        processing++;
      }
    }
    auto result = std::make_unique<MarkdownInlineNode>();
    result->SetText(content);
    ProcessDelimiters(tokens_, 0, tokens_.size(), result.get());
    return result;
  }

 private:
  static uint32_t UnicodeBefore(const char* current, const char* begin) {
    while (current != begin) {
      --current;
      if (IsUtf8StartByte(*current)) {
        int32_t len;
        return GetUnicodeFromUtf8String(current, &len);
      }
    }
    return 0;
  }
  static std::string_view DecodeEntity(const std::string_view entity) {
    static constexpr std::pair<std::string_view, std::string_view>
        entity_map[] = {
            {"amp", "\u0026"},    {"nbsp", "\u00a0"},   {"ensp", "\u2002"},
            {"emsp", "\u2003"},   {"thinsp", "\u2009"}, {"zwnj", "\u200c"},
            {"zwj", "\u200d"},    {"lrm", "\u200e"},    {"rlm", "\u200f"},
            {"ndash", "\u2013"},  {"lsquo", "\u2018"},  {"rsquo", "\u2019"},
            {"sbquo", "\u201a"},  {"ldquo", "\u201c"},  {"rdquo", "\u201d"},
            {"bdquo", "\u201e"},  {"dagger", "\u2020"}, {"Dagger", "\u2021"},
            {"bull", "\u2022"},   {"hellip", "\u2026"}, {"permil", "\u2030"},
            {"prime", "\u2032"},  {"Prime", "\u2033"},  {"lsaquo", "\u2039"},
            {"rsquo", "\u203a"},  {"oline", "\u203e"},  {"frasl", "\u2044"},
            {"lt", "\u003c"},     {"gt", "\u003e"},     {"middot", "\u00b7"},
            {"mldr", "\u2026"},   {"cacute", "\u0107"}, {"quot", "\u0022"},
            {"amacr", "\u0101"},  {"caron", "\u02c7"},  {"emacr", "\u0113"},
            {"mdash", "\u2014"},  {"copy", "\u00a9"},   {"times", "\u00d7"},
            {"darr", "\u2193"},   {"imacr", "\u0121"},  {"iacute", "\u00ed"},
            {"igrave", "\u00ec"}, {"agrave", "\u00e0"}, {"ge", "\u2265"},
            {"le", "\u2264"},
        };
    for (const auto& [entity_string, unicode] : entity_map) {
      if (entity == entity_string) {
        return unicode;
      }
    }
    return "";
  }

  static std::string DecodeHtmlEntity(std::string_view raw) {
    if (raw[1] == '#') {
      int32_t unicode = 0;
      bool success = false;
      if (raw[2] == 'x') {
        // &#x000000;
        const std::string hex_string(raw.data() + 3, raw.length() - 4);
        success = base::StringToInt(hex_string, &unicode, 16);
      } else {
        // &#000000;
        const std::string decimal_string(raw.data() + 2, raw.length() - 3);
        success = base::StringToInt(decimal_string, &unicode, 10);
      }
      if (!success || unicode > 0x10ffff || unicode < 0) {
        return "";
      }
      char32_t unicode_char = unicode;
      return base::U32StringToU8({&unicode_char, 1});
    }
    return std::string(DecodeEntity(raw.substr(1, raw.length() - 2)));
  }

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

  static bool IsDelimiter(const Token& token) {
    return token.type_ == TokenType::kKeywordStars ||
           token.type_ == TokenType::kKeywordUnderlines ||
           token.type_ == TokenType::kKeywordWavy;
  }

  static void ProcessDelimiters(std::vector<Token>& tokens, const int32_t start,
                                const int32_t end, MarkdownInlineNode* node) {
    bool has_delimiters = false;
    for (int32_t i = start; i < end; i++) {
      if (IsDelimiter(tokens[i])) {
        has_delimiters = true;
        break;
      }
    }
    if (!has_delimiters) {
      MergePiecesToNode(tokens, start, end, node);
      return;
    }
    std::vector<Token> new_tokens;
    new_tokens.reserve(end - start);
    for (int32_t i = start; i < end; i++) {
      auto& token = tokens[i];
      if (token.type_ == TokenType::kKeywordStars) {
        MatchSymmetryKeyword<TokenType::kKeywordStars, TokenType::kKeywordStars,
                             StarCountToNode>(new_tokens, token.content_, true,
                                              token.can_start_, token.can_end_);
      } else if (token.type_ == TokenType::kKeywordUnderlines) {
        MatchSymmetryKeyword<TokenType::kKeywordUnderlines,
                             TokenType::kKeywordUnderlines, StarCountToNode>(
            new_tokens, token.content_, true, token.can_start_, token.can_end_);
      } else if (token.type_ == TokenType::kKeywordWavy) {
        MatchSymmetryKeyword<TokenType::kKeywordWavy, TokenType::kKeywordWavy,
                             WavyToNode>(new_tokens, token.content_, true,
                                         token.can_start_, token.can_end_);
      } else {
        new_tokens.emplace_back(std::move(token));
      }
    }
    MergePiecesToNode(new_tokens, 0, new_tokens.size(), node);
  }

  static void MergePiecesToNode(std::vector<Token>& tokens, const int32_t start,
                                const int32_t end, MarkdownInlineNode* node) {
    std::string_view merged_text;
    for (int32_t i = start; i < end; i++) {
      if (tokens[i].type_ == TokenType::kRawText ||
          (tokens[i].type_ > TokenType::kKeyword &&
           tokens[i].type_ < TokenType::kNode)) {
        if (merged_text.empty()) {
          merged_text = tokens[i].content_;
        } else {
          merged_text = MergeRawText(merged_text, tokens[i].content_);
        }
      } else if (tokens[i].type_ == TokenType::kNode) {
        if (!merged_text.empty()) {
          node->AppendChild(std::make_unique<MarkdownRawTextNode>(merged_text));
          merged_text = "";
        }
        node->AppendChild(std::move(tokens[i].node_));
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
  static void MatchSymmetryKeyword(std::vector<Token>& tokens,
                                   std::string_view stars,
                                   bool delimiter_processed, bool can_start,
                                   bool can_end) {
    int32_t rest_stars_count = stars.size();
    if (can_end) {
      for (int32_t i = tokens.size() - 1; i >= 0; i--) {
        if (tokens[i].type_ == match_type && tokens[i].can_start_) {
          auto piece_start_content = tokens[i].content_;
          int32_t piece_star_count = tokens[i].content_.size();
          int32_t keyword_count = std::min(piece_star_count, rest_stars_count);
          auto [node_syntax, eat] = Generate(keyword_count);
          if (node_syntax == MarkdownInlineSyntax::kNone)
            continue;
          auto new_node = std::make_unique<MarkdownInlineNode>();
          new_node->SetSyntax(node_syntax);
          keyword_count = eat;
          auto text = MergeRawText(
              piece_start_content.substr(piece_star_count - keyword_count),
              stars.substr(0, keyword_count));
          new_node->SetText(text);
          if (delimiter_processed) {
            MergePiecesToNode(tokens, i + 1, tokens.size(), new_node.get());
          } else {
            ProcessDelimiters(tokens, i + 1, tokens.size(), new_node.get());
          }
          if (eat == piece_star_count) {
            tokens.resize(i);
          } else {
            tokens.resize(i + 1);
            tokens[i].content_ =
                tokens[i].content_.substr(0, piece_star_count - keyword_count);
          }
          tokens.emplace_back(Token{.type_ = TokenType::kNode,
                                    .content_ = text,
                                    .node_ = std::move(new_node)});
          if (eat == rest_stars_count) {
            rest_stars_count = 0;
            break;
          }
          stars = stars.substr(keyword_count);
          rest_stars_count = stars.size();
        }
      }
    }
    if (rest_stars_count > 0) {
      if (current_type == TokenType::kRawText || !can_start) {
        PushRawTextToTokens(tokens, stars);
      } else {
        tokens.emplace_back(Token{.type_ = current_type,
                                  .content_ = std::move(stars),
                                  .can_start_ = can_start,
                                  .can_end_ = can_end});
      }
    }
  }

  void PushDelimiter(const std::string_view content, const TokenType type,
                     const char* processing, const char* begin,
                     const char* end) {
    const char32_t unicode_before = UnicodeBefore(processing, begin);
    int32_t len;
    const char32_t unicode_after =
        (processing + content.length() == end)
            ? 0
            : GetUnicodeFromUtf8String(processing + content.length(), &len);
    const bool is_empty_before = IsEmptyChar(unicode_before);
    const bool is_punc_before = IsPunctuation(unicode_before);
    const bool is_empty_after = IsEmptyChar(unicode_after);
    const bool is_punc_after = IsPunctuation(unicode_after);
    bool can_start = !is_empty_after &&
                     (!is_punc_after || is_empty_before || is_punc_before);
    bool can_end = !is_empty_before &&
                   (!is_punc_before || is_empty_after || is_punc_after);
    if (type == TokenType::kKeywordUnderlines) {
      const bool can_start_underline =
          can_start && (!can_end || is_punc_before);
      const bool can_end_underline = can_end && (!can_start || is_punc_after);
      can_start = can_start_underline;
      can_end = can_end_underline;
    }
    tokens_.emplace_back(Token{.type_ = type,
                               .content_ = content,
                               .can_start_ = can_start,
                               .can_end_ = can_end});
  }

  void PushBackSlash(std::string_view backslash) {
    auto node = std::make_unique<MarkdownInlineNode>(
        MarkdownInlineSyntax::kEscape, backslash);
    node->AppendChild(
        std::make_unique<MarkdownRawTextNode>(backslash.substr(1)));
    tokens_.emplace_back(Token{.type_ = TokenType::kNode,
                               .content_ = backslash,
                               .node_ = std::move(node)});
  }

  void PushInlineCode(std::string_view inline_code, int32_t backtick_count) {
    auto node = std::make_unique<MarkdownInlineNode>(
        MarkdownInlineSyntax::kInlineCode, inline_code);
    auto raw = std::make_unique<MarkdownRawTextNode>(inline_code.substr(
        backtick_count, inline_code.size() - 2 * backtick_count));
    node->AppendChild(std::move(raw));
    tokens_.emplace_back(Token{.type_ = TokenType::kNode,
                               .content_ = inline_code,
                               .node_ = std::move(node)});
  }

  void PushRawText(std::string_view raw_text) {
    PushRawTextToTokens(tokens_, raw_text);
  }
  static void PushRawTextToTokens(std::vector<Token>& tokens,
                                  std::string_view raw_text) {
    if (!tokens.empty() && tokens.back().type_ == TokenType::kRawText) {
      tokens.back().content_ = MergeRawText(tokens.back().content_, raw_text);
    } else {
      tokens.emplace_back(
          Token{.type_ = TokenType::kRawText, .content_ = raw_text});
    }
  }

  void PushHtmlEntity(std::string_view raw_text, std::string&& entity) {
    tokens_.emplace_back(Token{
        .type_ = TokenType::kNode,
        .content_ = raw_text,
        .node_ = std::make_unique<MarkdownHtmlEntityNode>(raw_text,
                                                          std::move(entity)),
    });
  }

  void PushExclamation(std::string_view image_start) {
    tokens_.emplace_back(Token{.type_ = TokenType::kKeywordExclamation,
                               .content_ = image_start});
  }

  void PushSquareBracketsStart(std::string_view brackets) {
    tokens_.emplace_back(Token{.type_ = TokenType::kKeywordLeftSquareBrackets,
                               .content_ = brackets,
                               .can_start_ = true});
  }

  void PushSquareBracketsEnd(std::string_view brackets) {
    if (brackets.size() >= 2) {
      MatchSymmetryKeyword<TokenType::kKeywordLeftSquareBrackets,
                           TokenType::kKeywordRightSquareBrackets,
                           SquareBracketToNode>(tokens_, brackets, false, false,
                                                true);
    } else {
      tokens_.emplace_back(
          Token{.type_ = TokenType::kKeywordRightSquareBrackets,
                .content_ = brackets});
    }
  }

  void PushRoundBracketStart(std::string_view brackets) {
    tokens_.emplace_back(Token{.type_ = TokenType::kKeywordLeftRoundBrackets,
                               .content_ = brackets,
                               .can_start_ = true});
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
              auto text = MergeRawText(tokens_[j - 1].content_, brackets);
              auto img = std::make_unique<MarkdownImageNode>(text, url);
              if (i > j + 2) {
                img->SetAltText(MergeRawText(tokens_[j + 1].content_,
                                             tokens_[i - 2].content_));
                ProcessDelimiters(tokens_, j + 1, i - 1, img.get());
              }
              if (!extra.empty()) {
                const auto [width, height, caption] = ParseImageExtra(extra);
                if (width > 0) {
                  img->SetWidth(width);
                  img->SetHeight(height);
                }
                img->SetCaption(caption);
              }
              tokens_.resize(j - 1);
              tokens_.emplace_back(Token{.type_ = TokenType::kNode,
                                         .content_ = text,
                                         .node_ = std::move(img)});
            } else {
              // link: [...](...)
              auto text = MergeRawText(tokens_[j].content_, brackets);
              auto node = std::make_unique<MarkdownLinkNode>(text, url);
              ProcessDelimiters(tokens_, j + 1, i - 1, node.get());
              tokens_.resize(j);
              tokens_.emplace_back(Token{.type_ = TokenType::kNode,
                                         .content_ = text,
                                         .node_ = std::move(node)});
            }
            return;
          }
        }
      }
    }
    PushRawText(brackets);
  }

  void PushBracesStart(std::string_view braces) {
    tokens_.emplace_back(Token{.type_ = TokenType::kKeywordLeftBraces,
                               .content_ = braces,
                               .can_start_ = true});
  }

  void PushBracesEnd(std::string_view braces) {
    MatchSymmetryKeyword<TokenType::kKeywordLeftBraces, TokenType::kRawText,
                         BracesToNode>(tokens_, braces, false, false, true);
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
        auto raw_text =
            MergeRawText(tokens_[j].content_, tokens_[end_index].content_);
        html_node->SetText(raw_text);
        html_node->SetTag(tokens_[j].tag_);
        html_node->SetAttributes(std::move(tokens_[j].attributes_));
        ProcessDelimiters(tokens_, j + 1, end_index, html_node.get());
        tokens_.resize(j);
        tokens_.emplace_back(Token{.type_ = TokenType::kNode,
                                   .content_ = raw_text,
                                   .node_ = std::move(html_node)});
        break;
      }
    }
  }
  static std::vector<std::string_view> SplitBySpaceAndQuote(
      const std::string_view text) {
    auto* processing = text.data();
    auto* end = text.data() + text.length();
    std::vector<std::string_view> split;
    auto* piece_start = processing;
    bool in_quote = false;
    char quote = '"';
    while (processing < end) {
      if (in_quote) {
        if (*processing == quote && *(processing - 1) != '\\') {
          in_quote = false;
          split.emplace_back(piece_start,
                             static_cast<size_t>(processing + 1 - piece_start));
          piece_start = processing + 1;
        }
      } else if (*processing == '"' || *processing == '\'') {
        in_quote = true;
        quote = *processing;
        piece_start = processing;
      } else if (*processing == ' ') {
        if (piece_start != processing) {
          split.emplace_back(piece_start,
                             static_cast<size_t>(processing - piece_start));
        }
        piece_start = processing + 1;
      }
      processing++;
    }
    if (processing != piece_start) {
      split.emplace_back(piece_start,
                         static_cast<size_t>(processing - piece_start));
    }
    return split;
  }
  static ImageExtra ParseImageExtra(std::string_view extra) {
    ImageExtra result;
    result.width_ = -1;
    result.height_ = -1;
    auto split = SplitBySpaceAndQuote(extra);
    for (auto& str : split) {
      if (str.substr(0, 6) == "width=") {
        if (!base::StringToFloat(std::string(str.substr(6)), result.width_)) {
          result.width_ = -1;
        }
      } else if (str.substr(0, 7) == "height=") {
        if (!base::StringToFloat(std::string(str.substr(7)), result.height_)) {
          result.height_ = -1;
        }
      } else {
        if (str.length() > 2 && str.front() == str.back() &&
            (str.front() == '\'' || str.front() == '"')) {
          result.caption_ = str.substr(1, str.length() - 2);
        }
      }
    }
    return result;
  }

  std::vector<Token> tokens_;
};

std::unique_ptr<MarkdownInlineNode> MarkdownInlineSyntaxParser::Parse(
    const std::string_view content) {
  MarkdownInlineSyntaxParserImpl parser;
  return parser.Parse(content);
}

}  // namespace lynx::markdown
