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