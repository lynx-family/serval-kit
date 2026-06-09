# ServalSVG 官方文档

## 1. 产品简介

`ServalSVG` 是 `serval-kit/svg` 提供的跨平台 SVG 渲染能力，面向 Android、iOS、Harmony 等客户端场景，适合用于图标、插画、运营素材、模板图形等静态或轻交互 SVG 内容的渲染。

本文档用于说明：

- 产品定位与能力边界
- 平台接入方式
- 最小使用示例
- 已实现与有限实现的 SVG 能力
- 已知限制与使用建议

如果你需要按组件接口、标签、属性、平台差异进行查表，建议同时阅读 `svg/docs/reference.md`。

### 1.1 适用场景

推荐将 `ServalSVG` 用于以下场景：

- 多平台共享的图标、插画、装饰图形
- 运营位图资源难以覆盖的轻量矢量素材
- 需要通过 `currentColor` 跟随宿主主题色的 SVG 资源
- 已经过模板化治理、结构稳定、可回归验证的业务素材

### 1.2 不适用场景

以下场景不建议直接依赖 `ServalSVG` 作为唯一渲染方案：

- 依赖复杂滤镜链、混合模式、完整 CSS 布局语义的 SVG
- 强依赖文本排版一致性、字距、描边文本等高级文本能力的素材
- 需要完整网络图片、外链资源、脚本、动画驱动的 SVG
- 未经素材治理、来源复杂且无法建立回归样例的动态 SVG 输入

## 2. 核心能力

ServalSVG 当前重点覆盖以下能力：

- 基础图形渲染：`rect`、`circle`、`ellipse`、`line`、`polygon`、`polyline`、`path`
- 容器与复用：`svg`、`g`、`defs`、`use`
- 渐变：`linearGradient`、`radialGradient`、`stop`
- 裁剪与遮罩：`clipPath`、`mask`
- 文本与图片的基础支持：`text`、`tspan`、`image`
- 主机默认颜色注入：用于 `currentColor` 场景
- 渲染诊断输出：可在接入层读取解析或更新错误

### 2.1 设计目标

`ServalSVG` 的目标是提供一套可控、可裁剪、可回归验证的客户端 SVG 渲染能力，而不是完整复刻浏览器 SVG 规范行为。对业务接入而言，这意味着：

- 优先保证基础图形、路径、渐变、裁剪、遮罩等主干能力稳定
- 对高复杂度能力按标签、属性、平台逐步开放，而不是默认全部支持
- 以当前版本中的实际解析能力、渲染能力和平台接入能力共同决定实际能力边界

## 3. 平台支持概览

以下为当前版本的实际能力结论：

| 能力分类 | Android | iOS | Harmony | Skity |
| --- | --- | --- | --- | --- |
| 基础图形与路径 | 支持 | 支持 | 支持 | 支持 |
| `defs` / `use` | 支持 | 支持 | 支持 | 支持 |
| 渐变 / `pattern` | 支持 | 支持 | 支持 | 支持 |
| `clipPath` / `mask` | 支持 | 支持 | 支持 | 支持 |
| `image` | 支持，需宿主图片提供者 | 支持，需宿主图片回调 | 支持，需通过 controller 注入 `imageFetcher` | 支持，需宿主图片回调 |
| `text` / `tspan` | 有实现，属性有限 | 有实现，属性有限 | 未完成 | 未完成 |
| `filter` / `fe*` | 有限支持 | 有限支持 | 有限支持 | 有限支持 |

说明：

- `<use>` 的真实渲染通过节点展开完成，不依赖各平台的 `DrawUse()`；各平台 `DrawUse()` 目前都是空实现。`<use>` 不支持引用 `<svg>` 节点。
- Harmony 的 `<image>` 渲染链路已经打通，但公开 `SvgView` 组件本身没有暴露 `imageFetcher` 入参；如果要用 `<image>`，应走 `SvgRenderNodeController.setImageFetcher(...)`。
- Harmony 与 Skity 的段落工厂当前返回空段落对象，因此 `<text>` / `<tspan>` 在这两个平台实际上不会绘制。
- `filter` 与 `feGaussianBlur`、`feOffset`、`feColorMatrix`、`feComposite`、`feBlend`、`feFlood` 在解析层都能识别；当前四端只保证线性 `SourceGraphic` 链路中的有限模型，复杂 filter graph、分叉输入、`input2` 组合与完整 W3C 滤镜图仍未完整支持。

