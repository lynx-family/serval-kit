# SVG Agent Guide

Use this file when working on the `svg/` subproject, especially on the
`serval-svg-expert` / `serval-svg` knowledge system.

## Mission

Maintain a two-layer SVG knowledge workflow:

- internal audit facts for engineering work
- external support answers for consumers of `serval-svg`

Never mix those roles.

## Boundary Rules

- `serval-svg` is the public support skill.
- `serval-svg-expert` is the internal audit and maintenance skill.
- `svg/docs/support-matrix.json` is the public support contract.
- `svg/docs/audit-matrix.json` is the internal audit artifact.
- `svg/docs/deepwiki.md` is internal engineering context.

If a task needs code evidence, parser/render/backend reasoning, or feature
planning, it belongs to `serval-svg-expert`.

If a task only needs a direct answer about supported tags, attributes, or
platform coverage, it belongs to `serval-svg`.

## Truth Rules

- Do not claim public support only because a tag is recognized by the parser.
- Do not claim public support only because an attribute is parsed.
- Only publish into `support-matrix.json` what the expert workflow is willing
  to stand behind as product-facing support.
- Treat unlisted tags and unlisted attributes in `support-matrix.json` as
  unsupported by default.
- Keep repository skill definitions vendor-neutral.
- Treat `SKILL.md`, `references/`, and optional `scripts/` as canonical.

## Required Inspection Order

When auditing support, inspect in this order:

1. `svg/src/parser/SrSVGDOM.cc`
   Determines which SVG tags are recognized and instantiated.
2. `svg/src/element/SrSVGNode.cc`
   Determines common attributes, inheritance, masking, clipping, filters, and
   shared render behavior.
3. `svg/src/element/*.cc`
   Determines tag-specific attributes and node behavior.
4. `svg/include/canvas/SrCanvas.h`
   Defines abstract canvas capabilities and default fallbacks.
5. `svg/include/platform/*` and `svg/platform/*`
   Determines actual platform implementation status.
6. `svg/test_cases/` and `svg/examples/`
   Provides examples, sample coverage, and regression material.

## Artifact Rules

### `audit-matrix.json`

Use the layered internal states here:

- `recognized`
- `parsed`
- `rendered`
- `platform-ready`

This file may include evidence, examples, and engineering notes.

### `support-matrix.json`

Use only public-facing states here:

- `supported`
- `partial`
- `unsupported`

Per tag, keep only:

- public support state
- platform support state
- supported attributes
- partially supported attributes
- unsupported attributes
- short public caveats

Do not leak source-file evidence or internal implementation structure into the
public contract.

## deepwiki Requirements

`svg/docs/deepwiki.md` should stay internal-facing and explain:

- repository structure
- parser pipeline
- node model and inheritance rules
- render pipeline
- platform backend split
- how to interpret `audit-matrix.json`
- how the public `support-matrix.json` is derived
- examples, test assets, and known gaps
- feature-extension workflow

## SVG Compatibility Analysis

When given an SVG snippet:

1. choose the right layer first
2. extract tags and important attributes
3. compare them with the selected matrix
4. identify blockers first
5. identify partial support and platform caveats
6. conclude using the language of that layer

Expected outputs:

- public mode:
  answer whether the SVG fits the published support surface
- expert mode:
  explain why support is missing or partial and where the gaps live

## Feature Development Checklist

When adding or completing a feature such as `feGaussianBlur`, check all of the
following before considering the task done:

1. parser node creation
2. tag enum or node class coverage
3. attribute parsing
4. render state plumbing
5. platform canvas implementation
6. examples or regression assets
7. audit-matrix update
8. support-matrix update
9. deep wiki update
10. skill update

## Repository Layout For This Plan

Keep the implementation centered around these paths:

- `svg/docs/support-matrix.json`
- `svg/docs/audit-matrix.json`
- `svg/docs/deepwiki.md`
- `svg/skills/serval-svg-expert/`
- `svg/skills/serval-svg/`

If scripts are added later, prefer putting them under `svg/tools/`.
