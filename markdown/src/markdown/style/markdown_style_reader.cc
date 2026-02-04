// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/style/markdown_style_reader.h"

#include "base/include/string/string_utils.h"
#include "markdown/element/markdown_attachments.h"
#include "markdown/element/markdown_document.h"
#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/style/markdown_style_initializer.h"
#include "markdown/style/markdown_style_value.h"
namespace lynx::markdown {
class MarkdownStyleReaderImpl {
 public:
  explicit MarkdownStyleReaderImpl(MarkdownResourceLoader* loader)
      : loader_(loader) {}
  ~MarkdownStyleReaderImpl() = default;

 private:
  MarkdownResourceLoader* loader_;

 public:
  // AUTO GEN START
  constexpr const char* ToString(MarkdownStyleTag value) {
    switch (value) {
      case MarkdownStyleTag::kNormalText:
        return "normalText";
      case MarkdownStyleTag::kH1:
        return "h1";
      case MarkdownStyleTag::kH2:
        return "h2";
      case MarkdownStyleTag::kH3:
        return "h3";
      case MarkdownStyleTag::kH4:
        return "h4";
      case MarkdownStyleTag::kH5:
        return "h5";
      case MarkdownStyleTag::kH6:
        return "h6";
      case MarkdownStyleTag::kLink:
        return "link";
      case MarkdownStyleTag::kInlineCode:
        return "inlineCode";
      case MarkdownStyleTag::kCodeBlock:
        return "codeBlock";
      case MarkdownStyleTag::kQuote:
        return "quote";
      case MarkdownStyleTag::kOrderedList:
        return "orderedList";
      case MarkdownStyleTag::kUnorderedList:
        return "unorderedList";
      case MarkdownStyleTag::kRef:
        return "ref";
      case MarkdownStyleTag::kTable:
        return "table";
      case MarkdownStyleTag::kTableCell:
        return "tableCell";
      case MarkdownStyleTag::kSplit:
        return "split";
      case MarkdownStyleTag::kTypewriterCursor:
        return "typewriterCursor";
      case MarkdownStyleTag::kMark:
        return "mark";
      case MarkdownStyleTag::kDoubleBraces:
        return "doubleBraces";
      case MarkdownStyleTag::kTruncation:
        return "truncation";
      case MarkdownStyleTag::kListItem:
        return "listItem";
      case MarkdownStyleTag::kTableHeader:
        return "tableHeader";
      case MarkdownStyleTag::kImage:
        return "image";
      case MarkdownStyleTag::kQuoteBorderLine:
        return "quoteBorderLine";
      case MarkdownStyleTag::kUnorderedListMarker:
        return "unorderedListMarker";
      case MarkdownStyleTag::kSpan:
        return "span";
      case MarkdownStyleTag::kOrderedListNumber:
        return "orderedListNumber";
      case MarkdownStyleTag::kImageCaption:
        return "imageCaption";
      case MarkdownStyleTag::kBold:
        return "bold";
      case MarkdownStyleTag::kItalic:
        return "italic";
    }
    return "";
  }
  constexpr const char* ToString(MarkdownStyleOp value) {
    switch (value) {
      case MarkdownStyleOp::kRangeEnd:
        return "rangeEnd";
      case MarkdownStyleOp::kFont:
        return "font";
      case MarkdownStyleOp::kFontSize:
        return "fontSize";
      case MarkdownStyleOp::kFontWeight:
        return "fontWeight";
      case MarkdownStyleOp::kFontStyle:
        return "fontStyle";
      case MarkdownStyleOp::kLineHeight:
        return "lineHeight";
      case MarkdownStyleOp::kParagraphSpace:
        return "paragraphSpace";
      case MarkdownStyleOp::kColor:
        return "color";
      case MarkdownStyleOp::kBackgroundColor:
        return "backgroundColor";
      case MarkdownStyleOp::kTextOverflow:
        return "textOverflow";
      case MarkdownStyleOp::kTextMaxline:
        return "textMaxline";
      case MarkdownStyleOp::kMarginTop:
        return "marginTop";
      case MarkdownStyleOp::kMarginBottom:
        return "marginBottom";
      case MarkdownStyleOp::kMarginLeft:
        return "marginLeft";
      case MarkdownStyleOp::kMarginRight:
        return "marginRight";
      case MarkdownStyleOp::kPaddingTop:
        return "paddingTop";
      case MarkdownStyleOp::kPaddingBottom:
        return "paddingBottom";
      case MarkdownStyleOp::kPaddingLeft:
        return "paddingLeft";
      case MarkdownStyleOp::kPaddingRight:
        return "paddingRight";
      case MarkdownStyleOp::kBorderType:
        return "borderType";
      case MarkdownStyleOp::kBorderColor:
        return "borderColor";
      case MarkdownStyleOp::kBorderWidth:
        return "borderWidth";
      case MarkdownStyleOp::kBorderRadius:
        return "borderRadius";
      case MarkdownStyleOp::kBackgroundImage:
        return "backgroundImage";
      case MarkdownStyleOp::kBackgroundSize:
        return "backgroundSize";
      case MarkdownStyleOp::kBackgroundRepeat:
        return "backgroundRepeat";
      case MarkdownStyleOp::kBackgroundPosition:
        return "backgroundPosition";
      case MarkdownStyleOp::kIndent:
        return "indent";
      case MarkdownStyleOp::kNumberType:
        return "numberType";
      case MarkdownStyleOp::kNumberFont:
        return "numberFont";
      case MarkdownStyleOp::kNumberFontSize:
        return "numberFontSize";
      case MarkdownStyleOp::kNumberColor:
        return "numberColor";
      case MarkdownStyleOp::kNumberMarginRight:
        return "numberMarginRight";
      case MarkdownStyleOp::kMarkType:
        return "markType";
      case MarkdownStyleOp::kMarkColor:
        return "markColor";
      case MarkdownStyleOp::kMarkSize:
        return "markSize";
      case MarkdownStyleOp::kMarkMarginRight:
        return "markMarginRight";
      case MarkdownStyleOp::kBackgroundType:
        return "backgroundType";
      case MarkdownStyleOp::kCustomCursor:
        return "customCursor";
      case MarkdownStyleOp::kVerticalAlign:
        return "verticalAlign";
      case MarkdownStyleOp::kTruncationType:
        return "truncationType";
      case MarkdownStyleOp::kContent:
        return "content";
      case MarkdownStyleOp::kTextDecorationStyle:
        return "textDecorationStyle";
      case MarkdownStyleOp::kTextDecorationLine:
        return "textDecorationLine";
      case MarkdownStyleOp::kTextDecorationColor:
        return "textDecorationColor";
      case MarkdownStyleOp::kTextDecorationThickness:
        return "textDecorationThickness";
      case MarkdownStyleOp::kLineSpace:
        return "lineSpace";
      case MarkdownStyleOp::kScrollX:
        return "scrollX";
      case MarkdownStyleOp::kMaxWidth:
        return "maxWidth";
      case MarkdownStyleOp::kWidth:
        return "width";
      case MarkdownStyleOp::kHeight:
        return "height";
      case MarkdownStyleOp::kRadius:
        return "radius";
      case MarkdownStyleOp::kShrink:
        return "shrink";
      case MarkdownStyleOp::kEnableAltText:
        return "enableAltText";
      case MarkdownStyleOp::kWordBreak:
        return "wordBreak";
      case MarkdownStyleOp::kDirection:
        return "direction";
      case MarkdownStyleOp::kAltImage:
        return "altImage";
      case MarkdownStyleOp::kTextAlign:
        return "textAlign";
      case MarkdownStyleOp::kCaptionPosition:
        return "captionPosition";
      case MarkdownStyleOp::kMaxHeight:
        return "maxHeight";
      case MarkdownStyleOp::kMinWidth:
        return "minWidth";
      case MarkdownStyleOp::kMinHeight:
        return "minHeight";
      case MarkdownStyleOp::kTableBorder:
        return "tableBorder";
      case MarkdownStyleOp::kTableBackground:
        return "tableBackground";
      case MarkdownStyleOp::kAltColor:
        return "altColor";
      case MarkdownStyleOp::kLineType:
        return "lineType";
      case MarkdownStyleOp::kTextIndent:
        return "textIndent";
      case MarkdownStyleOp::kLastLineAlignment:
        return "lastLineAlignment";
      case MarkdownStyleOp::kTableSplit:
        return "tableSplit";
    }
    return "";
  }
  constexpr const char* ToString(MarkdownBorderType value) {
    switch (value) {
      case MarkdownBorderType::kNone:
        return "none";
      case MarkdownBorderType::kSolid:
        return "solid";
    }
    return "";
  }

