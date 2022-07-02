#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <array>

#include "../core.h"
#include "../hash.h"
#include "../checker/compiler.h"
#include "../checker/vm2.h"
#include "./utils.h"

using namespace ts;
using namespace ts::vm2;

using std::string;
using std::string_view;

TEST(bench, size) {
    std::array<Type, 1> a1;
    std::array<Type, 2> a2;
    debug("std::vector<int> = {}", sizeof(std::vector<int>));
    debug("std::array<int, 1> = {}", sizeof(std::array<int, 1>));
    debug("std::array<int, 4> = {}", sizeof(std::array<int, 4>));
    debug("TypeBase = {}", sizeof(Type));
//    debug("TypeTuple = {}", sizeof(TypeTuple));
//    debug("TypeTupleMember = {}", sizeof(TypeTupleMember));
    debug("std::vector<TypeBase> = {}", sizeof(std::vector<Type>));
    debug("std::array<TypeBase, 1> = {}", sizeof(std::array<Type, 1>));
    debug("std::array<TypeBase, 4> = {}", sizeof(std::array<Type, 4>));
}

TEST(vm2, vm2Base1) {
    string code = R"(
const v1: string = "abc";
const v2: number = 123;
    )";
    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", "");
    run(module);
    EXPECT_EQ(module->errors.size(), 0);
    ts::vm2::gcStackAndFlush();
    //only v1, v2
    EXPECT_EQ(ts::vm2::pool.active, 2);

    ts::bench("first", 1000, [&] {
        module->clear();
        run(module);
    });
}

TEST(vm2, allocator) {
}

TEST(vm2, vm2Union) {
    string code = R"(
type a<T> = T | (string | number);
const v1: a<true> = 'yes';
const v2: a<true> = true;
const v3: a<true> = false;
)";
    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", code);
    run(module);
    module->printErrors();

    EXPECT_EQ(module->errors.size(), 1);
    ts::vm2::gcStackAndFlush();
    //only v1, v2, v3 plus for each 4 union (true | string | number)
    EXPECT_EQ(ts::vm2::pool.active, 3 * 4);

    testBench(code, 1);
}

TEST(vm2, vm2Base2) {
    string code = R"(
type a<T> = T extends string ? 'yes' : 'no';
const v1: a<number> = 'no';
const v2: a<string> = 'yes';
const v3: a<string> = 'nope';
)";
    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", code);
    run(module);
    module->printErrors();

    EXPECT_EQ(module->errors.size(), 1);
    //only v1, v2, v3, plus 'yes' : 'no' subroutine cached value should live
    ts::vm2::gcStackAndFlush();
    EXPECT_EQ(ts::vm2::pool.active, 5);

    testBench(code, 1);
}

TEST(vm2, vm2Base22) {
    string code = R"(
type a<K, T> = K | (T extends string ? 'yes': 'no');
const v1: a<true, number> = 'no';
const v2: a<true, string> = 'yes';
const v3: a<true, string> = true;
const v4: a<true, string|number> = 'yes';
const v5: a<true, string> = 'nope';
)";
    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", code);
    run(module);
    module->printErrors();

    EXPECT_EQ(module->errors.size(), 1);

    ts::bench("first", 1000, [&] {
        ts::vm2::clear(module);
//        module->clear();
        run(module);
    });
}

TEST(vm2, gc) {
    // The idea of garbage collection is this: Each type has `users` counter.
    // As soon as another type wants to hold on it, it increases `users`.
    // This happens automatically once a user pop() from the stack. The stack itself
    // is not an owner. If the user who pop()'d does not want to keep it, a drop(type) is necessary.
    // This schedules it for garbage collection.
    // If a type was received from somewhere else than the stack, the counter needs to be increased
    // manually by using use(type).
    // Subroutines like here `var1` keep the type for caching, hence this example has one active type
    // and after clearing the module, all its subroutines reset their cache, so that is not no active type anymore.
    string code = R"(
type b<T> = T;
type a<T> = 'a' extends b<T> ? T : never;
const var1: a<string> = false;
)";

    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", code);
    run(module);
    module->printErrors();
    EXPECT_EQ(module->errors.size(), 1);

    ts::vm2::gcFlush();
    //only var1 cached value should live
    EXPECT_EQ(ts::vm2::pool.active, 1);

    ts::vm2::clear(module);
    ts::vm2::gcStackAndFlush();
    EXPECT_EQ(ts::vm2::pool.active, 0);

    ts::bench("first", 1000, [&] {
//        ts::vm2::clear(module);
        module->clear();
        //no vm2::clear necessary sine run() resets the type pool anyways
        run(module);
    });
}

