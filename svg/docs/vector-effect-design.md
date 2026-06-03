# VectorEffect Design

## Background

The original SVG pipeline did not support
`vector-effect="non-scaling-stroke"`. By default, path geometry and stroke width
are both affected by the current transform matrix (CTM), which means stroke
thickness changes under scale.

Legacy Android SVG rendering already supported `non-scaling-stroke` through a
"pre-transform path + reset canvas matrix" strategy. This document records the
candidate approaches, their trade-offs, and the chosen first implementation for
the current cross-platform pipeline.

## Goal

Support:

- `vector-effect="none"`
- `vector-effect="non-scaling-stroke"`

Non-goals for phase 1:

- Full SVG 2 `vector-effect` value set
- Perfect cross-platform parity for every stroke paint server in the first
  milestone

## Candidate Approaches

### A. Paint-Level Width Compensation

Compensate `stroke-width` with the inverse scale extracted from the current CTM.
Optionally compensate dash array and dash offset as well.

Pros:

- Minimal code change
- Fastest path to an initial release
- Works well for uniform scaling

Cons:

- Incorrect for non-uniform scale and shear
- Join/cap visuals can diverge from spec behavior
- Hard to make fully consistent across platforms

### B. Uniform-Scale Fast Path

Only apply the width compensation strategy when `sx ~= sy`. For other CTMs,
fallback to normal stroke behavior.

Pros:

- Low implementation risk
- Easy to reason about
- Avoids obviously wrong behavior under complex transforms

Cons:

- Incomplete feature support
- Produces capability gaps for real-world SVGs

### C. Geometry-First Implementation

Implement `non-scaling-stroke` in the rendering engine rather than by tweaking
paint parameters.

There are two variants:

- **C1. Pre-transform path + identity-matrix stroke**
- **C2. Stroke-to-path + fill**

#### C1. Pre-transform path + identity-matrix stroke

This is the strategy used by the legacy Android SVG renderer:

1. Read the current CTM.
2. Transform the original path into device space.
3. Temporarily reset the canvas matrix to identity.
4. Draw the already-transformed path as a stroke.
5. Restore the original matrix and paint state.

The core property is:

- Geometry still follows transforms and viewBox mapping.
- Stroke thickness stays in device space and no longer scales with CTM.

Pros:

- Much closer to SVG semantics than width compensation
- Reuses the platform's native stroke rasterization
- Does not require a full stroker implementation

Cons:

- Still depends on platform stroke behavior for caps, joins, dashes, and
  shaders
- Gradient and pattern stroke handling needs extra work
- Requires access to current transform state

#### C2. Stroke-to-path + fill

Convert the stroke into a real outline path and render it as fill.

Pros:

- Most controllable and most spec-like approach
- Better long-term cross-platform consistency

Cons:

- Highest implementation complexity
- More expensive at runtime
- Blocked by backend capability gaps such as Skity stroke-to-path support

### D. Platform-Native Feature Proxy

If a platform exposes native `non-scaling-stroke` support, proxy the SVG
attribute directly to that API.

Pros:

- Low effort when the backend already supports the feature
- May benefit from platform optimizations

Cons:

- Behavior can diverge across platforms
- Some backends simply do not provide such an API
- Harder to guarantee consistent rendering

## Why C Was Chosen

Approach C is the best fit for the current repository because:

- It keeps semantic control in the SVG engine instead of relying on ad hoc
  width compensation.
- It matches the proven behavior of the legacy Android SVG renderer.
- It can be introduced incrementally as a C1 phase before considering a heavier
  C2 implementation.

For phase 1 we choose **C1**.

## Phase 1 Scope

### Supported

- Parse `vector-effect`
- Support `none`
- Support `non-scaling-stroke`
- Apply C1 to standard stroked paths

### Limited / Deferred

- `pattern stroke` under `non-scaling-stroke`
- Skity backend
- Full parity for every gradient case on every backend

## Platform Plan

### Android

Android can follow the legacy strategy directly:

1. Build the stroke paint as usual.
2. Read the current `Canvas` matrix.
3. Transform the path into device space.
4. Reset canvas matrix to identity.
5. If shader-based stroke is used, multiply shader local matrix by the original
   CTM.
6. Draw the transformed path.
7. Restore the original matrix and shader state.

This provides the most complete phase-1 implementation.

#### Android Validation Status

The Android implementation has been validated with focused demo SVGs.

Verified as working:

- Solid stroke under uniform scale
- Solid stroke under non-uniform scale
- Dashed stroke under scale

Observed status:

- Linear gradient stroke is considered working on Android based on the focused
  `vector-effect-gradient-line.svg` validation case, where non-scaling stroke
  width is clearly preserved and the gradient distribution remains visually
  stable.
