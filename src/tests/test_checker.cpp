#include <gtest/gtest.h>

#include "../parser2.h"
#include "../checker/compiler.h"
#include "../checker/vm.h"

using namespace ts;

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
    program.print();
}

TEST(checker, type) {
    Parser parser;

    std:string code = R"(
    const v1: string = "abc";
    const v2: number = 123;
    )";

    auto result = parser.parseSourceFile("app.ts", code, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});

    checker::Compiler compiler;

    auto program = compiler.compileSourceFile(result);
    program.print();
    auto bin = program.build();
    debug("Code {} chars, to {} bytes: {}", code.size(), bin.size(), bin);

    bench(100, [&] {
        vm::VM vm;
        vm.call(bin);
    });
}

TEST(checker, type2) {
    Parser parser;

    std:string code = R"(
    type a = number;
    type b = string | a;
    const v1: b = 'abc';
    )";

    auto result = parser.parseSourceFile("app.ts", code, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});

    checker::Compiler compiler;

    auto program = compiler.compileSourceFile(result);
    program.print();
    auto bin = program.build();
    debug("Code {} chars, to {} bytes: {}", code.size(), bin.size(), bin);

    vm::VM vm;
    vm.call(bin);
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
    program.print();
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

TEST(checker, basic) {
    Parser parser;

    auto code = R"(
    const i: number = 123;

    function print(v: string) {
    }

    print(i);
    )";

    auto result = parser.parseSourceFile("app.ts", code, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
    debug("done");
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
