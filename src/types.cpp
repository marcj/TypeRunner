
#include "types.h"

namespace ts {
    types::SyntaxKind BaseUnion::kind() {
        return node->kind;
    }
    bool BaseUnion::empty() {
        return node->kind == SyntaxKind::Unknown;
    }
}