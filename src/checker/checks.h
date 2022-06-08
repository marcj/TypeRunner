#pragma once

#include "./type_objects.h"
#include "../core.h"

namespace ts::vm {
    /**
     * Checks whether two types are assignable.
     *
     * `const value: left = right;`
     */
    bool isAssignable(shared<Type> left, shared<Type> right) {
        if (left->kind == TypeKind::String) {
            if (right->kind == TypeKind::String) return true;
            if (right->kind == TypeKind::Literal) return to<TypeLiteral>(right)->type == TypeLiteralType::String;
        }

        if (left->kind == TypeKind::Number) {
            if (right->kind == TypeKind::Number) return true;
            if (right->kind == TypeKind::Literal) return to<TypeLiteral>(right)->type == TypeLiteralType::Number;
        }

        if (left->kind == TypeKind::Literal && right->kind == TypeKind::Literal) {
            return to<TypeLiteral>(left)->type == to<TypeLiteral>(right)->type &&
                    to<TypeLiteral>(left)->literal == to<TypeLiteral>(right)->literal;
        }

        if (left->kind == TypeKind::Union) {
            if (right->kind != TypeKind::Union) {
                for (auto &&l: to<TypeUnion>(left)->types) {
                    if (isAssignable(l, right)) return true;
                }
                return false;
            } else {
                //e.g.: string|number = string|boolean
                const auto leftTypes = to<TypeUnion>(left)->types;
                const auto rightTypes = to<TypeUnion>(right)->types;

                for (auto &&r: rightTypes) {
                    bool valid = false;
                    for (auto &&l: leftTypes) {
                        if (isAssignable(l, r)) {
                            valid = true;
                            break;
                        };
                    }
                    if (!valid) return false;
                }
                return true;
            }
        }

        return false;
    }
}