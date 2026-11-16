# Skity SVG filter regression tests

These standalone tests build the current SVG sources and exercise the Metal
backend. They require macOS, a Metal-capable device, CMake 3.20+, a C++17
compiler, GoogleTest sources, and matching Skity sources and a compiled Skity
library. Build Skity with Metal enabled before configuring these tests. The
headers and library must come from the same Skity revision and configuration.

```sh
cmake -S svg/tests -B build/svg-filter-tests \
  -DSKITY_ROOT=/path/to/skity \
  -DSKITY_BUILD=/path/to/skity/build \
  -DGTEST_ROOT=/path/to/googletest
cmake --build build/svg-filter-tests --parallel
ctest --test-dir build/svg-filter-tests --output-on-failure
```

`GTEST_ROOT` defaults to `SKITY_ROOT/third_party/gtest`. `SKITY_LIBRARY` may
also be set explicitly to select a matching static or dynamic Skity library.
No external test dependencies are downloaded by this CMake project.

Coverage includes SourceAlpha and named inputs, normal blend ordering,
alpha-only arithmetic subtraction, primitive clipping, nested filters and
transforms, unsupported-graph fallback, resource admission limits and bounded
nested display-list recording. Rendering checks read back RGBA pixels from a
Metal texture. A system without Metal cannot run these rendering checks.

Regression cases also cover translated flood regions, blur sampling outside a
primitive's output subregion, downstream output clipping, disabled negative
blur deviations, near-parallel affine transforms and physical workspace
admission on Retina surfaces. Direct and recorded paths are exercised at 2x.

Hosts must pass the destination `GPUSurface::ContentScale()` to
`SrSkityCanvas(canvas, image_callback, content_scale)`. Use that destination
scale when wrapping a recording canvas too. The default of one is for 1x
surfaces; canvas matrix scaling is accounted for independently. Perspective
transforms are outside the admitted graph subset.

The supported graph subset is bounded and qualified for sRGB on Metal.
These tests do not establish software, OpenGL, Vulkan or mobile backend parity,
general arithmetic compositing, or linearRGB color interpolation support.