### 3.1 平台选型建议

- 如果你的素材主要由基础图形、路径、渐变、遮罩组成，可按统一跨平台方案接入
- 如果你的素材大量依赖 `image`，应优先确认宿主是否已经提供图片加载回调或资源管理器
- 如果你的素材依赖 `text` / `tspan`，应按 Android、iOS 单独验收，不要默认 Harmony 或 Skity 可用
- 如果你的素材依赖 `filter`，应单独验证具体 primitive 组合；不要默认复杂 W3C filter graph 已完整支持

## 4. 当前能力边界

### 4.1 当前可识别的标签

当前版本可识别以下标签：

- `svg`
- `g`
- `defs`
- `rect`
- `circle`
- `ellipse`
- `line`
- `polygon`
- `polyline`
- `path`
- `use`
- `pattern`
- `linearGradient`
- `radialGradient`
- `stop`
- `clipPath`
- `mask`
- `image`
- `text`
- `tspan`
- `filter`
- `feGaussianBlur`
- `feOffset`
- `feColorMatrix`
- `feComposite`
- `feBlend`
- `feFlood`

### 4.2 主干能力

以下能力具备完整的使用链路：

- 基础图形：`rect`、`circle`、`ellipse`、`line`、`polygon`、`polyline`、`path`
- 容器与继承：`svg`、`g`、`defs`
- 复用：`use` 的同文档 `#id` 引用展开
- 渐变：`linearGradient`、`radialGradient`、`stop`
- 图案：`pattern`
- 裁剪与遮罩：`clipPath`、`mask`
- 默认颜色注入：`currentColor`
- 运行时诊断：XML 构建错误、递归 `<use>` 等诊断信息

### 4.3 存在明显限制的能力

以下能力虽然有解析或局部实现，但使用时需要明确限制：

- 根节点 `svg@width` / `svg@height` 没有参与根节点解析，最终显示尺寸由宿主传入的 viewport 决定
- `<use>` 只接受同文档 `#id` 引用，不支持引用 `<svg>`；`x` / `y` / `transform` 会生效，`width` / `height` 不会缩放普通被引用图元
- `<use opacity="...">` 会按整体离屏 layer 合成；当前 layer bounds 仍以正确性优先，后续可继续优化离屏范围
- `<image>` 依赖宿主提供图片对象；没有宿主图片能力时不会显示；显式 `x` / `y` / `width` / `height`、百分比和 `opacity` 已支持，`auto` / intrinsic size 推导暂不支持
- `<text>` / `<tspan>` 只实现了很小一部分属性，且 Harmony / Skity 当前段落实现未完成
- `<filter>` 当前只支持有限线性模型；复杂 DAG、`feComposite`、`feBlend`、`feFlood` 等完整组合能力仍需逐项验证

### 4.4 明确不支持的能力

以下能力当前不是“待验证”或“部分支持”，而是明确不进入正式支持路径：

- `<use>` 引用 `<svg>` 节点：同文档 `#id` 引用只支持普通可渲染图元/容器展开，引用到 `<svg>` 会跳过
- `<image>` 的 `auto` / intrinsic size 推导：`width` / `height` 必须显式给出且解析后大于 0
- 完整 W3C filter graph：不支持任意 DAG、分叉 result 复用、复杂 `input` / `input2` 组合；当前只支持已回归的有限线性模型
- Harmony / Skity 的正式文本渲染：`text` / `tspan` 仍不作为这两个端的可用能力
- 根节点 `svg@width` / `svg@height` 作为布局输入：根 SVG 的实际 viewport 由宿主传入

## 5. 接入说明

### 5.1 Harmony

Harmony 发布包名为 `@lynx/servalsvg`。

在 `oh-package.json5` 中添加依赖：

```json5
{
  "dependencies": {
    "@lynx/servalsvg": "1.0.0"
  }
}
```

Harmony 当前更适合接入基础图形、路径、渐变、`pattern`、裁剪、遮罩等能力。

需要特别注意：

