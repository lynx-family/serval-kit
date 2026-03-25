# serval-svg Knowledge System

This directory defines a two-layer SVG knowledge workflow for `serval-svg`.

The key rule is simple:

- `serval-svg` is public-facing. It answers "what is supported?" directly and
  should not depend on internal implementation detail.
- `serval-svg-expert` is internal-facing. It audits code, maintains the
  support facts, explains why support behaves the way it does, and drives
  feature work.

## Skill Boundary

### `serval-svg`

Use `serval-svg` when the goal is to answer:

- whether a tag is supported
- which attributes are supported for that tag
- whether support is full, partial, or unsupported
- whether a backend such as Android, iOS, Harmony, or Skity is covered
- whether a concrete SVG fits the published support surface

`serval-svg` should read only the public support contract:

- `svg/docs/support-matrix.json`
- its own `references/`

It should not rely on `deepwiki.md`, source-file evidence, parser internals, or
backend implementation details unless the task is explicitly escalated to
`serval-svg-expert`.

### `serval-svg-expert`

Use `serval-svg-expert` when the goal is to:

- audit the repository from code facts
- regenerate support artifacts
- explain why a support claim is true or false
- analyze SVG compatibility with implementation detail
- maintain `deepwiki.md`
- plan or implement new SVG capabilities

`serval-svg-expert` owns the internal evidence layer:

- `svg/docs/audit-matrix.json`
- `svg/docs/deepwiki.md`
- `svg/tools/svg_support_audit.py`
- primary source files under `svg/src/`, `svg/include/`, `svg/platform/`

It is also responsible for publishing the external support contract consumed by
`serval-svg`.

## Versioned Artifacts

Keep these repository artifacts in sync:

- `svg/docs/support-matrix.json`
  Public support contract. This is the only structured source `serval-svg`
  should need for support answers.
- `svg/docs/audit-matrix.json`
  Internal audit facts. This includes parser status, render status, backend
  readiness, attribute caveats, evidence, and examples.
- `svg/docs/deepwiki.md`
  Internal human-readable engineering wiki generated from audit facts and code.
- `svg/skills/serval-svg/`
  Public-facing skill definition.
- `svg/skills/serval-svg-expert/`
  Internal expert skill definition.

The repository copy under `svg/skills/` is the source of truth. If another
agent framework needs packaging or vendor-specific metadata, keep that outside
this core definition layer.

## Source Of Truth

All support claims must come from code and the expert audit workflow, not from
README assumptions.

Primary evidence sources:

- parser tag registration:
  `svg/src/parser/SrSVGDOM.cc`
- common attribute parsing and render pipeline:
  `svg/src/element/SrSVGNode.cc`
- element-specific attribute parsing:
  `svg/src/element/*.cc`
- platform rendering capability:
  `svg/include/canvas/SrCanvas.h`
  `svg/include/platform/*`
  `svg/platform/*`
- examples and regression samples:
  `svg/test_cases/`
  `svg/examples/`

## Data Model

The knowledge system intentionally uses two different data shapes.

### Public Contract: `support-matrix.json`

This file is optimized for direct answers. Each tag should expose only:

- overall support: `supported`, `partial`, or `unsupported`
- backend availability by platform
- `supported_attributes`
- `partially_supported_attributes`
- `unsupported_attributes`
- short public caveats when needed

Anything not listed in the public contract should be treated as unsupported by
default.

### Internal Audit: `audit-matrix.json`

This file is optimized for maintenance. It can keep:

- parser recognition
- attribute parsing state
- render-path state
- backend readiness
- source files and evidence
- example hits
- engineering notes

This layer is for `serval-svg-expert`, not for the public support skill.

## Implementation Order

1. Audit the code and generate `svg/docs/audit-matrix.json`.
2. Derive `svg/docs/support-matrix.json` from that audit layer.
3. Refresh `svg/docs/deepwiki.md` from current audit facts.
4. Keep `serval-svg-expert` aligned with the audit layer.
5. Keep `serval-svg` aligned with the public contract only.

## SVG Analysis Workflow

Two analysis modes are expected:

- public analysis:
  compare an SVG against `svg/docs/support-matrix.json`
- expert analysis:
  compare an SVG against `svg/docs/audit-matrix.json` and explain why

Useful commands from repo root:

```bash
python3 svg/tools/svg_support_audit.py matrix --output svg/docs/support-matrix.json
python3 svg/tools/svg_support_audit.py audit-matrix --output svg/docs/audit-matrix.json
python3 svg/tools/svg_support_audit.py analyze --svg-file path/to/input.svg --platform all
python3 svg/tools/svg_support_audit.py analyze --profile audit --svg-file path/to/input.svg --platform all
```

## Feature Development Workflow

When adding a feature such as `feGaussianBlur`, the work is not complete when a
tag is merely parser-visible.

The full workflow is:

1. parser entry and tag construction
2. node model and attribute parsing
3. render-path integration
4. platform backend implementation
5. examples or regression coverage
6. audit-matrix update
7. public support-matrix update
8. deepwiki update
9. skill update

The support answer layer and the engineering audit layer must move together.
