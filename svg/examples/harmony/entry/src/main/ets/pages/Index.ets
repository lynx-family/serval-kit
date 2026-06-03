import { SvgData } from '@lynx/servalsvg';
import { ExampleSvgView } from '../components/ExampleSvgView';
import util from '@ohos.util';

class SvgPreviewMetadata {
  fileName: string = '';
  title: string = '';
  tags: string[] = [];
  descriptions: string[] = [];
}

class SvgPreviewItem {
  fileName: string = '';
  metadata?: SvgPreviewMetadata;
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
  private static readonly CARD_META_COLOR: string = '#FF5F6368';
  private static readonly CARD_TITLE_COLOR: string = '#FF202124';
  private static readonly PREVIEW_BORDER_COLOR: string = '#FFD9DCE3';
  private static readonly CATEGORY_OTHERS: string = 'Others';
  private static readonly CATEGORY_COLOR_PARSING: string = 'ColorParsing';
  private static readonly CATEGORY_CURRENT_COLOR: string = 'CurrentColor';
  private static readonly CATEGORY_ILLEGAL_PARSING: string = 'IllegalParsing';
  private static readonly CATEGORY_MASK: string = 'Mask';
  private static readonly CATEGORY_PATTERN: string = 'Pattern';
  private static readonly CATEGORY_SVG_ROOT: string = 'SvgRoot';
  private static readonly CATEGORY_USE: string = 'Use';
  private static readonly CATEGORY_GRADIENT: string = 'Gradient';
  private static readonly CATEGORY_SHAPE: string = 'Shape';
  private static readonly CATEGORY_VECTOR_EFFECT: string = 'VectorEffect';
  private static readonly HOST_COLOR_COMPARE_FILE: string = 'currentcolor-host-default-compare.svg';
  private static readonly HOST_COLOR_OVERRIDE_FILE: string = 'currentcolor-content-color-override.svg';
  private static readonly HOST_DEFAULT_COLOR: string = '#4F6BFF';
  private static readonly PREVIEW_METADATA_FILES: string[] = [
    'svg_root_metadata.json',
    'svg_shape_metadata.json',
    'svg_use_metadata.json',
    'svg_gradient_metadata.json'
  ];
  private readonly categories: string[] = [
    Index.CATEGORY_SHAPE,
    Index.CATEGORY_COLOR_PARSING,
    Index.CATEGORY_CURRENT_COLOR,
    Index.CATEGORY_ILLEGAL_PARSING,
    Index.CATEGORY_MASK,
    Index.CATEGORY_PATTERN,
    Index.CATEGORY_SVG_ROOT,
    Index.CATEGORY_USE,
    Index.CATEGORY_GRADIENT,
    Index.CATEGORY_VECTOR_EFFECT,
    Index.CATEGORY_OTHERS
  ];
  @State selectedCategory: string = Index.CATEGORY_SHAPE;
  @State previewItems: SvgPreviewItem[] = [];
  private previewMetadataMap: Map<string, SvgPreviewMetadata> = new Map();
  @State files: string[] = [
    'basic_shapes.svg',
    'clip_path.svg',
    'color-parsing-display-p3.svg',
    'color-parsing-hex-long-alpha.svg',
    'color-parsing-hex-long.svg',
    'color-parsing-hex-short-alpha.svg',
    'color-parsing-hex-short.svg',
    'color-parsing-invalid-fallback.svg',
    'color-parsing-named.svg',
    'color-parsing-rgb.svg',
    'color-parsing-rgba.svg',
    'currentcolor-content-color-override.svg',
    'currentcolor-host-default-compare.svg',
    'currentcolor-fill-opacity-use.svg',
    'defs_use.svg',
    'gradients.svg',
    'image.svg',
    'invalid-attr-gt-mismatched-close-tag.svg',
    'invalid-comment-gt-before-close.svg',
    'invalid-extra-close-tag.svg',
    'invalid-mismatched-close-tag.svg',
    'invalid-path-garbage-token.svg',
    'invalid-path-incomplete-command.svg',
    'invalid-path-arc-flag.svg',
    'invalid-style-garbage.svg',
    'invalid-text-mismatched-close-tag.svg',
    'invalid-transform-garbage-token.svg',
    'invalid-transform-incomplete.svg',
    'invalid-color-function.svg',
    'invalid-clipPath-recursive-iri.svg',
    'invalid-iri-empty.svg',
    'invalid-mask-recursive-iri.svg',
    'invalid-polyline-garbage-token.svg',
    'invalid-polyline-odd-points.svg',
    'invalid-pattern-href-cycle.svg',
    'invalid-pattern-use-cycle.svg',
    'invalid-use-deep-chain.svg',
    'invalid-use-long-cycle.svg',
    'invalid-use-mutual-reference.svg',
    'invalid-use-self-reference.svg',
    'invalid-use-self-reference-in-clipPath.svg',
    'invalid-unclosed-root-tag.svg',
    'invalid-viewBox-garbage-token.svg',
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
    'pattern-viewBox-par.svg',
    'stroke-gradient-vs-pattern.svg',
    'svg-preserve-aspect-ratio-meet-tall-max.svg',
    'svg-preserve-aspect-ratio-meet-tall-mid.svg',
    'svg-preserve-aspect-ratio-meet-tall-min.svg',
    'svg-preserve-aspect-ratio-meet-wide-max.svg',
    'svg-preserve-aspect-ratio-meet-wide-mid.svg',
    'svg-preserve-aspect-ratio-meet-x.svg',
    'svg-preserve-aspect-ratio-none.svg',
    'svg-root-explicit-width-height.svg',
    'svg-root-no-width-height.svg',
    'text.svg',
    'transforms.svg',
    'vector-effect-dash.svg',
    'vector-effect-gradient-line.svg',
    'vector-effect-gradient-stroke.svg',
    'vector-effect-non-scaling-stroke.svg',
    'vector-effect-pattern-stroke.svg',
    'string_test.svg'
  ];

