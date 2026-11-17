#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
"""Render shared pattern percentage cases in Chrome and verify RGBA probes.

Run from any directory. PNG references go to build/pattern-percent-chrome.
--update refreshes the manifest from Chrome; --sync-android copies the shared
manifest and SVGs into Android's packaged test assets. No browser profile or
long-running Chrome process is retained. This tool needs only Python's stdlib.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import signal
import struct
import subprocess
import tempfile
import time
import zlib

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "svg/testing/pattern_percent_cases.json"


def terminate(signum, _frame):
    # Let active browser/profile context managers clean up on CI cancellation.
    raise SystemExit(128 + signum)


def validate_manifest(manifest, updating=False):
    """Reject malformed or empty regressions before starting a browser."""
    if (not isinstance(manifest, dict)
            or type(manifest.get("schema_version")) is not int
            or manifest["schema_version"] != 1):
        raise ValueError("schema_version must be 1")
    cases = manifest.get("cases")
    if not isinstance(cases, list) or not cases:
        raise ValueError("cases must be a non-empty array")
    files = set()
    for index, case in enumerate(cases):
        label = f"cases[{index}]"
        if not isinstance(case, dict):
            raise ValueError(f"{label} must be an object")
        filename = case.get("file")
        if not isinstance(filename, str) or not filename:
            raise ValueError(f"{label}.file must name an SVG fixture")
        path = Path(filename)
        if path.is_absolute() or ".." in path.parts or path.suffix != ".svg":
            raise ValueError(f"{label}.file must stay inside svg/test_cases")
        if filename in files:
            raise ValueError(f"{label}.file duplicates {filename}")
        files.add(filename)
        if not (ROOT / "svg/test_cases" / path).is_file():
            raise ValueError(f"{label}.file does not exist: {filename}")
        for dimension in ("width", "height"):
            if type(case.get(dimension)) is not int or case[dimension] <= 0:
                raise ValueError(f"{label}.{dimension} must be a positive integer")
        samples = case.get("samples")
        if not isinstance(samples, list) or not samples:
            raise ValueError(f"{label}.samples must be a non-empty array")
        for sample_index, sample in enumerate(samples):
            sample_label = f"{label}.samples[{sample_index}]"
            if not isinstance(sample, dict):
                raise ValueError(f"{sample_label} must be an object")
            for coordinate, dimension in (("x", "width"), ("y", "height")):
                value = sample.get(coordinate)
                if type(value) is not int or not 0 <= value < case[dimension]:
                    raise ValueError(f"{sample_label}.{coordinate} is outside the SVG")
            if "rgba" not in sample:
                raise ValueError(f"{sample_label}.rgba is required")
            rgba = sample["rgba"]
            if updating and rgba is None:
                continue
            if (not isinstance(rgba, list) or len(rgba) != 4
                    or any(type(c) is not int or not 0 <= c <= 255 for c in rgba)):
                raise ValueError(f"{sample_label}.rgba must contain four bytes")


def chrome_command(binary, arguments, timeout, screenshot=None):
    if screenshot is not None:
        screenshot.unlink(missing_ok=True)
    process = subprocess.Popen(
        [binary, *arguments], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        start_new_session=True,
    )
    try:
        if screenshot is not None:
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                if screenshot.exists() and screenshot.read_bytes().endswith(
                        b"IEND\xaeB`\x82"):
                    # Some desktop Chrome builds stay alive after writing the
                    # completed screenshot. The file is our completion signal.
                    return ""
                if process.poll() is not None:
                    _, stderr = process.communicate()
                    raise RuntimeError(stderr.decode(errors="replace"))
                time.sleep(0.05)
            raise TimeoutError(f"Chrome did not finish {screenshot} in {timeout}s")
        stdout, stderr = process.communicate(timeout=timeout)
        if process.returncode:
            raise RuntimeError(stderr.decode(errors="replace"))
        return stdout.decode(errors="replace").strip()
    finally:
        # Chrome can leave helpers behind after its main process exits. They
        # inherit this dedicated process group; never touch an existing browser.
        try:
            os.killpg(process.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            pass
        # The main browser may exit before one of its helpers. Finish the
        # dedicated group even when only a helper ignored SIGTERM.
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.wait()
        process.stdout.close()
        process.stderr.close()


def read_png(path):
    """Decode Chrome's non-interlaced 8-bit RGB/RGBA PNG without dependencies."""
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"Not a PNG: {path}")
    offset, compressed = 8, bytearray()
    while offset < len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4:offset + 8]
        chunk = data[offset + 8:offset + 8 + length]
        if kind == b"IHDR":
            width, height, depth, color, _, _, interlace = struct.unpack(
                ">IIBBBBB", chunk
            )
            if depth != 8 or color not in (2, 6) or interlace:
                raise ValueError("Expected non-interlaced 8-bit RGB/RGBA PNG")
        elif kind == b"IDAT":
            compressed.extend(chunk)
        offset += length + 12
    channels = 4 if color == 6 else 3
    stride = width * channels
    raw = zlib.decompress(compressed)
    rows, previous, offset = [], bytearray(stride), 0
    for _ in range(height):
        method = raw[offset]
        row = bytearray(raw[offset + 1:offset + 1 + stride])
        offset += stride + 1
        for i in range(stride):
            left = row[i - channels] if i >= channels else 0
            above = previous[i]
            upper_left = previous[i - channels] if i >= channels else 0
            if method == 1:
                predictor = left
            elif method == 2:
                predictor = above
            elif method == 3:
                predictor = (left + above) // 2
            elif method == 4:
                p = left + above - upper_left
                distances = (abs(p - left), abs(p - above), abs(p - upper_left))
                predictor = (left, above, upper_left)[distances.index(min(distances))]
            elif method == 0:
                predictor = 0
            else:
                raise ValueError(f"Unsupported PNG filter {method}")
            row[i] = (row[i] + predictor) & 255
        rows.append(row)
        previous = row

    def pixel(x, y):
        values = list(rows[y][channels * x:channels * (x + 1)])
        return values if channels == 4 else [*values, 255]

    return pixel


