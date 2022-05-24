
#include "types.h"

namespace ts {
    Node::~Node() {
        if (data) delete data;
    }

    types::SyntaxKind Node::kind() {
        if (data) return data->kind;
        return types::Unknown;
    }
}