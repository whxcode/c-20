#include <iostream>
#include <ranges> // C++20 新特性：Ranges
#include <vector>

int main() {
  std::vector<int> numbers = {1, 2, 3, 4, 5, 6};

  // 使用 C++20 的 View 过滤偶数
  auto even_numbers =
      numbers | std::views::filter([](int n) { return n % 2 == 0; });

  std::cout << "C++20 项目运行完成，偶数有: " << std::endl;
  for (int n : even_numbers) {
    std::cout << n << " ";
  }
  std::cout << std::endl;
  return 0;
}
