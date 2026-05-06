## Markdown

### env prepare

run `source tools/envsetup.sh` script to initialize env.
run `./hab sync .` to download deps. for c++ unittests, add extra `--target testing` param.
these two step must be ran before build or unittests.

### build platform demo

run `markdown/tools/${PLATFORM}_build.sh` script to build android/ios/harmony platform examples.

### harmony markdown

Harmony markdown depends on `@lynx/lynxtextra` from OHPM. Keep the dependency as `"@lynx/lynxtextra": "0.1.1"` in `markdown/platform/harmony/serval_markdown/oh-package.json5`.
Do not add `third_party/lynx-textra/platform/harmony/lynxtextra` as a source module in the Harmony example `build-profile.json5`.
In CMake, use `find_package(lynxtextra CONFIG)` and link `lynxtextra::lynxtextra`; do not hardcode paths under `oh_modules/@lynx/lynxtextra/build/.../liblynxtextra.so`.

### c++ unittests

c++ unittests are all in `markdown/testing/markdown/` folder.
run `markdown/tools/run_unittests.sh` script to execute unittests for markdown.

### props

props defined in `markdown/tools/markdown_props.json`.
new prop should *append* to the props array, do not insert it in the middle of the array.
after modify the props array, run `markdown/tools/props_generator.py` script to update the props enums.
