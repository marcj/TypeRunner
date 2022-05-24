#pragma once

#include "types.h"

namespace ts {
    template<class T>
    concept CBaseNodeStructure = std::is_base_of<BaseNodeStructure, T>::value;

    template<class T>
    NodeType<T> createBaseNode() {
        NodeType<T> node;
        node.data = reinterpret_cast<Unknown *>(new T);
        node.data->kind = (types::SyntaxKind)T::KIND;
        return node;
    }
}