  void ReadMarkdownBorderTypeValue(const ValueMap& map, const std::string& key,
                                   MarkdownBorderType* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownBorderType::kNone,
                 MarkdownBorderType::kSolid,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownNumberType value) {
    switch (value) {
      case MarkdownNumberType::kNumber:
        return "number";
      case MarkdownNumberType::kAlphabet:
        return "alphabet";
      case MarkdownNumberType::kRomanNumerals:
        return "roman-numerals";
      case MarkdownNumberType::kMixed:
        return "mixed";
    }
    return "";
  }

  void ReadMarkdownNumberTypeValue(const ValueMap& map, const std::string& key,
                                   MarkdownNumberType* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownNumberType::kNumber,
                 MarkdownNumberType::kAlphabet,
                 MarkdownNumberType::kRomanNumerals,
                 MarkdownNumberType::kMixed,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownMarkType value) {
    switch (value) {
      case MarkdownMarkType::kCircle:
        return "circle";
      case MarkdownMarkType::kRing:
        return "ring";
      case MarkdownMarkType::kSquare:
        return "square";
      case MarkdownMarkType::kMixed:
        return "mixed";
    }
    return "";
  }

  void ReadMarkdownMarkTypeValue(const ValueMap& map, const std::string& key,
                                 MarkdownMarkType* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownMarkType::kCircle,
                 MarkdownMarkType::kRing,
                 MarkdownMarkType::kSquare,
                 MarkdownMarkType::kMixed,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownBackgroundType value) {
    switch (value) {
      case MarkdownBackgroundType::kNone:
        return "none";
      case MarkdownBackgroundType::kCapsule:
        return "capsule";
    }
    return "";
  }

  void ReadMarkdownBackgroundTypeValue(const ValueMap& map,
                                       const std::string& key,
                                       MarkdownBackgroundType* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownBackgroundType::kNone,
                 MarkdownBackgroundType::kCapsule,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownTextOverflow value) {
    switch (value) {
      case MarkdownTextOverflow::kClip:
        return "clip";
      case MarkdownTextOverflow::kEllipsis:
        return "ellipsis";
    }
    return "";
  }

  void ReadMarkdownTextOverflowValue(const ValueMap& map,
                                     const std::string& key,
                                     MarkdownTextOverflow* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownTextOverflow::kClip,
                 MarkdownTextOverflow::kEllipsis,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownVerticalAlign value) {
    switch (value) {
      case MarkdownVerticalAlign::kBaseline:
        return "baseline";
      case MarkdownVerticalAlign::kTop:
        return "top";
      case MarkdownVerticalAlign::kBottom:
        return "bottom";
      case MarkdownVerticalAlign::kCenter:
        return "center";
    }
    return "";
  }

  void ReadMarkdownVerticalAlignValue(const ValueMap& map,
                                      const std::string& key,
                                      MarkdownVerticalAlign* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownVerticalAlign::kBaseline,
                 MarkdownVerticalAlign::kTop,
                 MarkdownVerticalAlign::kBottom,
                 MarkdownVerticalAlign::kCenter,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownFontWeight value) {
    switch (value) {
      case MarkdownFontWeight::kNormal:
        return "normal";
      case MarkdownFontWeight::kBold:
        return "bold";
      case MarkdownFontWeight::k100:
        return "100";
      case MarkdownFontWeight::k200:
        return "200";
      case MarkdownFontWeight::k300:
        return "300";
      case MarkdownFontWeight::k400:
        return "400";
      case MarkdownFontWeight::k500:
        return "500";
      case MarkdownFontWeight::k600:
        return "600";
      case MarkdownFontWeight::k700:
        return "700";
      case MarkdownFontWeight::k800:
        return "800";
      case MarkdownFontWeight::k900:
        return "900";
    }
    return "";
  }

  void ReadMarkdownFontWeightValue(const ValueMap& map, const std::string& key,
                                   MarkdownFontWeight* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownFontWeight::kNormal,
                 MarkdownFontWeight::kBold,
                 MarkdownFontWeight::k100,
                 MarkdownFontWeight::k200,
                 MarkdownFontWeight::k300,
                 MarkdownFontWeight::k400,
                 MarkdownFontWeight::k500,
                 MarkdownFontWeight::k600,
                 MarkdownFontWeight::k700,
                 MarkdownFontWeight::k800,
                 MarkdownFontWeight::k900,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownTruncationType value) {
    switch (value) {
      case MarkdownTruncationType::kText:
        return "text";
      case MarkdownTruncationType::kView:
        return "view";
    }
    return "";
  }

