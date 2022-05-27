#pragma once

#include <string>
#include <regex>
#include <any>
#include "types.h"
#include "utf.h"

using namespace ts::types;
using namespace std;

namespace ts {
    struct ScanNumber {
        SyntaxKind type;
        string value;
    };
    using ErrorCallback = function<void(DiagnosticMessage message, int length)>;

    class Scanner {
    public:
        string text;

        // Current position (end position of text of current token)
        int pos{};

        // end of text
        int end{};

        // Start position of whitespace before current token
        int startPos{};

        // Start position of text of current token
        int tokenPos{};

        bool skipTrivia = false;

        ScriptTarget languageVersion;

        int inJSDocType = 0;

        SyntaxKind token;
        string tokenValue;
        int tokenFlags{};

        LanguageVariant languageVariant = LanguageVariant::Standard;

        vector<CommentDirective> commentDirectives;

        optional<ErrorCallback> onError;

        explicit Scanner(const string &text): text(text) {
            end = text.size();
        }

        explicit Scanner(ScriptTarget languageVersion, bool skipTrivia): languageVersion(languageVersion), skipTrivia(skipTrivia) {
        }

        SyntaxKind scan();

        SyntaxKind scanIdentifier(CharCode startCharacter, ScriptTarget languageVersion);

        bool hasUnicodeEscape() {
            //todo
            return false;
        }

        void setTextPos(int textPos) {
            assert(textPos >= 0);
            pos = textPos;
            startPos = textPos;
            tokenPos = textPos;
            token = SyntaxKind::Unknown;
            tokenValue = "";
            tokenFlags = TokenFlags::None;
        }

        void setOnError(optional<ErrorCallback> errorCallback) {
            onError = errorCallback;
        }

        void setText(string newText = "", int start = 0, int length = - 1) {
            text = newText;
            end = length == - 1 ? text.size() : start + length;
            setTextPos(start);
        }

        void setScriptTarget(ScriptTarget scriptTarget) {
            languageVersion = scriptTarget;
        }

        void setLanguageVariant(LanguageVariant variant) {
            languageVariant = variant;
        }

        void clearCommentDirectives() {
            commentDirectives.clear();
        }

        bool hasExtendedUnicodeEscape() {
            //todo
            return false;
        }

        int getTextPos() {
            return pos;
        }

        int getTokenPos() {
            return tokenPos;
        }

        int getStartPos() {
            return startPos;
        }

        bool lookAhead(function<any()> callback);

        bool tryScan(function<any()> callback);

        bool speculationHelper(function<any()> callback, bool isLookahead);

        bool hasPrecedingLineBreak() {
            return (tokenFlags & TokenFlags::PrecedingLineBreak) != 0;
        }

    private:
        string scanBinaryOrOctalDigits(int base);

        int scanExactNumberOfHexDigits(int count, bool canHaveSeparators);

        SyntaxKind scanTemplateAndSetTokenValue(bool isTaggedTemplate);

        ScanNumber scanNumber();

        CharCode peekExtendedUnicodeEscape();

        CharCode peekUnicodeEscape();

        SyntaxKind checkBigIntSuffix();

        string scanIdentifierParts();

        string scanNumberFragment();

        void checkForIdentifierStartAfterNumericLiteral(int numericStart, bool isScientific = false);

        string scanExtendedUnicodeEscape();

        string scanHexadecimalEscape(int numDigits);

        string scanMinimumNumberOfHexDigits(int count, bool canHaveSeparators);

        string scanHexDigits(int minCount, bool scanAsManyAsPossible, bool canHaveSeparators);

        string scanEscapeSequence(bool isTaggedTemplate = false);

        string scanString(bool jsxAttributeString = false);

        bool isOctalDigit(CharCode code);

        int error(DiagnosticMessage message, int errPos = - 1, int length = - 1);

        vector<CommentDirective> appendIfCommentDirective(vector<CommentDirective> &commentDirectives, const string &text, const regex &commentDirectiveRegEx, int lineStart);

        int scanOctalDigits();

        int scanConflictMarkerTrivia(string &text, int pos);

        SyntaxKind getIdentifierToken();
    };
}
