#pragma once

#include <memory>
#include "types.h"

using namespace std;

namespace ts {
    struct IncrementalElement : ReadonlyTextRange {
        Node *parent = nullptr;
        bool intersectsChange;
        int length;
//        _children: Node[] | undefined;
        int pos;
        int end;
    };

    enum InvalidPosition {
        Value = -1
    };

    //not used
//    struct IncrementalNode : Node, IncrementalElement {
//        bool hasBeenIncrementallyParsed;
//    };

    class SyntaxCursor {
        SourceFile *sourceFile;
//        NodeArray *currentArray;

        int currentArrayIndex = 0;

//        assert(currentArrayIndex < currentArray.length);
        Node *current;
        int lastQueriedPosition = InvalidPosition::Value;
    public:
        SyntaxCursor(SourceFile *sourceFile) : sourceFile(sourceFile) {
//            currentArray = &sourceFile->statements;
//            current = &currentArray->list[currentArrayIndex];
        }

        Node currentNode(int position);

        // Finds the highest element in the tree we can find that starts at the provided position.
        // The element must be a direct child of some node list in the tree.  This way after we
        // return it, we can easily return its next sibling in the list.
        void findHighestListElementThatStartsAtPosition(int position);
    };

} // ts

