// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.servalkit.demoMarkdown;

import static com.lynx.servalkit.demoMarkdown.CustomDemoView.mCustomCursorId;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import androidx.appcompat.app.AppCompatActivity;
import com.lynx.markdown.IResourceLoader;
import com.lynx.markdown.Markdown;
import com.lynx.markdown.ServalMarkdownView;
import com.lynx.markdown.tttext.IRunDelegate;
import com.lynx.servalkit.R;
import java.util.HashMap;
import java.util.Map;

public class MarkdownDemoActivity extends AppCompatActivity {
  ServalMarkdownView mMarkdownView;

  String mContent1 =
      "比雅尼·斯特劳斯特鲁普博士在贝尔实验室工作期间在20世纪80年代发明并实现了C++。起初，这种语言被称作“C with Classes”（“包含‘类’的C语言”），作为C语言的增强版出现。随后，C++不断增加新特性。虚函数、运算符重载、多继承、标准模板库、异常处理、运行时类型信息、命名空间等概念逐渐纳入标准草案。1998年，国际标准组织颁布了C++程序设计语言的第一个国际标准ISO/IEC 14882:1998，目前最新标准为ISO/IEC 14882:2020。ISO/IEC 14882通称ISO C++。ISO C++主要包含了核心语言和标准库的规则。尽管从核心语言到标准库都有显著不同，ISO C++直接正式（normative）引用了ISO/IEC 9899（通称ISO C），且ISO C++标准库的一部分和ISO C的标准库的API完全相同，另有很小一部分和C标准库略有差异（例如，strcat等函数提供对const类型的重载）。这使得C和C++的标准库实现常常被一并提供，在核心语言规则很大一部分兼容的情况下，进一步确保用户通常较容易把符合ISO C的源程序不经修改或经极少修改直接作为C++源程序使用，也是C++语言继C语言之后流行的一个重要原因。\n"
      + "\n"
      + "### 以下是标题：\n"
      + "\n"
      + "## 二级标题\n"
      + "#### 四级标题\n"
      + "##### 五级标题\n"
      + "\n"
      + "### 以下是普通文本：\n"
      + "\n"
      +
      "这是一段普通文本，包含**粗体**、*斜体*、***粗体+斜体***、~~删除线~~、\\`inline code\\`，这些行内的样式，以及表情：\uD83D\uDE04 \n"
      + "\n"
      + "### 以下是引用块：\n"
      + "\n"
      + "> 读一本好书，就是在和高尚的人谈话。 ——歌德\n"
      + "> \n"
      + "> 雇用制度对工人不利，但工人根本无力摆脱这个制度。——阮一峰\n"
      + "\n"
      + "### 以下是列表：\n"
      + "\n"
      + "1. 有序列表1\n"
      + "    1. 嵌套有序列表1\n"
      + "    2. 嵌套有序列表2\n"
      + "    3. 嵌套有序列表3\n"
      + "2. 有序列表2\n"
      + "3. 有序列表3\n"
      + "\n"
      + "----\n"
      + "\n"
      + "- 无序列表1\n"
      + "    - 嵌套无序列表1\n"
      + "    - 嵌套无序列表2\n"
      + "        - 三级嵌套无序列表\n"
      + "    - 嵌套无序列表3\n"
      + "- 无需列表2\n"
      + "- 无需列表3\n"
      + "\n"
      + "----\n"
      + "\n"
      + "100. 序号从100开始的有序列表\n"
      + "1. 序号为101的有序里列表\n"
      + "\n"
      + "----\n"
      + "\n"
      + "1. 有序列表\n"
      + "    - 有序列表内嵌套无序列表1\n"
      + "    - 无序列表2\n"
      + "    - 无需列表3\n"
      + "        1. 无需列表下再嵌套有序列表\n"
      + "        2. 有序列表2\n"
      + "        3. 有序列表3\n"
      + "    1. 与无序列表同级的有序列表，表现为无序列表\n"
      + "    2. 与无序列表同级的有序列表2，表现为无序列表\n"
      + "2. 有序列表2\n"
      + "3. 有序列表3\n"
      + "----\n"
      + "\n"
      + "### 以下是代码块：\n"
      + "\n"
      + "\\`\\`\\` js\n"
      + "var foo = function (bar) {\n"
      + "  return bar++;\n"
      + "};\n"
      + "\\`\\`\\`\n"
      + "\n"
      + "    var foo = function (bar) {\n"
      + "    return bar++;\n"
      + "    };\n"
      + "\n"
      + "### 以下是表格：\n"
      + "\n"
      + "|header1|header2|header3|\n"
      + "|:--|:--:|--:|\n"
      + "|text|**bold**|*italic*|\n"
      + "||text||\n"
      + "\n"
      + "### 以下是图片：\n"
      + "\n"
      + "- 大图，居中显示：\n"
      + "\n"
      + "![这里写图片描述]()\n"
      + "\n"
      + "- 无效url：\n"
      + "\n"
      + "![无效图片](http://invalid)\n"
      + "\n"
      + "- 行内图：\n"
      + "\n"
      + "这是一段文本，里边图片指定了10*10的大小![这里写图片描述]()。\n"
      + "\n"
      + "### 以下是链接:\n"
      + "\n"
      + "链接显示为蓝色，无点击效果:\n"
      + "这是一段普通文本，里边有一个[链接](http://test.url)\n"
      + "\n"
      + "### 以下是inline-view：\n"
      + "\n"
      +
      "这是一段文本里边嵌套了inline-view：![](inlineview://baseline_align), 另一个向上偏移了10px的inline-view：![](inlineview://vertical_align)。\n"
      + "\n"
      + "1. 下面是一段block-view，不应该受到当前有序列表的缩进\n"
      + "![](blockview://block)\n"
      + "\n"
      + "### 以下是gradient背景：\n"
      + "\n"
      + "> {{雇用制度对工人不利，但工人根本无力摆脱这个制度。}}[[2]] ——阮一峰\n"
      + "<mark>高亮文本</mark>";

