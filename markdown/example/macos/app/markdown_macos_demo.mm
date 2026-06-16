// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <textra/fontmgr_collection.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/utils/markdown_value.h"
#include "markdown/view/markdown_view.h"
#include "mtl_app.h"
#include "skity_demo_font_manager.h"
#include "skity_markdown_canvas.h"
#include "testing/markdown/markdown_case_builder.h"

#ifndef MARKDOWN_CASES_ROOT
#define MARKDOWN_CASES_ROOT ""
#endif

namespace serval::markdown::example {
namespace {
namespace fs = std::filesystem;

using MarkdownCaseBuilder = serval::markdown::testing::MarkdownCaseBuilder;
using MarkdownCaseConfig = serval::markdown::testing::MarkdownCaseConfig;
using MarkdownCaseEntry = serval::markdown::testing::MarkdownCaseEntry;
using ValuePtr = serval::markdown::testing::MarkdownCaseValuePtr;

constexpr float kDemoWidth = 720.f;
constexpr float kDemoHeight = 100000.f;
constexpr float kContentLeft = 40.f;
constexpr float kContentTop = 36.f;
constexpr float kContentBottom = 36.f;
constexpr float kScrollStep = 48.f;

int64_t NowMilliseconds() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

void AddString(ValueMap* map, const std::string& key, std::string value) {
  map->emplace(key, Value::MakeString(std::move(value)));
}

void AddDouble(ValueMap* map, const std::string& key, double value) {
  map->emplace(key, Value::MakeDouble(value));
}

void AddMap(ValueMap* map, const std::string& key, ValueMap value) {
  map->emplace(key, Value::MakeMap(std::move(value)));
}

ValueMap MakeDemoStyle() {
  ValueMap normal_text;
  AddString(&normal_text, "font", "Inter");
  AddDouble(&normal_text, "fontSize", 17);
  AddDouble(&normal_text, "lineHeight", 26);
  AddString(&normal_text, "color", "#1f2937");
  AddDouble(&normal_text, "paragraphSpace", 12);

  ValueMap h1;
  AddString(&h1, "font", "Inter");
  AddDouble(&h1, "fontSize", 34);
  AddDouble(&h1, "lineHeight", 42);
  AddString(&h1, "fontWeight", "bold");
  AddString(&h1, "color", "#111827");
  AddDouble(&h1, "paragraphSpace", 16);

  ValueMap h2;
  AddString(&h2, "font", "Inter");
  AddDouble(&h2, "fontSize", 24);
  AddDouble(&h2, "lineHeight", 32);
  AddString(&h2, "fontWeight", "bold");
  AddString(&h2, "color", "#111827");

  ValueMap inline_code;
  AddString(&inline_code, "font", "Menlo");
  AddDouble(&inline_code, "fontSize", 15);
  AddString(&inline_code, "color", "#9a3412");
  AddString(&inline_code, "backgroundColor", "#ffedd5");
  AddDouble(&inline_code, "borderRadius", 5);

  ValueMap code_block;
  AddString(&code_block, "font", "Menlo");
  AddDouble(&code_block, "fontSize", 14);
  AddDouble(&code_block, "lineHeight", 22);
  AddString(&code_block, "color", "#e5e7eb");
  AddString(&code_block, "backgroundColor", "#111827");
  AddDouble(&code_block, "borderRadius", 8);
  AddDouble(&code_block, "paddingTop", 14);
  AddDouble(&code_block, "paddingBottom", 14);
  AddDouble(&code_block, "paddingLeft", 16);
  AddDouble(&code_block, "paddingRight", 16);

  ValueMap quote;
  AddString(&quote, "backgroundColor", "#f8fafc");
  AddString(&quote, "borderColor", "#2dd4bf");
  AddDouble(&quote, "borderWidth", 4);
  AddDouble(&quote, "borderRadius", 8);
  AddDouble(&quote, "paddingTop", 12);
  AddDouble(&quote, "paddingBottom", 12);
  AddDouble(&quote, "paddingLeft", 16);
  AddDouble(&quote, "paddingRight", 16);

  ValueMap link;
  AddString(&link, "color", "#2563eb");

  ValueMap mark;
  AddString(&mark, "backgroundImage",
            "linear-gradient(180deg, rgba(251, 191, 36, 0.05) 0%, "
            "rgba(251, 191, 36, 0.75) 100%)");
  AddDouble(&mark, "borderRadius", 7);

  ValueMap table;
  AddString(&table, "borderColor", "#cbd5e1");
  AddDouble(&table, "borderWidth", 1);
  AddString(&table, "backgroundColor", "#ffffff");
  AddString(&table, "altColor", "#f8fafc");

  ValueMap style;
  AddMap(&style, "normalText", std::move(normal_text));
  AddMap(&style, "h1", std::move(h1));
  AddMap(&style, "h2", std::move(h2));
  AddMap(&style, "inlineCode", std::move(inline_code));
  AddMap(&style, "codeBlock", std::move(code_block));
  AddMap(&style, "quote", std::move(quote));
  AddMap(&style, "link", std::move(link));
  AddMap(&style, "mark", std::move(mark));
  AddMap(&style, "table", std::move(table));
  return style;
}

std::string DemoMarkdown() {
  return R"(# Serval Markdown on macOS

This demo renders **Markdown** with `skity` on top of a Metal surface. It uses the same markdown core as Android, iOS and Harmony, but the host view is a small GLFW macOS shell.

> The content below is laid out by Textra and painted through a Skity-backed `ICanvasHelper`. Backgrounds, rounded corners, bullets, tables and gradients are all rendered by the demo adapter.

## Rendering coverage

- Paragraph, bold, italic and inline code
- Ordered and unordered lists
- Quote blocks and code blocks
- Tables with alternating rows
- <span class="mark">Inline gradient background</span>

| Layer | Responsibility |
| --- | --- |
| MarkdownView | parse, layout, animation |
| Textra | text shaping and line layout |
| Skity | GPU drawing |
| Metal | macOS backend |

```cpp
SkityMarkdownCanvas canvas(GetCanvas());
markdown_view.Draw(&canvas, 0, 0);
```

Press Space to toggle the typewriter animation and R to restart it.)";
}

class MacMarkdownPlatform final : public MarkdownPlatform {
 public:
  MacMarkdownPlatform()
      : font_manager_(std::make_shared<SkityDemoFontManager>()),
        font_collection_(MakeFontCollection(font_manager_)),
        text_layout_(&font_collection_, tttext::kSelfRendering) {}

