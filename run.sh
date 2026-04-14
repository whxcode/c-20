#!/bin/bash

# 1. 定义 build 文件夹名称
BUILD_DIR="build"

# 2. 如果不存在 build 目录则创建，如果存在则直接进入
if [ ! -d "$BUILD_DIR" ]; then
  mkdir "$BUILD_DIR"
fi

# 3. 进入 build 目录
cd "$BUILD_DIR" || exit

# 4. 运行 CMake 配置
# 使用 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 确保 clangd 始终有最新的索引
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 5. 编译项目
# 使用 -j$(nproc) 可以利用多核并行编译，速度更快
make -j$(nproc)

# 6. 检查编译是否成功
if [ $? -eq 0 ]; then
  echo -e "\n---------------- 执行结果 ----------------"
  # 运行程序（假设你的可执行文件名是 study_app）
  ./study_app
else
  echo -e "\n❌ 编译失败，请检查代码错误。"
fi
