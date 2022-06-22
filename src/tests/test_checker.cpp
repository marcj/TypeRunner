#include <gtest/gtest.h>

#include "../parser2.h"
#include "../checker/compiler.h"
#include "../checker/vm.h"
#include "../checker/debug.h"

using namespace ts;

string compile(string code, bool print = true) {
    Parser parser;
    auto result = parser.parseSourceFile("app.ts", code, ScriptTarget::Latest, false, ScriptKind::TS, {});
    checker::Compiler compiler;
    auto program = compiler.compileSourceFile(result);
    auto bin = program.build();
    if (print) checker::printBin(bin);
    return bin;
}

void test(string code, unsigned int expectedErrors = 0) {
    auto bin = compile(code);
    auto module = make_shared<vm::Module>(bin, "app.ts", code);
    vm::VM vm;
    vm.run(module);
    vm.printErrors();
    EXPECT_EQ(expectedErrors, vm.getErrors());
}

void testBench(string code, unsigned int expectedErrors = 0) {
    auto bin = compile(code);
    auto module = make_shared<vm::Module>(bin, "app.ts", code);
    vm::VM vm;
    vm.run(module);
    vm.printErrors();
    EXPECT_EQ(expectedErrors, vm.getErrors());

    auto iterations = 10;

    auto warmTime = benchRun(iterations, [&module] {
        module->clear();
        vm::VM vm;
        vm.run(module);
    });

    auto compileTime = benchRun(iterations, [&code] {
        compile(code, false);
    });

    auto coldTime = benchRun(iterations, [&code] {
        vm::VM vm;
        auto module = make_shared<vm::Module>(compile(code, false), "app.ts", code);
        vm.run(module);
    });

    fmt::print("{} iterations: compile {}/op, cold {}/op, warm {}/op", iterations, compileTime.count() / iterations, coldTime.count() / iterations, warmTime.count() / iterations);
}

TEST(checker, program) {
    checker::Program program;

    program.pushOp(instructions::OP::Call);
    program.pushAddress(1);

    program.pushOp(instructions::OP::Var);
    program.pushAddress(2);

    program.pushOp(instructions::OP::Any);
    program.pushOp(instructions::OP::Assign);
    program.pushAddress(2);

    fmt::print("bytes {}", program.ops);
}

TEST(checker, type) {
    Parser parser;

    string code = R"(
    const v1: string = "abc";
    const v2: number = 123;
    )";

    testBench(code, 0);
}

TEST(checker, typeError) {
    Parser parser;

    string code = R"(
type a<T> = T; const v2: a<number>  = "23";
    )";

    testBench(code, 1);
}

TEST(checker, typeUnion) {
    Parser parser;

    string code = R"(
    type a = number;
    type b = string | a;
    const v1: b = 'abc';
    const v2: b = 23;
    const v3: b = true;
    )";

    test(code, 1);
}

TEST(checker, typeGeneric) {
    string code = R"(
    type a<K, T> = T;
    const v1: a<true, number> = 34;
    const v2: a<true, number> = "as";
    )";

    test(code, 1);
}

TEST(checker, typeGenericUnion) {
    Parser parser;

    string code = R"(
    type a<K, T> = K | (T extends string ? 'yes' : 'no');
    const v1: a<true, number> = 'no';
    const v2: a<true, string> = 'yes';
    const v3: a<true, string> = true;
    const v4: a<true, string | number> = 'yes';
    const v5: a<true, string> = 'nope';
    )";

    test(code);
}

TEST(checker, tuple) {
    Parser parser;

    string code = R"(
    type StringToNum<T extends string, A extends 0[] = []> = `${A['length']}` extends T ? A['length'] : StringToNum<T, [...A, 0]>;
    const var1: StringToNum<'999'> = 1002;
    )";

    testBench(code, 1);
}

TEST(checker, assign) {
    Parser parser;

    auto code = R"(
        const v: number | string = '';
        //Assign to '', change to string

        v = 123; //Assign to 123, change to number
    )";

    auto result = parser.parseSourceFile("app.ts", code, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});

    checker::Compiler compiler;

    auto program = compiler.compileSourceFile(result);
    auto bin = program.build();
    checker::printBin(bin);
    debug("done");
}

TEST(checker, stackFrame) {
    Parser parser;

    auto code = R"(
    type a = string;

    type Generic<T> = a | T;

    function another() {
        type a = ReturnType<typeof print>;
    }

    function print() {
        type b = a;
        type c = Generic<string>;

        function p() {
            type d = Generic<string>;
        }
        return c;
    }
    )";

    auto result = parser.parseSourceFile("app.ts", code, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
    debug("done");
}

TEST(checker, functionCall) {
    auto code = R"(
    const i: number = 123;

    function doIt(v: number) {
    }

    doIt(i);
    doIt('123');
    )";

    test(code);
}

TEST(checker, functionGenericCall) {
    auto code = R"(
    function doIt<T extends number>(v: T) {
    }
    doIt<number>(23);
    )";

    test(code, 0);
}

TEST(checker, controlFlow1) {
    auto code = R"(
    function boolFunc(t: true) {}
    let bool = true;
    boolFunc(bool);

    bool = false;
    boolFunc(bool);

    bool = Date.now() > 1000 ? true : false;
    boolFunc(bool);
    )";

    testBench(code, 2);
}

TEST(checker, functionGenericExpressionCall) {
    auto code = R"(
    function doIt<T extends number>(v: T) {
    }

    const a = doIt<number>;
    a(23);
    )";

    testBench(code, 0);
}

TEST(checker, functionGenericCallBench) {
    //todo: implement inferring types from passed function arguments
    auto code = R"(
//    const i: number = 123;

    function doIt<T extends number>(v: T) {
    }

    doIt<number>(23);
//    doIt<number>('23');
//    doIt<34>(i);
//    doIt<string>(i);
//    doIt(i);
//    doIt('asd');
//    doIt();
    )";

    testBench(code);
}

TEST(checker, objectLiteral1) {
    auto code = R"(
type Person = {name: string, age: number}

const a: Person = {name: 'Peter', age: 52};
const b: Person = {name: 'Peter', age: '52'};
    )";

    testBench(code);
}

TEST(checker, mapLiteralToClassConditional) {
    auto code = R"(
    class A {};
    class B {};
    class C {};
    class D {};

    type Resolve<T> =
        T extends "a" ? A :
        T extends "b" ? B :
        T extends "c" ? C :
        T extends "d" ? D :
        never;

    type found = Resolve<"d">;
    )";

    testBench(code);
}
TEST(checker, mapLiteralToClassMap) {
    auto code = R"(
    class A {}
    class B {}
    class C {}
    class D {}
    type ClassMap = {
        a: A, b: B, c: C, d: D
    }

    type Resolve<T extends keyof ClassMap> = ClassMap[T];

    type found = Resolve<"d">;
    )";

    testBench(code);
}

TEST(checker, basic2) {
    Parser parser;

    auto code = R"(
//    i = 1;
    const i: number | string = 123;

    if ('number' === typeof i) {
       function print(v: string) {
            i = 3;
            const i = v;
        }
    }
    //type print = (v: string) => void;
    //const print = function(v: string) {} //both equivalent, except of the symbol scope

    i = "no";
    print(i);
    )";

    auto result = parser.parseSourceFile("app.ts", code, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
    debug("done");
}
