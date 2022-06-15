#pragma once

#include "./type_objects.h"
#include "../core.h"

namespace ts::vm {
    /**
     * Checks whether left extends right.
     *
     * `left extends right ? true : false`
     */
    bool isExtendable(shared<Type> &left, shared<Type> &right) {
        if (right->kind == TypeKind::String) {
            if (left->kind == TypeKind::String) return true;
            if (left->kind == TypeKind::Literal) return to<TypeLiteral>(left)->type == TypeLiteralType::String;
        }

        if (right->kind == TypeKind::Number) {
            if (left->kind == TypeKind::Number) return true;
            if (left->kind == TypeKind::Literal) return to<TypeLiteral>(left)->type == TypeLiteralType::Number;
        }

        if (left->kind == TypeKind::Literal && right->kind == TypeKind::Literal) {
            return to<TypeLiteral>(left)->type == to<TypeLiteral>(right)->type &&
                    to<TypeLiteral>(left)->text() == to<TypeLiteral>(right)->text();
        }

        if (right->kind == TypeKind::Tuple && left->kind == TypeKind::Tuple) {
            auto &rtypes = to<TypeTuple>(right)->types;
            auto &ltypes = to<TypeTuple>(left)->types;
            for (auto i = 0; i < rtypes.size(); i++) {
//                auto rightType = indexAccess(left, { kind: TypeKind::Literal, literal: i });
//                auto leftType = indexAccess(right, { kind: TypeKind::Literal, literal: i });
//                if (rightType->kind == TypeKind::Infer || leftType->kind == TypeKind::Infer) continue;
//                auto valid = isExtendable(leftType, rightType, extendStack);
//                if (!valid) return false;
            }
//            inferFromTuple(right, left);

//            return true;
        }

        if (right->kind == TypeKind::Union) {
            if (left->kind != TypeKind::Union) {
                for (auto &&l: to<TypeUnion>(right)->types) {
                    if (isExtendable(left, l)) return true;
                }
                return false;
            } else {
                //e.g.: string|number = string|boolean
                auto rightTypes = to<TypeUnion>(right)->types;
                auto leftTypes = to<TypeUnion>(left)->types;

                for (auto &&r: leftTypes) {
                    bool valid = false;
                    for (auto &&l: rightTypes) {
                        if (isExtendable(l, r)) {
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