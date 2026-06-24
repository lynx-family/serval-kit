import { DrawContext } from "@kit.ArkUI";
import { image } from "@kit.ImageKit";

export interface SvgRenderResult {
  hasError: boolean;
  errorMessage?: string;
}

export class SvgDrawable {
  constructor();

  render(canvas: DrawContext, seconds?: number): void;
  hasAnimations(): boolean;
  animationTimelineEndSeconds(): number;
  startAnimation(): void;
  stopAnimation(): void;
  resetAnimationClock(): void;
  needsAnimationFrame(): boolean;
  onFrameTimeNanos(frameTimeNanos: number): boolean;
  currentAnimationSeconds(): number;
  dispose(): void;
  setImageFetcher(
    fetcher?: (url: string) => Promise<image.PixelMap | undefined>
  ): void;
  setInvalidateCallback(callback?: () => void): void;
  retryFailedImages(): void;

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
