#include "./vm2.h"
#include "../hash.h"
#include "./check2.h"
#include "./vm2_utils.h"
#include "Tracy.hpp"

namespace ts::vm2 {
    void prepare(shared<Module> &module) {
        parseHeader(module);
        subroutine->module = module.get();
        subroutine->ip = module->subroutines[0].address; //first is main
        subroutine->depth = 0;
    }

    inline Type *use(Type *type) {
//        debug("use refCount={} {} ref={}", type->refCount, stringify(type), (void *) type);
        type->refCount++;
        return type;
    }

    // TypeRef is an owning reference
    TypeRef *useAsRef(Type *type, TypeRef *next = nullptr) {
        type->refCount++;
        return poolRef.construct(type, next);
    }

    void addHashChild(Type *type, Type *child, unsigned int size) {
        auto bucket = child->hash % size;
        auto &entry = type->children[bucket];
        if (entry.type) {
            //hash collision, prepend the list
            entry.next = useAsRef(child, entry.next);
        } else {
            entry.type = use(child);
        }
    }

    void addHashChildWithoutRefCounter(Type *type, Type *child, unsigned int size) {
        auto bucket = child->hash % size;
        auto &entry = type->children[bucket];
        if (entry.type) {
            //hash collision, append the list
            entry.next = poolRef.construct(child, entry.next);
        } else {
            entry.type = child;
        }
    }

    Type *allocate(TypeKind kind, uint64_t hash) {
        return pool.construct(kind, hash);
    }

    std::span<TypeRef> allocateRefs(unsigned int size) {
        return poolRefs.construct(size);
    }

    inline void gcWithoutChildren(Type *type) {
        //just for debugging
        if (type->flag & TypeFlag::Deleted) {
            throw std::runtime_error("Type already deleted");
        }
        type->flag |= TypeFlag::Deleted;

        pool.gc(type);
    }

    //void gc(TypeRef *type) {
    //    type->type->refCount--;
    //    gc(type->type);
    //    poolRef.gc(type);
    //}

    void gcFlush() {
        pool.gcFlush();
        poolRef.gcFlush();
    }

    void gc(Type *type) {
        //debug("gc refCount={} {} ref={}", type->refCount, stringify(type), (void *) type);
        if (type->refCount>0) return;
        gcWithoutChildren(type);

        switch (type->kind) {
            case TypeKind::Function:
            case TypeKind::Tuple:
            case TypeKind::TemplateLiteral: {
                auto current = (TypeRef *) type->type;
                while (current) {
                    auto next = current->next;
                    current->type->refCount--;
                    gc(current->type);
                    current = next;
                }

                current = (TypeRef *) type->type;
                while (current) {
                    poolRef.gc(current);
                    current = current->next;
                }
                break;
            }
            case TypeKind::MethodSignature:
            case TypeKind::PropertySignature: {
                auto nameRef = (TypeRef *) type->type;
                auto propTypeRef = nameRef->next;

                poolRef.gc(nameRef);
                poolRef.gc(propTypeRef);

                nameRef->type->refCount--;
                propTypeRef->type->refCount--;

                gc(nameRef->type);
                gc(propTypeRef->type);

                break;
            }
            case TypeKind::Union:
            case TypeKind::ObjectLiteral: {
                auto current = (TypeRef *) type->type;
                while (current) {
                    auto next = current->next;
                    current->type->refCount--;
                    gc(current->type);
                    current = next;
                }

                current = (TypeRef *) type->type;
                while (current) {
                    poolRef.gc(current);
                    current = current->next;
                }

                if (!type->children.empty()) {
                    for (auto &&child: type->children) {
                        //collision list needs to be GC, too
                        auto current = child.next;
                        while (current) {
                            auto next = current->next;
                            poolRef.gc(current);
                            current = next;
                        }
                    }
                    poolRefs.gc(type->children);
                }
                break;
            }
            case TypeKind::Array:
            case TypeKind::Rest:
            case TypeKind::TupleMember: {
                ((Type *) type->type)->refCount--;
                gc((Type *) type->type);
                break;
            }
        }
    }

    void drop(std::span<TypeRef> types) {
        for (auto &&type: types) {
            drop(type.type);
        }
        poolRefs.gc(types);
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
        pool.gcFlush();
        poolRef.gcFlush();
    }

