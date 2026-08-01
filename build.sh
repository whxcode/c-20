#!/bin/bash

bash ./include/kiwi/build.sh

SOURCE_DIR=$(
  cd $(dirname $0)
  pwd
)

BUILD_DIR="$SOURCE_DIR/build"

echo "--- 正在初始化 Debug 构建环境 ---"

[ -d "$BUILD_DIR" ] && rm -rf "$BUILD_DIR/*" || mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

# macOS 用 sysctl, Linux 用 nproc
if [ "$(uname)" = "Darwin" ]; then
  NPROC=$(sysctl -n hw.ncpu)
else
  NPROC=$(nproc 2>/dev/null || echo 4)
fi

cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j"$NPROC"

if [ $? -ne 0 ]; then
  echo "编译失败！"
  exit 1
fi

export ASAN_OPTIONS=abort_on_error=1

echo "--- 程序启动中... ---"

time ./study_app "$@"

EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
  echo -e "\n\033[31m[检测到程序崩溃或 ASan 报错！正在唤醒 GDB 进行现场勘察...]\033[0m"

  /usr/local/bin/gdb -ex "run $*" "./study_app"
else
  echo -e "\n\033[32m[运行成功，未发现异常。]\033[0m"
fi