  tttext::TextLayout* GetTextLayout() override { return &text_layout_; }

  MarkdownCanvasExtend* GetMarkdownCanvasExtend(
      tttext::ICanvasHelper* canvas) override {
    return static_cast<SkityMarkdownCanvas*>(canvas);
  }

  SkityDemoFontManager* GetFontManager() const { return font_manager_.get(); }

 private:
  static tttext::FontmgrCollection MakeFontCollection(
      const std::shared_ptr<SkityDemoFontManager>& font_manager) {
    tttext::FontmgrCollection font_collection(nullptr);
    font_collection.SetAssetFontManager(font_manager);
    return font_collection;
  }

  std::shared_ptr<SkityDemoFontManager> font_manager_;
  tttext::FontmgrCollection font_collection_;
  tttext::TextLayout text_layout_;
};

class DemoImageDrawable final : public MarkdownDrawable {
 public:
  DemoImageDrawable(float desire_width, float desire_height, float max_width,
                    float max_height)
      : desire_width_(desire_width),
        desire_height_(desire_height),
        max_width_(max_width),
        max_height_(max_height) {}

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    canvas->FillRect(x, y, x + measure_result_.width_,
                     y + measure_result_.height_, 0xffe5e7eb);
    auto painter = canvas->CreatePainter();
    painter->SetStrokeColor(0xffcbd5e1);
    painter->SetStrokeWidth(1);
    canvas->DrawRect(x, y, x + measure_result_.width_,
                     y + measure_result_.height_, painter.get());
  }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    constexpr float kFallbackWidth = 160.f;
    constexpr float kFallbackHeight = 90.f;
    float width = desire_width_ > 0 ? desire_width_ : kFallbackWidth;
    float height = desire_height_ > 0 ? desire_height_ : kFallbackHeight;

