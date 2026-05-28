# Illegal Parsing Regression Checklist

## Overview

This document tracks the current illegal SVG regression set used to verify
stability across Android, iOS, and Harmony examples.

Goal:

- Prevent crashes
- Prevent infinite recursion
- Prevent non-advancing parser loops
- Prevent long UI stalls or hangs
- Allow safe failure such as blank rendering, partial rendering, or fallback

## Resource Locations

The same invalid SVG cases are synced to all three example apps:

- Android: `svg/examples/android/app/src/main/assets/svg/`
- iOS: `svg/examples/iOS/ServalPark/ServalPark/svg/`
- Harmony: `svg/examples/harmony/entry/src/main/resources/rawfile/svg/`

Harmony keeps an explicit file list in:

- `svg/examples/harmony/entry/src/main/ets/pages/Index.ets`

Android and iOS group these files into `IllegalParsing` by the `invalid-`
filename prefix.

## Pass Criteria

Each case should meet all of the following:

- No crash
- No infinite recursion
- No non-terminating parse loop
- No long UI freeze
- The example page remains scrollable and interactive

Acceptable outcomes for a single case:

- Blank output
- Partial output
- Property fallback
- Ignored invalid subtree

## Risk Categories

### P0 Crash Risk

These cases historically map to native crash or parser loop bugs and should be
treated as must-pass:

- `invalid-extra-close-tag.svg`
- `invalid-mismatched-close-tag.svg`
- `invalid-unclosed-root-tag.svg`
- `invalid-path-garbage-token.svg`
- `invalid-path-incomplete-command.svg`
- `invalid-polyline-garbage-token.svg`
- `invalid-use-self-reference.svg`
- `invalid-use-mutual-reference.svg`
- `invalid-use-long-cycle.svg`

### P1 Hang Risk

These cases focus on recursion, deep reference chains, and expensive invalid
graph expansion:

- `invalid-use-deep-chain.svg`
- `invalid-use-self-reference-in-clipPath.svg`
- `invalid-clipPath-recursive-iri.svg`
- `invalid-mask-recursive-iri.svg`
- `invalid-pattern-use-cycle.svg`

### P1 Reference Resolution Risk

These cases verify malformed or cyclic IRI resolution:

- `invalid-iri-empty.svg`
- `invalid-iri-test.svg`
- `invalid-pattern-href-cycle.svg`

### P1 Parser Robustness Risk

These cases verify malformed numeric inputs, truncated commands, and invalid
tokens:

- `invalid-path-arc-flag.svg`
- `invalid-polyline-odd-points.svg`
- `invalid-viewBox-garbage-token.svg`
- `invalid-transform-garbage-token.svg`
- `invalid-transform-incomplete.svg`

### P2 Property Fallback Risk

These cases validate invalid style and color parsing fallback behavior:

- `invalid-style-garbage.svg`
- `invalid-color-function.svg`

## Suggested Execution Order

### Round 1: Core Stability

- `invalid-extra-close-tag.svg`
- `invalid-mismatched-close-tag.svg`
- `invalid-path-garbage-token.svg`
- `invalid-polyline-garbage-token.svg`
- `invalid-use-self-reference.svg`

### Round 2: Recursive Reference Safety

- `invalid-use-mutual-reference.svg`
- `invalid-use-long-cycle.svg`
- `invalid-use-self-reference-in-clipPath.svg`
- `invalid-mask-recursive-iri.svg`

### Round 3: Reference and Parser Follow-up

- `invalid-pattern-href-cycle.svg`
- `invalid-pattern-use-cycle.svg`
- `invalid-transform-garbage-token.svg`
- `invalid-viewBox-garbage-token.svg`

### Round 4: Full IllegalParsing Sweep

- Run the full `IllegalParsing` category on all three platforms
- Confirm the page remains interactive during and after rendering

## Known Fix Areas Covered By This Set

The current regression set covers bugs or risk areas around:

- XML end-tag stack safety
- unterminated root / container tag rejection
- `path` invalid token handling
- `polyline` invalid token handling
- `<use>` recursive self-reference and cycle detection
- `clipPath` recursive reference expansion
- `mask` recursive reference expansion
- `pattern href` cycle handling
- invalid transform, style, color, and viewBox parsing

## Maintenance Notes

When adding new invalid cases:

- Prefix filenames with `invalid-`
- Keep the SVG minimal and single-purpose
- Prefer one failure mode per file
- Sync the file to Android, iOS, and Harmony resources
- Add the file to Harmony's explicit list in `Index.ets`
- Classify the case into one of the categories in this document

Recommended future additions:

- gradient href cycles
- empty mask / empty pattern / empty gradient fallback
- extremely deep nested container chains
- oversized numeric values and overflow-focused inputs
