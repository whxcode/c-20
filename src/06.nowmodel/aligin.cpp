#include <array>
#include <vector>

#include "include/06.nowmodel/constrcuor.h"

template <std::size_t N>
struct FixedSize {
    std::array<std::byte, N> data;
};

static void test01() {
    struct P1 {
        char a;
        double b;
        char c;
    };

    struct P2 {
        double b;
        char a;
        double c;
        // 8,4____,8
    };

    struct P3 {
        double b;
        FixedSize<300> a;
        double c;
    };

    struct P4 {
        double b;
        char x;
        FixedSize<300> a;
        int c;
    };

    struct P5 {
        FixedSize<11> a;
        char x;
    };

    struct P6 {
        int x;
        FixedSize<11> a;
    };

    struct alignas(4) P7 {
        FixedSize<11> a;
    };

    struct alignas(8) P8 {
        FixedSize<11> a;
    };

    struct alignas(8) P9 {
        char a;           // 1
        bool b;           // 1
        FixedSize<11> c;  // 11
        int d;
    };

    struct P10 {
        int a;
        double b;
        P9 c;
        char d;
    };

    struct P11 {
        char a;    // 0,
        double b;  // 7
    };

    struct P12 {
        double b;  // 7
        char a;    // 0,
    };

    // std::cout << sizeof(P1) << std::endl;
    // std::cout << sizeof(P2) << std::endl;
    // std::cout << sizeof(P3) << std::endl;
    // std::cout << sizeof(P4) << std::endl;
    // std::cout << sizeof(P5) << std::endl;
    // std::cout << sizeof(P6) << std::endl;
    std::cout << sizeof(P7) << std::endl;
    std::cout << sizeof(P8) << std::endl;
    std::cout << sizeof(P9) << std::endl;   // 24
    std::cout << sizeof(P10) << std::endl;  // 48
    /**
     * offset = 0
     * for prop in props:
     *  offset = align_up(offset,alignof(prop))
     *  offset + = sizeof(of)
     *
     * offset = align_up(offset,alignof(P))
     * ---
     * */
}

// Array of Strcuties (单个)
static void AOS() {
    // 对象思维。管理单个对象。
    struct Point {
        int x{0};
        int y{0};
    };

    /**
     * memory layout [x,y],[x,y]...[x,y]
     * using points[i].x,points[].y
     * */
    std::vector<Point> points;
}

// Structure of Array (批量)
static void SOA() {
    /**
     * 批量处理思维，起内存布局
     * 适合执行一组批量数据处理。比如类似于矩阵运算的列/行运算。
     *
     * xs: [x,...,x]
     * ys: [y,...,y]
     * */

    struct Points {
        std::vector<float> xs;
        std::vector<float> xy;
    };
}

__attribute__((noinline)) static void ECS() {
    /**
     * 批量处理思维，起内存布局
     * 适合执行一组批量数据处理。比如类似于矩阵运算的列/行运算。
     *
     * xs: [x,...,x]
     * ys: [y,...,y]
     * */

    struct SOAPoints {
        SOAPoints() {
            xs.resize(10);
            ys.resize(10);
        }
        std::vector<float> xs;
        std::vector<float> ys;
    };

    struct AOSPoints {
        float x{0};
        float y{0};
    };

    std::vector<AOSPoints> aos(10);
    SOAPoints soa;

    auto updateAOS = [](std::vector<AOSPoints>& points) {
        for (auto& p : points) {
            p.x += 1.0f;
        }
    };

    auto updateSOA = [](SOAPoints& points) {
        for (auto& x : points.xs) {
            x += 1.0f;
        }
    };

    updateAOS(aos);
    updateSOA(soa);

    // 防止编译器把结果完全优化掉
    std::cout << aos[0].x << "\n";
    std::cout << soa.xs[0] << "\n";
}

static void TestSparseSet() {
    class SparseSet {
    public:
    private:
    };
}

static void test06() {
    struct P0 {
        char a;
        double b;  // 16
        int c;     // 20
    };

    struct P1 {
        char a;    // 8
        int c;     // 8
        double b;  // 16
    };

    std::cout << sizeof(P0) << std::endl;
    std::cout << sizeof(P1) << std::endl;
}

__attribute__((noinline)) void NowModelMemoryAlign() {
    test06();
    // std::cout << "NowModelMemoryAlign" << std::endl;
    // ECS();
    // test01();
}
