#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <memory>

#include "../core.h"
#include "../hash.h"
#include "../checker/compiler.h"
#include "../checker/vm2.h"
#include "./utils.h"

using namespace ts;
using namespace ts::vm2;

using std::string;
using std::string_view;

//TEST_CASE("bench", "size") {
//    std::array<Type, 1> a1;
//    std::array<Type, 2> a2;
//    debug("std::vector<int> = {}", sizeof(std::vector<int>));
//    debug("std::array<int, 1> = {}", sizeof(std::array<int, 1>));
//    debug("std::array<int, 4> = {}", sizeof(std::array<int, 4>));
//    debug("TypeBase = {}", sizeof(Type));
////    debug("TypeTuple = {}", sizeof(TypeTuple));
////    debug("TypeTupleMember = {}", sizeof(TypeTupleMember));
//    debug("std::vector<TypeBase> = {}", sizeof(std::vector<Type>));
//    debug("std::array<TypeBase, 1> = {}", sizeof(std::array<Type, 1>));
//    debug("std::array<TypeBase, 4> = {}", sizeof(std::array<Type, 4>));
//}

TEST_CASE("vm2Base1") {
    string code = R"(
const v1: string = "abc";
const v2: number = 123;
    )";
    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", "");
    run(module);
    REQUIRE(module->errors.size() == 0);
    ts::vm2::gcStackAndFlush();
    //only v1, v2
    REQUIRE(ts::vm2::pool.active == 2);

    testBench(code, 0);
}

TEST_CASE("vm2TwoTests") {
    test(R"(
const v1: string = "abc";
const v2: number = 123;
    )", 0);

    test(R"(
const v1: string = "aa";
const v2: number = 44;
    )", 0);
}

TEST_CASE("vm2Union") {
    string code = R"(
type a<T> = T | (string | number);
const v1: a<true> = 'yes';
const v2: a<true> = true;
const v3: a<true> = false;
)";
    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", code);
    run(module);
    module->printErrors();

    REQUIRE(module->errors.size() == 1);
    ts::vm2::gcStackAndFlush();
    //only v1, v2, v3 plus for each 4 union (true | string | number)
    REQUIRE(ts::vm2::pool.active == 3 * 4);

    testBench(code, 1);
}

TEST_CASE("vm2Base2") {
    string code = R"(
type a<T> = T extends string ? 'yes' : 'no';
const v1: a<number> = 'no';
const v2: a<string> = 'yes';
const v3: a<string> = 'nope';
)";
    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", code);
    run(module);
    module->printErrors();

    REQUIRE(module->errors.size() == 1);
    //only v1, v2, v3 subroutine cached value should live
    ts::vm2::gcStackAndFlush();
    REQUIRE(ts::vm2::pool.active == 3);

    testBench(code, 1);
}

TEST_CASE("vm2Base22") {
    string code = R"(
type a<K, T> = K | (T extends string ? 'yes' : 'no');
const v1: a<true, number> = 'no';
const v2: a<true, string> = 'yes';
const v3: a<true, string> = true;
const v4: a<true, string|number> = 'yes';
const v5: a<true, string> = 'nope';
)";
    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", code);
    run(module);
    module->printErrors();

    test(code, 1);

    testBench(code, 1);
}

