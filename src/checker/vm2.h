#pragma once

#include <asmjit/a64.h>
#include <stdio.h>
#include "./MemoryPool.h"
#include <array>
#include <string>
#include <span>
#include <memory>
#include "../core.h"
#include "./utils.h"
#include "./types2.h"
#include "./module2.h"
#include "./instructions.h"

namespace ts::vm2 {
    using instructions::OP;
    using std::string_view;

    constexpr auto memoryDefault = 4096 * 10;

//    struct TypeMemoryPool2 {
//        MemoryPool<Type, memoryDefault> unknown;
//        MemoryPool<TypeNever, memoryDefault> never;
//        MemoryPool<TypeAny, memoryDefault> any;
//        MemoryPool<TypeLiteral, memoryDefault> literal;
//        MemoryPool<TypeObjectLiteral, memoryDefault> objectLiteral;
//        MemoryPool<TypeUnion, memoryDefault> unions;
//        MemoryPool<TypeTuple, memoryDefault> tuple;
//        MemoryPool<TypeTupleMember, memoryDefault> tupleMember;
//        MemoryPool<TypePropertySignature, memoryDefault> propertySignature;
//    };

    constexpr auto poolUSize = sizeof(Type) * 1000;
    inline MemoryPool<Type, poolUSize> poolU;

    inline Type *allocate(TypeKind kind) {
        auto item = poolU.allocate();
        item->kind = kind;
        return item;
    }

    /**
     * For each active subroutine this object is created.
     */
    struct ActiveSubroutine {
        Module *module;
//        ActiveSubroutine *previous = nullptr;

        unsigned int ip = 0; //current instruction pointer
//        unsigned int index = 0;
        unsigned int depth = 0;
//        bool active = true;

        unsigned int typeArguments = 0;

//        explicit ProgressingSubroutine(shared<Module> module, ModuleSubroutine *subroutine): module(module), subroutine(subroutine) {}

        uint32_t parseUint32() {
            auto val = vm::readUint32(module->bin, ip + 1);
            ip += 4;
            return val;
        }

        uint16_t parseUint16() {
            auto val = vm::readUint16(module->bin, ip + 1);
            ip += 2;
            return val;
        }
    };

    constexpr auto maxGcSize = 4069;
    inline std::array<Type *, maxGcSize> gcQueue;
    inline unsigned int gcQueueIdx;

    // The stack does not own Type
    inline std::array<Type *, 4069 * 10> stack;
    inline unsigned int sp = 0;

    struct LoopHelper {
        std::span<Type *> types;
        unsigned int i = 0;

        void set(std::span<Type *> types) {
            this->types = types;
            i = 0;
        }

        Type *next() {
            if (i == types.size()) return nullptr;
            return types[i++];
        }
    };

    struct Frame {
        unsigned int initialSp = 0; //initial stack pointer
        unsigned int variables = 0; //the amount of registered variable slots on the stack. will be subtracted when doing popFrame()
        LoopHelper *loop = nullptr;

        unsigned int size() {
            return sp - initialSp;
        }
    };

    template<class T, int Size>
    struct StackPool {
        std::array<T, Size> values;
        unsigned int i;

        T *at(unsigned int pos) {
            return &values[pos];
        }

        T *reset() {
            i = 0;
            return &values[0];
        }

        T *push() {
            return &values[++i];
        }

        T *pop() {
            return &values[--i];
        }
    };

    constexpr auto stackSize = 4069;
    inline StackPool<Frame, stackSize> frames;
    inline StackPool<ActiveSubroutine, stackSize> activeSubroutines;
    inline StackPool<LoopHelper, stackSize> loops;

//    inline std::array<Frame, 4069> frames;
//    inline unsigned int frameIdx;

//    inline std::array<ActiveSubroutine, 4069> activeSubroutines;
//    inline unsigned int activeSubroutineIdx;

    inline Frame *frame = nullptr;
    inline ActiveSubroutine *activeSubroutine = nullptr;
//    inline TypeMemoryPool2 pool;

    void process();

