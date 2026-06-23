#include "include/06.nowmodel/constrcuor.h"

#include <functional>
#include <memory>
#include <vector>

class P : public std::enable_shared_from_this<P> {
public:
    P() {
        printf("Default Constrcuor\n");
    }
    P(const P& p) {
        printf("const Copy Constrcuor\n");
    }

    P(P& p) {
        printf("Copy Constrcuor\n");
    }

    P(P&& p) {
        printf("&& Constrcuor\n");
    }
    ~P() {
        printf("Destrcuor\n");
    }

    void setParent(

        const std::shared_ptr<P>& parent

    ) {
        this->parent = parent;
        parent->children.push_back(as<P>());
    }

    template <class T>

    std::shared_ptr<T> as() {
        return std::static_pointer_cast<T>(shared_from_this());
    }

public:
    std::weak_ptr<P> parent{};
    std::vector<std::shared_ptr<P>> children{};
};

static void f1(const P& p) {
    printf("const P& p\n");
    auto f = std::move(p);
}

static void f1(const P&& p) {
    printf("const P&& p\n");
    auto f = std::move(p);
}

static void test01() {
    P p1;
    f1(std::move(p1));
}

static void test02() {
    auto p1 = std::make_shared<P>();

    auto f1 = [](std::shared_ptr<P>& p) {
        std::vector<std::shared_ptr<P>> a;
        a.push_back(p->as<P>());
        return p;
    };

    auto a = f1(p1);
}

static void test03() {
    auto p0 = std::make_shared<P>();  // 2
    auto p1 = std::make_shared<P>();  // 2

    p1->setParent(p0);
    /**
     * 没调用 setParent 之前  p0,p1 的引用计数都是1.
     *
     * 调用之后. p1->parent = parent, p0.ref = 2
     * parent.children.push_back() p1.ref = 2;
     * */
}

static void test04() {
    /*
      using T = std::string;
      struct P {
          T a{"whx"};
          T& getA() {
              return a;
          }
      };
      using PS = std::vector<P>;

      PS v(10);

      std::function<void(T && v)> f2 = [](T&& v) {
          auto p = std::move(v);
      };

      auto f1 = [&f2](PS& ps) {
          for (auto& p : ps) {
              f2("whx");
              f2(std::move(p.a));
              // std::move(p.getA());
              // printf("p.a.size() = %s\n", p.a.c_str());
          }
      };

      f1(v);
    */

    using T = std::vector<int>;
    const T a(100);

    // 没有报错
    auto b = std::move(a);

    std::function<void(T && f)> f1 = [](T&& f) {
        auto c = std::move(f);
    };

    // const T c(100);

    //  报错;
    f1(std::move(T{}));
}

/**
 * 总结一下。
 * std::move(v) 只是说允许移动，但具体移动不移动还是要看起内部机制。
 * 比如 void f(T &&v)
 * f(std::move(v)) // 并不移动
 *
 * */
void NowModelCppConstructor() {
    test04();
    // test02();
    // test03();
}
