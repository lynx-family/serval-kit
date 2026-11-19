#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Build and run the portable SVG parser regressions with ASan and UBSan."""

import concurrent.futures
import os
from pathlib import Path
import subprocess
import tempfile

SVG = Path(__file__).resolve().parents[1]
CC = os.environ.get("CC", "clang")
CXX = os.environ.get("CXX", "clang++")
FLAGS = ["-g", "-O1", "-fno-omit-frame-pointer", "-fsanitize=address,undefined",
         "-fno-sanitize-recover=all", "-DSR_SVG_MIN_LOG_LEVEL=6", "-I" + str(SVG / "include")]


def main():
    sources = sorted((SVG / "src").rglob("*.cc"))
    sources += sorted((SVG / "src").rglob("*.c"))
    sources += [SVG / "testing/parser_regressions.cc"]
    with tempfile.TemporaryDirectory(prefix="serval-svg-parser-") as build:
        def compile_source(source):
            obj = Path(build) / (source.name + ".o")
            compiler, standard = (CC, "gnu2x") if source.suffix == ".c" else (CXX, "c++17")
            args = [compiler, "-std=" + standard, *FLAGS]
            if source.name == "SrSVGMemory.c":
                args += ["-include", str(SVG / "testing/allocation_hooks.h")]
            subprocess.run([*args, "-c", str(source), "-o", str(obj)], check=True)
            return obj

        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            objects = list(pool.map(compile_source, sources))
        executable = Path(build) / "parser_regressions"
        subprocess.run([CXX, *FLAGS, *map(str, objects), "-o", str(executable)], check=True)
        subprocess.run([str(executable), *map(str, sorted((SVG / "test_cases").glob("*.svg")))],
                       check=True, env={**os.environ, "ASAN_OPTIONS":
                           os.environ.get("ASAN_OPTIONS", "") + ":detect_stack_use_after_return=1"})


if __name__ == "__main__":
    main()
