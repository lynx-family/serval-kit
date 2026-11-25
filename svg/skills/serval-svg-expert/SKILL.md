---
name: serval-svg-expert
description: "Use this skill when auditing the serval-svg repository, regenerating support artifacts, interpreting internal SVG support facts, analyzing whether an SVG can render correctly with code-grounded reasoning, or planning and implementing new SVG features."
---

# Serval SVG Expert

## Overview

This is the internal audit and maintenance skill for `serval-svg`.

Its job is to inspect the codebase, maintain the knowledge artifacts, explain
why support behaves the way it does, and publish the public support contract
consumed by `serval-svg`.

## Canonical Inputs

Load these files first:

- `../../docs/audit-matrix.json`
- `../../docs/deepwiki.md`
- `../../docs/support-matrix.json`
- `../../tools/svg_support_audit.py`
- `../../README.md`
- `../../AGENTS.md`
- [references/workflows.md](references/workflows.md)
- [references/feature-extension.md](references/feature-extension.md)

## Core Responsibilities

### 1. Repository Audit

Use this skill when the user wants:

- a code audit of `serval-svg`
- a support inventory grounded in implementation
- an explanation of parser/render/backend gaps
- refreshed SVG knowledge artifacts

The audit source of truth is:

- `audit-matrix.json` for structured facts
- `deepwiki.md` for engineering context
- source files for verification

### 2. Public Contract Maintenance

This skill owns the publication flow from internal facts to external support
answers.

When support changes:

1. regenerate `svg/docs/audit-matrix.json`
2. regenerate `svg/docs/support-matrix.json`
3. refresh `svg/docs/deepwiki.md`
4. update both skills if the support surface changed

### 3. SVG Compatibility Analysis

Use this skill when the user provides an SVG snippet or file and asks whether
`serval-svg` can render it correctly, especially if the answer needs code
reasoning.

Preferred command:

```bash
python3 svg/tools/svg_support_audit.py analyze --profile audit --svg-file path/to/input.svg --platform all
```

Use the public profile only when validating the published support contract:

```bash
python3 svg/tools/svg_support_audit.py analyze --svg-file path/to/input.svg --platform all
```

### 4. Feature Development Guidance

Use this skill when the user wants to add or complete SVG support, especially
for filters, masks, text, image, or backend parity work.

Always evaluate all layers:

1. parser registration
2. node fields and attribute parsing
3. render-path integration
4. backend implementation
5. sample or regression coverage
6. audit-matrix update
7. support-matrix update
8. deep wiki update
9. skill update

## Response Rules

- Distinguish `recognized`, `parsed`, `rendered`, and `platform-ready`.
- Use `audit-matrix.json` as the structured internal source of truth.
- Use `support-matrix.json` to verify what the public skill is allowed to say.
- Use `deepwiki.md` for architecture and historical caveats.
- Be explicit when a tag is parsed but not rendered.
- Be explicit when something only works on one backend.
- If you infer behavior from code rather than from a sample, say so.
- For pattern images, check the host image loader as well as tile transforms,
  clipping, and referenced pattern content in each target backend.
- Keep pattern content transforms separate from the referencing SVG viewport
  used by percentages. Validate numeric bbox fractions and percentages against
  Chrome; a pattern tile or pattern viewBox does not establish a new viewport.
- Harmony text uses NativeDrawing typography. Verify alphabetic baseline,
  measured-width anchoring, nested style inheritance, opacity, and font
  collection lifetime with `svg/tools/run_harmony_text_tests.py`; the tests
  compare real SVG pixels against independent native typography on a device.
- For markers, verify original path vertices and endpoint tangents rather than
  flattened curves. Distinguish whole-path start/end from mid vertices across
  subpaths. Check definition style ancestry, explicit context paint, clipping,
  reference-point/viewBox transforms, and non-scaling stroke.
- Run `svg/tools/run_marker_tests.sh` and the Chrome/Skity marker manifest.
  Android's `MarkerInstrumentedTest` checks the same 37 probes in software and
  hardware Canvas on a physical device; both modes passed on MIX 2S/API 29.
  The expanded eight fixtures and 99 probes, including three MDN examples,
  passed on Skity software and physical iPhone 11 /
  iOS 18.3.1 (SrSVG/Core Graphics) and ALN-AL80 / OpenHarmony 6.1.0.115
  (SvgRenderNodeController/ArkUI snapshots). Use the iOS standalone device app
  and Harmony `markerRegression` launch flag to export PNGs and JSON reports.
  MDN's context paint stylesheet example is tested with equivalent inline
  marker styles; the original stylesheet form remains unsupported and is
  retained separately. Do not infer stylesheet support from this regression.
  Context paint servers currently render as none because referencing-element
  coordinates and bounds are not bridged; do not claim full SVG 2 compliance
  or device parity from the shared tests.

## Boundary

Do not use this as the first tool for a simple consumer-facing support question
if the public contract already answers it cleanly. In that case, prefer
`serval-svg`.
