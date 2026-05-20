import { DrawContext } from "@kit.ArkUI";
import { image } from "@kit.ImageKit";

export interface SvgRenderResult {
  hasError: boolean;
  errorMessage?: string;
}

export class SvgDrawable {
  constructor();

  render(canvas: DrawContext): void;
  setImageLoader(loader?: (url: string) => image.PixelMap | undefined): void;

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
