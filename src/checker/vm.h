#pragma once

#include "./types.h"
#include "../core.h"
#include "./instructions.h"
#include "./checks.h"
#include "./utils.h"

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <span>
#include <unordered_map>
#include <iostream>
#include <algorithm>

namespace tr::vm {
    using namespace tr::checker;
    using std::string_view;
    using std::span;
    using std::vector;
    using std::runtime_error;
    using instructions::OP;
    using instructions::ErrorCode;

    inline bool isConditionTruthy(node<Type> node) {
        if (auto n = to<TypeLiteral>(node)) return n->text() == "true";
        return false;
    }

    struct CStack {
        vector<node<Type>> iterator;
        unsigned int i;
        unsigned int round;
    };

    class CartesianProduct {
        vector<CStack> stack;
    public:

        node<Type> current(CStack &s) {
            return s.iterator[s.i];
        }

        bool next(CStack &s) {
            return (++s.i == s.iterator.size()) ? (s.i = 0, false) : true;
        }

        vector<node<Type>> toGroup(node<Type> &type) {
            if (type->kind == TypeKind::Boolean) {
                return {make_shared<TypeLiteral>("false", TypeLiteralType::String), make_shared<TypeLiteral>("true", TypeLiteralType::String)};
            } else if (type->kind == TypeKind::Null) {
                return {make_shared<TypeLiteral>("null", TypeLiteralType::String)};
//                return [{ kind: TypeKind::literal, literal: 'null' }];
            } else if (type->kind == TypeKind::Undefined) {
                return {make_shared<TypeLiteral>("undefined", TypeLiteralType::String)};
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
                vector<node<Type>> result;
                for (auto &&v: to<TypeUnion>(type)->types) {
                    auto g = toGroup(v);
                    for (auto &&s: g) result.push_back(s);
                }

                return result;
            } else {
                return {type};
            }
        }

        void add(node<Type> &item) {
            stack.push_back({.iterator=toGroup(item), .i= 0, .round= 0});
        }