    void gcStack() {
        for (unsigned int i = 0; i<sp; i++) {
            gc(stack[i]);
        }
        sp = 0;
    }

    //Used in the vm
    enum TypeWidenFlag: unsigned int {
        Any = 1<<1,
        String = 1<<2,
        Number = 1<<3,
        Boolean = 1<<4,
        BigInt = 1<<5,
    };

    inline Type *_widen(Type *type) {
        //see https://github.com/Microsoft/TypeScript/pull/10676
        switch (type->kind) {
            case TypeKind::Literal: {
                if (type->flag & TypeFlag::StringLiteral) return allocate(TypeKind::String, hash::const_hash("string"));
                if (type->flag & TypeFlag::NumberLiteral) return allocate(TypeKind::Number, hash::const_hash("number"));
                if (type->flag & TypeFlag::BooleanLiteral) return allocate(TypeKind::Boolean, hash::const_hash("boolean"));
                if (type->flag & TypeFlag::BigIntLiteral) return allocate(TypeKind::BigInt, hash::const_hash("bigint"));
                throw std::runtime_error("Invalid literal to widen");
            }
            case TypeKind::Union: {
                auto current = (TypeRef *) type->type;
                bool found = false;
                while (!found && current) {
                    switch (current->type->kind) {
                        case TypeKind::Null:
                        case TypeKind::Undefined:
                        case TypeKind::Literal: {
                            found = true;
                            break;
                        }
                    }
                    current = current->next;
                }

                if (found) {
                    //we found something to widen
                    auto current = (TypeRef *) type->type;
                    //widen each literal in the union.
                    //remove duplicates (like "12" | "23" => string | string => string)
                    unsigned int flag = 0;
                    auto newUnion = allocate(TypeKind::Union, hash::const_hash("union"));

                    while (current) {
                        switch (current->type->kind) {
                            case TypeKind::Null:
                            case TypeKind::Undefined: {
                                if (!(flag & TypeWidenFlag::Any)) {
                                    newUnion->appendChild(useAsRef(allocate(TypeKind::Any)));
                                    flag |= TypeWidenFlag::Any;
                                }
                                break;
                            }
                            case TypeKind::Literal: {
                                if (current->type->flag & TypeFlag::StringLiteral && !(flag & TypeWidenFlag::String)) {
                                    newUnion->appendChild(useAsRef(allocate(TypeKind::String)));
                                    flag |= TypeWidenFlag::String;
                                }
                                if (current->type->flag & TypeFlag::NumberLiteral && !(flag & TypeWidenFlag::Number)) {
                                    newUnion->appendChild(useAsRef(allocate(TypeKind::Number)));
                                    flag |= TypeWidenFlag::Number;
                                }
                                if (current->type->flag & TypeFlag::BooleanLiteral && !(flag & TypeWidenFlag::Boolean)) {
                                    newUnion->appendChild(useAsRef(allocate(TypeKind::Boolean)));
                                    flag |= TypeWidenFlag::Boolean;
                                }
                                if (current->type->flag & TypeFlag::BigIntLiteral && !(flag & TypeWidenFlag::BigInt)) {
                                    newUnion->appendChild(useAsRef(allocate(TypeKind::BigInt)));
                                    flag |= TypeWidenFlag::BigInt;
                                }
                                break;
                            }
                            default: {
                                newUnion->appendChild(useAsRef(current->type));
                                break;
                            };
                        }
                        current = current->next;
                    }
                    return newUnion;
                }
                break;
            }
        }

        return type;
    }

    inline Type *widen(Type *type) {
        auto widened = _widen(type);
        if (type == widened) {
            return type;
        } else {
            gc(type);
            return widened;
        }
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
        //sp = subroutine->initialSp;
        //frame = frames.pop();
    }

    inline std::span<Type *> popFrame() {
        throw std::runtime_error("deprecated");
        //auto start = subroutine->initialSp + subroutine->variables;
        //std::span<Type *> sub{stack.data() + start, sp - start};
        //if (subroutine->variables>0) {
        //    //we need to GC all variables
        //    for (unsigned int i = 0; i<subroutine->variables; i++) {
        //        gc(stack[subroutine->initialSp + i]);
        //    }
        //}
        //sp = subroutine->initialSp;
        //frame = frames.pop(); //&frames[--frameIdx];
        //return sub;
    }

