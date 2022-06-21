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
    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
    EXPECT_EQ(expectedErrors, vm.errors.size());
}

void run(string code, unsigned int expectedErrors = 0) {
    auto bin = compile(code);
    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
    EXPECT_EQ(expectedErrors, vm.errors.size());

    auto iterations = 10;
    auto compileTime = benchRun(iterations, [&code] {
        compile(code, false);
    });

    auto coldTime = benchRun(iterations, [&code] {
        vm::VM vm;
        vm.run(compile(code, false));
    });

    auto warmTime = benchRun(iterations, [&bin] {
        vm::VM vm;
        vm.run(bin);
    });

    fmt::print("{} iterations took: compile {}, cold {}, warm {}", iterations, compileTime.count() / iterations, coldTime.count() / iterations, warmTime.count() / iterations);
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

    auto bin = compile(code);

    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
    EXPECT_EQ(vm.errors.size(), 0);

    bench(10, [&] {
        vm::VM vm;
        vm.run(bin);
    });
}

TEST(checker, type2) {
    Parser parser;

    string code = R"(
    type a = number;
    type b = string | a;
    const v1: b = 'abc';
    const v2: b = 23;
    const v3: b = true;
    )";

    auto bin = compile(code);

    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
    EXPECT_EQ(vm.errors.size(), 1);
}

TEST(checker, type31) {
    string code = R"(
    type a<K, T> = T;
    const v1: a<true, number> = 34;
    const v2: a<true, number> = "as";
    )";

    auto bin = compile(code);

    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
    EXPECT_EQ(vm.errors.size(), 1);
    bench(10, [&] {
        vm::VM vm;
        vm.run(bin);
    });
}

TEST(checker, type3) {
    Parser parser;

    string code = R"(
    type a<K, T> = K | (T extends string ? 'yes' : 'no');
    const v1: a<true, number> = 'no';
    const v2: a<true, string> = 'yes';
    const v3: a<true, string> = true;
    const v4: a<true, string | number> = 'yes';
    const v5: a<true, string> = 'nope';
    )";

    auto bin = compile(code);

    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
    EXPECT_EQ(vm.errors.size(), 1);

    bench(10, [&] {
        vm::VM vm;
        vm.run(bin);
    });
}

TEST(checker, tuple) {
    Parser parser;

    string code = R"(
//    type a<T> = [1, ...T];
//    const v1: a<[string, number]> = [1, 'abc'];

//    type a = [string, number];
//    const var1: a['length'] = 3;
//
//    type a = [string, boolean];
//    type b = [...a, number];
//    const var1: b = ['asd', 123];

//    type a<T = string> = T;
//    const var1: a<number> = 'asd';

    type StringToNum<T extends string, A extends 0[] = []> = `${A['length']}` extends T ? A['length'] : StringToNum<T, [...A, 0]>;
    const var1: StringToNum<'999'> = 1002;
    )";

    auto bin = compile(code);

    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
//    EXPECT_EQ(vm.errors.size(), 1);

    bench(10, [&] {
        vm::VM vm;
        vm.run(bin);
    });
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

    //each stack frame gets a number. The ref `a` references this stack number + symbol index.
    //when resolving in VM it searches frames upwards until correct frame was found.
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

    auto bin = compile(code);
    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
    EXPECT_EQ(vm.errors.size(), 1);

    bench(10, [&] {
        vm::VM vm;
        vm.run(bin);
    });
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

    run(code, 2);
}

TEST(checker, functionGenericExpressionCall) {
    auto code = R"(
    function doIt<T extends number>(v: T) {
    }

    const a = doIt<number>;
    a(23);
    )";

    run(code, 0);
}

TEST(checker, functionGenericCallBench) {
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

    run(code);
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

    run(code);
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

    run(code);
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
