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

//struct StringLiteral {
//    string_view text;
//};

//struct Type;
//
//struct PropertySignature {
//    string_view name;
////    std::reference_wrapper<Type> type;
//};
//
//struct ObjectLiteral {
//    string_view name;
//    std::array<shared<Type>, 4> types;
////    std::vector<Type> types;
//};
//
//struct Type {
//    unsigned int ip;
//    TypeKind type = TypeKind::Unknown;
//
//    union T {
//        struct StringLiteral stringLiteral;
//        struct PropertySignature propertySignature;
////        struct ObjectLiteral objectLiteral;
//    };
//
//    T v;
//};

template<class T>
struct Pool {
    std::array<T, 300> pool;
    unsigned int i{};

    void clear() {
        i = 0;
    }

    T *allocate() {
        return &pool[i++];
    }

//    std::vector<T> pool;
//    Pool () {
//        pool.reserve(300);
//    }
//    T *make() {
//        return &pool.emplace_back();
//    }

//    shared<T> make() {
//        return std::make_shared<T>();
//    }
};
//
//struct TypeMemoryPool {
//    Pool<TypeNever> never;
//    Pool<TypeAny> any;
//    Pool<TypeLiteral> literal;
//    Pool<TypeObjectLiteral> objectLiteral;
//    Pool<TypePropertySignature> propertySignature;
//
//    void clear() {
//        never.clear();
//        any.clear();
//        literal.clear();
//        objectLiteral.clear();
//        propertySignature.clear();
//    }
//};

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

//TEST(vm2, reinterpret) {
//    auto base = std::make_shared<Type>();
////    printKind(base);
//    printKind(std::reinterpret_pointer_cast<TypeNever>(base));
//    auto never = std::make_shared<TypeNever>();
//    printKind(never);
////    printKind(std::reinterpret_pointer_cast<TypeBase>(never));
//}

TEST(vm2, vm2Base1) {
    string code = R"(
const v1: string = "abc";
const v2: number = 123;
    )";
    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", "");
    run(module);
    EXPECT_EQ(module->errors.size(), 0);

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
    ts::vm2::gcFlush();
    //only v1, v2, v3 cached value should live
    EXPECT_EQ(ts::vm2::pool.active, 3);

    ts::bench("first", 1000, [&] {
        ts::vm2::clear(module);
//        module->clear();
        run(module);
    });
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
    ts::vm2::gcFlush();
    //only v1, v2, v3, plus 'yes' : 'no' subroutine cached value should live
    EXPECT_EQ(ts::vm2::pool.active, 5);

    ts::bench("first", 1000, [&] {
        module->clear();
        run(module);
    });
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
    ts::vm2::gcFlush();
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
    for (auto i = 0; i<10; i++) {
        program.pushOp(OP::StringLiteral);
        program.pushStorage("a" + to_string(i));
    }
    program.pushOp(OP::Union);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    EXPECT_EQ(module->errors.size(), 0);
    ts::vm2::gcStack();
    ts::vm2::gcFlush();
    EXPECT_EQ(ts::vm2::pool.active, 0);
}

TEST(vm2, gcTuple) {
    ts::checker::Program program;
    program.pushOp(OP::Frame);
    for (auto i = 0; i<10; i++) {
        program.pushOp(OP::String);
        program.pushOp(OP::TupleMember);
    }
    program.pushOp(OP::Tuple);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    EXPECT_EQ(module->errors.size(), 0);
    ts::vm2::gcStack();
    ts::vm2::gcFlush();
    EXPECT_EQ(ts::vm2::pool.active, 0);
}

TEST(vm2, gcObject) {
    ts::checker::Program program;
    program.pushOp(OP::Frame);
    for (auto i = 0; i<10; i++) {
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
    ts::vm2::gcStack();
    ts::vm2::gcFlush();
    EXPECT_EQ(ts::vm2::pool.active, 0);
}

TEST(vm2, vm2Base3) {
    string code = R"(
type StringToNum<T extends string, A extends 0[] = []> = `${A['length']}` extends T ? A['length'] : StringToNum<T, [...A, 0]>;
const var1: StringToNum<'999'> = 1002;
)";
    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", code);
    run(module);
    module->printErrors();

    EXPECT_EQ(module->errors.size(), 1);

    ts::bench("first", 1000, [&] {
        module->clear();
        run(module);
    });
}

TEST(vm2, bigUnion) {
    ts::checker::Program program;
    program.pushOp(OP::Frame);
    for (auto i = 0; i<300; i++) {
        program.pushOp(OP::StringLiteral);
        program.pushStorage("a" + to_string(i));
    }
    program.pushOp(OP::Union);

    program.pushOp(OP::Frame);
    for (auto i = 0; i<300; i++) {
        program.pushOp(OP::Frame);
        program.pushOp(OP::StringLiteral);
        program.pushStorage("a");
        program.pushOp(OP::StringLiteral);
        program.pushStorage("foo1");
        program.pushOp(OP::PropertySignature);
        program.pushOp(OP::ObjectLiteral);
        program.pushOp(OP::TupleMember);
    }
    program.pushOp(OP::Tuple);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    EXPECT_EQ(module->errors.size(), 0);

    ts::vm2::clear(module);
    ts::vm2::gcStack();
    ts::vm2::gcFlush();
    EXPECT_EQ(ts::vm2::pool.active, 0);

    ts::bench("first", 1000, [&] {
        module->clear();
//        ts::vm2::clear(module);
        run(module);
//        EXPECT_EQ(process(ops), 300 * 6);
    });
}
