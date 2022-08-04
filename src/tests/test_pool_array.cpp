#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>

#include <string>
#include "../checker/pool_array.h"
#include "../core.h"
#include <vector>

struct Item {
    std::string_view title;
    unsigned int i;
    std::vector<int> jopp;
};

TEST_CASE("basic") {
    ts::debug("std::span<Item> = {}", sizeof(std::span<Item>));
}

TEST_CASE("block creation") {
    PoolArray<Item, 2> pool;
    REQUIRE(pool.pool(1).blocks == 0);
    REQUIRE(pool.active == 0);

    auto p1 = pool.allocate(1);
    REQUIRE(pool.active == 1);
    REQUIRE(pool.pool(1).blocks == 1);

    auto p2 = pool.allocate(1);
    REQUIRE(pool.pool(1).blocks == 1);
    REQUIRE(pool.active == 2);

    pool.deallocate(p1);
    REQUIRE(pool.pool(1).blocks == 1);
    REQUIRE(pool.active == 1);

    pool.deallocate(p2);
    REQUIRE(pool.pool(1).blocks == 1);
    REQUIRE(pool.active == 0);

    auto p3 = pool.allocate(1);
    REQUIRE(pool.active == 1);
    REQUIRE(pool.pool(1).blocks == 1);

    auto p4 = pool.allocate(1);
    REQUIRE(pool.pool(1).blocks == 1);
    REQUIRE(pool.active == 2);
}

TEST_CASE("multi pool") {
    PoolArray<Item, 10> pool;
    auto p1 = pool.construct(2);
    REQUIRE(pool.active == 2);
    REQUIRE(pool.pool(2).blocks == 1);

    auto p2 = pool.construct(3);
    REQUIRE(pool.active == 5);
    REQUIRE(pool.pool(2).blocks == 1);
    REQUIRE(pool.pool(4).blocks == 1);

    pool.destruct(p2);
    REQUIRE(pool.active == 2);
    REQUIRE(pool.pool(2).blocks == 1);
    REQUIRE(pool.pool(4).blocks == 1);
}

TEST_CASE("mixed") {
    PoolArray<Item, 5> pool;
    REQUIRE(pool.pool(1).blocks == 0);
    REQUIRE(pool.active == 0);

    auto p1 = pool.allocate(1);
    REQUIRE(pool.pool(1).blocks == 1);
    REQUIRE(pool.active == 1);

    auto p2 = pool.allocate(2);
    REQUIRE(pool.pool(1).blocks == 1);
    REQUIRE(pool.active == 3);

    pool.deallocate(p1);
    REQUIRE(pool.pool(1).blocks == 1);
    REQUIRE(pool.active == 2);

    pool.deallocate(p2);
    REQUIRE(pool.pool(1).blocks == 1);
    REQUIRE(pool.active == 0);

    auto p3 = pool.allocate(1);
    REQUIRE(pool.pool(1).blocks == 1);
    REQUIRE(pool.active == 1);

    pool.deallocate(p3);
    REQUIRE(pool.pool(1).blocks == 1);
    REQUIRE(pool.active == 0);
}

TEST_CASE("allocator2") {
    PoolArray<Item, 2> pool;
    auto p1 = pool.allocate(1);
    auto p2 = pool.allocate(1);
    REQUIRE(pool.active == 2);
    REQUIRE(pool.pool(1).blocks == 1);

    auto p3 = pool.allocate(1);
    REQUIRE(pool.active == 3);
    REQUIRE(pool.pool(1).blocks == 2);

    auto p4 = pool.allocate(1);
    REQUIRE(pool.active == 4);
    REQUIRE(pool.pool(1).blocks == 2);

    auto p5 = pool.allocate(1);
    REQUIRE(pool.active == 5);
    REQUIRE(pool.pool(1).blocks == 3);
}

TEST_CASE("allocator3") {
    PoolArray<Item, 2> pool;
    {
        auto p1 = pool.allocate(1);
        auto p2 = pool.allocate(1);
        REQUIRE(pool.active == 2);
        REQUIRE(pool.pool(1).blocks == 1);

        auto p3 = pool.allocate(1);
        REQUIRE(pool.active == 3);
        REQUIRE(pool.pool(1).blocks == 2);

        auto p4 = pool.allocate(1);
        REQUIRE(pool.active == 4);
        REQUIRE(pool.pool(1).blocks == 2);
    }

    {
        pool.clear();
        auto p1 = pool.allocate(1);
        REQUIRE(pool.active == 1);
        REQUIRE(pool.pool(1).blocks == 2);

        auto p2 = pool.allocate(1);
        REQUIRE(pool.active == 2);
        REQUIRE(pool.pool(1).blocks == 2);

        //now block 2 should be reused
        auto p3 = pool.allocate(1);
        REQUIRE(pool.active == 3);
        REQUIRE(pool.pool(1).blocks == 2);

        //now block 2 should be reused
        auto p4 = pool.allocate(1);
        REQUIRE(pool.active == 4);
        REQUIRE(pool.pool(1).blocks == 2);

        //now block 3 should be created
        auto p5 = pool.allocate(1);
        REQUIRE(pool.active == 5);
        REQUIRE(pool.pool(1).blocks == 3);
    }
}

TEST_CASE("allocator4") {
    PoolArray<Item, 2> pool;
    pool.clear();

    auto p1 = pool.construct(1);
    REQUIRE(p1[0].i == 0);
    REQUIRE(p1[0].title == "");

    auto p2 = pool.construct(1);
    REQUIRE(p2[0].i == 0);
    REQUIRE(p2[0].title == "");
    p2[0].i = 2;
    REQUIRE(p2[0].i == 2);

    pool.destruct(p2);

    auto p3 = pool.construct(1);
    REQUIRE(&p3[0] == &p2[0]);
    REQUIRE(p3[0].i == 0);
    REQUIRE(p3[0].title == "");
}

TEST_CASE("allocator5") {
    PoolArray<Item, 2> pool;
    pool.clear();

    auto p1 = pool.construct(1);
    p1[0].i = 1;
    auto p2 = pool.construct(1);
    p2[0].i = 2;

    auto p3 = pool.construct(1);
    p3[0].i = 3;
    auto p4 = pool.construct(1);
    p4[0].i = 4;

    auto p5 = pool.construct(1);
    p5[0].i = 5;
    auto p6_ = pool.construct(1);
    pool.destruct(p6_);

    auto p6 = pool.construct(1);
    p6[0].i = 6;
    REQUIRE(pool.active == 6);
    REQUIRE(pool.pool(1).blocks == 3);

    REQUIRE(p1[0].i == 1);
    REQUIRE(p2[0].i == 2);
    REQUIRE(p3[0].i == 3);
    REQUIRE(p4[0].i == 4);
    REQUIRE(p5[0].i == 5);
    REQUIRE(p6[0].i == 6);
}