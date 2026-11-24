# serval-svg Deep Wiki

## Purpose

`serval-svg` is a cross-platform SVG renderer inside the `svg/` subproject of
`serval-kit`. The engine is split into:

- a platform-independent XML + SVG DOM layer
- a platform-independent node/render-state layer
- platform canvas backends for Android, iOS, Harmony, and Skity

This document is the human-readable companion to
`svg/docs/audit-matrix.json`. The audit matrix is optimized for internal
tooling and engineering analysis; this wiki explains the code structure,
platform split, and how the public `svg/docs/support-matrix.json` is derived.

## Repository Map

- `svg/src/parser/`
  XML parsing, DOM building, and SVG node construction.
- `svg/src/element/`
  Node classes, attribute parsing, inheritance, render-state assembly, masks,
  clip paths, gradients, filters, and text nodes.
- `svg/include/`
  Public headers and backend abstraction.
- `svg/platform/android/`
  JNI bridge and Java rendering implementation.
- `svg/platform/iOS/`
  Quartz 2D / TextKit backend.
- `svg/platform/harmony/servalsvg/`
  Harmony drawing backend.
- `svg/platform/skity/`
  Skity backend, including the only current filter-capable canvas.
- `svg/test_cases/`
  Small reference SVG samples.
- `svg/examples/`
  Larger example assets and app integrations.
- `svg/tools/svg_support_audit.py`
  Knowledge-system script for support matrix generation and SVG analysis.

## How Rendering Works

### 1. XML to DOM

`SrDOMParser` and `SrDOM` build a lightweight XML tree from SVG text.

Parsing has explicit resource limits: 16 MiB input, 32 MiB copied DOM storage,
1 MiB per copied string, 65,536 nodes (including text), and 128 tree levels.
An element may have at most 256 attributes. Exceeding a limit or failing an
allocation aborts the build and cleans up the partial tree. XML cleanup does not
allocate. Attribute ownership moves only after container capacity is available.

The shared decimal-number parser rejects malformed, non-finite, and
unrepresentable geometry values. Path commands and transform argument lists
must be complete before their output is committed; invalid path/polygon data
is discarded. Build diagnostic sinks are detached after attribute parsing so
later animation evaluation cannot retain stack pointers from construction.
Portable sanitizer regressions are documented in `svg/testing/README.md`.

Key files:

- `svg/src/parser/SrDOMParser.cc`
- `svg/src/parser/SrDOM.cc`
- `svg/src/parser/SrXMLParser.cc`

The shared parser is implemented in `SrSVGParse.c`: a bounded cursor, typed
outputs, and a nonallocating `{code, offset}` error distinguish invalid data,
range overflow, resource limits, and OOM. The legacy C wrappers retain their
signatures. `SrSVGMemory.c` owns checked growth and allocation, and
`SrSVGArray.h` provides fallible scratch arrays for the XML parser. A path command
is published only after complete validation and both buffer reservations;
whole paths/polygons transfer ownership only after parsing succeeds.

`SrSVGArc.c` is the allocation-free iOS/Harmony arc conversion. It uses fixed
storage, bounded radius correction/subdivision, finite output checks, absolute
radii, and a line fallback for zero radii or unrepresentable geometry. Platform
backends only emit the resulting curves. Arc commands remain in SrPathData until
after SMIL interpolation, preserving command topology and flag compatibility.

No-exception OOM recovery currently applies to this shared parsing layer and
XML DOM with nonallocating callbacks. SVG node construction, diagnostic vectors,
SMIL strings/vectors/streams, and platform caches still use the standard library;
these can terminate on OOM when exceptions are disabled. Build success and
normal rendering equivalence do not prove whole-renderer OOM recovery. See
`svg/testing/README.md` for the allocation-failure tests and fuzz entry point.

### 2. DOM to SVG Nodes

`SrSVGDOM::construct_svg_node()` is the canonical tag registry for supported
SVG elements.

Key file:

- `svg/src/parser/SrSVGDOM.cc`

This file answers the first support question:

- Is a tag recognized at all?

Current recognized tags include:

- `svg`, `g`, `defs`
- `rect`, `circle`, `ellipse`, `line`, `polygon`, `polyline`, `path`
- `use`, `image`
- `text`, `tspan`
- `linearGradient`, `radialGradient`, `stop`
- `clipPath`, `mask`
- `filter`, `feGaussianBlur`, `feOffset`, `feColorMatrix`, `feComposite`,
  `feBlend`, `feFlood`

Anything not constructed here is not part of the active renderer surface.

### 3. Attribute Parsing

Support is distributed across the node hierarchy:

