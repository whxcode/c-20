#include "include/common.h"

#include <random>  // 必须包含这个头文件

int generateRandom() {
    // 1. 硬件随机数生成器，用来做种子
    std::random_device rd;

    // 2. 使用梅森旋转算法引擎 (Mersenne Twister)，并用种子初始化
    std::mt19937 gen(rd());

    // 3. 定义分布范围 [0, 100]，闭区间
    std::uniform_int_distribution<> dis(0, 100);

    return dis(gen);
}
