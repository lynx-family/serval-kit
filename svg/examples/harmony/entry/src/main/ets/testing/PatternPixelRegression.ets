// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { SvgData, SvgRenderNodeController } from '@lynx/servalsvg';
import { UIContext } from '@kit.ArkUI';
import { image } from '@kit.ImageKit';
import { util } from '@kit.ArkTS';
import resourceManager from '@ohos.resourceManager';
import fs from '@ohos.file.fs';

interface ReferenceProbe {
  x: number;
  y: number;
  rgba: number[];
}

interface ReferenceCase {
  file: string;
  width: number;
  height: number;
  samples: ReferenceProbe[];
}

interface ReferenceGenerator {
  browser: string;
  version: string;
  command: string;
}

interface ReferenceManifest {
  schema_version: number;
  generator: ReferenceGenerator;
  cases: ReferenceCase[];
}

interface ProbeResult {
  x: number;
  y: number;
  expected: number[];
  actual: number[];
  passed: boolean;
}

interface CaseResult {
  file: string;
  passed: boolean;
  probes: ProbeResult[];
  error?: string;
}

export interface PatternRegressionReport {
  schema_version: number;
  generator: ReferenceGenerator;
  passed: boolean;
  caseCount: number;
  probeCount: number;
  failedCaseCount: number;
  failedProbeCount: number;
  cases: CaseResult[];
}

export type PatternSnapshot = (controller: SvgRenderNodeController, width: number,
  height: number) => Promise<image.PixelMap>;

function readText(resources: resourceManager.ResourceManager, path: string): string {
  const decoder = new util.TextDecoder('utf-8', { ignoreBOM: true });
  return decoder.decodeWithStream(resources.getRawFileContentSync(path), { stream: false });
}

function validateReference(testCase: ReferenceCase): void {
  if (!Number.isInteger(testCase.width) || !Number.isInteger(testCase.height) ||
    testCase.width <= 0 || testCase.height <= 0 || testCase.samples.length === 0) {
    throw new Error('Regression fixture has invalid dimensions or no probes.');
  }
  for (const sample of testCase.samples) {
    if (!Number.isInteger(sample.x) || !Number.isInteger(sample.y) || sample.x < 0 || sample.y < 0 ||
      sample.x >= testCase.width || sample.y >= testCase.height || sample.rgba.length !== 4) {
      throw new Error(`Invalid reference probe (${sample.x},${sample.y}).`);
    }
    for (const channel of sample.rgba) {
      if (!Number.isFinite(channel) || !Number.isInteger(channel) || channel < 0 || channel > 255) {
        throw new Error(`Invalid reference RGBA at (${sample.x},${sample.y}).`);
      }
    }
  }
}

async function decodeImages(content: string): Promise<Map<string, image.PixelMap>> {
  const images = new Map<string, image.PixelMap>();
  const hrefPattern = /\b(?:xlink:)?href\s*=\s*["'](data:image\/[^"']+)["']/g;
  let match = hrefPattern.exec(content);
  while (match !== null) {
    const href = match[1];
    if (!images.has(href)) {
      const separator = href.indexOf(';base64,');
      if (separator < 0) {
        throw new Error('Regression fixtures require base64 data images.');
      }
      const bytes = new util.Base64Helper().decodeSync(href.substring(separator + 8));
      const source = image.createImageSource(bytes.buffer);
      try {
        images.set(href, await source.createPixelMap({ desiredPixelFormat: image.PixelMapFormat.RGBA_8888 }));
      } finally {
        await source.release();
      }
    }
    match = hrefPattern.exec(content);
  }
  return images;
}

function sampleRGBA(bytes: Uint8Array, offset: number, format: image.PixelMapFormat,
  alphaType: image.AlphaType): number[] {
  if (!Number.isInteger(offset) || offset < 0 || offset + 3 >= bytes.length) {
    throw new Error('Regression probe lies outside the PixelMap buffer.');
  }
  let rgba: number[];
  if (format === image.PixelMapFormat.BGRA_8888) {
    rgba = [bytes[offset + 2], bytes[offset + 1], bytes[offset], bytes[offset + 3]];
  } else if (format === image.PixelMapFormat.ARGB_8888) {
    rgba = [bytes[offset + 1], bytes[offset + 2], bytes[offset + 3], bytes[offset]];
  } else if (format === image.PixelMapFormat.RGBA_8888) {
    rgba = [bytes[offset], bytes[offset + 1], bytes[offset + 2], bytes[offset + 3]];
  } else {
    throw new Error(`Unsupported readback pixel format: ${format}.`);
  }
  if (alphaType === image.AlphaType.PREMUL && rgba[3] > 0 && rgba[3] < 255) {
    for (let channel = 0; channel < 3; channel++) {
      rgba[channel] = Math.round(rgba[channel] * 255 / rgba[3]);
    }
  }
  for (const channel of rgba) {
    if (!Number.isFinite(channel)) {
      throw new Error('Regression readback produced a non-finite channel.');
    }
  }
  return rgba;
}

