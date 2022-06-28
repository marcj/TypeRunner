#include <iostream>

#include "./src/core.h"
#include "./src/fs.h"
#include "./src/parser2.h"
#include "./src/checker/vm.h"
#include "./src/checker/debug.h"
#include "./src/checker/compiler.h"

using namespace ts;


void run(const string &bytecode, const string &code, const string &fileName) {
    vm::VM vm;
    auto module = make_shared<vm::Module>(bytecode, fileName, code);
    bench(1, [&]{
        vm.run(module);
        vm.printErrors();
    });
}

void compileAndRun(const string &code, const string &file, const string &fileName) {
    auto bytecodePath = file + ".tsb";
    auto buffer = readFile(file);
    checker::Compiler compiler;
    Parser parser;
    auto result = parser.parseSourceFile(file, buffer, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
    auto program = compiler.compileSourceFile(result);
    auto bin = program.build();
    writeFile(bytecodePath, bin);
    std::filesystem::last_write_time(bytecodePath, std::filesystem::last_write_time(file));
    vm::VM vm;
    checker::printBin(bin);
    auto module = make_shared<vm::Module>(bin, fileName, code);
    vm.run(module);
    vm.printErrors();
}

int main(int argc, char *argv[]) {
    std::string file = "/Users/marc/bude/typescript-cpp/tests/big1.ts";
    auto cwd = std::filesystem::current_path();

    if (argc > 1) {
        file = cwd.string() + "/" + argv[1];
    }

    auto code = readFile(file);
    auto bytecode = file + ".tsb";
    auto relative = std::filesystem::relative(file, cwd);

    if (exists(bytecode) && std::filesystem::last_write_time(bytecode) == std::filesystem::last_write_time(file)) {
        run(readFile(bytecode), code, relative.string());
    } else {
        compileAndRun(code, file, relative.string());
    }
    return 0;
}
