## Markdown

### build ios

run `markdown/tools/ios_build.sh` script to build ios example.

### build android

run `markdown/tools/android_build.sh` script to build android example.

### c++ unittests

c++ unittests are all in `markdown/testing/markdown/` folder.
run `markdown/tools/run_unittests.sh` script to execute unittests for markdown.

### props

props defined in `markdown/tools/markdown_props.json`.
new prop should *append* to the props array, do not insert it in the middle of the array.
after modify the props array, run `markdown/tools/props_generator.py` script to update the props enums.
