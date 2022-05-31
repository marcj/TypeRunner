#include <gtest/gtest.h>

#include <iostream>
#include "../scanner.h"

#include "magic_enum.hpp"

using namespace ts;
using namespace magic_enum;

TEST(scanner, basisc) {

    auto start = std::chrono::high_resolution_clock::now();
    Scanner scanner("const i = 123;");

    auto i = 0;
    for (i = 0; i <10000; i++) {
        scanner.setText("const i = 123");
//        scanner.setOnError([this](auto ...a) { scanError(a...); });
        scanner.setScriptTarget(ts::types::ScriptTarget::Latest);
        scanner.setLanguageVariant(ts::types::LanguageVariant::Standard);

        while (scanner.scan() != ts::types::SyntaxKind::EndOfFileToken) {

        }

        scanner.clearCommentDirectives();
        scanner.setText("");
        scanner.setOnError(nullopt);
    }
    std::chrono::duration<double, std::milli> took = std::chrono::high_resolution_clock::now() - start;
    debug("scan %d took %fms", i, took.count());

//    std::cout << enum_name(scanner.scan()) << "\n";
//    std::cout << enum_name(scanner.scan()) << "\n";
//    std::cout << enum_name(scanner.scan()) << "\n";
//    std::cout << enum_name(scanner.scan()) << "\n";
//    std::cout << enum_name(scanner.scan()) << "\n";
//    std::cout << enum_name(scanner.scan()) << "\n";
//    std::cout << enum_name(scanner.scan()) << "\n";
//    std::cout << enum_name(scanner.scan()) << "\n";
//    std::cout << enum_name(scanner.scan()) << "\n";
}