TEST(vm2, gcUnion) {
    ts::checker::Program program;
    program.pushOp(OP::Frame);
    for (auto i = 0; i < 10; i++) {
        program.pushOp(OP::StringLiteral);
        program.pushStorage("a" + to_string(i));
    }
    program.pushOp(OP::Union);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    EXPECT_EQ(module->errors.size(), 0);
    ts::vm2::gcStackAndFlush();
    EXPECT_EQ(ts::vm2::pool.active, 0);
}

TEST(vm2, gcTuple) {
    ts::checker::Program program;
    program.pushOp(OP::Frame);
    for (auto i = 0; i < 10; i++) {
        program.pushOp(OP::String);
        program.pushOp(OP::TupleMember);
    }
    program.pushOp(OP::Tuple);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    EXPECT_EQ(module->errors.size(), 0);
    ts::vm2::gcStackAndFlush();
    EXPECT_EQ(ts::vm2::pool.active, 0);
}

TEST(vm2, gcObject) {
    ts::checker::Program program;
    program.pushOp(OP::Frame);
    for (auto i = 0; i < 10; i++) {
        program.pushOp(OP::StringLiteral);
        program.pushStorage("a");
        program.pushOp(OP::StringLiteral);
        program.pushStorage("foo1");
        program.pushOp(OP::PropertySignature);
    }
    program.pushOp(OP::ObjectLiteral);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    EXPECT_EQ(module->errors.size(), 0);
    ts::vm2::gcStackAndFlush();
    EXPECT_EQ(ts::vm2::pool.active, 0);
}

TEST(vm2, vm2TemplateLiteral1) {
    string code = R"(
type L = `${string}`;
const var1: L = 'abc';
const var2: L = 22;
)";
    ts::testBench(code, 1);
}

TEST(vm2, vm2TemplateLiteral2) {
    string code = R"(
type L = `${34}`;
const var1: L = '34';
const var2: L = 34;
)";
    ts::testBench(code, 1);
}

TEST(vm2, vm2TemplateLiteral3) {
    string code = R"(
type L = `a${string}`;
const var1: L = 'abc';
const var2: L = 'bbc';
)";
    ts::testBench(code, 1);
}

TEST(vm2, vm2TemplateLiteralSize) {
    string code = R"(
type A = [1];
type L = `${A['length']}`;
const var1: L = "1";
const var2: L = "10";
)";
    ts::testBench(code, 1);
}

TEST(vm2, vm2TupleMerge) {
    string code = R"(
type A = [1, 2];
type L = [...A, 3];
const var1: L = [1, 2, 3];
const var2: L = [1, 2]; // Error
const var3: A = [1, 2];
)";
    test(code, 1);
    debug("active {}", ts::vm2::pool.active);

    ts::testBench(code, 1);
}

TEST(vm2, vm2Tuple2) {
    string code = R"(
type A = [1, 2];
const var1: A = [1, 2];
)";
    test(code, 0);
    ts::vm2::gcFlush();
    EXPECT_EQ(ts::vm2::pool.active, 1 + (2 * 2));
}

TEST(vm2, vm2Tuple3) {
    string code = R"(
type T = [1];
type A = [...T, 2];
const var1: A = [1, 2];
)";
    test(code, 0);
    ts::vm2::gcFlush();
    EXPECT_EQ(ts::vm2::pool.active, (1 + 2) + (1 + (2 * 2)));
}

TEST(vm2, vm2Fn1) {
    string code = R"(
type F<T> = T;
const var1: F<string> = 'abc';
)";
    test(code, 0);
    EXPECT_EQ(ts::vm2::pool.active, 2);
    ts::vm2::gcFlush();
    EXPECT_EQ(ts::vm2::pool.active, 1);
}

TEST(vm2, vm2Fn2) {
    string code = R"(
type F<T extends any> = T;
const var1: F<string> = 'abc';
)";
    //todo extends not added yet
    test(code, 0);
    EXPECT_EQ(ts::vm2::pool.active, 3);
    ts::vm2::gcFlush();
    EXPECT_EQ(ts::vm2::pool.active, 1);
}

