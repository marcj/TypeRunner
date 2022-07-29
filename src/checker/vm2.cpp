#include "./vm2.h"
#include "../hash.h"
#include "./check2.h"
#include "./vm2_utils.h"
#include <ranges>

namespace ts::vm2 {
    void prepare(shared<Module> &module) {
        parseHeader(module);
        activeSubroutine->module = module.get();
        activeSubroutine->ip = module->subroutines[0].address; //first is main
        activeSubroutine->depth = 0;
    }

    // TypeRef is an owning reference
    TypeRef *useAsRef(Type *type) {
        type->refCount++;
        auto t = poolRef.newElement();
        t->type = type;
        return t;
    }

    Type *allocate(TypeKind kind) {
        auto type = pool.newElement();
        type->kind = kind;
        type->refCount = 0;
//        debug("allocate {}", type->kind);
        return type;
    }

    void gc(TypeRef *typeRef) {
        if (gcQueueRefIdx>=maxGcSize) {
            //garbage collect now
            gcRefFlush();
        }
        gcQueueRef[gcQueueRefIdx++] = typeRef;
    }

    inline void gcWithoutChildren(Type *type) {
        if (gcQueueIdx>=maxGcSize) {
            //garbage collect now
            gcFlush();
        }
        if (type->flag & TypeFlag::Deleted) {
            throw std::runtime_error("Type already deleted");
        }
        type->flag |= TypeFlag::Deleted;
        gcQueue[gcQueueIdx++] = type;
    }

    inline Type *widen(Type *type) {
        switch (type->kind) {
            case TypeKind::Literal: {
                if (type->flag & TypeFlag::StringLiteral) return allocate(TypeKind::String);
                if (type->flag & TypeFlag::NumberLiteral) return allocate(TypeKind::Number);
                if (type->flag & TypeFlag::BooleanLiteral) return allocate(TypeKind::Boolean);
                if (type->flag & TypeFlag::BigIntLiteral) return allocate(TypeKind::BigInt);
            }
        }

        return type;
    }

    void gc(Type *type) {
        //debug("gc refCount={} {} ref={}", type->refCount, stringify(type), (void *) type);
        if (type->refCount>0) return;
        gcWithoutChildren(type);

        switch (type->kind) {
            case TypeKind::Union:
            case TypeKind::Tuple:
            case TypeKind::TemplateLiteral:
            case TypeKind::ObjectLiteral: {
                auto current = (TypeRef *) type->type;
                while (current) {
                    current->type->refCount--;
                    auto next = current->next;
                    gc(current);
                    gc(current->type);
                    current = next;
                }
                break;
            }
            case TypeKind::Array:
            case TypeKind::Rest:
            case TypeKind::PropertySignature:
            case TypeKind::TupleMember: {
                ((Type *) type->type)->refCount--;
                gc((Type *) type->type);
                break;
            }
        }
    }

    inline Type *use(Type *type) {
        type->refCount++;
//        debug("use refCount={} {} ref={}", type->refCount, stringify(type), (void *) type);
        return type;
    }

    void gcRefFlush() {
//        debug("gcRefFlush");
        for (unsigned int i = 0; i<gcQueueRefIdx; i++) {
            auto type = gcQueueRef[i];
            poolRef.deleteElement(type);
        }
        gcQueueRefIdx = 0;
    }

    void gcFlush() {
//        debug("gcFlush");
        for (unsigned int i = 0; i<gcQueueIdx; i++) {
            auto type = gcQueue[i];
            if (type->refCount) continue;
            pool.deleteElement(type);
        }
        gcQueueIdx = 0;
    }

    void drop(TypeRef *typeRef) {
        if (typeRef == nullptr) return;
        gc(typeRef);
        drop(typeRef->type);
        drop(typeRef->next);
    }

    void drop(Type *type) {
        if (type == nullptr) return;

        if (type->refCount == 0) {
            debug("type {} not used already!", stringify(type));
            return;
        }
        type->refCount--;
//        debug("drop refCount={} {}  ref={}", type->refCount, stringify(type), (void *) type);
        if (type->refCount == 0) {
            gc(type);
        }
    }

    void gcStackAndFlush() {
        gcStack();
        gcFlush();
    }

    void gcStack() {
        for (unsigned int i = 0; i<sp; i++) {
            gc(stack[i]);
        }
        sp = 0;
    }

    /**
     * Unuse all cached types of subroutines and make them available for GC.
     */
    void clear(shared<ts::vm2::Module> &module) {
        for (auto &&subroutine: module->subroutines) {
            if (subroutine.result) drop(subroutine.result);
            if (subroutine.narrowed) drop(subroutine.narrowed);
        }
        module->clear();
    }

    inline void push(Type *type) {
        stack[sp++] = type; //reinterpret_cast<Type *>(type);
    }

    inline Type *pop() {
        return stack[--sp];
    }

    inline void popFrameWithoutGC() {
        throw std::runtime_error("deprecated");
        //sp = frame->initialSp;
        //frame = frames.pop();
    }