TEST_CASE("gc") {
    // The idea of garbage collection is this: Each type has refCount.
    // As soon as another type wants to hold on it, it increases refCount.
    // This happens automatically once a user pop() from the stack. The stack itself
    // is not an owner. If the user who pop()'d does not want to keep it, a drop(type) is necessary.
    // This schedules it for garbage collection.
    // If a type was received from somewhere else than the stack, the counter needs to be increased
    // manually by using use(type).
    // Subroutines like here `var1` keep the type for caching, hence this example has one active type
    // and after clearing the module, all its subroutines reset their cache, so that is not no active type anymore.
    string code = R"(
type b<T> = T;
type a<T> = b<T>;
const var1: a<string> = false;
)";

    auto module = std::make_shared<vm2::Module>(ts::compile(code), "app.ts", code);
    run(module);
    module->printErrors();
    REQUIRE(module->errors.size() == 1);

    ts::vm2::gcFlush();
    //only var1 cached value should live
    //todo: currently subroutines do not cache results because of TailCall. Is this good?
    REQUIRE(ts::vm2::pool.active == 1);

    ts::vm2::clear(module);
    ts::vm2::gcStackAndFlush();
    REQUIRE(ts::vm2::pool.active == 0);

    ts::bench("first", 1000, [&] {
//        ts::vm2::clear(module);
        module->clear();
        //no vm2::clear necessary sine run() resets the type pool anyways
        run(module);
    });
}

TEST_CASE("gcUnion") {
    ts::checker::Program program;
    for (auto i = 0; i<10; i++) {
        program.pushOp(OP::StringLiteral);
        program.pushStorage("a" + to_string(i));
    }
    program.pushOp(OP::Union);
    program.pushUint16(10);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    REQUIRE(module->errors.size() == 0);
    ts::vm2::gcStackAndFlush();
    REQUIRE(ts::vm2::pool.active == 0);
}

TEST_CASE("gcTuple") {
    ts::checker::Program program;
    program.pushOp(OP::Frame);
    for (auto i = 0; i<10; i++) {
        program.pushOp(OP::String);
        program.pushOp(OP::TupleMember);
    }
    program.pushOp(OP::Tuple);
    program.pushUint16(10);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    REQUIRE(module->errors.size() == 0);
    ts::vm2::gcStackAndFlush();
    REQUIRE(ts::vm2::pool.active == 0);
}

TEST_CASE("gcObject") {
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
    program.pushUint16(10);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    REQUIRE(module->errors.size() == 0);
    ts::vm2::gcStackAndFlush();
    REQUIRE(ts::vm2::pool.active == 0);
}

TEST_CASE("vm2TemplateLiteral1") {
    string code = R"(
type L = `${string}`;
const var1: L = 'abc';
const var2: L = 22;
)";
    ts::testBench(code, 1);
}

TEST_CASE("vm2TemplateLiteral2") {
    string code = R"(
type L = `${34}`;
const var1: L = '34';
const var2: L = 34;
)";
    ts::testBench(code, 1);
}

TEST_CASE("vm2TemplateLiteral3") {
    string code = R"(
type L = `a${string}`;
const var1: L = 'abc';
const var2: L = 'bbc';
)";
    //not implemented yet
    ts::testBench(code, 1);
}

TEST_CASE("vm2TemplateLiteralSize") {
    string code = R"(
type A = [1];
type L = `${A['length']}`;
const var1: L = "1";
const var2: L = "10";
)";
    ts::testBench(code, 1);
}

TEST_CASE("vm2TemplateLiteralSizeGc") {
    string code = R"(
type A = [1];
type L = `${A['length']}`;
const var1: L = "1";
)";
    ts::test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == 4); //A|var1 (literal+tupleMember+tuple) + L (literal)
}

TEST_CASE("vm2TupleMerge") {
    string code = R"(
type A = [1, 2];
type L = [...A, 3];
const var1: L = [1, 2, 3];
const var2: L = [1, 2]; // Error
const var3: A = [1, 2];
)";
    test(code, 1);
    debug("active {}", ts::vm2::pool.active);
    ts::test(code, 1);
}

TEST_CASE("vm2Tuple2") {
    string code = R"(
type A = [1, 2];
const var1: A = [1, 2];
)";
    test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == 1 + (2 + 2));
}

TEST_CASE("vm2Tuple3") {
    string code = R"(
type T = [1];
type A = [...T, 2];
const var1: A = [1, 2];
)";
    auto module = test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == (1 + 2) + (1 + 2)); //[1], [1, 2], where "1" in second tuple is shared with first "1" in [1]
}

