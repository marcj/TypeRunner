#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include <string>
#include "../checker/MemoryPool.h"
#include <vector>

struct Item {
    std::string_view title;
    unsigned int i;
    std::vector<int> jopp;
};

TEST_CASE("allocator1") {
    MemoryPool<Item, 2> pool;
    REQUIRE(pool.blocks == 0);
    REQUIRE(pool.active == 0);

    auto p1 = pool.allocate();
    REQUIRE(pool.active == 1);
    REQUIRE(pool.blocks == 1);

    auto p2 = pool.allocate();
    REQUIRE(pool.blocks == 1);
    REQUIRE(pool.active == 2);

    pool.deallocate(p1);
    REQUIRE(pool.blocks == 1);
    REQUIRE(pool.active == 1);

    pool.deallocate(p2);
    REQUIRE(pool.blocks == 1);
    REQUIRE(pool.active == 0);

    auto p3 = pool.allocate();
    REQUIRE(pool.active == 1);
    REQUIRE(pool.blocks == 1);

    auto p4 = pool.allocate();
    REQUIRE(pool.blocks == 1);
    REQUIRE(pool.active == 2);
}

TEST_CASE("allocator2") {
    MemoryPool<Item, 2> pool;
    auto p1 = pool.allocate();
    auto p2 = pool.allocate();
    REQUIRE(pool.active == 2);
    REQUIRE(pool.blocks == 1);

    auto p3 = pool.allocate();
    REQUIRE(pool.active == 3);
    REQUIRE(pool.blocks == 2);

    auto p4 = pool.allocate();
    REQUIRE(pool.active == 4);
    REQUIRE(pool.blocks == 2);

    auto p5 = pool.allocate();
    REQUIRE(pool.active == 5);
    REQUIRE(pool.blocks == 3);
}

TEST_CASE("allocator3") {
    MemoryPool<Item, 2> pool;
    {
        auto p1 = pool.allocate();
        auto p2 = pool.allocate();
        REQUIRE(pool.active == 2);
        REQUIRE(pool.blocks == 1);

        auto p3 = pool.allocate();
        REQUIRE(pool.active == 3);
        REQUIRE(pool.blocks == 2);

        auto p4 = pool.allocate();
        REQUIRE(pool.active == 4);
        REQUIRE(pool.blocks == 2);
    }

    {
        pool.clear();
        auto p1 = pool.allocate();
        REQUIRE(pool.active == 1);
        REQUIRE(pool.blocks == 2);

        auto p2 = pool.allocate();
        REQUIRE(pool.active == 2);
        REQUIRE(pool.blocks == 2);

        //now block 2 should be reused
        auto p3 = pool.allocate();
        REQUIRE(pool.active == 3);
        REQUIRE(pool.blocks == 2);

        //now block 2 should be reused
        auto p4 = pool.allocate();
        REQUIRE(pool.active == 4);
        REQUIRE(pool.blocks == 2);

        //now block 3 should be created
        auto p5 = pool.allocate();
        REQUIRE(pool.active == 5);
        REQUIRE(pool.blocks == 3);
    }
}

TEST_CASE("allocator4") {
    MemoryPool<Item, 2> pool;
    pool.clear();

    auto p1 = pool.newElement();
    REQUIRE(p1->i == 0);
    REQUIRE(p1->title == "");

    auto p2 = pool.newElement();
    REQUIRE(p2->i == 0);
    REQUIRE(p2->title == "");
    p2->i = 2;
    REQUIRE(p2->i == 2);

    pool.deleteElement(p2);

    auto p3 = pool.newElement();
    REQUIRE(p3 == p2);
    REQUIRE(p3->i == 0);
    REQUIRE(p3->title == "");
}

TEST_CASE("allocator5") {
    MemoryPool<Item, 2> pool;
    pool.clear();

    auto p1 = pool.newElement();
    p1->i = 1;
    auto p2 = pool.newElement();
    p2->i = 2;

    auto p3 = pool.newElement();
    p3->i = 3;
    auto p4 = pool.newElement();
    p4->i = 4;

    auto p5 = pool.newElement();
    p5->i = 5;
    auto p6_ = pool.newElement();
    pool.deleteElement(p6_);

    auto p6 = pool.newElement();
    p6->i = 6;
    REQUIRE(pool.active == 6);
    REQUIRE(pool.blocks == 3);

    REQUIRE(p1->i == 1);
    REQUIRE(p2->i == 2);
    REQUIRE(p3->i == 3);
    REQUIRE(p4->i == 4);
    REQUIRE(p5->i == 5);
    REQUIRE(p6->i == 6);
}