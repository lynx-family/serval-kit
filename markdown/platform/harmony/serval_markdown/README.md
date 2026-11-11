<div>

## Introduction

serval-markdown is a cross-platform markdown rendering library supporting iOS, Android, and HarmonyOS.

## Installation

```bash
ohpm install @lynx/serval_markdown
```

## How to use

You can use serval-markdown in your HarmonyOS project by adding the dependency in `oh-package.json5`:

```json5
{
  "dependencies": {
    "@lynx/serval_markdown": "0.0.28"
  }
}
```

</div>

## Native host integration

`NativeMarkdownMeasurer` owns the document independently of its ArkUI view.
A host that owns layout and rendering on separate threads should:

1. Create and update the measurer on the layout thread. Install native resource
   and event adapters before measuring. Resource adapters used off the UI thread
   must avoid creating or measuring ArkUI nodes there.
2. Call `NativeServalMarkdownView::Create(false)` on the UI thread and attach the
   measurer with `SetMeasurer()`. One measurer can be attached to one view at a
   time. Keep it alive until the view has detached. Each gesture callback holds
   the view until it returns. The view unregisters its gestures on destruction.
3. Call `DisableInternalVSync(true)` and `SetExternalMeasure(true)` on the UI
   thread. Both options are explicit; standalone views retain their existing
   automatic layout and frame handling by default.
4. Run `Measure()` and `Align()` on the layout thread. Pass the returned
   `MeasureResult` to the UI thread and call `SetExternalMeasuredSize()` there.
   Dimensions are physical pixels. ArkUI consumes this snapshot without
   entering the document's measurement or alignment paths.
5. Drive `NativeMarkdownMeasurer::OnLayoutFrame()` on the layout thread and
   `NativeServalMarkdownView::OnRendererFrame()` on the UI thread. Both accept
   nanosecond timestamps. Continue supplying layout timestamps while animation
   is paused so that resuming excludes the elapsed paused time.
6. Stop layout operations that use the attached view before calling
   `DetachMeasurer()` on the UI thread. The document can then survive view
   destruction and attach to a replacement view. View creation, attachment,
   detachment, frame registration and external-size updates belong to the UI
   thread. Host callbacks must dispatch to their owning threads.

To control the visible render region, call `SetExternalRenderRegion(true)` on
the UI thread, then call `SetRenderRegion(RectF::MakeLTRB(left, top, right,
bottom))` before `OnRendererFrame()`. The rectangle uses view-local coordinates
in physical pixels and is retained across frames until replaced. It also drives
image and link exposure checks. Call `SetExternalRenderRegion(false)` to resume
automatic screen-based region calculation on the next frame. Automatic region
updates are enabled by default and are independent of the VSync and measurement
options above.

`PauseAnimation()` / `ResumeAnimation(step)` control the layout animation
clock. `MarkdownView::PauseRenderUpdate()` freezes the displayed snapshot, and
`GetRenderedAnimationStep()` identifies the step actually shown. Hosts can use
both controls to implement the same pause/resume behavior as Android and iOS.
`OnImageLoaded()` and `OnFontLoaded()` invalidate layout and request a host
measurement. Native font loaders implement `LoadFont(family, weight, style)`
with numeric weight and style.

## Native configuration and queries

`NativeMarkdownMeasurer::SetConfig()` accepts content completion/ID, animation
frame rate, dynamic-height transitions, exposure tags, text attachments and
markdown effects. Updates apply only the supplied fields. Native hosts can also
use `SetNumberProp()`, `SetStringProp()`, `SetArrayProp()` and `SetMapProp()`.

Read content/range queries, selection geometry, hit testing and image/link
queries through `GetMarkdownView()` on the UI thread. Document changes,
measurement, alignment and layout frames belong to the layout thread.