TEST_CASE("vm2Tuple30") {
    string code = R"(
type T = 1;
type A = [T, 2];
const var1: A = [1, 2];
)";
    auto module = test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == (1 + 2 + 2)); //$1, [$1, 2]
}

TEST_CASE("vm2Tuple31") {
    string code = R"(
type T = [1];
type A<B> = [...B, 2];
const var1: A<T> = [1, 2];
)";
    test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == (1 + 2) + (1 + 2)); //[1], [1, 2], where "1" in second tuple is shared with first "1" in [1]
}

TEST_CASE("vm2Tuple32") {
    string code = R"(
type A<B> = [...B, 2];
const var1: A<[1]> = [1, 2];
)";
    test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == (1 + 2 + 2)); //[1, 2]
}

TEST_CASE("vm2Tuple33") {
    string code = R"(
type A<B> = [...B, 2];
const var1: A<[]> = [2];
)";
    test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == (1 + 2)); // [2]
}

TEST_CASE("vm2Fn1") {
    string code = R"(
type F<T> = T;
const var1: F<string> = 'abc';
)";
    test(code, 0);
    //REQUIRE(ts::vm2::pool.active == 2);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == 1);
}

TEST_CASE("vm2Fn2") {
    string code = R"(
type F<T extends any> = T;
const var1: F<string> = 'abc';
)";
    //todo extends not added yet
    test(code, 0);
    //REQUIRE(ts::vm2::pool.active == 3);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == 1);
}

TEST_CASE("vm2Fn3") {
    string code = R"(
type F<T = string> = T;
const var1: F = 'abc';
)";
    test(code, 0);
    //REQUIRE(ts::vm2::pool.active == 2);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == 1);
}

TEST_CASE("vm2Fn4") {
    string code = R"(
type F1 = [0];
const var1: F1 = [0];
)";
    test(code, 0);
    ts::vm2::gcStackAndFlush();
    REQUIRE(ts::vm2::pool.active == 3); //[0]
}

TEST_CASE("vm2Fn4_1") {
    string code = R"(
type F1<T> = [0];
const var1: F1<false> = [0];
)";
    test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == 3); //[0]
}

TEST_CASE("vm2Fn4_2") {
    string code = R"(
type F1<T> = [...T, 0];
const var1: F1<[]> = [0];
)";
    test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == 3); //[0]
}

TEST_CASE("vm2Fn5") {
    //todo: make sure that temporary parameter are not GC inside the callee.
    // currently `if (T->users == 1) {` is not true since T gets GC in ::Extends before [...T, 0] is reached
    // the idea is to flag parameter [] as input, so that it doesn't get GC. We can't increase 'users' since that would mean
    // we have no indicator whether it can be simply stolen in [...T].
    // question is when is <[]> GC? => always the stack that creates the objects should clear it. So when a Call is done, a Return cleans it up for it after switching to the caller.
    string code = R"(
type F1<T> = T extends any ? [...T, 0] : never;
const var1: F1<[]> = [0];
)";
    test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == 3); //[0]
}

TEST_CASE("vm2Fn7") {
    string code = R"(
type F1<T> = [...T, 0];
type T = [];
const var1: F1<T> = [0];
const var2: T = [];
)";
    test(code, 0);
    ts::vm2::gcFlush();
    //a new tuple is generated, but the same amount of active elements is active
    REQUIRE(ts::vm2::pool.active == 1 + 3); //[] + [0]
}

TEST_CASE("vm2FnArg") {
    string code = R"(
type F1<T, K> = [...T, 0];
type F2<T> = F1<T, false>
const var1: F1<[]> = [0];
const var2: F2<[]> = [0];
)";
    test(code, 0);
    ts::vm2::gcFlush();

    //The idea is that for F1<[]> the [] is refCount=0, and for each argument in `type F1<>` the refCount is increased
    // and dropped at the end (::Return). This makes sure that [] in F1<[]> does not get stolen in F1.
    // To support stealing in tail calls, the drop (and frame cleanup) happens before the next function is called.
    REQUIRE(ts::vm2::pool.active == 3 + 3); //two tuples
}

