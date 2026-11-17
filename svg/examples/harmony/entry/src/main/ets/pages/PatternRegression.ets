// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { common } from '@kit.AbilityKit';
import { image } from '@kit.ImageKit';
import { SvgRenderNodeController } from '@lynx/servalsvg';
import { runPatternPixelRegression } from '../testing/PatternPixelRegression';

@Entry
@Component
struct PatternRegression {
  @State status: string = 'Waiting to run Chrome pixel regression';
  private started: boolean = false;

  @Builder
  private fixture(controller: SvgRenderNodeController, width: number, height: number) {
    NodeContainer(controller)
      .width(`${width}px`)
      .height(`${height}px`)
      .backgroundColor(Color.Transparent)
  }

  private async snapshot(controller: SvgRenderNodeController, width: number,
    height: number): Promise<image.PixelMap> {
    return this.getUIContext().getComponentSnapshot().createFromBuilder(() => {
      this.fixture(controller, width, height);
    }, 0, false, { scale: 1, waitUntilRenderFinished: true });
  }

  private async start(): Promise<void> {
    if (this.started) {
      return;
    }
    this.started = true;
    this.status = 'Running Chrome pixel regression';
    try {
      const context = this.getUIContext().getHostContext() as common.UIAbilityContext;
      const report = await runPatternPixelRegression(context.resourceManager, context.filesDir,
        this.getUIContext(), (controller: SvgRenderNodeController, width: number, height: number) => {
          return this.snapshot(controller, width, height);
        });
      this.status = `${report.passed ? 'PASS' : 'FAIL'}: ${report.caseCount} cases, ${report.probeCount} probes, ` +
        `${report.failedProbeCount} failures`;
    } catch (error) {
      this.status = `ERROR: ${(error as Error).message}`;
      console.error(`SVGPATTERN_FATAL ${this.status}`);
    }
  }

  build() {
    Column() {
      Text(this.status).fontSize(18).padding(24)
    }
    .width('100%')
    .height('100%')
    .onAppear(() => {
      this.start();
    })
  }
}
