# SVG rendering regressions

## Marker regressions

`marker_tests.cc` exercises shared marker geometry and DOM rendering without a
platform UI. It covers inheritance, CSS shorthand and overrides, subpaths,
closed paths, zero-length segments, Bezier and arc tangents, units, reference
points, orientation, clipping, recursion, opacity, and animation restoration.
It also checks shared container/use inheritance restoration, nested use paint
precedence, use stroke overrides, unresolved-marker opacity composition, and
marker placement after animated path updates. Reentrant marker/use cases cover
context paint lifetimes, currentColor snapshots across instances, and restoration
of enclosing and initial shape render state.
Default `PathFactory` coverage checks command recording and returned paths,
primitive vertices and arc tangents, polygon closure, single-point paths,
rounded-rect radius clamping, and empty shapes.

From the OSS repository root:

```bash
bash svg/tools/run_marker_tests.sh
SERVAL_MARKER_SANITIZE=1 bash svg/tools/run_marker_tests.sh
```

The runner prepares the toolchain and testing dependencies. For an already
synchronized checkout, `SERVAL_MARKER_SKIP_SYNC=1` avoids another Habitat sync.
Sanitized runs enable ASan stack-use-after-return detection.
Build products stay in the temporary directory; override
`SERVAL_MARKER_TEST_BUILD_DIR` when needed.

`marker_cases.json` records Chrome reference pixels for
`svg/test_cases/marker_*.svg`. Verify or intentionally regenerate them with:

```bash
python3 svg/tools/check_marker_chrome.py
python3 svg/tools/check_marker_chrome.py --update
```

Run the same probes through the actual Skity software backend on macOS:

```bash
bash svg/tools/run_skity_pattern_percent_tests.sh \
  --skity-source /path/to/skity \
  --manifest "$PWD/svg/testing/marker_cases.json" \
  --build-dir "$PWD/build/svg-marker-skity"
```

The runner emits PNGs and `results/results.json`. Probe locations avoid
antialiasing edges; channels allow a difference of two bytes for compositing.
Device pixel validation on iOS and Harmony remains separate work.
Context paints currently support solid colors and none; context gradients and
patterns render as none. Marker content otherwise uses the renderer's existing
child-element capabilities.

Android packages these fixtures in its example and exposes a `Marker` preview
category. With a connected Android device, run the marker instrumentation:

```bash
cd svg/examples/android
./gradlew :app:connectedDebugAndroidTest \
  -Pandroid.testInstrumentationRunnerArguments.class=com.lynx.serval.svg.example.MarkerInstrumentedTest
```

The tests check transparent software output and hardware Canvas output captured
with PixelCopy. Hardware probes compare Chrome pixels composited over the
window's white background. Both modes write per-case PNGs and `results.json`
under the app's private `files/marker-regression/software` and `hardware`
directories. PixelCopy needs Android 8 or later.
The five cases and 37 probes passed in both modes on a physical Xiaomi MIX 2S
running Android 10 (API 29).

Open the examples directly on the device:

```bash
adb shell am start -n com.lynx.serval.svg.example/.MainActivity --es category Marker
```

## Marker physical-device runners

All eight marker fixtures and 99 RGBA probes have passed on iPhone 11
(iOS 18.3.1, Core Graphics) and ALN-AL80 (OpenHarmony 6.1.0.115, ArkUI snapshots).
Channel tolerance is 3/255; transparent reference pixels are checked too.

After `source tools/envsetup.sh` and `./hab sync .`, build the standalone iOS
app with a valid signing identity and a profile covering your physical device:

```bash
python3 svg/tools/build_marker_ios_device.py \
  --profile "$SERVAL_IOS_PROFILE" --identity "$SERVAL_IOS_IDENTITY" \
  --output build/marker-ios-device
xcrun devicectl device install app --device "$SERVAL_IOS_DEVICE" build/marker-ios-device/MarkerDevice.app
xcrun devicectl device process launch --device "$SERVAL_IOS_DEVICE" --terminate-existing com.lynx.servalkit.svg.marker
xcrun devicectl device copy from --device "$SERVAL_IOS_DEVICE" \
  --domain-type appDataContainer --domain-identifier com.lynx.servalkit.svg.marker \
  --source Documents --destination build/marker-ios-results
```

