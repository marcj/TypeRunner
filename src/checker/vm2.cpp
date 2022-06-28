#include "./vm2.h"
#include "../hash.h"
#include "./check2.h"

namespace ts::vm2 {

    void prepare(shared<Module> &module) {
        parseHeader(module);
//        if (activeSubroutine) throw std::runtime_error("Subroutine already running");

//        activeSubroutineIdx = 0;
        activeSubroutine = activeSubroutines.reset();
        frame = frames.reset();
//        frameIdx = 0;

        activeSubroutine->module = module.get();
        activeSubroutine->ip = module->mainAddress;
        activeSubroutine->depth = 0;
    }

    Type *pop() {
        return stack[--sp];
    }

    void push(Type *type) {
        stack[sp++] = type; //reinterpret_cast<Type *>(type);
    }

    std::span<Type *> popFrame() {
        auto start = frame->initialSp + frame->variables;
        std::span<Type *> sub{stack.data() + start, sp - start};
        frame = frames.pop(); //&frames[--frameIdx];
        sp = frame->initialSp;
        return sub;
    }

    void moveFrame(std::vector<Type *> to) {
        auto start = frame->initialSp + frame->variables;
//        std::span<Type *> sub{stack.data() + start, frame->sp - start};
        to.insert(to.begin(), stack.begin() + start, stack.begin() + sp - start);

        frame = frames.pop(); //&frames[--frameIdx];
        sp = frame->initialSp;
    }

    void gcFlush() {
        for (unsigned int i = 0; i<100; i++) {
//            switch (gcQueue[i]->kind) {
//                case TypeKind::PropertySignature: {
//                    pool.propertySignature.deleteElement(reinterpret_cast<TypePropertySignature *>(gcQueue[i]));
//                    break;
//                }
//                case TypeKind::Literal: {
//                    pool.literal.deleteElement(reinterpret_cast<TypeLiteral *>(gcQueue[i]));
//                    break;
//                }
//                case TypeKind::ObjectLiteral: {
//                    pool.objectLiteral.deleteElement(reinterpret_cast<TypeObjectLiteral *>(gcQueue[i]));
//                    break;
//                }
//                case TypeKind::Union: {
//                    pool.unions.deleteElement(reinterpret_cast<TypeUnion *>(gcQueue[i]));
//                    break;
//                }
//                case TypeKind::Unknown: {
//                    pool.unknown.deleteElement(reinterpret_cast<Type *>(gcQueue[i]));
//                    break;
//                }
//                case TypeKind::Never: {
//                    pool.never.deleteElement(reinterpret_cast<TypeNever *>(gcQueue[i]));
//                    break;
//                }
//                default: {
//                    debug("gc kind {}:{}", i, (int)gcQueue[i]->kind);
//                    throw std::runtime_error("Unhandled kind for gc");
//                }
//            }
        }
        gcQueueIdx = 0;
    }

    void gc(Type *type) {
//        debug("gc {}:{}", gcQueueIdx, (int)type->kind);
        if (gcQueueIdx>=maxGcSize) {
            //garbage collect now
            gcFlush();
        }
        gcQueue[gcQueueIdx++] = type;
    }

    inline void report(DiagnosticMessage message) {
        message.module = activeSubroutine->module;
        message.module->errors.push_back(message);
    }

    inline void report(const string &message, Type *node) {
        report(DiagnosticMessage(message, node->ip));
    }

    inline void report(const string &message) {
        report(DiagnosticMessage(message, activeSubroutine->ip));
    }

    inline void pushFrame() {
        auto next = frames.push(); ///&frames[frameIdx++];
        next->initialSp = sp;
        frame = next;
    }

    //Returns true if it actually jumped to another subroutine, false if it just pushed its cached type.
    inline bool call(unsigned int address, unsigned int arguments) {
        auto routine = activeSubroutine->module->getSubroutine(address);
        if (routine->narrowed) {
            push(routine->narrowed);
            return false;
        }
        if (routine->result && arguments == 0) {
            push(routine->result);
            return false;
        }
//                    printf("Call!");

        activeSubroutine->ip++;
        auto nextActiveSubroutine = activeSubroutines.push(); //&activeSubroutines[++activeSubroutineIdx];
        nextActiveSubroutine->ip = routine->address;
        nextActiveSubroutine->module = activeSubroutine->module;
        nextActiveSubroutine->depth = activeSubroutine->depth + 1;
        activeSubroutine = nextActiveSubroutine;

        auto nextFrame = frames.push(); //&frames[++frameIdx];
        nextFrame->initialSp = sp;
        if (arguments) {
            //we move x arguments from the old stack frame to the new one
            nextFrame->initialSp -= arguments;
        }
        frame = nextFrame;
        return true;
    }