- `vector-effect-gradient-stroke.svg` is retained as an auxiliary rounded-rect
  observation case. It does not currently show obvious rendering errors, but it
  is not treated as the strongest conformance proof because the visual
  difference is less pronounced than in the line-based case.

Pattern stroke follow-up:

- `pattern stroke + non-scaling-stroke` now participates in the C1 flow.
- Pattern stroke still uses the dedicated `RenderPatternStroke()` branch, but
  that branch now generates its stroke clip path in device space when
  `vector-effect="non-scaling-stroke"` is active.
- Pattern tiles are rendered under the original CTM after the device-space clip
  is installed, so the stroke geometry is non-scaling without changing normal
  pattern coordinate semantics.

### iOS

iOS can implement C1 with `CGContextGetCTM()` and a transformed `CGPath`.

#### iOS Validation Status

Current implementation status:

- Solid color stroke follows the C1 path in `StrokePath()`
- The implementation reads the current `CGContext` CTM
- The stroked geometry is transformed into device space before drawing
- Linear gradient stroke under `non-scaling-stroke` now follows the same
  transformed-path flow as solid stroke and is validated by focused demo cases

Supported in phase 1:

- Solid stroke under `non-scaling-stroke`
- Gradient stroke under `non-scaling-stroke`
- Pattern stroke under `non-scaling-stroke`

### Harmony

Harmony can implement C1 with:

- internally tracked CTM state in `SrHarmonyCanvas`
- transformed path copies via `CreateTransformCopy()`
- temporary identity-matrix drawing

#### Harmony Validation Status

Current implementation status:

- Solid color stroke follows a C1-style path in `StrokePath()`
- Harmony maintains its own CTM stack because the drawing API does not expose a
  public "get current matrix" capability in the same way Android and iOS do
- The stroked geometry is transformed using the tracked CTM and then drawn after
  removing the current transform
- Gradient stroke under `non-scaling-stroke` is now wired into the tracked-CTM
  C1 path and matches the focused iOS/Android validation cases

Supported in phase 1:

- Solid stroke under `non-scaling-stroke`
- Gradient stroke under `non-scaling-stroke`
- Pattern stroke under `non-scaling-stroke`

### Skity

Skity uses the same tracked-CTM C1 approach as Harmony. Solid and gradient
strokes are drawn through the non-scaling stroke path, and pattern strokes use a
device-space stroke clip before rendering pattern tiles under the original CTM.

## Data Model Changes

Add a render-state enum:

- `SR_SVG_VECTOR_EFFECT_NONE`
- `SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE`

Parse it from `vector-effect` and carry it through `SrSVGRenderState`.

## Rendering Rules

When `vector-effect == non-scaling-stroke`:

- Fill rendering is unchanged.
- Stroke width should remain in device space.
- Geometry still follows all normal transforms.
- The implementation should prefer C1 over width compensation.

When `vector-effect == none`:

- Keep current rendering behavior unchanged.

## Testing Plan

Recommended regression cases:

- Uniform scale + solid stroke
- Non-uniform scale + solid stroke
- Rotation + solid stroke
- Scale + dashed stroke
- Scale + linear gradient stroke
- Scale + radial gradient stroke
- Scale + pattern stroke under `non-scaling-stroke`

iOS / Harmony follow-up validation priorities:

- Uniform scale + solid stroke
- Non-uniform scale + solid stroke
- Pattern stroke under `non-scaling-stroke`

Android-focused demo validation used during implementation:

- `vector-effect-non-scaling-stroke.svg`
- `vector-effect-dash.svg`
- `vector-effect-gradient-stroke.svg`
- `vector-effect-gradient-line.svg`

Current Android interpretation of those demos:

- `vector-effect-non-scaling-stroke.svg`: pass
- `vector-effect-dash.svg`: pass
- `vector-effect-gradient-stroke.svg`: auxiliary observation case; no obvious
  error, but not the primary proof case
- `vector-effect-gradient-line.svg`: pass

## Phase-1 Summary

The chosen implementation is:

- **Approach C**
- specifically **C1: pre-transform path + identity-matrix stroke**

This delivers a practical first version that is materially closer to spec than
stroke-width compensation, while keeping room for a later C2 evolution if
backend parity becomes a priority.

At the current milestone:

- Android solid stroke support is validated
- Android dashed stroke support is validated
- Android linear gradient stroke support is validated by focused line-based demo
  coverage, while more complex gradient edge cases can still receive follow-up
  verification
- Android pattern stroke under `non-scaling-stroke` is implemented through the
  native pattern stroke branch using device-space stroke clipping
- iOS solid stroke and linear gradient stroke are validated on the phase-1 C1
  path
- Harmony solid stroke and linear gradient stroke are validated on the phase-1
  C1 path
- Pattern stroke under `non-scaling-stroke` is implemented on Android, iOS,
  Harmony, and Skity
