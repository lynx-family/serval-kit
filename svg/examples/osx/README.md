# macOS Metal SVG Rendering Example

This is an Xcode project example for rendering SVG on macOS using the Metal backend of the Skity library.

## Directory Structure

```
osx/
├── README.md                              # This documentation
└── SVGMetaRenderer/
    ├── SVGMetaRenderer.xcodeproj/        # Xcode project file
    │   ├── project.pbxproj
    │   └── project.xcworkspace/
    └── SVGMetaRenderer/                   # Source code directory
        ├── AppDelegate.h                  # Application delegate header
        ├── AppDelegate.m                  # Application delegate implementation
        ├── ViewController.h               # View controller header
        ├── ViewController.mm              # View controller implementation
        ├── SVGMetalView.h                 # Metal rendering view header
        ├── SVGMetalView.mm                # Metal rendering view implementation (core rendering logic)
        ├── main.m                         # Program entry point
        ├── Info.plist                     # Application configuration
        └── svg/                           # SVG resource file directory
            ├── basic_shapes.svg
            ├── clip_path.svg
            ├── defs_use.svg
            ├── gradients.svg
            ├── image.svg
            ├── mask-alpha-units-test.svg
            ├── mask-comprehensive-test.svg
            ├── mask-luminance-gradient-test.svg
            ├── paths.svg
            ├── text.svg
            └── transforms.svg
```

## Features

1. **Metal Hardware Acceleration**: High-performance SVG rendering using the Metal backend of the Skity library
2. **SVG Parsing and Rendering**: Integrated with the Serval SVG library for SVG file parsing and rendering
3. **Interactive SVG Selection**: Switch between different SVG example files via dropdown menu
4. **Real-time Rendering**: Smooth screen updates using CVDisplayLink

## Core Files Description

### SVGMetalView.mm

This is the core rendering implementation file, containing the following key features:

1. **Skity GPU Context Initialization**:
   ```objc
   _gpuContext = skity::MTLContextCreate(_device, _commandQueue);
   ```

2. **GPU Surface Creation**:
   ```objc
   skity::GPUSurfaceDescriptorMTL desc{};
   desc.backend = skity::GPUBackendType::kMetal;
   desc.surface_type = skity::MTLSurfaceType::kLayer;
   desc.layer = _metalLayer;
   _gpuSurface = _gpuContext->CreateSurface(&desc);
   ```

3. **SVG Rendering Pipeline**:
   - Load SVG file and parse into SrSVGDOM
   - Create SrSkityCanvas to wrap Skity Canvas
   - Call svg_dom_->Render() for rendering

4. **CVDisplayLink Animation Loop**: 60fps rendering loop using Core Video Display Link

### ViewController.mm

Responsible for UI interaction and SVG file loading:

- Create and layout UI elements
- Manage SVGMetalView lifecycle
- Handle SVG file loading logic

## Compilation and Execution

### Prerequisites

1. Xcode 14.0 or later
2. macOS 12.0 or later
3. Skity library (located at `/skity`)
4. Serval SVG library (located at `/serval-kit/svg`)

### Project Dependency Configuration

This project is pre-configured with:

1. **Serval SVG Library**:
   - All source files added to the ServalSVG group in the Xcode project
   - Header search paths configured
   - Source files included in the compilation phase

2. **Skity Library**:
   - Header search paths configured
   - Requires Skity library to be built before linking

### Building the Skity Library

Due to the large number of Skity library source files, it is recommended to use Skity's provided packaging script to build an xcframework:

```bash
cd /skity

# Sync dependencies
./tools/hab sync

# Build xcframework
python3 tools/pack_skity_framework.py
```

After building is complete, `skity.xcframework` will be generated in the `out/` directory.
Drag `skity.xcframework` into the Xcode project.

### Configuring the Xcode Project

1. Open the Xcode project:
   ```bash
   open SVGMetaRenderer/SVGMetaRenderer.xcodeproj
   ```
2. Select target device (My Mac)

3. Click Build & Run (⌘R)

### Current Project Configuration

This project is pre-configured with:

**Header Search Paths** (already configured):
```
$(inherited)
$(SRCROOT)/../../../include
$(SRCROOT)/../../../../skity/include
$(SRCROOT)/../../../../skity
$(SRCROOT)/../../../../skity/third_party/glm
```

**User Header Search Paths** (already configured):
```
$(inherited)
$(SRCROOT)/../../../include
$(SRCROOT)/../../../../skity/include
```

**Other C++ Flags** (already configured):
```
-std=c++17 -stdlib=libc++
```

**ServalSVG Source Files** (already added to the project):
- Skity platform adapter: SrSkityCanvas.cc, SrSkityParagraph.cc
- SVG elements: All SrSVG*.cc files
- SVG parser: All SrDOM*.cc, SrXML*.cc files

## Code References

This project references the following code:

1. **Skity Basic Example**: `skity/example/case/basic`
2. **iOS SVG Example**: `serval-kit/svg/examples/iOS`
3. **Skity Metal Window**: `skity/example/common/mtl`

## Extended Features

Features that can be further extended:

1. **Image Loading Support**: Implement SrSkityCanvas's ImageCallback to support the `<image>` element in SVG
2. **SVG Zoom and Pan**: Add gesture support to zoom and pan SVG
3. **SVG Animation**: Support SVG animation elements
4. **SVG Editing**: Add simple SVG editing capabilities
5. **Export Functionality**: Support exporting rendering results as images

## License

Copyright 2025 The Lynx Authors. All rights reserved.
Licensed under the Apache License Version 2.0.
