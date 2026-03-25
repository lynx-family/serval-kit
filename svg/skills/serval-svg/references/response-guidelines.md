# Response Guidelines

Use these guidelines when answering support questions with `serval-svg`.

## Default Answer Shape

For concise support questions:

1. answer the support question directly
2. state `supported`, `partial`, or `unsupported`
3. mention platform differences if they matter
4. mention the most important attribute caveat only

Example:

- `linearGradient` is supported across Android, iOS, Harmony, and Skity.
- `feGaussianBlur` is partial support only. It is not a cross-platform feature,
  and only Skity has partial availability.

## Concrete SVG Inputs

When the user gives a real SVG:

1. analyze it with the public profile
2. summarize blockers first
3. then summarize partial support
4. then state the likely supported subset

Use:

```bash
python3 svg/tools/svg_support_audit.py analyze --svg-file path/to/input.svg --platform all
```

## Boundary

Stay inside the public support contract.

- Do not explain parser or render internals.
- Do not cite source files.
- Do not rely on `deepwiki.md`.

If the user wants the reason behind the support answer, escalate to
`serval-svg-expert`.
