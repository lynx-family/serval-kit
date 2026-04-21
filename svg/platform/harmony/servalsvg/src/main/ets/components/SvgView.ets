import { display, FrameNode, LayoutConstraint, NodeController } from '@kit.ArkUI';
import svg from 'libservalsvg.so';
import { SvgData } from './SvgTypes';


@Component
export struct SvgView {
  @Prop @Watch('watchSvgSrc') src: string = '';
  @Prop @Watch('watchSvgData') svgData: SvgData = new SvgData();
  @Prop @Watch('watchColor') color?: number;
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

  update(data?: SvgData, parentColor?: number) {
    if (data) {
      this.data = data;
      this.native.update(data.width, data.height, data.x, data.y, data.density,
        data.content, data.antiAlias, parentColor);
      this.invalidate();
    }
  }
}


export class SvgRenderNodeController extends NodeController {
  private rootNode: SvgRenderNode | null = null;
  private data?: SvgData;
  private color?: number;
  private needUpdateData = false;

  constructor() {
    super();
  }

  updateData(data?: SvgData, color?: number) {
    this.data = data;
    this.color = color;
    if (this.rootNode) {
      this.needUpdateData = false;
      this.rootNode.update(data, color);
    } else {
      this.needUpdateData = true;
    }
  }

  makeNode(uiContext: UIContext): FrameNode | null {
    if (this.rootNode === null) {
      this.rootNode = new SvgRenderNode(uiContext, new svg.SvgDrawable());
      if (this.needUpdateData) {
        this.needUpdateData = false;
        this.rootNode.update(this.data, this.color);
      }
      return this.rootNode;
    }
    return this.rootNode
  }
}
