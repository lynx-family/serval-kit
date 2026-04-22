# Color Parsing

This document describes the cross-platform color string parsing behavior used by
`serval-svg`.

## Supported Input

The shared parser accepts these color forms:

- `#rgb`
- `#rgba`
- `#rrggbb`
- `#rrggbbaa`
- `rgb(...)`
- `rgba(...)`
- named colors, see [Supported Named Colors](#supported-named-colors)
- `color(display-p3 ...)`

## Supported Named Colors

The shared parser currently supports these 147 named color keywords:

`red`, `green`, `blue`, `yellow`, `cyan`, `magenta`, `black`, `grey`,
`gray`, `white`, `aliceblue`, `antiquewhite`, `aqua`, `aquamarine`,
`azure`, `beige`, `bisque`, `blanchedalmond`, `blueviolet`, `brown`,
`burlywood`, `cadetblue`, `chartreuse`, `chocolate`, `coral`,
`cornflowerblue`, `cornsilk`, `crimson`, `darkblue`, `darkcyan`,
`darkgoldenrod`, `darkgray`, `darkgreen`, `darkgrey`, `darkkhaki`,
`darkmagenta`, `darkolivegreen`, `darkorange`, `darkorchid`, `darkred`,
`darksalmon`, `darkseagreen`, `darkslateblue`, `darkslategray`,
`darkslategrey`, `darkturquoise`, `darkviolet`, `deeppink`,
`deepskyblue`, `dimgray`, `dimgrey`, `dodgerblue`, `firebrick`,
`floralwhite`, `forestgreen`, `fuchsia`, `gainsboro`, `ghostwhite`,
`gold`, `goldenrod`, `greenyellow`, `honeydew`, `hotpink`, `indianred`,
`indigo`, `ivory`, `khaki`, `lavender`, `lavenderblush`, `lawngreen`,
`lemonchiffon`, `lightblue`, `lightcoral`, `lightcyan`,
`lightgoldenrodyellow`, `lightgray`, `lightgreen`, `lightgrey`,
`lightpink`, `lightsalmon`, `lightseagreen`, `lightskyblue`,
`lightslategray`, `lightslategrey`, `lightsteelblue`, `lightyellow`,
`lime`, `limegreen`, `linen`, `maroon`, `mediumaquamarine`,
`mediumblue`, `mediumorchid`, `mediumpurple`, `mediumseagreen`,
`mediumslateblue`, `mediumspringgreen`, `mediumturquoise`,
`mediumvioletred`, `midnightblue`, `mintcream`, `mistyrose`, `moccasin`,
`navajowhite`, `navy`, `oldlace`, `olive`, `olivedrab`, `orange`,
`orangered`, `orchid`, `palegoldenrod`, `palegreen`, `paleturquoise`,
`palevioletred`, `papayawhip`, `peachpuff`, `peru`, `pink`, `plum`,
`powderblue`, `purple`, `rosybrown`, `royalblue`, `saddlebrown`,
`salmon`, `sandybrown`, `seagreen`, `seashell`, `sienna`, `silver`,
`skyblue`, `slateblue`, `slategray`, `slategrey`, `snow`,
`springgreen`, `steelblue`, `tan`, `teal`, `thistle`, `tomato`,
`turquoise`, `violet`, `wheat`, `whitesmoke`, `yellowgreen`

The strict parser entry is:

```c
bool parse_svg_color(const char* str, uint32_t* out_color);
```

Behavior:

- returns `true` and writes `out_color` when parsing succeeds
- returns `false` when the input string is invalid

## Platform Behavior

Android, iOS, and Harmony align on the same string-based default color
injection behavior.

- Android: `SVGRender.setColor(String color)`
- iOS: `SrSVGView.color`
- Harmony: `SvgView.color?: string`

When the injected color string is invalid:

- the platform layer does not apply a fallback host color
- the render path resets the default color override
- SVG content continues rendering without the injected default color

This behavior differs from internal SVG content parsing, where invalid inline
SVG color values still fall back to a shared gray default during SVG node model
construction.

## Demo Samples

The examples app exposes a dedicated `ColorParsing` group on Android, iOS, and
Harmony.

Files in that group:

- `color-parsing-hex-short.svg`
- `color-parsing-hex-short-alpha.svg`
- `color-parsing-hex-long.svg`
- `color-parsing-hex-long-alpha.svg`
- `color-parsing-rgb.svg`
- `color-parsing-rgba.svg`
- `color-parsing-named.svg`
- `color-parsing-display-p3.svg`
- `color-parsing-invalid-fallback.svg`

What they cover:

- one SVG file per supported color form
- supported `fill` and `stroke` parsing for each form
- invalid values falling back to gray inside SVG content parsing

## Minimal Usage

### Android

```java
SVGRender render = new SVGRender();
render.setColor("#4F6BFF");
```

### iOS

```objective-c
SrSVGView* view = [[SrSVGView alloc] initWithString:svgContent];
view.color = @"#4F6BFF";
```

### Harmony

```ts
SvgView({
  svgData: data,
  color: '#4F6BFF'
})
```