    inline void report(DiagnosticMessage message) {
        message.module = subroutine->module;
        message.module->errors.push_back(message);
    }

    inline void report(const string &message, Type *node) {
        report(DiagnosticMessage(message, node->ip));
    }

    inline void report(const string &message) {
        report(DiagnosticMessage(message, subroutine->ip));
    }

    inline void report(const string &message, unsigned int ip) {
        report(DiagnosticMessage(message, ip));
    }

//    inline void pushFrame() {
//        auto next = frames.push(); ///&frames[frameIdx++];
//        //important to reset necessary stuff, since we reuse
//        next->initialSp = sp;
//        next->depth = subroutine->depth + 1;
////        debug("pushFrame {}", sp);
//        next->variables = 0;
//        frame = next;
//    }

    //Returns true if it actually jumped to another subroutine, false if it just pushed its cached type.
    inline bool tailCall(unsigned int address, unsigned int arguments) {
        auto routine = subroutine->module->getSubroutine(address);
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

        //stack could look like that:
        // | [T] [T] [V] [P1] [P2] [TailCall] |
        // T=TypeArgument, V=TypeVariable, but we do not need anything of that, so we GC that. P indicates argument for the call.
        for (unsigned int i = 0; i<subroutine->typeArguments; i++) {
            drop(stack[subroutine->initialSp + i]);
        }

        //stack could look like that:
        // | [T] [T] [V] [P1] [P2] [TailCall] |
        //in this case we have to move P1 and P2 at the beginning
        // | [P1] [P2]
        // T, T, and V were already dropped above.
        if (subroutine->variables) {
            for (unsigned int i = 0; i<arguments; i++) {
                stack[subroutine->initialSp + i] = stack[subroutine->initialSp + subroutine->variables + i];
            }
        }

        //we want to reuse the same frame, so reset it
        subroutine->variables = 0;
        assert(subroutine->loop == nullptr);
        sp = subroutine->initialSp + arguments;

        //jump to the new address
        subroutine->ip = routine->address;
        subroutine->module = subroutine->module;
        subroutine->subroutine = routine;
        subroutine->depth = subroutine->depth + 1;
        subroutine->typeArguments = 0;

        //debug("[{}] TailCall", subroutine->ip - 4 - 2);
        //printStack();
        return true;
    }

    inline ActiveSubroutine *pushSubroutine(ModuleSubroutine *routine, unsigned int arguments) {
        auto nextSubroutine = activeSubroutines.push(); //&activeSubroutines[++activeSubroutineIdx];
        //important to reset necessary stuff, since we reuse
        nextSubroutine->ip = routine->address;
        nextSubroutine->module = subroutine->module;
        nextSubroutine->subroutine = routine;
        nextSubroutine->depth = subroutine->depth + 1;
        nextSubroutine->typeArguments = 0;
        nextSubroutine->variables = 0;
        subroutine = nextSubroutine;

        //we move x arguments from the old stack frame to the new one
        subroutine->initialSp = sp - arguments;
        for (unsigned int i = 0; i<arguments; i++) {
            use(stack[subroutine->initialSp + i]);
        }
    }

