import { DrawContext } from "@kit.ArkUI";

export interface SvgRenderResult {
  hasError: boolean;
  errorMessage?: string;
}

export class SvgDrawable {
  constructor();

  render(canvas: DrawContext): void;

  update(
    width: number,
    height: number,
    x: number,
    y: number,
    density: number,
    content: string,
    antiAlias?: boolean,
    color?: string
  ): SvgRenderResult;
}
