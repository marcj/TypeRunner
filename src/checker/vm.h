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

        vector<shared<Type>> toGroup(shared<Type> type) {
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

        void add(shared<Type> item) {
            stack.push_back({iterator: toGroup(item), i: 0, round: 0});
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
    shared<Type> indexAccess(shared<Type> container, shared<Type> index) {
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

        Frame(Frame *previous = nullptr): previous(previous) {
            if (previous) {
                sp = previous->sp;
                initialSp = previous->sp;
            }
        }
    };

    struct Subroutine {
        const string_view &bin;
        unsigned int ip = 0; //instruction pointer
        unsigned int end = 0; //last instruction pointer

        unsigned int depth = 0;
        bool active = true;
        unsigned int typeArguments = 0;
        sharedOpt<Type> resultType = nullptr;
        sharedOpt<Subroutine> previous = nullptr;
        Subroutine(const string_view &bin): bin(bin) {}
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

    class VM {
    public:
        vector<string> errors;
        /**
         * Linked list of subroutines to execute. For each external call this subroutine will be changed.
         */
        sharedOpt<Subroutine> subroutine;
        Frame *frame = nullptr;

        vector<shared<Type>> stack;

        unordered_map<unsigned int, function<shared<Type>(VM &)>> optimised;

        VM() {
            if (frame) delete frame;
//            stack.reserve(1000);
        }

        void printErrors() {
            debug("errors ({}):", errors.size());
            for (auto &&e: errors) {
                debug("  " + e);
            }
        }

        void call(const string_view &bin, unsigned int address = 0, unsigned int arguments = 0) {
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

            auto next = make_shared<Subroutine>(bin);
            next->ip = address;
            next->end = bin.size();
//            debug("call {}({})", address, arguments);
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

            if (!loopRunning) this->process();

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

        span<shared<Type>> popFrame() {
//            auto frameSize = frame->initialSp - frame->sp;
//            while (frameSize--) stack.pop();
            span<shared<Type>> sub{stack.data() + frame->initialSp, frame->sp - frame->initialSp};
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
                    const auto op = (OP) subroutine->bin[subroutine->ip];
//                    debug("[{}] OP {} ({} -> {})", subroutine->depth, op, subroutine->ip, (unsigned int)bin[subroutine->ip]);


                    if (op == OP::TypeArgument && (subroutine->bin[subroutine->ip + 1]) == OP::TypeArgument) {
                    }

                    switch (op) {
                        case OP::Jump: {
                            subroutine->ip = readUint32(subroutine->bin, 1) - 1; //minus 1 because for's i++
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
                                const auto loopProgram = readUint32(subroutine->bin, subroutine->ip + 1);
                                subroutine->ip--; //we jump back if the loop is not done, so that this section is executed when the following call() is done
                                push(next);
                                call(subroutine->bin, loopProgram, 1);
                            }
                            break;
                        }
                        case OP::JumpCondition: {
                            auto condition = pop();
                            const auto leftProgram = readUint16(subroutine->bin, subroutine->ip + 1);
                            const auto rightProgram = readUint16(subroutine->bin, subroutine->ip + 3);
                            subroutine->ip += 4;
//                            debug("{} ? {} : {}", stringify(condition), leftProgram, rightProgram);
                            call(subroutine->bin, isConditionTruthy(condition) ? leftProgram : rightProgram);
                            break;
                        }
                        case OP::Return: {
                            auto previous = frame->previous;
                            delete frame;
                            frame = previous;
                            frame->sp++; //consume the new stack entry from the function call, making it part of the previous frame
                            subroutine->active = false;
                            subroutine->ip++; //we jump directly to another subroutine, so for()'s increment doesn't apply
                            subroutine = subroutine->previous;
                            subroutine->ip--; //for()'s increment applies the wrong subroutine, so we decrease first
//                            if (!t) {
//                                throw runtime_error("Stack empty");
//                            }
//                            debug("routine result: {}", vm::stringify(last()));
                            break;
                        }
                        case OP::TypeArgument: {
                            if (frame->size() <= subroutine->typeArguments) {
                                auto unknown = make_shared<TypeUnknown>();
                                unknown->unprovidedArgument = true;
                                push(unknown);
                            }
                            subroutine->typeArguments++;
                            break;
                        }
                        case OP::TypeArgumentDefault: {
                            auto t = frameEntryAt(subroutine->typeArguments - 1);
                            //t is always set because TypeArgument ensures that
                            if (t->kind == TypeKind::Unknown && to<TypeUnknown>(t)->unprovidedArgument) {
                                frame->sp--; //remove unknown type from stack
                                const auto address = readUint32(subroutine->bin, subroutine->ip + 1);
                                subroutine->ip += 4; //jump over address
                                call(subroutine->bin, address, 0); //the result is pushed on the stack
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
                            const auto frameOffset = readUint16(subroutine->bin, subroutine->ip + 1);
                            const auto varIndex = readUint16(subroutine->bin, subroutine->ip + 3);
                            subroutine->ip += 4;
                            if (frameOffset == 0) {
                                //todo: this does not work with std::stack. what could be an alternative?
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
                        case OP::Call: {
                            const auto address = readUint32(subroutine->bin, subroutine->ip + 1);
                            const auto arguments = readUint16(subroutine->bin, subroutine->ip + 5);
                            subroutine->ip += 6;
                            call(subroutine->bin, address, arguments);
                            break;
                        }
                        case OP::Assign: {
                            auto lvalue = pop();
                            auto rvalue = pop();
                            if (!isExtendable(lvalue, rvalue)) {
                                errors.push_back(fmt::format("{}={} not assignable!", stringify(lvalue), stringify(rvalue)));
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
                        case OP::Optional: {
                            auto l = last();
                            if (l->kind == TypeKind::TupleMember) {
                                to<TypeTupleMember>(l)->optional = true;
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
                            auto types = popFrame();
                            for (auto &&v: types) t->types.push_back(v);
                            push(t);
                            break;
                        }
                        case OP::NumberLiteral: {
                            const auto address = readUint32(subroutine->bin, subroutine->ip + 1);
                            subroutine->ip += 4;
                            push(make_shared<TypeLiteral>(readStorage(subroutine->bin, address), TypeLiteralType::Number));
                            break;
                        }
                        case OP::BigIntLiteral: {
                            const auto address = readUint32(subroutine->bin, subroutine->ip + 1);
                            subroutine->ip += 4;
                            push(make_shared<TypeLiteral>(readStorage(subroutine->bin, address), TypeLiteralType::Bigint));
                            break;
                        }
                        case OP::StringLiteral: {
                            const auto address = readUint32(subroutine->bin, subroutine->ip + 1);
                            subroutine->ip += 4;
                            auto text = readStorage(subroutine->bin, address);
                            push(make_shared<TypeLiteral>(text, TypeLiteralType::String));
                            break;
                        }
                        default: {
                            throw runtime_error(fmt::format("unhandeled op {}", op));
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
                push(types[0]);
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