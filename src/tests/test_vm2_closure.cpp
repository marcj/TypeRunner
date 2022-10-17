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

TEST_CASE("closure1") {
    string code = R"(
function test<T>() {
  function b(): T {
    return {} as any;
  }

  return b;
}

const b: () => string = test<string>();
const c: string = b();
const d: number = b();
)";
    test(code, 1);
    //testBench(code, 1);
}

//todo: this breaks since we do not capture T and generate a FunctionRef since b is generic and needs to be instantiated.
// FunctionRef needs to capture T and acts as closure. That's the solution right?
TEST_CASE("closure2") {
    string code = R"(
function test<T>() {
  function b<K>(v: K): K | T {
    return {} as any;
  }

  return b;
}

const b = test<string>();
const c: string | number = b<number>(3);
const d: string = b<string>('3');
)";
    test(code, 1);
    //testBench(code, 1);
}

TEST_CASE("closure3") {
    string code = R"(
function test<T>() {
  function b<K>(k: K): K & T {
    return {} as any;
  }

  return b;
}

const b = test<string>();
)";
    test(code, 0);
    //testBench(code, 1);
}