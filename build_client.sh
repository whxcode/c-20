#!/bin/bash
set -e

SOURCE_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SOURCE_DIR"

# 编译和链接 client.cpp 所需的源文件
# -I. 让 #include "include/kiwi/include/schema.h" 能从项目根目录找到
g++ -std=c++20 \
  -I. \
  client.cpp \
  src/protocol/http.cpp \
  src/kiwi_impl.cpp \
  -o client

./client
