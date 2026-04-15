#!/bin/bash

# 1. 确保在脚本所在目录执行，避免路径混乱
SOURCE_DIR=$(
  cd $(dirname $0)
  pwd
)
BUILD_DIR="$SOURCE_DIR/build"

# 2. 深度清理与重新构建 (确保 p dogs 能生效)
echo "--- 正在初始化 Debug 构建环境 ---"
if [ -d "$BUILD_DIR" ]; then
  rm -rf "$BUILD_DIR/*"
else
  mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# 3. 强制开启 Debug 模式并编译
cmake -DCMAKE_BUILD_TYPE=Debug ..
if [ $? -ne 0 ]; then
  echo "CMake 配置失败！"
  exit 1
fi

make -j$(nproc)
if [ $? -ne 0 ]; then
  echo "编译失败！"
  exit 1
fi

# 4. 配置 ASan 环境变量：发现错误立即触发 SIGABRT 给 GDB 捕获
export ASAN_OPTIONS=abort_on_error=1

echo "--- 正在启动 GDB 调试 (已开启 ASan) ---"

# 5. 启动 GDB 并直接运行
# -ex "run": 启动后自动跑起来，直到撞到 f1 = 100 报错
gdb -ex "run" "./study_app"
