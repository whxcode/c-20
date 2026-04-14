# 1. 指定最低版本和项目名称
cmake_minimum_required(VERSION 3.10)
project(Cpp20Study)

# 2. 核心设置：C++20 标准与 Clangd 补全支持
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 这一行非常重要，它生成的 json 文件能让你的 Neovim 拥有丝滑的补全
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 3. 自动搜集所有的源文件 (.cpp)
# 使用 GLOB_RECURSE 会自动深层查找 src 目录下所有的 cpp 文件
file(GLOB_RECURSE ALL_SOURCES
    "main.cpp"
    "src/*.cpp"
)

# 4. 自动搜集所有的头文件路径
# 我们不需要手动写 include/01Exmale，直接让 CMake 找 include 文件夹
# 这样你在代码里写 #include "01Exmale/01.example.h" 就能直接识别
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 5. 生成可执行程序
add_executable(study_app ${ALL_SOURCES})

# 打印一下找到的文件，方便你调试（编译时会在终端显示）
message(STATUS "Found sources: ${ALL_SOURCES}")
