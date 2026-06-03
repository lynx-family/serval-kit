# Android Pattern Rendering Design

## Background

SVG `pattern` is a paint server feature with semantics beyond simple texture repeat.
It needs to support:

- `href` inheritance
- `patternUnits`
- `patternContentUnits`
- `patternTransform`
- `viewBox`
- `preserveAspectRatio`
- both `fill` and `stroke` usage

The earlier Android implementation used a bitmap-based rendering path:

1. Render a single pattern tile into an offscreen `Bitmap`
2. Wrap that bitmap into a `BitmapShader`
3. Repeat the shader over the target fill or stroke area

This path was straightforward, but over time it showed several limitations in
quality, semantics, and cross-platform extensibility.

## Goals

- Make Android `pattern` rendering match SVG semantics more closely
- Unify `fill` and `stroke` around the same pattern resolution model
- Extract reusable logic for future iOS / Harmony / other backend reuse
- Remove the bitmap pattern path as the main rendering pipeline
- Preserve a structure that is understandable and incrementally optimizable

## Non-Goals

- Fully solve all performance concerns in the first vector implementation
- Introduce a brand new cross-platform low-level clipping API at this stage
- Rewrite all platform paint server logic together with `pattern`

## Option A: Bitmap Pattern

### Overview

The bitmap path renders one pattern tile offscreen and then repeats it as a
shader over the destination shape.

### Strengths

- Simple implementation model: render tile once, then repeat it
- Good fit for Android's existing `BitmapShader`
- Short render path at runtime for many repeated tiles
- Lower JNI churn during final painting because repetition is delegated to the
  shader system
- Easier to bootstrap a first usable implementation

### Weaknesses

- Quality depends on the bitmap tile resolution
- Scaling and high-DPI scenarios can look blurry
- `stroke` is especially sensitive, especially dashed pattern stroke
- Harder to preserve SVG semantics consistently for:
  - `patternContentUnits`
  - `viewBox`
  - `preserveAspectRatio`
  - `patternTransform`
  - `objectBoundingBox` coordinate handling
- Cross-platform abstraction is weaker because each backend has to adapt around
  a bitmap intermediate representation

### Best Fit

- Fast initial rollout
- Simple pattern content
- Cases where visual sharpness is less critical than implementation speed

## Option B: Vector Pattern

### Overview

The vector path resolves the pattern into a normalized `ResolvedPattern`, then
tiles the destination area and renders pattern content directly as vector
graphics inside each tile.

### Strengths

- No bitmap intermediate, so scaling quality is significantly better
- More faithful SVG semantics
- `fill` and `stroke` can share the same pattern resolution model
- Dashed pattern stroke can be handled through stroke outline generation and
  tile clipping instead of a bitmap fallback
- The architecture separates:
  - semantic resolution
  - geometry helpers
  - platform drawing
- Better long-term fit for multi-backend reuse

### Weaknesses

- Higher implementation complexity
- Performance hotspots are more explicit
- Android currently pays more per-tile setup cost through:
  - `Save/Restore`
  - clip construction
  - transform setup
  - JNI round trips
  - repeated pattern content rendering
- More edge cases must be handled carefully:
  - recursion and `href` chains
  - pattern vs gradient IRI routing
  - fill/stroke ordering
  - `viewBox` and bounding-box transforms

### Best Fit

- Long-term mainline implementation
- Cases that need better sharpness and more accurate SVG behavior
- Backends that need a reusable semantic pattern model

## Chosen Direction

Android now uses the vector path as the main pattern rendering strategy for both
`fill` and `stroke`.

This choice was made because:

- visual quality is better
- pattern semantics are easier to preserve
- stroke handling, including dashed stroke pattern, behaves more naturally
- the shared resolver model is more suitable for multi-backend evolution

The bitmap path was useful as an early bootstrap implementation, but it is no
longer the desired long-term architecture.

## Current Architecture

### 1. Pattern Resolver

`ResolvePatternFromIri(...)` converts `url(#pattern-id)` into a normalized
`ResolvedPattern`.

