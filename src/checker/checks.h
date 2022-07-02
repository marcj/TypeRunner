#pragma once

#include "./types.h"
#include "../core.h"

namespace ts::vm {
    static auto emptyString = HashString("");

    HashString &getName(const shared<Type> &member) {
        switch (member->kind) {
            case TypeKind::MethodSignature: return to<TypeMethodSignature>(member)->name;
            case TypeKind::Method: return to<TypeMethod>(member)->name;
            case TypeKind::PropertySignature: return to<TypePropertySignature>(member)->name;
            case TypeKind::Property: return to<TypeProperty>(member)->name;
        }
        return emptyString;
    }

    sharedOpt<Type> findMember(const vector<shared<Type>> &members, HashString &name) {
        for (auto &&member: members) {
            switch (member->kind) {
                case TypeKind::MethodSignature: if (to<TypeMethodSignature>(member)->name == name) return member;
                    break;
                case TypeKind::Method: if (to<TypeMethod>(member)->name == name) return member;
                    break;
                case TypeKind::PropertySignature: if (to<TypePropertySignature>(member)->name == name) return member;
                    break;
                case TypeKind::Property: if (to<TypeProperty>(member)->name == name) return member;
                    break;
            }
        }
        debug("member {} not found", name.getString());
        return nullptr;
    }

    bool isMember(const shared<Type> &type) {
        return type->kind == TypeKind::PropertySignature || type->kind == TypeKind::Property
               || type->kind == TypeKind::MethodSignature || type->kind == TypeKind::Method;
    }

    struct StackEntry {
        const shared<Type> left;
        const shared<Type> right;
    };

    struct ExtendableStack {
        vector<StackEntry> stack{};

        bool isFailed = false;

        string path() {
            string r = "";
            for (auto &&entry: stack) {
                switch (entry.left->kind) {
                    case TypeKind::Property: {
                        r += to<TypeProperty>(entry.left)->name.getString();
                        break;
                    }
                    case TypeKind::PropertySignature: {
                        r += to<TypePropertySignature>(entry.left)->name.getString();
                        break;
                    }
                }
                r += ".";
            }
            return r;
        }

        DiagnosticMessage errorMessage() {
            if (stack.empty()) return DiagnosticMessage("Type '{}' is not assignable to type '{}'", 0);

            auto [left, right] = stack.back();
//            auto [left, right] = stack.front();
            auto message = fmt::format("Type '{}' is not assignable to type '{}'", stringify(left), stringify(right));
//            auto message = fmt::format("Type '' is not assignable to type ''");

            return DiagnosticMessage(message, left->ip);
        }

        void push(const shared<Type> &left, const shared<Type> &right) {
            stack.push_back({left, right});
        }

        void pop() {
            if (isFailed) return; //we maintain the stack for nice error messages
            if (!stack.empty()) stack.pop_back();
        }

        unsigned int getState() {
            return stack.size();
        }

        void revertTo(unsigned int state) {
//            stack.resize(state);
        }

        void forcePop() {
            if (!stack.empty()) stack.pop_back();
        }

        bool failed() {
            isFailed = true;
            return false;
        }

        bool valid() {
            pop();
            return true;
        }

        bool has(const shared<Type> &left, const shared<Type> &right) {
            for (auto &&entry: stack) {
                if (entry.left == left && entry.right == right) return true;
            }
            return false;
        }
    };

    inline bool bla = false;