Wait for the app's PASS/FAIL label before copying. `Documents` contains
`results.json`, original-size PNGs, and `app-view.png` captured from the
running view. The build uses current C/C++ and iOS sources directly, without
requiring CocoaPods or an XCTest host.

Build/install Harmony's signed example as described below, then run:

```bash
hdc -t "$SERVAL_HARMONY_DEVICE" shell aa force-stop net.lynx.servalsvg
hdc -t "$SERVAL_HARMONY_DEVICE" shell aa start -a EntryAbility -b net.lynx.servalsvg --pb markerRegression true
hdc -t "$SERVAL_HARMONY_DEVICE" file recv -b net.lynx.servalsvg \
  /data/storage/el2/base/haps/entry/files/marker-regression.json ./marker-regression.json
```

Wait for the PASS/FAIL label before exporting. Each `marker_*.png` is saved in
the same ability directory. The page displays these snapshot images. The
runner uses the marker manifest while retaining the pattern regression defaults.

### MDN marker examples

Three cases come from the [MDN marker examples](https://developer.mozilla.org/en-US/docs/Web/SVG/Reference/Element/marker#examples):

| Fixture | Coverage | Probes |
| --- | --- | --- |
| `marker_mdn_arrowheads.svg` | Line and cubic-path arrowheads; automatic start/mid/end orientation | 14 |
| `marker_mdn_polymarkers.svg` | Polyline axis arrowheads and red vertex dots | 20 |
| `marker_mdn_context_paint.svg` | Default fill, context-fill/context-stroke, none, and stroke-width scaling | 28 |

Explicit pixel dimensions are added without changing the viewBox or geometry.
The context paint case expands MDN's `path { marker: url("#circle"); }` rule
into equivalent inline `style` attributes. The original stylesheet example is
retained separately as `testing/fixtures/marker_mdn_context_paint_stylesheet.svg`;
it is not a positive render regression because stylesheet selectors remain
unsupported. Probe locations avoid antialiased shape boundaries.

Regenerate/sync the shared reference and both packaged asset sets with:

```bash
python3 svg/tools/check_marker_chrome.py --update --sync-android --sync-harmony --sync-ios
```

Skity's shared path drawing entry now honors the SVG initial black fill when
no fill is specified. These MDN arrows verify that behavior, and the context
paint fixture verifies that explicit `fill="none"` remains transparent.
The original 14 Skity pattern fixtures also pass after the correction.
Android's packaged assets contain all eight cases; its previously recorded
physical-device run covered the original five cases and 37 probes.
All eight SVGs are also present under the iOS example's `svg` folder, which
Xcode bundles as a folder resource. Android, Harmony and iOS example browsers
expose a `Marker` category. The iOS pixel/device verification above uses the
standalone regression app; the ordinary ServalPark browser is a separate entry.

## Pattern percentage regressions

For Harmony native text positioning, style inheritance, opacity, and resource
lifetime regressions, see [harmony/README.md](harmony/README.md).

`pattern_percent_cases.json` contains shared RGBA probes generated by Chrome
from the SVGs in `svg/test_cases/`. Fixtures use embedded PNGs, so pixel checks
do not depend on network access. Probe locations avoid antialiased boundaries.

The cases distinguish numeric bounding-box fractions from percentages and cover
pattern content units, pattern viewBox transforms, tile origins, image geometry,
`use`, percentage stroke widths, and ordinary image/vector controls. A pattern
does not establish a new SVG viewport; its content transform must not replace
the referencing SVG's percentage basis.

From the repository root, verify the browser reference with:

```bash
python3 svg/tools/check_pattern_percent_chrome.py
```

Use `--chrome PATH` or `CHROME_BIN` if Chrome is not found automatically. To
intentionally regenerate reference pixels and Android's packaged test assets:

```bash
python3 svg/tools/check_pattern_percent_chrome.py --update --sync-android
```

The manifest records the browser version. Rendered Chrome PNGs are written to
the ignored `build/pattern-percent-chrome/` directory.

Run the Android instrumented tests with a connected device or emulator:

```bash
source tools/envsetup.sh
./hab sync .
cd svg/examples/android
./gradlew :app:connectedDebugAndroidTest
```

Run the iOS XCTest suite on an available simulator:

```bash
bash svg/tools/run_ios_pattern_tests.sh
```

It requires Xcode, an installed iOS simulator runtime, Ruby, CocoaPods, and the
`xcodeproj` gem. Pass an xcodebuild destination as the first argument to choose
a device. The generated project, PNG attachments, and `.xcresult` bundles stay
under `build/ios-pattern-tests/`.
The same suite also checks the demo image loader's request coalescing, weak view
listeners, and the actual preview's redraw after an asynchronous image arrives.

Run the Skity software backend on macOS with a populated Skity checkout:

```bash
bash svg/tools/run_skity_pattern_percent_tests.sh --skity-source /path/to/skity
```

This builds the dependency and the actual ServalSVG backend, then writes PNGs
and `results.json` under `build/svg-pattern-percent-skity/runner/results/`.
Use `--help` to select existing dependency/build directories or a CMake binary.

Harmony's example includes a regression page using the same manifest and
fixtures. It snapshots the real `SvgRenderNodeController`, waits for embedded
images and completed rendering, and checks PixelMap RGBA samples. With the
DevEco toolchain and signing configured, build the example's signed entry HAP:

```bash
source tools/envsetup.sh
./hab sync .
cd svg/examples/harmony
./hvigorw assembleHap --mode module -p module=entry@default -p product=default -p buildMode=debug --no-daemon
```

Put the SDK's `hdc` on PATH and select a connected device from `hdc list targets`:

```bash
SERVAL_HARMONY_DEVICE=your-device-id
hdc -t "$SERVAL_HARMONY_DEVICE" install -r entry/build/default/outputs/default/entry-default-signed.hap
hdc -t "$SERVAL_HARMONY_DEVICE" shell aa force-stop net.lynx.servalsvg
hdc -t "$SERVAL_HARMONY_DEVICE" shell aa start -a EntryAbility -b net.lynx.servalsvg --pb patternRegression true
hdc -t "$SERVAL_HARMONY_DEVICE" shell hilog -x -e SVGPATTERN
```

Wait for `SVGPATTERN_RESULT`, then export the report with
`hdc -t "$SERVAL_HARMONY_DEVICE" file recv -b net.lynx.servalsvg /data/storage/el2/base/haps/entry/files/pattern-regression.json ./pattern-regression.json`.
Force-stop and start the ability without `--pb patternRegression true` to return
to the ordinary example browser. An NDK build alone does not check pixel parity.

## SVG parser and rendering regressions

Run the shared parser and DOM-to-canvas tests with Clang, ASan and UBSan.
The second command compiles the parser with C++ exceptions disabled, injects
persistent allocation failures and runs the optional libFuzzer target.

```sh
python3 svg/tools/run_parser_regressions.py
python3 svg/tools/run_parser_core_tests.py --fuzz-seconds 60
```

Use `--fuzzer-library /path/to/libFuzzer.a` if Clang has no bundled fuzzer.
No-exception OOM coverage applies to the shared parser and XML DOM. SVG node
construction, SMIL and platform caches still contain allocating STL calls.

The iOS XCTest command above also checks arc geometry, malformed inputs and
SMIL path and dash rendering. For macOS image loading, pattern fill and stroke,
and SMIL path frames on Metal, run the preview regression with a built Skity
XCFramework.

```sh
python3 svg/tools/run_macos_skity_regressions.py
```
