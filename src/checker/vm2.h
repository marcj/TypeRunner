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

    constexpr auto poolSize = 10000;
    inline MemoryPool<Type, poolSize> pool;
    inline MemoryPool<TypeRef, poolSize> poolRef;
    void gcFlush();
    void gcRefFlush();
    void printStack();

    enum ActiveSubroutineFlag {
    };

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

        unsigned int flag = 0;

//        bool active = true;

        unsigned int typeArguments = 0;

//        explicit ProgressingSubroutine(shared<Module> module, ModuleSubroutine *subroutine): module(module), subroutine(subroutine) {}

        uint32_t parseUint32() {
            auto val = vm::readUint32(module->bin, ip + 1);
            ip += 4;
            return val;
        }

        bool isMain() {
            return !subroutine;
        }

        int32_t parseInt32() {
            auto val = vm::readInt32(module->bin, ip + 1);
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
        TypeRef *current = nullptr;
        unsigned int ip = 0;
        unsigned int startSP = 0;
        unsigned int var1 = 0;
        LoopHelper *previous = nullptr;

        void set(unsigned int var1, TypeRef *typeRef) {
            this->var1 = var1;
            current = typeRef;
        }

        bool next() {
            if (!current) return false;
            stack[var1] = current->type;
            current = current->next;
            return true;
        }
    };

    enum FrameFlag: uint8_t {
        //InSingleDistribute = 1<<0
    };

    struct Frame {
        unsigned int initialSp = 0; //initial stack pointer
        //the amount of registered variable slots on the stack. will be subtracted when doing popFrame()
        //type arguments of type functions and variables like for mapped types
        unsigned int variables = 0;

        //for every ActiveSubroutine this starts from 0 and increases within the subroutine.
        //important to know which frame should be removed when TailCall/Return
        unsigned int depth;

        uint8_t flags = 0;
        LoopHelper *loop = nullptr;

        LoopHelper *createLoop(unsigned int var1, TypeRef *type);
        LoopHelper *createEmptyLoop();

        void popLoop();

        unsigned int size() {
            return sp - initialSp;
        }

        std::span<Type *> pop(unsigned int size) {
            sp -= size;
            return {stack.data() + sp, size};
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
            if (i>=Size) {
                throw std::runtime_error("Stack overflow");
            }
            return &values[++i];
        }

        T *pop() {
            if (i == 0) {
                throw std::runtime_error("Popped out of stack");
            }
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
    void gcStackAndFlush();
    TypeRef *useAsRef(Type *type);
    Type *allocate(TypeKind kind);

    std::span<Type *> popFrame();

    static void run(shared<Module> module) {
//        profiler.clear();
//        pool = MemoryPool<Type, poolSize>();
//        poolRef = MemoryPool<TypeRef, poolSize>();
        pool.clear();
        poolRef.clear();

        gcQueueIdx = 0;
        gcQueueRefIdx = 0;
        sp = 0;
        activeSubroutine = activeSubroutines.reset();
        frame = frames.reset();
        loops.reset();

        prepare(module);
        process();
    }

    void call(shared<Module> &module, unsigned int index = 0, unsigned int arguments = 0);

    struct CStack {
        vector<Type *> iterator;
        unsigned int i;
        unsigned int round;
    };

    class CartesianProduct {
        vector<CStack> stack;
    public:

        Type *current(CStack &s) {
            return s.iterator[s.i];
        }

        bool next(CStack &s) {
            return (++s.i == s.iterator.size()) ? (s.i = 0, false) : true;
        }

        vector<Type *> toGroup(Type *type) {
            if (type->kind == TypeKind::Boolean) {
                return {allocate(TypeKind::Literal)->setFlag(TypeFlag::True), allocate(TypeKind::Literal)->setFlag(TypeFlag::False)};
            } else if (type->kind == TypeKind::Null) {
                return {allocate(TypeKind::Literal)->setLiteral(TypeFlag::StringLiteral, "null")};
            } else if (type->kind == TypeKind::Undefined) {
                return {allocate(TypeKind::Literal)->setLiteral(TypeFlag::StringLiteral, "undefined")};
                // } else if (type->kind == TypeKind::templateLiteral) {
                // //     //todo: this is wrong
                // //     return type.types;
                //     const result: Type[] = [];
                //     for (const s of type.types) {
                //         const g = this.toGroup(s);
                //         result.push(...g);
                //     }
                //
                //     return result;
            } else if (type->kind == TypeKind::Union) {
                vector<Type *> result;
                auto current = (TypeRef *) type->type;
                while (current) {
                    auto g = toGroup(current->type);
                    for (auto &&s: g) result.push_back(s);
                    current = current->next;
                }

                return result;
            } else {
                return {type};
            }
        }

        void add(Type *item) {
            stack.push_back({.iterator=toGroup(item), .i= 0, .round= 0});
        }

        vector<vector<Type *>> calculate() {
            vector<vector<Type *>> result;

            outer:
            while (true) {
                vector<Type *> row;
                for (auto &&s: stack) {
                    auto item = current(s);
                    if (item->kind == TypeKind::TemplateLiteral) {
                        auto current = (TypeRef *) item->type;
                        while (current) {
                            row.push_back(current->type);
                            current = current->next;
                        }
                    } else {
                        row.push_back(item);
                    }
                }
                result.push_back(row);

                for (unsigned int i = stack.size() - 1; i>=0; i--) {
                    auto active = next(stack[i]);
                    //when that i stack is active, continue in main loop
                    if (active) goto outer;

                    //i stack was rewinded. If it's the first, it means we are done
                    if (i == 0) {
                        goto done;
                    }
                }
                break;
            }

            done:
            return result;
        }
    };

}