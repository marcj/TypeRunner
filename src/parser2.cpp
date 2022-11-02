
#include "parser2.h"

namespace tr {
    SyntaxKind Parser::token() {
        return currentToken;
    }

    SyntaxKind Parser::nextTokenWithoutCheck() {
        return currentToken = scanner.scan();
    }

    optional<DiagnosticWithDetachedLocation> Parser::parseErrorAtPosition(int start, int length, const shared_ptr<DiagnosticMessage> &message, DiagnosticArg arg) {
        ZoneScoped;

        auto lastError = lastOrUndefined(parseDiagnostics);
        // Don't report another error if it would just be at the same position as the last error.
        if (!lastError || start != lastError->start) {
            auto d = createDetachedDiagnostic(fileName, start, length, message, {arg});
            parseDiagnostics.push_back(d);
            return d;
        }

        // Mark that we've encountered an error.  We'll set an appropriate bit on the next
        // node we finish so that it can't be reused incrementally.
        parseErrorBeforeNextFinishedNode = true;

        return nullopt;
    }

    node<Node> Parser::countNode(const node<Node> &node) {
        nodeCount++;
        return node;
    }
}