        vector<vector<node<Type>>> calculate() {
            vector<vector<node<Type>>> result;

            outer:
            while (true) {
                vector<node<Type>> row;
                for (auto &&s: stack) {
                    auto item = current(s);
                    if (item->kind == TypeKind::TemplateLiteral) {
                        for (auto &&v: to<TypeTemplateLiteral>(item)->types) row.push_back(v);
                    } else {
                        row.push_back(item);
                    }
                }
                result.push_back(row);

                for (unsigned int i = stack.size() - 1; i >= 0; i--) {
                    auto active = next(stack[i]);
                    //when that i stack is active, continue in main loop
                    if (active) goto outer;

                    //i stack was rewinded. If its the first, it means we are done
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

    /**
     * Query a container type and return the result.
     *
     * container[index]
     *
     * e.g. {a: string}['a'] => string
     * e.g. {a: string, b: number}[keyof T] => string | number
     * e.g. [string, number][0] => string
     * e.g. [string, number][number] => string | number
     */
    node<Type> indexAccess(node<Type> &container, node<Type> &index) {
        if (container->kind == TypeKind::Array) {
//            if ((index.kind == TypeKind::literal && 'number' == typeof index.literal) || index.kind == TypeKind::number) return container.type;
//            if (index.kind == TypeKind::literal && index.literal == 'length') return { kind: TypeKind::number };
        } else if (container->kind == TypeKind::Tuple) {
            if (index->kind == TypeKind::Literal && to<TypeLiteral>(index)->text() == "length") {
                auto t = make_shared<TypeLiteral>("", TypeLiteralType::Number);
                t->append(to<TypeTuple>(container)->types.size());
                return t;
            }

//            if (index.kind == TypeKind::literal && 'number' == typeof index.literal && index.literal < 0) {
//                index = { kind: TypeKind::number };
//            }
//    
//            if (index.kind == TypeKind::literal && 'number' == typeof index.literal) {
//                type b0 = [string, boolean?][0]; //string
//                type b1 = [string, boolean?][1]; //boolean|undefined
//                type a0 = [string, ...number[], boolean][0]; //string
//                type a1 = [string, ...number[], boolean][1]; //number|boolean
//                type a2 = [string, ...number[], boolean][2]; //number|boolean
//                type a22 = [string, ...number[], boolean][3]; //number|boolean
//                // type a23 = [string, number, boolean][4]; //number|boolean
//                type a3 = [string, number, ...number[], boolean][1]; //number
//                type a4 = [string, number, ...number[], boolean][-2]; //string|number|boolean, minus means all
//                type a5 = [string, number, ...number[], boolean][number]; //string|number|boolean
//    
//                let restPosition = -1;
//                for (let i = 0; i < container.types.length; i++) {
//                    if (container.types[i].type.kind == TypeKind::rest) {
//                        restPosition = i;
//                        break;
//                    }
//                }
//    
//                if (restPosition == -1 || index.literal < restPosition) {
//                    const sub = container.types[index.literal];
//                    if (!sub) return { kind: TypeKind::undefined };
//                    if (sub.optional) return { kind: TypeKind::union, types: [sub.type, { kind: TypeKind::undefined }] };
//                    return sub.type;
//                }
//    
//                //index beyond a rest, return all beginning from there as big enum
//    
//                const result: TypeUnion = { kind: TypeKind::union, types: [] };
//                for (let i = restPosition; i < container.types.length; i++) {
//                    const member = container.types[i];
//                    const type = member.type.kind == TypeKind::rest ? member.type.type : member.type;
//                    if (!isTypeIncluded(result.types, type)) result.types.push(type);
//                    if (member.optional && !isTypeIncluded(result.types, { kind: TypeKind::undefined })) result.types.push({ kind: TypeKind::undefined });
//                }
//    
//                return unboxUnion(result);
//            } else if (index.kind == TypeKind::number) {
//                const union: TypeUnion = { kind: TypeKind::union, types: [] };
//                for (const sub of container.types) {
//                    if (sub.type.kind == TypeKind::rest) {
//                        if (isTypeIncluded(union.types, sub.type.type)) continue;
//                        union.types.push(sub.type.type);
//                    } else {
//                        if (isTypeIncluded(union.types, sub.type)) continue;
//                        union.types.push(sub.type);
//                    }
//                }
//                return unboxUnion(union);
//            } else {
//                return { kind: TypeKind::never };
//            }
//        } else if (container.kind == TypeKind::objectLiteral || container.kind == TypeKind::class) {
//            if (index.kind == TypeKind::literal) {
//                return resolveObjectIndexType(container, index);
//            } else if (index.kind == TypeKind::union) {
//                const union: TypeUnion = { kind: TypeKind::union, types: [] };
//                for (const t of index.types) {
//                    const result = resolveObjectIndexType(container, t);
//                    if (result.kind == TypeKind::never) continue;
//    
//                    if (result.kind == TypeKind::union) {
//                        for (const resultT of result.types) {
//                            if (isTypeIncluded(union.types, resultT)) continue;
//                            union.types.push(resultT);
//                        }
//                    } else {
//                        if (isTypeIncluded(union.types, result)) continue;
//                        union.types.push(result);
//                    }
//                }
//                return unboxUnion(union);
//            } else {
//                return { kind: TypeKind::never };
//            }
//        } else if (container.kind == TypeKind::any) {
//            return container;
        }
        return make_shared<TypeNever>();
    }

    struct LoopHelper {
        vector<node<Type>> types;
        unsigned int i = 0;

        LoopHelper(const node<Type> &type) {
            if (auto t = to<TypeUnion>(type)) {
                types = t->types;
            } else {
                types = {type};
            }
        }

        optionalNode<Type> next() {
            if (i == types.size()) return nullptr;
            return types[i++];
        }
    };

    /**
     * For each active subroutine this object is created.
     */
    struct ProgressingSubroutine {
        node<Module> module;
        ModuleSubroutine *subroutine;

        unsigned int ip = 0; //instruction pointer
        unsigned int end = 0; //last instruction pointer
        unsigned int index = 0;

        unsigned int depth = 0;
        bool active = true;
        unsigned int typeArguments = 0;

        optionalNode<ProgressingSubroutine> previous = nullptr;

        explicit ProgressingSubroutine(node<Module> module, ModuleSubroutine *subroutine): module(module), subroutine(subroutine) {}

        uint32_t parseUint32() {
            auto val = readUint32(module->bin, ip + 1);
            ip += 4;
            return val;
        }

        uint16_t parseUint16() {
            auto val = readUint16(module->bin, ip + 1);
            ip += 2;
            return val;
        }
    };

    struct Frame {
        unsigned int sp = 0; //stack pointer
        unsigned int initialSp = 0; //initial stack pointer
        unsigned int variables = 0; //the amount of registered variable slots on the stack. will be subtracted when doing popFrame()
//        vector<unsigned int> variableIPs; //only used when stepper is active
        optionalNode<LoopHelper> loop = nullptr;
//        ModuleSubroutine *subroutine;

        unsigned int size() {
            return sp - initialSp;
        }

        void fromFrame(Frame &previous) {
            sp = previous.sp;
            initialSp = previous.sp;
//            subroutine = previous.subroutine;
        }
//
//        explicit Frame(Frame *previous = nullptr): previous(previous) {
//            if (previous) {
//                sp = previous->sp;
//                initialSp = previous->sp;
//                subroutine = previous->subroutine;
//            }
//        }
    };

    static unsigned char emptyTuple[] = {
            OP::Frame, OP::Tuple, OP::Return
    };
    static unsigned char stringToNumPattern[] = {
            OP::TypeArgument, OP::TypeArgument, OP::TypeArgumentDefault, 1, 0, 0, 0,
            OP::Frame,
            OP::Loads, 1, 0, 1, 0,
            OP::StringLiteral, 0, 0, 0, 0,
            OP::IndexAccess, OP::TemplateLiteral,
            OP::Loads, 0, 0, 0, 0,
            OP::Extends,
            OP::JumpCondition, 0, 0, 0, 0,
            OP::Return
    };

    class VM;

    node<Type> stringToNum(VM &vm);

    class MemoryPool {
    public:
        vector<TypeLiteral> typeLiterals;
        node<TypeLiteral> defaultTypeLiteral = make_shared<TypeLiteral>();
        node<TypeTupleMember> defaultTypeTupleMember = make_shared<TypeTupleMember>();
        node<TypeObjectLiteral> defaultTypeObjectLiteral = make_shared<TypeObjectLiteral>();
        node<TypePropertySignature> defaultPropertySignature = make_shared<TypePropertySignature>();
        node<TypeUnknown> defaultUnknown = make_shared<TypeUnknown>();

        vector<TypeUnknown> unknowns;

        MemoryPool() {
            unknowns.reserve(1000);
//            typeLiterals.reserve(1000);
        }

        void makeUnknown() {
            unknowns.emplace_back();
//            return defaultUnknown;
//            return make_shared<TypeUnknown>();
        }

        node<TypePropertySignature> makePropertySignature(const node<Type> &type) {
            defaultPropertySignature->type = type;
            return defaultPropertySignature;
        }

        node<TypeTupleMember> makeTupleMember(const node<Type> &type) {
            defaultTypeTupleMember->type = type;
            return defaultTypeTupleMember;
        }

        node<TypeObjectLiteral> makeObjectLiteral() {
            return defaultTypeObjectLiteral;
        }

        node<TypeLiteral> makeTypeLiteral(const string_view &literal, TypeLiteralType type) {
//            return make_shared<vm::TypeLiteral>(literal, type);
            return defaultTypeLiteral;
//            auto t = make_shared<TypeLiteral>(std::move(typeLiterals.emplace_back(literal, type)));
//            typeLiterals.pop_back();
//            return t;
        }
    };

    class VM {
    public:
        /**
         * Linked list of subroutines to execute. For each external call this subroutine will be changed.
         */
        optionalNode<ProgressingSubroutine> subroutine = nullptr;
        MemoryPool pool;
        vector<Frame> frames;
        vector<node<Type>> stack;
        //when a OP is processes, its instruction pointer is stores here, and used in push() to set the ip to the new type generated by this OP.
        //diagnostics/debugger can use this information to map the type to the sourcecode.
        unsigned int ip{};

        bool stepper = false;

        vector<node<Module>> modules;

        unordered_map<unsigned int, function<node<Type>(VM &)>> optimised;

        VM() {
//            stack.reserve(1000);
        }

        ~VM() {
        }

        void printErrors() {
            for (auto &&module: modules) {
                module->printErrors();
            }
        }

        unsigned int getErrors() {
            unsigned int errors = 0;
            for (auto &&module: modules) {
                errors += module->errors.size();
            }
            return errors;
        }

        void prepare(node<Module> module) {
            parseHeader(module);
            modules.push_back(module);

            if (subroutine) throw runtime_error("Subroutine already running");

            auto next = make_shared<ProgressingSubroutine>(module, module->getMain());
            next->ip = module->mainAddress;
            next->end = module->bin.size();
            next->previous = subroutine;
            subroutine = next;
            frames.emplace_back();
            if (frames.size() > 1) frames.back().fromFrame(frames[frames.size() - 2]);
        }

        void run(node<Module> module) {
            profiler.clear();
            prepare(module);
            process();
        }

        void call(node<Module> &module, unsigned int index = 0, unsigned int arguments = 0) {
            const auto loopRunning = !!subroutine;

            auto routine = module->getSubroutine(index);
            if (routine->narrowed) {
                push(routine->narrowed);
                return;
            }
            if (routine->result && arguments == 0) {
                push(routine->result);
                return;
            }

            auto next = make_shared<ProgressingSubroutine>(module, routine);
            next->ip = routine->address;
//            debug("Call {} -> {}", index, routine->address);
            next->end = module->bin.size();
            next->previous = subroutine;
            next->depth = subroutine ? subroutine->depth + 1 : 0;

            frames.emplace_back();
            if (frames.size() > 1) frames.back().fromFrame(frames[frames.size() - 2]);
//            frames.back().subroutine = next->subroutine;

            if (arguments) {
                //we move x arguments from the old stack to the new one
                frames[frames.size() - 2].sp -= arguments;
                frames.back().initialSp -= arguments;
            }

            if (loopRunning) subroutine->ip++; //`subroutine` is set to something new in next line, so for() increments its ip, not ours
            subroutine = next;
            if (loopRunning) subroutine->ip--; //`subroutine` is set to something new, so for() increments its ip, which we don't want

            if (!loopRunning) process();
        }

        /**
         * Read frame types without popping frame.
         */
        span<node<Type>> readFrame() {
            auto start = frames.back().initialSp + frames.back().variables;
            return {stack.data() + start, frames.back().sp - start};
        }

        span<node<Type>> popFrame() {
//            auto frameSize = frame->initialSp - frame->sp;
//            while (frameSize--) stack.pop();
            auto start = frames.back().initialSp + frames.back().variables;
            span<node<Type>> sub{stack.data() + start, frames.back().sp - start};
//            std::span<node<Type>> sub{stack.begin() + frame->initialSp, frameSize};
            frames.pop_back();
            return sub;
        }

        unsigned int frameSize() {
            return frames.back().initialSp - frames.back().sp;
        }

        void pushFrame() {
            frames.emplace_back();
            if (frames.size() > 1) frames.back().fromFrame(frames[frames.size() - 2]);
//            frames.back().subroutine = subroutine->subroutine;
        }

        node<Type> &last() {
            if (frames.back().sp == 0) throw runtime_error("stack is empty. Can not return last");
            return stack[frames.back().sp - 1];
        }

        node<Type> pop() {
            if (frames.back().sp == 0) throw runtime_error("stack is empty. Can not pop");
            frames.back().sp--;
            if (frames.back().sp < frames.back().initialSp) {
                throw runtime_error("Popped through frame");
            }
            return std::move(stack[frames.back().sp]);
        }

        optionalNode<Type> frameEntryAt(unsigned int position) {
            auto i = frames.back().initialSp + position;
            if (i > frames.back().sp) return nullptr;
            return stack[i];
        }

        void push(const node<Type> &type) {
//            type->ip = ip;
//            if (frames.back().sp >= stack.size()) {
//                stack.push_back(type);
//            } else {
//                stack[frames.back().sp] = type;
//            }
//            frames.back().sp++;
        }

        void print() {
            debug("");
            debug("");
            debug("----------------------------------");
            auto current = subroutine;
            debug("# Debug VM: {} stack entries", stack.size());
            while (current) {
                debug("# Routine {} ({})", current->depth, current->active);
                current = current->previous;
            }

//            auto currentFrame = frame;
//            auto frameId = 0;
//            while (currentFrame) {
//                debug("  # Frame {}: {} ({}->{}) stack frame entries ({})",
//                      frameId, currentFrame->sp - currentFrame->initialSp, currentFrame->initialSp, currentFrame->sp,
//                      currentFrame->loop ? "loop" : "");
//                if (currentFrame->sp && currentFrame->initialSp != currentFrame->sp) {
////                    auto sub = slice(stack, currentFrame->initialSp, currentFrame->sp);
////                    auto j = currentFrame->initialSp;
////                    for (auto &&i: sub) {
////                        debug("  - {}: {}", j++, i ? stringify(i) : "empty");
////                    }
//                }
//                currentFrame = currentFrame->previous;
//                frameId++;
//            }
        }

        void report(DiagnosticMessage message) {
            message.module = subroutine->module;
            subroutine->module->errors.push_back(message);
        }

        void report(const string &message, const node<Type> node) {
            report(DiagnosticMessage(message, node->ip));
        }

        void extends() {
            auto right = pop();
        }

        void process() {
            while (subroutine) {
                auto &bin = subroutine->module->bin;
                for (; subroutine->active && subroutine->ip < subroutine->end; subroutine->ip++) {
                    ip = subroutine->ip;
                    const auto op = (OP) bin[subroutine->ip];
//                    debug("[{}] OP {} ({} -> {})", subroutine->depth, op, subroutine->ip, (unsigned int) op);

                    switch (op) {
                        case OP::Halt: {
                            return;
                        }
                        case OP::Noop: {
                            break;
                        }
//                        case OP::Jump: {
//                            subroutine->ip = readUint32(bin, 1) - 1; //minus 1 because for's i++
//                            break;
//                        }
//                        case OP::Extends: {
//                            auto right = pop();
//                            auto left = pop();
////                            debug("{} extends {} => {}", stringify(left), stringify(right), isAssignable(right, left));
//                            const auto valid = isExtendable(left, right);
//                            push(make_shared<TypeLiteral>(valid ? "true" : "false", TypeLiteralType::Boolean));
//                            break;
//                        }
//                        case OP::Distribute: {
//                            if (!frames.back().loop) {
//                                auto type = pop();
//                                pushFrame();
//                                frames.back().loop = make_shared<LoopHelper>(type);
//                            }
//
//                            auto next = frames.back().loop->next();
//                            if (!next) {
//                                //done
//                                auto types = popFrame();
//                                if (types.empty()) {
//                                    push(make_shared<TypeNever>());
//                                } else if (types.size() == 1) {
//                                    push(types[0]);
//                                } else {
//                                    auto result = make_shared<TypeUnion>();
//                                    for (auto &&v: types) {
//                                        if (v->kind != TypeKind::Never) result->types.push_back(v);
//                                    }
//                                    push(result);
//                                }
//                                //jump over parameter
//                                subroutine->ip += 4;
//                            } else {
//                                //next
//                                const auto loopProgram = readUint32(bin, subroutine->ip + 1);
//                                subroutine->ip--; //we jump back if the loop is not done, so that this section is executed when the following call() is done
//                                push(next);
//                                call(subroutine->module, loopProgram, 1);
//                            }
//                            break;
//                        }
//                        case OP::JumpCondition: {
//                            auto condition = pop();
//                            const auto leftProgram = subroutine->parseUint16();
//                            const auto rightProgram = subroutine->parseUint16();
////                            debug("{} ? {} : {}", stringify(condition), leftProgram, rightProgram);
//                            call(subroutine->module, isConditionTruthy(condition) ? leftProgram : rightProgram);
//                            break;
//                        }
//                        case OP::Return: {
//                            //the current frame could not only have the return value, but variables and other stuff,
//                            //which we don't want. So if size is bigger than 1, we move last stack entry to first
//                            // | [T] [T] [R] |
//                            if (frames.back().size() > 1) {
//                                stack[frames.back().initialSp] = stack[frames.back().sp - 1];
//                            }
//                            frames.pop_back();
//                            frames.back().sp++; //consume the new stack entry from the function call, making it part of the caller frame
//                            subroutine->active = false;
//                            subroutine->ip++; //we jump directly to another subroutine, so for()'s increment doesn't apply
//                            subroutine = subroutine->previous;
//                            subroutine->ip--; //for()'s increment applies the wrong subroutine, so we decrease first
//                            if (subroutine->typeArguments == 0) {
//                                subroutine->subroutine->result = last();
//                            }
////                            debug("routine {} result: {}", subroutine->name, vm::stringify(last()));
//                            break;
//                        }
//                        case OP::TypeArgument: {
//                            if (frames.back().size() <= subroutine->typeArguments) {
//                                auto unknown = make_shared<TypeUnknown>();
//                                unknown->unprovidedArgument = true;
//                                push(unknown);
//                            }
//                            subroutine->typeArguments++;
//                            frames.back().variables++;
//                            break;
//                        }
//                        case OP::TypeArgumentDefault: {
//                            auto t = frameEntryAt(subroutine->typeArguments - 1);
//                            //t is always set because TypeArgument ensures that
//                            if (t->kind == TypeKind::Unknown && to<TypeUnknown>(t)->unprovidedArgument) {
//                                frames.back().sp--; //remove unknown type from stack
//                                const auto address = subroutine->parseUint32();
//                                call(subroutine->module, address, 0); //the result is pushed on the stack
//                            } else {
//                                subroutine->ip += 4; //jump over address
//                            }
//                            break;
//                        }
//                        case OP::TemplateLiteral: {
//                            handleTemplateLiteral();
//                            break;
//                        }
//                        case OP::Var: {
//                            frames.back().variables++;
//                            push(make_shared<TypeUnknown>());
//                            break;
//                        }
//                        case OP::Loads: {
//                            const auto frameOffset = subroutine->parseUint16();
//                            const auto varIndex = subroutine->parseUint16();
//                            if (frameOffset == 0) {
//                                push(stack[frames.back().initialSp + varIndex]);
//                            } else if (frameOffset == 1) {
//                                push(stack[frames[frames.size() - 2].initialSp + varIndex]);
//                            } else if (frameOffset == 2) {
//                                push(stack[frames[frames.size() - 2].initialSp + varIndex]);
//                            } else {
//                                throw runtime_error("frame offset not implement");
//                            }
////                            debug("load var {}/{}", frameOffset, varIndex);
//                            break;
//                        }
//                        case OP::Frame: {
//                            pushFrame();
//                            break;
//                        }
//                        case OP::FunctionRef: {
//                            const auto address = subroutine->parseUint32();
//                            push(make_shared<TypeFunctionRef>(address));
//                            break;
//                        }
//                        case OP::Dup: {
//                            push(last());
//                            break;
//                        }
//                        case OP::Widen: {
//                            push(widen(pop()));
//                            break;
//                        }
//                        case OP::Set: {
//                            const auto address = subroutine->parseUint32();
//                            subroutine->module->getSubroutine(address)->narrowed = pop();
//                            break;
//                        }
//                        case OP::Instantiate: {
//                            const auto arguments = subroutine->parseUint16();
//                            auto ref = pop(); //FunctionRef/Class
//
//                            switch (ref->kind) {
//                                case TypeKind::FunctionRef: {
//                                    auto a = to<TypeFunctionRef>(ref);
//                                    call(subroutine->module, a->address, arguments);
//                                    break;
//                                }
//                                default: {
//                                    throw runtime_error(fmt::format("Can not instantiate {}", ref->kind));
//                                }
//                            }
//                            break;
//                        }
//                        case OP::PropertySignature: {
//                            auto name = pop();
//                            auto type = pop();
//                            auto prop = pool.makePropertySignature(type);
//                            auto valid = true;
//                            switch (name->kind) {
//                                case TypeKind::Literal: {
//                                    prop->name = to<TypeLiteral>(name)->text(); //do we need a copy?
//                                    break;
//                                }
//                                default: {
//                                    valid = false;
//                                    report("A computed property name in an interface must refer to an expression whose type is a literal type or a 'unique symbol' type", type);
//                                }
//                            }
//                            if (valid) {
//                                push(prop);
//                            }
//                            break;
//                        }
//                        case OP::ObjectLiteral: {
//                            auto types = popFrame();
//                            auto objectLiteral = pool.makeObjectLiteral();
//                            pushObjectLiteralTypes(objectLiteral, types);
//                            push(objectLiteral);
//                            break;
//                        }
//                        case OP::CallExpression: {
//                            const auto parameterAmount = subroutine->parseUint16();
//
//                            span<node<Type>> parameters{stack.data() + frames.back().sp - parameterAmount, parameterAmount};
//                            frames.back().sp -= parameterAmount;
//
//                            auto typeToCall = pop();
//                            switch (typeToCall->kind) {
//                                case TypeKind::Function: {
//                                    auto fn = to<TypeFunction>(typeToCall);
//                                    //it's important to handle parameters/typeArguments first before changing the stack with push()
//                                    //since we have a span.
//                                    auto end = fn->parameters.size();
//                                    for (unsigned int i = 0; i < end; i++) {
//                                        auto parameter = fn->parameters[i];
//                                        auto optional = isOptional(parameter);
//                                        if (i > parameters.size() - 1) {
//                                            //parameter not provided
//                                            if (!optional && !parameter->initializer) {
//                                                report(fmt::format("An argument for '{}' was not provided.", parameter->name), parameter);
//                                            }
//                                            break;
//                                        }
//                                        auto lvalue = parameters[i];
//                                        auto rvalue = reinterpret_pointer_cast<Type>(parameter);
//                                        ExtendableStack stack;
//                                        if (!isExtendable(lvalue, rvalue, stack)) {
//                                            //rerun again with
//                                            report(stack.errorMessage());
//                                        }
//                                    }
//
//                                    //we could convert parameters to a tuple and then run isExtendable() on two tuples
//                                    break;
//                                }
//                                default: {
//                                    throw runtime_error(fmt::format("CallExpression on {} not handled", typeToCall->kind));
//                                }
//                            }
//                            break;
//                        }
//                        case OP::Call: {
//                            const auto address = subroutine->parseUint32();
//                            const auto arguments = subroutine->parseUint16();
//                            call(subroutine->module, address, arguments);
//                            break;
//                        }
//                        case OP::Function: {
//                            //OP::Function has always its own subroutine, so it doesn't have it so own stack frame.
//                            //thus we readFrame() and not popFrame() (since Op::Return pops frame already).
//                            auto types = readFrame();
//                            auto function = make_shared<TypeFunction>();
//                            if (types.size() > 1) {
//                                auto end = std::prev(types.end());
//                                for (auto iter = types.begin(); iter != end; ++iter) {
//                                    function->parameters.push_back(reinterpret_pointer_cast<TypeParameter>(*iter));
//                                }
//                            } else if (types.empty()) {
//                                throw runtime_error("No types given for function");
//                            }
//                            function->returnType = types.back();
//                            push(function);
//                            break;
//                        }
//                        case OP::Parameter: {
//                            const auto address = subroutine->parseUint32();
//                            auto name = readStorage(bin, address);
//                            push(make_shared<TypeParameter>(name, pop()));
//                            break;
//                        }
//                        case OP::Assign: {
//                            auto rvalue = pop();
//                            auto lvalue = pop();
//                            ExtendableStack stack;
//                            if (!isExtendable(lvalue, rvalue, stack)) {
//                                auto error = stack.errorMessage();
//                                error.ip = ip;
//                                report(error);
//                            }
//                            break;
//                        }
//                        case OP::IndexAccess: {
//                            auto right = pop();
//                            auto left = pop();
//
////                            if (!isType(left)) {
////                                push({ kind: TypeKind::never });
////                            } else {
//
//                            //todo: we have to put all members of `left` on the stack, since subroutines could be required
//                            // to resolve super classes.
//                            auto t = indexAccess(left, right);
////                                if (isWithAnnotations(t)) {
////                                    t.indexAccessOrigin = { container: left as TypeObjectLiteral, index: right as Type };
////                                }
//
////                                t.parent = undefined;
//                            push(t);
////                            }
//                            break;
//                        }
//                        case OP::Rest: {
//                            push(make_shared<TypeRest>(pop()));
//                            break;
//                        }
//                        case OP::Array: {
//                            push(make_shared<TypeArray>(pop()));
//                            break;
//                        }
//                        case OP::TupleMember: {
//                            push(pool.makeTupleMember(pop()));
//                            break;
//                        }
//                        case OP::TupleNamedMember: {
//                            break;
//                        }
//                        case OP::Tuple: {
//                            auto types = popFrame();
//
//                            //short path for [...x, y];
////                            if (types.size() == 2 && to<TypeTupleMember>(types[0])->type->kind == TypeKind::Rest) {
////                                //todo: we need a heuristic to determine if the array `x` actually can be mutated
////                                auto t = to<TypeRest>(to<TypeTupleMember>(types[0])->type);
////                                if (t->type->kind == TypeKind::Tuple && to<TypeTupleMember>(types[1])->type->kind != TypeKind::Rest) {
////                                    auto result = to<TypeTuple>(t->type);
////                                    result->types.push_back(to<TypeTupleMember>(types[1]));
////                                    push(result);
////                                    break;
////                                }
////                            }
//
////                            auto tuple = make_shared<TypeTuple>();
////                            for (auto &&type: types) {
////                                if (type->kind != TypeKind::TupleMember) {
////                                    debug("No tuple member in stack frame, but {}", type->kind);
////                                    continue;
////                                }
////                                auto member = reinterpret_pointer_cast<TypeTupleMember>(type);
////                                if (member->type->kind == TypeKind::Rest) {
////                                    auto rest = to<TypeRest>(member->type);
////                                    if (rest->type->kind == TypeKind::Tuple) {
////                                        for (auto &&sub: to<TypeTuple>(rest->type)->types) {
////                                            tuple->types.push_back(sub);
////                                        }
////                                    } else {
////                                        tuple->types.push_back(member);
////                                    }
////                                } else {
////                                    tuple->types.push_back(member);
////                                }
////                            }
////                            push(tuple);
//                            push(make_shared<TypeUnknown>());
//                            break;
//                        }
//                        case OP::Initializer: {
//                            auto t = pop();
//                            auto l = last();
//                            switch (l->kind) {
//                                case TypeKind::Parameter:to<TypeParameter>(l)->initializer = t;
//                                    break;
//                            }
//                            break;
//                        }
//                        case OP::Optional: {
//                            auto l = last();
//                            switch (l->kind) {
//                                case TypeKind::TupleMember: to<TypeTupleMember>(l)->optional = true;
//                                    break;
//                                case TypeKind::Parameter: to<TypeParameter>(l)->optional = true;
//                                    break;
//                                case TypeKind::PropertySignature: to<TypePropertySignature>(l)->optional = true;
//                                    break;
//                            }
//                            break;
//                        }
//                        case OP::Readonly: {
//                            auto l = last();
//                            switch (l->kind) {
//                                case TypeKind::PropertySignature: to<TypePropertySignature>(l)->readonly = true;
//                                    break;
//                            }
//                            break;
//                        }
//                        case OP::True: push(make_shared<TypeLiteral>("true", TypeLiteralType::Boolean));
//                            break;
//                        case OP::False: push(make_shared<TypeLiteral>("false", TypeLiteralType::Boolean));
//                            break;
//                        case OP::String: push(make_shared<TypeString>());
//                            break;
//                        case OP::Number: push(make_shared<TypeNumber>());
//                            break;
//                        case OP::Boolean: push(make_shared<TypeBoolean>());
//                            break;
//                        case OP::Unknown: {
//                            pool.makeUnknown();
////                            push();
//                            break;
//                        }
//                        case OP::Undefined: push(make_shared<TypeUndefined>());
//                            break;
//                        case OP::Void: push(make_shared<TypeVoid>());
//                            break;
//                        case OP::Never: push(make_shared<TypeNever>());
//                            break;
//                        case OP::Any: push(make_shared<TypeAny>());
//                            break;
//                        case OP::Symbol: push(make_shared<TypeSymbol>());
//                            break;
//                        case OP::Object: push(make_shared<TypeObject>());
//                            break;
//                        case OP::Union: {
//                            auto types = popFrame();
//                            if (types.size() == 2) {
//                                //convert `true|false` into `boolean
//                                if (types[0]->kind == TypeKind::Literal && types[1]->kind == TypeKind::Literal) {
//                                    auto first = to<TypeLiteral>(types[0]);
//                                    auto second = to<TypeLiteral>(types[1]);
//                                    if (first->type == TypeLiteralType::Boolean && second->type == TypeLiteralType::Boolean) {
//                                        if (!first->equal(second)) {
//                                            push(make_shared<TypeBoolean>());
//                                            break;
//                                        }
//                                    }
//                                }
//                            }
////                            auto t = make_shared<TypeUnion>();
////                            t->types.insert(t->types.begin(), types.begin(), types.end());
////                            push(t);
//                            push(make_shared<TypeUnknown>());
//                            break;
//                        }
//                        case OP::NumberLiteral: {
//                            const auto address = subroutine->parseUint32();
//                            push(make_shared<TypeLiteral>(readStorage(bin, address), TypeLiteralType::Number));
//                            break;
//                        }
//                        case OP::BigIntLiteral: {
//                            const auto address = subroutine->parseUint32();
//                            push(make_shared<TypeLiteral>(readStorage(bin, address), TypeLiteralType::Bigint));
//                            break;
//                        }
//                        case OP::StringLiteral: {
////                            const auto address = subroutine->parseUint32();
//                            subroutine->ip += 4;
////                            auto text = readStorage(bin, address);
//                            push(pool.makeTypeLiteral("", TypeLiteralType::String));
////                            push(make_shared<TypeLiteral>());
//                            break;
//                        }
//                        case OP::Error: {
//                            const auto code = (instructions::ErrorCode) subroutine->parseUint16();
//                            switch (code) {
//                                case ErrorCode::CannotFind: {
//                                    report(DiagnosticMessage(fmt::format("Cannot find name '{}'", subroutine->module->findIdentifier(ip)), ip));
//                                    break;
//                                }
//                                default: {
//                                    report(DiagnosticMessage(fmt::format("{}", code), ip));
//                                }
//                            }
//                            break;
//                        }
//                        default: {
//                            throw runtime_error(fmt::format("unhandled op {}", op));
//                        }
                    }

                    if (stepper) {
                        if (op == instructions::TypeArgument) {
//                            frames.back().variableIPs.push_back(ip);
                        }
                        subroutine->ip++;
//                        debug("Routine {} (ended={})", subroutine->depth, subroutine->ip == subroutine->end);
                        if (subroutine->ip == subroutine->end) {
                            //subroutine is done
                            subroutine = subroutine->previous;
                        }
                        //process() needs to be executed for each step
                        return;
                    }
                }

                subroutine = subroutine->previous;
            }
        }

        void handleTemplateLiteral() {
            auto types = popFrame();

            //short path for `{'asd'}`
            if (types.size() == 1 && types[0]->kind == TypeKind::Literal) {
                //we can not just change the TypeLiteral->type, we have to create a new one
//                to<TypeLiteral>(types[0])->type = TypeLiteralType::String;
                auto t = to<TypeLiteral>(types[0]);
                auto res = make_shared<TypeLiteral>(t->literal, TypeLiteralType::String);
                if (t->dynamicString) {
                    res->dynamicString = new string(*t->dynamicString);
                }
                push(res);
                return;
            }

            auto result = make_shared<TypeUnion>();
            CartesianProduct cartesian;
            for (auto &&type: types) {
                cartesian.add(type);
            }
            auto product = cartesian.calculate();

            outer:
            for (auto &&combination: product) {
                auto templateType = make_shared<TypeTemplateLiteral>();
                bool hasPlaceholder = false;
//                let lastLiteral: { kind: TypeKind::literal, literal: string, parent?: Type } | undefined = undefined;
                optionalNode<TypeLiteral> lastLiteral;

                //merge a combination of types, e.g. [string, 'abc', '3'] as template literal => `${string}abc3`.
                for (auto &&item: combination) {
                    if (item->kind == TypeKind::Never) {
                        //template literals that contain a never like `prefix.${never}` are completely ignored
                        goto outer;
                    }

                    if (item->kind == TypeKind::Literal) {
                        if (lastLiteral) {
                            lastLiteral->append(to<TypeLiteral>(item)->text());
                        } else {
                            lastLiteral = make_shared<TypeLiteral>("", TypeLiteralType::String);
                            lastLiteral->append(to<TypeLiteral>(item)->text());
                            templateType->types.push_back(lastLiteral);
                        }
                    } else {
                        hasPlaceholder = true;
                        lastLiteral = nullptr;
//                        item.parent = template;
                        templateType->types.push_back(item);
                    }
                }

                if (hasPlaceholder) {
                    if (templateType->types.size() == 1 && templateType->types[0]->kind == TypeKind::String) {
//                        templateType->types[0].parent = result;
                        result->types.push_back(templateType->types[0]);
                    } else {
//                        templateType->parent = result;
                        result->types.push_back(templateType);
                    }
                } else if (lastLiteral) {
//                    lastLiteral.parent = result;
                    result->types.push_back(lastLiteral);
                }
            }

            auto t = vm::unboxUnion(result);
//        if (t.kind == TypeKind::union) for (const member of t.types) member.parent = t;
//        debug("handleTemplateLiteral: {}", stringify(t));
            push(t);
        }

        void pushObjectLiteralTypes(node<TypeObjectLiteral> &type, const span<node<Type>> &types) {
            for (auto &&member: types) {
                switch (member->kind) {
                    case TypeKind::Never: {
                        break;
                    }
                    case TypeKind::ObjectLiteral: {
                        pushObjectLiteralTypes(type, to<TypeObjectLiteral>(member)->types);
                        break;
                    }
                    case TypeKind::IndexSignature: {
                        //note: is it possible to overwrite an index signature?
                        type->types.push_back(member);
                        break;
                    }
                    case TypeKind::MethodSignature:
                    case TypeKind::PropertySignature: {
                        type->types.push_back(member);
                        break;
                    }
                    case TypeKind::Method: {
                        //todo: convert to MethodSignature
                        throw runtime_error("Method to MethodProperty not implemented yet");
                        break;
                    }
                    case TypeKind::Property: {
                        //todo: convert to PropertySignature
                        throw runtime_error("Property to PropertySignature not implemented yet");
                        break;
                    }
                }
            }
        }
    };
}