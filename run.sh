#!/bin/bash

SOURCE_DIR=$(
  cd $(dirname $0)
  pwd
)
BUILD_DIR="$SOURCE_DIR/build"

echo "--- 正在初始化 Debug 构建环境 ---"
[ -d "$BUILD_DIR" ] && rm -rf "$BUILD_DIR/*" || mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

# 3. 编译
cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j$(nproc)
if [ $? -ne 0 ]; then
  echo "编译失败！"
  exit 1
fi

# 4. 配置 ASan：发生错误时产生 abort 信号
export ASAN_OPTIONS=abort_on_error=1

echo "--- 程序启动中... ---"

# 5. 【核心逻辑】：直接运行程序
./study_app

# 获取程序的退出状态
EXIT_CODE=$?

# 6. 判断是否崩溃
# 在开启 ASan 的情况下，如果发生非法访问，状态码通常是 134 (SIGABRT) 或由 ASan 指定的错误码
if [ $EXIT_CODE -ne 0 ]; then
  echo -e "\n\033[31m[检测到程序崩溃或 ASan 报错！正在唤醒 GDB 进行现场勘察...]\033[0m"
  # 使用新安装的 GDB 启动现场
  /usr/local/bin/gdb -ex "run" "./study_app"
else
  echo -e "\n\033[32m[运行成功，未发现异常。]\033[0m"
fi