    void prepare(shared<ts::vm2::Module> &module);

    std::span<Type *> popFrame();

    static void run(shared<Module> module) {
//        profiler.clear();
//        pool = TypeMemoryPool2();
        poolU = MemoryPool<Type, poolUSize>();
        gcQueueIdx = 0;
        sp = 0;
        prepare(module);
        process();
    }

    void call(shared<Module> &module, unsigned int index = 0, unsigned int arguments = 0);

//    struct Type2 {
//        string_view typeName;
//        TypeKind kind = TypeKind::Never;
////        unsigned int kind = (int)TypeKind::Never;
//        //this is the OP position (instruction pointer) of the bytecode. sourcemap is necessary to map it to source positions.
//        unsigned int ip = 0;
//    };
//    unsigned int called = 0;
//
//    typedef void (*instruction_t)();
//    static instruction_t program[] = {
//            0, 0, 0, 0, 0
//    };
//    instruction_t *s = nullptr;
//    const char *ip = nullptr;
//    const char *bin = nullptr;
//
//    inline uint32_t readUint32() {
//        return *(uint32_t *) (ip);
//    }
//
//    static void Noop() {
//        ++ip;
//        called++;
//        (*program[*ip])();
//    }
//
//    static void Main() {
//        ++ip;
//        auto a = readUint32();
//        ip += a;
//        (*program[*ip])();
//    }
//
//    static void SourceMap() {
//        auto size = readUint32();
//        ip += 4 + size;
//
//        (*program[*ip])();
//    }
//
//    static void Jump() {
//        ++ip;
//        auto a = readUint32();
//        ip += a;
//        (*program[*ip])();
//    }
//
//    static void Halt() {
//    }
//
////    static instruction_t program[] = {
////            Noop,
////            Jump,
////            Halt,
////            SourceMap,
////            Main
////    };
//
//    void jump(shared<ts::vm::Module> &module) {
//        //https://christopherschwaab.wordpress.com/2018/04/13/generating-a-threaded-arm-interpreter-with-templates/
//        // We're using the GNU C, labels-as-values extension here.
//        void *prog[] = {&&PUSH, (void*)6,
//        &&PUSH, (void*)7,
//        &&MUL, &&PRINT, &&HALT};
//
//        void **vPC = prog;
//        void *stack[4], **sp = stack;
//
//        goto
//        **vPC++;
//
//        PUSH:
//        *sp++ = *vPC++;
//        goto
//        **vPC++;
//
//        MUL:
//        *(sp - 1) = (void *) (*(long *) (sp - 1) * *(long *) (sp - 2));
//        --sp;
//        goto
//        **vPC++;
//
//        PRINT:
//        printf("%li\n", *(long *) sp--);
//        goto
//        **vPC++;
//
//        HALT:
//        return;
//    }
//
//    void run(shared<ts::vm::Module> &module) {
//        program[0] = Noop;
//        program[1] = Jump;
//        program[2] = Halt;
//        program[3] = SourceMap;
//        program[4] = Main;
//
//        prepare(module);
//        bin = &module->bin[0];
//        s = &program[0];
//        ip = bin + module->mainAddress;
//        //replace instruction with actual function addresses
//        (*program[*ip])();
//    }
//
//    void naive(shared<ts::vm::Module> &module) {
//        prepare(module);
//        auto end = module->bin.size();
//        auto &bin = module->bin;
////        auto a = module->bin[0];
////
////        std::array<Type2, 300 * 6> stack;
////        unsigned int sp = 0;
////
////        std::array<Type2, 100'000> pool;
////        unsigned int pp = 0;
////
////        vector<Type2> unknowns;
////        unknowns.reserve(300 * 6);
////
////        auto TypeUknown;
//        int ip = module->mainAddress;
//
//        for (; ip < end; ip++) {
//            const auto op = (OP) bin[ip];
//
//            switch (op) {
//                case OP::StringLiteral: {
//                    ip += 4;
//                    break;
//                }
//                case OP::Unknown: {
//                    called += 4;
//                    break;
//                }
//                case OP::Noop: {
////                    called = 1800;
////                    (TypeUnknown *)(a) = TypeUnknown();
////                    unknowns.emplace_back();
////                    ip++;
////                    stack[sp++] = pool[pp++];
////                    stack[0].unprovidedArgument
////                    unknowns.push_back(&stack[u++]);
////                    unknowns.push_back(std::move(u));
////                    volatile auto a = unknowns[u++];
////                    volatile auto a = new TypeUnknown2();
////                    volatile auto a = std::make_shared<TypeUnknown2>();
//                    break;
//                }
//                case OP::Jump: {
//                    ip = ts::checker::readUint32(bin, 1) - 1; //minus 1 because for's i++
//                    break;
//                }
//            }
//        }
//    }
//
////    unsigned int called = 0;
//    __attribute__((noinline)) static void calledFunc() {
//        called++;
////        printf("Hello world\n");
//    }
//
//    // Signature of the generated function.
//    typedef int (*Func)(void);
//
//    // Runtime designed for JIT - it hold relocated functions and controls their lifetime.
//    JitRuntime rt;
//
//    Func jitBuild() {
//        // Holds code and relocation information during code generation.
//        CodeHolder code;
//
//        // Code holder must be initialized before it can be used. The simples way to initialize
//        // it is to use 'Environment' from JIT runtime, which matches the target architecture,
//        // operating system, ABI, and other important properties.
//        code.init(rt.environment());
//
//        // Emitters can emit code to CodeHolder - let's create 'x86::Assembler', which can emit
//        // either 32-bit (x86) or 64-bit (x86_64) code. The following line also attaches the
//        // assembler to CodeHolder, which calls 'code.attach(&a)' implicitly.
//        a64::Compiler cc(&code);
//
////        FileLogger logger(stdout);
////        code.setLogger(&logger);
//
//        // Use the x86::Assembler to emit some code to .text section in CodeHolder:
////        a.mov(x86::eax, 1);  // Emits 'mov eax, 1' - moves one to 'eax' register.
////        a.ret();             // Emits 'ret'        - returns from a function.
//        cc.addFunc(FuncSignatureT<void>());
//
////        arm::Gp r = cc.newUInt32("r");
////        arm::Gp fn2 = cc.newUIntPtr("fn");
////        cc.mov(fn2, (uint64_t) calledFunc);
//
//        //found: You can use imm_ptr(puts), that would convert the function pointer into an immediate value
//        arm::Gp pCalled = cc.newUIntPtr("called");
////        arm::Gp pByOne = cc.newUIntPtr("byOne");
//        cc.mov(pCalled, 0);
//
//        for (auto i = 0; i < 1; i++) {
//            cc.add(pCalled, pCalled, 1);
//        }
//
//        arm::Gp r = cc.newUIntPtr("r");
//        cc.mov(r, (uint64_t) &called);
//        cc.str(pCalled, arm::ptr(r));
//
////        cc.str(pCalled, arm::ptr((uint64_t)&called));
//
////        cc.str(pCalled, arm::ptr(&called));
//
////        InvokeNode *invokeNode;
//
////        for (auto i = 0; i < 300; i++) {
////            cc.invoke(&invokeNode, fn2, FuncSignatureT<void>(CallConvId::kHost));
////            cc.invoke(&invokeNode, fn2, FuncSignatureT<void>(CallConvId::kHost));
////            cc.invoke(&invokeNode, fn2, FuncSignatureT<void>(CallConvId::kHost));
////            cc.invoke(&invokeNode, fn2, FuncSignatureT<void>(CallConvId::kHost));
////            cc.invoke(&invokeNode, fn2, FuncSignatureT<void>(CallConvId::kHost));
////            cc.invoke(&invokeNode, fn2, FuncSignatureT<void>(CallConvId::kHost));
////        }
//
//        cc.ret();
//        cc.endFunc();
//        cc.finalize();
//
//        // 'x86::Assembler' is no longer needed from here and can be destroyed or explicitly
//        // detached via 'code.detach(&a)' - which detaches an attached emitter from code holder.
//
//        // Now add the generated code to JitRuntime via JitRuntime::add(). This function would
//        // copy the code from CodeHolder into memory with executable permission and relocate it.
//        Func fn;
//        Error err = rt.add(&fn, &code);
//
//        // It's always a good idea to handle errors, especially those returned from the Runtime.
//        if (err) {
//            printf("AsmJit failed: %s\n", DebugUtils::errorAsString(err));
//            throw runtime_error("error");
//        }
//
//        return fn;
//    }
//
//    void jitTest() {
//
//        auto fn = jitBuild();
//
//        // CodeHolder is no longer needed from here and can be safely destroyed. The runtime now
//        // holds the relocated function, which we have generated, and controls its lifetime. The
//        // function will be freed with the runtime, so it's necessary to keep the runtime around.
//        //
//        // Use 'code.reset()' to explicitly free CodeHolder's content when necessary.
//
//        // Execute the generated function and print the resulting '1', which it moves to 'eax'.
//        int result = fn();
//
////        CodeHolder code;
////        JitRuntime runtime;
////
////        a64::Compiler cc(&code);
////
////        // Call a function.
////        InvokeNode* invokeNode;
////        cc.invoke(&invokeNode, imm((void*)calledFunc), FuncSignatureT<int, int, int, int>(CallConvId::kHost));
//////        cc.ret();
////        cc.finalize();
////
////        code.init(runtime.environment());
////
////        Func func;
////        runtime.add(&func, &code);
////
////        int result = func();
//    }
//
//    void jit(shared<ts::vm::Module> &module) {
////        a64::Compiler c;
////        c.add(Jump);
//
////        JitRuntime rt;                    // Runtime specialized for JIT code execution.
////        CodeHolder code;                  // Holds code and relocation information.
////        code.init(rt.environment());      // Initialize code to match the JIT environment.
////        a64::Assembler a(&code);          // Create and attach x86::Assembler to code.
////        a.mov(a64::eax, 1);               // Move one to eax register.
////        a.ret();                          // Return from function.
////        // ===== x86::Assembler is no longer needed from here and can be destroyed =====
////        Func fn;                          // Holds address to the generated function.
////        Error err = rt.add(&fn, &code);   // Add the generated code to the runtime.
////        if (err) return 1;                // Handle a possible error returned by AsmJit.
////        // ===== CodeHolder is no longer needed from here and can be destroyed =====
////        int result = fn();                // Execute the generated code.
////        printf("%d\n", result);           // Print the resulting "1".
////        // All classes use RAII, all resources will be released before `main()` returns,
////        // the generated function can be, however, released explicitly if you intend to
////        // reuse or keep the runtime alive, which you should in a production-ready code.
////        rt.release(fn);
//
//
////        end = module->bin.size();
//
////        auto &bin = module->bin;
////        auto a = module->bin[0];
//
////        std::array<Type2, 300*6> stack;
////        unsigned int sp = 0;
////
////        std::array<Type2, 100'000> pool;
////        unsigned int pp = 0;
//
////        vector<Type2> unknowns;
////        unknowns.reserve(300*6);
//
////        auto TypeUknown;
////
////        for (; ip < end; ip++) {
////            const auto op = (OP) bin[ip];
////
////            switch (op) {
////                case OP::Noop: {
//////                    (TypeUnknown *)(a) = TypeUnknown();
//////                    unknowns.emplace_back();
//////                    stack[sp++] = pool[pp++];
//////                    stack[0].unprovidedArgument
//////                    unknowns.push_back(&stack[u++]);
//////                    unknowns.push_back(std::move(u));
//////                    volatile auto a = unknowns[u++];
//////                    volatile auto a = new TypeUnknown2();
//////                    volatile auto a = std::make_shared<TypeUnknown2>();
////                    break;
////                }
////                case OP::Jump: {
////                    ip = ts::checker::readUint32(bin, 1) - 1; //minus 1 because for's i++
////                    break;
////                }
////            }
////        }
////
////        return ip;
//    }
}