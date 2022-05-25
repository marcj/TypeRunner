#pragma once

#include "types.h"

namespace ts {
//    template<class T>
//    concept CBaseNodeStructure = std::is_base_of<BaseNode, T>::value;

    template<class T>
    T createBaseNode() {
        T node;
//        node.data = reinterpret_cast<Node *>(new T);
//        node.data->kind = (types::SyntaxKind)T::KIND;
        return node;
    }
}
