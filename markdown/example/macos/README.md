# Serval Markdown macOS Demo

This demo renders Serval Markdown in a GLFW macOS window using Skity with the
Metal backend.

The demo is split into two layers:

- `markdown/example/skity_demo`: pure C++ Skity/Markdown demo logic, built as
  `serval_markdown_skity_demo`.
- `markdown/example/macos/app`: thin macOS host for GLFW, Cocoa, Metal surface
  management and input normalization.

Build from the repository root:

```bash
markdown/tools/macos_build.sh
```

Run:

```bash
./markdown/example/macos/build/serval_markdown_macos_demo
```

Keyboard:

- `Esc`: close the window
- `Left` / `Right`: switch markdown cases
- mouse wheel or `Up` / `Down`: scroll
- `Space`: toggle typewriter animation
- `R`: restart the animation

The demo also has clickable toolbar controls for case navigation, animation
pause/play and animation reset.

Mouse:

- Click markdown links and images to exercise `MarkdownEventListener` callbacks.
- Long press text to enter selection, then drag the selection handles.
- Drag wide table/code regions horizontally when the markdown case exposes a
  scroll-x region.
- Drag the document vertically as an alternative to wheel/key scrolling.

Frame scheduling:

- `MarkdownView::OnLayoutFrame` and `MarkdownView::OnRendererFrame` are driven
  by the main Metal render loop.
