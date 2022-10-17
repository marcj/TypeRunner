#include <gtest/gtest.h>

#include <chrono>
#include "../parser2.h"
#include <unistd.h>

using namespace tr;

TEST(parser, single) {
    Parser parser;

    auto code = "const i = 123;";
    /**
ConstKeyword
WhitespaceTrivia
Identifier
WhitespaceTrivia
EqualsToken
WhitespaceTrivia
NumericLiteral
SemicolonToken
EndOfFileToken
     */

    auto result = parser.parseSourceFile("app.ts", code, tr::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
    debug("done");
}

TEST(parser, bench) {
    Parser parser;
    string code;

    for (int i = 0; i <100; i++) {
        code += string("const i").append(to_string(i)).append(" = 123;");
    }

    usleep(100'000);

    bench(1, [&]{
        auto result = parser.parseSourceFile("app.ts", code, tr::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
//        auto sourceFile = parser.createSourceFile("app.ts", tr::types::ScriptTarget::Latest, ScriptKind::TS, false, make_shared<NodeArray>(), make_shared<EndOfFileToken>(), 0, [](auto s) {});
    });
    fmt::print("parse {} bytes ", code.size());

    usleep(100'000);
}

TEST(parser, bench2) {
    Parser parser;
    string code = R"(
type Person = { name: string, age: number }

type Student = { name: string, age: number, gpa: number }

type FunctionContravarianceTest = (student: Student) => boolean

const contravariance: FunctionContravarianceTest = (person: Person) => true

type MyFn = (name: string) => { name: string}

const woops: MyFn = (name: number) => ({ name })

type Union = string number boolean

const arr: Union[] = ["hello", 420, false, (}]

const fibonacci = (n: number): number => n <= 1 ? n : fibonacci(n - 1) + fibonacci(n - 2)

type RedundantBigAssUnion = string | number string| number string number string| number| string number boolean boolean number

const thisWorks: RedundantBigAssUnion[] = ["hello", 123]
)";

    usleep(100'000);

    bench(1, [&]{
        auto result = parser.parseSourceFile("app.ts", code, tr::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
//        auto sourceFile = parser.createSourceFile("app.ts", tr::types::ScriptTarget::Latest, ScriptKind::TS, false, make_shared<NodeArray>(), make_shared<EndOfFileToken>(), 0, [](auto s) {});
    });
    fmt::print("parse {} bytes ", code.size());

    usleep(100'000);
}