It is responsible for:

- locating the referenced node from `id_mapper`
- validating that the target node is actually a `pattern`
- resolving `href` inheritance chains
- cascading child-over-parent attributes
- converting `x/y/width/height` into usable float values
- applying `patternUnits` semantics
- resolving `viewBox` / `preserveAspectRatio`
- producing the final content transform used by the platform renderer

There is also a lightweight `IsPatternIri(...)` helper for cheap IRI routing
without triggering full pattern resolution or path bounds computation.

### 2. Pattern Geometry Helpers

Pattern-related affine helpers are kept in `SrSVGPatternUtils` under `utils/`.

They handle:

- identity transform checks
- affine inverse
- point mapping
- mapped bounds calculation

These helpers are renderer-agnostic and intentionally separate from DOM
semantics.

### 3. Android Canvas Execution

Android rendering is split into:

- `RenderPatternFill(...)`
- `RenderPatternStroke(...)`
- `RenderPatternTiles(...)`

`RenderPatternFill(...)`:

- verifies the IRI is a pattern
- computes path bounds
- resolves the pattern
- clips to the target fill path
- delegates tile rendering

`RenderPatternStroke(...)`:

- verifies the IRI is a pattern
- computes object bounds
- resolves the pattern
- builds a stroke outline path, including dashed stroke support
- clips to the outline
- delegates tile rendering

`RenderPatternTiles(...)`:

- determines the visible tile area
- applies `patternTransform`
- iterates tiles
- sets up clip and transform per tile
- renders pattern content directly

## Why Vector Won

In practice, the vector path produced the clearest improvement in the following
areas:

- pattern fill correctness
- pattern stroke correctness
- dashed stroke pattern quality
- consistency across `patternUnits`, `patternContentUnits`, and `viewBox`

During validation:

- standard pattern fill worked correctly
- pattern stroke worked correctly
- dashed pattern stroke no longer relied on bitmap blur
- gradient stroke and pattern stroke routing were both preserved correctly

## Trade-Offs

### What We Gain

- better visual sharpness
- better SVG semantic fidelity
- cleaner separation between shared resolution logic and platform execution
- a stronger base for future iOS / Harmony reuse

### What We Pay

- more implementation complexity
- more explicit per-tile runtime cost
- more JNI and canvas operations in large repeat scenarios

This trade-off is acceptable because the project now values correctness and
long-term maintainability over the simplicity of a bitmap-based shortcut.

## Performance Considerations

The biggest remaining hotspot is the tile loop in `RenderPatternTiles(...)`.

Potential cost sources include:

- the double tile loop itself
- per-tile `Save/Restore`
- per-tile clip path creation
- per-tile transform setup
- per-tile pattern content rendering
- JNI round trips triggered by those actions

This is a known follow-up optimization area, but it does not change the
architectural direction.

## Follow-Up Work

- evaluate and optimize JNI churn inside `RenderPatternTiles(...)`
- reduce per-tile object creation, especially clip path construction
- precompute more tile-invariant transform state outside the inner loops
- consider lightweight fast paths for simple pattern cases
- reuse `IsPatternIri(...)` and `ResolvePatternFromIri(...)` in other backends

## Current Limitations

- Pattern stroke still depends on converting the stroke into a clip-able outline
  path, so backend stroke-outline quality remains important for edge cases.
- `pattern stroke` under `vector-effect="non-scaling-stroke"` now uses
  device-space stroke clipping on Android, iOS, Harmony, and Skity.
- Very small Skity stroke widths inside
  `patternContentUnits="objectBoundingBox"` remain tracked as a Skity backend
  stroke-geometry issue instead of a Serval pattern-rendering bypass.

## Conclusion

Bitmap pattern rendering was a good transitional implementation because it was
easy to build and stable to run. However, it was not strong enough as a
long-term solution for SVG pattern fidelity, especially for stroke and dashed
stroke scenarios.

The vector pattern path is more complex, but it aligns much better with the SVG
model, produces better output, and establishes a cleaner shared foundation for
future multi-backend support.