TEST(vm2, vm2Fn3) {
    string code = R"(
type F<T = string> = T;
const var1: F = 'abc';
)";
    test(code, 0);
    EXPECT_EQ(ts::vm2::pool.active, 2);
    ts::vm2::gcFlush();
    EXPECT_EQ(ts::vm2::pool.active, 1);
}

TEST(vm2, vm2Cartesian) {
    {
        vm2::CartesianProduct cartesian;
        //`${'a'}${'b'}` => StringLiteral|StringLiteral
        cartesian.add(vm2::allocate(TypeKind::Literal)->setLiteral(TypeFlag::StringLiteral, "a"));
        cartesian.add(vm2::allocate(TypeKind::Literal)->setLiteral(TypeFlag::StringLiteral, "b"));
        auto product = cartesian.calculate();
        EXPECT_EQ(product.size(), 1);
        auto first = product[0];
        EXPECT_EQ(first.size(), 2);
        EXPECT_EQ(stringify(first[0]), "\"a\"");
        EXPECT_EQ(stringify(first[1]), "\"b\"");
    }
    {
        vm2::CartesianProduct cartesian;
        //`${'a'}${'b'|'c'}` => ('a'|'b')|('a'|'c')
        cartesian.add(vm2::allocate(TypeKind::Literal)->setLiteral(TypeFlag::StringLiteral, "a"));
        auto unionType = allocate(TypeKind::Union);
        unionType->appendChild(useAsRef(vm2::allocate(TypeKind::Literal)->setLiteral(TypeFlag::StringLiteral, "b")));
        unionType->appendChild(useAsRef(vm2::allocate(TypeKind::Literal)->setLiteral(TypeFlag::StringLiteral, "c")));
        cartesian.add(unionType);
        auto product = cartesian.calculate();
        EXPECT_EQ(product.size(), 2);
        auto first = product[0];
        EXPECT_EQ(first.size(), 2);
        EXPECT_EQ(stringify(first[0]), "\"a\"");
        EXPECT_EQ(stringify(first[1]), "\"b\"");

        auto second = product[1];
        EXPECT_EQ(second.size(), 2);
        EXPECT_EQ(stringify(second[0]), "\"a\"");
        EXPECT_EQ(stringify(second[1]), "\"c\"");
    }
}

TEST(vm2, vm2Complex1) {
    //todo: this breaks when T is 4, also memory usage is way too big
    string code = R"(
type StringToNum<T, A> = `${A['length']}` extends T ? A['length'] : StringToNum<T, [...A, 0]>;
const var1: StringToNum<'1', []> = 1;
//const var2: StringToNum<'999'> = 1002;
)";
    test(code, 0);
    debug("active {}", ts::vm2::pool.active);
    ts::vm2::gcFlush();
    debug("active gc {}", ts::vm2::pool.active);
//    EXPECT_EQ(ts::vm2::pool.active, (1 + 2) + (1 + (2 * 2)));

//    ts::bench("first", 1000, [&] {
//        module->clear();
//        run(module);
//    });
}

TEST(vm2, bigUnion) {
    ts::checker::Program program;

    program.pushOp(OP::Frame);
    for (auto i = 0; i < 300; i++) {
        program.pushOp(OP::Frame);
        program.pushOp(OP::StringLiteral);
        program.pushStorage((new string("foo"))->append(to_string(i)));
        program.pushOp(OP::StringLiteral);
        program.pushStorage("a");
        program.pushOp(OP::PropertySignature);
        program.pushOp(OP::ObjectLiteral);
        program.pushOp(OP::TupleMember);
    }
    program.pushOp(OP::Tuple);

    program.pushOp(OP::Frame);
    program.pushOp(OP::Frame);
    for (auto i = 0; i < 300; i++) {
        program.pushOp(OP::StringLiteral);
        program.pushStorage((new string("foo"))->append(to_string(i)));
    }
    program.pushOp(OP::Union);
    program.pushOp(OP::StringLiteral);
    program.pushStorage("a");
    program.pushOp(OP::PropertySignature);
    program.pushOp(OP::ObjectLiteral);
    program.pushOp(OP::Array);

    program.pushOp(OP::Assign);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    module->printErrors();
    EXPECT_EQ(module->errors.size(), 0);

    ts::vm2::clear(module);
    ts::vm2::gcStackAndFlush();
    EXPECT_EQ(ts::vm2::pool.active, 0);

    ts::bench("first", 1000, [&] {
        module->clear();
//        ts::vm2::clear(module);
        run(module);
//        EXPECT_EQ(process(ops), 300 * 6);
    });
}