    if (max_width_ > 0) {
      width = std::min(width, max_width_);
    }
    if (max_height_ > 0) {
      height = std::min(height, max_height_);
    }
    if (spec.width_mode_ != tttext::LayoutMode::kIndefinite &&
        spec.width_ > 0) {
      width = std::min(width, spec.width_);
    }
    if (spec.height_mode_ != tttext::LayoutMode::kIndefinite &&
        spec.height_ > 0) {
      height = std::min(height, spec.height_);
    }

    width = std::max(1.f, width);
    height = std::max(1.f, height);
    return {.width_ = width, .height_ = height, .baseline_ = height};
  }

 private:
  float desire_width_{0};
  float desire_height_{0};
  float max_width_{0};
  float max_height_{0};
};

class DemoResourceLoader final : public MarkdownResourceLoader {
 public:
  explicit DemoResourceLoader(SkityDemoFontManager* font_manager)
      : font_manager_(font_manager) {}

  std::shared_ptr<MarkdownDrawable> LoadImage(const char* src,
                                              float desire_width,
                                              float desire_height,
                                              float max_width,
                                              float max_height,
                                              float border_radius) override {
    (void)src;
    (void)border_radius;
    return std::make_shared<DemoImageDrawable>(desire_width, desire_height,
                                               max_width, max_height);
  }

  std::shared_ptr<MarkdownDrawable> LoadInlineView(const char* id_selector,
                                                   float max_width,
                                                   float max_height) override {
    (void)id_selector;
    (void)max_width;
    (void)max_height;
    return nullptr;
  }

  void* LoadFont(const char* family, MarkdownFontWeight weight) override {
    (void)weight;
    if (font_manager_ == nullptr) {
      return nullptr;
    }
    return font_manager_->GetPlatformFont(
        family == nullptr || family[0] == '\0' ? "sans-serif" : family);
  }

  std::shared_ptr<MarkdownDrawable> LoadReplacementView(
      void* ud, int32_t id, float max_width, float max_height) override {
    (void)ud;
    (void)id;
    (void)max_width;
    (void)max_height;
    return nullptr;
  }

 private:
  SkityDemoFontManager* font_manager_{nullptr};
};

class DemoPlatformView : public MarkdownPlatformView,
                         public MarkdownCustomViewHandle {
 public:
  DemoPlatformView() = default;
  ~DemoPlatformView() override = default;

  void AttachDrawable(std::shared_ptr<MarkdownDrawable> drawable) override {
    MarkdownCustomViewHandle::AttachDrawable(std::move(drawable));
  }

  void RequestMeasure() override { needs_measure_ = true; }
  void RequestAlign() override { needs_align_ = true; }
  void RequestDraw() override { needs_draw_ = true; }

  void Align(float left, float top) override {
    align_position_ = {left, top};
    if (drawable_ != nullptr) {
      drawable_->Align(left, top);
    }
  }

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    if (visible_ && drawable_ != nullptr) {
      canvas->Save();
      canvas->Translate(x, y);
      if (measured_size_.width_ > 0 && measured_size_.height_ > 0) {
        canvas->ClipRect(0, 0, measured_size_.width_, measured_size_.height_,
                         true);
      }
      drawable_->Draw(canvas, 0, 0);
      canvas->Restore();
    }
    needs_draw_ = false;
  }

  PointF GetAlignedPosition() override { return align_position_; }
  SizeF GetMeasuredSize() override { return measured_size_; }
  void SetMeasuredSize(SizeF size) override { measured_size_ = size; }
  void SetAlignPosition(PointF position) override { align_position_ = position; }
  void SetVisibility(bool visible) override { visible_ = visible; }
  MarkdownCustomViewHandle* GetCustomViewHandle() override { return this; }

  bool TakeNeedsMeasure() { return std::exchange(needs_measure_, false); }
  bool TakeNeedsAlign() { return std::exchange(needs_align_, false); }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    if (drawable_ == nullptr) {
      return {};
    }
    const auto result = drawable_->Measure(spec);
    measured_size_ = {result.width_, result.height_};
    return result;
  }

 private:
  PointF align_position_;
  SizeF measured_size_;
  bool visible_{true};
  bool needs_measure_{true};
  bool needs_align_{true};
  bool needs_draw_{true};
};

