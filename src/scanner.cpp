#include <vector>
#include <regex>
#include "scanner.h"
#include "utf.h"
#include "core.h"
#include "utilities.h"
#include "diagnostic_messages.h"
#include <optional>

using namespace ts;
using namespace std;

namespace ts {
    bool isShebangTrivia(const string &text, int pos) {
        // Shebangs check must only be done at the start of the file
        //    Debug.assert(pos == 0);
        //    return shebangTriviaRegex.test(text);
        //todo: implement
        return false;
    }

    bool isWhiteSpaceSingleLine(const CharCode &ch) {
        // Note: nextLine is in the Zs space, and should be considered to be a whitespace.
        // It is explicitly not a line-break as it isn't in the exact set specified by EcmaScript.
        return ch.code == CharacterCodes::space ||
               ch.code == CharacterCodes::tab ||
               ch.code == CharacterCodes::verticalTab ||
               ch.code == CharacterCodes::formFeed ||
               ch.code == CharacterCodes::nonBreakingSpace ||
               ch.code == CharacterCodes::nextLine ||
               ch.code == CharacterCodes::ogham ||
               (ch.code >= CharacterCodes::enQuad && ch.code <= CharacterCodes::zeroWidthSpace) ||
               ch.code == CharacterCodes::narrowNoBreakSpace ||
               ch.code == CharacterCodes::mathematicalSpace ||
               ch.code == CharacterCodes::ideographicSpace ||
               ch.code == CharacterCodes::byteOrderMark;
    }

    string Scanner::scanString(bool jsxAttributeString) {
        ZoneScoped;
        auto quote = charCodeAt(text, pos);
        pos++;
        string result;
        auto start = pos;
        while (true) {
            if (pos >= end) {
                result += substring(text, start, pos);
                tokenFlags |= TokenFlags::Unterminated;
//            error(Diagnostics::Unterminated_string_literal());
                break;
            }
            auto ch = charCodeAt(text, pos);
            if (ch.code == quote.code) {
                result += substring(text, start, pos);
                pos++;
                break;
            }
            if (ch.code == CharacterCodes::backslash && !jsxAttributeString) {
                result += substring(text, start, pos);
                result += scanEscapeSequence();
                start = pos;
                continue;
            }
            if (isLineBreak(ch) && !jsxAttributeString) {
                result += substring(text, start, pos);
                tokenFlags |= TokenFlags::Unterminated;
//            error(Diagnostics::Unterminated_string_literal());
                break;
            }
            pos++;
        }
        return result;
    }

    bool isDigit(const CharCode &ch) {
        return ch.code >= CharacterCodes::_0 && ch.code <= CharacterCodes::_9;
    }

    bool isHexDigit(const CharCode &ch) {
        return isDigit(ch) || (ch.code >= CharacterCodes::A && ch.code <= CharacterCodes::F) ||
               (ch.code >= CharacterCodes::a && ch.code <= CharacterCodes::f);
    }

    string Scanner::scanMinimumNumberOfHexDigits(int count, bool canHaveSeparators) {
        return scanHexDigits(/*minCount*/ count, /*scanAsManyAsPossible*/ true, canHaveSeparators);
    }

    string Scanner::scanHexDigits(int minCount, bool scanAsManyAsPossible, bool canHaveSeparators) {
        ZoneScoped;
        auto allowSeparator = false;
        auto isPreviousTokenSeparator = false;
        auto found = 0;
        string result;
        while (found < minCount || scanAsManyAsPossible) {
            auto ch = charCodeAt(text, pos);
            if (canHaveSeparators && ch.code == CharacterCodes::_) {
                tokenFlags |= TokenFlags::ContainsSeparator;
                if (allowSeparator) {
                    allowSeparator = false;
                    isPreviousTokenSeparator = true;
                } else if (isPreviousTokenSeparator) {
//                error(Diagnostics::Multiple_consecutive_numeric_separators_are_not_permitted(), pos, 1);
                } else {
//                error(Diagnostics::Numeric_separators_are_not_allowed_here(), pos, 1);
                }
                pos++;
                continue;
            }
            allowSeparator = canHaveSeparators;
            if (ch.code >= CharacterCodes::A && ch.code <= CharacterCodes::F) {
                ch.code += CharacterCodes::a - CharacterCodes::A; // standardize hex literals to lowercase
            } else if (!((ch.code >= CharacterCodes::_0 && ch.code <= CharacterCodes::_9) ||
                         (ch.code >= CharacterCodes::a && ch.code <= CharacterCodes::f)
            )) {
                break;
            }
            found++;
            result.append(substring(text, pos, ch.length));
            pos++;
            isPreviousTokenSeparator = false;
        }

//    if (charCodeAt(text, pos - 1) == CharacterCodes::_) {
//        error(Diagnostics::Numeric_separators_are_not_allowed_here(), pos - 1, 1);
//    }

        return result;
    }

    bool isCodePoint(const CharCode &ch) {
        return ch.code <= 0x10FFFF;
    }

    int Scanner::scanExactNumberOfHexDigits(int count, bool canHaveSeparators) {
        auto valueString = scanHexDigits(/*minCount*/ count, /*scanAsManyAsPossible*/ false, canHaveSeparators);
        return !valueString.empty() ? stoi(valueString, nullptr, 16) : -1;
    }

    string Scanner::scanHexadecimalEscape(int numDigits) {
        auto escapedValue = scanExactNumberOfHexDigits(numDigits, /*canHaveSeparators*/ false);

        if (escapedValue >= 0) {
            return fromCharCode(escapedValue);
        } else {
//        error(Diagnostics::Hexadecimal_digit_expected());
            return "";
        }
    }

    string Scanner::scanExtendedUnicodeEscape() {
        ZoneScoped;
        auto escapedValueString = scanMinimumNumberOfHexDigits(1, /*canHaveSeparators*/ false);
        auto escapedValue = !escapedValueString.empty() ? stoi(escapedValueString, nullptr, 16) : -1;
        auto isInvalidExtendedEscape = false;

        // Validate the value of the digit
        if (escapedValue < 0) {
//        error(Diagnostics::Hexadecimal_digit_expected());
            isInvalidExtendedEscape = true;
        } else if (escapedValue > 0x10FFFF) {
//        error(Diagnostics::An_extended_Unicode_escape_value_must_be_between_0x0_and_0x10FFFF_inclusive());
            isInvalidExtendedEscape = true;
        }

        if (pos >= end) {
//        error(Diagnostics::Unexpected_end_of_text());
            isInvalidExtendedEscape = true;
        } else if (charCodeAt(text, pos).code == CharacterCodes::closeBrace) {
            // Only swallow the following character up if it's a '}'.
            pos++;
        } else {
//        error(Diagnostics::Unterminated_Unicode_escape_sequence());
            isInvalidExtendedEscape = true;
        }

        if (isInvalidExtendedEscape) {
            return "";
        }

        return fromCharCode(escapedValue);
//    return utf16EncodeAsString(escapedValue);
    }

