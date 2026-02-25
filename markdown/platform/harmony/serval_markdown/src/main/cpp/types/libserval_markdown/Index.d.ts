import { NodeContent } from "@kit.ArkUI";

export const createNativeMarkdownNode: (node: NodeContent) => void;

export const setMarkdownContent: (node: NodeContent, content: string) => void;

export const setMarkdownStyle: (node: NodeContent, style: object) => void;

export const setMarkdownConfig: (node: NodeContent, config: object) => void;

export const registerImageLoader: (node: NodeContent, fn: Function) => void;

export const registerFontLoader: (node: NodeContent, fn: Function) => void;

export const registerInlineViewLoader: (
  node: NodeContent,
  fn: Function
) => void;

export const registerReplacementViewLoader: (
  node: NodeContent,
  fn: Function
) => void;

export const bindEvent: (node: NodeContent, name: string, fn: Function) => void;

export const bindExposure: (
  node: NodeContent,
  name: string,
  fn: Function
) => void;

export const applyStyleInRange: (
  node: NodeContent,
  style: object,
  start: number,
  end: number
) => void;
