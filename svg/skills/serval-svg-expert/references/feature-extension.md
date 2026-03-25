# Feature Extension Checklist

Use this checklist for new SVG capabilities or when converting parsed-only
support into rendered support.

## Required Layers

1. Parser
   Add or update construction in `svg/src/parser/SrSVGDOM.cc`.
2. Node Model
   Add the node class, fields, and `ParseAndSetAttribute()` handling.
3. Render Path
   Connect the node into `OnRender()`, `AsPath()`, reference resolution, or
   render-state assembly.
4. Backend Support
   Implement the behavior in the relevant backends. Parser-only support is not
   feature completion.
5. Samples / Tests
   Add or update SVG fixtures in `svg/test_cases/` or `svg/examples/`.
6. Internal Audit
   Regenerate `svg/docs/audit-matrix.json` and update `svg/docs/deepwiki.md`.
7. Public Contract
   Regenerate `svg/docs/support-matrix.json`.
8. Skills
   Update both `serval-svg-expert` and `serval-svg` if the support surface
   changed.

## Filter-Specific Guidance

For filter work such as `feGaussianBlur`:

- verify `SupportsFilters()` on each target backend
- check whether the backend consumes only selected primitive attributes
- check whether `in` / `result` chaining is actually implemented
- check whether filter region attributes are consumed or ignored
- test the primitive both alone and inside a multi-step filter chain

## Definition Of Done

The work is not done when the parser recognizes the tag.

The work is done when:

- the target backend renders the intended effect
- the examples cover it
- the audit matrix reflects it accurately
- the public support contract reflects it accurately
- the deep wiki reflects it accurately
- the skills stop describing the old behavior
