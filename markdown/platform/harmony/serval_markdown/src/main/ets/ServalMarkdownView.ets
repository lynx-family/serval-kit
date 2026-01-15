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

export class ServalMarkdownView extends NodeContent {
  constructor() {
    super();
    serval_markdown.createNativeMarkdownNode(this)
  }

  setContent(content: string) {
    serval_markdown.setMarkdownContent(this, content)
  }

  setStyle(style: ServalMarkdownStyleSheet) {
    serval_markdown.setMarkdownStyle(this, style)
  }

  applyStyleInRange(style:ServalMarkdownStyle, start:number, end: number) {
    serval_markdown.applyStyleInRange(this, style, start, end);
  }

  setConfig(config: ServalMarkdownConfig) {
    serval_markdown.setMarkdownConfig(this, config)
  }

  registerInlineViewLoader(fn: (id: string) => FrameNode | null) {
    serval_markdown.registerInlineViewLoader(this, fn)
  }

  registerReplacementViewLoader(fn: (ud: number) => FrameNode | null) {
    serval_markdown.registerReplacementViewLoader(this, fn)
  }

  bindEvent(name: string, fn: Function) {
    serval_markdown.bindEvent(this, name, fn);
  }

  bindExposure(name: string, fn: Function) {
    serval_markdown.bindExposure(this, name, fn);
  }
}