async function runCase(testCase: ReferenceCase, resources: resourceManager.ResourceManager,
  uiContext: UIContext, snapshot: PatternSnapshot): Promise<CaseResult> {
  validateReference(testCase);
  const content = readText(resources, `svg/${testCase.file}`);
  // Maps are unique to this drawable because its existing dispose contract
  // releases image-fetcher results. Decode before drawing; no network is used.
  const images = await decodeImages(content);
  const requested = new Set<string>();
  const requests: Promise<image.PixelMap | undefined>[] = [];
  const controller = new SvgRenderNodeController();
  let rendered: image.PixelMap | undefined;
  try {
    controller.setImageFetcher((url: string): Promise<image.PixelMap | undefined> => {
      requested.add(url);
      const ready = Promise.resolve(images.get(url));
      requests.push(ready);
      return ready;
    });
    controller.makeNode(uiContext);
    const data = new SvgData(content);
    data.width = testCase.width;
    data.height = testCase.height;
    data.x = 0;
    data.y = 0;
    data.density = 1;
    data.antiAlias = true;
    const parseResult = controller.updateData(data);
    if (parseResult.hasError) {
      throw new Error(parseResult.errorMessage ?? 'SVG parse failed.');
    }

    rendered = await snapshot(controller, testCase.width, testCase.height);
    if (requests.length > 0) {
      // The first draw installs the existing native Promise handlers. Await
      // those resolved cache requests, then capture a fresh completed render.
      await Promise.all(requests);
      await rendered.release();
      rendered = undefined;
      controller.invalidate();
      rendered = await snapshot(controller, testCase.width, testCase.height);
    }
    let info = await rendered.getImageInfo();
    if (info.size.width !== testCase.width || info.size.height !== testCase.height) {
      throw new Error(`Snapshot size ${info.size.width}x${info.size.height}, expected ` +
        `${testCase.width}x${testCase.height}.`);
    }
    if (info.pixelFormat !== image.PixelMapFormat.RGBA_8888 &&
      info.pixelFormat !== image.PixelMapFormat.BGRA_8888 && info.pixelFormat !== image.PixelMapFormat.ARGB_8888) {
      await rendered.convertPixelFormat(image.PixelMapFormat.RGBA_8888);
      info = await rendered.getImageInfo();
    }
    const buffer = new ArrayBuffer(rendered.getPixelBytesNumber());
    await rendered.readPixelsToBuffer(buffer);
    const bytes = new Uint8Array(buffer);
    const stride = rendered.getBytesNumberPerRow();
    const probes: ProbeResult[] = [];
    let passed = true;
    for (const sample of testCase.samples) {
      const actual = sampleRGBA(bytes, sample.y * stride + sample.x * 4, info.pixelFormat, info.alphaType);
      let matches = true;
      for (let channel = 0; channel < 4; channel++) {
        if (!Number.isFinite(actual[channel]) || Math.abs(actual[channel] - sample.rgba[channel]) > 3) {
          matches = false;
        }
      }
      probes.push({ x: sample.x, y: sample.y, expected: sample.rgba, actual: actual, passed: matches });
      if (!matches) {
        passed = false;
        console.error(`SVGPATTERN_PROBE_FAIL file=${testCase.file} x=${sample.x} y=${sample.y} ` +
          `expected=${JSON.stringify(sample.rgba)} actual=${JSON.stringify(actual)}`);
      }
    }
    return { file: testCase.file, passed: passed, probes: probes };
  } finally {
    try {
      if (rendered !== undefined) {
        await rendered.release();
      }
    } finally {
      try {
        controller.dispose();
      } finally {
        const releases: Promise<void>[] = [];
        const uris = Array.from(images.keys());
        for (const href of uris) {
          const pixelMap = images.get(href);
          if (!requested.has(href) && pixelMap !== undefined) {
            releases.push(pixelMap.release());
          }
        }
        try {
          await Promise.all(releases);
        } finally {
          images.clear();
        }
      }
    }
  }
}

export async function runPatternPixelRegression(resources: resourceManager.ResourceManager,
  filesDir: string, uiContext: UIContext, snapshot: PatternSnapshot): Promise<PatternRegressionReport> {
  const manifest = JSON.parse(readText(resources, 'pattern_percent_cases.json')) as ReferenceManifest;
  if (manifest.schema_version !== 1 || manifest.cases.length === 0) {
    throw new Error('Unsupported or empty pattern reference manifest.');
  }
  const report: PatternRegressionReport = {
    schema_version: 1, generator: manifest.generator, passed: true, caseCount: manifest.cases.length,
    probeCount: 0, failedCaseCount: 0, failedProbeCount: 0, cases: []
  };
  for (const testCase of manifest.cases) {
    let result: CaseResult;
    try {
      result = await runCase(testCase, resources, uiContext, snapshot);
    } catch (error) {
      const message = (error as Error).message;
      const probes: ProbeResult[] = testCase.samples.map((sample: ReferenceProbe): ProbeResult => {
        return { x: sample.x, y: sample.y, expected: sample.rgba, actual: [], passed: false };
      });
      result = { file: testCase.file, passed: false, error: message, probes: probes };
      console.error(`SVGPATTERN_CASE_ERROR file=${testCase.file} error=${message}`);
    }
    report.cases.push(result);
    report.probeCount += result.probes.length;
    if (!result.passed) {
      report.failedCaseCount++;
    }
    for (const probe of result.probes) {
      if (!probe.passed) {
        report.failedProbeCount++;
      }
    }
    console.info(`SVGPATTERN_CASE file=${testCase.file} passed=${result.passed} probes=${result.probes.length}`);
  }
  report.passed = report.failedCaseCount === 0;
  const path = `${filesDir}/pattern-regression.json`;
  const file = fs.openSync(path, fs.OpenMode.CREATE | fs.OpenMode.READ_WRITE | fs.OpenMode.TRUNC);
  try {
    fs.writeSync(file.fd, JSON.stringify(report, null, 2));
  } finally {
    fs.closeSync(file);
  }
  console.info(`SVGPATTERN_RESULT passed=${report.passed} cases=${report.caseCount} probes=${report.probeCount} ` +
    `failedCases=${report.failedCaseCount} failedProbes=${report.failedProbeCount} report=${path}`);
  return report;
}