    inline std::span<Type *> popFrame() {
        throw std::runtime_error("deprecated");
        //auto start = frame->initialSp + frame->variables;
        //std::span<Type *> sub{stack.data() + start, sp - start};
        //if (frame->variables>0) {
        //    //we need to GC all variables
        //    for (unsigned int i = 0; i<frame->variables; i++) {
        //        gc(stack[frame->initialSp + i]);
        //    }
        //}
        //sp = frame->initialSp;
        //frame = frames.pop(); //&frames[--frameIdx];
        //return sub;
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

    inline void report(const string &message, unsigned int ip) {
        report(DiagnosticMessage(message, ip));
    }

//    inline void pushFrame() {
//        auto next = frames.push(); ///&frames[frameIdx++];
//        //important to reset necessary stuff, since we reuse
//        next->initialSp = sp;
//        next->depth = frame->depth + 1;
////        debug("pushFrame {}", sp);
//        next->variables = 0;
//        frame = next;
//    }

    //Returns true if it actually jumped to another subroutine, false if it just pushed its cached type.
    inline bool tailCall(unsigned int address, unsigned int arguments) {
        auto routine = activeSubroutine->module->getSubroutine(address);
        if (routine->narrowed) {
            push(routine->narrowed);
            return false;
        }
        if (routine->result && arguments == 0) {
            push(routine->result);
            return false;
        }

        //first make sure all arguments get refCount++ so they won't be GC in next step
        for (unsigned int i = 0; i<arguments; i++) {
            use(stack[sp - i - 1]);
        }

        ////remove all open frames
        //while (frame->depth>0) {
        //    for (unsigned int i = 0; i<frame->variables; i++) {
        //        drop(stack[frame->initialSp + i]);
        //    }
        //    frame = frames.pop();
        //}

        //stack could look like that:
        // | [T] [T] [V] [P1] [P2] [TailCall] |
        // T=TypeArgument, V=TypeVariable, but we do not need anything of that, so we GC that. P indicates argument for the call.
        for (unsigned int i = 0; i<activeSubroutine->typeArguments; i++) {
            drop(stack[frame->initialSp + i]);
        }

        //stack could look like that:
        // | [T] [T] [V] [P1] [P2] [TailCall] |
        //in this case we have to move P1 and P2 at the beginning
        // | [P1] [P2]
        // T, T, and V were already dropped above.
        if (frame->variables) {
            for (unsigned int i = 0; i<arguments; i++) {
                stack[frame->initialSp + i] = stack[frame->initialSp + frame->variables + i];
            }
        }

        //we want to reuse the same frame, so reset it
        frame->variables = 0;
        assert(frame->loop == nullptr);
        sp = frame->initialSp + arguments;

        //jump to the new address
        activeSubroutine->ip = routine->address;
        activeSubroutine->module = activeSubroutine->module;
        activeSubroutine->subroutine = routine;
        activeSubroutine->depth = activeSubroutine->depth + 1;
        activeSubroutine->typeArguments = 0;

        //debug("[{}] TailCall", activeSubroutine->ip - 4 - 2);
        //printStack();
        return true;
    }

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

        activeSubroutine->ip++;
        auto nextActiveSubroutine = activeSubroutines.push(); //&activeSubroutines[++activeSubroutineIdx];
        //important to reset necessary stuff, since we reuse
        nextActiveSubroutine->ip = routine->address;
        nextActiveSubroutine->module = activeSubroutine->module;
        nextActiveSubroutine->subroutine = routine;
        nextActiveSubroutine->depth = activeSubroutine->depth + 1;
        nextActiveSubroutine->typeArguments = 0;
        activeSubroutine = nextActiveSubroutine;

        auto nextFrame = frames.push(); //&frames[++frameIdx];
        nextFrame->depth = 0;
        //important to reset necessary stuff, since we reuse

        // | (initialSp) [P1] [P1] (sp) |

        //debug("Call &{}[{}] {}", address, arguments, routine->name);
        //printStack();
        nextFrame->initialSp = sp - arguments; //we move x arguments from the old stack frame to the new one
        for (unsigned int i = 0; i<arguments; i++) {
            //debug("arg {} refCount={} {} ref={}", i, stack[nextFrame->initialSp + i]->refCount, stringify(stack[nextFrame->initialSp + i]), (void *) stack[nextFrame->initialSp + i]);
            use(stack[nextFrame->initialSp + i]);
        }
        nextFrame->variables = 0;
        frame = nextFrame;
        return true;
    }

    inline bool isConditionTruthy(Type *type) {
        return type->flag & TypeFlag::True;
    }

    unsigned int refLength(TypeRef *current) {
        unsigned int i = 0;
        while (current) {
            i++;
            current = current->next;
        }
        return i;
    }

    Type *resolveObjectIndexType(Type *object, Type *index) {
        auto current = (TypeRef *) object->type;
        switch (index->kind) {
            case TypeKind::Literal: {
                while (current) {
                    auto member = current->type;
                    switch (member->kind) {
                        case TypeKind::Method:
                        case TypeKind::MethodSignature:
                        case TypeKind::Property:
                        case TypeKind::PropertySignature: {
                            if (object->kind == TypeKind::Class && !(member->flag & TypeFlag::Static)) break;
                            auto first = (TypeRef *) member->type;
                            //first->type == index compares unique symbol equality
                            if (first->type == index || first->type->hash == index->hash) return member;
                            break;
                        }
                    }
                    current = current->next;
                }
                return allocate(TypeKind::Never);
            }
            case TypeKind::Number:
            case TypeKind::BigInt:
            case TypeKind::Symbol:
            case TypeKind::String: {
                while (current) {
                    auto member = current->type;
                    switch (member->kind) {
                        case TypeKind::IndexSignature: {
                            //todo
                            //if (extends(index, member->type)) return member.type;
                            break;
                        }
                    }
                    current = current->next;
                }
                return allocate(TypeKind::Never);
            }
        }
    }

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
    inline uint64_t lengthHash = hash::const_hash("length");

