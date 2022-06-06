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

        return false;
    }
}