- `<image>` 可以渲染，但图片能力是通过 native `setImageFetcher` 链路接入的
- 当前公开 `SvgView` 组件没有 `imageFetcher` 属性；如果你要使用 `<image>`，应使用 `SvgRenderNodeController.setImageFetcher(...)`
- `<text>` / `<tspan>` 当前不会真正绘制，因为 Harmony 的段落工厂尚未完成
- `filter` 已进入有限渲染路径，但只建议使用已回归的线性 `SourceGraphic` primitive 组合

### 5.2 iOS

iOS 提供 `CocoaPods` 规范文件，组件名为 `ServalSVG`。

```ruby
pod 'ServalSVG'
```

如需固定仓库版本，可结合团队发布方式或版本管理策略接入。

### 5.3 Android

Android 提供了原生库模块与示例工程，核心渲染类为 `SVGRender`。如果你的团队已有制品发布链路，可直接接入对应模块；如果暂无统一发布制品，建议先参考示例工程完成接入。

### 5.4 接入前检查清单

建议在正式接入前，先完成以下检查：

- 确认素材标签与关键属性已经在目标平台验证通过
- 确认目标平台是否具备图片、文本、滤镜等所需能力
- 确认宿主接入层是否已经补齐必要依赖
- 确认是否依赖 `image`、`text`、`filter` 等有限实现能力
- 确认是否需要宿主主题色注入 `currentColor`
- 为核心 SVG 素材建立最小回归样例
- 将渲染错误信息接入日志或灰度观测链路

## 6. 快速开始

### 6.1 Harmony 示例

```ts
import { SvgData, SvgView } from '@lynx/servalsvg'

const data = new SvgData()
data.width = 120
data.height = 120
data.content = `
<svg viewBox="0 0 120 120" xmlns="http://www.w3.org/2000/svg">
  <rect x="10" y="10" width="100" height="100" rx="16" fill="#4F6BFF"/>
</svg>
`

SvgView({
  svgData: data,
  color: '#4F6BFF'
})
```

建议：

- 对简单图标和插画优先直接使用 `SvgView`
- 对需要读取诊断结果的场景使用 `SvgRenderNodeController`
- 对需要 `<image>` 的场景使用 `SvgRenderNodeController.setImageFetcher(...)`
- 对有限实现能力先在目标机型上做真实素材验收

如果你需要读取诊断信息，也可以直接使用 `SvgRenderNodeController`：

```ts
import { SvgData, SvgRenderNodeController } from '@lynx/servalsvg'

const controller = new SvgRenderNodeController()
const data = new SvgData()
data.width = 120
data.height = 120
data.content = svgContent

const result = controller.updateData(data, '#4F6BFF')
if (result.hasError) {
  console.info(result.errorMessage ?? '')
}
```

如果 SVG 内容里包含 `<image>`，Harmony 侧应通过 `SvgRenderNodeController` 绑定图片获取函数：

```ts
import { SvgData, SvgImageFetcher, SvgRenderNodeController } from '@lynx/servalsvg'

const controller = new SvgRenderNodeController()
const fetcher: SvgImageFetcher = async (url: string) => {
  // 返回 PixelMap；这里省略具体下载和解码逻辑
  return pixelMap
}

controller.setImageFetcher(fetcher)
controller.updateData(data)
```

### 6.2 Android 示例

```java
SVGRender render = new SVGRender();
render.setColor("#4F6BFF");
render.setResourceManager(resourceManager); // 使用 <image> 时建议提供

Rect viewport = new Rect(0, 0, 120, 120);
SVGRender.SVGRenderResult result =
    render.renderPictureWithResult(svgContent, viewport);

if (result.hasError) {
  Log.i("ServalSVG", result.errorMessage);
}

SVGDrawable drawable = new SVGDrawable(result.picture);
imageView.setImageDrawable(drawable);
```

### 6.3 iOS 示例

```objective-c
SrSVGView* svgView = [[SrSVGView alloc] initWithFrame:CGRectMake(0, 0, 120, 120)];
svgView.color = @"#4F6BFF";

SrSVGRenderResult* result = [svgView parseContentWithResult:svgContent];
if (result.hasError) {
  NSLog(@"%@", result.errorMessage);
}

[self.view addSubview:svgView];
```

## 7. 使用判断规则

如果你需要判断某个标签或属性是否适合在业务中正式使用，建议同时确认以下几点：