  void ReadMarkdownTruncationTypeValue(const ValueMap& map,
                                       const std::string& key,
                                       MarkdownTruncationType* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownTruncationType::kText,
                 MarkdownTruncationType::kView,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownTextDecorationStyle value) {
    switch (value) {
      case MarkdownTextDecorationStyle::kNone:
        return "none";
      case MarkdownTextDecorationStyle::kSolid:
        return "solid";
      case MarkdownTextDecorationStyle::kDouble:
        return "double";
      case MarkdownTextDecorationStyle::kDotted:
        return "dotted";
      case MarkdownTextDecorationStyle::kDashed:
        return "dashed";
      case MarkdownTextDecorationStyle::kWavy:
        return "wavy";
    }
    return "";
  }

  void ReadMarkdownTextDecorationStyleValue(
      const ValueMap& map, const std::string& key,
      MarkdownTextDecorationStyle* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownTextDecorationStyle::kNone,
                 MarkdownTextDecorationStyle::kSolid,
                 MarkdownTextDecorationStyle::kDouble,
                 MarkdownTextDecorationStyle::kDotted,
                 MarkdownTextDecorationStyle::kDashed,
                 MarkdownTextDecorationStyle::kWavy,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownTextDecorationLine value) {
    switch (value) {
      case MarkdownTextDecorationLine::kNone:
        return "none";
      case MarkdownTextDecorationLine::kUnderline:
        return "underline";
      case MarkdownTextDecorationLine::kOverline:
        return "overline";
      case MarkdownTextDecorationLine::kLineThrough:
        return "line-through";
    }
    return "";
  }

  void ReadMarkdownTextDecorationLineValue(const ValueMap& map,
                                           const std::string& key,
                                           MarkdownTextDecorationLine* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownTextDecorationLine::kNone,
                 MarkdownTextDecorationLine::kUnderline,
                 MarkdownTextDecorationLine::kOverline,
                 MarkdownTextDecorationLine::kLineThrough,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownWordBreak value) {
    switch (value) {
      case MarkdownWordBreak::kNormal:
        return "normal";
      case MarkdownWordBreak::kBreakAll:
        return "break-all";
    }
    return "";
  }

  void ReadMarkdownWordBreakValue(const ValueMap& map, const std::string& key,
                                  MarkdownWordBreak* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownWordBreak::kNormal,
                 MarkdownWordBreak::kBreakAll,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownDirection value) {
    switch (value) {
      case MarkdownDirection::kNormal:
        return "normal";
      case MarkdownDirection::kLtr:
        return "ltr";
      case MarkdownDirection::kRtl:
        return "rtl";
    }
    return "";
  }

  void ReadMarkdownDirectionValue(const ValueMap& map, const std::string& key,
                                  MarkdownDirection* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownDirection::kNormal,
                 MarkdownDirection::kLtr,
                 MarkdownDirection::kRtl,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownTextAlign value) {
    switch (value) {
      case MarkdownTextAlign::kUndefined:
        return "undefined";
      case MarkdownTextAlign::kLeft:
        return "left";
      case MarkdownTextAlign::kCenter:
        return "center";
      case MarkdownTextAlign::kRight:
        return "right";
      case MarkdownTextAlign::kJustify:
        return "justify";
    }
    return "";
  }

  void ReadMarkdownTextAlignValue(const ValueMap& map, const std::string& key,
                                  MarkdownTextAlign* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownTextAlign::kUndefined,
                 MarkdownTextAlign::kLeft,
                 MarkdownTextAlign::kCenter,
                 MarkdownTextAlign::kRight,
                 MarkdownTextAlign::kJustify,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownCaptionPosition value) {
    switch (value) {
      case MarkdownCaptionPosition::kBottom:
        return "bottom";
      case MarkdownCaptionPosition::kTop:
        return "top";
    }
    return "";
  }

  void ReadMarkdownCaptionPositionValue(const ValueMap& map,
                                        const std::string& key,
                                        MarkdownCaptionPosition* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownCaptionPosition::kBottom,
                 MarkdownCaptionPosition::kTop,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownTableBorder value) {
    switch (value) {
      case MarkdownTableBorder::kNone:
        return "none";
      case MarkdownTableBorder::kFullRect:
        return "full-rect";
      case MarkdownTableBorder::kUnderline:
        return "underline";
    }
    return "";
  }

  void ReadMarkdownTableBorderValue(const ValueMap& map, const std::string& key,
                                    MarkdownTableBorder* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownTableBorder::kNone,
                 MarkdownTableBorder::kFullRect,
                 MarkdownTableBorder::kUnderline,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownTableBackground value) {
    switch (value) {
      case MarkdownTableBackground::kNone:
        return "none";
      case MarkdownTableBackground::kSolid:
        return "solid";
      case MarkdownTableBackground::kChessBoard:
        return "chess-board";
    }
    return "";
  }

  void ReadMarkdownTableBackgroundValue(const ValueMap& map,
                                        const std::string& key,
                                        MarkdownTableBackground* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownTableBackground::kNone,
                 MarkdownTableBackground::kSolid,
                 MarkdownTableBackground::kChessBoard,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownLineType value) {
    switch (value) {
      case MarkdownLineType::kNone:
        return "none";
      case MarkdownLineType::kSolid:
        return "solid";
      case MarkdownLineType::kDouble:
        return "double";
      case MarkdownLineType::kDotted:
        return "dotted";
      case MarkdownLineType::kDashed:
        return "dashed";
      case MarkdownLineType::kWavy:
        return "wavy";
    }
    return "";
  }

  void ReadMarkdownLineTypeValue(const ValueMap& map, const std::string& key,
                                 MarkdownLineType* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownLineType::kNone,
                 MarkdownLineType::kSolid,
                 MarkdownLineType::kDouble,
                 MarkdownLineType::kDotted,
                 MarkdownLineType::kDashed,
                 MarkdownLineType::kWavy,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownTableSplit value) {
    switch (value) {
      case MarkdownTableSplit::kNone:
        return "none";
      case MarkdownTableSplit::kVertical:
        return "vertical";
      case MarkdownTableSplit::kHorizontal:
        return "horizontal";
      case MarkdownTableSplit::kAll:
        return "all";
    }
    return "";
  }

