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
                if (!extends((Type *) left->type, (Type *) right->type)) return false;

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
                        return extends(((TypeRef *) left->type)->next->type, ((TypeRef *) right->type)->next->type);
                    }
                }
                return false;
            }
            case TypeKind::ObjectLiteral: {
                switch (left->kind) {
                    case TypeKind::ObjectLiteral: {
                        auto valid = true;
                        forEachChild(right, [&left, &valid](auto child, auto &stop) {
                            auto leftMember = findChild(left, child->hash);
                            if (!leftMember || !extends(leftMember, child)) {
                                stop = true;
                                valid = false;
                            }
                        });
                        return valid;

                        //auto rightCurrent = (TypeRef *) right->type;
                        //auto leftStart = (TypeRef *) left->type;
                        //
                        //auto i = 0;
                        //while (rightCurrent) {
                        //    auto rightName = getPropertyOrMethodName(rightCurrent->type);
                        //    //if (rightName) {
                        //        //switch (rightCurrent->type->kind) {
                        //        //    case TypeKind::Method:
                        //        //    case TypeKind::MethodSignature:
                        //        //    case TypeKind::Property:
                        //        //    case TypeKind::PropertySignature: {
                        //        //        i++;
                        //        //        auto found = findMember(leftStart, rightName->hash);
                        //        //        if (!found) return false;
                        //        //        if (!extends(getPropertyOrMethodType(found), getPropertyOrMethodType(rightCurrent->type))) return false;
                        //        //    }
                        //        //}
                        //    //}
                        //    rightCurrent = rightCurrent->next;
                        //}
                        //if (i == 0) return false;

                        return true;
                    }
                }
                return false;
            }
            case TypeKind::Union: {
                if (left->kind == TypeKind::Union) {
                    //number | string extends number => false
                    //number | string extends number | string => true
                    //string extends number | string => true

                    //pretty inefficient I know
                    auto current = (TypeRef *) right->type;
                    while (current) {
                        auto currentLeft = (TypeRef *) left->type;
                        bool fit = false;
                        while (currentLeft) {
                            if (extends(currentLeft->type, current->type)) {
                                fit = true;
                                break;
                            };
                            currentLeft = currentLeft->next;
                        }
                        if (!fit) return false;
                        current = current->next;
                    }
                    return true;
                } else {
                    //fast path first, if hash exists
                    if (!right->children.empty()) {
                        TypeRef *entry = &right->children[left->hash % right->children.size()];
                        if (entry->type) {
                            while (entry && entry->type->hash != left->hash) {
                                //follow collision link
                                entry = entry->next;
                            }
                            if (entry) return true;
                        }
                    }

                    //slow path, full scan
                    auto valid = false;
                    forEachChild(right, [&left, &valid](Type *child, bool &stop) {
                        if (extends(left, child)) {
                            stop = true;
                            valid = true;
                        }
                    });
                    return valid;
                }
            }
            case TypeKind::Literal: {
                switch (left->kind) {
                    case TypeKind::Literal:
                        if ((left->flag & TypeFlag::StringLiteral && right->flag & TypeFlag::StringLiteral) || (left->flag & TypeFlag::NumberLiteral && right->flag & TypeFlag::NumberLiteral))
                            return left->hash == right->hash;
                        return (left->flag & TypeFlag::True && right->flag & TypeFlag::True) || (left->flag & TypeFlag::False && right->flag & TypeFlag::False);
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
            case TypeKind::Parameter: {
                switch (left->kind) {
                    case TypeKind::Parameter:
                        return extends((Type *) left->type, (Type *) right->type);
                }
                return extends(left, (Type *) right->type);
            }
        }
        return false;
    }


}