- `SrSVGNode.cc`
  common attributes such as `fill`, `stroke`, `opacity`, `clip-path`, `mask`,
  `filter`, `transform`, `style`, `color`
- `SrSVGShape.cc`
  shape-level `fill-rule` and render-state assembly
- per-tag files in `svg/src/element/*.cc`
  geometry and specialized attributes such as `path@d`,
  `linearGradient@gradientTransform`, `mask@maskContentUnits`, and
  `feGaussianBlur@stdDeviation`

Important consequence:

- A tag may be parser-recognized and attribute-parsed, but still not be fully
  rendered.

### 4. Inheritance and Referencing

Important inheritance / reference systems:

- groups and root containers propagate fill, stroke, color, mask, clip-path,
  stroke width, and opacity-related state to children
- reusable IDs are collected while parsing in `SrSVGDOM.cc`
- `url(#id)` references are resolved through an `IDMapper`
- gradients, masks, clip paths, filters, and `<use>` all depend on this map

Notable implementation detail:

- `<use>` is rendered in node code (`SrSVGUse.cc`) by resolving and rendering
  the referenced node directly, not through the platform `DrawUse()` method

## Support Model

The repository must be understood with four layers:

1. `recognized`
   The parser can instantiate a node.
2. `parsed`
   The node stores the relevant attributes.
3. `rendered`
   The render path actually consumes that node meaningfully.
4. `platform-ready`
   A concrete backend renders it on Android, iOS, Harmony, and/or Skity.

This layered model is necessary because the repository contains several
important mismatches:

- tags parsed but not rendered by any backend
- attributes parsed but ignored at render time
- features rendered only on one backend
- container semantics that are approximated rather than spec-complete

## Current Support Snapshot

### Strongest Current Surface

The most reliable cross-platform area is:

- root/container flow: `svg`, `g`
- basic shapes: `rect`, `circle`, `ellipse`, `line`, `polygon`, `polyline`,
  `path`
- reusable definitions: `defs`, `use`
- gradients: `linearGradient`, `radialGradient`, `stop`
- clipping and masking: `clipPath`, `mask`

These are also the areas best represented by sample assets in
`svg/test_cases/` and `svg/examples/`.

### Partial Surface

The main partial-support areas are:

- `image`
  Android / iOS / Skity can draw images, but Harmony currently cannot.
  Geometry currently uses raw stored length values in `SrSVGImage::onDraw()`,
  so non-px / percentage behavior is not fully audited.
- `text`, `tspan`
  Android, iOS, and Harmony have text bridges, but the support surface is much smaller
  than generic SVG presentation attributes suggest. Stroke-oriented text
  styling is not really rendered. The Skity text backend is still stubbed.
- `filter`, `feGaussianBlur`, `feOffset`, `feColorMatrix`
  The parser and node model recognize them, but actual filter execution is
  gated behind `SrCanvas::SupportsFilters()`, and only Skity enables that.
  Even on Skity, the implementation is closer to targeted heuristics than a
  full SVG filter graph.

### Parsed-Only Surface

These tags are currently parser-visible but not actually rendered:

- `feComposite`
- `feBlend`
- `feFlood`

Their attributes are parsed into node state, but the backends do not consume
them.

## Platform Snapshot

### Android

Strengths:

- shapes, gradients, masks, clip paths, text

Caveats:

- filters are disabled because the Android backend does not override
  `SupportsFilters()`
- images depend on an app-provided resource manager

Key files:

- `svg/platform/android/src/main/cpp/SrAndroidCanvas.cc`
- `svg/platform/android/src/main/java/SVGRender.java`
- `svg/platform/android/src/main/cpp/SrAndroidParagraphFactory.cc`

### iOS

Strengths:

- shapes, gradients, masks, clip paths, text

Caveats:

- filters are disabled because the iOS backend does not override
  `SupportsFilters()`
- images depend on an image callback

Key files:

- `svg/platform/iOS/SrIOSCanvas.mm`
- `svg/platform/iOS/SrIOSParagraph.mm`

### Harmony

Strengths:

- shapes, gradients, masks, clip paths, and images through a host fetcher

Current gaps:

- basic text and nested tspan use NativeDrawing typography with system fonts
- textPath, per-character positions, text stroke, and per-span transforms are not implemented
- filters are disabled because the Harmony backend does not override
  `SupportsFilters()`

Key files:

- `svg/platform/harmony/servalsvg/src/main/cpp/platform/harmony/sr_harmony_canvas.cc`
- `svg/include/platform/harmony/sr_harmony_paragraph.h`
- `svg/platform/harmony/servalsvg/src/main/cpp/platform/harmony/sr_harmony_paragraph.cc`

### Skity

Strengths:

- shapes, gradients, masks, clip paths, images
- the only backend with `SupportsFilters() == true`

