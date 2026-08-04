import serval_markdown from "libserval_markdown.so"

import { NodeContent } from '@kit.ArkUI'

export interface ServalMarkdownStyle {
  font?: string
  fontSize?: number
  fontWeight?: string
  fontStyle?: string
  lineHeight?: number
  color?: string
  width?: number
  maxWidth ?: number
  height?: number
  marginLeft?: number
  marginRight?: number
  marginTop?: number
  marginBottom?: number
  paddingLeft?: number
  paddingRight?: number
  paddingTop?: number
  paddingBottom?: number
  borderWidth?: number
  borderColor?: string
  scrollX ?: boolean
}

export interface ServalMarkdownStyleSheet {
  normalText?: ServalMarkdownStyle
  link?: ServalMarkdownStyle
  h1?: ServalMarkdownStyle
  codeBlock?: ServalMarkdownStyle
  quote?: ServalMarkdownStyle
  unorderedList?: ServalMarkdownStyle
  unorderedListMarker?: ServalMarkdownStyle
  orderedList?: ServalMarkdownStyle
  orderedListNumber?: ServalMarkdownStyle
}

export interface ServalMarkdownConfig {
  animationType?: string
  animationVelocity?: number
  typewriterDynamicHeight?: boolean
  enableSelection?: boolean
  selectionHighlightColor?: string
  selectionHandleColor?: string
  selectionHandleSize?: number
  selectionHandleTouchMargin?: number
  sourceType?: string
  parser?: string
}

export enum MarkdownMeasureMode {
  Indefinite = 0,
  Definite = 1,
  AtMost = 2,
}

export interface MarkdownMeasureSpec {
  width: number
  widthMode: MarkdownMeasureMode
  height: number
  heightMode: MarkdownMeasureMode
}

export interface MarkdownMeasureResult {
  width: number
  height: number
  baseline: number
}

export class MarkdownMeasurer {
  constructor() {
    serval_markdown.createNativeMarkdownMeasurer(this)
  }

  setContent(content: string) {
    serval_markdown.setMarkdownContent(this, content)
  }

  setStyle(style: ServalMarkdownStyleSheet) {
    serval_markdown.setMarkdownStyle(this, style)
  }

  markDirty() {
    serval_markdown.markDirty(this)
  }

  applyStyleInRange(style: ServalMarkdownStyle, start: number, end: number) {
    serval_markdown.applyStyleInRange(this, style, start, end)
  }

  setConfig(config: ServalMarkdownConfig) {
    serval_markdown.setMarkdownConfig(this, config)
  }

  measure(spec: MarkdownMeasureSpec): MarkdownMeasureResult {
    return serval_markdown.measureMarkdown(this, spec) as MarkdownMeasureResult
  }

  registerInlineViewLoader(fn: (id: string) => FrameNode | null) {
    serval_markdown.registerInlineViewLoader(this, fn)
  }

  registerImageLoader(fn: Function) {
    serval_markdown.registerImageLoader(this, fn)
  }

  registerFontLoader(fn: Function) {
    serval_markdown.registerFontLoader(this, fn)
  }

  registerReplacementViewLoader(fn: (ud: number) => FrameNode | null) {
    serval_markdown.registerReplacementViewLoader(this, fn)
  }

  bindEvent(name: string, fn: Function) {
    serval_markdown.bindEvent(this, name, fn)
  }

  bindExposure(name: string, fn: Function) {
    serval_markdown.bindExposure(this, name, fn)
  }

  setRequestMeasureCallback(fn?: () => void) {
    serval_markdown.setRequestMeasureCallback(this, fn)
  }
}

export class ServalMarkdownView extends NodeContent {
  private markdownMeasurer?: MarkdownMeasurer

  constructor(createMeasurer: boolean = true) {
    super();
    serval_markdown.createNativeMarkdownNode(this)
    if (createMeasurer) {
      this.setMarkdownMeasurer(new MarkdownMeasurer())
    }
  }

  setMarkdownMeasurer(measurer: MarkdownMeasurer): boolean {
    if (this.markdownMeasurer !== undefined || measurer === undefined ||
      !serval_markdown.setNativeMarkdownMeasurer(this, measurer)) {
      return false
    }
    this.markdownMeasurer = measurer
    return true
  }

  getMarkdownMeasurer(): MarkdownMeasurer | undefined {
    return this.markdownMeasurer
  }

  setContent(content: string) {
    this.markdownMeasurer?.setContent(content)
  }

  setStyle(style: ServalMarkdownStyleSheet) {
    this.markdownMeasurer?.setStyle(style)
  }

  markDirty() {
    this.markdownMeasurer?.markDirty()
  }

  applyStyleInRange(style:ServalMarkdownStyle, start:number, end: number) {
    this.markdownMeasurer?.applyStyleInRange(style, start, end)
  }

  setConfig(config: ServalMarkdownConfig) {
    this.markdownMeasurer?.setConfig(config)
  }

  registerInlineViewLoader(fn: (id: string) => FrameNode | null) {
    this.markdownMeasurer?.registerInlineViewLoader(fn)
  }

  registerImageLoader(fn: Function) {
    this.markdownMeasurer?.registerImageLoader(fn)
  }

  registerFontLoader(fn: Function) {
    this.markdownMeasurer?.registerFontLoader(fn)
  }

  registerReplacementViewLoader(fn: (ud: number) => FrameNode | null) {
    this.markdownMeasurer?.registerReplacementViewLoader(fn)
  }

  bindEvent(name: string, fn: Function) {
    this.markdownMeasurer?.bindEvent(name, fn)
  }

  bindExposure(name: string, fn: Function) {
    this.markdownMeasurer?.bindExposure(name, fn)
  }
}
