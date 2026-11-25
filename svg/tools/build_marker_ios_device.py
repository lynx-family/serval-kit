#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Build the standalone Marker regression app using current iOS/shared sources."""

import argparse
import hashlib
import json
import pathlib
import plistlib
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--profile", type=pathlib.Path, required=True)
parser.add_argument("--identity", required=True, help="codesign identity name or SHA-1")
parser.add_argument("--output", type=pathlib.Path, required=True)
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
work = args.output.resolve()
app = work / "MarkerDevice.app"
app.mkdir(parents=True, exist_ok=True)
sdk = subprocess.check_output(["xcrun", "--sdk", "iphoneos", "--show-sdk-path"], text=True).strip()
common = ["-target", "arm64-apple-ios15.0", "-isysroot", sdk, "-I", str(root / "include"), "-I", str(root), "-O0", "-g"]
sources = sorted((root / "src").rglob("*.cc")) + sorted((root / "src").rglob("*.c"))
sources += sorted((root / "platform/iOS").glob("*.mm"))
sources += [root / "testing/ios/MarkerDeviceApp.mm"]
objects = []
for source in sources:
    obj = work / (hashlib.sha256(str(source).encode()).hexdigest()[:16] + ".o")
    objc = source.suffix == ".mm"
    compiler = "clang" if source.suffix == ".c" else "clang++"
    compiler_args = ["xcrun", compiler, *common]
    if compiler == "clang++":
        compiler_args += ["-std=c++17"]
    if objc:
        compiler_args += ["-fobjc-arc", "-include", "UIKit/UIKit.h"]
    subprocess.run([*compiler_args, "-c", str(source), "-o", str(obj)], check=True)
    objects.append(str(obj))
subprocess.run(["xcrun", "clang++", *common, *objects, "-o", str(app / "MarkerDevice"),
    *sum((["-framework", f] for f in ["UIKit", "Foundation", "CoreGraphics", "CoreText", "CoreImage", "ImageIO", "QuartzCore"]), [])], check=True)
bundle = "com.lynx.servalkit.svg.marker"
(app / "Info.plist").write_bytes(plistlib.dumps({
    "CFBundleIdentifier": bundle, "CFBundleExecutable": "MarkerDevice",
    "CFBundleName": "MarkerDevice", "CFBundleDisplayName": "SVG Marker",
    "CFBundleVersion": "1", "CFBundleShortVersionString": "1.0", "CFBundlePackageType": "APPL",
    "LSRequiresIPhoneOS": True, "MinimumOSVersion": "15.0", "UIDeviceFamily": [1, 2],
    "UIApplicationSceneManifest": {
        "UIApplicationSupportsMultipleScenes": False,
        "UISceneConfigurations": {
            "UIWindowSceneSessionRoleApplication": [
                {
                    "UISceneConfigurationName": "Default",
                    "UISceneDelegateClassName": "MarkerDeviceScene",
                }
            ],
        },
    },
    "UILaunchScreen": {}, "UISupportedInterfaceOrientations": ["UIInterfaceOrientationPortrait"]
}))
manifest = root / "testing/marker_cases.json"
shutil.copyfile(manifest, app / manifest.name)
for case in json.loads(manifest.read_text())["cases"]:
    shutil.copyfile(root / "test_cases" / case["file"], app / case["file"])
profile = args.profile.resolve()
data = plistlib.loads(subprocess.check_output(["security", "cms", "-D", "-i", str(profile)], stderr=subprocess.DEVNULL))
print("Profile:", data["ExpirationDate"], data["Entitlements"]["application-identifier"])
entitlements = data["Entitlements"].copy()
entitlements["application-identifier"] = data["TeamIdentifier"][0] + "." + bundle
entitlements["keychain-access-groups"] = [x.replace("*", bundle) for x in entitlements.get("keychain-access-groups", [])]
ent = work / "entitlements.plist"
ent.write_bytes(plistlib.dumps(entitlements))
shutil.copyfile(profile, app / "embedded.mobileprovision")
subprocess.run(["codesign", "--force", "--sign", args.identity,
    "--entitlements", str(ent), "--timestamp=none", str(app)], check=True)
subprocess.run(["codesign", "--verify", "--deep", "--strict", str(app)], check=True)
print(app)
