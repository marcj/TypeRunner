#pragma once

#include "./type_objects.h"
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

namespace ts::vm {
    using namespace ts::checker;
    using std::string_view;
    using std::span;
    using std::vector;
    using std::runtime_error;
    using instructions::OP;

    inline string_view readStorage(const string_view &bin, const uint32_t offset) {
        const auto size = readUint16(bin, offset);
        return string_view(reinterpret_cast<const char *>(bin.data() + offset + 2), size);
    }

    inline bool isConditionTruthy(shared<Type> node) {
        if (auto n = to<TypeLiteral>(node)) return n->text() == "true";
        return false;
    }

    struct CStack {
        vector<shared<Type>> iterator;
        unsigned int i;
        unsigned int round;
    };

    class CartesianProduct {
        vector<CStack> stack;
    public:

        shared<Type> current(CStack &s) {
            return s.iterator[s.i];
        }

        bool next(CStack &s) {
            return (++s.i == s.iterator.size()) ? (s.i = 0, false) : true;
        }

        vector<shared<Type>> toGroup(shared<Type> &type) {
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
                vector<shared<Type>> result;
                for (auto &&v: to<TypeUnion>(type)->types) {
                    auto g = toGroup(v);
                    for (auto &&s: g) result.push_back(s);
                }

                return result;
            } else {
                return {type};
            }
        }

        void add(shared<Type> &item) {
            stack.push_back({.iterator=toGroup(item), .i= 0, .round= 0});
        }

