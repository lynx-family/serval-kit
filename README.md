# Serval Kit

Serval Kit aims to build a cross‑platform, general‑purpose UI component library for multi‑device scenarios. By providing unified rendering and interaction capabilities, it enables consistent experiences and efficient delivery across platforms. At this stage, the project focuses on SVG and Markdown, delivering reliable solutions for vector graphics and rich‑text rendering in mobile and cross‑platform applications, laying a foundation for future component expansion and ecosystem growth.

## Project Structure
- markdown: Core Markdown renderer and platform adaptation
  - Headers and core implementation: [include](https://github.com/lynx-family/serval-kit/tree/main/markdown/include) and [src](https://github.com/lynx-family/serval-kit/tree/main/markdown/src)
  - Platform bindings: Android, iOS, HarmonyOS: [platform](https://github.com/lynx-family/serval-kit/tree/main/markdown/platform)
  - Example projects and templates: [example](https://github.com/lynx-family/serval-kit/tree/main/markdown/example)
  - Unit and integration tests: [testing](https://github.com/lynx-family/serval-kit/tree/main/markdown/testing)
- svg: Core SVG renderer and platform adaptation
  - Samples and demo projects: [examples](https://github.com/lynx-family/serval-kit/tree/main/svg/examples)
  - Platform projects and packaging: [platform](https://github.com/lynx-family/serval-kit/tree/main/svg/platform)
  - Headers and core implementation: [include](https://github.com/lynx-family/serval-kit/tree/main/svg/include)

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
  - Core podspecs: [ServalMarkdown.podspec](https://github.com/lynx-family/serval-kit/tree/main/ServalMarkdown.podspec) and [ServalSVG.podspec](https://github.com/lynx-family/serval-kit/tree/main/svg/ServalSVG.podspec)

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
  - Test directory: [testing](https://github.com/lynx-family/serval-kit/tree/main/markdown/testing)
  - Build and run tests via CMake

## Code Style
- Unified Clang‑Format and Clang‑Tidy: [.clang-format](https://github.com/lynx-family/serval-kit/tree/main/.clang-format), [.clang-tidy](https://github.com/lynx-family/serval-kit/tree/main/.clang-tidy)
- GN and CMake build scripts: [CMakeLists.txt](https://github.com/lynx-family/serval-kit/tree/main/CMakeLists.txt), [BUILD.gn](https://github.com/lynx-family/serval-kit/tree/main/svg/BUILD.gn)

## License
- For commercial use, please review the licenses of the dependencies and follow the corresponding requirements

## Contributing
- Contributions and feedback via Pull Requests and Issues are welcome
- Before submitting, please ensure tests pass and follow the project’s code style
