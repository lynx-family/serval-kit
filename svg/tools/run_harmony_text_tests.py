#!/usr/bin/env python3
"""Build and run the native SVG text regressions inside the Harmony example app."""

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import uuid

import json5


ROOT = Path(__file__).resolve().parents[2]


def run(args, cwd=None):
    subprocess.run([str(arg) for arg in args], cwd=cwd, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path,
                        default=Path("/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony"))
    parser.add_argument("--device", required=True)
    parser.add_argument("--signing-config", type=Path, required=True,
                        help="Local build-profile.json5 containing app.signingConfigs.")
    args = parser.parse_args()
    native = args.sdk / "native"
    cmake = native / "build-tools/cmake/bin/cmake"
    hdc = args.sdk / "toolchains/hdc"
    signing = json5.loads(args.signing_config.read_text())
    configs = signing["app"]["signingConfigs"]
    if not configs:
        raise ValueError("Signing configuration contains no signingConfigs.")

    with tempfile.TemporaryDirectory(prefix="serval-harmony-text-") as directory:
        work = Path(directory).resolve()
        build = work / "native"
        run([cmake, "-S", ROOT / "svg/testing/harmony", "-B", build, "-G", "Ninja",
             f"-DCMAKE_TOOLCHAIN_FILE={native}/build/cmake/ohos.toolchain.cmake",
             f"-DCMAKE_MAKE_PROGRAM={native}/build-tools/cmake/bin/ninja",
             "-DOHOS_ARCH=arm64-v8a", "-DOHOS_PLATFORM_LEVEL=12", "-DCMAKE_BUILD_TYPE=Debug"])
        run([cmake, "--build", build, "--target", "text_tests_napi"])
        app = work / "app"
        shutil.copytree(ROOT / "svg/examples/harmony", app,
                        ignore=shutil.ignore_patterns("build", "oh_modules", ".hvigor",
                                                     "local.properties"))
        profile = json5.loads((app / "build-profile.json5").read_text())
        profile["app"]["signingConfigs"] = configs
        profile["app"]["products"][0]["signingConfig"] = configs[0]["name"]
        serval = work / "servalsvg"
        shutil.copytree(ROOT / "svg/platform/harmony/servalsvg", serval,
                        ignore=shutil.ignore_patterns("build", "oh_modules", ".hvigor"))
        cmake_file = serval / "src/main/cpp/CMakeLists.txt"
        cmake_file.write_text(cmake_file.read_text().replace(
            "set(SVG_SRC_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..)",
            f'set(SVG_SRC_DIRECTORY "{ROOT}/svg")'))
        module_profile = serval / "build-profile.json5"
        module_options = json5.loads(module_profile.read_text())
        module_options["buildOption"]["nativeLib"]["headerPath"] = str(
            ROOT / "svg/include/platform/harmony/public")
        module_profile.write_text(json.dumps(module_options, indent=2))
        for module in profile["modules"]:
            if module["name"] == "servalsvg":
                module["srcPath"] = "../servalsvg"
        (app / "build-profile.json5").write_text(json.dumps(profile, indent=2))
        package_file = app / "entry/oh-package.json5"
        package = json5.loads(package_file.read_text())
        package["dependencies"]["@lynx/servalsvg"] = f"file:{serval}"
        package["dependencies"]["libtext_tests.so"] = "file:./src/main/cpp/types/libtext_tests"
        package_file.write_text(json.dumps(package, indent=2))
        types = app / "entry/src/main/cpp/types/libtext_tests"
        types.mkdir(parents=True)
        (types / "oh-package.json5").write_text(json.dumps({
            "name": "libtext_tests.so", "version": "1.0.0", "types": "./Index.d.ts"}))
        (types / "Index.d.ts").write_text("export const run: () => string;\n")
        libraries = app / "entry/libs/arm64-v8a"
        libraries.mkdir(parents=True, exist_ok=True)
        shutil.copy2(build / "libtext_tests.so", libraries / "libtext_tests.so")
        marker = f"SVGTEXT_RESULT_{uuid.uuid4().hex}"
        page = app / "entry/src/main/ets/pages/TextRegression.ets"
        page.write_text(
            "import { run } from 'libtext_tests.so';\n"
            "import { hilog } from '@kit.PerformanceAnalysisKit';\n"
            "import { common } from '@kit.AbilityKit';\n"
            "import fs from '@ohos.file.fs';\n"
            "@Entry\n@Component\nstruct TextRegression {\n"
            "  @State result: string = 'Running';\n"
            "  aboutToAppear(): void {\n"
            "    this.result = run();\n"
            "    const context = this.getUIContext().getHostContext() as common.UIAbilityContext;\n"
            "    const file = fs.openSync(`${context.filesDir}/text-regression.txt`,\n"
            "      fs.OpenMode.CREATE | fs.OpenMode.READ_WRITE | fs.OpenMode.TRUNC);\n"
            f"    fs.writeSync(file.fd, '{marker}\\n' + this.result);\n"
            "    fs.closeSync(file);\n"
            f"    hilog.info(0, 'SVGTEXT', '{marker}=%{{public}}s', this.result);\n"
            "  }\n"
            "  build() { Column() { Text(this.result) } }\n"
            "}\n")
        pages = app / "entry/src/main/resources/base/profile/main_pages.json"
        pages.write_text(json.dumps({"src": ["pages/TextRegression"]}))
        ability = app / "entry/src/main/ets/entryability/EntryAbility.ets"
        ability.write_text(ability.read_text().replace(
            "this.patternRegression ? 'pages/PatternRegression' : 'pages/Index'",
            "'pages/TextRegression'"))
        ohpm = Path("/Applications/DevEco-Studio.app/Contents/tools/ohpm/bin/ohpm")
        run([ohpm, "install"], cwd=app)
        run(["bash", app / "hvigorw", "assembleHap", "--mode", "module",
             "-p", "module=entry@default", "-p", "product=default",
             "-p", "buildMode=debug", "--no-daemon"], cwd=app)
        hap = app / "entry/build/default/outputs/default/entry-default-signed.hap"
        if not hap.is_file():
            raise FileNotFoundError(f"Signed HAP not produced: {hap}")
        run([hdc, "-t", args.device, "install", "-r", hap])
        run([hdc, "-t", args.device, "shell", "aa", "force-stop", "net.lynx.servalsvg"])
        run([hdc, "-t", args.device, "shell", "aa", "start", "-a", "EntryAbility",
             "-b", "net.lynx.servalsvg"])
        time.sleep(5)
        report = work / "text-regression.txt"
        run([hdc, "-t", args.device, "file", "recv", "-b", "net.lynx.servalsvg",
             "/data/storage/el2/base/haps/entry/files/text-regression.txt", report])
        result = report.read_text()
        print(result)
        if marker not in result or "TEXT_TESTS_RESULT=PASS" not in result:
            raise RuntimeError("Current text regression run did not report PASS.")


if __name__ == "__main__":
    main()