    inline bool call(unsigned int address, unsigned int arguments) {
        auto routine = subroutine->module->getSubroutine(address);
        if (routine->narrowed) {
            push(routine->narrowed);
            return false;
        }
        if (routine->result && arguments == 0) {
            push(routine->result);
            return false;
        }

        subroutine->ip++;
        pushSubroutine(routine, arguments);
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
        switch (index->kind) {
            case TypeKind::Literal: {
                auto member = findChild(object, index->hash);
                if (!member) {
                    return allocate(TypeKind::Never);
                }
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
                return allocate(TypeKind::Never);
            }
            case TypeKind::Number:
            case TypeKind::BigInt:
            case TypeKind::Symbol:
            case TypeKind::String: {
                auto current = (TypeRef *) object->type;
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
        auto size = subroutine->parseUint16();
        auto types = subroutine->pop(size);

        //short path for `{'asd'}`
        auto first = types[0];
        if (types.size() == 1 && first->kind == TypeKind::Literal) {
            if (first->refCount == 0) {
                //reuse it
                if (first->flag & TypeFlag::StringLiteral) {
                    push(first);
                } else if (first->flag & TypeFlag::NumberLiteral) {
                    first->flag |= ~TypeFlag::NumberLiteral;
                    first->flag = TypeFlag::StringLiteral;
                    push(first);
                }
                return;
            } else if (first->flag & TypeFlag::NumberLiteral || first->flag & TypeFlag::StringLiteral) {
                //create new one
                auto res = allocate(TypeKind::Literal);
                res->fromLiteral(first);
                res->flag = TypeFlag::StringLiteral; //convert number to string literal if necessary
                gc(first);
                push(res);
                return;
            }
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

    void printStack() {
        debug("");
        debug("~~~~~~~~~~~~~~~");
        debug("Stack sp={}", sp);
        debug("~~~~~~~~~~~~~~~");
        //for (int i = activeSubroutines.i; i>=0; i--) {
        //    if (!activeSubroutines.at(i)->subroutine) {
        //        debug("Main");
        //    } else {
        //        debug("Routine {} (typeArguments={})", activeSubroutines.at(i)->subroutine->name, activeSubroutines.at(i)->typeArguments);
        //    }
        //}
        debug("-");

//        auto top = sp;
//        for (int i = frames.i; i>=0; i--) {
//            auto frame = frames.at(i);
//            debug("Frame {} depth={} variables={} initialSp={}", i, subroutine->depth, subroutine->variables, subroutine->initialSp);
//
//            if (top>0) {
//                auto size = top - subroutine->initialSp;
//                for (unsigned int j = 0; j<size; j++) {
//                    auto i = top - j - 1;
//                    debug("  {}: {} refCount={} ref={}", i, stringify(stack[i]), stack[i]->refCount, (void *) stack[i]);
//                }
//                top -= size;
//            }
//
////            if (size != 0) {
////                do {
////                    auto i = top - size;
////                    debug("{}: {} refCount={} ref={}", i, stringify(stack[i]), stack[i]->refCount, (void *) stack[i]);
////                    size--;
////                } while (size>0);
////            }
//
////            do {
////                if (top == 0) break;
////                if (top == subroutine->initialSp) break;
////                top--;
////            } while (top != 0);
//        }
        debug("~~~~~~~~~~~~~~~");
        debug("");
    }

    inline void print(Type *type, char *title = "") {
        debug("[{}] {} refCount={} {} ref={}", subroutine->ip, title, type->refCount, stringify(type), (void *) type);
    }

    Type *handleFunction(TypeKind kind) {
        const auto size = subroutine->parseUint16();

        auto name = pop();
        auto type = allocate(kind, name->hash);
        //first is the name
        type->type = useAsRef(name);

        auto types = subroutine->pop(size);
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

    inline auto start = std::chrono::high_resolution_clock::now();
    //string_view frameName;
    void process() {
        ZoneScoped;
        start:
        auto &bin = subroutine->module->bin;
        while (true) {
            ZoneScoped;
            //std::chrono::duration<double, std::milli> took = std::chrono::high_resolution_clock::now() - start;
            //fmt::print(" - took {:.9f}ms\n", took.count());
            //start = std::chrono::high_resolution_clock::now();
            //fmt::print("[{}:{}] OP {} {}\n", subroutine->depth, subroutine->depth, subroutine->ip, (OP) bin[subroutine->ip]);
            //auto frameName = new string_view(fmt::format("[{}] {}", subroutine->ip, (OP) bin[subroutine->ip]));
            //ZoneName(frameName->begin(), frameName->size());

            switch ((OP) bin[subroutine->ip]) {
                case OP::Halt: {
//                    subroutine = activeSubroutines.reset();
//                    frame = frames.reset();
//                    gcStack();
//                    gcFlush();
//                    printStack();
                    return;
                }
                case OP::Error: {
                    auto ip = subroutine->ip;
                    const auto code = (instructions::ErrorCode) subroutine->parseUint16();
                    switch (code) {
                        case instructions::ErrorCode::CannotFind: {
                            report(DiagnosticMessage(fmt::format("Cannot find name '{}'", subroutine->module->findIdentifier(ip)), ip));
                            break;
                        }
                        default: {
                            report(DiagnosticMessage(fmt::format("{}", code), ip));
                        }
                    }
                    break;
                }
                case OP::Pop: {
                    auto type = pop();
                    gc(type);
                    break;
                }
                case OP::Never: {
                    stack[sp++] = allocate(TypeKind::Never, hash::const_hash("never"));
                    break;
                }
                case OP::Any: {
                    stack[sp++] = allocate(TypeKind::Any, hash::const_hash("any"));
                    break;
                }
                case OP::Undefined: {
                    stack[sp++] = allocate(TypeKind::Undefined, hash::const_hash("undefined"));
                    break;
                }
                case OP::Null: {
                    stack[sp++] = allocate(TypeKind::Null, hash::const_hash("null"));
                    break;
                }
                case OP::Unknown: {
                    stack[sp++] = allocate(TypeKind::Unknown, hash::const_hash("unknown"));
                    break;
                }
                case OP::Parameter: {
                    const auto address = subroutine->parseUint32();
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
                    const auto address = subroutine->parseUint32();
                    auto type = allocate(TypeKind::FunctionRef, hash::const_hash("function"));
                    type->size = address;
                    stack[sp++] = type;
                    break;
                }
                case OP::Instantiate: {
                    const auto arguments = subroutine->parseUint16();
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
                case OP::New: {
                    const auto arguments = subroutine->parseUint16();
                    auto ref = pop(); //Class/Object with constructor signature

                    switch (ref->kind) {
                        case TypeKind::ClassInstance: {
                            report("Can not call new on a class instance.");
                            break;
                        }
                        case TypeKind::Class: {
                            //push()
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
                    const auto parameterAmount = subroutine->parseUint16();
                    auto parameters = subroutine->pop(parameterAmount);
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
                                gc(parameter);
                            }

                            //we could convert parameters to a tuple and then run isExtendable() on two tuples,
                            //necessary for REST parameters.

                            if (typeToCall->refCount == 0) {
                                //detach returnType from typeToCall so it won't be GC
                                typeToCall->type = ((TypeRef *) typeToCall->type)->next;
                            }
                            push(returnType);
                            gc(typeToCall);
                            break;
                        }
                        default: {
                            throw std::runtime_error(fmt::format("CallExpression on {} not handled", typeToCall->kind));
                        }
                    }
                    break;
                }
                case OP::Widen: {
                    stack[sp - 1] = widen(stack[sp - 1]);
                    break;
                }
                case OP::Set: {
                    const auto address = subroutine->parseUint32();
                    auto type = pop();
                    auto subroutineToSet = subroutine->module->getSubroutine(address);
                    if (subroutineToSet->narrowed) drop(subroutineToSet->narrowed);
                    subroutineToSet->narrowed = use(type);
                    push(type);
                    break;
                }
                case OP::Assign: {
                    auto rvalue = pop();
                    auto lvalue = pop();
                    //debug("assign {} = {}", stringify(rvalue), stringify(lvalue));
                    if (!extends(lvalue, rvalue)) {
//                        auto error = stack.errorMessage();
//                        error.ip = ip;
                        report(fmt::format("Type '{}' is not assignable to type '{}'", stringify(lvalue), stringify(rvalue)));
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
                    if (subroutine->isMain()) {
                        subroutine = activeSubroutines.reset();
                        return;
                    }

                    //printStack();
                    //gc all parameters
                    for (unsigned int i = 0; i<subroutine->typeArguments; i++) {
                        if (stack[subroutine->initialSp + i] != stack[sp - 1]) {
                            //only if the parameter is not at the same time the return value
                            drop(stack[subroutine->initialSp + i]);
                        } else {
                            //we decrease refCount for return value though, to remove ownership. The callee is responsible to clean it up now
                            stack[subroutine->initialSp + i]->refCount--;
                        }
                    }
                    //the current frame could not only have the return value, but variables and other stuff,
                    //which we don't want. So if size is bigger than 1, we move last stack entry to first
                    // | [T] [T] [R] |
                    if (subroutine->size()>1) {
                        stack[subroutine->initialSp] = stack[sp - 1];
                    }

                    sp = subroutine->initialSp + 1;
                    if (subroutine->typeArguments == 0 || subroutine->flags & SubroutineFlag::InferBody) {
//                        debug("keep type result {}", subroutine->subroutine->name);
                        subroutine->subroutine->result = use(stack[sp - 1]);
                        subroutine->subroutine->result->flag |= TypeFlag::Stored;
                    }
                    subroutine = activeSubroutines.pop(); //&activeSubroutines[--activeSubroutineIdx];
                    goto start;
                }
                case OP::Inline: {
                    const auto address = subroutine->parseUint32();
                    auto routine = subroutine->module->getSubroutine(address);
                    break;
                }
                case OP::TailCall: {
                    const auto address = subroutine->parseUint32();
                    const auto arguments = subroutine->parseUint16();
                    //if (subroutine->flag & ActiveSubroutineFlag::BlockTailCall) {
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
                case OP::UnwrapInferBody: {
                    auto returnType = stack[sp - 1];
                    if (returnType->size == 0) {
                        returnType->kind = TypeKind::Never;
                    } else if (returnType->size == 1) {
                        auto type = ((TypeRef *) returnType->type)->type;
                        //We do not gc(returnType) since it was loaded from TypeArgument which will be drop() later in ::Return
                        stack[sp - 1] = widen(type);
                    } else {
                        //We do not gc(returnType) since it was loaded from TypeArgument which will be drop() later in ::Return
                        stack[sp - 1] = widen(returnType);
                    }
                    break;
                }
                case OP::ReturnStatement: {
                    if (subroutine->flags & SubroutineFlag::InferBody) {
                        //first entry in the new stack frame is for getting all ReturnStatement calls in a union.
                        auto returnType = stack[subroutine->initialSp];
                        auto type = pop();
                        returnType->appendChild(useAsRef(type));
                    } else {

                    }
                    break;
                }
                case OP::CheckBody: {
                    const auto address = subroutine->parseUint32();
                    auto expectedType = stack[sp - 1];
                    break;
                }
                case OP::InferBody: {
                    const auto address = subroutine->parseUint32();
                    auto routine = subroutine->module->getSubroutine(address);
                    if (routine->result) {
                        push(routine->result);
                        break;
                    }
                    subroutine->ip++;
                    pushSubroutine(routine, 0);
                    //subroutine is now set to a new one.
                    //If this is set, OP::ReturnStatement acts different
                    subroutine->flags |= SubroutineFlag::InferBody;

                    //first entry in the new stack frame is for getting all ReturnStatement calls in a union.
                    //use() since it is in the place of TypeArgument, we are the owner. Will be dropped in ::Return.
                    push(use(allocate(TypeKind::Union)));
                    goto start;
                }
                case OP::SelfCheck: {
                    const auto address = subroutine->parseUint32();
                    //todo: this needs more definition: A type alias like `type a<T> = T`; needs to type check as well without throwing `Generic type 'a' requires 1 type argument(s).`
                    auto routine = subroutine->module->getSubroutine(address);
                    if (routine->result) break;

                    if (call(address, 0)) {
                        goto start;
                    }
                    break;
                }
                case OP::Call: {
                    const auto address = subroutine->parseUint32();
                    const auto arguments = subroutine->parseUint16();
                    if (call(address, arguments)) {
                        goto start;
                    }
                    break;
                }
                    //case OP::FrameReturnJump: {
                    //    if (subroutine->size()>subroutine->variables) {
                    //        //there is a return value on the stack, which we need to preserve
                    //        auto ret = pop();
                    //        popFrame();
                    //        push(ret);
                    //    } else {
                    //        //throw away the whole stack
                    //        popFrame();
                    //    }
                    //    const auto address = subroutine->parseInt32();
                    //    //debug("FrameEndJump to {} ({})", subroutine->ip + address - 4, address);
                    //    subroutine->ip += address - 4; //decrease by uint32 too
                    //    goto start;
                    //}
                case OP::Jump: {
                    const auto address = subroutine->parseInt32();
                    //debug("Jump to {} ({})", subroutine->ip + address - 4, address);
                    subroutine->ip += address - 4;
                    goto start;
                }
                case OP::JumpCondition: {
                    auto condition = pop();
                    const auto rightProgram = subroutine->parseUint32();
                    auto valid = isConditionTruthy(condition);
                    //debug("JumpCondition {}", valid);
                    gc(condition);
                    if (!valid) {
                        subroutine->ip += rightProgram - 4;
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
                    auto slot = subroutine->parseUint16();
                    //if there is OP::Distribute, then there was always before this OP
                    //a OP::Loads to push the type on the stack.
                    if (!subroutine->loop || subroutine->loop->ip != subroutine->ip) {
                        //no loop for this distribute created yet
                        auto type = pop();
                        if (type->kind == TypeKind::Union) {
                            subroutine->createLoop(subroutine->initialSp + slot, (TypeRef *) type->type);
                        } else {
                            subroutine->createEmptyLoop();
                            stack[subroutine->initialSp + slot] = type;
                            //jump over parameters, right to the distribute section
                            subroutine->ip += 1 + 4;
                            goto start;
                        }
                    }

                    auto next = subroutine->loop->next();
                    if (!next) {
                        //done
                        //printStack();
                        auto types = subroutine->pop(sp - subroutine->loop->startSP);
                        subroutine->popLoop();
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
                        const auto loopEnd = vm::readUint32(bin, subroutine->ip + 1);
                        subroutine->ip += loopEnd - 1 - 2;
                    } else {
                        //jump over parameters, right to the distribute section
                        subroutine->ip += 1 + 4;
                        goto start;
                    }
                    break;
                }
                case OP::Loads: {
                    const auto varIndex = subroutine->parseUint16();
                    push(stack[subroutine->initialSp + varIndex]);
                    //debug("Loads {}:{} -> {}", frameOffset, varIndex, stringify(stack[index]));
                    //if (frameOffset == 0) {
                    //    push(stack[subroutine->initialSp + varIndex]);
                    //} else {
                    //    push(stack[frames.at(frames.i - frameOffset)->initialSp + varIndex]);
                    //}
                    break;
                }
                case OP::Slots: {
                    auto size = subroutine->parseUint16();
                    subroutine->variables += size;
                    sp += size;
                    break;
                }
                case OP::TypeArgumentConstraint: {
                    auto constraint = pop();
                    if (subroutine->size() == subroutine->typeArguments) {
                        auto argument = stack[subroutine->initialSp + subroutine->typeArguments];
                        if (!extends(argument, constraint)) {
                            report(fmt::format("Type '{}' does not satisfy the constraint '{}'", stringify(argument), stringify(constraint)));
                        }
                    }
                    gc(constraint);
                    break;
                }
                case OP::TypeArgument: {
                    if (subroutine->size()<=subroutine->typeArguments) {
                        //all variables will be dropped at the end of the subroutine
                        push(use(allocate(TypeKind::Unknown, hash::const_hash("unknown"))));
                    } else {
                        //for provided argument we do not increase refCount, because it's the caller's job
                        //check constraints
                    }
                    subroutine->typeArguments++;
                    subroutine->variables++;
                    break;
                }
                case OP::TypeArgumentDefault: {
                    if (subroutine->size()<=subroutine->typeArguments) {
                        subroutine->typeArguments++;
                        subroutine->variables++;
                        //load default value
                        const auto address = subroutine->parseUint32();
                        if (call(address, 0)) { //the result is pushed on the stack
                            goto start;
                        }
                    } else {
                        //for provided argument we do not increase refCount, because it's the caller's job

                        subroutine->ip += 4; //jump over address

                        subroutine->typeArguments++;
                        subroutine->variables++;
                    }
                    break;

//                    auto t = stack[subroutine->initialSp + subroutine->typeArguments - 1];
//                    //t is always set because TypeArgument ensures that
//                    if (t->flag & TypeFlag::UnprovidedArgument) {
//                        //gc(stack[sp - 1]);
//                        sp--; //remove unknown type from stack
//                        const auto address = subroutine->parseUint32();
//                        if (call(address, 0)) { //the result is pushed on the stack
//                            goto start;
//                        }
//                    } else {
//                        subroutine->ip += 4; //jump over address
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
                    gc(container);
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
                    stack[sp++] = allocate(TypeKind::String, hash::const_hash("string"));
                    break;
                }
                case OP::Number: {
                    stack[sp++] = allocate(TypeKind::Number, hash::const_hash("number"));
                    break;
                }
                case OP::Boolean: {
                    stack[sp++] = allocate(TypeKind::Boolean, hash::const_hash("boolean"));
                    break;
                }
                case OP::NumberLiteral: {
                    auto item = allocate(TypeKind::Literal);
                    const auto address = subroutine->parseUint32();
                    item->readStorage(bin, address);
                    item->flag |= TypeFlag::NumberLiteral;
                    stack[sp++] = item;
                    break;
                }
                case OP::StringLiteral: {
                    auto item = allocate(TypeKind::Literal);
                    const auto address = subroutine->parseUint32();
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
                    type->hash = name->hash;
                    push(type);
                    break;
                }
                case OP::Class: {
                    const auto size = subroutine->parseUint16();
                    auto type = allocate(TypeKind::Class);
                    if (!size) {
                        push(type);
                        break;
                    }

                    //for class type->children acts as static hash map
                    type->children = allocateRefs(size);
                    auto types = subroutine->pop(size);
                    for (unsigned int i = 0; i<size; i++) {
                        addHashChild(type, types[i], size);
                    }
                    push(type);
                    break;
                }
                case OP::ObjectLiteral: {
                    const auto size = subroutine->parseUint16();
                    auto type = allocate(TypeKind::ObjectLiteral);
                    if (!size) {
                        push(type);
                        break;
                    }

                    type->size = size;
                    auto types = subroutine->pop(size);
                    if (size<5) {
                        type->type = useAsRef(types[0]);
                        auto current = (TypeRef *) type->type;
                        for (unsigned int i = 1; i<size; i++) {
                            current->next = useAsRef(types[i]);
                            current = current->next;
                        }
                    } else {
                        type->children = allocateRefs(size);
                        for (unsigned int i = 0; i<size; i++) {
                            addHashChild(type, types[i], size);
                        }
                    }
                    push(type);
                    break;
                }
                case OP::Union: {
                    const auto size = subroutine->parseUint16();
                    auto type = allocate(TypeKind::Union);
                    if (!size) {
                        push(type);
                        break;
                    }

                    auto types = subroutine->pop(size);
                    auto allocationSize = size;
                    for (auto &&child: types) {
                        if (child->kind == TypeKind::Union) allocationSize += child->size - 1;
                    }

                    type->size = allocationSize;
                    auto first = types[0];

                    if (first->kind == TypeKind::Union) {
                        TypeRef *current = nullptr;
                        forEachChild(first, [&type, &current](Type *child, auto) {
                            if (current) {
                                current = current->next = useAsRef(child);
                            } else {
                                type->type = current = useAsRef(child);
                            }
                        });
                        gc(first);
                    } else {
                        type->type = useAsRef(first);
                    }

                    auto current = (TypeRef *) type->type;
                    for (unsigned int i = 1; i<size; i++) {
                        if (types[i]->kind == TypeKind::Union) {
                            forEachChild(types[i], [&current](Type *child, auto) {
                                current = current->next = useAsRef(child);
                            });
                            gc(types[i]);
                        } else {
                            current = (current->next = useAsRef(types[i]));
                        }
                    }

                    if (allocationSize>5) {
                        type->children = allocateRefs(allocationSize);
                        for (unsigned int i = 0; i<size; i++) {
                            if (types[i]->kind == TypeKind::Union) {
                                forEachChild(types[i], [&allocationSize, &type](Type *child, auto) {
                                    addHashChildWithoutRefCounter(type, child, allocationSize);
                                });
                            } else {
                                addHashChildWithoutRefCounter(type, types[i], allocationSize);
                            }
                        }
                    }
                    push(type);
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
                    const auto size = subroutine->parseUint16();
                    if (size == 0) {
                        auto item = allocate(TypeKind::Tuple);
                        item->type = nullptr;
                        push(item);
                        break;
                    }

                    auto types = subroutine->pop(size);
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
                    debug("[{}] OP {} not handled!", subroutine->ip, (OP) bin[subroutine->ip]);
                }
            }
            subroutine->ip++;
        }
    }

    LoopHelper *ActiveSubroutine::createLoop(unsigned int var1, TypeRef *type) {
        auto newLoop = loops.push();
        newLoop->set(var1, type);
        newLoop->ip = ip;
        newLoop->startSP = sp;
        newLoop->previous = loop;
        return loop = newLoop;
    }

    LoopHelper *ActiveSubroutine::createEmptyLoop() {
        auto newLoop = loops.push();
        newLoop->ip = ip;
        newLoop->startSP = sp;
        newLoop->previous = loop;
        return loop = newLoop;
    }

    void ActiveSubroutine::popLoop() {
        loop = loop->previous;
        loops.pop();
    }
};