#include "syntax_cursor.h"

using namespace ts;

Node SyntaxCursor::currentNode(int position) {
    // Only compute the current node if the position is different than the last time
    // we were asked.  The parser commonly asks for the node at the same position
    // twice.  Once to know if can read an appropriate list element at a certain point,
    // and then to actually read and consume the node.
//    if (position != lastQueriedPosition) {
//        // Much of the time the parser will need the very next node in the array that
//        // we just returned a node from.So just simply check for that case and move
//        // forward in the array instead of searching for the node again.
//        if (current && current->end == position && currentArrayIndex < (currentArray->length() - 1)) {
//            currentArrayIndex++;
//            current = &currentArray->list[currentArrayIndex];
//        }
//
//        // If we don't have a node, or the node we have isn't in the right position,
//        // then try to find a viable node at the position requested.
//        if (!current || current->pos != position) {
//            findHighestListElementThatStartsAtPosition(position);
//        }
//    }

    // Cache this query so that we don't do any extra work if the parser calls back
    // into us.  Note: this is very common as the parser will make pairs of calls like
    // 'isListElement -> parseListElement'.  If we were unable to find a node when
    // called with 'isListElement', we don't want to redo the work when parseListElement
    // is called immediately after.
    lastQueriedPosition = position;

    // Either we don'd have a node, or we have a node at the position being asked for.
//    assert(!current || current->pos == position);
    return *current;
}

void SyntaxCursor::findHighestListElementThatStartsAtPosition(int position) {
// Clear out any cached state about the last node we found.
//    currentArray = nullptr;
//    currentArrayIndex = InvalidPosition::Value;
//    current = nullptr;
//
//    // Recurse into the source file to find the highest node at this position.
//    forEachChild(sourceFile, visitNode, visitArray);
//    return;
//
//    function visitNode(node: Node) {
//        if (position >= node.pos && position < node.end) {
//            // Position was within this node.  Keep searching deeper to find the node.
//            forEachChild(node, visitNode, visitArray);
//
//            // don't proceed any further in the search.
//            return true;
//        }
//
//        // position wasn't in this node, have to keep searching.
//        return false;
//    }
//
//    function visitArray(array: NodeArray<Node>) {
//        if (position >= array.pos && position < array.end) {
//            // position was in this array.  Search through this array to see if we find a
//            // viable element.
//            for (let i = 0; i < array.length; i++) {
//                const child = array[i];
//                if (child) {
//                    if (child.pos === position) {
//                        // Found the right node.  We're done.
//                        currentArray = array;
//                        currentArrayIndex = i;
//                        current = child;
//                        return true;
//                    }
//                    else {
//                        if (child.pos < position && position < child.end) {
//                            // Position in somewhere within this child.  Search in it and
//                            // stop searching in this array.
//                            forEachChild(child, visitNode, visitArray);
//                            return true;
//                        }
//                    }
//                }
//            }
//        }
//
//        // position wasn't in this array, have to keep searching.
//        return false;
//    }
}
