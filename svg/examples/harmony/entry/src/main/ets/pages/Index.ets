import { SvgData, SvgView } from '@lynx/servalsvg';
import util from '@ohos.util';

class SvgPreviewItem {
  fileName: string = '';
  svgData: SvgData = new SvgData();
}

@Entry
@Component
struct Index {
  private static readonly CARD_GAP: number = 12;
  private static readonly CARD_PADDING: number = 12;
  private static readonly CARD_RADIUS: number = 8;
  private static readonly PREVIEW_WIDTH: number = 260;
  private static readonly PREVIEW_HEIGHT: number = 195;
  private static readonly CARD_BACKGROUND: string = '#FFFFFFFF';
  private static readonly CARD_TITLE_COLOR: string = '#FF202124';
  private static readonly PREVIEW_BORDER_COLOR: string = '#FFD9DCE3';
  private static readonly CATEGORY_CORE: string = 'Core';
  private static readonly CATEGORY_CURRENT_COLOR: string = 'CurrentColor';
  private static readonly CATEGORY_MASK: string = 'Mask';
  private static readonly CATEGORY_PATTERN: string = 'Pattern';
  private static readonly CATEGORY_VECTOR_EFFECT: string = 'VectorEffect';
  private static readonly HOST_COLOR_COMPARE_FILE: string = 'currentcolor-host-default-compare.svg';
  private static readonly HOST_COLOR_OVERRIDE_FILE: string = 'currentcolor-content-color-override.svg';
  private static readonly HOST_DEFAULT_COLOR: number = 0xFF4F6BFF;
  private readonly categories: string[] = [
    Index.CATEGORY_CORE,
    Index.CATEGORY_CURRENT_COLOR,
    Index.CATEGORY_MASK,
    Index.CATEGORY_PATTERN,
    Index.CATEGORY_VECTOR_EFFECT
  ];
  @State selectedCategory: string = Index.CATEGORY_CORE;
  @State previewItems: SvgPreviewItem[] = [];
  @State files: string[] = [
    'basic_shapes.svg',
    'clip_path.svg',
    'currentcolor-content-color-override.svg',
    'currentcolor-host-default-compare.svg',
    'currentcolor-fill-opacity-use.svg',
    'defs_use.svg',
    'gradients.svg',
    'image.svg',
    'invalid-iri-test.svg',
    'mask-alpha-units-test.svg',
    'mask-comprehensive-test.svg',
    'mask-luminance-gradient-test.svg',
    'paths.svg',
    'polygon-vs-polyline.svg',
    'pattern-basic.svg',
    'pattern-content-units-obb.svg',
    'pattern-content-units-obb-numeric.svg',
    'pattern-content-units-obb-percent.svg',
    'pattern-href.svg',
    'pattern-stroke-basic.svg',
    'pattern-stroke-dash.svg',
    'pattern-stroke-fill-stroke.svg',
    'pattern-stroke-obb.svg',
    'pattern-stroke-open-path.svg',
    'pattern-stroke-transform.svg',
    'pattern-transform.svg',
    'pattern-units-obb.svg',
    'pattern-viewbox-par.svg',
    'stroke-gradient-vs-pattern.svg',
    'text.svg',
    'transforms.svg',
    'vector-effect-dash.svg',
    'vector-effect-gradient-line.svg',
    'vector-effect-gradient-stroke.svg',
    'vector-effect-non-scaling-stroke.svg',
    'string_test.svg'
  ];

  aboutToAppear() {
    this.selectedCategory = this.categoryForFile('basic_shapes.svg');
    this.reloadCategory(this.selectedCategory);
  }

  private categoryForFile(fileName: string): string {
    if (fileName.startsWith('currentcolor-')) {
      return Index.CATEGORY_CURRENT_COLOR;
    }
    if (fileName.startsWith('mask-')) {
      return Index.CATEGORY_MASK;
    }
    if (fileName.startsWith('pattern-') || fileName === 'stroke-gradient-vs-pattern.svg') {
      return Index.CATEGORY_PATTERN;
    }
    if (fileName.startsWith('vector-effect-')) {
      return Index.CATEGORY_VECTOR_EFFECT;
    }
    return Index.CATEGORY_CORE;
  }

  private filesForCategory(category: string): string[] {
    return this.files.filter((file) => this.categoryForFile(file) === category);
  }

