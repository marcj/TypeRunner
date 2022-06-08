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

    inline string_view readStorage(const string_view &bin, const uint32_t offset) {
        const auto size = readUint16(bin, offset);
        return string_view(reinterpret_cast<const char *>(bin.data() + offset + 2), size);
    }

    inline bool isConditionTruthy(shared<Type> node) {
        if (auto n = to<TypeLiteral>(node)) return n->literal == "true";
        return false;
    }

    struct LoopHelper {
        vector<shared<Type>> types;
        unsigned int i = 0;

        LoopHelper(const shared<Type> &type) {
            if (auto t = to<TypeUnion>(type)) {
                types = t->types;
            } else {
                types = {type};
            }
        }

        sharedOpt<Type> next() {
            if (i == types.size()) return nullptr;
            return types[i++];
        }
    };

    struct Frame {
        unsigned int sp = 0; //stack pointer
        unsigned int initialSp = 0; //initial stack pointer
        unsigned int variables = 0; //the amount of registered variable slots on the stack. will be subtracted when doing popFrame()
        sharedOpt<LoopHelper> loop = nullptr;
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
        vector<string> errors;
        /**
         * Linked list of subroutines to execute. For each external call this subroutine will this be changed.
         */
        sharedOpt<Subroutine> subroutine;

        vector<shared<Type>> stack;

        void printErrors() {
            debug("errors ({}):", errors.size());
            for (auto &&e: errors) {
                debug("  " + e);
            }
        }

        shared<Type> call(const string_view &bin, unsigned int address = 0, unsigned int arguments = 0) {
            //todo: check if address was already calculated using unordered_map?
            const auto loopRunning = !!subroutine;
            if (!loopRunning) {
                errors.clear();
            }
            if (subroutine) {
                //subtract `arguments` from the stack pointer so popFrame() returns correct entries.
                //they do no longer belong to the current frame, but will be moved to the next one as inputs.
                subroutine->frame->sp -= arguments;
            }
            auto next = make_shared<Subroutine>(bin);
            next->ip = address;
//            debug("call {}({})", address, arguments);
            next->previous = subroutine;
            next->depth = subroutine ? subroutine->depth + 1 : 0;
            next->frame = make_shared<Frame>();
            next->frame->sp = stack.size();
            next->frame->initialSp = next->frame->sp - arguments; //we reuse the types that were prepared as arguments

            if (loopRunning) subroutine->ip++; //`subroutine` is set to something new in next line, so for() increments its ip, not ours
            subroutine = next;
            if (loopRunning) subroutine->ip--; //`subroutine` is set to something new, so for() increments its ip, which we don't want

            if (!loopRunning) this->process();

//            print();
            return next->resultType;
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

        shared<Type> pop() {
            if (stack.size() == 0) throw runtime_error("stack is empty. Can not pop");
            auto r = stack.back();
            stack.pop_back();
            subroutine->frame->sp--;
            if (subroutine->frame->sp < subroutine->frame->initialSp) {
                throw runtime_error("Popped through frame");
            }
            return r;
        }
        void push(const shared<Type> &type) {
            subroutine->frame->sp++;
            stack.push_back(type);
        }

        void print() {
            debug("");
            debug("");
            debug("----------------------------------");
            auto current = subroutine;
            debug("# Debug VM: {} stack entries", stack.size());
            while (current) {
                auto currentFrame = current->frame;
                debug("# Routine {} ({})", current->depth, current->active);
                auto frameId = 0;
                while (currentFrame) {
                    debug("  # Frame {}: {} ({}->{}) stack frame entries ({})",
                          frameId, currentFrame->sp - currentFrame->initialSp, currentFrame->initialSp, currentFrame->sp,
                          currentFrame->loop ? "loop" : "");
                    if (currentFrame->sp && currentFrame->initialSp != currentFrame->sp) {
                        auto sub = slice(stack, currentFrame->initialSp, currentFrame->sp);
                        auto j = currentFrame->initialSp;
                        for (auto &&i: sub) {
                            debug("  - {}: {}", j++, i ? stringify(i) : "empty");
                        }
                    }
                    currentFrame = currentFrame->previous;
                    frameId++;
                }

                current = current->previous;
            }
        }

        void process() {
            while (subroutine) {
                const auto &bin = subroutine->bin;
                const auto end = bin.size();

                for (; subroutine->active && subroutine->ip < end; subroutine->ip++) {
                    const auto op = (OP) bin[subroutine->ip];
//                    debug("[{}] OP {} ({} -> {})", subroutine->depth, op, subroutine->ip, (unsigned int)bin[subroutine->ip]);
                    switch (op) {
                        case OP::Jump: {
                            subroutine->ip = readUint32(bin, 1) - 1; //minus 1 because for's i++
                            break;
                        }
                        case OP::Extends: {
                            auto right = pop();
                            auto left = pop();
//                            debug("{} extends {} => {}", stringify(left), stringify(right), isAssignable(right, left));
                            push(make_shared<TypeLiteral>(isAssignable(right, left) ? "true" : "false", TypeLiteralType::Boolean));
                            break;
                        }
                        case OP::Distribute: {
                            if (!subroutine->frame->loop) {
                                auto type = pop();
                                pushFrame();
                                subroutine->frame->loop = make_shared<LoopHelper>(type);
                            }

                            auto next = subroutine->frame->loop->next();
                            if (!next) {
                                //done
                                auto types = popFrame();
                                if (types.size() == 0) {
                                    push(make_shared<TypeNever>());
                                } else if (types.size() == 1) {
                                    push(types[0]);
                                } else {
                                    auto result = make_shared<TypeUnion>();
                                    for (auto &&v: types) {
                                        if (v->kind != TypeKind::Never) result->types.push_back(v);
                                    }
                                    push(result);
                                }
                                //jump over parameter
                                subroutine->ip += 4;
                            } else {
                                //next
                                const auto loopProgram = readUint32(bin, subroutine->ip + 1);
                                subroutine->ip--; //we jump back if the loop is not done, so that this section is executed when the following call() is done
                                push(next);
                                call(bin, loopProgram, 1);
                            }
                            break;
                        }
                        case OP::JumpCondition: {
                            auto condition = pop();
                            const auto leftProgram = readUint16(bin, subroutine->ip + 1);
                            const auto rightProgram = readUint16(bin, subroutine->ip + 3);
                            subroutine->ip += 4;
//                            debug("{} ? {} : {}", stringify(condition), leftProgram, rightProgram);
                            call(bin, isConditionTruthy(condition) ? leftProgram : rightProgram);
                            break;
                        }
                        case OP::Return: {
                            auto t = pop();
                            stack.resize(subroutine->frame->initialSp);
                            subroutine->active = false;
                            subroutine->ip++; //we jump directly to another subroutine, so for()'s increment doesn't apply
                            subroutine = subroutine->previous;
                            subroutine->ip--; //for()'s increment applies the wrong subroutine, so we decrease first
                            if (!t) {
                                throw runtime_error("Stack empty");
                            }
//                            debug("routine result: {}", vm::stringify(t));
                            stack.push_back(t);
                            subroutine->frame->sp++;
                            break;
                        }
                        case OP::TypeArgument: {
                            break;
                        }
                        case OP::Var: {
                            subroutine->frame->variables++;
                            push(make_shared<TypeUnknown>());
                            break;
                        }
                        case OP::Loads: {
                            const auto frameOffset = readUint16(bin, subroutine->ip + 1);
                            const auto varIndex = readUint16(bin, subroutine->ip + 3);
                            subroutine->ip += 4;
                            if (frameOffset == 0) {
                                push(stack[subroutine->frame->initialSp + varIndex]);
                            } else if (frameOffset == 1) {
                                push(stack[subroutine->frame->previous->initialSp + varIndex]);
                            } else {
                                throw runtime_error("frame offset not implement");
                            }
//                            debug("load var {}/{}", frameOffset, varIndex);
                            break;
                        }
                        case OP::Frame: {
                            pushFrame();
                            break;
                        }
                        case OP::Call: {
                            const auto address = readUint32(bin, subroutine->ip + 1);
                            const auto arguments = readUint16(bin, subroutine->ip + 5);
                            subroutine->ip += 6;
//                            subroutine->ip++;
                            call(bin, address, arguments);
//                            subroutine->ip--;
                            break;
                        }
                        case OP::Assign: {
                            const auto lvalue = pop();
                            const auto rvalue = pop();
                            if (!isAssignable(lvalue, rvalue)) {
                                errors.push_back(fmt::format("{}={} not assignable!", stringify(lvalue), stringify(rvalue)));
                            }
                            break;
                        }
                        case OP::True: push(make_shared<TypeLiteral>("true", TypeLiteralType::Boolean));
                            break;
                        case OP::False: push(make_shared<TypeLiteral>("false", TypeLiteralType::Boolean));
                            break;
                        case OP::String: push(make_shared<TypeString>());
                            break;
                        case OP::Number: push(make_shared<TypeNumber>());
                            break;
                        case OP::Boolean: push(make_shared<TypeBoolean>());
                            break;
                        case OP::Unknown: push(make_shared<TypeUnknown>());
                            break;
                        case OP::Undefined: push(make_shared<TypeUndefined>());
                            break;
                        case OP::Void: push(make_shared<TypeVoid>());
                            break;
                        case OP::Never: push(make_shared<TypeNever>());
                            break;
                        case OP::Any: push(make_shared<TypeAny>());
                            break;
                        case OP::Symbol: push(make_shared<TypeSymbol>());
                            break;
                        case OP::Object: push(make_shared<TypeObject>());
                            break;
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
                        default: {
                            throw runtime_error("unhandeled op");
                        }
                    }
                }

                subroutine = subroutine->previous;
            }

//            return stack.back();
        }
    };
}