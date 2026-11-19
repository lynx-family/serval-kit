#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
"""Run the shared parser without exceptions; optionally fuzz the same OOM paths."""
import argparse
import concurrent.futures
import os
from pathlib import Path
import re
import subprocess
import tempfile

SVG = Path(__file__).resolve().parents[1]
CORE = ["src/parser/SrSVGMemory.c", "src/parser/SrSVGParse.c", "src/utils/SrSVGArc.c",
        "src/element/SrSVGTypes.c", "src/parser/SrXMLExtractor.c", "src/parser/SrDOM.cc",
        "src/parser/SrDOMParser.cc", "src/parser/SrXMLParser.cc", "src/parser/SrXMLParserError.cc",
        "testing/core_test_support.cc"]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fuzz-seconds", type=int, default=0)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--fuzzer-library", type=Path, help="Optional LLVM libFuzzer runtime archive")
    args = parser.parse_args()
    build = args.output or Path(tempfile.mkdtemp(prefix="serval-svg-core-"))
    build.mkdir(parents=True, exist_ok=True)
    cc, cxx = os.environ.get("CC", "clang"), os.environ.get("CXX", "clang++")
    flags = ["-g", "-O1", "-fno-omit-frame-pointer", "-fsanitize=address,undefined",
             "-fno-sanitize-recover=all", "-I" + str(SVG / "include"),
             "-I" + str(SVG / "testing")]
    if args.fuzz_seconds:
        flags += ["-fsanitize=fuzzer-no-link"]
    sources = [SVG / path for path in CORE] + [SVG / "testing/core_regressions.cc"]
    def compile_source(source):
        obj = build / (source.name + ".o")
        c = source.suffix == ".c"
        opts = ["-std=gnu2x"] if c else ["-std=c++17", "-fno-exceptions", "-fno-rtti"]
        if source.name == "SrSVGMemory.c":
            opts += ["-include", str(SVG / "testing/allocation_hooks.h")]
        subprocess.run([cc if c else cxx, *flags, *opts, "-c", str(source), "-o", str(obj)], check=True)
        return obj
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        objects = list(pool.map(compile_source, sources))
    executable = build / "core_regressions"
    subprocess.run([cxx, *flags, *map(str, objects), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
    if args.fuzz_seconds:
        obj = compile_source(SVG / "testing/parser_fuzz.cc")
        fuzzer = build / "parser_fuzz"
        runtime = [str(args.fuzzer_library.resolve())] if args.fuzzer_library else ["-fsanitize=fuzzer"]
        subprocess.run([cxx, *flags, *map(str, objects[:-1]), str(obj), *runtime, "-o", str(fuzzer)], check=True)
        corpus = build / "corpus"
        corpus.mkdir(exist_ok=True)
        seeds = [b"M0 0 A1 1 0", b"M0 0 C1 2 3 4 5 6 S7 8 9 10 Z", b"M0 0 A20 30 45 01 80 90",
                 b"0 0 10 10", b"rotate(10,20)", b"matrix(1 0 0 1 2 3)", b"1e999", b"1em", b"0 0 100 100"]
        for fixture in sorted((SVG / "test_cases").glob("*.svg")):
            text = fixture.read_bytes(); seeds.append(text)
            seeds += re.findall(rb'\bd="([^"]*)"', text)
        for i, seed in enumerate(seeds): (corpus / f"seed-{i}").write_bytes(seed)
        with (build / "fuzz.log").open("w") as log:
            subprocess.run([str(fuzzer), str(corpus), "-max_len=65536", "-timeout=5",
                            f"-max_total_time={args.fuzz_seconds}", f"-artifact_prefix={build}/"],
                           stdout=log, stderr=subprocess.STDOUT, check=True)
        print("\n".join((build / "fuzz.log").read_text().splitlines()[-8:]))
    print(f"Artifacts: {build}")

if __name__ == "__main__":
    main()