    string Scanner::scanEscapeSequence(bool isTaggedTemplate) {
        ZoneScoped;
        auto start = pos;
        pos++;
        if (pos >= end) {
//        error(Diagnostics::Unexpected_end_of_text());
            return "";
        }
        auto ch = charCodeAt(text, pos);
        pos++;
        switch (ch.code) {
            case CharacterCodes::_0:
                // '\01'
                if (isTaggedTemplate && pos < end && isDigit(charCodeAt(text, pos))) {
                    pos++;
                    tokenFlags |= TokenFlags::ContainsInvalidEscape;
                    return substring(text, start, pos);
                }
                return "\0";
            case CharacterCodes::b:
                return "\b";
            case CharacterCodes::t:
                return "\t";
            case CharacterCodes::n:
                return "\n";
            case CharacterCodes::v:
                return "\v";
            case CharacterCodes::f:
                return "\f";
            case CharacterCodes::r:
                return "\r";
            case CharacterCodes::singleQuote:
                return "\'";
            case CharacterCodes::doubleQuote:
                return "\"";
            case CharacterCodes::u:
                if (isTaggedTemplate) {
                    // '\u' or '\u0' or '\u00' or '\u000'
                    for (auto escapePos = pos; escapePos < pos + 4; escapePos++) {
                        if (escapePos < end && !isHexDigit(charCodeAt(text, escapePos)) &&
                            charCodeAt(text, escapePos).code != CharacterCodes::openBrace) {
                            pos = escapePos;
                            tokenFlags |= TokenFlags::ContainsInvalidEscape;
                            return substring(text, start, pos);
                        }
                    }
                }
                // '\u{DDDDDDDD}'
                if (pos < end && charCodeAt(text, pos).code == CharacterCodes::openBrace) {
                    pos++;

                    // '\u{'
                    if (isTaggedTemplate && !isHexDigit(charCodeAt(text, pos))) {
                        tokenFlags |= TokenFlags::ContainsInvalidEscape;
                        return substring(text, start, pos);
                    }

                    if (isTaggedTemplate) {
                        auto savePos = pos;
                        auto escapedValueString = scanMinimumNumberOfHexDigits(1, /*canHaveSeparators*/ false);

                        try {
                            auto escapedValue = !escapedValueString.empty() ? stoi(escapedValueString, nullptr, 16) : -1;

                            // '\u{Not Code Point' or '\u{CodePoint'
                            if (charCodeAt(text, pos).code != CharacterCodes::closeBrace) {
                                tokenFlags |= TokenFlags::ContainsInvalidEscape;
                                return substring(text, start, pos);
                            } else {
                                pos = savePos;
                            }
                        } catch (invalid_argument &error) {
                            tokenFlags |= TokenFlags::ContainsInvalidEscape;
                            return substring(text, start, pos);
                        }
                    }
                    tokenFlags |= TokenFlags::ExtendedUnicodeEscape;
                    return scanExtendedUnicodeEscape();
                }

                tokenFlags |= TokenFlags::UnicodeEscape;
                // '\uDDDD'
                return scanHexadecimalEscape(/*numDigits*/ 4);

            case CharacterCodes::x:
                if (isTaggedTemplate) {
                    if (!isHexDigit(charCodeAt(text, pos))) {
                        tokenFlags |= TokenFlags::ContainsInvalidEscape;
                        return substring(text, start, pos);
                    } else if (!isHexDigit(charCodeAt(text, pos + 1))) {
                        pos++;
                        tokenFlags |= TokenFlags::ContainsInvalidEscape;
                        return substring(text, start, pos);
                    }
                }
                // '\xDD'
                return scanHexadecimalEscape(/*numDigits*/ 2);

                // when encountering a LineContinuation (i.e. a backslash and a line terminator sequence),
                // the line terminator is interpreted to be "the empty code unit sequence".
            case CharacterCodes::carriageReturn:
                if (pos < end && charCodeAt(text, pos).code == CharacterCodes::lineFeed) {
                    pos++;
                }
                // falls through
            case CharacterCodes::lineFeed:
            case CharacterCodes::lineSeparator:
            case CharacterCodes::paragraphSeparator:
                return "";
            default:
                return fromCharCode(ch.code);
        }
    }

    SyntaxKind Scanner::scanTemplateAndSetTokenValue(bool isTaggedTemplate) {
        ZoneScoped;
        auto startedWithBacktick = charCodeAt(text, pos).code == CharacterCodes::backtick;

        pos++;
        auto start = pos;
        string contents;
        SyntaxKind resultingToken;

        while (true) {
            if (pos >= end) {
                contents += substring(text, start, pos);
                tokenFlags |= TokenFlags::Unterminated;
//            error(Diagnostics::Unterminated_template_literal());
                resultingToken = startedWithBacktick ? SyntaxKind::NoSubstitutionTemplateLiteral : SyntaxKind::TemplateTail;
                break;
            }

            auto currChar = charCodeAt(text, pos);

            // '`'
            if (currChar.code == CharacterCodes::backtick) {
                contents += substring(text, start, pos);
                pos++;
                resultingToken = startedWithBacktick ? SyntaxKind::NoSubstitutionTemplateLiteral : SyntaxKind::TemplateTail;
                break;
            }

            // '${'
            if (currChar.code == CharacterCodes::$ && pos + 1 < end && charCodeAt(text, pos + 1).code == CharacterCodes::openBrace) {
                contents += substring(text, start, pos);
                pos += 2;
                resultingToken = startedWithBacktick ? SyntaxKind::TemplateHead : SyntaxKind::TemplateMiddle;
                break;
            }

            // Escape character
            if (currChar.code == CharacterCodes::backslash) {
                contents += substring(text, start, pos);
                contents += scanEscapeSequence(isTaggedTemplate);
                start = pos;
                continue;
            }

            // Speculated ECMAScript 6 Spec 11.8.6.1:
            // <CR><LF> and <CR> LineTerminatorSequences are normalized to <LF> for Template Values
            if (currChar.code == CharacterCodes::carriageReturn) {
                contents += substring(text, start, pos);
                pos++;

                if (pos < end && charCodeAt(text, pos).code == CharacterCodes::lineFeed) {
                    pos++;
                }

                contents += "\n";
                start = pos;
                continue;
            }

            pos++;
        }

//    Debug.assert(resultingToken !== undefined);

        tokenValue = contents;
        return resultingToken;
    }

    string Scanner::scanNumberFragment() {
        ZoneScoped;
        auto start = pos;
        auto allowSeparator = false;
        auto isPreviousTokenSeparator = false;
        string result;
        while (true) {
            auto ch = charCodeAt(text, pos);
            if (ch.code == CharacterCodes::_) {
                tokenFlags |= TokenFlags::ContainsSeparator;
                if (allowSeparator) {
                    allowSeparator = false;
                    isPreviousTokenSeparator = true;
                    result += substring(text, start, pos);
                } else if (isPreviousTokenSeparator) {
//                error(Diagnostics::Multiple_consecutive_numeric_separators_are_not_permitted(), pos, 1);
                } else {
//                error(Diagnostics::Numeric_separators_are_not_allowed_here(), pos, 1);
                }
                pos++;
                start = pos;
                continue;
            }
            if (isDigit(ch)) {
                allowSeparator = true;
                isPreviousTokenSeparator = false;
                pos++;
                continue;
            }
            break;
        }
        if (charCodeAt(text, pos - 1).code == CharacterCodes::_) {
//        error(Diagnostics::Numeric_separators_are_not_allowed_here(), pos - 1, 1);
        }
        return result + substring(text, start, pos);
    }

/**
 * Test for whether a single line comment with leading whitespace trimmed's text contains a directive.
 */
    const regex commentDirectiveRegExSingleLine("^///?\\s*@(ts-expect-error|ts-ignore)");

/**
 * Test for whether a multi-line comment with leading whitespace trimmed's last line contains a directive.
 */
    const regex commentDirectiveRegExMultiLine("^(?:/|\\*)*\\s*@(ts-expect-error|ts-ignore)");

// All conflict markers consist of the same character repeated seven times.  If it is
// a <<<<<<< or >>>>>>> marker then it is also followed by a space.
    const unsigned long mergeConflictMarkerLength = size("<<<<<<<") - 1;

    bool isConflictMarkerTrivia(const string &text, int pos) {
        ZoneScoped;
        assert(pos >= 0);

        // Conflict markers must be at the start of a line.
        if (pos == 0 || isLineBreak(charCodeAt(text, pos - 1))) {
            auto ch = charCodeAt(text, pos);

            if ((pos + mergeConflictMarkerLength) < text.size()) {
                for (int i = 0; i < mergeConflictMarkerLength; i++) {
                    if (charCodeAt(text, pos + i).code != ch.code) {
                        return false;
                    }
                }

                return ch.code == CharacterCodes::equals || charCodeAt(text, pos + mergeConflictMarkerLength).code == CharacterCodes::space;
            }
        }

        return false;
    }

    int Scanner::error(const shared<DiagnosticMessage> &message, int errPos, int length) {
        if (errPos == -1) errPos = pos;

        cout << "Error: " << message->code << ": " << message->message << " at " << errPos << "\n";

        if (onError) {
            (*onError)(message, length);
        }
    }

    int scanConflictMarkerTrivia(string &text, int pos) {
        ZoneScoped;
        auto ch = charCodeAt(text, pos);
        auto len = text.size();

        if (ch.code == CharacterCodes::lessThan || ch.code == CharacterCodes::greaterThan) {
            while (pos < len && !isLineBreak(charCodeAt(text, pos))) {
                pos++;
            }
        } else {
            assert(ch.code == CharacterCodes::bar || ch.code == CharacterCodes::equals);
            // Consume everything from the start of a ||||||| or ===== marker to the start
            // of the next ===== or >>>>>>> marker.
            while (pos < len) {
                auto currentChar = charCodeAt(text, pos);
                if ((currentChar.code == CharacterCodes::equals || currentChar.code == CharacterCodes::greaterThan) && currentChar.code != ch.code && isConflictMarkerTrivia(text, pos)) {
                    break;
                }

                pos++;
            }
        }

        return pos;
    }

