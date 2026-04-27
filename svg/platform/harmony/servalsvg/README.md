<div>

## Introduction

serval-svg is a cross-platform svg library supporting iOS, Android, Harmony platform.

## Installation

```bash
ohpm install @lynx/servalsvg
```

## How to use

You can use serval-svg in your HarmonyOS project. add dependency in oh-package.json5 like this:

```json5
{
  "dependencies": {
    "@lynx/servalsvg": "0.0.1-alpha.1",
  }
}
```

If you need to inspect SVG parse/update errors, call `updateData()` and only log
when the returned result reports an error:

```ts
import { SvgData, SvgRenderNodeController } from '@lynx/servalsvg'

const controller = new SvgRenderNodeController()
const result = controller.updateData(new SvgData({
  width: 100,
  height: 100,
  content: svgContent,
}))

if (result.hasError) {
  console.info(`serval-svg error: ${result.errorMessage ?? ''}`)
}
```
</div>
