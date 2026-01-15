// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <cstdio>
#include <fstream>

#include "../../src/main/include/parser/SrDOM.h"
#include "../../src/main/include/parser/SrSVGDOM.h"
int main() {
  FILE* fp = fopen("./demo.svg", "r");
  if (fp == NULL) {
    return -1;
  }
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  rewind(fp);
  char* buffer = (char*)malloc(file_size + 1);
  if (buffer == NULL) {
    return -1;
  }
  if (fread(buffer, 1, file_size, fp) != file_size) {
    return -1;
  }

  buffer[file_size] = '\0';

  auto svgDom = SrSVGDOM::make(buffer, file_size + 1);

  if (svgDom) {
    printf("build success!\n");
  } else {
    printf("build failed!\n");
  }
  fclose(fp);
  return 0;
}
