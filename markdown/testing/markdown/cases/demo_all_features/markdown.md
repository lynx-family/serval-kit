# Markdown Feature Walkthrough

Paragraph text covers **bold**, _italic_, ***bold italic***, ~~delete~~, `inline code`, escaped \*stars\*, entities &amp; &lt;tag&gt;, decimal &#64;, {{double braces}}, [[double square brackets]], and [a nested **link**](https://example.com/path?a=1&b=2).

<p class="demo-box">HTML paragraph with a <span class="demo-warn">styled span</span><br>and an inline break.</p>

> Quote text can contain **inline styles**.
> It keeps multiple source lines in one quoted block.

- [ ] unchecked task
- [x] checked task
  - nested unordered item
  1. nested ordered item

1. ordered one
2. ordered two

---

```cpp
int add(int lhs, int rhs) {
  return lhs + rhs;
}
```

| Left | Center | Right |
|:--|:-:|--:|
| a \| b | <mark>marked cell</mark><br>next line | [cell link](url://cell) |
| ![](inlineview://baseline_align) | **bold cell** | *italic cell* |

![Captioned image](test.jpeg width=72 height=40 'Image caption')

![Invalid image alt fallback](invalid width=88 height=36 'Fallback caption')

Block view below:

![](blockview://block)

Final paragraph after block view.
