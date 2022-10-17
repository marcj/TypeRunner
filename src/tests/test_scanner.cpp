#include <gtest/gtest.h>

#include <iostream>
#include "../scanner.h"

using namespace tr;

TEST(scanner, basisc) {

    auto start = std::chrono::high_resolution_clock::now();
    Scanner scanner("const i = 123;const i2 = 123;const i3 = 123;const i4 = 123;const i5 = 123;const i6 = 123;const i7 = 123;const i8 = 123;");

    auto i = 0;
    for (i = 0; i <10000; i++) {
        scanner.setText("const i = 123");
//        scanner.setOnError([this](auto ...a) { scanError(a...); });
        scanner.setScriptTarget(tr::types::ScriptTarget::Latest);
        scanner.setLanguageVariant(tr::types::LanguageVariant::Standard);

        while (scanner.scan() != tr::types::SyntaxKind::EndOfFileToken) {

        }

        scanner.clearCommentDirectives();
        scanner.setText("");
        scanner.setOnError(nullopt);
    }
    std::chrono::duration<double, std::milli> took = std::chrono::high_resolution_clock::now() - start;
    debug("scan {} took {}ms", i, took.count());

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