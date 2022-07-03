#include "../parser2.h"
#include "../checker/compiler.h"
#include "../checker/debug.h"
#include "../checker/vm2.h"

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

    shared<vm2::Module> test(string code, unsigned int expectedErrors = 0) {
        auto bin = compile(code);
        auto module = make_shared<vm2::Module>(bin, "app.ts", code);
        vm2::run(module);
        module->printErrors();
        EXPECT_EQ(expectedErrors, module->errors.size());
        return module;
    }

    void testBench(string code, unsigned int expectedErrors = 0, int iterations = 1000) {
        auto bin = compile(code);
        auto module = make_shared<vm2::Module>(bin, "app.ts", code);
        vm2::run(module);
        module->printErrors();
        EXPECT_EQ(expectedErrors, module->errors.size());
        if (expectedErrors != module->errors.size()) return;

        auto warmTime = benchRun(iterations, [&module] {
            module->clear();
            vm2::run(module);
        });

        auto compileTime = benchRun(iterations, [&code] {
            compile(code, false);
        });

        auto coldTime = benchRun(iterations, [&code] {
            auto module = make_shared<vm2::Module>(compile(code, false), "app.ts", code);
            vm2::run(module);
        });

        fmt::print("{} iterations (it): compile {:.9f}ms/it, cold {:.9f}ms/it, warm {:.9f}ms/it", iterations, compileTime.count() / iterations, coldTime.count() / iterations, warmTime.count() / iterations);
    }

}