  String mContent2 =
      "[I'm an inline-style link](https://www.google.com)\n"
      + "\n"
      +
      "[I'm an inline-style link with title](https://www.google.com \"Google's Homepage\")\n"
      + "\n"
      +
      "[I'm a reference-style link][Arbitrary case-insensitive reference text]\n"
      + "\n"
      +
      "[I'm a relative reference to a repository file](../blob/master/LICENSE)\n"
      + "\n"
      + "[You can use numbers for reference-style link definitions][1]\n"
      + "\n"
      + "Or leave it empty and use the [link text itself].\n"
      + "\n"
      +
      "URLs and URLs in angle brackets will automatically get turned into links.\n"
      + "http://www.example.com or <http://www.example.com> and sometimes\n"
      + "example.com (but not on Github, for example).\n"
      + "\n"
      + "Some text to show that the reference links can follow later.\n"
      + "\n"
      + "[arbitrary case-insensitive reference text]: https://www.mozilla.org\n"
      + "[1]: http://slashdot.org\n"
      + "[link text itself]: http://www.reddit.com";
  HashMap generateMarkdownStyle() {
    HashMap<String, Object> style = new HashMap<>();

    Map<String, Object> normalText = new HashMap<>();
    normalText.put("font", "FZHei");
    normalText.put("fontSize", 22);
    normalText.put("lineHeight", 30);

    Map<String, Object> h1 = new HashMap<>();
    h1.put("fontSize", 50);
    h1.put("lineHeight", 70);
    h1.put("color", "ff0000");

    Map<String, Object> codeBlock = new HashMap<>();
    codeBlock.put("font", "Times");
    codeBlock.put("borderColor", "e8e8e8");

    Map<String, Object> unorderedList = new HashMap<>();
    unorderedList.put("markSize", 10);
    unorderedList.put("indent", 15);

    style.put("normalText", normalText);
    style.put("h1", h1);
    style.put("codeBlock", codeBlock);
    style.put("unorderedList", unorderedList);

    //link style
    Map<String, Object> linkStyle = new HashMap<>();
    linkStyle.put("color", "0000ff");
    style.put("link", linkStyle);

    //typewriteCursor style
    Map<String, Object> customCursorStyle = new HashMap<>();
    customCursorStyle.put("customCursor", mCustomCursorId);
    customCursorStyle.put("verticalAlign", "center");
    style.put("typewriterCursor", customCursorStyle);

    return style;
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    Markdown.init(null);
    setContentView(R.layout.activity_markdown_demo);

    Intent intent = getIntent();
    String content = intent.getStringExtra("content");

    Log.e("linxs", "content:" + content);

    ScrollView scrollView = findViewById(R.id.myScrollView);
    LinearLayout scrollContentLayout = findViewById(R.id.scrollContentLayout);

    mMarkdownView = new ServalMarkdownView(this);
    scrollContentLayout.addView(
        mMarkdownView,
        new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                      ViewGroup.LayoutParams.WRAP_CONTENT));
    //set markdown content
    mMarkdownView.setContent(mContent2);
  }
}