    int Scanner::scanConflictMarkerTrivia(string &text, int pos) {
        error(Diagnostics::Merge_conflict_marker_encountered(), pos, mergeConflictMarkerLength);
        return scanConflictMarkerTrivia(text, pos);
    }

    bool lookupInUnicodeMap(CharCode code, const vector<int> &map) {
        ZoneScoped;
        // Bail out quickly if it couldn't possibly be in the map.
        if (code.code < map[0]) {
            return false;
        }

        // Perform binary search in one of the Unicode range maps
        auto lo = 0;
        auto hi = map.size();
        int mid;

        while (lo + 1 < hi) {
            mid = lo + (hi - lo) / 2;
            // mid has to be even to catch a range's beginning
            mid -= mid % 2;
            if (map[mid] <= code.code && code.code <= map[mid + 1]) {
                return true;
            }

            if (code.code < map[mid]) {
                hi = mid;
            } else {
                lo = mid + 2;
            }
        }

        return false;
    }

    bool isUnicodeIdentifierStart(CharCode code, ScriptTarget languageVersion) {
        return languageVersion >= ScriptTarget::ES2015 ?
               lookupInUnicodeMap(code, unicodeESNextIdentifierStart()) :
               languageVersion == ScriptTarget::ES5 ? lookupInUnicodeMap(code, unicodeES5IdentifierStart()) :
               lookupInUnicodeMap(code, unicodeES3IdentifierStart());
    }

    bool isUnicodeIdentifierPart(const CharCode &ch, ScriptTarget languageVersion) {
        return languageVersion >= ScriptTarget::ES2015 ?
               lookupInUnicodeMap(ch, unicodeESNextIdentifierPart()) :
               languageVersion == ScriptTarget::ES5 ? lookupInUnicodeMap(ch, unicodeES5IdentifierPart()) :
               lookupInUnicodeMap(ch, unicodeES3IdentifierPart());
    }

    bool isIdentifierStart(const CharCode &ch, ScriptTarget languageVersion) {
        return (ch.code >= CharacterCodes::A && ch.code <= CharacterCodes::Z) || (ch.code >= CharacterCodes::a && ch.code <= CharacterCodes::z) ||
               ch.code == CharacterCodes::$ || ch.code == CharacterCodes::_ ||
               (ch.code > CharacterCodes::maxAsciiCharacter && isUnicodeIdentifierStart(ch, languageVersion));
    }

    /* @internal */
    bool isIdentifierText(string name, ScriptTarget languageVersion, LanguageVariant identifierVariant) {
        auto ch = charCodeAt(name, 0);
        if (!isIdentifierStart(ch, languageVersion)) {
            return false;
        }

        for (int i = ch.length; i < name.size(); i += ch.length) {
            if (!isIdentifierPart(ch = charCodeAt(name, i), languageVersion, identifierVariant)) {
                return false;
            }
        }

        return true;
    }

    bool isIdentifierPart(const CharCode &ch, ScriptTarget languageVersion, LanguageVariant identifierVariant) {
        return (ch.code >= CharacterCodes::A && ch.code <= CharacterCodes::Z) || (ch.code >= CharacterCodes::a && ch.code <= CharacterCodes::z) ||
               (ch.code >= CharacterCodes::_0 && ch.code <= CharacterCodes::_9) || ch.code == CharacterCodes::$ || ch.code == CharacterCodes::_ ||
               // "-" and ":" are valid in JSX Identifiers
               (identifierVariant == LanguageVariant::JSX ? (ch.code == CharacterCodes::minus || ch.code == CharacterCodes::colon) : false) ||
               (ch.code > CharacterCodes::maxAsciiCharacter && isUnicodeIdentifierPart(ch, languageVersion));
    }

    const regex shebangTriviaRegex("^#!.*");

    int scanShebangTrivia(const string &text, int pos) {
        smatch m;
        if (regex_search(text, m, shebangTriviaRegex)) {
            pos = pos + m[1].length();
        }
        return pos;
    }

    bool isWhiteSpaceLike(const CharCode &ch) {
        return isWhiteSpaceSingleLine(ch) || isLineBreak(ch);
    }

    /* @internal */
    int ts::skipTrivia(string &text, int pos, optional<bool> stopAfterLineBreak, optional<bool> stopAtComments, optional<bool> inJSDoc) {
        ZoneScoped;
        if (positionIsSynthesized(pos)) {
            return pos;
        }

        auto canConsumeStar = false;
        // Keep in sync with couldStartTrivia
        while (true) {
            auto ch = charCodeAt(text, pos);
            switch (ch.code) {
                case CharacterCodes::carriageReturn:
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::lineFeed) {
                        pos++;
                    }
                    // falls through
                case CharacterCodes::lineFeed:
                    pos++;
                    if (stopAfterLineBreak && *stopAfterLineBreak) {
                        return pos;
                    }
                    canConsumeStar = inJSDoc && *inJSDoc;
                    continue;
                case CharacterCodes::tab:
                case CharacterCodes::verticalTab:
                case CharacterCodes::formFeed:
                case CharacterCodes::space:
                    pos++;
                    continue;
                case CharacterCodes::slash:
                    if (stopAtComments && *stopAtComments) {
                        break;
                    }
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::slash) {
                        pos += 2;
                        while (pos < text.size()) {
                            if (isLineBreak(charCodeAt(text, pos))) {
                                break;
                            }
                            pos++;
                        }
                        canConsumeStar = false;
                        continue;
                    }
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::asterisk) {
                        pos += 2;
                        while (pos < text.size()) {
                            if (charCodeAt(text, pos).code == CharacterCodes::asterisk && charCodeAt(text, pos + 1).code == CharacterCodes::slash) {
                                pos += 2;
                                break;
                            }
                            pos++;
                        }
                        canConsumeStar = false;
                        continue;
                    }
                    break;

                case CharacterCodes::lessThan:
                case CharacterCodes::bar:
                case CharacterCodes::equals:
                case CharacterCodes::greaterThan:
                    if (isConflictMarkerTrivia(text, pos)) {
                        pos = scanConflictMarkerTrivia(text, pos);
                        canConsumeStar = false;
                        continue;
                    }
                    break;

                case CharacterCodes::hash:
                    if (pos == 0 && isShebangTrivia(text, pos)) {
                        pos = scanShebangTrivia(text, pos);
                        canConsumeStar = false;
                        continue;
                    }
                    break;

                case CharacterCodes::asterisk:
                    if (canConsumeStar) {
                        pos++;
                        canConsumeStar = false;
                        continue;
                    }
                    break;