TEST_CASE("vm2FnTailCall") {
    string code = R"(
type F1<T, K> = [...T, 0];
type F2<T> = F1<T, []>;
const var1: F1<[]> = [0];
)";
    test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == 3);
}

TEST_CASE("vm2FnTailCallConditional1") {
    string code = R"(
type F1<T1, K> = [...T1, 0];
type F2<T2> = [T2] extends [any] ? F1<T2> : never;
const var1: F2<[]> = [0];
)";
    test(code, 0);
}

TEST_CASE("vm2FnTailCallConditional2") {
    string code = R"(
type F1<T1, K> = [...T1, 0];
type F2<T2> = [T2] extends [any] ? never : F1<T2>;
const var1: F2<[]> = [0];
)";
    test(code, 1);
}

TEST_CASE("vm2FnTailCallConditional3") {
    string code = R"(
type F1<T1, K> = [...T1, 0];
type F2<T2> = [T2] extends [any] ? F1<T2> : F1<T2>;
const var1: F2<[]> = [0];
)";
    test(code, 0);
}

TEST_CASE("vm2FnTailCallConditional4") {
    string code = R"(
type F1<T1, K> = [...T1, 0];
type F2<K, T2> = T2 extends any ? F1<T2> : [];
const var1: F2<false, []> = [0];
)";
    test(code, 0);
}

TEST_CASE("vm2FnTailCallConditional5") {
    string code = R"(
type F1<T1, K> = [...T1, 0];
type F2<K, T2> = T2 extends any ? T2 extends any ? [0] : 1 : 2;
const var1: F2<false, []> = [0];
)";
    //compile(code);
    test(code, 0);
}

TEST_CASE("vm2FnTailCallConditionDeeper") {
    string code = R"(
type F1<T1, K> = [...T1, 0];
type F2<T2> = T2 extends any ? T2 extends any ? F1<T2> : 1 : 2;
const var1: F2<[]> = [0];
)";
    test(code, 0);
    //todo:
    // 1. Remove NJump, make Jump int32
    // 2. Find all tail calls.
    // 2.1 Restructure the way subroutines are built? How? => Into Sections. An sections know where they are used (ConditionalTrue, ConditionalFalse, Distributive, MappedType, MappedTypeName)
    // 2.2. Find all Call[] and check if it eventually hits a Return? (inefficient) => No

    //todo: Detect all tail calls in above
    //what to consider
    // conditional type
    // distributive conditional type
    // infer in conditional type
    // mapped type
    // tail call optimisation
    // frames
}

TEST_CASE("vm2FnTailCallCondition") {
    string code = R"(
type F1<T1, K> = [...T1, 0];
type F2<T2> = T2 extends any ? T2 extends any ? F1<T2> : 1 : 2;
const var1: F2<[]> = [0];
)";
    //todo: TailCall for all exits in JumpCondition and Distributive, even when nested `T extends x ? T extends y ? y : x : x`
    // find all Call and look either
    //  - subsequent OP is Return, so convert it to TailCall
    //  - subsequent OP is Jump that leads eventually to Return, so convert it to TailCall
    test(code, 0);
    ts::vm2::gcFlush();
    REQUIRE(ts::vm2::pool.active == 3);
}

TEST_CASE("vm2BenchOverhead") {
    ts::bench("nothing", 1000, [&] {
    });
}

