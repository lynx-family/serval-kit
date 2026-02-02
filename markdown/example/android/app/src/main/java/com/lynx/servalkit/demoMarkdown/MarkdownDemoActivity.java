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
import com.lynx.markdown.Constants;
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
      "### Headings:\n"
      + "\n"
      + "## Heading level 2\n"
      + "#### Heading level 4\n"
      + "##### Heading level 5\n"
      + "\n"
      + "### Plain text:\n"
      + "\n"
      +
      "This is a paragraph containing **bold**, *italic*, ***bold + italic***, ~~strikethrough~~, \\`inline code\\`, mixed inline styles, and emoji: \uD83D\uDE04 \n"
      + "\n"
      + "### Blockquote:\n"
      + "\n"
      + "> Reading a good book is like talking to a noble soul. — Goethe\n"
      + "> \n"
      +
      "> The employment system is unfavorable to workers, but workers are powerless to break free from it. — Ruan Yifeng\n"
      + "\n"
      + "### Lists:\n"
      + "\n"
      + "1. Ordered list 1\n"
      + "    1. Nested ordered list 1\n"
      + "    2. Nested ordered list 2\n"
      + "    3. Nested ordered list 3\n"
      + "2. Ordered list 2\n"
      + "3. Ordered list 3\n"
      + "\n"
      + "----\n"
      + "\n"
      + "- Unordered list 1\n"
      + "    - Nested unordered list 1\n"
      + "    - Nested unordered list 2\n"
      + "        - Third‑level nested unordered list\n"
      + "    - Nested unordered list 3\n"
      + "- Unordered list 2\n"
      + "- Unordered list 3\n"
      + "\n"
      + "----\n"
      + "\n"
      + "100. Ordered list starting at 100\n"
      + "1. Ordered list item numbered 101\n"
      + "\n"
      + "----\n"
      + "\n"
      + "1. Ordered list\n"
      + "    - Unordered list nested inside ordered list 1\n"
      + "    - Unordered list 2\n"
      + "    - Unordered list 3\n"
      + "        1. Ordered list nested under unordered list\n"
      + "        2. Ordered list 2\n"
      + "        3. Ordered list 3\n"
      +
      "    1. Ordered list at same level as unordered list, displayed as unordered\n"
      + "    2. Ordered list 2 at same level, displayed as unordered\n"
      + "2. Ordered list 2\n"
      + "3. Ordered list 3\n"
      + "----\n"
      + "\n"
      + "### Code blocks:\n"
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
      + "### Table:\n"
      + "\n"
      + "|header1|header2|header3|\n"
      + "|:--|:--:|--:|\n"
      + "|text|**bold**|*italic*|\n"
      + "||text||\n"
      + "\n"
      + "### Images:\n"
      + "\n"
      + "- Large image, centered:\n"
      + "\n"
      + "![image description here]()\n"
      + "\n"
      + "- Invalid URL:\n"
      + "\n"
      + "![invalid image](http://invalid)\n"
      + "\n"
      + "- Inline image:\n"
      + "\n"
      +
      "This is a paragraph with an inline image sized 10x10 ![image description here]().\n"
      + "\n"
      + "### Links:\n"
      + "\n"
      + "Links appear in blue without click effects:\n"
      + "This is a paragraph that contains a [link](http://test.url)\n"
      + "\n"
      + "### Inline view:\n"
      + "\n"
      +
      "This is a paragraph embedding an inline view: ![](inlineview://baseline_align), and another inline view offset upward by 10px: ![](inlineview://vertical_align).\n"
      + "\n"
      +
      "1. Below is a block view, which should not be affected by the current ordered list indentation\n"
      + "![](blockview://block)\n"
      + "\n"
      + "### Gradient background:\n"
      + "\n"
      +
      "> {{The employment system is unfavorable to workers, but workers are powerless to break free from it.}}[[2]] — Ruan Yifeng\n"
      + "<mark>highlighted text</mark>";

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
    normalText.put("fontSize", 17);
    normalText.put("lineHeight", 22);

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

    // link style
    Map<String, Object> linkStyle = new HashMap<>();
    linkStyle.put("color", "0000ff");
    style.put("link", linkStyle);

    return style;
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    Markdown.init(null);
    setContentView(R.layout.activity_markdown_demo);

    Intent intent = getIntent();
    String content = intent.getStringExtra("content");

    ScrollView scrollView = findViewById(R.id.myScrollView);
    LinearLayout scrollContentLayout = findViewById(R.id.scrollContentLayout);

    mMarkdownView = new ServalMarkdownView(this);
    scrollContentLayout.addView(
        mMarkdownView,
        new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                      ViewGroup.LayoutParams.WRAP_CONTENT));
    // set markdown content
    mMarkdownView.setContent(mContent1);
    mMarkdownView.setStyle(generateMarkdownStyle());
    mMarkdownView.setAnimationType(Constants.ANIMATION_TYPE_TYPEWRITER);
    mMarkdownView.setAnimationVelocity(100);
    mMarkdownView.setInitialAnimationStep(4);
  }
}
