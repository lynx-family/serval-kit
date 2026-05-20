import { display, FrameNode, LayoutConstraint, NodeController } from '@kit.ArkUI';
import { image } from '@kit.ImageKit';
import svg from 'libservalsvg.so';
import { SvgData, SvgRenderResult } from './SvgTypes';

export type SvgImageFetcher = (url: string) => Promise<image.PixelMap | undefined>;

@Component
export struct SvgView {
  @Prop @Watch('watchSvgSrc') src: string = '';
  @Prop @Watch('watchSvgData') svgData: SvgData = new SvgData();
  @Prop @Watch('watchColor') color?: string;
  private svgNodeController?: SvgRenderNodeController;

  build() {
    if (this.svgData && this.svgNodeController) {
      NodeContainer(this.svgNodeController).size({ width: this.svgData.width, height: this.svgData.height })
    }
  }

  aboutToAppear(): void {
    this.checkSrc();
    if (this.svgData && !this.svgData.density) {
      this.svgData.density = display.getDefaultDisplaySync().scaledDensity;
    }
    this.watchSvgData();
  }

  aboutToDisappear(): void {
    this.svgNodeController?.dispose();
    this.svgNodeController = undefined;
  }

  watchSvgSrc() {
    this.checkSrc();
    this.watchSvgData();
  }

  watchColor() {
    this.watchSvgData();
  }

  checkSrc() {
    if (this.src && this.src.length > 0) {
      this.svgData.content = this.src;
    }
  }

  watchSvgData() {
    if (!this.svgNodeController) {
      this.svgNodeController = new SvgRenderNodeController();
    }
    this.svgNodeController.updateData(this.svgData, this.color);
  }
}


export class SvgRenderNode extends FrameNode {
  native: svg.SvgDrawable;
  data?: SvgData;

  constructor(uiContext: UIContext, native: svg.SvgDrawable) {
    super(uiContext);
    this.native = native;
  }

  onMeasure(constraint: LayoutConstraint): void {
    if (this.data) {
      this.setMeasuredSize({
        width: Math.round(this.data.width * this.data.density!),
        height: Math.round(this.data.height * this.data.density!)
      });
      this.invalidate();
    }
  }

  onDraw(context: DrawContext) {
    if (this.data) {
      this.native.render(context);
    }
  }

  setImageFetcher(fetcher?: SvgImageFetcher): void {
    this.native.setImageFetcher(fetcher);
  }

  setInvalidateCallback(callback?: () => void): void {
    this.native.setInvalidateCallback(callback);
  }

  retryFailedImages(): void {
    this.native.retryFailedImages();
  }

  disposeNative(): void {
    this.native.dispose();
    this.data = undefined;
  }

  update(data?: SvgData): SvgRenderResult;
  update(data?: SvgData, parentColor?: string): SvgRenderResult;
  update(data?: SvgData, parentColor?: string) {
    if (data) {
      this.data = data;
      const result: svg.SvgRenderResult =
        this.native.update(data.width, data.height, data.x, data.y,
          data.density, data.content, data.antiAlias, parentColor);
      this.invalidate();
      return new SvgRenderResult(result.hasError, result.errorMessage);
    }
    return new SvgRenderResult();
  }
}


export class SvgRenderNodeController extends NodeController {
  private rootNode: SvgRenderNode | null = null;
  private data?: SvgData;
  private color?: string;
  private needUpdateData = false;
  private imageFetcher?: SvgImageFetcher;

  setImageFetcher(fetcher?: SvgImageFetcher): void {
    this.imageFetcher = fetcher;
    this.rootNode?.setImageFetcher(fetcher);
  }

  invalidate(): void {
    this.rootNode?.retryFailedImages();
    this.rootNode?.invalidate();
  }

  dispose(): void {
    this.rootNode?.disposeNative();
    this.rootNode = null;
    this.data = undefined;
    this.color = undefined;
    this.imageFetcher = undefined;
    this.needUpdateData = false;
  }

  updateData(data?: SvgData): SvgRenderResult;
  updateData(data?: SvgData, color?: string): SvgRenderResult;
  updateData(data?: SvgData, color?: string) {
    this.data = data;
    this.color = color;
    if (this.rootNode) {
      this.needUpdateData = false;
      return this.rootNode.update(data, color);
    } else {
      this.needUpdateData = true;
    }
    return new SvgRenderResult();
  }

  makeNode(uiContext: UIContext): FrameNode | null {
    if (this.rootNode === null) {
      this.rootNode = new SvgRenderNode(uiContext, new svg.SvgDrawable());
      this.rootNode.setImageFetcher(this.imageFetcher);
      this.rootNode.setInvalidateCallback(() => {
        this.rootNode?.invalidate();
      });
      if (this.needUpdateData) {
        this.needUpdateData = false;
        this.rootNode.update(this.data, this.color);
      }
      return this.rootNode;
    }
    return this.rootNode
  }
}
