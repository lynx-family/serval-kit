# Serval Kit

Serval Kit aims to build a cross‑platform, general‑purpose UI component library for multi‑device scenarios. By providing unified rendering and interaction capabilities, it enables consistent experiences and efficient delivery across platforms. At this stage, the project focuses on SVG and Markdown, delivering reliable solutions for vector graphics and rich‑text rendering in mobile and cross‑platform applications, laying a foundation for future component expansion and ecosystem growth.

## Project Structure
- markdown: Core Markdown renderer and platform adaptation
  - Headers and core implementation: [include](file:///Users/bytedance/Downloads/open/serval-kit/markdown/include) and [src](file:///Users/bytedance/Downloads/open/serval-kit/markdown/src)
  - Platform bindings: Android, iOS, HarmonyOS: [platform](file:///Users/bytedance/Downloads/open/serval-kit/markdown/platform)
  - Example projects and templates: [example](file:///Users/bytedance/Downloads/open/serval-kit/markdown/example)
  - Unit and integration tests: [testing](file:///Users/bytedance/Downloads/open/serval-kit/markdown/testing)
- svg: Core SVG renderer and platform adaptation
  - Samples and demo projects: [examples](file:///Users/bytedance/Downloads/open/serval-kit/svg/examples)
  - Platform projects and packaging: [platform](file:///Users/bytedance/Downloads/open/serval-kit/svg/platform)
  - Headers and core implementation: [include](file:///Users/bytedance/Downloads/open/serval-kit/svg/include)

## Quick Start
- Android sample (Markdown):

```bash
cd markdown/example/android
./gradlew assembleDebug
```

- Android sample (SVG):

```bash
cd svg/examples/android
./gradlew assembleDebug
```

- iOS samples (Markdown / Swift / Objective‑C):
  - Run `pod install` in the corresponding sample directory, then open the `.xcworkspace` in Xcode to build and run
  - Core podspecs: [ServalMarkdown.podspec](file:///Users/bytedance/Downloads/open/serval-kit/ServalMarkdown.podspec) and [ServalSVG.podspec](file:///Users/bytedance/Downloads/open/serval-kit/svg/ServalSVG.podspec)

- HarmonyOS samples:
  - Navigate to `markdown/example/harmony` or `svg/examples/harmony`, open and build with DevEco Studio

## Build & Integration
- Build the core library with CMake (Markdown as an example):

```bash
cd markdown
cmake -S . -B build
cmake --build build -j
```

## Testing
- Markdown core tests:
  - Test directory: [testing](file:///Users/bytedance/Downloads/open/serval-kit/markdown/testing)
  - Build and run tests via CMake

## Code Style
- Unified Clang‑Format and Clang‑Tidy: [.clang-format](file:///Users/bytedance/Downloads/open/serval-kit/.clang-format), [.clang-tidy](file:///Users/bytedance/Downloads/open/serval-kit/.clang-tidy)
- GN and CMake build scripts: [CMakeLists.txt](file:///Users/bytedance/Downloads/open/serval-kit/CMakeLists.txt), [BUILD.gn](file:///Users/bytedance/Downloads/open/serval-kit/svg/BUILD.gn)

## License
- For commercial use, please review the licenses of the dependencies and follow the corresponding requirements

## Contributing
- Contributions and feedback via Pull Requests and Issues are welcome
- Before submitting, please ensure tests pass and follow the project’s code style
