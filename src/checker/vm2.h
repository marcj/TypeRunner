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

//    constexpr auto memoryDefault = 4096 * 10;

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

    constexpr auto poolSize = sizeof(Type) * 2048;
    inline MemoryPool<Type, poolSize> pool;
    inline MemoryPool<TypeRef, poolSize> poolRef;
    void gcFlush();
    void gcRefFlush();

    /**
     * For each active subroutine this object is created.
     */
    struct ActiveSubroutine {
        Module *module;
        ModuleSubroutine *subroutine;
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

    inline std::array<TypeRef *, maxGcSize> gcQueueRef;
    inline unsigned int gcQueueRefIdx;

    // The stack does not own Type
    inline std::array<Type *, 4069 * 10> stack;
    inline unsigned int sp = 0;

    struct LoopHelper {
        TypeRef *current;

        explicit LoopHelper() {
        }

        explicit LoopHelper(TypeRef *typeRef) {
            set(typeRef);
        }

        void set(TypeRef *typeRef) {
            current = typeRef;
        }

        Type *next() {
            auto t = current->type;
            current = current->next;
            return t;
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

    void clear(shared<ts::vm2::Module> &module);
    void prepare(shared<ts::vm2::Module> &module);
    void drop(Type *type);
    void drop(TypeRef *type);
    void gc(TypeRef *typeRef);
    void gc(Type *type);
    // Garbage collect whatever is left on the stack
    void gcStack();

    std::span<Type *> popFrame();

    static void run(shared<Module> module) {
//        profiler.clear();
        pool = MemoryPool<Type, poolSize>();
        poolRef = MemoryPool<TypeRef, poolSize>();
        gcQueueIdx = 0;
        sp = 0;
        prepare(module);
        process();
    }

    void call(shared<Module> &module, unsigned int index = 0, unsigned int arguments = 0);
}