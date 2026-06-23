#include "include/05commit/commit.h"

#include <any>
#include <cassert>
#include <chrono>
#include <future>
#include <thread>

#include "include/05commit/collector.h"
#include "include/05commit/commitServer.h"
#include "include/05commit/document.h"
#include "include/06PriorityQueue.h"

static void test01() {
    auto doc = new Document();
    auto commitServer = new CommitServer(doc);

    auto model1 = doc->getModel(0);
    assert(model1 != nullptr);
    assert(model1->getName() == "默认名称1");

    model1->setName("新图层1");
    doc->removeChild(model1);
    commitServer->commit();

    commitServer->undo();

    auto restored = doc->getModel(0);
    assert(restored != nullptr);
    assert(restored->getName() == "默认名称1");

    delete doc;
    delete commitServer;
}

static void test02() {
    auto* doc = new Document();
    auto* commitServer = new CommitServer(doc);

    auto* model3 = doc->createModel(100, "图层3");
    doc->addChild(model3);
    doc->removeChild(model3);
    commitServer->commit();

    commitServer->undo();

    assert(doc->getModel(100) == nullptr);

    delete doc;
    delete commitServer;
}

static void test03() {
    auto doc = new Document();
    auto commitServer = new CommitServer(doc);

    auto model1 = doc->getModel(0);
    assert(model1 != nullptr);

    doc->removeChild(model1);
    doc->addChild(model1);
    commitServer->commit();

    commitServer->undo();

    auto current = doc->getModel(0);
    assert(current != nullptr);
    assert(current->getName() == "默认名称1");

    delete doc;
    delete commitServer;
}

static void test04() {
    auto doc = new Document();
    auto commitServer = new CommitServer(doc);

    auto model1 = doc->getModel(0);
    assert(model1 != nullptr);

    model1->setName("新图层1");
    commitServer->commit();

    doc->removeChild(model1);
    commitServer->commit();

    commitServer->undo();

    auto* restored = doc->getModel(0);
    assert(restored != nullptr);
    assert(restored->getName() == "新图层1");

    delete doc;
    delete commitServer;
}

static void test05() {
    auto doc = new Document();
    auto commitServer = new CommitServer(doc);

    auto model1 = doc->getModel(0);
    assert(model1 != nullptr);

    doc->removeChild(model1);
    commitServer->commit();

    assert(doc->getModel(0) == nullptr);

    commitServer->undo();
    auto restored = doc->getModel(0);
    assert(restored != nullptr);
    assert(restored->getName() == "默认名称1");

    commitServer->redo();
    assert(doc->getModel(0) == nullptr);

    delete doc;
    delete commitServer;
}

static void test06() {
    auto doc = new Document();
    auto commitServer = new CommitServer(doc);

    auto model1 = doc->getModel(0);
    assert(model1 != nullptr);

    model1->setName("第一次提交");
    commitServer->commit();

    model1->setName("第二次提交");
    commitServer->commit();

    commitServer->undo();
    auto current = doc->getModel(0);
    assert(current != nullptr);
    assert(current->getName() == "第一次提交");

    current->setName("新分支");
    commitServer->commit();

    commitServer->redo();
    current = doc->getModel(0);
    assert(current != nullptr);
    assert(current->getName() == "新分支");

    delete doc;
    delete commitServer;
}

static void test07() {
    auto doc = new Document();
    auto commitServer = new CommitServer(doc);

    auto model1 = doc->getModel(0);
    assert(model1 != nullptr);

    model1->setName("批量修改");
    model1->setX(100);
    model1->setY(200);
    commitServer->commit();

    assert(model1->getName() == "批量修改");
    assert(model1->getX() == 100);
    assert(model1->getY() == 200);

    commitServer->undo();
    auto restored = doc->getModel(0);
    assert(restored != nullptr);
    assert(restored->getName() == "默认名称1");
    assert(restored->getX() == 0);
    assert(restored->getY() == 0);

    delete doc;
    delete commitServer;
}

static void test08() {
    auto doc = new Document();
    auto commitServer = new CommitServer(doc);

    auto model1 = doc->getModel(0);
    assert(model1 != nullptr);

    for (int i = 1; i <= 12; ++i) {
        model1->setX(i);
        commitServer->commit();
    }

    for (int i = 0; i < 10; ++i) {
        commitServer->undo();
    }

    auto current = doc->getModel(0);
    assert(current != nullptr);
    assert(current->getX() == 2);

    commitServer->undo();
    assert(current->getX() == 2);

    delete doc;
    delete commitServer;
}

void CommitTest() {
    test01();
    test02();
    test03();
    test04();
    test05();
    test06();
    test07();
    test08();

    printf("commit tests passed\n");
}

void TestTake() {
    std::promise<int> promise;
    auto f2 = promise.get_future();

    std::thread t2{[&promise]() {
        promise.set_value(20);
    }};

    std::packaged_task<int()> p1{[]() {
        return 10;
    }};

    auto f1 = p1.get_future();

    std::thread t1{std::move(p1)};

    t1.join();
    t2.join();
    auto f3 = std::async([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        return 1000;
    });

    std::cout << "f1:" << f1.get() << std::endl;
    std::cout << "f2:" << f2.get() << std::endl;
    std::cout << "f3:" << f3.get() << std::endl;
    /*
    PriorityQueue pq;

    pq.enqueue(1, []() {
        printf("任务 0\n");
    });

    auto taskId1 = pq.enqueue(2, []() {
        printf("任务 1\n");
    });

    auto taskId2 = pq.enqueue(3, []() {
        printf("任务 2\n");
    });

    pq.update(taskId2, 0);

    while (!pq.empty()) {
        auto t = pq.dequeue();
        if (t.has_value()) {
            (*t)();
        }
    }
  */
}
