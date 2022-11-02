#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <memory>

#include "../core.h"
#include "../hash.h"
#include "../checker/compiler.h"
#include "../checker/vm2.h"
#include "./utils.h"

using namespace tr;
using namespace tr::vm2;

TEST_CASE("function1") {
    string code = R"(
    function doIt<T extends number>(v: T) {
    }
    const a = doIt<number>;
    a(23);
)";
    test(code, 0);
    testBench(code, 0);
}

TEST_CASE("function2") {
    string code = R"(
    function doIt<T extends number>(v: T) {
    }
    doIt<number>(23);
    doIt<34>(33);
)";
    test(code, 1);
    testBench(code, 1);
}

TEST_CASE("function3") {
    ZoneScoped;
    string code = R"(
    function doIt() {
        return 1;
    }
    const var1: number = doIt();
    const var2: string = doIt();
)";
    test(code, 1);
    testBench(code, 1);
}

TEST_CASE("function4") {
    string code = R"(
    function doIt(v: number) {
        if (v == 1) return 'yes';
        if (v == 2) return 'yes';
        if (v == 3) return 'yes';
        if (v == 4) return 'yes';
        if (v == 5) return 'yes';
        if (v == 6) return 'yes';
        if (v == 7) return 'yes';
        if (v == 8) return 'yes';
    if (v == 9) return 'yes';
        if (v == 10) return 'yes';
        return 1;
    }
    const var1: number | string = doIt(0);
    const var2: number = doIt(0);
)";
    test(code, 1);
    testBench(code, 1);
}

TEST_CASE("function5") {
    string code = R"(
    function doIt(): string {
        return 1;
    }
    doIt();
)";
    test(code, 1);
    testBench(code, 1);
}

//a function can have multiple definitions (overloading definitions where the last is ignored in finding a candidate).
//we need to instantiate doIt with value argument 3,
//basically converts to a tuple: [infer T extends number] so that rest parameters work out of the box.
TEST_CASE("function6") {
    string code = R"(
    function doIt<T extends number>(v: T): T {
    }
    const a: 3 = doIt(3);
    const b: number = doIt(3);
)";
    test(code, 1);
    testBench(code, 1);
}

TEST_CASE("function7") {
    string code = R"(
    function doIt<T>(v: T): T {
    }
    const a: number = doIt(3);
    const b: 3 = doIt(3);
)";
    test(code, 1);
    testBench(code, 1);
}