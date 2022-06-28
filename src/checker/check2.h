#pragma once

#include "./types2.h"

namespace ts::vm2 {
    /**
     * `left extends right ? true : false`
     */
    bool extends(Type *left, Type *right) {
        switch (right->kind) {
            case TypeKind::Literal: {
                switch (left->kind) {
                    case TypeKind::Literal:
                        //todo: literal type
                        return left->hash == right->hash;
                }
                break;
            }
            case TypeKind::String: {
                switch (left->kind) {
                    case TypeKind::String: return true;
                    case TypeKind::Literal: return left->flag | TypeFlag::StringLiteral;
                }
                break;
            }
            case TypeKind::Number: {
                switch (left->kind) {
                    case TypeKind::Number: return true;
                    case TypeKind::Literal: return left->flag | TypeFlag::NumberLiteral;
                }
                break;
            }
        }
        return false;
    }
}