TEST_CASE("vm2Complex1") {
    string code = R"(
type StringToNum<T, A> = `${A['length']}` extends T ? A['length'] : StringToNum<T, [...A, 0]>; //yes
//type StringToNum<T, A> = `${A['length']}` extends T ? A['length'] : StringToNum<T, [A, ...A, 0]>; //no, A refCount too big.
//type StringToNum<T, A> = `${A['length']}` extends T ? A['length'] : StringToNum<A, [...A, 0]>; //????, this makes it to 'use' A before the call and [...A, 0] happen
//type StringToNum<T, A> = `${A['length']}` extends T ? A['length'] : StringToNum<T, [...A, 0, A]>; //no, A used after ...A
//type StringToNum<T, A> = `${A['length']}` extends T ? A['length'] : StringToNum<T, [...A, ...A]>; //no, A used after ...A
//type StringToNum<T, A> = (`${A['length']}` extends T ? A['length'] : StringToNum<T, [...A, 0]>) | A; //no, A used after ...A
const var1: StringToNum<'999', []> = 1002;
//const var2: StringToNum<'999'> = 1002;
)";
    test(code, 1);
    for (auto i = 0; i <1000; i++) {
        auto bin = compile(code, false);
        assert(bin[439] == OP::TailCall);
    }

    testBench(code, 1);
//    debug("active {}", ts::vm2::pool.active);
//    ts::vm2::gcFlush();
//    debug("active gc {}", ts::vm2::pool.active);
//    REQUIRE(ts::vm2::pool.active == 1);

//    ts::bench("first", 10, [&] {
//        module->clear();
//        run(module);
//    });
}

TEST_CASE("vm2Cartesian") {
    {
        vm2::CartesianProduct cartesian;
        //`${'a'}${'b'}` => StringLiteral|StringLiteral
        cartesian.add(vm2::allocate(TypeKind::Literal)->setLiteral(TypeFlag::StringLiteral, "a"));
        cartesian.add(vm2::allocate(TypeKind::Literal)->setLiteral(TypeFlag::StringLiteral, "b"));
        auto product = cartesian.calculate();
        REQUIRE(product.size() == 1);
        auto first = product[0];
        REQUIRE(first.size() == 2);
        REQUIRE(stringify(first[0]) == "\"a\"");
        REQUIRE(stringify(first[1]) == "\"b\"");
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
        REQUIRE(product.size() == 2);
        auto first = product[0];
        REQUIRE(first.size() == 2);
        REQUIRE(stringify(first[0]) == "\"a\"");
        REQUIRE(stringify(first[1]) == "\"b\"");

        auto second = product[1];
        REQUIRE(second.size() == 2);
        REQUIRE(stringify(second[0]) == "\"a\"");
        REQUIRE(stringify(second[1]) == "\"c\"");
    }
}

TEST_CASE("bigUnion") {
    ts::checker::Program program;

    for (auto i = 0; i<300; i++) {
        program.pushOp(OP::StringLiteral);
        program.pushStorage((new string("foo"))->append(to_string(i)));
        program.pushOp(OP::StringLiteral);
        program.pushStorage("a");
        program.pushOp(OP::PropertySignature);
        program.pushOp(OP::ObjectLiteral);
        program.pushUint16(1);
        program.pushOp(OP::TupleMember);
    }
    program.pushOp(OP::Tuple);
    program.pushUint16(300);

    for (auto i = 0; i<300; i++) {
        program.pushOp(OP::StringLiteral);
        program.pushStorage((new string("foo"))->append(to_string(i)));
    }
    program.pushOp(OP::Union);
    program.pushUint16(300);

    program.pushOp(OP::StringLiteral);
    program.pushStorage("a");
    program.pushOp(OP::PropertySignature);
    program.pushOp(OP::ObjectLiteral);
    program.pushUint16(1);
    program.pushOp(OP::Array);

    program.pushOp(OP::Assign);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    module->printErrors();
    REQUIRE(module->errors.size() == 0);

    ts::vm2::clear(module);
    ts::vm2::gcStackAndFlush();
    REQUIRE(ts::vm2::pool.active == 0);

    ts::bench("first", 1000, [&] {
        module->clear();
//        ts::vm2::clear(module);
        run(module);
//        REQUIRE(process(ops) == 300 * 6);
    });
}