def find_chrome(override):
    candidates = [override, os.environ.get("CHROME_BIN")]
    candidates += [shutil.which(name) for name in
                   ("google-chrome", "chromium", "chromium-browser")]
    candidates.append("/Applications/Google Chrome.app/Contents/MacOS/Google Chrome")
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return candidate
    raise RuntimeError("Chrome was not found; pass --chrome or set CHROME_BIN")


def main():
    signal.signal(signal.SIGTERM, terminate)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--chrome")
    parser.add_argument("--update", action="store_true")
    parser.add_argument("--sync-android", action="store_true")
    parser.add_argument("--output-dir", type=Path,
                        default=ROOT / "build/pattern-percent-chrome")
    parser.add_argument("--timeout", type=float, default=30)
    args = parser.parse_args()
    try:
        manifest = json.loads(MANIFEST.read_text())
        validate_manifest(manifest, updating=args.update)
    except (OSError, ValueError) as error:
        parser.error(f"{MANIFEST}: {error}")
    binary = find_chrome(args.chrome)
    version = chrome_command(binary, ["--version"], args.timeout)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    errors = []
    with tempfile.TemporaryDirectory(prefix="serval-pattern-chrome-") as temporary:
        temporary = Path(temporary)
        for case in manifest["cases"]:
            source = ROOT / "svg/test_cases" / case["file"]
            html = temporary / "fixture.html"
            html.write_text(
                "<!doctype html><style>html,body{margin:0;padding:0;"
                "background:transparent;overflow:hidden}svg{display:block}</style>"
                + source.read_text()
            )
            screenshot = (args.output_dir / (source.stem + ".png")).resolve()
            profile = temporary / "profile"
            chrome_command(binary, [
                "--headless", "--no-first-run", "--no-default-browser-check",
                "--disable-background-networking", "--disable-extensions",
                "--disable-component-update", "--disable-sync",
                "--disable-crash-reporter", "--disable-breakpad",
                "--force-device-scale-factor=1", "--hide-scrollbars",
                "--default-background-color=00000000",
                "--run-all-compositor-stages-before-draw",
                "--virtual-time-budget=1000",
                f"--user-data-dir={profile}",
                f"--window-size={max(500, case['width'])},{max(500, case['height'])}",
                f"--screenshot={screenshot}", html.as_uri(),
            ], args.timeout, screenshot=screenshot)
            pixel = read_png(screenshot)
            for sample in case["samples"]:
                actual = pixel(sample["x"], sample["y"])
                if args.update:
                    sample["rgba"] = actual
                elif actual != sample["rgba"]:
                    errors.append(f"{case['file']} ({sample['x']},{sample['y']}): "
                                  f"expected {sample['rgba']}, Chrome {actual}")
            print(f"Rendered {case['file']}: {len(case['samples'])} probes", flush=True)
            shutil.rmtree(profile, ignore_errors=True)
    if args.update:
        manifest["generator"] = {
            "browser": "Chrome Headless", "version": version,
            "command": "python3 svg/tools/check_pattern_percent_chrome.py --update --sync-android",
            "png_directory": "build/pattern-percent-chrome",
            "notes": "SVG occupies the top-left width x height pixels; reference PNG canvas is at least 500 x 500.",
        }
        MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n")
    if args.sync_android:
        assets = ROOT / "svg/examples/android/app/src/main/assets"
        shutil.copyfile(MANIFEST, assets / MANIFEST.name)
        for case in manifest["cases"]:
            shutil.copyfile(ROOT / "svg/test_cases" / case["file"],
                            assets / "svg" / case["file"])
    if errors:
        raise SystemExit("\n".join(errors))
    print(f"{version}: all {sum(len(c['samples']) for c in manifest['cases'])} probes match")


if __name__ == "__main__":
    main()
