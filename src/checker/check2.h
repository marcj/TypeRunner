#pragma once

#include "./types2.h"

namespace ts::vm2 {

    namespace check {
        struct Check {
            Type *left;
            Type *right;
        };

        inline std::array<Check, 4096> checks;
        inline unsigned int depth;
    }

    inline Type *findMember(TypeRef *start, uint64_t hash) {
        auto current = start;
        while (current) {
            if (current->type->hash == hash) return current->type;
            current = current->next;
        }
        return nullptr;
    }

    /**
     * `left extends right ? true : false`
     */
    bool extends(Type *left, Type *right) {
        switch (right->kind) {
            case TypeKind::Any: {
                return true;
            }
            case TypeKind::TupleMember: {
                if (left->kind != TypeKind::TupleMember) return false;
                //todo: handle optional
                if (!extends((Type *)left->type, (Type *)right->type)) return false;

                return true;
            }
            case TypeKind::Tuple: {
                switch (left->kind) {
                    case TypeKind::Tuple: {
                        //todo: comparing tuple is much more complex than that
                        auto rightCurrent = (TypeRef *) right->type;
                        auto leftCurrent = (TypeRef *) left->type;
                        if (rightCurrent && !leftCurrent) return false;
                        if (!rightCurrent && leftCurrent) return false;

                        while (rightCurrent) {
                            if (rightCurrent && !leftCurrent) return false;
                            if (!rightCurrent && leftCurrent) return false;
                            if (!extends(leftCurrent->type, rightCurrent->type)) return false;

                            rightCurrent = rightCurrent->next;
                            leftCurrent = leftCurrent->next;
                        }
                        return true;
                    }
                    case TypeKind::Array: {
                        auto elementType = (Type *) left->type;
                        if (elementType->kind == TypeKind::Any) return true;

                        auto current = (TypeRef *) right->type;
                        while (current) {
                            if (!extends(elementType, current->type)) return false;
                            current = current->next;
                        }
                        return true;
                    }
                }

                return false;
            }
            case TypeKind::Array: {
                switch (left->kind) {
                    case TypeKind::Array: {
                        return extends((Type *) left->type, (Type *) right->type);
                    }
                    case TypeKind::Tuple: {
                        auto elementType = (Type *) right->type;
                        if (elementType->kind == TypeKind::Any) return true;

                        auto current = (TypeRef *) left->type;
                        while (current) {
                            //current->type is TupleMember
                            if (!extends((Type *) current->type->type, elementType)) return false;
                            current = current->next;
                        }
                        return true;
                    }
                }
                return false;
            }
            case TypeKind::PropertySignature: {
                switch (left->kind) {
                    case TypeKind::PropertySignature: {
                        return true;
                    }
                }
                return false;
            }
            case TypeKind::ObjectLiteral: {
                switch (left->kind) {
                    case TypeKind::ObjectLiteral: {
                        auto rightCurrent = (TypeRef *) right->type;
                        auto leftStart = (TypeRef *) left->type;

                        while (rightCurrent) {
                            switch (rightCurrent->type->kind) {
//                                case TypeKind::PropertySignature:
                                case TypeKind::PropertySignature: {
                                    auto found = findMember(leftStart, rightCurrent->type->hash);
                                    if (!found) return false;
                                    if (!extends((Type *) found->type, (Type *) rightCurrent->type->type)) return false;
                                }
                            }
                            rightCurrent = rightCurrent->next;
                        }

                        return true;
                    }
                }
                return false;
            }
            case TypeKind::Union: {
                auto current = (TypeRef *) right->type;
                while (current) {
                    if (extends(left, current->type)) return true;
                    current = current->next;
                }
                return false;
            }
            case TypeKind::Literal: {
                switch (left->kind) {
                    case TypeKind::Literal:
                        //todo: literal type
                        if ((left->flag & TypeFlag::StringLiteral && right->flag & TypeFlag::StringLiteral) || (left->flag & TypeFlag::NumberLiteral && right->flag & TypeFlag::NumberLiteral))
                            return left->hash == right->hash;
                        return (left->flag & TypeFlag::True && right->flag & TypeFlag::True) || left->flag & TypeFlag::False && right->flag & TypeFlag::False;
                }
                return false;
            }
            case TypeKind::String: {
                switch (left->kind) {
                    case TypeKind::String:
                        return true;
                    case TypeKind::Literal:
                        return left->flag & TypeFlag::StringLiteral;
                }
                return false;
            }
            case TypeKind::Number: {
                switch (left->kind) {
                    case TypeKind::Number:
                        return true;
                    case TypeKind::Literal:
                        return left->flag & TypeFlag::NumberLiteral;
                }
                break;
            }
            case TypeKind::Boolean: {
                switch (left->kind) {
                    case TypeKind::Boolean:
                        return true;
                    case TypeKind::Literal:
                        return left->flag & TypeFlag::True || left->flag & TypeFlag::False;
                }
                break;
            }
        }
        return false;
    }


}