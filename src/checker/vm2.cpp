#include "./vm2.h"
#include "../hash.h"
#include "./check2.h"
#include <ranges>

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

    // TypeRef is an owning reference
    inline TypeRef *useAsRef(Type *type) {
        type->users++;
        auto t = poolRef.newElement();
        t->type = type;
        return t;
    }

    inline Type *allocate(TypeKind kind) {
        auto type = pool.newElement();
        type->kind = kind;
        type->users = 0;
//        debug("allocate {}", stringify(type));
        return type;
    }

    void gc(TypeRef *typeRef) {
        if (gcQueueRefIdx>=maxGcSize) {
            //garbage collect now
            gcRefFlush();
        }
        gcQueueRef[gcQueueRefIdx++] = typeRef;
    }

    void gc(Type *type) {
        if (gcQueueIdx>=maxGcSize) {
            //garbage collect now
            gcFlush();
        }
//        debug("gc ({}) {}", type->users, stringify(type));
        switch (type->kind) {
            case TypeKind::Union:
            case TypeKind::Tuple:
            case TypeKind::TemplateLiteral:
            case TypeKind::ObjectLiteral: {
                auto current = (TypeRef *) type->type;
                while (current) {
                    current->type->users--;
                    gc(current->type);
                    current = current->next;
                }
                break;
            }
            case TypeKind::Array:
            case TypeKind::PropertySignature:
            case TypeKind::TupleMember: {
                ((Type *) type->type)->users--;
                gc((Type *) type->type);
                break;
            }
        }
        gcQueue[gcQueueIdx++] = type;
    }

    inline Type *use(Type *type) {
//        debug("use({})", stringify(type));
        type->users++;
        return type;
    }

    void gcRefFlush() {
        for (unsigned int i = 0; i<gcQueueRefIdx; i++) {
            auto type = gcQueueRef[i];
            poolRef.deleteElement(type);
        }
        gcQueueRefIdx = 0;
    }

    void gcFlush() {
        for (unsigned int i = 0; i<gcQueueIdx; i++) {
            auto type = gcQueue[i];
            if (type->users) continue;
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

        type->users--;
//        debug("drop ({}) {}", type->users, stringify(type));
        if (!type->users) {
            gc(type);
        }
    }

    void gcStack() {
        for (unsigned int i = 0; i<sp; i++) {
            gc(stack[i]);
        }
    }

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
        auto type = stack[--sp];
        type->users++;
        return type;
    }

    inline std::span<Type *> popFrame() {
        auto start = frame->initialSp + frame->variables;
        std::span<Type *> sub{stack.data() + start, sp - start};
        sp = frame->initialSp;
        frame = frames.pop(); //&frames[--frameIdx];
        return sub;
    }

    inline void moveFrame(std::vector<Type *> to) {
        auto start = frame->initialSp + frame->variables;
//        std::span<Type *> sub{stack.data() + start, frame->sp - start};
        to.insert(to.begin(), stack.begin() + start, stack.begin() + sp - start);

        frame = frames.pop(); //&frames[--frameIdx];
        sp = frame->initialSp;
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
        //important to reset necessary stuff, since we reuse
        next->initialSp = sp;
//        debug("pushFrame {}", sp);
        next->variables = 0;
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
        //important to reset necessary stuff, since we reuse
        nextActiveSubroutine->ip = routine->address;
        nextActiveSubroutine->module = activeSubroutine->module;
        nextActiveSubroutine->subroutine = routine;
        nextActiveSubroutine->depth = activeSubroutine->depth + 1;
        nextActiveSubroutine->typeArguments = 0;
        activeSubroutine = nextActiveSubroutine;

        auto nextFrame = frames.push(); //&frames[++frameIdx];
        //important to reset necessary stuff, since we reuse
        nextFrame->initialSp = sp;
        nextFrame->variables = 0;
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
    inline Type *indexAccess(Type *container, Type *index) {
        if (container->kind == TypeKind::Array) {
//            if ((index.kind == TypeKind::literal && 'number' == typeof index.literal) || index.kind == TypeKind::number) return container.type;
//            if (index.kind == TypeKind::literal && index.literal == 'length') return { kind: TypeKind::number };
        } else if (container->kind == TypeKind::Tuple) {
            if (index->hash == hash::const_hash("length")) {
                auto t = allocate(TypeKind::Literal);
//                t->setLiteral(TypeFlag::NumberLiteral, std::to_string(container->types.size()));
//                auto t = make_shared<TypeLiteral>("", TypeLiteralType::Number);
//                t->append(to<TypeTuple>(container)->types.size());
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
//        auto types = popFrame();
//
//        //short path for `{'asd'}`
//        if (types.size() == 1 && types[0]->kind == TypeKind::Literal) {
//            //we can not just change the TypeLiteral->type, we have to create a new one
////                to<TypeLiteral>(types[0])->type = TypeLiteralType::String;
//            auto t = types[0];
//            auto res = make_shared<TypeLiteral>(t->literal, TypeLiteralType::String);
//            if (t->dynamicString) {
//                res->dynamicString = new string(*t->dynamicString);
//            }
//            push(res);
//            return;
//        }
//
//        auto result = make_shared<TypeUnion>();
//        CartesianProduct cartesian;
//        for (auto &&type: types) {
//            cartesian.add(type);
//        }
//        auto product = cartesian.calculate();
//
//        outer:
//        for (auto &&combination: product) {
//            auto templateType = make_shared<TypeTemplateLiteral>();
//            bool hasPlaceholder = false;
////                let lastLiteral: { kind: TypeKind::literal, literal: string, parent?: Type } | undefined = undefined;
//            sharedOpt<TypeLiteral> lastLiteral;
//
//            //merge a combination of types, e.g. [string, 'abc', '3'] as template literal => `${string}abc3`.
//            for (auto &&item: combination) {
//                if (item->kind == TypeKind::Never) {
//                    //template literals that contain a never like `prefix.${never}` are completely ignored
//                    goto outer;
//                }
//
//                if (item->kind == TypeKind::Literal) {
//                    if (lastLiteral) {
//                        lastLiteral->append(to<TypeLiteral>(item)->text());
//                    } else {
//                        lastLiteral = make_shared<TypeLiteral>("", TypeLiteralType::String);
//                        lastLiteral->append(to<TypeLiteral>(item)->text());
//                        templateType->types.push_back(lastLiteral);
//                    }
//                } else {
//                    hasPlaceholder = true;
//                    lastLiteral = nullptr;
////                        item.parent = template;
//                    templateType->types.push_back(item);
//                }
//            }
//
//            if (hasPlaceholder) {
//                if (templateType->types.size() == 1 && templateType->types[0]->kind == TypeKind::String) {
////                        templateType->types[0].parent = result;
//                    result->types.push_back(templateType->types[0]);
//                } else {
////                        templateType->parent = result;
//                    result->types.push_back(templateType);
//                }
//            } else if (lastLiteral) {
////                    lastLiteral.parent = result;
//                result->types.push_back(lastLiteral);
//            }
//        }
//
//        auto t = vm::unboxUnion(result);
////        if (t.kind == TypeKind::union) for (const member of t.types) member.parent = t;
////        debug("handleTemplateLiteral: {}", stringify(t));
//        push(t);
    }

    inline void mapFrameToChildren(Type *container) {
        auto i = frame->initialSp + frame->variables;
        auto current = (TypeRef *) (container->type = useAsRef(stack[i++]));
        for (; i<sp; i++) {
            current->next = useAsRef(stack[i]);
            current = current->next;
        }
        current->next = nullptr;

//        unsigned int start = 0;
//        std::span<Type *> sub{stack.data() + start, sp - start};
//        sp = frame->initialSp;
//        frame = frames.pop(); //&frames[--frameIdx];
//
//        TypeRef * current = allocateRef();
//        for_each(++types.begin(), types.end(), [&current](auto v) {
//            current->next = allocateRef(v);
//            current = current->next;
//        });
//        current->next = nullptr;
    }

    void process() {
        start:
        auto &bin = activeSubroutine->module->bin;
        while (true) {
//            debug("[{}] OP {} {}", activeSubroutine->depth, activeSubroutine->ip, (OP) bin[activeSubroutine->ip]);
            switch ((OP) bin[activeSubroutine->ip]) {
                case OP::Halt: {
//                    activeSubroutine = activeSubroutines.reset();
//                    frame = frames.reset();
//                    gcFlush();
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
//                    debug("assign {} = {}", stringify(rvalue), stringify(lvalue));
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
                    drop(lvalue);
                    drop(rvalue);
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
                    if (activeSubroutine->typeArguments == 0) {
//                        debug("keep type result {}", activeSubroutine->subroutine->name);
                        activeSubroutine->subroutine->result = use(stack[sp - 1]);
                    }
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
                    auto valid = isConditionTruthy(condition);
                    drop(condition);
                    if (call(valid ? leftProgram : rightProgram, 0)) {
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
                    drop(right);
                    drop(left);
                    break;
                }
                case OP::TemplateLiteral: {
                    handleTemplateLiteral();
                    break;
                }
                case OP::Distribute: {
                    if (!frame->loop) {
                        auto type = pop();
                        if (type->kind == TypeKind::Union) {
                            pushFrame();
                            frame->loop = loops.push(); // new LoopHelper(type);
                            frame->loop->set((TypeRef *) type->type);
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
                            TypeRef *current = useAsRef(types[0]);
                            for_each(++types.begin(), types.end(), [&current](auto v) {
                                current->next = useAsRef(v);
                                current = current->next;
                            });
                            current->next = nullptr;
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
                case OP::TypeArgumentDefault: {
                    auto t = stack[frame->initialSp + activeSubroutine->typeArguments - 1];
                    //t is always set because TypeArgument ensures that
                    if (t->flag & TypeFlag::UnprovidedArgument) {
                        gc(stack[sp]);
                        sp--; //remove unknown type from stack
                        const auto address = activeSubroutine->parseUint32();
                        if (call(address, 0)) { //the result is pushed on the stack
                            goto start;
                        }
                    } else {
                        activeSubroutine->ip += 4; //jump over address
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
                case OP::PropertySignature: {
                    auto nameLiteral = pop();
                    auto type = pop();
                    switch (nameLiteral->kind) {
                        case TypeKind::Literal: {
                            auto item = allocate(TypeKind::PropertySignature);
                            item->type = type;
                            item->text = nameLiteral->text;
                            drop(nameLiteral);
                            push(item);
                            break;
                        }
                        default: {
                            //popped types need either be consumed (owned) or deallocated.
                            drop(type);
                            drop(nameLiteral);
                            report("A computed property name in an interface must refer to an expression whose type is a literal type or a 'unique symbol' type", type);
                        }
                    }
                    break;
                }
                case OP::ObjectLiteral: {
                    auto item = allocate(TypeKind::ObjectLiteral);
                    auto types = popFrame();
                    if (types.empty()) {
                        item->type = nullptr;
                        push(item);
                        break;
                    }

                    item->type = useAsRef(types[0]);
                    auto current = (TypeRef *) item->type;
                    for_each(++types.begin(), types.end(), [&current](auto v) {
                        current->next = useAsRef(v);
                        current = current->next;
                    });
                    current->next = nullptr;
                    stack[sp++] = item;
                    break;
                }
                case OP::Union: {
                    auto item = allocate(TypeKind::Union);
                    auto types = popFrame();
                    if (types.empty()) {
                        item->type = nullptr;
                        break;
                    }

                    auto type = types[0];
                    if (type->kind == TypeKind::Union) {
                        //if type has no owner, we can steal its children
                        if (type->users == 0) {
                            item->type = type->type;
                            item->type = nullptr;
                            //set current to the end of the list
                        } else {
                            throw std::runtime_error("Can not merge used union");
                        }
                    } else {
                        item->type = useAsRef(type);
                    }
                    auto current = (TypeRef *) item->type;

                    //set current to the end of the list
                    while (current->next) current = current->next;

                    for_each(++types.begin(), types.end(), [&current](auto v) {
                        if (v->kind == TypeKind::Union) {
                            //if type has no owner, we can steal its children
                            if (v->users == 0) {
                                current->next = (TypeRef *) v->type;
                                v->type = nullptr;
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
                    if (types.empty()) {
                        item->type = nullptr;
                        push(item);
                        break;
                    }

                    item->type = useAsRef(types[0]);
                    auto current = (TypeRef *) item->type;
                    for_each(++types.begin(), types.end(), [&current](auto v) {
                        current->next = useAsRef(v);
                        current = current->next;
                    });
                    current->next = nullptr;
                    push(item);
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