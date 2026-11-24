# Harmony text regressions

The tests render the actual ServalSVG backend into NativeDrawing RGBA bitmaps
and compare it with independently constructed native typography using the same
system fonts. This checks SVG positioning and style semantics without assuming
identical glyphs across operating systems.

Cases cover alphabetic baselines, start/middle/end anchors, Chinese/English
mixed text, nested tspan style restoration, currentColor (including the host
default), fill-opacity inheritance, inline style, transform and whole-text
opacity, invisible text, and paragraph lifetime after factory reset/destruction.
Pixel channels allow a tolerance of 2 for premultiplied alpha rounding.

Run from the repository root with DevEco Studio, an authorized device, and a
local signing configuration containing `app.signingConfigs`:

```bash
source tools/envsetup.sh
./hab sync .
python3 svg/tools/run_harmony_text_tests.py \
  --device <hdc-device-id> \
  --signing-config /path/to/local-build-profile.json5
```

The runner builds an isolated copy of the example application and a test-only
NAPI module in the system temporary directory. It installs the signed example
HAP (`net.lynx.servalsvg`), launches the regression page, exports the report from
the app sandbox, and requires the current run to report `TEXT_TESTS_RESULT=PASS`.
It replaces the installed example app with the test app; rebuild/install the
ordinary example to return to the preview browser. Tests are not linked into
the published ServalSVG library.

On devices allowing shell executables, `svg/testing/harmony/CMakeLists.txt`
also builds a standalone `text_tests` executable.