  private shouldUseHostColor(fileName: string): boolean {
    return fileName === Index.HOST_COLOR_COMPARE_FILE ||
      fileName === Index.HOST_COLOR_OVERRIDE_FILE;
  }

  private createSvgData(content: string): SvgData {
    const data = new SvgData(content);
    data.width = Index.PREVIEW_WIDTH;
    data.height = Index.PREVIEW_HEIGHT;
    return data;
  }

  private stringTestContent(): string {
    return '<svg width="200" height="200" viewBox="0 0 200 200" xmlns="http://www.w3.org/2000/svg">\n'
      + '  <defs>\n'
      + '    <pattern id="TrianglePattern" x="0" y="0" width="20" height="20" patternUnits="userSpaceOnUse">\n'
      + '      <path d="M 0 0 L 10 0 L 5 10 Z" fill="red" />\n'
      + '    </pattern>\n'
      + '  </defs>\n'
      + '  <rect x="0" y="0" width="200" height="200" fill="url(#TrianglePattern)" />\n'
      + '</svg>\n';
  }

  private async loadSvgText(fileName: string): Promise<string> {
    if (fileName === 'string_test.svg') {
      return this.stringTestContent();
    }
    return new Promise((resolve, reject) => {
      try {
        getContext(this).resourceManager.getRawFileContent('svg/' + fileName, (error, value) => {
          if (error != null) {
            reject(error);
            return;
          }
          const textDecoder = new util.TextDecoder('utf-8', { ignoreBOM: true });
          resolve(textDecoder.decodeWithStream(value, { stream: false }));
        });
      } catch (error) {
        reject(error);
      }
    });
  }

  private async reloadCategory(category: string) {
    this.selectedCategory = category;
    const items: SvgPreviewItem[] = [];
    for (const fileName of this.filesForCategory(category)) {
      const item = new SvgPreviewItem();
      item.fileName = fileName;
      try {
        const content = await this.loadSvgText(fileName);
        item.svgData = this.createSvgData(content);
      } catch (error) {
        console.error(`load ${fileName} failed: ${JSON.stringify(error)}`);
        item.svgData = this.createSvgData('');
      }
      items.push(item);
    }
    this.previewItems = items;
  }

  build() {
    Column() {
      Select(this.categories.map((category) => {
        return { value: category } as SelectOption;
      }))
        .value(this.selectedCategory)
        .onSelect((index: number, value: string) => {
          this.reloadCategory(value);
        })
        .width('80%')
        .height(50)
        .margin({ top: 20, bottom: 16 })

      Scroll() {
        Column({ space: Index.CARD_GAP }) {
          ForEach(this.previewItems, (item: SvgPreviewItem) => {
            Column({ space: Index.CARD_GAP }) {
              Text(item.fileName)
                .fontSize(14)
                .fontColor(Index.CARD_TITLE_COLOR)
                .width('100%')
                .textAlign(TextAlign.Start)

              Stack() {
                if (item.svgData.content.length > 0) {
                  SvgView({
                    svgData: item.svgData,
                    color: this.shouldUseHostColor(item.fileName)
                      ? Index.HOST_DEFAULT_COLOR
                      : undefined
                  })
                    .width('100%')
                    .height('100%')
                } else {
                  Text('Load failed')
                    .width('100%')
                    .height('100%')
                    .textAlign(TextAlign.Center)
                    .fontColor(Color.Gray)
                }
              }
              .width(Index.PREVIEW_WIDTH)
              .height(Index.PREVIEW_HEIGHT)
              .clip(true)
              .backgroundColor(Index.CARD_BACKGROUND)
              .border({ width: 1, color: Index.PREVIEW_BORDER_COLOR })
              .alignSelf(ItemAlign.Center)
            }
            .width('100%')
            .padding(Index.CARD_PADDING)
            .backgroundColor(Index.CARD_BACKGROUND)
            .borderRadius(Index.CARD_RADIUS)
          }, (item: SvgPreviewItem) => item.fileName)
        }
        .width('100%')
        .padding({ left: 16, right: 16, bottom: 16 })
      }
    }
    .width('100%')
    .height('100%')
    .justifyContent(FlexAlign.Start)
    .alignItems(HorizontalAlign.Center)
  }
}
