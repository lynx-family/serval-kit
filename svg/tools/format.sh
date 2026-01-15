#!/bin/sh

files=$(find ../Svg/src -type f -name "*.cc" -o -name "*.c" -o -name "*.h")

for f in $files; do
  echo "Formatting $f"
  clang-format -i "$f"
done