class DemoMainView final : public DemoPlatformView,
                           public MarkdownViewContainerHandle {
 public:
  DemoMainView() = default;
  ~DemoMainView() override = default;

  std::shared_ptr<MarkdownPlatformView> CreateCustomSubView() override {
    auto view = std::make_shared<DemoPlatformView>();
    subviews_.emplace_back(view);
    return view;
  }

  std::shared_ptr<MarkdownPlatformView> CreateSelectionHandleSubView(
      SelectionHandleType type, float size, uint32_t color) override {
    (void)type;
    (void)size;
    (void)color;
    return CreateCustomSubView();
  }

  std::shared_ptr<MarkdownPlatformView> CreateSelectionHighlightSubView(
      uint32_t color) override {
    (void)color;
    return CreateCustomSubView();
  }

  void RemoveSubView(MarkdownPlatformView* subview) override {
    subviews_.erase(std::remove_if(subviews_.begin(), subviews_.end(),
                                   [subview](const auto& item) {
                                     return item.get() == subview;
                                   }),
                    subviews_.end());
  }

  void RemoveAllSubViews() override { subviews_.clear(); }

  RectF GetViewRectInScreen() override {
    const auto size = GetMeasuredSize();
    return RectF::MakeLTWH(0, 0, size.width_, size.height_);
  }

  MarkdownViewContainerHandle* GetViewContainerHandle() override {
    return this;
  }

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    DemoPlatformView::Draw(canvas, x, y);
    for (const auto& subview : subviews_) {
      if (subview != nullptr) {
        const auto position = subview->GetAlignedPosition();
        subview->Draw(canvas, x + position.x_, y + position.y_);
      }
    }
  }

 private:
  std::vector<std::shared_ptr<MarkdownPlatformView>> subviews_;
};

class MarkdownDemoApp final : public MTLApp {
 public:
  MarkdownDemoApp()
      : MTLApp(900, 720, "Serval Markdown macOS Demo",
               {0.965f, 0.976f, 0.992f, 1.f}) {}
  ~MarkdownDemoApp() override = default;

 protected:
  void OnStart() override {
    auto platform = std::make_unique<MacMarkdownPlatform>();
    auto* font_manager = platform->GetFontManager();
    resource_loader_ = std::make_unique<DemoResourceLoader>(font_manager);
    auto context = std::make_shared<MarkdownContext>(std::move(platform));

    markdown_view_ = std::make_unique<MarkdownView>(&root_view_, context);
    root_view_.AttachDrawable(
        std::shared_ptr<MarkdownDrawable>(markdown_view_.get(),
                                          [](MarkdownDrawable*) {}));
    markdown_view_->SetResourceLoader(resource_loader_.get());
    markdown_view_->SetTypewriterDynamicHeight(false);
    markdown_view_->SetPaddings(0, 0, 0, 0);
    LoadCases();
    if (cases_.empty()) {
      ApplyFallbackContent();
    } else {
      ApplyCase(case_index_);
    }
    last_tick_ = NowMilliseconds();
  }

  void OnUpdate(float) override {
    const auto now = NowMilliseconds();
    if (animation_enabled_) {
      markdown_view_->OnLayoutFrame(now);
    }

    MeasureAndAlignIfNeeded();
    SyncViewportState();
    markdown_view_->OnRendererFrame(now);

    SkityMarkdownCanvas canvas(GetCanvas());
    canvas.Save();
    canvas.ClipRect(kContentLeft, kContentTop, kContentLeft + LayoutWidth(),
                    kContentTop + ViewportHeight(), true);
    canvas.Translate(kContentLeft, kContentTop - scroll_y_);
    root_view_.Draw(&canvas, 0, 0);
    canvas.Restore();

    last_tick_ = now;
  }