    /**
     * Checks whether left extends right.
     *
     * `left extends right ? true : false`
     */
    bool isExtendable(shared<Type> &left, shared<Type> &right, ExtendableStack &stack) {
        if (right->kind == TypeKind::Parameter) {
            if (left->kind == TypeKind::Undefined && isOptional(right)) return true;
            right = to<TypeParameter>(right)->type;
        }
        profiler.compare(left, right);
//        return true;

        switch (right->kind) {
            case TypeKind::ObjectLiteral: {
                switch (left->kind) {
                    case TypeKind::ObjectLiteral: {
                        auto leftTypes = to<TypeObjectLiteral>(left)->types;
                        auto rightTypes = to<TypeObjectLiteral>(right)->types;

                        //check constructor signature
                        //check index signature
                        //check call signature

                        //check properties
                        for (auto &&member: rightTypes) {
                            if (isMember(member)) {
                                auto leftMember = findMember(leftTypes, getName(member));
                                if (!leftMember) return stack.failed();
                                if (!isExtendable(leftMember, member, stack)) return stack.failed();
                            }
                        }
                        return stack.valid();
                    }
                }
                return stack.failed();
            }
            case TypeKind::PropertySignature: {
                switch (left->kind) {
                    case TypeKind::Property: {
                        auto l = to<TypeProperty>(left);
                        auto r = to<TypePropertySignature>(right);
                        if (!r->optional && isOptional(l)) return stack.failed();
                        return isExtendable(l->type, r->type, stack) ? stack.valid() : stack.failed();
                    }
                    case TypeKind::PropertySignature: {
                        auto l = to<TypePropertySignature>(left);
                        auto r = to<TypePropertySignature>(right);
                        if (!r->optional && isOptional(l)) return stack.failed();
                        return isExtendable(l->type, r->type, stack) ? stack.valid() : stack.failed();
                    }
                    default: {
                        auto r = to<TypePropertySignature>(right);
                        if (!r->optional && isOptional(left)) return stack.failed();
                        return isExtendable(left, r->type, stack) ? stack.valid() : stack.failed();
                    }
                }
            }
            case TypeKind::Property: {
                switch (left->kind) {
                    case TypeKind::Property: {
                        auto l = to<TypeProperty>(left);
                        auto r = to<TypeProperty>(right);
                        if (!r->optional && isOptional(l)) return stack.failed();
                        return isExtendable(l->type, r->type, stack) ? stack.valid() : stack.failed();
                    }
                    case TypeKind::PropertySignature: {
                        auto l = to<TypePropertySignature>(left);
                        auto r = to<TypeProperty>(right);
                        if (!r->optional && isOptional(l)) return stack.failed();
                        return isExtendable(l->type, r->type, stack) ? stack.valid() : stack.failed();
                    }
                    default: {
                        auto r = to<TypeProperty>(right);
                        if (!r->optional && isOptional(left)) return stack.failed();
                        return isExtendable(left, r->type, stack) ? stack.valid() : stack.failed();
                    }
                }
            }
            case TypeKind::String: {
                if (left->kind == TypeKind::String) return stack.valid();
                if (left->kind == TypeKind::Literal) return to<TypeLiteral>(left)->type == TypeLiteralType::String ? stack.valid() : stack.failed();
                return stack.failed();
            }
            case TypeKind::Number: {
                if (left->kind == TypeKind::Number) return stack.valid();
                if (left->kind == TypeKind::Literal) return to<TypeLiteral>(left)->type == TypeLiteralType::Number ? stack.valid() : stack.failed();
                return stack.failed();
            }
            case TypeKind::Boolean: {
                if (left->kind == TypeKind::Boolean) return stack.valid();
                if (left->kind == TypeKind::Literal) return to<TypeLiteral>(left)->type == TypeLiteralType::Boolean ? stack.valid() : stack.failed();
                return stack.failed();
            }
            case TypeKind::Literal: {
                if (left->kind == TypeKind::Literal) {
//                    //todo: this is weirdly slow?
//                    auto a = to<TypeLiteral>(left)->literal.hash;
//                    auto b = to<TypeLiteral>(right)->literal.hash;
////                    bla = a == b; // ? stack.valid() : stack.failed();
////                    if (!bla) {
////                        debug("Not equal {}={} {}={}", to<TypeLiteral>(left)->literal.getString(), to<TypeLiteral>(right)->literal.getString(), a, b);
////                    }
//                    return a == b ? stack.valid() : stack.failed();
                    return to<TypeLiteral>(left)->equal(to<TypeLiteral>(right)) ? stack.valid() : stack.failed();
                }
                return stack.failed();
            }
            case TypeKind::Array: {
                if (left->kind == TypeKind::Array) {
                    return isExtendable(to<TypeArray>(left)->type, to<TypeArray>(right)->type, stack) ? stack.valid() : stack.failed();
                } else if (left->kind == TypeKind::Tuple) {
                    auto array = to<TypeArray>(right);
                    auto tuple = to<TypeTuple>(left);

                    if (tuple->types.empty()) return stack.valid();
                    if (array->type->kind == TypeKind::Any) return stack.valid();
                    auto optional = isOptional(array->type);

                    for (auto &&tupleMember: tuple->types) {
                        if (tupleMember->optional && optional) {
                            //valid
                        } else {
                            if (!isExtendable(tupleMember->type, array->type, stack)) return stack.failed();
                        }
                    }

                    return stack.valid();
                }
                return stack.failed();
            }
            case TypeKind::Union: {
                if (left->kind != TypeKind::Union) {
                    auto rightUnion = to<TypeUnion>(right);
//                    if (left->kind == TypeKind::Literal && rightUnion->fastLiteralCheck()) {
//                        return rightUnion->includes(left) ? stack.valid() : stack.failed();
//                    }

                    auto valid = false;
                    for (auto &&l: rightUnion->types) {
                        if (isExtendable(left, l, stack)) {
                            valid = true;
                            break;
                        }
                    }
                    return valid ? stack.valid() : stack.failed();
                } else {
                    //e.g.: string|number = string|boolean
                    auto rightTypes = to<TypeUnion>(right)->types;
                    auto leftTypes = to<TypeUnion>(left)->types;

                    for (auto &&r: leftTypes) {
                        bool valid = false;
                        auto state = stack.getState();
                        for (auto &&l: rightTypes) {
                            if (isExtendable(l, r, stack)) {
                                valid = true;
                                break;
                            } else {
                                stack.revertTo(state); //we're not interested in failures inside the union, right?
                            }
                        }
                        if (!valid) return stack.failed();
                    }
                    return stack.valid();
                }
            }
        }

        return false;
    }