    inline bool isConditionTruthy(Type *type) {
        return type->flag & TypeFlag::True;
    }

    void process() {
        start:
        auto &bin = activeSubroutine->module->bin;
        while (true) {
//            debug("[{}] OP {} {}", activeSubroutine->depth, activeSubroutine->ip, (OP)bin[activeSubroutine->ip]);
            switch ((OP) bin[activeSubroutine->ip]) {
                case OP::Halt: {
//                    activeSubroutine = activeSubroutines.reset();
//                    frame = frames.reset();
                    return;
                }
                case OP::Never: {
                    stack[sp++] = allocate(TypeKind::Never);
                    break;
                }
                case OP::Any: {
                    auto item = allocate(TypeKind::Any);
                    stack[sp++] = allocate(TypeKind::Any);
                    break;
                }
                case OP::Frame: {
                    pushFrame();
                    break;
                }
                case OP::Assign: {
                    auto rvalue = pop();
                    auto lvalue = pop();
                    if (!extends(lvalue, rvalue)) {
//                        auto error = stack.errorMessage();
//                        error.ip = ip;
                        report("not assignable");
                    }
//                    ExtendableStack stack;
//                    if (!isExtendable(lvalue, rvalue, stack)) {
//                        auto error = stack.errorMessage();
//                        error.ip = ip;
//                        report(error);
//                    }
                    gc(lvalue);
                    gc(rvalue);
                    break;
                }
                case OP::Return: {
                    //the current frame could not only have the return value, but variables and other stuff,
                    //which we don't want. So if size is bigger than 1, we move last stack entry to first
                    // | [T] [T] [R] |
                    if (frame->size()>1) {
                        stack[frame->initialSp] = stack[sp - 1];
                    }
                    sp = frame->initialSp + 1;
                    frame = frames.pop(); //&frames[--frameIdx];
                    activeSubroutine = activeSubroutines.pop(); //&activeSubroutines[--activeSubroutineIdx];
                    goto start;
                    break;
                }
                case OP::Call: {
                    const auto address = activeSubroutine->parseUint32();
                    const auto arguments = activeSubroutine->parseUint16();
                    if (call(address, arguments)) {
                        goto start;
                    }
                    break;
                }
                case OP::JumpCondition: {
                    auto condition = pop();
                    const auto leftProgram = activeSubroutine->parseUint16();
                    const auto rightProgram = activeSubroutine->parseUint16();
//                            debug("{} ? {} : {}", stringify(condition), leftProgram, rightProgram);
                    if (call(isConditionTruthy(condition) ? leftProgram : rightProgram, 0)) {
                        goto start;
                    }
                    break;
                }
                case OP::Extends: {
                    auto right = pop();
                    auto left = pop();
//                 debug("{} extends {} => {}", stringify(left), stringify(right), isAssignable(right, left));
                    const auto valid = extends(left, right);
                    auto item = allocate(TypeKind::Literal);
                    item->flag |= TypeFlag::BooleanLiteral;
                    item->flag |= valid ? TypeFlag::True : TypeFlag::False;
                    push(item);
//                    push(make_shared<TypeLiteral>(valid ? "true" : "false", TypeLiteralType::Boolean));
                    break;
                }
                case OP::Distribute: {
                    if (!frame->loop) {
                        auto type = pop();
                        if (type->kind == TypeKind::Union) {
                            pushFrame();
                            frame->loop = loops.push(); // new LoopHelper(type);
                            frame->loop->set(type->types);
                        } else {
                            push(type);
                            const auto loopProgram = vm::readUint32(bin, activeSubroutine->ip + 1);
                            //jump over parameter
                            activeSubroutine->ip += 4;
                            if (call(loopProgram, 1)) {
                                goto start;
                            }
                            break;
                        }
                    }

                    auto next = frame->loop->next();
                    if (!next) {
                        //done
                        auto types = popFrame();
                        if (types.empty()) {
                            push(allocate(TypeKind::Never));
                        } else if (types.size() == 1) {
                            push(types[0]);
                        } else {
                            auto result = allocate(TypeKind::Union);
                            for (auto &&v: types) {
                                if (v->kind != TypeKind::Never) result->types.push_back(v);
                            }
                            push(result);
                        }
                        loops.pop();
                        frame->loop = nullptr;
                        //jump over parameter
                        activeSubroutine->ip += 4;
                    } else {
                        //next
                        const auto loopProgram = vm::readUint32(bin, activeSubroutine->ip + 1);
                        push(next);
//                        debug("distribute jump {}", activeSubroutine->ip);
                        activeSubroutine->ip--; //we jump back if the loop is not done, so that this section is executed again when the following call() is done
                        if (call(loopProgram, 1)) {
                            goto start;
                        }
                        break;
//                        call(subroutine->module, loopProgram, 1);
                    }
                    break;
                }
                case OP::Loads: {
                    const auto frameOffset = activeSubroutine->parseUint16();
                    const auto varIndex = activeSubroutine->parseUint16();
                    if (frameOffset == 0) {
                        push(stack[frame->initialSp + varIndex]);
                    } else if (frameOffset == 1) {
                        push(stack[frames.at(frames.i - 2)->initialSp + varIndex]);
                    } else if (frameOffset == 2) {
                        push(stack[frames.at(frames.i - 2)->initialSp + varIndex]);
                    } else {
                        throw std::runtime_error("frame offset not implement");
                    }
//                            debug("load var {}/{}", frameOffset, varIndex);
                    break;
                    break;
                }
                case OP::TypeArgument: {
                    if (frame->size()<=activeSubroutine->typeArguments) {
                        auto unknown = allocate(TypeKind::Unknown);
                        unknown->flag |= TypeFlag::UnprovidedArgument;
                        push(unknown);
                    }
                    activeSubroutine->typeArguments++;
                    frame->variables++;
                    break;
                }
                case OP::String: {
                    stack[sp++] = allocate(TypeKind::String);
                    break;
                }
                case OP::Number: {
                    stack[sp++] = allocate(TypeKind::Number);
                    break;
                }
                case OP::Boolean: {
                    stack[sp++] = allocate(TypeKind::Boolean);
                    break;
                }
                case OP::NumberLiteral: {
                    auto item = allocate(TypeKind::Literal);
                    const auto address = activeSubroutine->parseUint32();
                    item->text = vm::readStorage(bin, address);
                    item->hash = hash::runtime_hash(item->text);
                    item->flag |= TypeFlag::NumberLiteral;
                    stack[sp++] = item;
                    break;
                }
                case OP::StringLiteral: {
                    auto item = allocate(TypeKind::Literal);
                    const auto address = activeSubroutine->parseUint32();
                    item->text = vm::readStorage(bin, address);
                    item->hash = hash::runtime_hash(item->text);
                    item->flag |= TypeFlag::StringLiteral;
                    stack[sp++] = item;
                    break;
                }
                case OP::PropertySignature: {
                    auto nameLiteral = pop();
                    auto type = pop();
                    auto valid = true;
                    string_view name;
                    switch (nameLiteral->kind) {
                        case TypeKind::Literal: {
                            name = nameLiteral->text; //do we need a copy?
                            break;
                        }
                        default: {
                            valid = false;
                            report("A computed property name in an interface must refer to an expression whose type is a literal type or a 'unique symbol' type", type);
                        }
                    }
                    gc(nameLiteral);
                    if (valid) {
                        auto item = allocate(TypeKind::PropertySignature);
                        item->type = type;
                        item->text = name;
                        push(item);
                    } else {
                        //popped types need either be consumed (owned) or deallocated.
                        gc(type);
                    }
                    break;
                }
                case OP::ObjectLiteral: {
                    auto item = allocate(TypeKind::ObjectLiteral);
                    auto types = popFrame();
                    item->types.insert(item->types.begin(), types.begin(), types.end());
                    stack[sp++] = item;
                    break;
                }
                case OP::Union: {
                    auto item = allocate(TypeKind::Union);
                    auto types = popFrame();
                    item->types.insert(item->types.begin(), types.begin(), types.end());
                    stack[sp++] = item;
                    break;
                }
                case OP::TupleMember: {
                    auto item = allocate(TypeKind::TupleMember);
                    item->type = pop();
                    stack[sp++] = item;
                    break;
                }
                case OP::Tuple: {
                    auto item = allocate(TypeKind::Tuple);
                    auto types = popFrame();
                    item->types.insert(item->types.begin(), types.begin(), types.end());
                    stack[sp++] = item;
                    break;
                }
                default: {
                    debug("OP {} not handled!", (OP) bin[activeSubroutine->ip]);
                }
            }
            activeSubroutine->ip++;
        }
    }
};