  void OnHandleKey(int key, int scancode, int action, int mods) override {
    (void)scancode;
    (void)mods;
    if (action != GLFW_RELEASE) {
      return;
    }
    switch (key) {
      case GLFW_KEY_SPACE:
        animation_enabled_ = !animation_enabled_;
        break;
      case GLFW_KEY_R:
        markdown_view_->SetAnimationStep(0);
        animation_enabled_ = true;
        break;
      case GLFW_KEY_N:
      case GLFW_KEY_RIGHT_BRACKET:
        SelectCase(1);
        break;
      case GLFW_KEY_P:
      case GLFW_KEY_LEFT_BRACKET:
        SelectCase(-1);
        break;
      case GLFW_KEY_DOWN:
        ScrollBy(kScrollStep);
        break;
      case GLFW_KEY_UP:
        ScrollBy(-kScrollStep);
        break;
      case GLFW_KEY_PAGE_DOWN:
        ScrollBy(ViewportHeight() * 0.85f);
        break;
      case GLFW_KEY_PAGE_UP:
        ScrollBy(-ViewportHeight() * 0.85f);
        break;
      case GLFW_KEY_HOME:
        ScrollTo(0.f);
        break;
      case GLFW_KEY_END:
        ScrollTo(content_height_);
        break;
      default:
        MTLApp::OnHandleKey(key, scancode, action, mods);
        break;
    }
  }

  void OnScroll(double offset_x, double offset_y) override {
    (void)offset_x;
    ScrollBy(-static_cast<float>(offset_y) * kScrollStep);
  }

 private:
  void LoadCases() {
    const fs::path cases_root(MARKDOWN_CASES_ROOT);
    default_attributes_ = MarkdownCaseBuilder::ReadJsonFileToValue(
        cases_root / "template" / "attributes.json");
    cases_ = MarkdownCaseBuilder::LoadCases(cases_root);
    const auto default_case =
        std::find_if(cases_.begin(), cases_.end(), [](const auto& item) {
          return item.name_ == "normal_basic_document";
        });
    if (default_case != cases_.end()) {
      case_index_ = static_cast<size_t>(
          std::distance(cases_.begin(), default_case));
    }
    std::fprintf(stderr, "loaded %zu markdown demo cases from %s\n",
                 cases_.size(), cases_root.string().c_str());
  }

  void ApplyFallbackContent() {
    content_width_ = kDemoWidth;
    viewport_height_ = kDemoHeight;
    markdown_view_->SetSourceType(SourceType::kMarkdown);
    markdown_view_->SetTextMaxLines(-1);
    markdown_view_->SetStyle(MakeDemoStyle());
    markdown_view_->SetContent(DemoMarkdown());
    markdown_view_->SetContentComplete(true);
    markdown_view_->SetTextAttachments(nullptr);
    markdown_view_->SetAnimationType(MarkdownAnimationType::kTypewriter);
    markdown_view_->SetAnimationVelocity(80);
    markdown_view_->SetInitialAnimationStep(0);
    markdown_view_->SetAnimationStep(0);
    animation_enabled_ = true;
    scroll_y_ = 0.f;
    content_height_ = 0.f;
    root_view_.RequestMeasure();
    root_view_.RequestAlign();
    SetWindowTitle("Serval Markdown macOS Demo");
  }