## Host Color Contract

The renderer supports a host-provided default color for `currentColor`
resolution. This is an integration-level input, not an SVG document attribute.

Rules:

- If the SVG content defines `color` on the current node, use that value.
- Otherwise, if an ancestor provides inherited `color`, use the inherited
  value.
- Otherwise, if the host sets a default color, use the host value.
- Otherwise, fall back to black.

Compatibility notes:

- If the host does not provide a color, existing rendering behavior remains
  unchanged.
- Host default color only affects `currentColor` resolution. It does not
  override explicit `fill`, `stroke`, gradients, patterns, or `none`.
- Host background color must not be treated as the SVG default color unless
  product logic explicitly wants `currentColor` to follow that foreground
  value.

Public integration surface:

- Android: `SVGRender#setColor(@Nullable Long color)`
- iOS: `SrSVGView.color`
- Harmony: `SvgView.color?: number`

Harmony API note:

- `SvgData` does not expose `color`. `color` is intentionally exposed only on
  `SvgView` to avoid duplicated public entry points and precedence ambiguity.

Current gaps:

- text paragraphs are stubbed
- filter support is partial and heuristic-based

Key files:

- `svg/include/platform/skity/SrSkityCanvas.h`
- `svg/platform/skity/SrSkityCanvas.cc`
- `svg/include/platform/skity/SrSkityParagraph.h`

## Important Behavior Notes

### Root `<svg>` Width / Height

Root `width` and `height` are common in input files, but the current engine
renders against the externally supplied viewport context. In practice:

- `viewBox` and `preserveAspectRatio` are the real root geometry controls
- root `width` / `height` are not the primary driver of output sizing

### `<use>`

`<use>` is one of the more interesting implementations in this repo:

- parser support exists
- node support exists
- rendering works through node resolution and inheritance forwarding
- platform `DrawUse()` stubs are not the decisive implementation path

Known caveat:

- `use@width` and `use@height` are parsed but not applied in
  `SrSVGUse::renderRealNode()`

### Pattern Support

`<pattern>` is a paint-server definition. It is resolved lazily when a shape
uses `fill="url(#...)"` or `stroke="url(#...)"`.

Current backend status:

- Android, iOS, Harmony, and Skity use resolver-driven tile rendering for
  vector pattern fill and stroke.
- Image content in pattern fills and strokes reaches each backend's existing
  `DrawImage()` implementation, with the pattern tile transforms and clipping.
  The host must supply the image callback, resource manager, or image fetcher.
  Remote requests, caching, and domain policies follow that loader.