                default:
                    if (ch.code > CharacterCodes::maxAsciiCharacter && (isWhiteSpaceLike(ch))) {
                        pos++;
                        continue;
                    }
                    break;
            }
            return pos;
        }
    }

    CharCode Scanner::peekExtendedUnicodeEscape() {
        if (languageVersion >= ScriptTarget::ES2015 && charCodeAt(text, pos + 1).code == CharacterCodes::u && charCodeAt(text, pos + 2).code == CharacterCodes::openBrace) {
            auto start = pos;
            pos += 3;
            auto escapedValueString = scanMinimumNumberOfHexDigits(1, /*canHaveSeparators*/ false);
            auto escapedValue = !escapedValueString.empty() ? stoi(escapedValueString, nullptr, 16) : -1;
            pos = start;
            return {escapedValue, 1};
        }
        return {-1, 0};
    }

    CharCode Scanner::peekUnicodeEscape() {
        if (pos + 5 < end && charCodeAt(text, pos + 1).code == CharacterCodes::u) {
            auto start = pos;
            pos += 2;
            auto value = scanExactNumberOfHexDigits(4, /*canHaveSeparators*/ false);
            pos = start;
            return {value, 1};
        }
        return {-1, 0};
    }

    string Scanner::scanIdentifierParts() {
        string result;
        auto start = pos;
        while (pos < end) {
            auto ch = charCodeAt(text, pos);
            if (isIdentifierPart(ch, languageVersion)) {
                pos += ch.length;
            } else if (ch.code == CharacterCodes::backslash) {
                ch = peekExtendedUnicodeEscape();
                if (ch.code >= 0 && isIdentifierPart(ch, languageVersion)) {
                    pos += 3;
                    tokenFlags |= TokenFlags::ExtendedUnicodeEscape;
                    result += scanExtendedUnicodeEscape();
                    start = pos;
                    continue;
                }
                ch = peekUnicodeEscape();
                if (!(ch.code >= 0 && isIdentifierPart(ch, languageVersion))) {
                    break;
                }
                tokenFlags |= TokenFlags::UnicodeEscape;
                result += substring(text, start, pos);
                result += fromCharCode(ch.code);
                // Valid Unicode escape is always six characters
                pos += 6;
                start = pos;
            } else {
                break;
            }
        }
        result += substring(text, start, pos);
        return result;
    }

    void Scanner::checkForIdentifierStartAfterNumericLiteral(int numericStart, bool isScientific) {
        if (!isIdentifierStart(charCodeAt(text, pos), languageVersion)) {
            return;
        }

        auto identifierStart = pos;
        auto parts = scanIdentifierParts();

        if (parts.size() == 1 && text[identifierStart] == 'n') {
            if (isScientific) {
                error(Diagnostics::A_bigint_literal_cannot_use_exponential_notation(), numericStart, identifierStart - numericStart + 1);
            } else {
                error(Diagnostics::A_bigint_literal_must_be_an_integer(), numericStart, identifierStart - numericStart + 1);
            }
        } else {
            error(Diagnostics::An_identifier_or_keyword_cannot_immediately_follow_a_numeric_literal(), identifierStart, parts.size());
            pos = identifierStart;
        }
    }

    SyntaxKind Scanner::checkBigIntSuffix() {
        ZoneScoped;
        if (charCodeAt(text, pos).code == CharacterCodes::n) {
            tokenValue += "n";
            // Use base 10 instead of base 2 or base 8 for shorter literals
            if (tokenFlags & TokenFlags::BinaryOrOctalSpecifier) {
                tokenValue = parsePseudoBigInt(tokenValue) + "n";
            }
            pos++;
            return SyntaxKind::BigIntLiteral;
        } else {
            // not a bigint, so can convert to number in simplified form
            // Number() may not support 0b or 0o, so use parseInt() instead
            auto numericValue = tokenFlags & TokenFlags::BinarySpecifier
                                ? stoi(substring(tokenValue, 2), 0, 2) // skip "0b"
                                : tokenFlags & TokenFlags::OctalSpecifier
                                  ? stoi(substring(tokenValue, 2), 0, 8) // skip "0o"
                                  : stoi(tokenValue);
            tokenValue = std::to_string(numericValue);
            return SyntaxKind::NumericLiteral;
        }
    }

    optional<CommentDirectiveType> getDirectiveFromComment(const string &text, const regex &commentDirectiveRegEx) {
        cmatch match;
        if (regex_search("//@ts-ignore", match, commentDirectiveRegEx)) {
            if (match[1] == "ts-expect-error") return CommentDirectiveType::ExpectError;
            if (match[1] == "ts-ignore") return CommentDirectiveType::Ignore;
        }
    }

    SyntaxKind Scanner::scanJsxAttributeValue() {
        ZoneScoped;
        startPos = pos;

        switch (charCodeAt(text, pos).code) {
            case CharacterCodes::doubleQuote:
            case CharacterCodes::singleQuote:
                tokenValue = scanString(/*jsxAttributeString*/ true);
                return token = SyntaxKind::StringLiteral;
            default:
                // If this scans anything other than `{`, it's a parse error.
                return scan();
        }
    }

    SyntaxKind Scanner::scanJsxIdentifier() {
        ZoneScoped;
        if (tokenIsIdentifierOrKeyword(token)) {
            // An identifier or keyword has already been parsed - check for a `-` or a single instance of `:` and then append it and
            // everything after it to the token
            // Do note that this means that `scanJsxIdentifier` effectively _mutates_ the visible token without advancing to a new token
            // Any caller should be expecting this behavior and should only read the pos or token value after calling it.
            auto namespaceSeparator = false;
            while (pos < end) {
                auto ch = charCodeAt(text, pos);
                if (ch.code == CharacterCodes::minus) {
                    tokenValue += "-";
                    pos++;
                    continue;
                } else if (ch.code == CharacterCodes::colon && !namespaceSeparator) {
                    tokenValue += ":";
                    pos++;
                    namespaceSeparator = true;
                    token = SyntaxKind::Identifier; // swap from keyword kind to identifier kind
                    continue;
                }
                auto oldPos = pos;
                tokenValue += scanIdentifierParts(); // reuse `scanIdentifierParts` so unicode escapes are handled
                if (pos == oldPos) {
                    break;
                }
            }
            // Do not include a trailing namespace separator in the token, since this is against the spec.
            if (substring(tokenValue, -1) == ":") {
                tokenValue = substring(tokenValue, 0, -1);
                pos--;
            }
            return getIdentifierToken();
        }
        return token;
    }

    SyntaxKind Scanner::scanJsxToken(bool allowMultilineJsxText) {
        ZoneScoped;
        startPos = tokenPos = pos;

        if (pos >= end) {
            return token = SyntaxKind::EndOfFileToken;
        }

        auto charCode = charCodeAt(text, pos);
        if (charCode.code == CharacterCodes::lessThan) {
            if (charCodeAt(text, pos + 1).code == CharacterCodes::slash) {
                pos += 2;
                return token = SyntaxKind::LessThanSlashToken;
            }
            pos++;
            return token = SyntaxKind::LessThanToken;
        }

        if (charCode.code == CharacterCodes::openBrace) {
            pos++;
            return token = SyntaxKind::OpenBraceToken;
        }

        // First non-whitespace character on this line.
        int firstNonWhitespace = 0;

        // These initial values are special because the first line is:
        // firstNonWhitespace = 0 to indicate that we want leading whitespace,

        while (pos < end) {
            charCode = charCodeAt(text, pos);
            if (charCode.code == CharacterCodes::openBrace) {
                break;
            }
            if (charCode.code == CharacterCodes::lessThan) {
                if (isConflictMarkerTrivia(text, pos)) {
                    pos = scanConflictMarkerTrivia(text, pos);
                    return token = SyntaxKind::ConflictMarkerTrivia;
                }
                break;
            }
            if (charCode.code == CharacterCodes::greaterThan) {
                error(Diagnostics::Unexpected_token_Did_you_mean_or_gt(), pos, 1);
            }
            if (charCode.code == CharacterCodes::closeBrace) {
                error(Diagnostics::Unexpected_token_Did_you_mean_or_rbrace(), pos, 1);
            }

            // FirstNonWhitespace is 0, then we only see whitespaces so far. If we see a linebreak, we want to ignore that whitespaces.
            // i.e (- : whitespace)
            //      <div>----
            //      </div> becomes <div></div>
            //
            //      <div>----</div> becomes <div>----</div>
            if (isLineBreak(charCode) && firstNonWhitespace == 0) {
                firstNonWhitespace = -1;
            } else if (!allowMultilineJsxText && isLineBreak(charCode) && firstNonWhitespace > 0) {
                // Stop JsxText on each line during formatting. This allows the formatter to
                // indent each line correctly.
                break;
            } else if (!isWhiteSpaceLike(charCode)) {
                firstNonWhitespace = pos;
            }

            pos++;
        }

        tokenValue = substring(text, startPos, pos);

        return firstNonWhitespace == -1 ? SyntaxKind::JsxTextAllWhiteSpaces : SyntaxKind::JsxText;
    }

    bool isLineBreak(const CharCode &ch) {
        // ES5 7.3:
        // The ECMAScript line terminator characters are listed in Table 3.
        //     Table 3: Line Terminator Characters
        //     Code Unit Value     Name                    Formal Name
        //     \u000A              Line Feed               <LF>
        //     \u000D              Carriage Return         <CR>
        //     \u2028              Line separator          <LS>
        //     \u2029              Paragraph separator     <PS>
        // Only the characters in Table 3 are treated as line terminators. Other new line or line
        // breaking characters are treated as white space but not as line terminators.

        return ch.code == CharacterCodes::lineFeed ||
               ch.code == CharacterCodes::carriageReturn ||
               ch.code == CharacterCodes::lineSeparator ||
               ch.code == CharacterCodes::paragraphSeparator;
    }

    vector<CommentDirective> Scanner::appendIfCommentDirective(
            vector<CommentDirective> &commentDirectives,
            const string &text,
            const regex &commentDirectiveRegEx,
            int lineStart
    ) {
        auto type = getDirectiveFromComment(trimStringStart(text), commentDirectiveRegEx);
        if (!type.has_value()) {
            return commentDirectives;
        }

        commentDirectives.push_back(CommentDirective{.range =  {.pos =  lineStart, .end = pos}, .type = type.value()});
        return commentDirectives;
    }

    ScanNumber Scanner::scanNumber() {
        ZoneScoped;
        auto start = pos;
        auto mainFragment = scanNumberFragment();
        string decimalFragment;
        bool decimalFragmentSet = false;
        string scientificFragment;
        bool scientificFragmentSet = false;
        if (charCodeAt(text, pos).code == CharacterCodes::dot) {
            pos++;
            decimalFragment = scanNumberFragment();
            decimalFragmentSet = true;
        }
        auto end = pos;
        if (charCodeAt(text, pos).code == CharacterCodes::E || charCodeAt(text, pos).code == CharacterCodes::e) {
            pos++;
            tokenFlags |= TokenFlags::Scientific;
            if (charCodeAt(text, pos).code == CharacterCodes::plus || charCodeAt(text, pos).code == CharacterCodes::minus) pos++;
            auto preNumericPart = pos;
            auto finalFragment = scanNumberFragment();
            if (!finalFragment.size()) {
//            error(Diagnostics::Digit_expected());
            } else {
                scientificFragment = substring(text, end, preNumericPart) + finalFragment;
                scientificFragmentSet = true;
                end = pos;
            }
        }
        string result;
        if (tokenFlags & TokenFlags::ContainsSeparator) {
            result = mainFragment;
            if (decimalFragment.size()) {
                result += "." + decimalFragment;
            }
            if (scientificFragment.size()) {
                result += scientificFragment;
            }
        } else {
            result = substring(text, start, end); // No need to use all the fragments; no _ removal needed
        }

        if (decimalFragmentSet || tokenFlags & TokenFlags::Scientific) {
            checkForIdentifierStartAfterNumericLiteral(start, !decimalFragmentSet && !!(tokenFlags & TokenFlags::Scientific));
            return {
                    .type =  SyntaxKind::NumericLiteral,
                    .value =  "" + stoi(result) // if value is not an integer, it can be safely coerced to a number
            };
        } else {
            tokenValue = result;
            auto type = checkBigIntSuffix(); // if value is an integer, check whether it is a bigint
            checkForIdentifierStartAfterNumericLiteral(start);
            return {type, .value = tokenValue};
        }
    }

    string Scanner::scanBinaryOrOctalDigits(int base) {
        ZoneScoped;
        string value;
        // For counting number of digits; Valid binaryIntegerLiteral must have at least one binary digit following B or b.
        // Similarly valid octalIntegerLiteral must have at least one octal digit following o or O.
        bool separatorAllowed = false;
        bool isPreviousTokenSeparator = false;
        while (true) {
            auto ch = charCodeAt(text, pos);
            // Numeric separators are allowed anywhere within a numeric literal, except not at the beginning, or following another separator
            if (ch.code == CharacterCodes::_) {
                tokenFlags |= TokenFlags::ContainsSeparator;
                if (separatorAllowed) {
                    separatorAllowed = false;
                    isPreviousTokenSeparator = true;
                } else if (isPreviousTokenSeparator) {
//                error(Diagnostics::Multiple_consecutive_numeric_separators_are_not_permitted(), pos, 1);
                } else {
//                error(Diagnostics::Numeric_separators_are_not_allowed_here(), pos, 1);
                }
                pos++;
                continue;
            }
            separatorAllowed = true;
            if (!isDigit(ch) || ch.code - CharacterCodes::_0 >= base) {
                break;
            }
            value += text[pos];
            pos++;
            isPreviousTokenSeparator = false;
        }
        if (charCodeAt(text, pos - 1).code == CharacterCodes::_) {
            // Literal ends with underscore - not allowed
//        error(Diagnostics::Numeric_separators_are_not_allowed_here(), pos - 1, 1);
        }
        return value;
    }

    SyntaxKind Scanner::getIdentifierToken() {
        ZoneScoped;
        // Reserved words are between 2 and 12 characters long and start with a lowercase letter
        auto len = tokenValue.size();
        if (len >= 2 && len <= 12) {
            auto ch = charCodeAt(tokenValue, 0);
            if (ch.code >= CharacterCodes::a && ch.code <= CharacterCodes::z) {
                switch (const_hash(tokenValue)) {
                    case "abstract"_hash:
                        return SyntaxKind::AbstractKeyword;
                    case "any"_hash:
                        return SyntaxKind::AnyKeyword;
                    case "as"_hash:
                        return SyntaxKind::AsKeyword;
                    case "asserts"_hash:
                        return SyntaxKind::AssertsKeyword;
                    case "assert"_hash:
                        return SyntaxKind::AssertKeyword;
                    case "bigint"_hash:
                        return SyntaxKind::BigIntKeyword;
                    case "boolean"_hash:
                        return SyntaxKind::BooleanKeyword;
                    case "break"_hash:
                        return SyntaxKind::BreakKeyword;
                    case "case"_hash:
                        return SyntaxKind::CaseKeyword;
                    case "catch"_hash:
                        return SyntaxKind::CatchKeyword;
                    case "class"_hash:
                        return SyntaxKind::ClassKeyword;
                    case "continue"_hash:
                        return SyntaxKind::ContinueKeyword;
                    case "const"_hash:
                        return SyntaxKind::ConstKeyword;
                    case "constructor"_hash:
                        return SyntaxKind::ConstructorKeyword;
                    case "debugger"_hash:
                        return SyntaxKind::DebuggerKeyword;
                    case "declare"_hash:
                        return SyntaxKind::DeclareKeyword;
                    case "default"_hash:
                        return SyntaxKind::DefaultKeyword;
                    case "delete"_hash:
                        return SyntaxKind::DeleteKeyword;
                    case "do"_hash:
                        return SyntaxKind::DoKeyword;
                    case "else"_hash:
                        return SyntaxKind::ElseKeyword;
                    case "enum"_hash:
                        return SyntaxKind::EnumKeyword;
                    case "export"_hash:
                        return SyntaxKind::ExportKeyword;
                    case "extends"_hash:
                        return SyntaxKind::ExtendsKeyword;
                    case "false"_hash:
                        return SyntaxKind::FalseKeyword;
                    case "finally"_hash:
                        return SyntaxKind::FinallyKeyword;
                    case "for"_hash:
                        return SyntaxKind::ForKeyword;
                    case "from"_hash:
                        return SyntaxKind::FromKeyword;
                    case "function"_hash:
                        return SyntaxKind::FunctionKeyword;
                    case "get"_hash:
                        return SyntaxKind::GetKeyword;
                    case "if"_hash:
                        return SyntaxKind::IfKeyword;
                    case "implements"_hash:
                        return SyntaxKind::ImplementsKeyword;
                    case "import"_hash:
                        return SyntaxKind::ImportKeyword;
                    case "in"_hash:
                        return SyntaxKind::InKeyword;
                    case "infer"_hash:
                        return SyntaxKind::InferKeyword;
                    case "instanceof"_hash:
                        return SyntaxKind::InstanceOfKeyword;
                    case "interface"_hash:
                        return SyntaxKind::InterfaceKeyword;
                    case "intrinsic"_hash:
                        return SyntaxKind::IntrinsicKeyword;
                    case "is"_hash:
                        return SyntaxKind::IsKeyword;
                    case "keyof"_hash:
                        return SyntaxKind::KeyOfKeyword;
                    case "let"_hash:
                        return SyntaxKind::LetKeyword;
                    case "module"_hash:
                        return SyntaxKind::ModuleKeyword;
                    case "namespace"_hash:
                        return SyntaxKind::NamespaceKeyword;
                    case "never"_hash:
                        return SyntaxKind::NeverKeyword;
                    case "new"_hash:
                        return SyntaxKind::NewKeyword;
                    case "null"_hash:
                        return SyntaxKind::NullKeyword;
                    case "number"_hash:
                        return SyntaxKind::NumberKeyword;
                    case "object"_hash:
                        return SyntaxKind::ObjectKeyword;
                    case "package"_hash:
                        return SyntaxKind::PackageKeyword;
                    case "private"_hash:
                        return SyntaxKind::PrivateKeyword;
                    case "protected"_hash:
                        return SyntaxKind::ProtectedKeyword;
                    case "public"_hash:
                        return SyntaxKind::PublicKeyword;
                    case "override"_hash:
                        return SyntaxKind::OverrideKeyword;
                    case "out"_hash:
                        return SyntaxKind::OutKeyword;
                    case "readonly"_hash:
                        return SyntaxKind::ReadonlyKeyword;
                    case "require"_hash:
                        return SyntaxKind::RequireKeyword;
                    case "global"_hash:
                        return SyntaxKind::GlobalKeyword;
                    case "return"_hash:
                        return SyntaxKind::ReturnKeyword;
                    case "set"_hash:
                        return SyntaxKind::SetKeyword;
                    case "static"_hash:
                        return SyntaxKind::StaticKeyword;
                    case "string"_hash:
                        return SyntaxKind::StringKeyword;
                    case "super"_hash:
                        return SyntaxKind::SuperKeyword;
                    case "switch"_hash:
                        return SyntaxKind::SwitchKeyword;
                    case "symbol"_hash:
                        return SyntaxKind::SymbolKeyword;
                    case "this"_hash:
                        return SyntaxKind::ThisKeyword;
                    case "throw"_hash:
                        return SyntaxKind::ThrowKeyword;
                    case "true"_hash:
                        return SyntaxKind::TrueKeyword;
                    case "try"_hash:
                        return SyntaxKind::TryKeyword;
                    case "type"_hash:
                        return SyntaxKind::TypeKeyword;
                    case "typeof"_hash:
                        return SyntaxKind::TypeOfKeyword;
                    case "undefined"_hash:
                        return SyntaxKind::UndefinedKeyword;
                    case "unique"_hash:
                        return SyntaxKind::UniqueKeyword;
                    case "unknown"_hash:
                        return SyntaxKind::UnknownKeyword;
                    case "var"_hash:
                        return SyntaxKind::VarKeyword;
                    case "void"_hash:
                        return SyntaxKind::VoidKeyword;
                    case "while"_hash:
                        return SyntaxKind::WhileKeyword;
                    case "with"_hash:
                        return SyntaxKind::WithKeyword;
                    case "yield"_hash:
                        return SyntaxKind::YieldKeyword;
                    case "async"_hash:
                        return SyntaxKind::AsyncKeyword;
                    case "await"_hash:
                        return SyntaxKind::AwaitKeyword;
                    case "of"_hash:
                        return SyntaxKind::OfKeyword;
                }

//                auto it = textToKeyword.find(tokenValue);
//                if (it != textToKeyword.end()) {
//                    return token = it->second;
//                }
            }
        }
        return token = SyntaxKind::Identifier;
    }

    SyntaxKind Scanner::scanIdentifier(const CharCode &startCharacter, ScriptTarget languageVersion) {
        ZoneScoped;
        auto ch = startCharacter;
        if (isIdentifierStart(ch, languageVersion)) {
            pos += ch.length;
            while (pos < end && isIdentifierPart(ch = charCodeAt(text, pos), languageVersion)) pos += ch.length;
            tokenValue = substring(text, tokenPos, pos);
            if (ch.code == CharacterCodes::backslash) {
                tokenValue += scanIdentifierParts();
            }
            return getIdentifierToken();
        }
        return SyntaxKind::Unknown;
    }

    SyntaxKind Scanner::scan() {
        ZoneScoped;
        startPos = pos;
        tokenFlags = TokenFlags::None;
        bool asteriskSeen = false;

        while (true) {
            tokenPos = pos;

            if (pos >= end) {
                return token = SyntaxKind::EndOfFileToken;
            }

            auto ch = charCodeAt(text, pos);

            // Special handling for shebang
            if (pos == 0 && ch.code == CharacterCodes::hash && isShebangTrivia(text, pos)) {
                pos = scanShebangTrivia(text, pos);
                if (skipTrivia) {
                    continue;
                } else {
                    return token = SyntaxKind::ShebangTrivia;
                }
            }

            switch (ch.code) {
                case CharacterCodes::lineFeed:
                case CharacterCodes::carriageReturn:
                    tokenFlags |= TokenFlags::PrecedingLineBreak;
                    if (skipTrivia) {
                        pos++;
                        continue;
                    } else {
                        if (ch.code == CharacterCodes::carriageReturn && pos + 1 < end &&
                            charCodeAt(text, pos + 1).code == CharacterCodes::lineFeed) {
                            // consume both CR and LF
                            pos += 2;
                        } else {
                            pos++;
                        }
                        return token = SyntaxKind::NewLineTrivia;
                    }
//            case CharacterCodes::tab:

                case CharacterCodes::tab:
                case CharacterCodes::verticalTab:
                case CharacterCodes::formFeed:
                case CharacterCodes::space:
                case CharacterCodes::nonBreakingSpace:
                case CharacterCodes::ogham:
                case CharacterCodes::enQuad:
                case CharacterCodes::emQuad:
                case CharacterCodes::enSpace:
                case CharacterCodes::emSpace:
                case CharacterCodes::threePerEmSpace:
                case CharacterCodes::fourPerEmSpace:
                case CharacterCodes::sixPerEmSpace:
                case CharacterCodes::figureSpace:
                case CharacterCodes::punctuationSpace:
                case CharacterCodes::thinSpace:
                case CharacterCodes::hairSpace:
                case CharacterCodes::zeroWidthSpace:
                case CharacterCodes::narrowNoBreakSpace:
                case CharacterCodes::mathematicalSpace:
                case CharacterCodes::ideographicSpace:
                case CharacterCodes::byteOrderMark:
                    if (skipTrivia) {
                        pos++;
                        continue;
                    } else {
                        int size;
                        while (pos < end && isWhiteSpaceSingleLine(charCodeAt(text, pos, &size))) {
                            pos += size;
                        }
                        return token = SyntaxKind::WhitespaceTrivia;
                    }
                case CharacterCodes::exclamation:
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                        if (charCodeAt(text, pos + 2).code == CharacterCodes::equals) {
                            return pos += 3, token = SyntaxKind::ExclamationEqualsEqualsToken;
                        }
                        return pos += 2, token = SyntaxKind::ExclamationEqualsToken;
                    }
                    pos++;
                    return token = SyntaxKind::ExclamationToken;
                case CharacterCodes::doubleQuote:
                case CharacterCodes::singleQuote:
                    tokenValue = scanString();
                    return token = SyntaxKind::StringLiteral;
                case CharacterCodes::backtick:
                    return token = scanTemplateAndSetTokenValue(/* isTaggedTemplate */ false);
                case CharacterCodes::percent:
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                        return pos += 2, token = SyntaxKind::PercentEqualsToken;
                    }
                    pos++;
                    return token = SyntaxKind::PercentToken;
                case CharacterCodes::ampersand:
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::ampersand) {
                        if (charCodeAt(text, pos + 2).code == CharacterCodes::equals) {
                            return pos += 3, token = SyntaxKind::AmpersandAmpersandEqualsToken;
                        }
                        return pos += 2, token = SyntaxKind::AmpersandAmpersandToken;
                    }
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                        return pos += 2, token = SyntaxKind::AmpersandEqualsToken;
                    }
                    pos++;
                    return token = SyntaxKind::AmpersandToken;
                case CharacterCodes::openParen:
                    pos++;
                    return token = SyntaxKind::OpenParenToken;
                case CharacterCodes::closeParen:
                    pos++;
                    return token = SyntaxKind::CloseParenToken;
                case CharacterCodes::asterisk:
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                        return pos += 2, token = SyntaxKind::AsteriskEqualsToken;
                    }
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::asterisk) {
                        if (charCodeAt(text, pos + 2).code == CharacterCodes::equals) {
                            return pos += 3, token = SyntaxKind::AsteriskAsteriskEqualsToken;
                        }
                        return pos += 2, token = SyntaxKind::AsteriskAsteriskToken;
                    }
                    pos++;
                    if (inJSDocType && !asteriskSeen && (tokenFlags & TokenFlags::PrecedingLineBreak)) {
                        // decoration at the start of a JSDoc comment line
                        asteriskSeen = true;
                        continue;
                    }
                    return token = SyntaxKind::AsteriskToken;
                case CharacterCodes::plus:
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::plus) {
                        return pos += 2, token = SyntaxKind::PlusPlusToken;
                    }
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                        return pos += 2, token = SyntaxKind::PlusEqualsToken;
                    }
                    pos++;
                    return token = SyntaxKind::PlusToken;
                case CharacterCodes::comma:
                    pos++;
                    return token = SyntaxKind::CommaToken;
                case CharacterCodes::minus:
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::minus) {
                        return pos += 2, token = SyntaxKind::MinusMinusToken;
                    }
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                        return pos += 2, token = SyntaxKind::MinusEqualsToken;
                    }
                    pos++;
                    return token = SyntaxKind::MinusToken;
                case CharacterCodes::dot:
                    if (isDigit(charCodeAt(text, pos + 1))) {
                        tokenValue = scanNumber().value;
                        return token = SyntaxKind::NumericLiteral;
                    }
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::dot && charCodeAt(text, pos + 2).code == CharacterCodes::dot) {
                        return pos += 3, token = SyntaxKind::DotDotDotToken;
                    }
                    pos++;
                    return token = SyntaxKind::DotToken;
                case CharacterCodes::slash:
                    // Single-line comment
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::slash) {
                        pos += 2;

                        while (pos < end) {
                            if (isLineBreak(charCodeAt(text, pos))) {
                                break;
                            }
                            pos++;
                        }

                        commentDirectives = appendIfCommentDirective(
                                commentDirectives,
                                substring(text, tokenPos, pos),
                                commentDirectiveRegExSingleLine,
                                tokenPos
                        );

                        if (skipTrivia) {
                            continue;
                        } else {
                            return token = SyntaxKind::SingleLineCommentTrivia;
                        }
                    }
                    // Multi-line comment
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::asterisk) {
                        pos += 2;
                        if (charCodeAt(text, pos).code == CharacterCodes::asterisk && charCodeAt(text, pos + 1).code != CharacterCodes::slash) {
                            tokenFlags |= TokenFlags::PrecedingJSDocComment;
                        }

                        auto commentClosed = false;
                        auto lastLineStart = tokenPos;
                        while (pos < end) {
                            auto ch = charCodeAt(text, pos);

                            if (ch.code == CharacterCodes::asterisk && charCodeAt(text, pos + 1).code == CharacterCodes::slash) {
                                pos += 2;
                                commentClosed = true;
                                break;
                            }

                            pos++;

                            if (isLineBreak(ch)) {
                                lastLineStart = pos;
                                tokenFlags |= TokenFlags::PrecedingLineBreak;
                            }
                        }

                        commentDirectives = appendIfCommentDirective(commentDirectives, substring(text, lastLineStart, pos),
                                                                     commentDirectiveRegExMultiLine, lastLineStart);

                        if (!commentClosed) {
//                        error(Diagnostics::Asterisk_Slash_expected());
                        }

                        if (skipTrivia) {
                            continue;
                        } else {
                            if (!commentClosed) {
                                tokenFlags |= TokenFlags::Unterminated;
                            }
                            return token = SyntaxKind::MultiLineCommentTrivia;
                        }
                    }

                    if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                        return pos += 2, token = SyntaxKind::SlashEqualsToken;
                    }

                    pos++;
                    return token = SyntaxKind::SlashToken;

                case CharacterCodes::_0:
                    if (pos + 2 < end && (charCodeAt(text, pos + 1).code ==
                                          CharacterCodes::X || charCodeAt(text, pos + 1).code == CharacterCodes::x)) {
                        pos += 2;
                        tokenValue = scanMinimumNumberOfHexDigits(1, /*canHaveSeparators*/ true);
                        if (tokenValue.empty()) {
//                        error(Diagnostics::Hexadecimal_digit_expected());
                            tokenValue = "0";
                        }
                        tokenValue = "0x" + tokenValue;
                        tokenFlags |= TokenFlags::HexSpecifier;
                        return token = checkBigIntSuffix();
                    } else if (pos + 2 < end && (charCodeAt(text, pos + 1).code ==
                                                 CharacterCodes::B || charCodeAt(text, pos + 1).code == CharacterCodes::b)) {
                        pos += 2;
                        tokenValue = scanBinaryOrOctalDigits(/* base */ 2);
                        if (tokenValue.empty()) {
//                        error(Diagnostics::Binary_digit_expected());
                            tokenValue = "0";
                        }
                        tokenValue = "0b" + tokenValue;
                        tokenFlags |= TokenFlags::BinarySpecifier;
                        return token = checkBigIntSuffix();
                    } else if (pos + 2 < end && (charCodeAt(text, pos + 1).code == CharacterCodes::O || charCodeAt(text, pos + 1).code == CharacterCodes::o)) {
                        pos += 2;
                        tokenValue = scanBinaryOrOctalDigits(/* base */ 8);
                        if (tokenValue.empty()) {
//                        error(Diagnostics::Octal_digit_expected());
                            tokenValue = "0";
                        }
                        tokenValue = "0o" + tokenValue;
                        tokenFlags |= TokenFlags::OctalSpecifier;
                        return token = checkBigIntSuffix();
                    }
                    // Try to parse as an octal
                    if (pos + 1 < end && isOctalDigit(charCodeAt(text, pos + 1))) {
                        tokenValue = to_string(scanOctalDigits());
                        tokenFlags |= TokenFlags::Octal;
                        return token = SyntaxKind::NumericLiteral;
                    }
                    // This fall-through is a deviation from the EcmaScript grammar. The grammar says that a leading zero
                    // can only be followed by an octal digit, a dot, or the end of the number literal. However, we are being
                    // permissive and allowing decimal digits of the form 08* and 09* (which many browsers also do).
                    // falls through
                case CharacterCodes::_1:
                case CharacterCodes::_2:
                case CharacterCodes::_3:
                case CharacterCodes::_4:
                case CharacterCodes::_5:
                case CharacterCodes::_6:
                case CharacterCodes::_7:
                case CharacterCodes::_8:
                case CharacterCodes::_9: {
                    auto n = scanNumber();
                    token = n.type;
                    tokenValue = n.value;
                    return token;
                }
                case CharacterCodes::colon:
                    pos++;
                    return token = SyntaxKind::ColonToken;
                case CharacterCodes::semicolon:
                    pos++;
                    return token = SyntaxKind::SemicolonToken;
                case CharacterCodes::lessThan:
                    if (isConflictMarkerTrivia(text, pos)) {
                        pos = scanConflictMarkerTrivia(text, pos);
                        if (skipTrivia) {
                            continue;
                        } else {
                            return token = SyntaxKind::ConflictMarkerTrivia;
                        }
                    }

                    if (charCodeAt(text, pos + 1).code == CharacterCodes::lessThan) {
                        if (charCodeAt(text, pos + 2).code == CharacterCodes::equals) {
                            return pos += 3, token = SyntaxKind::LessThanLessThanEqualsToken;
                        }
                        return pos += 2, token = SyntaxKind::LessThanLessThanToken;
                    }
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                        return pos += 2, token = SyntaxKind::LessThanEqualsToken;
                    }
                    if (languageVariant == LanguageVariant::JSX &&
                        charCodeAt(text, pos + 1).code == CharacterCodes::slash &&
                        charCodeAt(text, pos + 2).code != CharacterCodes::asterisk) {
                        return pos += 2, token = SyntaxKind::LessThanSlashToken;
                    }
                    pos++;
                    return token = SyntaxKind::LessThanToken;
                case CharacterCodes::equals:
                    if (isConflictMarkerTrivia(text, pos)) {
                        pos = scanConflictMarkerTrivia(text, pos);
                        if (skipTrivia) {
                            continue;
                        } else {
                            return token = SyntaxKind::ConflictMarkerTrivia;
                        }
                    }

                    if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                        if (charCodeAt(text, pos + 2).code == CharacterCodes::equals) {
                            return pos += 3, token = SyntaxKind::EqualsEqualsEqualsToken;
                        }
                        return pos += 2, token = SyntaxKind::EqualsEqualsToken;
                    }
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::greaterThan) {
                        return pos += 2, token = SyntaxKind::EqualsGreaterThanToken;
                    }
                    pos++;
                    return token = SyntaxKind::EqualsToken;
                case CharacterCodes::greaterThan:
                    if (isConflictMarkerTrivia(text, pos)) {
                        pos = scanConflictMarkerTrivia(text, pos);
                        if (skipTrivia) {
                            continue;
                        } else {
                            return token = SyntaxKind::ConflictMarkerTrivia;
                        }
                    }

                    pos++;
                    return token = SyntaxKind::GreaterThanToken;
                case CharacterCodes::question:
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::dot && !isDigit(charCodeAt(text, pos + 2))) {
                        return pos += 2, token = SyntaxKind::QuestionDotToken;
                    }
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::question) {
                        if (charCodeAt(text, pos + 2).code == CharacterCodes::equals) {
                            return pos += 3, token = SyntaxKind::QuestionQuestionEqualsToken;
                        }
                        return pos += 2, token = SyntaxKind::QuestionQuestionToken;
                    }
                    pos++;
                    return token = SyntaxKind::QuestionToken;
                case CharacterCodes::openBracket:
                    pos++;
                    return token = SyntaxKind::OpenBracketToken;
                case CharacterCodes::closeBracket:
                    pos++;
                    return token = SyntaxKind::CloseBracketToken;
                case CharacterCodes::caret:
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                        return pos += 2, token = SyntaxKind::CaretEqualsToken;
                    }
                    pos++;
                    return token = SyntaxKind::CaretToken;
                case CharacterCodes::openBrace:
                    pos++;
                    return token = SyntaxKind::OpenBraceToken;
                case CharacterCodes::bar:
                    if (isConflictMarkerTrivia(text, pos)) {
                        pos = scanConflictMarkerTrivia(text, pos);
                        if (skipTrivia) {
                            continue;
                        } else {
                            return token = SyntaxKind::ConflictMarkerTrivia;
                        }
                    }

                    if (charCodeAt(text, pos + 1).code == CharacterCodes::bar) {
                        if (charCodeAt(text, pos + 2).code == CharacterCodes::equals) {
                            return pos += 3, token = SyntaxKind::BarBarEqualsToken;
                        }
                        return pos += 2, token = SyntaxKind::BarBarToken;
                    }
                    if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                        return pos += 2, token = SyntaxKind::BarEqualsToken;
                    }
                    pos++;
                    return token = SyntaxKind::BarToken;
                case CharacterCodes::closeBrace:
                    pos++;
                    return token = SyntaxKind::CloseBraceToken;
                case CharacterCodes::tilde:
                    pos++;
                    return token = SyntaxKind::TildeToken;
                case CharacterCodes::at:
                    pos++;
                    return token = SyntaxKind::AtToken;
                case CharacterCodes::backslash: {
                    auto extendedCookedChar = peekExtendedUnicodeEscape();
                    if (extendedCookedChar.code >= 0 && isIdentifierStart(extendedCookedChar, languageVersion)) {
                        pos += 3;
                        tokenFlags |= TokenFlags::ExtendedUnicodeEscape;
                        tokenValue = scanExtendedUnicodeEscape() + scanIdentifierParts();
                        return token = getIdentifierToken();
                    }

                    auto cookedChar = peekUnicodeEscape();
                    if (cookedChar.code >= 0 && isIdentifierStart(cookedChar, languageVersion)) {
                        pos += 6;
                        tokenFlags |= TokenFlags::UnicodeEscape;
                        tokenValue = fromCharCode(cookedChar.code) + scanIdentifierParts();
                        return token = getIdentifierToken();
                    }

                    error(Diagnostics::Invalid_character());
                    pos++;
                    return token = SyntaxKind::Unknown;
                }
                case CharacterCodes::hash:
                    if (pos != 0 && text[pos + 1] == '!') {
                        error(Diagnostics::can_only_be_used_at_the_start_of_a_file());
                        pos++;
                        return token = SyntaxKind::Unknown;
                    }

                    if (isIdentifierStart(charCodeAt(text, pos + 1), languageVersion)) {
                        pos++;
                        scanIdentifier(charCodeAt(text, pos), languageVersion);
                    } else {
                        tokenValue = fromCharCode(charCodeAt(text, pos).code);
                        error(Diagnostics::Invalid_character(), pos++, ch.length);
                    }
                    return token = SyntaxKind::PrivateIdentifier;
                default: {
                    auto identifierKind = scanIdentifier(ch, languageVersion);
                    if (identifierKind != SyntaxKind::Unknown) {
                        return token = identifierKind;
                    } else if (isWhiteSpaceSingleLine(ch)) {
                        pos += ch.length;
                        continue;
                    } else if (isLineBreak(ch)) {
                        tokenFlags |= TokenFlags::PrecedingLineBreak;
                        pos += ch.length;
                        continue;
                    }
                    auto size = ch.length;
                    error(Diagnostics::Invalid_character(), pos, size);
                    pos += size;
                    return token = SyntaxKind::Unknown;
                }
            }
        }

        return SyntaxKind::EndOfFileToken;
    }

    bool Scanner::isOctalDigit(const CharCode &ch) {
        return ch.code >= CharacterCodes::_0 && ch.code <= CharacterCodes::_7;
    }

    int Scanner::scanOctalDigits() {
        auto start = pos;
        while (isOctalDigit(charCodeAt(text, pos))) {
            pos++;
        }
        return stoi(substring(text, start, pos));
    }

    SyntaxKind Scanner::reScanJsxToken(bool allowMultilineJsxText) {
        pos = tokenPos = startPos;
        return token = scanJsxToken(allowMultilineJsxText);
    }

    SyntaxKind Scanner::reScanGreaterToken() {
        ZoneScoped;
        if (token == SyntaxKind::GreaterThanToken) {
            if (charCodeAt(text, pos).code == CharacterCodes::greaterThan) {
                if (charCodeAt(text, pos + 1).code == CharacterCodes::greaterThan) {
                    if (charCodeAt(text, pos + 2).code == CharacterCodes::equals) {
                        return pos += 3, token = SyntaxKind::GreaterThanGreaterThanGreaterThanEqualsToken;
                    }
                    return pos += 2, token = SyntaxKind::GreaterThanGreaterThanGreaterThanToken;
                }
                if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                    return pos += 2, token = SyntaxKind::GreaterThanGreaterThanEqualsToken;
                }
                pos++;
                return token = SyntaxKind::GreaterThanGreaterThanToken;
            }
            if (charCodeAt(text, pos).code == CharacterCodes::equals) {
                pos++;
                return token = SyntaxKind::GreaterThanEqualsToken;
            }
        }
        return token;
    }

    SyntaxKind Scanner::reScanSlashToken() {
        ZoneScoped;
        if (token == SyntaxKind::SlashToken || token == SyntaxKind::SlashEqualsToken) {
            auto p = tokenPos + 1;
            auto inEscape = false;
            auto inCharacterClass = false;
            while (true) {
                // If we reach the end of a file, or hit a newline, then this is an unterminated
                // regex.  Report error and return what we have so far.
                if (p >= end) {
                    tokenFlags |= TokenFlags::Unterminated;
                    error(Diagnostics::Unterminated_regular_expression_literal());
                    break;
                }

                auto ch = charCodeAt(text, p);
                if (isLineBreak(ch)) {
                    tokenFlags |= TokenFlags::Unterminated;
                    error(Diagnostics::Unterminated_regular_expression_literal());
                    break;
                }

                if (inEscape) {
                    // Parsing an escape character;
                    // reset the flag and just advance to the next char.
                    inEscape = false;
                } else if (ch.code == CharacterCodes::slash && !inCharacterClass) {
                    // A slash within a character class is permissible,
                    // but in general it signals the end of the regexp literal.
                    p++;
                    break;
                } else if (ch.code == CharacterCodes::openBracket) {
                    inCharacterClass = true;
                } else if (ch.code == CharacterCodes::backslash) {
                    inEscape = true;
                } else if (ch.code == CharacterCodes::closeBracket) {
                    inCharacterClass = false;
                }
                p++;
            }

            while (p < end && isIdentifierPart(charCodeAt(text, p), languageVersion)) {
                p++;
            }
            pos = p;
            tokenValue = substring(text, tokenPos, pos);
            token = SyntaxKind::RegularExpressionLiteral;
        }
        return token;
    }

    SyntaxKind Scanner::reScanInvalidIdentifier() {
        ZoneScoped;
//            Debug.assert(token == SyntaxKind::Unknown, "'reScanInvalidIdentifier' should only be called when the current token is 'SyntaxKind::Unknown'.");
        pos = tokenPos = startPos;
        tokenFlags = 0;
        auto ch = charCodeAt(text, pos);
        auto identifierKind = scanIdentifier(ch, ScriptTarget::ESNext);
        if (identifierKind != SyntaxKind::Unknown) {
            return token = identifierKind;
        }
        pos += ch.length;
        return token; // Still `SyntaKind.Unknown`
    };
}