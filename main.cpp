#include <iostream>
#include <fstream>

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

int main(int argc, char *argv[]) {
//    std::string cwd = "";
//    if (argc > 0) cwd = argv[0];
    std::string file = "/Users/marc/bude/typescript-cpp/tests/basic1.ts";
    if (argc > 1) file = argv[1];

//    cwd.append("/");
//    file = cwd + file;

    auto bytecode = file + ".tsbytecode";
    if (exists(bytecode)) {
        auto buffer = readFile(file);
        vm::VM vm;
        vm.call(buffer);
    } else {
        auto buffer = readFile(file);
        checker::Compiler compiler;
        Parser parser;
        auto result = parser.parseSourceFile(file, buffer, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
        auto program = compiler.compileSourceFile(result);
//        program.print();
        auto bin = program.build();
        writeFile(bytecode, bin);
        vm::VM vm;
        vm.call(bin);
    }
    return 0;
}
