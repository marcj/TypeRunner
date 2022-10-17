#include <iostream>
#include <memory>
#include <unistd.h>

#include "./src/core.h"
#include "./src/fs.h"
#include "./src/parser2.h"
#include "./src/checker/vm2.h"
#include "./src/checker/module2.h"
#include "./src/checker/debug.h"
#include "./src/checker/compiler.h"

using namespace tr;

void compileAndRun(const string &code, const string &fileName) {
    ZoneScoped;
    auto iterations = 1000;
    auto cold = benchRun(iterations, [&] {
        checker::Compiler compiler;
        Parser parser;
        auto result = parser.parseSourceFile(fileName, code, types::ScriptTarget::Latest, false, ScriptKind::TS, {});
        auto program = compiler.compileSourceFile(result);
        auto bin = program.build();
        auto module = make_shared<vm2::Module>(bin, fileName, code);
        vm2::run(module);
    });

    checker::Compiler compiler;
    Parser parser;
    auto result = parser.parseSourceFile(fileName, code, types::ScriptTarget::Latest, false, ScriptKind::TS, {});
    auto program = compiler.compileSourceFile(result);
    auto bin = program.build();
    auto module = make_shared<vm2::Module>(bin, fileName, code);
    auto warm = benchRun(iterations, [&] {
        module->clear();
        vm2::run(module);
    });

    std::cout << fmt::format("typerunner: {} iterations (it): cold {:.9f}ms/it, warm {:.9f}ms/it\n", iterations, cold.count() / iterations, warm.count() / iterations);
}

int main(int argc, char *argv[]) {
    ZoneScoped;
    std::string file;
    auto cwd = std::filesystem::current_path();

    if (argc > 1) {
        file = cwd.string() + "/" + argv[1];
    } else {
        file = cwd.string() + "/../tests/basic1.ts";
    }

    if (!fileExists(file)) {
        std::cout << "File not found " << file << "\n";
        return 4;
    }
    auto code = fileRead(file);
    auto relative = std::filesystem::relative(file, cwd);
    compileAndRun(code, relative.string());
    return 0;
}
