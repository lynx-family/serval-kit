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
  Skity backend, including the bounded multi-input filter graph executor.
- `svg/test_cases/`
  Small reference SVG samples.
- `svg/examples/`
  Larger example assets and app integrations.
- `svg/tools/svg_support_audit.py`
  Knowledge-system script for support matrix generation and SVG analysis.

## How Rendering Works

### 1. XML to DOM

`SrDOMParser` and `SrDOM` build a lightweight XML tree from SVG text.

Key files:

- `svg/src/parser/SrDOMParser.cc`
- `svg/src/parser/SrDOM.cc`
- `svg/src/parser/SrXMLParser.cc`

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
  Android and iOS have text bridges, but the support surface is much smaller
  than generic SVG presentation attributes suggest. Stroke-oriented text
  styling is not really rendered. Harmony and Skity text backends are still
  stubbed.
- `filter`, `feGaussianBlur`, `feOffset`, `feColorMatrix`
  The parser and node model recognize them, but actual filter execution is
  gated behind `SrCanvas::SupportsFilters()` and `SupportsFilterModel()`.
  Android, iOS and Harmony retain their linear SourceGraphic implementations.
  Skity Metal additionally supports the bounded graph subset documented below.

### Backend-Dependent Filter Primitives

These tags execute in bounded Skity Metal graphs, but remain parsed-only on
Android, iOS and Harmony:

- `feComposite`
- `feBlend`
- `feFlood`

Their attributes are resolved into the shared filter model. Skity consumes the
qualified subset; other backends reject these graphs and keep source fallback.

## Platform Snapshot

### Android

Strengths:

- shapes, gradients, masks, clip paths, text

Caveats:

- filters are limited to supported linear SourceGraphic chains
- images depend on an app-provided resource manager

Key files:

- `svg/platform/android/src/main/cpp/SrAndroidCanvas.cc`
- `svg/platform/android/src/main/java/SVGRender.java`
- `svg/platform/android/src/main/cpp/SrAndroidParagraphFactory.cc`

### iOS

Strengths:

- shapes, gradients, masks, clip paths, text

Caveats:

- filters are limited to supported linear SourceGraphic chains
- images depend on an image callback

Key files:

- `svg/platform/iOS/SrIOSCanvas.mm`
- `svg/platform/iOS/SrIOSParagraph.mm`

### Harmony

Strengths:

- shapes, gradients, masks, clip paths

Current gaps:

- image backend methods are empty
- text paragraph implementation is incomplete
- filters are limited to supported linear SourceGraphic chains; non-zero
  offsets require native offset image-filter availability

Key files:

- `svg/platform/harmony/servalsvg/sr_harmony_canvas.cc`
- `svg/include/platform/harmony/sr_harmony_paragraph.h`

### Skity

Strengths:

- shapes, gradients, masks, clip paths, images
- bounded multi-input filter graphs on Metal, alongside the linear fast path

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
- bounded filter graphs are qualified on Metal in sRGB; software parity is not available

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
- Skity stroke patterns depend on `SrPathFactorySkity::CreateStrokePath()`,
  which converts the stroked outline to a fill path via `skity::Stroke`.
- `pattern stroke` under `vector-effect="non-scaling-stroke"` uses
  device-space stroke clipping before rendering pattern tiles under the original
  CTM.

### Text Support Is Smaller Than The Parser Surface

Because text goes through paragraph bridges instead of shape stroking/filling:

- fill color and font size are the safest text attributes
- generic shape-style attributes on text are not a good proxy for text support
- Harmony and Skity text are not production-ready

### Filter Support Is Backend-Gated

The most important filter fact in the repository is this:

- filters only run when `canvas->SupportsFilters()` is true

Right now that means:

- Android: supported linear SourceGraphic chains
- iOS: supported linear SourceGraphic chains
- Harmony: supported linear SourceGraphic chains, with native offset availability
- Skity: supported linear chains and bounded multi-input graphs on Metal

Skity preserves the linear SourceGraphic fast path and captures a display
list for multi-input graphs. Immutable indices resolve SourceGraphic,
SourceAlpha and earlier named results before playback. Flood, Gaussian blur,
offset, matrix/luminanceToAlpha, normal blend and Porter-Duff over/in/out/atop/xor
are supported on Metal. Arithmetic is restricted to alpha-only
`max(in2 - in, 0)` (k1=0, k2=-1, k3=1, k4=0); it is not approximated as out.

Admission limits bound primitive count, source replays, traversal visits, layer
depth and estimated RGBA8 workspace, including Gaussian outsets. Absolute
matrix operations are translated through layer origins during captured replay.
Texture dimensions use the affine transform's basis lengths and the destination
surface's content scale, matching Skity's physical layer sizing. Pass
`GPUSurface::ContentScale()` as the third `SrSkityCanvas` constructor argument,
also when recording for later playback on that surface. The default scale of
one applies to 1x surfaces; perspective transforms use the source fallback.
Costs are also combined across active nested filters, including linear layers
and flood-only captures, so nested source replay cannot grow exponentially.
Primitive subregions mask completed outputs instead of clipping dependency
sampling. Blur inputs retain the full filter input domain, and flood paints its
explicit local region. Finite negative blur deviations disable that primitive
without dropping the remaining chain; non-finite deviations remain rejected.
The implementation is qualified in sRGB on Metal; general colored arithmetic,
linearRGB interpolation, other blend modes, software and other GPU backend
parity are not claimed. Unsupported graphs keep the existing unfiltered-source
fallback and report through the SVG model diagnostics.

`svg/tests` provides standalone Metal regression tests against a separately
built, matching Skity source/library pair. `svg/test_cases/filter-graph.svg`
exercises source alpha, named inputs, flood, blur and normal blending.

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
- `svg/tests/`:
  Metal pixel checks, graph admission checks and nested recording-cost
  regressions; see its README for the standalone CMake invocation

What is missing:

- broad C++ engine-level SVG coverage beyond the Skity filter regressions
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
- current backends execute supported linear blur chains; Skity Metal also
  executes bounded multi-input graphs
- filter graph semantics such as `in` / `result` chaining are still not
  general-purpose

So the question is not “does the tag exist?” but rather:

- which backends execute it?
- which attributes are truly consumed?
- does it work as a standalone blur, as a shadow heuristic, or as a full SVG
  primitive?

That is the exact style of reasoning `serval-svg-expert` should apply to every
future SVG feature.
