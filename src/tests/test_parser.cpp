#include <gtest/gtest.h>

#include <chrono>
#include "../parser2.h"
#include <unistd.h>

using namespace ts;

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

    auto result = parser.parseSourceFile("app.ts", code, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
    debug("done");
}

TEST(parser, bench) {
    Parser parser;
    string code;

    for (int i = 0; i <20000; i++) {
        code += string("const i").append(to_string(i)).append(" = 123;");
    }

    usleep(100'000);

    auto start = std::chrono::high_resolution_clock::now();

    auto i = 0;
//    for (i = 0; i <10000; i++) {
        auto result = parser.parseSourceFile("app.ts", code, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
//        auto sourceFile = parser.createSourceFile("app.ts", ts::types::ScriptTarget::Latest, ScriptKind::TS, false, make_shared<NodeArray>(), make_shared<EndOfFileToken>(), 0, [](auto s) {});
//    }
    std::chrono::duration<double, std::milli> took = std::chrono::high_resolution_clock::now() - start;
    fmt::print("parse {} bytes took {}ms", code.size(), took.count());

    usleep(100'000);
}