    inline Type *indexAccess(Type *container, Type *index) {
        switch (container->kind) {
            case TypeKind::Array: {
                throw std::runtime_error("Not implemented");
                break;
            }
            case TypeKind::Tuple: {
                if (index->hash == lengthHash) {
                    auto t = allocate(TypeKind::Literal);
                    t->setDynamicLiteral(TypeFlag::NumberLiteral, std::to_string(container->size));
                    return t;
                }
                throw std::runtime_error("Not implemented");
                break;
            }
            case TypeKind::ObjectLiteral:
            case TypeKind::ClassInstance:
            case TypeKind::Class: {
                switch (index->kind) {
                    case TypeKind::Literal: {
                        return resolveObjectIndexType(container, index);
                    }
                    case TypeKind::Union: {
                        //    const union: TypeUnion = { kind: TypeKind::union, types: [] };
                        //    for (const t of index.types) {
                        //        const result = resolveObjectIndexType(container, t);
                        //        if (result.kind == TypeKind::never) continue;
                        //
                        //        if (result.kind == TypeKind::union) {
                        //            for (const resultT of result.types) {
                        //                if (isTypeIncluded(union.types, resultT)) continue;
                        //                union.types.push(resultT);
                        //            }
                        //        } else {
                        //            if (isTypeIncluded(union.types, result)) continue;
                        //            union.types.push(result);
                        //        }
                        //    }
                        //    return unboxUnion(union);
                        throw std::runtime_error("Not implemented");
                        break;
                    }
                    default: {
                        return allocate(TypeKind::Never);
                    }
                }
            }
        }
        if (container->kind == TypeKind::Array) {
//            if ((index.kind == TypeKind::literal && 'number' == typeof index.literal) || index.kind == TypeKind::number) return container.type;
//            if (index.kind == TypeKind::literal && index.literal == 'length') return { kind: TypeKind::number };
        } else if (container->kind == TypeKind::Tuple) {
            if (index->hash == lengthHash) {
                auto t = allocate(TypeKind::Literal);
                t->setDynamicLiteral(TypeFlag::NumberLiteral, std::to_string(container->size));
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
        return allocate(TypeKind::Never); //make_shared<TypeNever>();
    }

    void handleTemplateLiteral() {
        auto size = activeSubroutine->parseUint16();
        auto types = frame->pop(size);

        //short path for `{'asd'}`
        if (types.size() == 1 && types[0]->kind == TypeKind::Literal) {
            if (types[0]->refCount == 0 && types[0]->flag & TypeFlag::StringLiteral) {
                //reuse it
                push(types[0]);
            } else {
                //create new one
                auto res = allocate(TypeKind::Literal);
                res->fromLiteral(types[0]);
                res->flag = TypeFlag::StringLiteral; //convert number to string literal if necessary
                gc(types[0]);
                push(res);
            }
            return;
        }

        CartesianProduct cartesian;
        for (auto &&type: types) {
            cartesian.add(type);
        }
        auto product = cartesian.calculate();

        auto result = allocate(TypeKind::Union);
        for (auto &&combination: product) {
            auto templateType = allocate(TypeKind::TemplateLiteral);
            bool hasPlaceholder = false;
//                let lastLiteral: { kind: TypeKind::literal, literal: string, parent?: Type } | undefined = undefined;
            Type *lastLiteral = nullptr;

            //merge a combination of types, e.g. [string, 'abc', '3'] as template literal => `${string}abc3`.
            for (auto &&item: combination) {
                if (item->kind == TypeKind::Never) {
                    //template literals that contain a never like `prefix.${never}` are completely ignored
                    goto next;
                }

                if (item->kind == TypeKind::Literal) {
                    if (lastLiteral) {
                        lastLiteral->appendLiteral(item);
                    } else {
                        lastLiteral = allocate(TypeKind::Literal);
                        lastLiteral->setLiteral(TypeFlag::StringLiteral, item->text);
                        templateType->appendChild(useAsRef(lastLiteral));
                    }
                } else {
                    hasPlaceholder = true;
                    lastLiteral = nullptr;
                    templateType->appendChild(useAsRef(item));
                }
            }

            if (hasPlaceholder) {
                // `${string}` -> string
                if (templateType->singleChild() && templateType->child()->kind == TypeKind::String) {
                    result->appendChild(useAsRef(templateType->child()));
                    gc(templateType);
                } else {
                    result->appendChild(useAsRef(templateType));
                }
            } else if (lastLiteral) {
                result->appendChild(useAsRef(lastLiteral));
            }
            next:;
        }

//        auto t = vm::unboxUnion(result);
//        if (t.kind == TypeKind::union) for (const member of t.types) member.parent = t;
//        debug("handleTemplateLiteral: {}", stringify(t));
        push(result);
    }

//    inline void mapFrameToChildren(Type *container) {
//        auto i = frame->initialSp + frame->variables;
//        auto current = (TypeRef *) (container->type = useAsRef(stack[i++]));
//        for (; i<sp; i++) {
//            current->next = useAsRef(stack[i]);
//            current = current->next;
//        }
//        current->next = nullptr;
//
////        unsigned int start = 0;
////        std::span<Type *> sub{stack.data() + start, sp - start};
////        sp = frame->initialSp;
////        frame = frames.pop(); //&frames[--frameIdx];
////
////        TypeRef * current = allocateRef();
////        for_each(++types.begin(), types.end(), [&current](auto v) {
////            current->next = allocateRef(v);
////            current = current->next;
////        });
////        current->next = nullptr;
//    }

    void printStack() {
        debug("");
        debug("~~~~~~~~~~~~~~~");
        debug("Stack sp={}", sp);
        debug("~~~~~~~~~~~~~~~");
        for (int i = activeSubroutines.i; i>=0; i--) {
            if (!activeSubroutines.at(i)->subroutine) {
                debug("Main");
            } else {
                debug("Routine {} (typeArguments={})", activeSubroutines.at(i)->subroutine->name, activeSubroutines.at(i)->typeArguments);
            }
        }
        debug("-");

        auto top = sp;
        for (int i = frames.i; i>=0; i--) {
            auto frame = frames.at(i);
            debug("Frame {} depth={} variables={} initialSp={}", i, frame->depth, frame->variables, frame->initialSp);

            if (top>0) {
                auto size = top - frame->initialSp;
                for (unsigned int j = 0; j<size; j++) {
                    auto i = top - j - 1;
                    debug("  {}: {} refCount={} ref={}", i, stringify(stack[i]), stack[i]->refCount, (void *) stack[i]);
                }
                top -= size;
            }

//            if (size != 0) {
//                do {
//                    auto i = top - size;
//                    debug("{}: {} refCount={} ref={}", i, stringify(stack[i]), stack[i]->refCount, (void *) stack[i]);
//                    size--;
//                } while (size>0);
//            }

//            do {
//                if (top == 0) break;
//                if (top == frame->initialSp) break;
//                top--;
//            } while (top != 0);
        }
        debug("~~~~~~~~~~~~~~~");
        debug("");
    }

    inline void print(Type *type, char *title = "") {
        debug("[{}] {} refCount={} {} ref={}", activeSubroutine->ip, title, type->refCount, stringify(type), (void *) type);
    }

    Type *handleFunction(TypeKind kind) {
        const auto size = activeSubroutine->parseUint16();

        auto type = allocate(kind);
        //first is the name
        type->type = useAsRef(pop());

        auto types = frame->pop(size);
        auto current = (TypeRef *) type->type;

        //second is always the return type
        current->next = useAsRef(types[0]);
        current = current->next;

        if (types.size()>1) {
            for_each(++types.begin(), types.end(), [&current](auto v) {
                current->next = useAsRef(v);
                current = current->next;
            });
            current->next = nullptr;
        }
        stack[sp++] = type;
        return type;
    }

    void process() {
        start:
        auto &bin = activeSubroutine->module->bin;
        while (true) {
            debug("[{}:{}] OP {} {}", activeSubroutine->depth, frame->depth, activeSubroutine->ip, (OP) bin[activeSubroutine->ip]);
            switch ((OP) bin[activeSubroutine->ip]) {
                case OP::Halt: {
//                    activeSubroutine = activeSubroutines.reset();
//                    frame = frames.reset();
//                    gcStack();
//                    gcFlush();
//                    printStack();
                    return;
                }
                case OP::Never: {
                    stack[sp++] = allocate(TypeKind::Never);
                    break;
                }
                case OP::Any: {
                    stack[sp++] = allocate(TypeKind::Any);
                    break;
                }
                case OP::Undefined: {
                    stack[sp++] = allocate(TypeKind::Undefined);
                    break;
                }
                case OP::Null: {
                    stack[sp++] = allocate(TypeKind::Null);
                    break;
                }
                case OP::Unknown: {
                    stack[sp++] = allocate(TypeKind::Unknown);
                    break;
                }
                case OP::Parameter: {
                    const auto address = activeSubroutine->parseUint32();
                    auto type = allocate(TypeKind::Parameter);
                    type->readStorage(bin, address);
                    type->type = pop();
                    stack[sp++] = type;
                    break;
                }
                case OP::Function: {
                    handleFunction(TypeKind::Function);
                    break;
                }
                case OP::FunctionRef: {
                    const auto address = activeSubroutine->parseUint32();
                    auto type = allocate(TypeKind::FunctionRef);
                    type->size = address;
                    stack[sp++] = type;
                    break;
                }
                case OP::Instantiate: {
                    const auto arguments = activeSubroutine->parseUint16();
                    auto ref = pop(); //FunctionRef/Class

                    switch (ref->kind) {
                        case TypeKind::FunctionRef: {
                            call(ref->size, arguments);
                            break;
                        }
                        default: {
                            throw std::runtime_error(fmt::format("Can not instantiate {}", ref->kind));
                        }
                    }
                    break;
                }
                case OP::Static: {
                    stack[sp - 1]->flag |= TypeFlag::Static;
                    break;
                }
                case OP::Optional: {
                    stack[sp - 1]->flag |= TypeFlag::Optional;
                    break;
                    break;
                }
                case OP::CallExpression: {
                    const auto parameterAmount = activeSubroutine->parseUint16();
                    auto parameters = frame->pop(parameterAmount);
                    auto typeToCall = pop();

                    switch (typeToCall->kind) {
                        case TypeKind::Method:
                        case TypeKind::Function: {
                            //it's important to handle parameters/typeArguments first before changing the stack with push() since `parameters` is a std::span.
                            auto current = (TypeRef *) typeToCall->type;

                            //first always the name, so we jump over it
                            current = (TypeRef *) current->next;
                            auto returnType = current->type;

                            for (unsigned int i = 0;; i++) {
                                current = (TypeRef *) current->next;
                                if (!current) break; //end
                                auto parameter = current->type;
                                auto optional = isOptional(parameter);
                                if (i>parameters.size() - 1) {
                                    //parameter not provided
                                    if (!optional && !parameter->type) {
                                        report(fmt::format("An argument for '{}' was not provided.", parameter->text), parameter);
                                    }
                                    break;
                                }
                                auto lvalue = parameters[i];
                                //auto rvalue = reinterpret_pointer_cast<Type>(parameter);
                                //ExtendableStack stack;
                                if (!extends(lvalue, parameter)) {
                                    //rerun again with
                                    //report(stack.errorMessage());
                                    report(fmt::format("Argument of type '{}' is not assignable to parameter '{}' of type '{}'", stringify(lvalue), parameter->text, stringify(parameter)));
                                }
                            }

                            //we could convert parameters to a tuple and then run isExtendable() on two tuples,
                            //necessary for REST parameters.

                            push(returnType);
                            break;
                        }
                        default: {
                            throw std::runtime_error(fmt::format("CallExpression on {} not handled", typeToCall->kind));
                        }
                    }
                    break;
                }
                case OP::Widen: {
                    auto type = pop();
                    auto widened = widen(type);
                    push(widened);
                    if (widened != type) gc(type);
                    break;
                }
                case OP::Set: {
                    const auto address = activeSubroutine->parseUint32();
                    auto type = pop();
                    auto subroutine = activeSubroutine->module->getSubroutine(address);
                    if (subroutine->narrowed) drop(subroutine->narrowed);
                    subroutine->narrowed = use(type);
                    break;
                }
                case OP::Assign: {
                    auto rvalue = pop();
                    auto lvalue = pop();
                    //debug("assign {} = {}", stringify(rvalue), stringify(lvalue));
                    if (!extends(lvalue, rvalue)) {
//                        auto error = stack.errorMessage();
//                        error.ip = ip;
                        report(fmt::format("{} = {} not assignable", stringify(rvalue), stringify(lvalue)));
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
                    if (activeSubroutine->isMain()) {
                        activeSubroutine = activeSubroutines.reset();
                        return;
                    }

                    //printStack();
                    //gc all parameters
                    for (unsigned int i = 0; i<activeSubroutine->typeArguments; i++) {
                        if (stack[frame->initialSp + i] != stack[sp - 1]) {
                            //only if the parameter is not at the same time the return value
                            drop(stack[frame->initialSp + i]);
                        } else {
                            //we decrease refCount for return value though, to remove ownership. The callee is responsible to clean it up now
                            stack[frame->initialSp + i]->refCount--;
                        }
                    }
                    //the current frame could not only have the return value, but variables and other stuff,
                    //which we don't want. So if size is bigger than 1, we move last stack entry to first
                    // | [T] [T] [R] |
                    if (frame->size()>1) {
                        stack[frame->initialSp] = stack[sp - 1];
                    }

                    sp = frame->initialSp + 1;
                    frame = frames.pop(); //&frames[--frameIdx];
                    if (activeSubroutine->typeArguments == 0) {
//                        debug("keep type result {}", activeSubroutine->subroutine->name);
                        activeSubroutine->subroutine->result = use(stack[sp - 1]);
                        activeSubroutine->subroutine->result->flag |= TypeFlag::Stored;
                    }
                    activeSubroutine = activeSubroutines.pop(); //&activeSubroutines[--activeSubroutineIdx];
                    goto start;
                    break;
                }
                case OP::TailCall: {
                    const auto address = activeSubroutine->parseUint32();
                    const auto arguments = activeSubroutine->parseUint16();
                    //if (activeSubroutine->flag & ActiveSubroutineFlag::BlockTailCall) {
                    //    if (call(address, arguments)) {
                    //        goto start;
                    //    }
                    //    break;
                    //} else {
                    if (tailCall(address, arguments)) {
                        goto start;
                    }
                    break;
                    //}
                }
                case OP::SelfCheck: {
                    const auto address = activeSubroutine->parseUint32();
                    //todo: this needs more definition: A type alias like `type a<T> = T`; needs to type check as well without throwing `Generic type 'a' requires 1 type argument(s).`
                    auto routine = activeSubroutine->module->getSubroutine(address);
                    if (routine->result) {
                        break;
                    }

                    if (call(address, 0)) {
                        goto start;
                    }
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
                    //case OP::FrameReturnJump: {
                    //    if (frame->size()>frame->variables) {
                    //        //there is a return value on the stack, which we need to preserve
                    //        auto ret = pop();
                    //        popFrame();
                    //        push(ret);
                    //    } else {
                    //        //throw away the whole stack
                    //        popFrame();
                    //    }
                    //    const auto address = activeSubroutine->parseInt32();
                    //    //debug("FrameEndJump to {} ({})", activeSubroutine->ip + address - 4, address);
                    //    activeSubroutine->ip += address - 4; //decrease by uint32 too
                    //    goto start;
                    //}
                case OP::Jump: {
                    const auto address = activeSubroutine->parseInt32();
                    //debug("Jump to {} ({})", activeSubroutine->ip + address - 4, address);
                    activeSubroutine->ip += address - 4;
                    goto start;
                }
                case OP::JumpCondition: {
                    auto condition = pop();
                    const auto rightProgram = activeSubroutine->parseUint32();
                    auto valid = isConditionTruthy(condition);
                    //debug("JumpCondition {}", valid);
                    gc(condition);
                    if (!valid) {
                        activeSubroutine->ip += rightProgram - 4;
                        goto start;
                    }
                    break;
                }
                case OP::Extends: {
                    auto right = pop();
                    auto left = pop();
                    //debug("{} extends {} => {}", stringify(left), stringify(right), extends(left, right));
                    const auto valid = extends(left, right);
                    auto item = allocate(TypeKind::Literal);
                    item->flag |= TypeFlag::BooleanLiteral;
                    item->flag |= valid ? TypeFlag::True : TypeFlag::False;
                    push(item);
                    gc(right);
                    gc(left);
                    break;
                }
                case OP::TemplateLiteral: {
                    handleTemplateLiteral();
                    break;
                }
                case OP::Distribute: {
                    auto slot = activeSubroutine->parseUint16();
                    //if there is OP::Distribute, then there was always before this OP
                    //a OP::Loads to push the type on the stack.
                    if (!frame->loop || frame->loop->ip != activeSubroutine->ip) {
                        //no loop for this distribute created yet
                        auto type = pop();
                        if (type->kind == TypeKind::Union) {
                            frame->createLoop(frame->initialSp + slot, (TypeRef *) type->type);
                        } else {
                            frame->createEmptyLoop();
                            stack[frame->initialSp + slot] = type;
                            //jump over parameters, right to the distribute section
                            activeSubroutine->ip += 1 + 4;
                            goto start;
                        }
                    }

                    auto next = frame->loop->next();
                    if (!next) {
                        //done
                        //printStack();
                        auto types = frame->pop(sp - frame->loop->startSP);
                        frame->popLoop();
                        if (types.empty()) {
                            push(allocate(TypeKind::Never));
                        } else if (types.size() == 1) {
                            push(types[0]);
                        } else {
                            auto result = allocate(TypeKind::Union);
                            TypeRef *current = (TypeRef *) (result->type = useAsRef(types[0]));
                            for_each(++types.begin(), types.end(), [&current](auto v) {
                                current->next = useAsRef(v);
                                current = current->next;
                            });
                            current->next = nullptr;
                            push(result);
                        }
                        const auto loopEnd = vm::readUint32(bin, activeSubroutine->ip + 1);
                        activeSubroutine->ip += loopEnd - 1 - 2;
                    } else {
                        //jump over parameters, right to the distribute section
                        activeSubroutine->ip += 1 + 4;
                        goto start;
                    }
                    break;
                }
                case OP::Loads: {
                    const auto varIndex = activeSubroutine->parseUint16();
                    push(stack[frame->initialSp + varIndex]);
                    //debug("Loads {}:{} -> {}", frameOffset, varIndex, stringify(stack[index]));
                    //if (frameOffset == 0) {
                    //    push(stack[frame->initialSp + varIndex]);
                    //} else {
                    //    push(stack[frames.at(frames.i - frameOffset)->initialSp + varIndex]);
                    //}
                    break;
                }
                case OP::Slots: {
                    auto size = activeSubroutine->parseUint16();
                    frame->variables += size;
                    sp += size;
                    break;
                }
                case OP::TypeArgumentConstraint: {
                    auto constraint = pop();
                    if (frame->size() == activeSubroutine->typeArguments) {
                        auto argument = stack[frame->initialSp + activeSubroutine->typeArguments];
                        if (!extends(argument, constraint)) {
                            report(fmt::format("Type '{}' does not satisfy the constraint '{}'", stringify(argument), stringify(constraint)));
                        }
                    }
                    gc(constraint);
                    break;
                }
                case OP::TypeArgument: {
                    if (frame->size()<=activeSubroutine->typeArguments) {
                        //all variables will be dropped at the end of the subroutine
                        push(use(allocate(TypeKind::Unknown)));
                    } else {
                        //for provided argument we do not increase refCount, because it's the caller's job
                        //check constraints
                    }
                    activeSubroutine->typeArguments++;
                    frame->variables++;
                    break;
                }
                case OP::TypeArgumentDefault: {
                    if (frame->size()<=activeSubroutine->typeArguments) {
                        activeSubroutine->typeArguments++;
                        frame->variables++;
                        //load default value
                        const auto address = activeSubroutine->parseUint32();
                        if (call(address, 0)) { //the result is pushed on the stack
                            goto start;
                        }
                    } else {
                        //for provided argument we do not increase refCount, because it's the caller's job

                        activeSubroutine->ip += 4; //jump over address

                        activeSubroutine->typeArguments++;
                        frame->variables++;
                    }
                    break;

//                    auto t = stack[frame->initialSp + activeSubroutine->typeArguments - 1];
//                    //t is always set because TypeArgument ensures that
//                    if (t->flag & TypeFlag::UnprovidedArgument) {
//                        //gc(stack[sp - 1]);
//                        sp--; //remove unknown type from stack
//                        const auto address = activeSubroutine->parseUint32();
//                        if (call(address, 0)) { //the result is pushed on the stack
//                            goto start;
//                        }
//                    } else {
//                        activeSubroutine->ip += 4; //jump over address
//                    }
//                    break;
                }
                case OP::Length: {
                    auto container = pop();
                    auto t = allocate(TypeKind::Literal);
                    switch (container->kind) {
                        case TypeKind::Tuple: {
                            t->setDynamicLiteral(TypeFlag::NumberLiteral, std::to_string(container->size));
                            break;
                        }
                        default: {
                            t->setLiteral(TypeFlag::NumberLiteral, "0");
                            break;
                        }
                    }
                    push(t);
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
                    gc(left);
                    gc(right);
                    push(t);
//                            }
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
                    item->readStorage(bin, address);
                    item->flag |= TypeFlag::NumberLiteral;
                    stack[sp++] = item;
                    break;
                }
                case OP::StringLiteral: {
                    auto item = allocate(TypeKind::Literal);
                    const auto address = activeSubroutine->parseUint32();
                    item->readStorage(bin, address);
                    item->flag |= TypeFlag::StringLiteral;
                    stack[sp++] = item;
                    break;
                }
                case OP::False: {
                    auto item = allocate(TypeKind::Literal);
                    item->flag |= TypeFlag::False;
                    stack[sp++] = item;
                    break;
                }
                case OP::True: {
                    auto item = allocate(TypeKind::Literal);
                    item->flag |= TypeFlag::True;
                    stack[sp++] = item;
                    break;
                }
                case OP::PropertyAccess: {
                    auto name = pop();
                    auto container = pop();
                    //e.g. container.name
                    switch (container->kind) {
                        case TypeKind::Class: {
                            //MyClass.name, static access
                            auto type = indexAccess(container, name);
                            push(type);
                            break;
                        }
                        default: {
                            throw std::runtime_error(fmt::format("Property access to {} not supported", container->kind));
                        }
                    }

                    gc(name);
                    gc(container);
                    break;
                }
                case OP::Method: {
                    handleFunction(TypeKind::Method);
                    break;
                }
                case OP::PropertySignature: {
                    auto name = pop();
                    auto propertyType = pop();
                    //PropertySignature has a linked list of name->type
                    auto type = allocate(TypeKind::PropertySignature);
                    type->type = useAsRef(name);
                    ((TypeRef *) type->type)->next = useAsRef(propertyType);
                    push(type);
                    break;
                }
                case OP::Class: {
                    const auto size = activeSubroutine->parseUint16();
                    auto item = allocate(TypeKind::Class);
                    if (!size) {
                        item->type = nullptr;
                        push(item);
                        break;
                    }

                    auto types = frame->pop(size);
                    item->type = useAsRef(types[0]);
                    if (types.size()>1) {
                        auto current = (TypeRef *) item->type;
                        for_each(++types.begin(), types.end(), [&current](auto v) {
                            current->next = useAsRef(v);
                            current = current->next;
                        });
                        current->next = nullptr;
                    }
                    push(item);
                    break;
                }
                case OP::ObjectLiteral: {
                    const auto size = activeSubroutine->parseUint16();
                    auto item = allocate(TypeKind::ObjectLiteral);
                    if (!size) {
                        item->type = nullptr;
                        push(item);
                        break;
                    }

                    auto types = frame->pop(size);
                    item->type = useAsRef(types[0]);
                    if (types.size()>1) {
                        auto current = (TypeRef *) item->type;
                        for_each(++types.begin(), types.end(), [&current](auto v) {
                            current->next = useAsRef(v);
                            current = current->next;
                        });
                        current->next = nullptr;
                    }
                    push(item);
                    break;
                }
                case OP::Union: {
                    const auto size = activeSubroutine->parseUint16();
                    auto item = allocate(TypeKind::Union);
                    //printStack();
                    if (!size) {
                        item->type = nullptr;
                        push(item);
                        break;
                    }
                    auto types = frame->pop(size);
                    auto first = types[0];

                    if (first->kind == TypeKind::Union) {
                        //if type has no owner, we can steal its children
                        if (first->refCount == 0) {
                            item->type = first->type;
                            first->type = nullptr;
                            //since we stole its children, we want it to GC but without its children. their refCount count belongs now to us.
                            gc(first);
                        } else {
                            throw std::runtime_error("Can not merge used union");
                        }
                    } else {
                        item->type = useAsRef(first);
                    }
                    if (size>1) {
                        auto current = (TypeRef *) item->type;
                        //set current to the end of the list
                        while (current->next) current = current->next;

                        for_each(++types.begin(), types.end(), [&current](auto v) {
                            if (v->kind == TypeKind::Union) {
                                //if type has no owner, we can steal its children
                                if (v->refCount == 0) {
                                    current->next = (TypeRef *) v->type;
                                    v->type = nullptr;
                                    //since we stole its children, we want it to GC but without its children. their refCount count belongs now to us.
                                    gcWithoutChildren(v);
                                    //set current to the end of the list
                                    while (current->next) current = current->next;
                                } else {
                                    throw std::runtime_error("Can not merge used union");
                                }
                            } else {
                                current->next = useAsRef(v);
                                current = current->next;
                            }
                        });
                        current->next = nullptr;
                    }
                    push(item);
                    break;
                }
                case OP::Array: {
                    auto item = allocate(TypeKind::Array);
                    item->type = use(pop());
                    stack[sp++] = item;
                    break;
                }
                case OP::RestReuse: {
                    auto item = allocate(TypeKind::Rest);
                    item->flag |= TypeFlag::RestReuse;
                    item->type = use(pop());
                    stack[sp++] = item;
                    break;
                }
                case OP::Rest: {
                    auto item = allocate(TypeKind::Rest);
                    item->type = use(pop());
                    stack[sp++] = item;
                    break;
                }
                case OP::TupleMember: {
                    auto item = allocate(TypeKind::TupleMember);
                    item->type = use(pop());
                    stack[sp++] = item;
                    break;
                }
                case OP::Tuple: {
                    const auto size = activeSubroutine->parseUint16();
                    if (size == 0) {
                        auto item = allocate(TypeKind::Tuple);
                        item->type = nullptr;
                        push(item);
                        break;
                    }

                    auto types = frame->pop(size);
                    Type *item;
                    auto firstTupleMember = types[0];
                    auto firstType = (Type *) firstTupleMember->type;
                    if (firstType->kind == TypeKind::Rest) {
                        //[...T, x]
                        Type *T = (Type *) firstType->type;
                        if (T->kind == TypeKind::Array) {
                            item = allocate(TypeKind::Tuple);
                            //type T = number[];
                            //type New = [...T, x]; => [...number[], x];
                            throw std::runtime_error("assigning array rest in tuple not supported");
                        } else if (T->kind == TypeKind::Tuple) {
                            //type T = [y, z];
                            //type New = [...T, x]; => [y, z, x];
                            //auto length = refLength((TypeRef *) T->type);
                            //debug("...T of size {} with refCount={} *{}", T->size, T->refCount, (void *) T);

                            //if type has no owner, we can just use it as the new type
                            //T.refCount is minimum 1, because the T is owned by Rest, and Rest owned by TupleMember, and TupleMember by nobody,
                            //if T comes from a type argument, it is 2 since argument belongs to the caller.
                            //thus an expression of [...T] yields always T.refCount >= 2.
                            if (T->refCount == 2 && firstType->flag & TypeFlag::RestReuse && !(firstType->flag & TypeFlag::Stored)) {
                                item = T;
                                //item = use(T);
                                //print(item, "reuse tuple");
                            } else {
                                item = allocate(TypeKind::Tuple);
                                TypeRef *newCurrent = nullptr;
                                auto oldCurrent = (TypeRef *) T->type;
                                while (oldCurrent) {
                                    //we reuse the tuple member and increase its refCount.
                                    auto tupleMember = useAsRef(oldCurrent->type);

                                    item->size++;
                                    if (newCurrent) {
                                        newCurrent->next = tupleMember;
                                        newCurrent = newCurrent->next;
                                    } else {
                                        newCurrent = (TypeRef *) (item->type = tupleMember);
                                    }

                                    oldCurrent = oldCurrent->next;
                                }
                                //print(item, "new tuple");
                            }
                        } else {
                            item = allocate(TypeKind::Tuple);
                            debug("Error: [...T] where T is not an array/tuple.");
                        }
//                            debug("...T after merge size {}", refLength((TypeRef *) item->type));
                        //drop Rest operator, since it was consumed now, so its resources are freed.
                        //the tuple member has refCount=0 in a [...T] operation, so it also GCs its REST type.
                        //decreases T->refCount.
                        //this tuple member ...T is not needed anymore, since it was consumed, so we GC it.
                        gc(firstTupleMember);
                    } else {
                        item = allocate(TypeKind::Tuple);
                        item->type = useAsRef(types[0]);
                        item->size = 1;
                    }

                    if (types.size()>1) {
                        //note item->type could still be null, when T is empty for [...T, 0]
                        auto current = (TypeRef *) item->type;
                        //set current to the end of the list
                        while (current && current->next) current = current->next;

                        for_each(++types.begin(), types.end(), [&current, &item](auto tupleMember) {
                            auto type = (Type *) tupleMember->type;
                            if (type->kind == TypeKind::Rest) {
                                //[x, ...T]
                                Type *T = (Type *) type->type;
                                if (T->kind == TypeKind::Array) {
                                    //type T = number[];
                                    //type New = [...T, x]; => [...number[], x];
                                    throw std::runtime_error("assigning array rest in tuple not supported");
                                } else if (T->kind == TypeKind::Tuple) {
                                    //type T = [y, z];
                                    //type New = [...T, x]; => [y, z, x];
                                    auto oldCurrent = (TypeRef *) T->type;
                                    while (oldCurrent) {
                                        //we reuse the tuple member and increase its refCount.
                                        auto tupleMember = useAsRef(oldCurrent->type);
                                        current->next = tupleMember;
                                        current = current->next;
                                        item->size++;
                                        oldCurrent = oldCurrent->next;
                                    }
                                } else {
                                    debug("Error: [...T] where T is not an array/tuple.");
                                }
                                //drop Rest operator, since it was consumed now, so its resources is freed
                                gc(tupleMember);
                            } else {
                                item->size++;
                                if (current) {
                                    current->next = useAsRef(tupleMember);
                                    current = current->next;
                                } else {
                                    current = (TypeRef *) (item->type = useAsRef(tupleMember));
                                }
                            }
                        });
                    }
                    push(item);
                    break;
                }
                default: {
                    debug("[{}] OP {} not handled!", activeSubroutine->ip, (OP) bin[activeSubroutine->ip]);
                }
            }
            activeSubroutine->ip++;
        }
    }

    LoopHelper *Frame::createLoop(unsigned int var1, TypeRef *type) {
        auto newLoop = loops.push();
        newLoop->set(var1, type);
        newLoop->ip = activeSubroutine->ip;
        newLoop->startSP = sp;
        newLoop->previous = loop;
        return loop = newLoop;
    }

    LoopHelper *Frame::createEmptyLoop() {
        auto newLoop = loops.push();
        newLoop->ip = activeSubroutine->ip;
        newLoop->startSP = sp;
        newLoop->previous = loop;
        return loop = newLoop;
    }

    void Frame::popLoop() {
        loop = loop->previous;
        loops.pop();
    }
};