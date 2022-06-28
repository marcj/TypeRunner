#include "../parser2.h"
#include "../checker/compiler.h"
#include "../checker/debug.h"

namespace ts {
    std::string compile(std::string code, bool print = true) {
        Parser parser;
        auto result = parser.parseSourceFile("app.ts", code, ScriptTarget::Latest, false, ScriptKind::TS, {});
        checker::Compiler compiler;
        auto program = compiler.compileSourceFile(result);
        auto bin = program.build();
        if (print) checker::printBin(bin);
        return bin;
    }
}