    bool isExtendable2(shared<Type> &left, shared<Type> &right, ExtendableStack &stack) {
//        if (stack.has(left, right)) return true;
//        stack.push(left, right);
        if (stack.stack.empty()) stack.push(left, right);

        if (right->kind == TypeKind::Parameter) {
            if (left->kind == TypeKind::Undefined && isOptional(right)) return true;
            right = to<TypeParameter>(right)->type;
        }

        switch (right->kind) {
            case TypeKind::ObjectLiteral: {
                switch (left->kind) {
                    case TypeKind::ObjectLiteral: {
                        auto leftTypes = to<TypeObjectLiteral>(left)->types;
                        auto rightTypes = to<TypeObjectLiteral>(right)->types;

                        //check constructor signature
                        //check index signature
                        //check call signature

                        //check properties
                        for (auto &&member: rightTypes) {
                            if (isMember(member)) {
                                auto leftMember = findMember(leftTypes, getName(member));
                                if (!leftMember) return stack.failed();
                                if (!isExtendable(leftMember, member, stack)) return stack.failed();
                            }
                        }
                        return stack.valid();
                    }
                }
                return stack.failed();
            }
            case TypeKind::PropertySignature: {
                switch (left->kind) {
                    case TypeKind::Property: {
                        auto l = to<TypeProperty>(left);
                        auto r = to<TypePropertySignature>(right);
                        if (!r->optional && isOptional(l)) return stack.failed();
                        return isExtendable(l->type, r->type, stack) ? stack.valid() : stack.failed();
                    }
                    case TypeKind::PropertySignature: {
                        auto l = to<TypePropertySignature>(left);
                        auto r = to<TypePropertySignature>(right);
                        if (!r->optional && isOptional(l)) return stack.failed();
                        return isExtendable(l->type, r->type, stack) ? stack.valid() : stack.failed();
                    }
                    default: {
                        auto r = to<TypePropertySignature>(right);
                        if (!r->optional && isOptional(left)) return stack.failed();
                        return isExtendable(left, r->type, stack) ? stack.valid() : stack.failed();
                    }
                }
            }
            case TypeKind::Property: {
                switch (left->kind) {
                    case TypeKind::Property: {
                        auto l = to<TypeProperty>(left);
                        auto r = to<TypeProperty>(right);
                        if (!r->optional && isOptional(l)) return stack.failed();
                        return isExtendable(l->type, r->type, stack) ? stack.valid() : stack.failed();
                    }
                    case TypeKind::PropertySignature: {
                        auto l = to<TypePropertySignature>(left);
                        auto r = to<TypeProperty>(right);
                        if (!r->optional && isOptional(l)) return stack.failed();
                        return isExtendable(l->type, r->type, stack) ? stack.valid() : stack.failed();
                    }
                    default: {
                        auto r = to<TypeProperty>(right);
                        if (!r->optional && isOptional(left)) return stack.failed();
                        return isExtendable(left, r->type, stack) ? stack.valid() : stack.failed();
                    }
                }
            }
            case TypeKind::String: {
                if (left->kind == TypeKind::String) return stack.valid();
                if (left->kind == TypeKind::Literal) return to<TypeLiteral>(left)->type == TypeLiteralType::String ? stack.valid() : stack.failed();
                return stack.failed();
            }
            case TypeKind::Number: {
                if (left->kind == TypeKind::Number) return stack.valid();
                if (left->kind == TypeKind::Literal) return to<TypeLiteral>(left)->type == TypeLiteralType::Number ? stack.valid() : stack.failed();
                return stack.failed();
            }
            case TypeKind::Boolean: {
                if (left->kind == TypeKind::Boolean) return stack.valid();
                if (left->kind == TypeKind::Literal) return to<TypeLiteral>(left)->type == TypeLiteralType::Boolean ? stack.valid() : stack.failed();
                return stack.failed();
            }
            case TypeKind::Literal: {
                if (left->kind == TypeKind::Literal) {
//                    return stack.valid();
//                    debug("compare {}=={} ({}=={}) is {}",
//                          to<TypeLiteral>(left)->literal.getString(),
//                          to<TypeLiteral>(right)->literal.getString(),
//                          to<TypeLiteral>(left)->literal.getHash(),
//                          to<TypeLiteral>(right)->literal.getHash(),
//                          to<TypeLiteral>(left)->equal(to<TypeLiteral>(right))
//                      );
//                    return stack.valid();
//                    return to<TypeLiteral>(left)->literal.hash == to<TypeLiteral>(right)->literal.hash  ? stack.valid() : stack.failed();
                    return to<TypeLiteral>(left)->equal(to<TypeLiteral>(right)) ? stack.valid() : stack.failed();
                }
                return stack.failed();
            }
            case TypeKind::Array: {
                if (left->kind == TypeKind::Array) {
                    return isExtendable(to<TypeArray>(left)->type, to<TypeArray>(right)->type, stack) ? stack.valid() : stack.failed();
                } else if (left->kind == TypeKind::Tuple) {
                    auto array = to<TypeArray>(right);
                    auto tuple = to<TypeTuple>(left);

                    if (tuple->types.empty()) return stack.valid();
                    if (array->type->kind == TypeKind::Any) return stack.valid();
                    auto optional = isOptional(array->type);

                    for (auto &&tupleMember: tuple->types) {
                        if (tupleMember->optional && optional) {
                            //valid
                        } else {
                            if (!isExtendable(tupleMember->type, array->type, stack)) return stack.failed();
                        }
                    }

                    return stack.valid();
                }
                return stack.failed();
            }
            case TypeKind::Union: {
                if (left->kind != TypeKind::Union) {
                    auto valid = false;
                    auto state = stack.getState();
                    for (auto &&l: to<TypeUnion>(right)->types) {
                        if (isExtendable(left, l, stack)) {
                            valid = true;
                            break;
                        } else {
                            stack.revertTo(state); //we're not interested in failures inside the union, right?
                        }
                    }
                    return valid ? stack.valid() : stack.failed();
                } else {
                    //e.g.: string|number = string|boolean
                    auto rightTypes = to<TypeUnion>(right)->types;
                    auto leftTypes = to<TypeUnion>(left)->types;

                    for (auto &&r: leftTypes) {
                        bool valid = false;
                        auto state = stack.getState();
                        for (auto &&l: rightTypes) {
                            if (isExtendable(l, r, stack)) {
                                valid = true;
                                break;
                            } else {
                                stack.revertTo(state); //we're not interested in failures inside the union, right?
                            }
                        }
                        if (!valid) return stack.failed();
                    }
                    return stack.valid();
                }
            }
        }

//        if (left->kind == TypeKind::Literal && right->kind == TypeKind::Literal) {
//            return to<TypeLiteral>(left)->type == to<TypeLiteral>(right)->type &&
//                   to<TypeLiteral>(left)->text() == to<TypeLiteral>(right)->text();
//        }
//
//        if (right->kind == TypeKind::Tuple && left->kind == TypeKind::Tuple) {
//            auto &rtypes = to<TypeTuple>(right)->types;
//            auto &ltypes = to<TypeTuple>(left)->types;
//            for (auto i = 0; i < rtypes.size(); i++) {
////                auto rightType = indexAccess(left, { kind: TypeKind::Literal, literal: i });
////                auto leftType = indexAccess(right, { kind: TypeKind::Literal, literal: i });
////                if (rightType->kind == TypeKind::Infer || leftType->kind == TypeKind::Infer) continue;
////                auto valid = isExtendable(leftType, rightType, extendStack);
////                if (!valid) return false;
//            }
////            inferFromTuple(right, left);
//
////            return true;
//        }
//
//        if (right->kind == TypeKind::Union) {
//            if (left->kind != TypeKind::Union) {
//                for (auto &&l: to<TypeUnion>(right)->types) {
//                    if (isExtendable(left, l, stack)) return true;
//                }
//                return false;
//            } else {
//                //e.g.: string|number = string|boolean
//                auto rightTypes = to<TypeUnion>(right)->types;
//                auto leftTypes = to<TypeUnion>(left)->types;
//
//                for (auto &&r: leftTypes) {
//                    bool valid = false;
//                    for (auto &&l: rightTypes) {
//                        if (isExtendable(l, r, stack)) {
//                            valid = true;
//                            break;
//                        };
//                    }
//                    if (!valid) return false;
//                }
//                return true;
//            }
//        }
//
//        return false;
    }

    bool isExtendable(shared<Type> &left, shared<Type> &right) {
        ExtendableStack stack;
        return isExtendable(left, right, stack);
    }
}