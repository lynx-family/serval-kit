---
name: serval-svg
description: "Use this skill when the user wants a direct, product-facing answer about what serval-svg supports: supported SVG tags, supported attributes, platform coverage, or whether a concrete SVG fits the published support surface, without exposing repository internals."
---

# Serval SVG

## Overview

This is the public support-answer skill for `serval-svg`.

Its job is to answer support questions directly, accurately, and without
depending on internal implementation detail.

## Canonical Inputs

Load these first:

- `../../docs/support-matrix.json`
- [references/response-guidelines.md](references/response-guidelines.md)

For a concrete SVG file, use the public analysis mode:

```bash
python3 svg/tools/svg_support_audit.py analyze --svg-file path/to/input.svg --platform all
```

## Scope

This skill should answer:

- whether a tag is `supported`, `partial`, or `unsupported`
- which attributes are supported for that tag
- which attributes are only partially supported
- which attributes are unsupported
- whether support changes by backend
- whether a concrete SVG stays inside the published support surface

## Response Rules

- Start from `support-matrix.json`.
- Do not load or cite `deepwiki.md` for normal support answers.
- Do not explain parser internals, render internals, or backend source files.
- Treat unlisted tags and unlisted attributes as unsupported unless the public
  contract says otherwise.
- If support is partial, say so plainly.
- If platform support differs, say so plainly.
- For patterns containing images, explain the host app's image loading
  requirement and its control over remote requests and domain policies.
- Percentages inside pattern content use the referencing SVG viewport; do not
  describe them as interchangeable with numeric object-bounding-box fractions.
- Keep the answer direct and product-facing.
- Basic `text` / `tspan` is available with limited attributes on Android, iOS,
  and Harmony; Skity remains unavailable. Explain system-font differences and
  do not imply textPath, text stroke, or per-span positioning support.

## Escalation

Escalate to `serval-svg-expert` when the user wants:

- to know why support behaves a certain way
- source-file evidence
- a repository audit
- refreshed support artifacts
- feature-development guidance
- implementation planning or code changes
