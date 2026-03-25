# Expert Workflows

## Refresh Knowledge Artifacts

From the repository root:

```bash
python3 svg/tools/svg_support_audit.py audit-matrix --output svg/docs/audit-matrix.json
python3 svg/tools/svg_support_audit.py matrix --output svg/docs/support-matrix.json
```

Then review:

- `svg/docs/audit-matrix.json`
- `svg/docs/support-matrix.json`
- `svg/docs/deepwiki.md`

Refresh the deep wiki whenever:

- parser tag coverage changes
- platform readiness changes
- a tag or attribute moves from parsed-only to rendered
- a backend gains or loses support
- the public support contract changes meaningfully

## Analyze A Concrete SVG

For an internal engineering answer:

```bash
python3 svg/tools/svg_support_audit.py analyze --profile audit --svg-file path/to/input.svg --platform all
```

For a backend-specific engineering answer:

```bash
python3 svg/tools/svg_support_audit.py analyze --profile audit --svg-file path/to/input.svg --platform ios
python3 svg/tools/svg_support_audit.py analyze --profile audit --svg-file path/to/input.svg --platform harmony
```

For a public-contract check:

```bash
python3 svg/tools/svg_support_audit.py analyze --svg-file path/to/input.svg --platform all
```

Interpret expert results in this order:

1. unsupported tags
2. parsed-but-not-rendered attributes
3. partial tags or partial attributes
4. backend-specific gaps

## Audit A Support Claim

When a user asks "does serval-svg support X?":

1. check `svg/docs/support-matrix.json` for the public answer
2. check `svg/docs/audit-matrix.json` for the internal reasoning
3. verify the cited tag in `svg/src/parser/SrSVGDOM.cc`
4. verify the relevant attributes in `svg/src/element/*.cc`
5. verify backend readiness in `svg/platform/*`

Do not stop after step 1 if the task is an expert audit.
