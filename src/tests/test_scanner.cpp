#include <gtest/gtest.h>

#include <iostream>
#include "../scanner.h"

#include "magic_enum.hpp"

using namespace ts;
using namespace magic_enum;

TEST(scanner, basisc) {
    Scanner scanner("const i = 123;");

    std::cout << enum_name(scanner.scan()) << "\n";
    std::cout << enum_name(scanner.scan()) << "\n";
    std::cout << enum_name(scanner.scan()) << "\n";
    std::cout << enum_name(scanner.scan()) << "\n";
    std::cout << enum_name(scanner.scan()) << "\n";
    std::cout << enum_name(scanner.scan()) << "\n";
    std::cout << enum_name(scanner.scan()) << "\n";
    std::cout << enum_name(scanner.scan()) << "\n";
    std::cout << enum_name(scanner.scan()) << "\n";
}