- A pattern does not establish an SVG viewport. Its child content keeps the
  referencing context's viewport and viewBox for percentage lengths. The pattern's
  `viewBox` or `patternContentUnits="objectBoundingBox"` changes the canvas
  transform independently; it must not replace that percentage reference with
  the tile dimensions or a normalized 1x1 box. A child's `50%` is therefore not
  generally equivalent to the numeric bbox fraction `0.5`.
  See SVG 2 [viewport establishment](https://www.w3.org/TR/SVG2/coords.html#EstablishingANewSVGViewport)
  and [object bounding box units](https://www.w3.org/TR/SVG2/coords.html#ObjectBoundingBoxUnits).
- iOS and Skity image callbacks return an available image during tile drawing.
  The iOS demo uses the existing `SrSVG` bitmap callback in a demo-local view
  for image previews. Its `SvgImageLoader` decodes embedded base64 images,
  loads remote images asynchronously, returns cached images during drawing,
  and calls `setNeedsDisplay` on the main thread after loading. Pending requests
  share weak view listeners, so repeated pattern tiles do not duplicate requests
  or retain removed previews. Other previews keep the `SrSVGView` animation path.
  Android's `SVGRender.ResourceManager` must invoke its bitmap callback during
  the current draw, such as from a cache hit, while the tile canvas state is
  active. Harmony's `SvgDrawable` supports asynchronous fetching through its
  image cache and invalidate callback.
- The Android demo's `SvgImageLoader` downloads images off the main thread,
  supplies cached bitmaps through the existing `SVGRender.setResourceManager()`
  interface, and re-records image previews with `renderPictureWithResult()` when
  loading completes. The demo displays those pictures with `SVGDrawable(Picture)`;
  other previews keep the content-based drawable's session and animation path.
- Harmony's example image fetcher decodes embedded base64 images as well as
  remote images. Its optional `patternRegression` launch parameter opens a
  regression page that snapshots the existing render node and compares the
  shared Chrome probes; ordinary launches open the example browser.
- Skity stroke patterns depend on `SrPathFactorySkity::CreateStrokePath()`,
  which converts the stroked outline to a fill path via `skity::Stroke`.
- The macOS Skity preview caches decoded base64 PNGs and downloaded raster
  images per document. HTTP(S) downloads trigger a redraw on completion.
  Hardware and software rendering use the same cached pixmap.
- Skity maps intrinsic image coordinates to the destination once and clips to
  the image viewport. Oversized pattern fills are intersected with the device
  clip before drawing to avoid software rasterizer fixed-point overflow; paint
  resolution continues to use the original geometry bounds.
- `pattern stroke` under `vector-effect="non-scaling-stroke"` uses
  device-space stroke clipping before rendering pattern tiles under the original
  CTM.

### Text Support Is Smaller Than The Parser Surface

Because text goes through paragraph bridges instead of shape stroking/filling:

- solid fill, currentColor, font size, x/y, and text-anchor are supported by the text bridges
- nested tspan inherits fill, color, font size, and fill-opacity without resetting unspecified properties
- whole-text opacity uses an opacity layer; fill-opacity is applied once to each run's color alpha
- generic shape-style attributes on text are not a good proxy for text support
- Harmony uses NativeDrawing's alphabetic baseline and measured line width for SVG positioning
- the paragraph retains its font collection until typography destruction, independently of the factory
- Skity text remains unavailable; textPath, text stroke, per-span opacity and transforms remain unsupported
- `svg/testing/harmony/` compares SVG pixels with independently built native typography on a device

### Filter Support Is Backend-Gated

The most important filter fact in the repository is this:

- filters only run when `canvas->SupportsFilters()` is true

Right now that means:

- Android: no runtime filters
- iOS: no runtime filters
- Harmony: no runtime filters
- Skity: partial runtime filters

Inside Skity, the implementation focuses on:

- pure blur
- blur + offset + color-matrix style shadow patterns

It is not a general-purpose SVG filter graph evaluator.

## Testing Reality

The SVG project is currently much stronger in sample assets than in engine
unit tests.

Observed coverage:

- `svg/test_cases/`
  small reference assets for basic shapes, paths, gradients, text, image,
  transforms, clip paths, and defs/use
- `svg/examples/*.svg`
  larger manual samples, especially for masking
- Android example-side tests:
  `svg/examples/android/app/src/test/java/.../ExampleUnitTest.java`

What is missing:

- C++ engine-level SVG unit tests comparable to the markdown subproject
- backend parity tests for the same SVG fixture set
- golden-image style regression tests

## Knowledge-System Workflow

The repository knowledge loop should be:

1. Run `svg/tools/svg_support_audit.py audit-matrix`
   to regenerate `svg/docs/audit-matrix.json`
2. Run `svg/tools/svg_support_audit.py matrix`
   to regenerate `svg/docs/support-matrix.json`
3. Review platform differences and update this file if behavior changed
4. Use `svg/tools/svg_support_audit.py analyze --profile audit`
   for engineering analysis of concrete SVG snippets or files
5. Update `svg/skills/serval-svg-expert/`
   and `svg/skills/serval-svg/` when the support surface changes

Useful commands from repo root:

```bash
python3 svg/tools/svg_support_audit.py audit-matrix --output svg/docs/audit-matrix.json
python3 svg/tools/svg_support_audit.py matrix --output svg/docs/support-matrix.json
python3 svg/tools/svg_support_audit.py analyze --svg-file svg/test_cases/basic_shapes.svg --platform all
python3 svg/tools/svg_support_audit.py analyze --profile audit --svg-file path/to/input.svg --platform harmony
```

## How To Extend SVG Features

When adding a new capability, treat the work as a multi-layer change:

1. parser registration
   add or update the tag construction path in `svg/src/parser/SrSVGDOM.cc`
2. node model
   add the node class, fields, and attribute parsing
3. render integration
   connect the node into `OnRender`, `AsPath`, render-state flow, or reference
   resolution
4. backend work
   implement the feature on the target canvases instead of stopping at parser
   support
5. samples / tests
   add or update SVG fixtures that exercise the behavior
6. knowledge update
   rerun the support matrix, update this deep wiki, and update both skills

### Example: `feGaussianBlur`

This repository already shows why step-by-step support accounting matters:

- parser registration exists
- primitive attribute parsing exists
- the generic filter tag exists
- only the Skity backend actually executes blur-like behavior
- filter graph semantics such as `in` / `result` chaining are still not
  general-purpose

So the question is not “does the tag exist?” but rather:

- which backends execute it?
- which attributes are truly consumed?
- does it work as a standalone blur, as a shadow heuristic, or as a full SVG
  primitive?

That is the exact style of reasoning `serval-svg-expert` should apply to every
future SVG feature.
