#include <gtest/gtest.h>
#include <memory>

#include "../core.h"
#include "../hash.h"
#include "../checker/types.h"
#include "../checker/vm.h"
#include "../checker/vm2.h"
#include "../checker/compiler.h"
#include "../checker/debug.h"

using namespace tr;
using std::string;
using std::make_shared;
using std::string_view;

struct SimpleType {
    string_view typeName;
    vm::TypeKind kind = vm::TypeKind::Never;

    //this is the OP position (instruction pointer) of the bytecode. sourcemap is necessary to map it to source positions.
    unsigned int ip{};
};

struct SimpleType2 {
    vm::TypeKind kind = vm::TypeKind::Never;
};

TEST(bench, jit) {
    vm2::jitTest();
    debug("called {}", vm2::called);

    bench("jit", 1000, [&] {
        vm2::called = 0;
        vm2::jitTest();
//        EXPECT_EQ(vm2::called, 1800);
    });

    auto fn = vm2::jitBuild();
    vector<SimpleType> vec;
    bench("jit prebuild", 1000, [&] {
        for (auto i = 0; i < 300*6; i++) {
            volatile auto a = new SimpleType();
        }
        vm2::called = 0;
//        fn();
//        EXPECT_EQ(vm2::called, 1800);
    });

    bench("300*6 calls", 1000, [&] {
        vm2::called = 0;
        for (auto i = 0; i < 300; i++) {
            vm2::calledFunc();
            vm2::calledFunc();
            vm2::calledFunc();
            vm2::calledFunc();
            vm2::calledFunc();
            vm2::calledFunc();
        }
        EXPECT_EQ(vm2::called, 1800);
    });

    bench("300*6 jump", 1000, [&] {
        vm2::called = 0;
        volatile auto i = 0; //stop optimisation
        volatile vm::TypeUnknown* t;

        inc:
            vm2::called++;
//            t = new vm::TypeUnknown;
            goto next;

        next:
            if (++i < 1800) goto inc;
            goto end;

        end:
            EXPECT_EQ(vm2::called, 1800);
    });
}

TEST(bench, vm2) {
    checker::Program program;
//    program.pushOp(instructions::OP::Frame);
    for (auto i = 0; i < 300; i++) {
        program.pushOp(instructions::OP::Noop); //frame
        program.pushOp(instructions::OP::Noop); //string
        program.pushOp(instructions::OP::Noop); //property name

        program.pushOp(instructions::OP::Noop); //propertySignature
        program.pushOp(instructions::OP::Noop); //objectLiteral
        program.pushOp(instructions::OP::Noop); //TupleMember
    }
    program.pushOp(instructions::OP::Halt);

    auto bin = program.build();
    checker::printBin(bin);

    auto module = make_shared<vm::Module>(bin, "app", "");
    bench("vm1 old, less cases", 1000, [&] {
        module->clear();
        vm::VM vm;
        vm.run(module);
    });
    bench("vm2 direct threaded", 1000, [&] {
        vm2::called = 0;
        module->clear();
        vm2::run(module);
        EXPECT_EQ(vm2::called, 1800);
    });

    bench("vm2 naive switch-case", 1000, [&] {
        vm2::called = 0;
        module->clear();
        vm2::naive(module);
//        EXPECT_EQ(vm2::called, 1800);
    });
}

TEST(bench, vm3) {
    auto jump = [] {

        //https://christopherschwaab.wordpress.com/2018/04/13/generating-a-threaded-arm-interpreter-with-templates/

        // We're using the GNU C, labels-as-values extension here.
        void *prog[] = {&&PUSH,(void*)6,
        &&PUSH,(void*)7,
        &&MUL, &&HALT};
//        vector<void*> program;
//        void **prog = prog;

        void **vPC = prog;
        void *stack[4], **sp = stack;

        goto **vPC++;
        PUSH:  *sp++ = *vPC++; goto **vPC++;
        MUL:   *(sp-1) = (void*)(*(long*)(sp-1) * *(long*)(sp-2));
        --sp;
        goto **vPC++;;
        PRINT: //printf("%li\n", *(long*)sp--); goto **vPC++;;
        HALT:  return;
    };

    bench("jump", 1000, [&] {
        jump();
    });
}

TEST(bench, types) {
    bench("simple type", 1000, [&] {
        for (auto i = 0; i < 1000; i++) {
            volatile auto a = make_shared<SimpleType>();
        }
    });

    bench("simple type array", 1000, [&] {
        auto simpleTypes = vector<shared<SimpleType>>();
        simpleTypes.reserve(1000);
        for (auto i = 0; i < 1000; i++) {
            simpleTypes.emplace_back();
            simpleTypes.pop_back();
        }
    });

    bench("simple type 2", 1000, [&] {
        volatile auto a = make_shared<SimpleType2>();
    });

    bench("simple type 3", 1000, [&] {
        volatile auto a = new SimpleType2;
        delete a;
    });

    bench("simple type 4", 1000, [&] {
        volatile vm::TypeKind *a = new vm::TypeKind;
        *a = vm::TypeKind::Unknown;
        delete a;
    });

    bench("base type", 1000, [&] {
        volatile auto a = make_shared<vm::Type>();
    });

    bench("string type", 1000, [&] {
        for (auto i = 0; i < 1000; i++) {
            volatile auto a = make_shared<vm::TypeString>();
        }
    });

    bench("string type array", 1000, [&] {
        auto simpleTypes = vector<shared<vm::TypeString>>();
        simpleTypes.reserve(1000);
        for (auto i = 0; i < 1000; i++) {
            simpleTypes.emplace_back();
            simpleTypes.pop_back();
        }
    });

    bench("string literal type ", 1000, [&] {
        for (auto i = 0; i < 1000; i++) {
            volatile auto a = make_shared<vm::TypeLiteral>("", vm::TypeLiteralType::String);
        }
    });

    bench("string literal type array", 1000, [&] {
        auto simpleTypes = vector<shared<vm::TypeLiteral>>();
        simpleTypes.reserve(1000);
        for (auto i = 0; i < 1000; i++) {
            simpleTypes.emplace_back();
            simpleTypes.pop_back();
        }
    });
}

TEST(bench, hashing) {
    string_view sv = "foo188";
    string s = "foo188";

    bench("hash from view", 100, [&] {
        tr::hash::runtime_hash(s);
    });

    bench("hash from string", 100, [&] {
        tr::hash::runtime_hash(s);
    });
}
