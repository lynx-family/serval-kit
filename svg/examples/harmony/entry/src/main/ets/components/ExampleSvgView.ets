import { SvgData, SvgRenderNodeController } from '@lynx/servalsvg';
import { image } from '@kit.ImageKit';
import { http } from '@kit.NetworkKit';
import { display } from '@kit.ArkUI';

type SvgImageLoader = (url: string) => image.PixelMap | undefined;

interface SvgRenderNodeImageController {
  setImageLoader(loader?: SvgImageLoader): void;
  invalidate(): void;
}

class SvgExampleImageRepository {
  private static readonly imageCache: Map<string, image.PixelMap> = new Map();
  private static readonly pendingUrls: Set<string> = new Set();
  private static readonly listeners: Map<string, Set<() => void>> = new Map();

  static request(url: string, listener: () => void): image.PixelMap | undefined {
    const cachedImage = SvgExampleImageRepository.imageCache.get(url);
    if (cachedImage !== undefined) {
      return cachedImage;
    }
    if (!SvgExampleImageRepository.isNetworkUrl(url)) {
      console.error(`Only http/https image urls are supported in this demo: ${url}`);
      return undefined;
    }
    SvgExampleImageRepository.addListener(url, listener);
    if (SvgExampleImageRepository.pendingUrls.has(url)) {
      return undefined;
    }
    SvgExampleImageRepository.pendingUrls.add(url);
    SvgExampleImageRepository.fetchPixelMap(url)
      .then((pixelMap) => {
        if (pixelMap !== undefined) {
          SvgExampleImageRepository.imageCache.set(url, pixelMap);
          SvgExampleImageRepository.notifyListeners(url);
        } else {
          SvgExampleImageRepository.clearListeners(url);
        }
      })
      .catch((error: Object) => {
        console.error(`load image ${url} failed: ${JSON.stringify(error)}`);
        SvgExampleImageRepository.clearListeners(url);
      })
      .finally(() => {
        SvgExampleImageRepository.pendingUrls.delete(url);
      });
    return undefined;
  }

  private static addListener(url: string, listener: () => void): void {
    let listeners = SvgExampleImageRepository.listeners.get(url);
    if (listeners === undefined) {
      listeners = new Set();
      SvgExampleImageRepository.listeners.set(url, listeners);
    }
    listeners.add(listener);
  }

  private static notifyListeners(url: string): void {
    const listeners = SvgExampleImageRepository.listeners.get(url);
    SvgExampleImageRepository.listeners.delete(url);
    listeners?.forEach((listener) => {
      listener();
    });
  }

  private static clearListeners(url: string): void {
    SvgExampleImageRepository.listeners.delete(url);
  }

  private static async fetchPixelMap(url: string): Promise<image.PixelMap | undefined> {
    const content = await SvgExampleImageRepository.fetchRemoteContent(url);
    if (content === undefined) {
      return undefined;
    }
    return SvgExampleImageRepository.decodePixelMap(content, url);
  }

  private static isNetworkUrl(url: string): boolean {
    return url.startsWith('http://') || url.startsWith('https://');
  }

  private static async fetchRemoteContent(url: string): Promise<ArrayBuffer | undefined> {
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

  private static async decodePixelMap(data: ArrayBuffer, url: string): Promise<image.PixelMap | undefined> {
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
}

@Component
export struct ExampleSvgView {
  @Prop @Watch('updateSvgNode') svgData: SvgData = new SvgData();
  @Prop @Watch('updateSvgNode') color?: string;

  private readonly svgNodeController: SvgRenderNodeController = new SvgRenderNodeController();
  private readonly invalidateCurrentSvg: () => void = () => {
    const controller = this.svgNodeController as SvgRenderNodeImageController;
    controller.invalidate();
  };
  private readonly imageLoader: SvgImageLoader = (url: string): image.PixelMap | undefined => {
    return SvgExampleImageRepository.request(url, this.invalidateCurrentSvg);
  };

  aboutToAppear(): void {
    if (this.svgData.density === undefined) {
      this.svgData.density = display.getDefaultDisplaySync().scaledDensity;
    }
    this.updateSvgNode();
  }

  private updateSvgNode(): void {
    const controller = this.svgNodeController as SvgRenderNodeImageController;
    controller.setImageLoader(this.imageLoader);
    this.svgNodeController.updateData(this.svgData, this.color);
  }

  build() {
    NodeContainer(this.svgNodeController)
      .width('100%')
      .height('100%')
  }
}