  async aboutToAppear() {
    await this.loadPreviewMetadata();
    this.selectedCategory = Index.CATEGORY_SHAPE;
    await this.reloadCategory(this.selectedCategory);
  }

  private categoryForFile(fileName: string): string {
    if (fileName.startsWith('color-parsing-')) {
      return Index.CATEGORY_COLOR_PARSING;
    }
    if (fileName.startsWith('currentcolor-')) {
      return Index.CATEGORY_CURRENT_COLOR;
    }
    if (fileName.startsWith('invalid-')) {
      return Index.CATEGORY_ILLEGAL_PARSING;
    }
    if (fileName.startsWith('mask-')) {
      return Index.CATEGORY_MASK;
    }
    if (fileName.startsWith('pattern-') || fileName === 'stroke-gradient-vs-pattern.svg') {
      return Index.CATEGORY_PATTERN;
    }
    if (fileName.startsWith('svg-root-') || fileName.startsWith('svg-preserve-aspect-ratio-')) {
      return Index.CATEGORY_SVG_ROOT;
    }
    if (fileName.startsWith('use-')) {
      return Index.CATEGORY_USE;
    }
    if (fileName.startsWith('gradient-')) {
      return Index.CATEGORY_GRADIENT;
    }
    if (fileName.startsWith('shape-')) {
      return Index.CATEGORY_SHAPE;
    }
    if (fileName.startsWith('vector-effect-')) {
      return Index.CATEGORY_VECTOR_EFFECT;
    }
    return Index.CATEGORY_OTHERS;
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

  private async loadRawText(path: string): Promise<string> {
    return new Promise((resolve, reject) => {
      try {
        getContext(this).resourceManager.getRawFileContent(path, (error, value) => {
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

  private async loadPreviewMetadata() {
    this.previewMetadataMap.clear();
    const fileNames = [...this.files];
    for (const metadataFile of Index.PREVIEW_METADATA_FILES) {
      try {
        const content = await this.loadRawText(metadataFile);
        const parsed = JSON.parse(content) as Record<string, Object>;
        const cases = (parsed['cases'] as Array<Record<string, Object>>) || [];
        cases.forEach((item: Record<string, Object>) => {
          const fileName = String(item['fileName'] || '');
          if (fileName.length === 0) {
            return;
          }
          const metadata = new SvgPreviewMetadata();
          metadata.fileName = fileName;
          metadata.title = String(item['title'] || fileName);
          metadata.tags = ((item['tags'] as string[]) || []).map((tag) => String(tag));
          metadata.descriptions = ((item['descriptions'] as string[]) || [])
            .map((description) => String(description));
          this.previewMetadataMap.set(fileName, metadata);
          if (!fileNames.includes(fileName)) {
            fileNames.push(fileName);
          }
        });
      } catch (error) {
        console.error(`load preview metadata ${metadataFile} failed: ${JSON.stringify(error)}`);
      }
    }
    this.files = fileNames;
  }

  private previewMetadataForFile(fileName: string): SvgPreviewMetadata | undefined {
    return this.previewMetadataMap.get(fileName);
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
    return this.loadRawText('svg/' + fileName);
  }

  private async reloadCategory(category: string) {
    this.selectedCategory = category;
    const items: SvgPreviewItem[] = [];
    for (const fileName of this.filesForCategory(category)) {
      const item = new SvgPreviewItem();
      item.fileName = fileName;
      item.metadata = this.previewMetadataForFile(fileName);
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
              Text(item.metadata ? item.metadata.title : item.fileName)
                .fontSize(16)
                .fontColor(Index.CARD_TITLE_COLOR)
                .width('100%')
                .textAlign(TextAlign.Start)

              Text(`case: ${item.fileName.replace('.svg', '')}`)
                .fontSize(12)
                .fontColor(Index.CARD_META_COLOR)
                .width('100%')
                .textAlign(TextAlign.Start)

              if (item.metadata && item.metadata.tags.length > 0) {
                Text(`tags: ${item.metadata.tags.join(', ')}`)
                  .fontSize(12)
                  .fontColor(Index.CARD_META_COLOR)
                  .width('100%')
                  .textAlign(TextAlign.Start)
              }

              if (item.metadata && item.metadata.descriptions.length > 0) {
                Column({ space: 4 }) {
                  ForEach(item.metadata.descriptions, (description: string, index: number) => {
                    Text(`${index + 1}. ${description}`)
                      .fontSize(12)
                      .fontColor(Index.CARD_TITLE_COLOR)
                      .width('100%')
                      .textAlign(TextAlign.Start)
                  })
                }
                .width('100%')
              }

              Stack() {
                if (item.svgData.content.length > 0) {
                  ExampleSvgView({
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