  void ReadMarkdownTableSplitValue(const ValueMap& map, const std::string& key,
                                   MarkdownTableSplit* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownTableSplit::kNone,
                 MarkdownTableSplit::kVertical,
                 MarkdownTableSplit::kHorizontal,
                 MarkdownTableSplit::kAll,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  constexpr const char* ToString(MarkdownFontStyle value) {
    switch (value) {
      case MarkdownFontStyle::kUndefined:
        return "undefined";
      case MarkdownFontStyle::kNormal:
        return "normal";
      case MarkdownFontStyle::kItalic:
        return "italic";
    }
    return "";
  }

  void ReadMarkdownFontStyleValue(const ValueMap& map, const std::string& key,
                                  MarkdownFontStyle* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 MarkdownFontStyle::kUndefined,
                 MarkdownFontStyle::kNormal,
                 MarkdownFontStyle::kItalic,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }
  void ReadMarkdownBaseStylePart(const ValueMap& map,
                                 MarkdownBaseStylePart* style) {
    ReadFontValue(map, ToString(MarkdownStyleOp::kFont), &style->font_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kFontSize),
                    &style->font_size_);
    ReadMarkdownFontWeightValue(map, ToString(MarkdownStyleOp::kFontWeight),
                                &style->font_weight_);
    ReadMarkdownFontStyleValue(map, ToString(MarkdownStyleOp::kFontStyle),
                               &style->font_style_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kLineHeight),
                    &style->line_height_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kParagraphSpace),
                    &style->paragraph_space_);
    ReadColorValue(map, ToString(MarkdownStyleOp::kColor), &style->color_);
    ReadColorValue(map, ToString(MarkdownStyleOp::kBackgroundColor),
                   &style->background_color_);
    ReadMarkdownTextOverflowValue(map, ToString(MarkdownStyleOp::kTextOverflow),
                                  &style->text_overflow_);
    ReadIntValue(map, ToString(MarkdownStyleOp::kTextMaxline),
                 &style->text_maxline_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kLineSpace),
                    &style->line_space_);
    ReadMarkdownWordBreakValue(map, ToString(MarkdownStyleOp::kWordBreak),
                               &style->word_break_);
    ReadMarkdownDirectionValue(map, ToString(MarkdownStyleOp::kDirection),
                               &style->direction_);
    ReadMarkdownTextAlignValue(map, ToString(MarkdownStyleOp::kTextAlign),
                               &style->text_align_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kTextIndent),
                    &style->text_indent_);
    ReadMarkdownTextAlignValue(map,
                               ToString(MarkdownStyleOp::kLastLineAlignment),
                               &style->last_line_alignment_);
  }
  void ReadMarkdownBlockStylePart(const ValueMap& map,
                                  MarkdownBlockStylePart* style) {
    ReadLengthValue(map, ToString(MarkdownStyleOp::kMarginTop),
                    &style->margin_top_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kMarginBottom),
                    &style->margin_bottom_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kMarginLeft),
                    &style->margin_left_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kMarginRight),
                    &style->margin_right_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kPaddingTop),
                    &style->padding_top_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kPaddingBottom),
                    &style->padding_bottom_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kPaddingLeft),
                    &style->padding_left_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kPaddingRight),
                    &style->padding_right_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kMaxWidth),
                    &style->max_width_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kMaxHeight),
                    &style->max_height_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kMinWidth),
                    &style->min_width_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kMinHeight),
                    &style->min_height_);
  }
  void ReadMarkdownBorderStylePart(const ValueMap& map,
                                   MarkdownBorderStylePart* style) {
    ReadMarkdownBorderTypeValue(map, ToString(MarkdownStyleOp::kBorderType),
                                &style->border_type_);
    ReadColorValue(map, ToString(MarkdownStyleOp::kBorderColor),
                   &style->border_color_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kBorderWidth),
                    &style->border_width_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kBorderRadius),
                    &style->border_radius_);
  }
  void ReadMarkdownBackgroundStylePart(const ValueMap& map,
                                       MarkdownBackgroundStylePart* style) {
    ReadStringValue(map, ToString(MarkdownStyleOp::kBackgroundImage),
                    &style->background_image_);
    ReadStringValue(map, ToString(MarkdownStyleOp::kBackgroundSize),
                    &style->background_size_);
    ReadStringValue(map, ToString(MarkdownStyleOp::kBackgroundRepeat),
                    &style->background_repeat_);
    ReadStringValue(map, ToString(MarkdownStyleOp::kBackgroundPosition),
                    &style->background_position_);
  }
  void ReadMarkdownIndentStylePart(const ValueMap& map,
                                   MarkdownIndentStylePart* style) {
    ReadLengthValue(map, ToString(MarkdownStyleOp::kIndent), &style->indent_);
  }
  void ReadMarkdownOrderedListStylePart(const ValueMap& map,
                                        MarkdownOrderedListStylePart* style) {
    ReadMarkdownNumberTypeValue(map, ToString(MarkdownStyleOp::kNumberType),
                                &style->number_type_);
    ReadFontValue(map, ToString(MarkdownStyleOp::kNumberFont),
                  &style->number_font_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kNumberFontSize),
                    &style->number_font_size_);
    ReadColorValue(map, ToString(MarkdownStyleOp::kNumberColor),
                   &style->number_color_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kNumberMarginRight),
                    &style->number_margin_right_);
  }
  void ReadMarkdownUnorderedListStylePart(
      const ValueMap& map, MarkdownUnorderedListStylePart* style) {
    ReadMarkdownMarkTypeValue(map, ToString(MarkdownStyleOp::kMarkType),
                              &style->mark_type_);
    ReadColorValue(map, ToString(MarkdownStyleOp::kMarkColor),
                   &style->mark_color_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kMarkSize),
                    &style->mark_size_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kMarkMarginRight),
                    &style->mark_margin_right_);
  }
  void ReadMarkdownRefStylePart(const ValueMap& map,
                                MarkdownRefStylePart* style) {
    ReadMarkdownBackgroundTypeValue(map,
                                    ToString(MarkdownStyleOp::kBackgroundType),
                                    &style->background_type_);
  }
  void ReadMarkdownTypewriterCursorStylePart(
      const ValueMap& map, MarkdownTypewriterCursorStylePart* style) {
    ReadStringValue(map, ToString(MarkdownStyleOp::kCustomCursor),
                    &style->custom_cursor_);
    ReadMarkdownVerticalAlignValue(map,
                                   ToString(MarkdownStyleOp::kVerticalAlign),
                                   &style->vertical_align_);
  }
  void ReadMarkdownTruncationStylePart(const ValueMap& map,
                                       MarkdownTruncationStylePart* style) {
    ReadMarkdownTruncationTypeValue(map,
                                    ToString(MarkdownStyleOp::kTruncationType),
                                    &style->truncation_type_);
    ReadStringValue(map, ToString(MarkdownStyleOp::kContent), &style->content_);
  }
  void ReadMarkdownDecorationStylePart(const ValueMap& map,
                                       MarkdownDecorationStylePart* style) {
    ReadMarkdownTextDecorationStyleValue(
        map, ToString(MarkdownStyleOp::kTextDecorationStyle),
        &style->text_decoration_style_);
    ReadMarkdownTextDecorationLineValue(
        map, ToString(MarkdownStyleOp::kTextDecorationLine),
        &style->text_decoration_line_);
    ReadColorValue(map, ToString(MarkdownStyleOp::kTextDecorationColor),
                   &style->text_decoration_color_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kTextDecorationThickness),
                    &style->text_decoration_thickness_);
  }
  void ReadMarkdownScrollStylePart(const ValueMap& map,
                                   MarkdownScrollStylePart* style) {
    ReadBoolValue(map, ToString(MarkdownStyleOp::kScrollX), &style->scroll_x_);
  }
  void ReadMarkdownSizeStylePart(const ValueMap& map,
                                 MarkdownSizeStylePart* style) {
    ReadLengthValue(map, ToString(MarkdownStyleOp::kWidth), &style->width_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kHeight), &style->height_);
  }
  void ReadMarkdownLineStylePart(const ValueMap& map,
                                 MarkdownLineStylePart* style) {
    ReadLengthValue(map, ToString(MarkdownStyleOp::kWidth), &style->width_);
    ReadColorValue(map, ToString(MarkdownStyleOp::kColor), &style->color_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kRadius), &style->radius_);
    ReadLengthValue(map, ToString(MarkdownStyleOp::kShrink), &style->shrink_);
    ReadMarkdownLineTypeValue(map, ToString(MarkdownStyleOp::kLineType),
                              &style->line_type_);
  }
  void ReadMarkdownMarkerStylePart(const ValueMap& map,
                                   MarkdownMarkerStylePart* style) {
    ReadMarkdownMarkTypeValue(map, ToString(MarkdownStyleOp::kMarkType),
                              &style->mark_type_);
    ReadColorValue(map, ToString(MarkdownStyleOp::kColor), &style->color_);
  }
  void ReadMarkdownImageStylePart(const ValueMap& map,
                                  MarkdownImageStylePart* style) {
    ReadBoolValue(map, ToString(MarkdownStyleOp::kEnableAltText),
                  &style->enable_alt_text_);
    ReadStringValue(map, ToString(MarkdownStyleOp::kAltImage),
                    &style->alt_image_);
    ReadFloatValue(map, ToString(MarkdownStyleOp::kRadius), &style->radius_);
  }
  void ReadMarkdownAlignStylePart(const ValueMap& map,
                                  MarkdownAlignStylePart* style) {
    ReadMarkdownVerticalAlignValue(map,
                                   ToString(MarkdownStyleOp::kVerticalAlign),
                                   &style->vertical_align_);
  }
  void ReadMarkdownImageCaptionStylePart(const ValueMap& map,
                                         MarkdownImageCaptionStylePart* style) {
    ReadMarkdownCaptionPositionValue(
        map, ToString(MarkdownStyleOp::kCaptionPosition),
        &style->caption_position_);
  }
  void ReadMarkdownTableStylePart(const ValueMap& map,
                                  MarkdownTableStylePart* style) {
    ReadMarkdownTableBorderValue(map, ToString(MarkdownStyleOp::kTableBorder),
                                 &style->table_border_);
    ReadMarkdownTableBackgroundValue(
        map, ToString(MarkdownStyleOp::kTableBackground),
        &style->table_background_);
    ReadColorValue(map, ToString(MarkdownStyleOp::kBackgroundColor),
                   &style->background_color_);
    ReadColorValue(map, ToString(MarkdownStyleOp::kAltColor),
                   &style->alt_color_);
    ReadMarkdownTableSplitValue(map, ToString(MarkdownStyleOp::kTableSplit),
                                &style->table_split_);
  }
  void ReadMarkdownNormalTextStyle(const ValueMap& map,
                                   MarkdownNormalTextStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
  }
  void ReadMarkdownH1Style(const ValueMap& map, MarkdownH1Style* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
  }
  void ReadMarkdownH2Style(const ValueMap& map, MarkdownH2Style* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
  }
  void ReadMarkdownH3Style(const ValueMap& map, MarkdownH3Style* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
  }
  void ReadMarkdownH4Style(const ValueMap& map, MarkdownH4Style* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
  }
  void ReadMarkdownH5Style(const ValueMap& map, MarkdownH5Style* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
  }
  void ReadMarkdownH6Style(const ValueMap& map, MarkdownH6Style* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
  }
  void ReadMarkdownLinkStyle(const ValueMap& map, MarkdownLinkStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownDecorationStylePart(map, &style->decoration_);
  }
  void ReadMarkdownInlineCodeStyle(const ValueMap& map,
                                   MarkdownInlineCodeStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBorderStylePart(map, &style->border_);
    ReadMarkdownBlockStylePart(map, &style->block_);
  }
  void ReadMarkdownCodeBlockStyle(const ValueMap& map,
                                  MarkdownCodeBlockStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownBorderStylePart(map, &style->border_);
    ReadMarkdownScrollStylePart(map, &style->scroll_);
  }
  void ReadMarkdownQuoteStyle(const ValueMap& map, MarkdownQuoteStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownBorderStylePart(map, &style->border_);
    ReadMarkdownIndentStylePart(map, &style->indent_);
  }
  void ReadMarkdownOrderedListStyle(const ValueMap& map,
                                    MarkdownOrderedListStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownOrderedListStylePart(map, &style->ordered_list_);
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownIndentStylePart(map, &style->indent_);
  }
  void ReadMarkdownUnorderedListStyle(const ValueMap& map,
                                      MarkdownUnorderedListStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownUnorderedListStylePart(map, &style->unordered_list_);
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownIndentStylePart(map, &style->indent_);
  }
  void ReadMarkdownRefStyle(const ValueMap& map, MarkdownRefStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownRefStylePart(map, &style->ref_);
  }
  void ReadMarkdownTableStyle(const ValueMap& map, MarkdownTableStyle* style) {
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownBorderStylePart(map, &style->border_);
    ReadMarkdownScrollStylePart(map, &style->scroll_);
    ReadMarkdownTableStylePart(map, &style->table_);
  }
  void ReadMarkdownTableCellStyle(const ValueMap& map,
                                  MarkdownTableCellStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownAlignStylePart(map, &style->align_);
  }
  void ReadMarkdownSplitStyle(const ValueMap& map, MarkdownSplitStyle* style) {
    ReadMarkdownBorderStylePart(map, &style->border_);
    ReadMarkdownBlockStylePart(map, &style->block_);
  }
  void ReadMarkdownTypewriterCursorStyle(const ValueMap& map,
                                         MarkdownTypewriterCursorStyle* style) {
    ReadMarkdownTypewriterCursorStylePart(map, &style->typewriter_cursor_);
  }
  void ReadMarkdownMarkStyle(const ValueMap& map, MarkdownMarkStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownBorderStylePart(map, &style->border_);
    ReadMarkdownBackgroundStylePart(map, &style->background_);
  }
  void ReadMarkdownDoubleBracesStyle(const ValueMap& map,
                                     MarkdownDoubleBracesStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownBorderStylePart(map, &style->border_);
    ReadMarkdownBackgroundStylePart(map, &style->background_);
  }
  void ReadMarkdownTruncationStyle(const ValueMap& map,
                                   MarkdownTruncationStyle* style) {
    ReadMarkdownTruncationStylePart(map, &style->truncation_);
  }
  void ReadMarkdownListItemStyle(const ValueMap& map,
                                 MarkdownListItemStyle* style) {
    ReadMarkdownBlockStylePart(map, &style->block_);
  }
  void ReadMarkdownTableHeaderStyle(const ValueMap& map,
                                    MarkdownTableHeaderStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownAlignStylePart(map, &style->align_);
  }
  void ReadMarkdownImageStyle(const ValueMap& map, MarkdownImageStyle* style) {
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownSizeStylePart(map, &style->size_);
    ReadMarkdownImageStylePart(map, &style->image_);
  }
  void ReadMarkdownQuoteBorderLineStyle(const ValueMap& map,
                                        MarkdownQuoteBorderLineStyle* style) {
    ReadMarkdownLineStylePart(map, &style->line_);
  }
  void ReadMarkdownUnorderedListMarkerStyle(
      const ValueMap& map, MarkdownUnorderedListMarkerStyle* style) {
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownSizeStylePart(map, &style->size_);
    ReadMarkdownMarkerStylePart(map, &style->marker_);
    ReadMarkdownAlignStylePart(map, &style->align_);
  }
  void ReadMarkdownSpanStyle(const ValueMap& map, MarkdownSpanStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
    ReadMarkdownBorderStylePart(map, &style->border_);
    ReadMarkdownBackgroundStylePart(map, &style->background_);
    ReadMarkdownDecorationStylePart(map, &style->decoration_);
    ReadMarkdownAlignStylePart(map, &style->align_);
  }
  void ReadMarkdownOrderedListNumberStyle(
      const ValueMap& map, MarkdownOrderedListNumberStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownBlockStylePart(map, &style->block_);
  }
  void ReadMarkdownImageCaptionStyle(const ValueMap& map,
                                     MarkdownImageCaptionStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
    ReadMarkdownImageCaptionStylePart(map, &style->image_caption_);
  }
  void ReadMarkdownBoldStyle(const ValueMap& map, MarkdownBoldStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
  }
  void ReadMarkdownItalicStyle(const ValueMap& map,
                               MarkdownItalicStyle* style) {
    ReadMarkdownBaseStylePart(map, &style->base_);
  }
  void ReadMarkdownStyle(const ValueMap& map, MarkdownStyle* style) {
    auto value = map.find(ToString(MarkdownStyleTag::kNormalText));
    MarkdownStyleInitializer::InitialNormalTextStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownNormalTextStyle(value->second->AsMap(),
                                  &(style->normal_text_));
    }
    value = map.find(ToString(MarkdownStyleTag::kH1));
    MarkdownStyleInitializer::InitialH1Style(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownH1Style(value->second->AsMap(), &(style->h1_));
    }
    value = map.find(ToString(MarkdownStyleTag::kH2));
    MarkdownStyleInitializer::InitialH2Style(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownH2Style(value->second->AsMap(), &(style->h2_));
    }
    value = map.find(ToString(MarkdownStyleTag::kH3));
    MarkdownStyleInitializer::InitialH3Style(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownH3Style(value->second->AsMap(), &(style->h3_));
    }
    value = map.find(ToString(MarkdownStyleTag::kH4));
    MarkdownStyleInitializer::InitialH4Style(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownH4Style(value->second->AsMap(), &(style->h4_));
    }
    value = map.find(ToString(MarkdownStyleTag::kH5));
    MarkdownStyleInitializer::InitialH5Style(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownH5Style(value->second->AsMap(), &(style->h5_));
    }
    value = map.find(ToString(MarkdownStyleTag::kH6));
    MarkdownStyleInitializer::InitialH6Style(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownH6Style(value->second->AsMap(), &(style->h6_));
    }
    value = map.find(ToString(MarkdownStyleTag::kLink));
    MarkdownStyleInitializer::InitialLinkStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownLinkStyle(value->second->AsMap(), &(style->link_));
    }
    value = map.find(ToString(MarkdownStyleTag::kInlineCode));
    MarkdownStyleInitializer::InitialInlineCodeStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownInlineCodeStyle(value->second->AsMap(),
                                  &(style->inline_code_));
    }
    value = map.find(ToString(MarkdownStyleTag::kCodeBlock));
    MarkdownStyleInitializer::InitialCodeBlockStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownCodeBlockStyle(value->second->AsMap(), &(style->code_block_));
    }
    value = map.find(ToString(MarkdownStyleTag::kQuote));
    MarkdownStyleInitializer::InitialQuoteStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownQuoteStyle(value->second->AsMap(), &(style->quote_));
    }
    value = map.find(ToString(MarkdownStyleTag::kOrderedList));
    MarkdownStyleInitializer::InitialOrderedListStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownOrderedListStyle(value->second->AsMap(),
                                   &(style->ordered_list_));
    }
    value = map.find(ToString(MarkdownStyleTag::kUnorderedList));
    MarkdownStyleInitializer::InitialUnorderedListStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownUnorderedListStyle(value->second->AsMap(),
                                     &(style->unordered_list_));
    }
    value = map.find(ToString(MarkdownStyleTag::kRef));
    MarkdownStyleInitializer::InitialRefStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownRefStyle(value->second->AsMap(), &(style->ref_));
    }
    value = map.find(ToString(MarkdownStyleTag::kTable));
    MarkdownStyleInitializer::InitialTableStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownTableStyle(value->second->AsMap(), &(style->table_));
    }
    value = map.find(ToString(MarkdownStyleTag::kTableCell));
    MarkdownStyleInitializer::InitialTableCellStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownTableCellStyle(value->second->AsMap(), &(style->table_cell_));
    }
    value = map.find(ToString(MarkdownStyleTag::kSplit));
    MarkdownStyleInitializer::InitialSplitStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownSplitStyle(value->second->AsMap(), &(style->split_));
    }
    value = map.find(ToString(MarkdownStyleTag::kTypewriterCursor));
    MarkdownStyleInitializer::InitialTypewriterCursorStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownTypewriterCursorStyle(value->second->AsMap(),
                                        &(style->typewriter_cursor_));
    }
    value = map.find(ToString(MarkdownStyleTag::kMark));
    MarkdownStyleInitializer::InitialMarkStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownMarkStyle(value->second->AsMap(), &(style->mark_));
    }
    value = map.find(ToString(MarkdownStyleTag::kDoubleBraces));
    MarkdownStyleInitializer::InitialDoubleBracesStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownDoubleBracesStyle(value->second->AsMap(),
                                    &(style->double_braces_));
    }
    value = map.find(ToString(MarkdownStyleTag::kTruncation));
    MarkdownStyleInitializer::InitialTruncationStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownTruncationStyle(value->second->AsMap(),
                                  &(style->truncation_));
    }
    value = map.find(ToString(MarkdownStyleTag::kListItem));
    MarkdownStyleInitializer::InitialListItemStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownListItemStyle(value->second->AsMap(), &(style->list_item_));
    }
    value = map.find(ToString(MarkdownStyleTag::kTableHeader));
    MarkdownStyleInitializer::InitialTableHeaderStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownTableHeaderStyle(value->second->AsMap(),
                                   &(style->table_header_));
    }
    value = map.find(ToString(MarkdownStyleTag::kImage));
    MarkdownStyleInitializer::InitialImageStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownImageStyle(value->second->AsMap(), &(style->image_));
    }
    value = map.find(ToString(MarkdownStyleTag::kQuoteBorderLine));
    MarkdownStyleInitializer::InitialQuoteBorderLineStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownQuoteBorderLineStyle(value->second->AsMap(),
                                       &(style->quote_border_line_));
    }
    value = map.find(ToString(MarkdownStyleTag::kUnorderedListMarker));
    MarkdownStyleInitializer::InitialUnorderedListMarkerStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownUnorderedListMarkerStyle(value->second->AsMap(),
                                           &(style->unordered_list_marker_));
    }
    for (auto& [key, val] : map) {
      if (key.empty() || key[0] != '.' || val->GetType() != ValueType::kMap)
        continue;
      MarkdownSpanStyle span_style;
      MarkdownStyleInitializer::InitializeSpanStyle(&span_style);
      ReadMarkdownSpanStyle(val->AsMap(), &span_style);
      style->span_styles_.emplace(key.substr(1), span_style);
    }
    value = map.find(ToString(MarkdownStyleTag::kOrderedListNumber));
    MarkdownStyleInitializer::InitialOrderedListNumberStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownOrderedListNumberStyle(value->second->AsMap(),
                                         &(style->ordered_list_number_));
    }
    value = map.find(ToString(MarkdownStyleTag::kImageCaption));
    MarkdownStyleInitializer::InitialImageCaptionStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownImageCaptionStyle(value->second->AsMap(),
                                    &(style->image_caption_));
    }
    value = map.find(ToString(MarkdownStyleTag::kBold));
    MarkdownStyleInitializer::InitialBoldStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownBoldStyle(value->second->AsMap(), &(style->bold_));
    }
    value = map.find(ToString(MarkdownStyleTag::kItalic));
    MarkdownStyleInitializer::InitialItalicStyle(style);
    if (value != map.end() && value->second->GetType() == ValueType::kMap) {
      ReadMarkdownItalicStyle(value->second->AsMap(), &(style->italic_));
    }
  }
  // AUTO GEN END
  void ReadFontValue(const ValueMap& map, const std::string& key,
                     std::string* result) {
    auto it = map.find(key);
    if (it != map.end()) {
      if (it->second->GetType() == ValueType::kString) {
        *result = it->second->AsString();
      }
    }
  }
  void ReadColorValue(const ValueMap& map, const std::string& key,
                      uint32_t* result) {
    auto it = map.find(key);
    if (it != map.end()) {
      if (it->second->GetType() == ValueType::kString) {
        std::string_view color_str = it->second->AsString();
        *result = ConvertColor(color_str);
      }
    }
  }
  static uint32_t ConvertColor(std::string_view color_str) {
    if (color_str[0] == '#')
      color_str = color_str.substr(1);
    uint32_t color_int = strtoul(color_str.data(), nullptr, 16);
    if (color_str.length() <= 6) {
      color_int |= 0xff000000;
    }
    return color_int;
  }
  void ReadIntValue(const ValueMap& map, const std::string& key,
                    int32_t* result) {
    auto it = map.find(key);
    if (it != map.end()) {
      *result = it->second->GetInt();
    }
  }
  void ReadFloatValue(const ValueMap& map, const std::string& key,
                      float* result) {
    auto it = map.find(key);
    if (it != map.end()) {
      *result = it->second->GetDouble();
    }
  }
  void ReadLengthValue(const ValueMap& map, const std::string& key,
                       float* result) {
    auto it = map.find(key);
    if (it != map.end()) {
      *result = MarkdownLengthValue::FromDp(it->second->GetDouble()).GetPx();
    }
  }
  void ReadStringValue(const ValueMap& map, const std::string& key,
                       std::string* result) {
    auto it = map.find(key);
    if (it != map.end()) {
      *result = it->second->GetString();
    }
  }
  void ReadBoolValue(const ValueMap& map, const std::string& key,
                     bool* result) {
    auto it = map.find(key);
    if (it != map.end()) {
      *result = it->second->GetBool();
    }
  }

  void ReadMarkdownLengthValue(const ValueMap& map, const std::string& key,
                               std::unique_ptr<MarkdownStyleValue>* result) {
    auto it = map.find(key);
    if (it != map.end()) {
      if (it->second->GetType() == ValueType::kString) {
        *result = MarkdownStyleValue::ParseValue(it->second->AsString());
      } else if (it->second->GetType() == ValueType::kDouble) {
        *result = std::make_unique<MarkdownLengthValue>(it->second->GetDouble(),
                                                        StyleValuePattern::kPx);
      }
    }
  }

  void ReadGradientValue(const ValueMap& map, const std::string& key,
                         std::unique_ptr<tttext::RunDelegate>* result) {
    auto it = map.find(key);
    if (it != map.end()) {
      if (it->second->GetType() == ValueType::kString) {
        const auto& str = it->second->AsString();
        if (base::BeginsWith(str, "linear-gradient(") ||
            base::BeginsWith(str, "radial-gradient(")) {
          *result = loader_->LoadGradient(str.c_str(), 1, 1);
        } else {
          *result = nullptr;
        }
      }
    }
  }

  constexpr const char* ToString(CharIndexType index_type) {
    switch (index_type) {
      case CharIndexType::kParsedContent:
        return "char";
      case CharIndexType::kSource:
        return "source";
    }
    return "";
  }

  void ReadCharIndexType(const ValueMap& map, const std::string& key,
                         CharIndexType* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 CharIndexType::kParsedContent,
                 CharIndexType::kSource,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }

  constexpr const char* ToString(AttachmentLayer layer) {
    switch (layer) {
      case AttachmentLayer::kBackground:
        return "background";
      case AttachmentLayer::kForeGround:
        return "foreground";
    }
    return "";
  }

  void ReadAttachmentLayerType(const ValueMap& map, const std::string& key,
                               AttachmentLayer* result) {
    auto find = map.find(key);
    if (find != map.end()) {
      auto* value = find->second.get();
      if (value->GetType() == ValueType::kString) {
        for (auto item : {
                 AttachmentLayer::kBackground,
                 AttachmentLayer::kForeGround,
             }) {
          if (value->AsString() == ToString(item)) {
            *result = item;
            return;
          }
        }
      }
    }
  }

  void ReadMarkdownAttachment(const ValueMap& map,
                              MarkdownTextAttachment* attachment) {
    ReadIntValue(map, "startIndex", &(attachment->start_index_));
    ReadIntValue(map, "endIndex", &(attachment->end_index_));
    ReadCharIndexType(map, "indexType", &(attachment->index_type_));
    ReadAttachmentLayerType(map, "layer", &(attachment->attachment_layer_));
    ReadStringValue(map, "id", &(attachment->id_));
    ReadBoolValue(map, "clickable", &(attachment->clickable_));
    auto find = map.find("style");
    if (find != map.end() && find->second->GetType() == ValueType::kMap) {
      ReadMarkdownAttachmentStyle(find->second->AsMap(), attachment);
    }
  }

  void ReadMarkdownAttachmentStyle(const ValueMap& map,
                                   MarkdownTextAttachment* attachment) {
    ReadMarkdownLengthValue(map, "left", &(attachment->rect_.left_));
    ReadMarkdownLengthValue(map, "top", &(attachment->rect_.top_));
    ReadMarkdownLengthValue(map, "right", &(attachment->rect_.right_));
    ReadMarkdownLengthValue(map, "bottom", &(attachment->rect_.bottom_));
    ReadMarkdownLengthValue(map, "radius", &(attachment->rect_.radius_));
    ReadGradientValue(map, "color", &(attachment->rect_.gradient_));
    if (attachment->rect_.gradient_ == nullptr) {
      ReadColorValue(map, "color", &(attachment->rect_.color_));
    }
    auto find = map.find("borderLeft");
    if (find != map.end() && find->second->GetType() == ValueType::kMap) {
      ReadMarkdownAttachmentLineStyle(find->second->AsMap(),
                                      &(attachment->border_left_));
    }
    find = map.find("borderTop");
    if (find != map.end() && find->second->GetType() == ValueType::kMap) {
      ReadMarkdownAttachmentLineStyle(find->second->AsMap(),
                                      &(attachment->border_top_));
    }
    find = map.find("borderRight");
    if (find != map.end() && find->second->GetType() == ValueType::kMap) {
      ReadMarkdownAttachmentLineStyle(find->second->AsMap(),
                                      &(attachment->border_right_));
    }
    find = map.find("borderBottom");
    if (find != map.end() && find->second->GetType() == ValueType::kMap) {
      ReadMarkdownAttachmentLineStyle(find->second->AsMap(),
                                      &(attachment->border_bottom_));
    }
  }

  void ReadMarkdownAttachmentLineStyle(const ValueMap& map,
                                       MarkdownAttachmentLineStyle* line) {
    ReadMarkdownLineTypeValue(map, "lineType", &(line->line_type_));
    ReadMarkdownLengthValue(map, "width", &(line->width_));
    ReadGradientValue(map, "color", &(line->gradient_));
    if (line->gradient_ == nullptr) {
      ReadColorValue(map, "color", &(line->color_));
    }
    ReadMarkdownLengthValue(map, "elementSize", &(line->element_size_));
    ReadMarkdownLengthValue(map, "emptySize", &(line->empty_size_));
  }
};
MarkdownStyle MarkdownStyleReader::ReadStyle(
    const lynx::markdown::ValueMap& map, MarkdownResourceLoader* loader) {
  MarkdownStyleReaderImpl impl(loader);
  MarkdownStyle style;
  impl.ReadMarkdownStyle(map, &style);
  return style;
}
std::vector<std::unique_ptr<MarkdownTextAttachment>>
MarkdownStyleReader::ReadTextAttachments(Value* array,
                                         MarkdownDocument* document) {
  if (array->GetType() != ValueType::kArray) {
    return {};
  }
  std::vector<std::unique_ptr<MarkdownTextAttachment>> attachments;
  auto& values = array->AsArray();
  for (auto& value : values) {
    if (value->GetType() == ValueType::kMap) {
      MarkdownStyleReaderImpl impl(document->GetResourceLoader());
      auto attachment = std::make_unique<MarkdownTextAttachment>();
      impl.ReadMarkdownAttachment(value->AsMap(), attachment.get());
      if (attachment->index_type_ == CharIndexType::kSource) {
        attachment->start_index_ =
            document->MarkdownOffsetToCharOffset(attachment->start_index_);
        attachment->end_index_ =
            document->MarkdownOffsetToCharOffset(attachment->end_index_);
      }
      attachments.emplace_back(std::move(attachment));
    }
  }
  return attachments;
}
MarkdownBaseStylePart MarkdownStyleReader::ReadBaseStyle(
    const ValueMap& map, MarkdownResourceLoader* loader) {
  MarkdownStyleReaderImpl impl(loader);
  MarkdownBaseStylePart base_style_part;
  impl.ReadMarkdownBaseStylePart(map, &base_style_part);
  return base_style_part;
}
uint32_t MarkdownStyleReader::ReadColor(const std::string& color) {
  return MarkdownStyleReaderImpl::ConvertColor(color);
}

}  // namespace lynx::markdown
