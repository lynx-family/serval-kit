import { NodeContent } from "@kit.ArkUI";

export const createNativeMarkdownNode: (node: NodeContent) => void;

export const createNativeMarkdownMeasurer: (measurer: object) => void;

export const setNativeMarkdownMeasurer: (
  node: NodeContent,
  measurer: object
) => boolean;

export const setMarkdownContent: (measurer: object, content: string) => void;

export const setMarkdownStyle: (measurer: object, style: object) => void;

export const markDirty: (measurer: object) => void;

export const setMarkdownConfig: (measurer: object, config: object) => void;

export const measureMarkdown: (measurer: object, spec: object) => object;

export const registerImageLoader: (measurer: object, fn: Function) => void;

export const registerFontLoader: (measurer: object, fn: Function) => void;

export const registerInlineViewLoader: (measurer: object, fn: Function) => void;

export const registerReplacementViewLoader: (
  measurer: object,
  fn: Function
) => void;

export const bindEvent: (measurer: object, name: string, fn: Function) => void;

export const bindExposure: (
  measurer: object,
  name: string,
  fn: Function
) => void;

export const setRequestMeasureCallback: (
  measurer: object,
  fn?: () => void
) => void;

export const applyStyleInRange: (
  measurer: object,
  style: object,
  start: number,
  end: number
) => void;
