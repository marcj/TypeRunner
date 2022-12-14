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

void run(const string &bytecode, const string &code, const string &fileName) {
    ZoneScoped;
    auto module = std::make_shared<vm2::Module>(bytecode, fileName, code);
    bench(1, [&]{
        vm2::run(module);
        module->printErrors();
    });
}

void compileAndRun(const string &code, const string &file, const string &fileName) {
    ZoneScoped;
    auto bytecodePath = file + ".tsb";
    auto buffer = fileRead(file);
    checker::Compiler compiler;
    Parser parser;
    auto result = parser.parseSourceFile(file, buffer, types::ScriptTarget::Latest, false, ScriptKind::TS, {});
    auto program = compiler.compileSourceFile(result);
    auto bin = program.build();
    fileWrite(bytecodePath, bin);
    std::filesystem::last_write_time(bytecodePath, std::filesystem::last_write_time(file));
    checker::printBin(bin);
    auto module = make_shared<vm2::Module>(bin, fileName, code);
    vm2::run(module);
    module->printErrors();
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
    auto bytecode = file + ".tsb";
    auto relative = std::filesystem::relative(file, cwd);

    if (fileExists(bytecode) && std::filesystem::last_write_time(bytecode) == std::filesystem::last_write_time(file)) {
        run(fileRead(bytecode), code, relative.string());
    } else {
        compileAndRun(code, file, relative.string());
    }
    return 0;
}