        vector<vector<shared<Type>>> calculate() {
            vector<vector<shared<Type>>> result;

            outer:
            while (true) {
                vector<shared<Type>> row;
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
    shared<Type> indexAccess(shared<Type> &container, shared<Type> &index) {
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
        Frame *previous = nullptr;

        unsigned int size() {
            return sp - initialSp;
        }

        explicit Frame(Frame *previous = nullptr): previous(previous) {
            if (previous) {
                sp = previous->sp;
                initialSp = previous->sp;
            }
        }
    };

    struct ModuleSubroutine {
        string_view name;
        bool exported = false;
        unsigned int address;
        sharedOpt<Type> result = nullptr;
        sharedOpt<Type> narrowed = nullptr; //when control flow analysis sets a new value
    };

    struct Module {
        const string_view &bin;
        vector<ModuleSubroutine> subroutines;
        unsigned int mainAddress;

        Module(const string_view &bin): bin(bin) {
        }

        ModuleSubroutine &getSubroutine(unsigned int index) {
            return subroutines[index];
        }

        ModuleSubroutine &getMain() {
            return subroutines.back();
        }
    };

    /**
     * For each active subroutine this object is created.
     */
    struct ProgressingSubroutine {
        Module &module;
        ModuleSubroutine &subroutine;

        unsigned int ip = 0; //instruction pointer
        unsigned int end = 0; //last instruction pointer
        unsigned int index = 0;

        unsigned int depth = 0;
        bool active = true;
        unsigned int typeArguments = 0;

        sharedOpt<ProgressingSubroutine> previous = nullptr;

        explicit ProgressingSubroutine(Module &module, ModuleSubroutine &subroutine): module(module), subroutine(subroutine) {}

        uint32_t parseUint32() {
            auto val = readUint32(module.bin, ip + 1);
            ip += 4;
            return val;
        }

        uint16_t parseUint16() {
            auto val = readUint16(module.bin, ip + 1);
            ip += 2;
            return val;
        }
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

    shared<Type> stringToNum(VM &vm);

    inline Module parseHeader(const string_view &bin) {
        Module module(bin);

        for (unsigned int i = 0; i < bin.size(); i++) {
            const auto op = (OP) bin[i];
            switch (op) {
                case OP::Jump: {
                    i = readUint32(bin, i + 1) - 1; //minus 1 because for's i++
                    break;
                }
                case OP::Subroutine: {
                    unsigned int nameAddress = readUint32(bin, i + 1);
                    auto name = nameAddress ? readStorage(bin, nameAddress) : "";
                    unsigned int address = readUint32(bin, i + 5);
                    i += 8;
                    module.subroutines.push_back(ModuleSubroutine{
                            .name = name,
                            .address = address,
                    });
                    break;
                }
                case OP::Main: {
                    module.mainAddress = readUint32(bin, i + 1);
                    module.subroutines.push_back(ModuleSubroutine{
                            .name = "main",
                            .address = module.mainAddress,
                    });
                    return module;
                }
            }
        }

        throw runtime_error("No OP::Main found");
    }

    class VM {
    public:
        vector<string> errors;
        /**
         * Linked list of subroutines to execute. For each external call this subroutine will be changed.
         */
        sharedOpt<ProgressingSubroutine> subroutine;
        Frame *frame = nullptr;

        vector<shared<Type>> stack;

        unordered_map<unsigned int, function<shared<Type>(VM &)>> optimised;

        VM() {
            delete frame;
//            stack.reserve(1000);
        }

        void printErrors() {
            debug("errors ({}):", errors.size());
            for (auto &&e: errors) {
                debug("  " + e);
            }
        }

        void run(const string_view &bin) {
            auto module = parseHeader(bin);

            if (subroutine) throw runtime_error("Subroutine already running");

            auto next = make_shared<ProgressingSubroutine>(module, module.getMain());
            next->ip = module.mainAddress;
            next->end = module.bin.size();
            next->previous = subroutine;
            subroutine = next;
            frame = new Frame(frame);

            process();
        }

        void call(Module &module, unsigned int index = 0, unsigned int arguments = 0) {
            //todo: check if address was already calculated using unordered_map?
            const auto loopRunning = !!subroutine;
            if (!loopRunning) {
                errors.clear();
            }

//            auto found = std::equal(bin.begin() + address, bin.begin() + address + sizeof(emptyTuple), emptyTuple);
//            if (found) {
////                debug("found! {}", address);
//                push(make_shared<TypeTuple>());
//                return;
//            }
//                }

            auto routine = module.getSubroutine(index);
            if (routine.narrowed) {
                push(routine.narrowed);
                return;
            }
            if (routine.result && arguments == 0) {
                push(routine.result);
                return;
            }

            auto next = make_shared<ProgressingSubroutine>(module, routine);
            next->ip = routine.address;
            next->end = module.bin.size();
            next->previous = subroutine;
            next->depth = subroutine ? subroutine->depth + 1 : 0;

            frame = new Frame(frame);

            if (arguments) {
                //we move x arguments from the old stack to the new one
                frame->previous->sp -= arguments;
                frame->initialSp -= arguments;
            }

            if (loopRunning) subroutine->ip++; //`subroutine` is set to something new in next line, so for() increments its ip, not ours
            subroutine = next;
            if (loopRunning) subroutine->ip--; //`subroutine` is set to something new, so for() increments its ip, which we don't want

            if (!loopRunning) process();

//            print();
//            return next->resultType;
        }

//        span<shared<Type>> popFrame() {
//            span<shared<Type>> sub{stack.data() + frame->initialSp, frame->sp - frame->initialSp};
////            auto sub = slice(stack, frame->initialSp, frame->sp);
//            stack.resize(frame->initialSp);
//            frame = frame->previous;
//            if (!frame) throw runtime_error("Invalid pop frame");
//            return sub;
//        }

        /**
         * Read frame types without popping frame.
         */
        span<shared<Type>> readFrame() {
            auto start = frame->initialSp + frame->variables;
            return {stack.data() + start, frame->sp - start};
        }

        span<shared<Type>> popFrame() {
//            auto frameSize = frame->initialSp - frame->sp;
//            while (frameSize--) stack.pop();
            auto start = frame->initialSp + frame->variables;
            span<shared<Type>> sub{stack.data() + start, frame->sp - start};
//            std::span<shared<Type>> sub{stack.begin() + frame->initialSp, frameSize};
            auto previous = frame->previous;
            delete frame;
            frame = previous;
            return sub;
        }

        unsigned int frameSize() {
            return frame->initialSp - frame->sp;
        }
//
//        void popFrame(function<void(shared<Type>)> callback) {
//            auto frameSize = frame->initialSp - frame->sp;
//            while (frameSize-- ) {
//                auto &item = stack.top();
//                callback(item);
//                stack.pop();
//            }
//            frame = frame->previous;
//        }

        void pushFrame() {
            frame = new Frame(frame);
        }

        shared<Type> &last() {
            if (frame->sp == 0) throw runtime_error("stack is empty. Can not return last");
            return stack[frame->sp - 1];
        }

        shared<Type> &pop() {
            if (frame->sp == 0) throw runtime_error("stack is empty. Can not pop");
            frame->sp--;
            if (frame->sp < frame->initialSp) {
                throw runtime_error("Popped through frame");
            }
            return stack[frame->sp];
        }

        sharedOpt<Type> frameEntryAt(unsigned int position) {
            auto i = frame->initialSp + position;
            if (i > frame->sp) return nullptr;
            return stack[i];
        }

        void push(const shared<Type> &type) {
            if (frame->sp >= stack.size()) {
                stack.push_back(type);
            } else {
                stack[frame->sp] = type;
            }
            frame->sp++;
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

            auto currentFrame = frame;
            auto frameId = 0;
            while (currentFrame) {
                debug("  # Frame {}: {} ({}->{}) stack frame entries ({})",
                      frameId, currentFrame->sp - currentFrame->initialSp, currentFrame->initialSp, currentFrame->sp,
                      currentFrame->loop ? "loop" : "");
                if (currentFrame->sp && currentFrame->initialSp != currentFrame->sp) {
//                    auto sub = slice(stack, currentFrame->initialSp, currentFrame->sp);
//                    auto j = currentFrame->initialSp;
//                    for (auto &&i: sub) {
//                        debug("  - {}: {}", j++, i ? stringify(i) : "empty");
//                    }
                }
                currentFrame = currentFrame->previous;
                frameId++;
            }
        }

        void process() {
            while (subroutine) {
                for (; subroutine->active && subroutine->ip < subroutine->end; subroutine->ip++) {
                    const auto op = (OP) subroutine->module.bin[subroutine->ip];
//                    debug("[{}] OP {} ({} -> {})", subroutine->depth, op, subroutine->ip, (unsigned int)op);

                    switch (op) {
                        case OP::Jump: {
                            subroutine->ip = readUint32(subroutine->module.bin, 1) - 1; //minus 1 because for's i++
                            break;
                        }
                        case OP::Extends: {
                            auto right = pop();
                            auto left = pop();
//                            debug("{} extends {} => {}", stringify(left), stringify(right), isAssignable(right, left));
                            const auto valid = isExtendable(left, right);
                            push(make_shared<TypeLiteral>(valid ? "true" : "false", TypeLiteralType::Boolean));
                            break;
                        }
                        case OP::Distribute: {
                            if (!frame->loop) {
                                auto type = pop();
                                pushFrame();
                                frame->loop = make_shared<LoopHelper>(type);
                            }

                            auto next = frame->loop->next();
                            if (!next) {
                                //done
                                auto types = popFrame();
                                if (types.empty()) {
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
                                const auto loopProgram = readUint32(subroutine->module.bin, subroutine->ip + 1);
                                subroutine->ip--; //we jump back if the loop is not done, so that this section is executed when the following call() is done
                                push(next);
                                call(subroutine->module, loopProgram, 1);
                            }
                            break;
                        }
                        case OP::JumpCondition: {
                            auto condition = pop();
                            const auto leftProgram = subroutine->parseUint16();
                            const auto rightProgram = subroutine->parseUint16();
//                            debug("{} ? {} : {}", stringify(condition), leftProgram, rightProgram);
                            call(subroutine->module, isConditionTruthy(condition) ? leftProgram : rightProgram);
                            break;
                        }
                        case OP::Return: {
                            auto previous = frame->previous;
                            //the current frame could not only have the return value, but variables and other stuff,
                            //which we don't want. So if size is bigger than 1, we move last stack entry to first
                            // | [T] [T] [R] |
                            if (frame->size() > 1) {
                                stack[frame->initialSp] = stack[frame->sp - 1];
                            }
                            previous->sp++; //consume the new stack entry from the function call, making it part of the caller frame
                            delete frame;
                            frame = previous;
                            subroutine->active = false;
                            subroutine->ip++; //we jump directly to another subroutine, so for()'s increment doesn't apply
                            subroutine = subroutine->previous;
                            subroutine->ip--; //for()'s increment applies the wrong subroutine, so we decrease first
                            if (subroutine->typeArguments == 0) subroutine->subroutine.result = last();
//                            debug("routine {} result: {}", subroutine->name, vm::stringify(last()));
                            break;
                        }
                        case OP::TypeArgument: {
                            if (frame->size() <= subroutine->typeArguments) {
                                auto unknown = make_shared<TypeUnknown>();
                                unknown->unprovidedArgument = true;
                                push(unknown);
                            }
                            subroutine->typeArguments++;
                            frame->variables++;
                            break;
                        }
                        case OP::TypeArgumentDefault: {
                            auto t = frameEntryAt(subroutine->typeArguments - 1);
                            //t is always set because TypeArgument ensures that
                            if (t->kind == TypeKind::Unknown && to<TypeUnknown>(t)->unprovidedArgument) {
                                frame->sp--; //remove unknown type from stack
                                const auto address = subroutine->parseUint32();
                                call(subroutine->module, address, 0); //the result is pushed on the stack
                            } else {
                                subroutine->ip += 4; //jump over address
                            }
                            break;
                        }
                        case OP::TemplateLiteral: {
                            handleTemplateLiteral();
                            break;
                        }
                        case OP::Var: {
                            frame->variables++;
                            push(make_shared<TypeUnknown>());
                            break;
                        }
                        case OP::Loads: {
                            const auto frameOffset = subroutine->parseUint16();
                            const auto varIndex = subroutine->parseUint16();
                            if (frameOffset == 0) {
                                push(stack[frame->initialSp + varIndex]);
                            } else if (frameOffset == 1) {
                                push(stack[frame->previous->initialSp + varIndex]);
                            } else if (frameOffset == 2) {
                                push(stack[frame->previous->previous->initialSp + varIndex]);
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
                        case OP::FunctionRef: {
                            const auto address = subroutine->parseUint32();
                            push(make_shared<TypeFunctionRef>(address));
                            break;
                        }
                        case OP::Dup: {
                            push(last());
                            break;
                        }
                        case OP::Widen: {
                            push(widen(pop()));
                            break;
                        }
                        case OP::Set: {
                            const auto address = subroutine->parseUint32();
                            subroutine->module.getSubroutine(address).narrowed = pop();
                            break;
                        }
                        case OP::Instantiate: {
                            const auto arguments = subroutine->parseUint16();
                            auto ref = pop(); //FunctionRef/Class

                            switch (ref->kind) {
                                case TypeKind::FunctionRef: {
                                    auto a = to<TypeFunctionRef>(ref);
                                    call(subroutine->module, a->address, arguments);
                                    break;
                                }
                                default: {
                                    throw runtime_error(fmt::format("Can not instantiate {}", ref->kind));
                                }
                            }
                            break;
                        }
                        case OP::CallExpression: {
                            const auto parameterAmount = subroutine->parseUint16();

                            span<shared<Type>> parameters{stack.data() + frame->sp - parameterAmount, parameterAmount};
                            frame->sp -= parameterAmount;

                            auto typeToCall = pop();
                            switch (typeToCall->kind) {
                                case TypeKind::Function: {
                                    auto fn = to<TypeFunction>(typeToCall);
                                    //it's important to handle parameters/typeArguments first before changing the stack with push()
                                    //since we have a span.
                                    auto end = fn->parameters.size();
                                    for (unsigned int i = 0; i < end; i++) {
                                        auto parameter = fn->parameters[i];
                                        auto optional = isOptional(parameter);
                                        if (i > parameters.size() - 1) {
                                            //parameter not provided
                                            if (!optional && !parameter->initializer) {
                                                errors.push_back(fmt::format("An argument for '{}' was not provided.", parameter->name));
                                            }
                                            break;
                                        }
                                        auto lvalue = parameters[i];
                                        auto rvalue = reinterpret_pointer_cast<Type>(parameter);
                                        if (!isExtendable(lvalue, rvalue)) {
                                            errors.push_back(fmt::format("Argument of type '{}' is not assignable to parameter of type '{}'", stringify(lvalue), stringify(rvalue)));
                                        }
                                    }

                                    //we could convert parameters to a tuple and then run isExtendable() on two tuples
                                    break;
                                }
                                default: {
                                    throw runtime_error(fmt::format("CallExpression on {} not handled", typeToCall->kind));
                                }
                            }
                            break;
                        }
                        case OP::Call: {
                            const auto address = subroutine->parseUint32();
                            const auto arguments = subroutine->parseUint16();
                            call(subroutine->module, address, arguments);
                            break;
                        }
                        case OP::Function: {
                            //OP::Function has always its own subroutine, so it doesn't have it so own stack frame.
                            //thus we readFrame() and not popFrame() (since Op::Return pops frame already).
                            auto types = readFrame();
                            auto function = make_shared<TypeFunction>();
                            if (types.size() > 1) {
                                auto end = std::prev(types.end());
                                for (auto iter = types.begin(); iter != end; ++iter) {
                                    function->parameters.push_back(reinterpret_pointer_cast<TypeParameter>(*iter));
                                }
                            } else if (types.empty()) {
                                throw runtime_error("No types given for function");
                            }
                            function->returnType = types.back();
                            push(function);
                            break;
                        }
                        case OP::Parameter: {
                            const auto address = subroutine->parseUint32();
                            auto name = readStorage(subroutine->module.bin, address);
                            push(make_shared<TypeParameter>(name, pop()));
                            break;
                        }
                        case OP::Assign: {
                            auto rvalue = pop();
                            auto lvalue = pop();
                            if (!isExtendable(lvalue, rvalue)) {
                                errors.push_back(fmt::format("{}={} not assignable!", stringify(rvalue), stringify(lvalue)));
                            }
                            break;
                        }
                        case OP::IndexAccess: {
                            auto right = pop();
                            auto left = pop();

//                            if (!isType(left)) {
//                                push({ kind: TypeKind::never });
//                            } else {

                            //todo: we have to put all members of `left` on the stack, since subroutines could be required
                            // to resolve super classes.
                            auto t = indexAccess(left, right);
//                                if (isWithAnnotations(t)) {
//                                    t.indexAccessOrigin = { container: left as TypeObjectLiteral, index: right as Type };
//                                }

//                                t.parent = undefined;
                            push(t);
//                            }
                            break;
                        }
                        case OP::Rest: {
                            push(make_shared<TypeRest>(pop()));
                            break;
                        }
                        case OP::TupleMember: {
                            push(make_shared<TypeTupleMember>(pop()));
                            break;
                        }
                        case OP::TupleNamedMember: {
                            break;
                        }
                        case OP::Tuple: {
                            auto types = popFrame();

                            //short path for [...x, y];
                            if (types.size() == 2 && to<TypeTupleMember>(types[0])->type->kind == TypeKind::Rest) {
                                //todo: we need a heuristic to determine if the array `x` actually can be mutated
                                auto t = to<TypeRest>(to<TypeTupleMember>(types[0])->type);
                                if (t->type->kind == TypeKind::Tuple && to<TypeTupleMember>(types[1])->type->kind != TypeKind::Rest) {
                                    auto result = to<TypeTuple>(t->type);
                                    result->types.push_back(to<TypeTupleMember>(types[1]));
                                    push(result);
                                    break;
                                }
                            }

                            auto tuple = make_shared<TypeTuple>();
                            for (auto &&type: types) {
                                if (type->kind != TypeKind::TupleMember) {
                                    debug("No tuple member in stack frame, but {}", type->kind);
                                    continue;
                                }
                                auto member = reinterpret_pointer_cast<TypeTupleMember>(type);
                                if (member->type->kind == TypeKind::Rest) {
                                    auto rest = to<TypeRest>(member->type);
                                    if (rest->type->kind == TypeKind::Tuple) {
                                        for (auto &&sub: to<TypeTuple>(rest->type)->types) {
                                            tuple->types.push_back(sub);
                                        }
                                    } else {
                                        tuple->types.push_back(member);
                                    }
                                } else {
                                    tuple->types.push_back(member);
                                }
                            }
                            push(tuple);
                            break;
                        }
                        case OP::Initializer: {
                            auto t = pop();
                            auto l = last();
                            switch (l->kind) {
                                case TypeKind::Parameter:to<TypeParameter>(l)->initializer = t;
                                    break;
                            }
                            break;
                        }
                        case OP::Optional: {
                            auto l = last();
                            switch (l->kind) {
                                case TypeKind::TupleMember:to<TypeTupleMember>(l)->optional = true;
                                    break;
                                case TypeKind::Parameter:to<TypeParameter>(l)->optional = true;
                                    break;
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
                            auto types = popFrame();
                            if (types.size() == 2) {
                                if (types[0]->kind == TypeKind::Literal && types[1]->kind == TypeKind::Literal) {
                                    auto first = to<TypeLiteral>(types[0]);
                                    auto second = to<TypeLiteral>(types[1]);
                                    if (first->type == TypeLiteralType::Boolean && second->type == TypeLiteralType::Boolean) {
                                        if (first->text() != second->text()) {
                                            push(make_shared<TypeBoolean>());
                                            break;
                                        }
                                    }
                                }
                            }
                            auto t = make_shared<TypeUnion>();
                            for (auto &&v: types) t->types.push_back(v);
                            push(t);
                            break;
                        }
                        case OP::NumberLiteral: {
                            const auto address = subroutine->parseUint32();
                            push(make_shared<TypeLiteral>(readStorage(subroutine->module.bin, address), TypeLiteralType::Number));
                            break;
                        }
                        case OP::BigIntLiteral: {
                            const auto address = subroutine->parseUint32();
                            push(make_shared<TypeLiteral>(readStorage(subroutine->module.bin, address), TypeLiteralType::Bigint));
                            break;
                        }
                        case OP::StringLiteral: {
                            const auto address = subroutine->parseUint32();
                            auto text = readStorage(subroutine->module.bin, address);
                            push(make_shared<TypeLiteral>(text, TypeLiteralType::String));
                            break;
                        }
                        default: {
                            throw runtime_error(fmt::format("unhandled op {}", op));
                        }
                    }
                }

                subroutine = subroutine->previous;
            }

//            return stack.back();
        }

        void handleTemplateLiteral() {
            auto types = popFrame();

            //short path for `{'asd'}`
            if (types.size() == 1 && types[0]->kind == TypeKind::Literal) {
                //we can not just change the TypeLiteral->type, we have to create a new one
//                to<TypeLiteral>(types[0])->type = TypeLiteralType::String;
                auto t = to<TypeLiteral>(types[0]);
                auto res = make_shared<TypeLiteral>(t->literal, TypeLiteralType::String);
                if (t->literalText) {
                    res->literalText = new string(*t->literalText);
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
                sharedOpt<TypeLiteral> lastLiteral;

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
    };
}