# 1. 指定 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 2. 定义项目名称
project(Cpp20Study)

# 3. 【核心】指定使用 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. 【重要】为 clangd 生成编译指令文件，让 Neovim 的补全更精准
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 5. 添加执行文件
add_executable(study_app ./src/main.cpp)
