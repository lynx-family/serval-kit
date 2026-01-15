import { DrawContext } from "@kit.ArkUI";


export class SvgDrawable {
  constructor();

  render(canvas: DrawContext): void;

  update(width: number,
    height: number,
    x: number,
    y: number,
    density: number,
    content: string,antiAlias?:boolean): void;
}