  void ApplyCase(size_t index) {
    if (cases_.empty()) {
      return;
    }
    case_index_ = index % cases_.size();
    const auto& demo_case = cases_[case_index_];

    MarkdownCaseConfig attributes;
    attributes.width_ = kDemoWidth;
    attributes.height_ = kDemoHeight;
    attributes.animation_velocity_ = 80;
    MarkdownCaseBuilder builder(attributes);
    builder.ApplyAttributes(default_attributes_.get());
    builder.ApplyAttributes(demo_case.attributes_.get());

    content_width_ = std::max(1.f, attributes.width_);
    viewport_height_ = std::max(1.f, attributes.height_);
    markdown_view_->SetSourceType(attributes.source_type_);
    markdown_view_->SetTextMaxLines(attributes.max_lines_);
    markdown_view_->SetStyle(attributes.style_map_);
    markdown_view_->SetContent(demo_case.markdown_);
    markdown_view_->SetContentComplete(attributes.content_complete_);
    markdown_view_->SetTextAttachments(std::move(attributes.attachments_));
    markdown_view_->SetAnimationType(attributes.animation_type_);
    markdown_view_->SetAnimationVelocity(
        static_cast<float>(attributes.animation_velocity_));
    markdown_view_->SetInitialAnimationStep(attributes.animation_step_);
    markdown_view_->SetAnimationStep(attributes.animation_step_);
    animation_enabled_ =
        attributes.animation_type_ != MarkdownAnimationType::kNone;
    scroll_y_ = 0.f;
    content_height_ = 0.f;
    root_view_.RequestMeasure();
    root_view_.RequestAlign();

    const auto title = "Serval Markdown macOS Demo - " + demo_case.name_ +
                       " (" + std::to_string(case_index_ + 1) + "/" +
                       std::to_string(cases_.size()) + ")";
    SetWindowTitle(title);
    std::fprintf(stderr, "markdown demo case: %s\n",
                 demo_case.name_.c_str());
  }

  void SelectCase(int direction) {
    if (cases_.empty()) {
      return;
    }
    const auto size = static_cast<int>(cases_.size());
    const auto next = (static_cast<int>(case_index_) + direction + size) % size;
    ApplyCase(static_cast<size_t>(next));
  }

  void ScrollBy(float delta_y) { ScrollTo(scroll_y_ + delta_y); }

  void ScrollTo(float scroll_y) {
    scroll_y_ = scroll_y;
    SyncViewportState();
  }

  void MeasureAndAlignIfNeeded() {
    if (root_view_.TakeNeedsMeasure()) {
      const auto layout_width = LayoutWidth();
      const auto result = markdown_view_->Measure(
          {.width_ = layout_width,
           .width_mode_ = tttext::LayoutMode::kDefinite,
           .height_ = kDemoHeight,
           .height_mode_ = tttext::LayoutMode::kAtMost});
      content_height_ = std::max(result.height_, ViewportHeight());
      root_view_.SetMeasuredSize({layout_width, content_height_});
      ClampScrollY();
      root_view_.RequestAlign();
    }
    if (root_view_.TakeNeedsAlign()) {
      root_view_.Align(0, 0);
    }
  }

  void SyncViewportState() {
    ClampScrollY();
  }

  float ViewportHeight() const {
    const auto available_height =
        std::max(0.f, static_cast<float>(ScreenHeight()) - kContentTop -
                          kContentBottom);
    return std::min(viewport_height_, available_height);
  }

  float LayoutWidth() const {
    const auto available_width =
        std::max(1.f, static_cast<float>(ScreenWidth()) - kContentLeft * 2.f);
    return std::clamp(content_width_, 1.f, available_width);
  }

  void ClampScrollY() {
    const float max_scroll = std::max(0.f, content_height_ - ViewportHeight());
    scroll_y_ = std::clamp(scroll_y_, 0.f, max_scroll);
  }

 private:
  DemoMainView root_view_;
  std::unique_ptr<DemoResourceLoader> resource_loader_;
  std::unique_ptr<MarkdownView> markdown_view_;
  ValuePtr default_attributes_;
  std::vector<MarkdownCaseEntry> cases_;
  size_t case_index_{0};
  bool animation_enabled_{true};
  float content_width_{kDemoWidth};
  float viewport_height_{kDemoHeight};
  float content_height_{0.f};
  float scroll_y_{0.f};
  int64_t last_tick_{0};
};

}  // namespace
}  // namespace serval::markdown::example

int main(int, const char**) {
  serval::markdown::example::MarkdownDemoApp app;
  app.Run();
  return 0;
}
