#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Build the macOS preview and run its Metal regressions against a loopback PNG."""

import argparse
import base64
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
import platform
import subprocess
import tempfile
import threading
import xml.etree.ElementTree as ET


SVG = Path(__file__).resolve().parents[1]
EXAMPLE = SVG / "examples/osx/SVGMetaRenderer"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=EXAMPLE / "SVGMetaRenderer.xcodeproj")
    parser.add_argument("--derived-data", type=Path)
    parser.add_argument("--frames", type=Path, help="Save Metal readback frames as PNGs")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="skity-metal-") as temp:
        derived = (args.derived_data or Path(temp) / "build").resolve()
        arch = platform.machine()
        subprocess.run(["xcodebuild", "-project", str(args.project), "-scheme", "SVGMetaRenderer",
                        "-configuration", "Release", "-destination", f"platform=macOS,arch={arch}",
                        "-derivedDataPath", str(derived), "CODE_SIGNING_ALLOWED=NO", "-quiet", "build"],
                       check=True)
        products = derived / "Build/Products/Release"
        objects = (derived / "Build/Intermediates.noindex/SVGMetaRenderer.build/Release"
                   / "SVGMetaRenderer.build/Objects-normal" / arch)
        # Link the regression driver against the actual app objects and Skity framework.
        app_objects = [p for p in (objects / "SVGMetaRenderer.LinkFileList").read_text().splitlines()
                       if Path(p).name not in {"main.o", "ViewController.o", "AppDelegate.o"}]
        driver = Path(temp) / "skity_render_regression"
        subprocess.run(["xcrun", "clang++", "-std=c++17", "-fobjc-arc", "-I", str(SVG / "include"),
                        "-I", str(EXAMPLE / "SVGMetaRenderer"), "-F", str(products),
                        str(SVG / "testing/macos/SkityRenderRegression.mm"), *app_objects,
                        "-framework", "skity", "-framework", "Metal", "-framework", "Cocoa",
                        "-framework", "QuartzCore", "-framework", "CoreVideo", "-framework", "ImageIO",
                        "-lz", "-o", str(driver)], check=True)
        fixtures = products / "SVGMetaRenderer.app/Contents/Resources/svg"
        image = ET.parse(fixtures / "image.svg").getroot().find("{http://www.w3.org/2000/svg}image")
        png = base64.b64decode(image.attrib["href"].split(",", 1)[1], validate=True)
        requests = []

        class Handler(BaseHTTPRequestHandler):
            def do_GET(self):
                requests.append(self.path)
                if self.path.partition("?")[0] != "/image.png":
                    self.send_error(404)
                    return
                self.send_response(200)
                self.send_header("Content-Type", "image/png")
                self.send_header("Content-Length", str(len(png)))
                self.end_headers()
                self.wfile.write(png)

        with HTTPServer(("127.0.0.1", 0), Handler) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            try:
                url = f"http://127.0.0.1:{server.server_port}/image.png"
                command = [str(driver), str(fixtures), url, str(SVG / "testing/pattern_percent_cases.json")]
                if args.frames:
                    args.frames.mkdir(parents=True, exist_ok=True)
                    command.append(str(args.frames.resolve()))
                subprocess.run(command, check=True, timeout=60)
            finally:
                server.shutdown()
                thread.join()
        expected = ["/image.png", "/image.png?pattern-image-remote-whitelist",
                    "/image.png?pattern-image-remote-non-whitelist"]
        if requests != expected:
            raise RuntimeError(f"Expected one download per fixture across redraws, got {requests}")


if __name__ == "__main__":
    main()
