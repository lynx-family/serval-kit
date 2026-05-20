import { SvgData, SvgImageFetcher, SvgRenderNodeController } from '@lynx/servalsvg';
import { image } from '@kit.ImageKit';
import { http } from '@kit.NetworkKit';
import { display } from '@kit.ArkUI';

interface SvgRenderNodeImageController {
  dispose(): void;
  setImageFetcher(fetcher?: SvgImageFetcher): void;
}

@Component
export struct ExampleSvgView {
  @Prop @Watch('updateSvgNode') svgData: SvgData = new SvgData();
  @Prop @Watch('updateSvgNode') color?: string;

  private readonly svgNodeController: SvgRenderNodeController = new SvgRenderNodeController();
  private readonly imageFetcher: SvgImageFetcher = async (url: string): Promise<image.PixelMap | undefined> => {
    if (!this.isNetworkUrl(url)) {
      console.error(`unsupported image url: ${url}`);
      return undefined;
    }
    const content = await this.fetchRemoteContent(url);
    if (content === undefined) {
      return undefined;
    }
    return this.decodePixelMap(content, url);
  };

  aboutToAppear(): void {
    if (this.svgData.density === undefined) {
      this.svgData.density = display.getDefaultDisplaySync().scaledDensity;
    }
    this.updateSvgNode();
  }

  aboutToDisappear(): void {
    const controller = this.svgNodeController as SvgRenderNodeImageController;
    controller.dispose();
  }

  private isNetworkUrl(url: string): boolean {
    return url.startsWith('http://') || url.startsWith('https://');
  }

  private async fetchRemoteContent(url: string): Promise<ArrayBuffer | undefined> {
    const request = http.createHttp();
    try {
      const response = await request.request(url, {
        method: http.RequestMethod.GET,
        expectDataType: http.HttpDataType.ARRAY_BUFFER,
        connectTimeout: 10000,
        readTimeout: 10000,
      });
      if (response.responseCode < 200 || response.responseCode >= 300) {
        console.error(`request ${url} failed, responseCode=${response.responseCode}`);
        return undefined;
      }
      return response.result as ArrayBuffer;
    } finally {
      request.destroy();
    }
  }

  private async decodePixelMap(data: ArrayBuffer, url: string): Promise<image.PixelMap | undefined> {
    const imageSource = image.createImageSource(data);
    try {
      return await imageSource.createPixelMap();
    } catch (error) {
      console.error(`decode image ${url} failed: ${JSON.stringify(error)}`);
      return undefined;
    } finally {
      await imageSource.release();
    }
  }

  private updateSvgNode(): void {
    const controller = this.svgNodeController as SvgRenderNodeImageController;
    controller.setImageFetcher(this.imageFetcher);
    this.svgNodeController.updateData(this.svgData, this.color);
  }

  build() {
    NodeContainer(this.svgNodeController)
      .width('100%')
      .height('100%')
  }
}
