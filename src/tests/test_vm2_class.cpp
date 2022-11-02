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

TEST_CASE("class1") {
    string code = R"(
    class MyDate {
        static now(): number {
            return 0;
        }
    }
    const now: number = MyDate.now();
    const now2: string = MyDate.now();
)";
    test(code, 1);
    testBench(code, 1);
}

TEST_CASE(" class2") {
    string code = R"(
    class MyDate {
        now(): number {
            return 0;
        }
    }
    const now: number = new MyDate().now();
    const now2: string = new MyDate().now();
)";
    test(code, 1);
    testBench(code, 1);
}

TEST_CASE("class3") {
    string code = R"(
    class MyDate<T> {
        now(): T {
            return 0;
        }
    }
    const now: number = new MyDate<number>().now();
    const now2: string = new MyDate<string>().now();
    const now3: number = new MyDate<string>().now();
)";
    test(code, 1);
    testBench(code, 1);
}

//todo: requires handling Constructor, ThisKeyword, ThisType
// this['item'] relies on `item`, since we can't enforce the order in compile time, we convert all properties/methods to subroutines and call them however they are referenced,
// and detect circular dependencies in the runtime.
// How do we pass `this`? As first TypeArgument, or in a Slot?
// `this` could change, e.g. `new (MyDate & {now: () => string})`
// https://www.typescriptlang.org/play?#code/FAMwrgdgxgLglgewgAggCgJQC5VgLYBGApgE7IDewy1yJRMYJKADANzAC+wwUANgIYBnQcgCyATwAi-GEQpUaEBAHdMOCPmJlKNXbXqMW7XVwXUYCNchgALOCIBkFJauzJBMEnAgBzDvL0aOgYmazsRIWR+CHFjGlNdAHpEsPtkKCQAN1IYEVs5YMNrcQAHOQso5AATIhBSOirkJDk4GDNkAgErACUDJgAVUqIAHlt7AG0AchdJgF0APgDA-RCjdq5TZOKyyuQAXjEpGTknchccTH3Fjy9fLi2MiA9q4-3UImVD6Vl2HiRnghvCAfL7HTAAOgsmF+j2e51whFIbwI4Jc0O4sJgqBUACZ1JokQcUZ1+NCgA
TEST_CASE("class4") {
    string code = R"(
    class MyDate<T> {
        constructor(public item: T) {}
        now(): this['item'] {
            return this.item;
        }
    }
    const now: number = new MyDate<number>(123).now();
    const now2: string = new MyDate<string>('123').now();
)";
    test(code, 1);
    testBench(code, 1);
}

//todo: instance class from value arguments. does this work for functions already? -> no
//todo: infer type arguments from constructor. Maybe put the constructor signature in its own subroutine
// which we can call. Result would be that we have all type arguments
TEST_CASE("class41") {
    string code = R"(
    class MyDate<T> {
        constructor(public item: T) {}
        now(): T {
            return this.item;
        }
    }
    const now: number = new MyDate(123).now();
    const now2: string = new MyDate('123').now();
)";
    test(code, 0);
    testBench(code, 0);
}

//todo: Needs to resolve Base constructor and executes it to populate T
TEST_CASE("class42") {
    string code = R"(
    class Base<T> {
        constructor(public item: T) {}
    }

    class MyDate<T> extends Base<T> {
        now(): T {
            return this.item;
        }
    }
    const now: number = new MyDate(123).now();
    const now2: string = new MyDate('123').now();
)";
    test(code, 0);
    testBench(code, 0);
}

TEST_CASE("class5") {
    string code = R"(
    class Data<T> {
        constructor(public item: T) {}
    }

    class MyDate extends Data<string> {
        now(): this['item'] {
            return this.item;
        }
    }
    const now: number = new MyDate('123').now();
    const now2: string = new MyDate('123').now();
)";
    test(code, 1);
    testBench(code, 1);
}

TEST_CASE("class6") {
    string code = R"(
    class Data<T> {
        constructor(public item: T) {}
    }

    class MyDate extends Data<string> {
        public item: number;
        now(): this['item'] {
            return this.item;
        }
    }
    const now: number = new MyDate<number>(123).now();
    const now2: string = new MyDate<string>('123').now();
)";
    test(code, 1);
    testBench(code, 1);
}
