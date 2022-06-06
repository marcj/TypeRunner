#pragma once

#include "./type_objects.h"
#include "../core.h"
#include "./instructions.h"
#include "./checks.h"
#include "./utils.h"

#include <memory>
#include <string>
#include <vector>

namespace ts::vm {
    using namespace ts::checker;
    using std::string_view;
    using std::vector;
    using instructions::OP;

    string_view readStorage(const string_view &bin, const uint32_t offset) {
        const auto size = readUint16(bin, offset);
        return string_view(reinterpret_cast<const char *>(bin.data() + offset + 2), size);
    }

    struct Frame {
        unsigned int sp = 0; //stack pointer
        unsigned int initialSp = 0; //initial stack pointer
        unsigned int variables = 0; //the amount of registered variable slots on the stack. will be subtracted when doing popFrame()
        bool isMappedType = false;
        sharedOpt<Frame> previous = nullptr;
        Frame(sharedOpt<Frame> previous = nullptr): previous(previous) {}
    };

    struct Subroutine {
        const string_view &bin;
        unsigned int ip = 0; //instruction pointer

        unsigned int depth = 0;
        bool active = true;
        sharedOpt<Type> resultType = nullptr;
        sharedOpt<Subroutine> previous = nullptr;
        shared<Frame> frame;
        Subroutine(const string_view &bin): bin(bin) {}
    };

    class VM {
    public:
        /**
         * Linked list of subroutines to execute. For each external call this subroutine will this be changed.
         */
        sharedOpt<Subroutine> subroutine;

        vector<shared<Type>> stack;
        vector<string_view> errors;

        void call(const string_view &bin, unsigned int address = 0) {
            //todo: check if address was already calculated using unordered_map?
            const auto loopRunning = !!subroutine;
            auto next = make_shared<Subroutine>(bin);
            next->ip = address;
            next->previous = subroutine;
            next->depth = subroutine ? subroutine->depth + 1 : 0;
            next->frame = make_shared<Frame>();
            next->frame->sp = stack.size();
            next->frame->initialSp = next->frame->sp;

            subroutine = next;

            if (!loopRunning) this->process();
        }

        shared<Type> pop() {
            if (stack.size() == 0) throw runtime_error("stack is empty. Can not pop");
            auto r = stack.back();
            stack.pop_back();
            return r;
        }

        vector<shared<Type>> popFrame() {
            auto sub = slice(stack, subroutine->frame->initialSp, subroutine->frame->sp);
            stack.resize(subroutine->frame->initialSp);
            subroutine->frame = subroutine->frame->previous;
            if (!subroutine->frame) throw runtime_error("Invalid pop frame");
            return sub;
        }

        void pushFrame() {
            subroutine->frame = make_shared<Frame>(subroutine->frame);
            subroutine->frame->sp = stack.size();
            subroutine->frame->initialSp = subroutine->frame->sp;
        }

        void push(const shared<Type> &type) {
            subroutine->frame->sp++;
            stack.push_back(type);
        }

        void process() {
            while (subroutine) {
                const auto &bin = subroutine->bin;
                const auto end = bin.size();

                for (; subroutine->active && subroutine->ip < end; subroutine->ip++) {
                    const auto op = (OP) bin[subroutine->ip];
//                    debug("[{}] OP {} ({})", subroutine->depth, op, bin[subroutine->ip]);
                    switch (op) {
                        case OP::Jump: {
                            subroutine->ip = readUint32(bin, 1) - 1; //minus 1 because for's i++
                            break;
                        }
                        case OP::Return: {
                            auto t = pop();
                            stack.resize(subroutine->frame->sp);
                            subroutine = subroutine->previous;
                            push(t);
                            subroutine->active = false;
                            break;
                        }
                        case OP::Frame: {
                            pushFrame();
                            break;
                        }
                        case OP::Call: {
                            const auto address = readUint32(bin, subroutine->ip + 1);
                            subroutine->ip += 4;
                            subroutine->ip++; //`subroutine` is set to something new in next line, so for() increments its ip, not ours
                            call(bin, address);
                            subroutine->ip--; //`subroutine` is set to something new, so for() increments its ip, which we don't want
                            break;
                        }
                        case OP::Assign: {
                            const auto lvalue = pop();
                            const auto rvalue = pop();
                            if (!isAssignable(lvalue, rvalue)) {
//                                debug("{}={} not assignable!", stringify(lvalue), stringify(rvalue));
                            }
                            break;
                        }
                        case OP::String: push(make_shared<TypeString>()); break;
                        case OP::Number: push(make_shared<TypeNumber>()); break;
                        case OP::Boolean: push(make_shared<TypeBoolean>()); break;
                        case OP::Unknown: push(make_shared<TypeUnknown>()); break;
                        case OP::Undefined: push(make_shared<TypeUndefined>()); break;
                        case OP::Void: push(make_shared<TypeVoid>()); break;
                        case OP::Never: push(make_shared<TypeNever>()); break;
                        case OP::Any: push(make_shared<TypeAny>()); break;
                        case OP::Symbol: push(make_shared<TypeSymbol>()); break;
                        case OP::Object: push(make_shared<TypeObject>()); break;
                        case OP::Union: {
                            auto t = make_shared<TypeUnion>();
                            t->types = popFrame();
                            push(t);
                            break;
                        }
                        case OP::NumberLiteral: {
                            const auto address = readUint32(bin, subroutine->ip + 1);
                            subroutine->ip += 4;
                            push(make_shared<TypeLiteral>(readStorage(bin, address), TypeLiteralType::Number));
                            break;
                        }
                        case OP::BigIntLiteral: {
                            const auto address = readUint32(bin, subroutine->ip + 1);
                            subroutine->ip += 4;
                            push(make_shared<TypeLiteral>(readStorage(bin, address), TypeLiteralType::Bigint));
                            break;
                        }
                        case OP::StringLiteral: {
                            const auto address = readUint32(bin, subroutine->ip + 1);
                            subroutine->ip += 4;
                            push(make_shared<TypeLiteral>(readStorage(bin, address), TypeLiteralType::String));
                            break;
                        }
                    }
                }

                subroutine = subroutine->previous;
            }

//            return stack.back();
        }
    };
}