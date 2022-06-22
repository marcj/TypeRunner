#include <iostream>
#include <fstream>
#include <filesystem>

#include "./src/core.h"
#include "./src/parser2.h"
#include "./src/checker/compiler.h"
#include "./src/checker/vm.h"

using namespace ts;

auto readFile(const string &file) {
    std::ifstream t;
    t.open(file);
    t.seekg(0, std::ios::end);
    size_t size = t.tellg();
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(&buffer[0], size);
    return std::move(buffer);
}

void writeFile(const string &file, const string_view &content) {
    std::ofstream t;
    t.open(file);
    t << content;
}

bool exists(const string &file)
{
    std::ifstream infile(file);
    return infile.good();
}

void run(const string &bytecode, const string &code, const string &fileName) {
    vm::VM vm;
    vm.run(bytecode, code, fileName);
    vm.printErrors();
}

void compileAndRun(const string &code, const string &file, const string &fileName) {
    auto bytecode = file + ".tsbytecode";
    auto buffer = readFile(file);
    checker::Compiler compiler;
    Parser parser;
    auto result = parser.parseSourceFile(file, buffer, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
    auto program = compiler.compileSourceFile(result);
    auto bin = program.build();
    writeFile(bytecode, bin);
    vm::VM vm;
    vm.run(bin, code, fileName);
    vm.printErrors();
}

int main(int argc, char *argv[]) {
    std::string file = "/Users/marc/bude/typescript-cpp/tests/basicError1.ts";
    auto cwd = std::filesystem::current_path();

    if (argc > 1) {
        file = cwd.string() + "/" + argv[1];
    }

    auto code = readFile(file);
    auto bytecode = file + ".tsb";
    auto relative = std::filesystem::relative(file, cwd);

    if (exists(bytecode)) {
        run(readFile(bytecode), code, relative.string());
    } else {
        compileAndRun(code, file, relative.string());
    }
    return 0;
}