- 标签是否能被组件正确识别
- 关键属性是否会实际影响渲染结果
- 目标平台是否具备对应能力
- 宿主依赖是否已经补齐，例如图片加载、颜色注入、回调绑定等
- 是否已经有真实素材的验证与回归样例

## 8. 重点限制说明

以下限制对业务接入影响较大，建议优先关注：

- 根节点 `svg` 的 `viewBox`、`preserveAspectRatio` 是推荐使用的布局控制方式
- 根节点 `svg` 的 `width`、`height` 当前没有作为根节点布局输入解析，不应作为主要尺寸控制手段
- `<use>` 只支持同文档内部引用，不支持引用 `<svg>`；`width`、`height` 当前不会改变普通被引用节点的渲染结果
- `<image>` 在所有平台都依赖宿主图片能力；Harmony 额外要求通过 controller 注入 `imageFetcher`；`auto` / intrinsic size 暂不支持
- `<text>`、`tspan` 在 Android、iOS 只建议使用 `fill`、`color`、`font-size`、`text-anchor`、基础定位；Harmony、Skity 当前不建议使用
- `filter` 只建议使用已验证的有限线性 primitive 组合，不应依赖完整 W3C filter graph
- `line` 实际上是以描边为主的渲染路径，不建议依赖其填充语义
- `clipPath`、`mask` 虽已支持，但复杂组合场景仍建议使用实际素材做回归验证

## 9. 默认颜色能力

ServalSVG 支持由宿主注入默认颜色，用于 SVG 中的 `currentColor` 解析。

对应接入入口如下：

- Android: `SVGRender.setColor(String color)`
- iOS: `SrSVGView.color`
- Harmony: `SvgView.color`

该颜色只影响 `currentColor` 解析，不会覆盖 SVG 中已经显式声明的 `fill`、`stroke`、渐变或 `none`。

## 10. 错误处理建议

建议业务统一接入渲染诊断，避免在异常 SVG 输入下静默失败。

推荐做法：

- 每次渲染或更新后检查结果对象中的 `hasError`
- 在测试或灰度阶段记录 `errorMessage`
- 对外部输入或动态模板输入增加兜底逻辑
- 针对 `image`、`text`、`filter` 等有限实现能力增加素材级回归验证

### 10.1 推荐错误处理策略

- 对解析失败或更新失败的素材，回退到本地兜底图、占位图或静态 PNG
- 对外部下发素材建立白名单或模板校验，避免直接渲染未治理输入
- 将 `hasError` 与 `errorMessage` 接入测试日志、灰度日志或埋点系统
- 对同一批素材建立快照回归，避免平台升级后出现不可见回归

## 11. 最佳实践

- 优先使用基础图形、路径、渐变、裁剪、遮罩能力构建素材
- 尽量使用 `viewBox` 管理自适应布局
- 在跨平台场景中避免依赖复杂滤镜链
- 在需要高一致性的场景中，优先使用已验证的 SVG 模板集合
- 对 `image` 与文本场景分别做平台验收，不要只在单平台验证

## 12. 常见问题

### 12.1 为什么同一份 SVG 在不同平台表现不完全一致？

因为 `ServalSVG` 的公开能力是按标签、属性、平台分别约束的。即使同一标签被支持，不同平台在文本、图片、滤镜、裁剪边界等细节上仍可能存在差异。业务应以目标平台验收结果为准。

### 12.2 为什么有些浏览器可用的 SVG 到这里不可用？

浏览器 SVG 实现覆盖了更完整的规范语义，而 `ServalSVG` 只覆盖当前版本已经打通的能力范围。只具备局部能力、但没有完整渲染链路的特性，不能视为可用。

### 12.3 如何控制 SVG 的显示尺寸？

推荐通过宿主布局尺寸配合 `viewBox` 与 `preserveAspectRatio` 控制。不要依赖根节点 `svg@width` / `svg@height` 作为唯一布局手段。

### 12.4 如何判断当前素材是否适合接入？

优先检查：

- 是否经过目标平台和真实素材验证
- 是否依赖 `image`、`text`、`filter`
- 是否能建立跨平台回归样例
- 是否接受平台间的有限差异

## 13. 版本与文档说明

本文档描述的是当前版本的实际能力边界。若后续能力发生变化，应同步更新本文档。
