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

bool isShebangTrivia(string &text, int pos) {
    // Shebangs check must only be done at the start of the file
    //    Debug.assert(pos == 0);
    //    return shebangTriviaRegex.test(text);
    //todo: implement
    return false;
}

bool isLineBreak(CharCode ch) {
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


bool isWhiteSpaceSingleLine(CharCode ch) {
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
    auto quote = charCodeAt(text, pos);
    pos ++;
    string result = "";
    auto start = pos;
    while (true) {
        if (pos >= end) {
            result += substr(text, start, pos);
            tokenFlags |= TokenFlags::Unterminated;
//            error(Diagnostics::Unterminated_string_literal);
            break;
        }
        auto ch = charCodeAt(text, pos);
        if (ch.code == quote.code) {
            result += substr(text, start, pos);
            pos ++;
            break;
        }
        if (ch.code == CharacterCodes::backslash && ! jsxAttributeString) {
            result += substr(text, start, pos);
            result += scanEscapeSequence();
            start = pos;
            continue;
        }
        if (isLineBreak(ch) && ! jsxAttributeString) {
            result += substr(text, start, pos);
            tokenFlags |= TokenFlags::Unterminated;
//            error(Diagnostics::Unterminated_string_literal);
            break;
        }
        pos ++;
    }
    return result;
}

bool isDigit(CharCode ch) {
    return ch.code >= CharacterCodes::_0 && ch.code <= CharacterCodes::_9;
}

bool isHexDigit(CharCode ch) {
    return isDigit(ch) || (ch.code >= CharacterCodes::A && ch.code <= CharacterCodes::F) ||
           (ch.code >= CharacterCodes::a && ch.code <= CharacterCodes::f);
}

string Scanner::scanMinimumNumberOfHexDigits(int count, bool canHaveSeparators) {
    return scanHexDigits(/*minCount*/ count, /*scanAsManyAsPossible*/ true, canHaveSeparators);
}

string Scanner::scanHexDigits(int minCount, bool scanAsManyAsPossible, bool canHaveSeparators) {
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
//                error(Diagnostics::Multiple_consecutive_numeric_separators_are_not_permitted, pos, 1);
            } else {
//                error(Diagnostics::Numeric_separators_are_not_allowed_here, pos, 1);
            }
            pos ++;
            continue;
        }
        allowSeparator = canHaveSeparators;
        if (ch.code >= CharacterCodes::A && ch.code <= CharacterCodes::F) {
            ch.code += CharacterCodes::a - CharacterCodes::A; // standardize hex literals to lowercase
        } else if (! ((ch.code >= CharacterCodes::_0 && ch.code <= CharacterCodes::_9) ||
                      (ch.code >= CharacterCodes::a && ch.code <= CharacterCodes::f)
        )) {
            break;
        }
        found ++;
        result.append(substr(text, pos, ch.length));
        pos ++;
        isPreviousTokenSeparator = false;
    }

//    if (charCodeAt(text, pos - 1) == CharacterCodes::_) {
//        error(Diagnostics::Numeric_separators_are_not_allowed_here, pos - 1, 1);
//    }

    return result;
}

bool isCodePoint(CharCode ch) {
    return ch.code <= 0x10FFFF;
}

int Scanner::scanExactNumberOfHexDigits(int count, bool canHaveSeparators) {
    auto valueString = scanHexDigits(/*minCount*/ count, /*scanAsManyAsPossible*/ false, canHaveSeparators);
    return ! valueString.empty() ? stoi(valueString, nullptr, 16) : - 1;
}

string Scanner::scanHexadecimalEscape(int numDigits) {
    auto escapedValue = scanExactNumberOfHexDigits(numDigits, /*canHaveSeparators*/ false);

    if (escapedValue >= 0) {
        return fromCharCode(escapedValue);
    } else {
//        error(Diagnostics::Hexadecimal_digit_expected);
        return "";
    }
}

string Scanner::scanExtendedUnicodeEscape() {
    auto escapedValueString = scanMinimumNumberOfHexDigits(1, /*canHaveSeparators*/ false);
    auto escapedValue = ! escapedValueString.empty() ? stoi(escapedValueString, nullptr, 16) : - 1;
    auto isInvalidExtendedEscape = false;

    // Validate the value of the digit
    if (escapedValue < 0) {
//        error(Diagnostics::Hexadecimal_digit_expected);
        isInvalidExtendedEscape = true;
    } else if (escapedValue > 0x10FFFF) {
//        error(Diagnostics::An_extended_Unicode_escape_value_must_be_between_0x0_and_0x10FFFF_inclusive);
        isInvalidExtendedEscape = true;
    }

    if (pos >= end) {
//        error(Diagnostics::Unexpected_end_of_text);
        isInvalidExtendedEscape = true;
    } else if (charCodeAt(text, pos).code == CharacterCodes::closeBrace) {
        // Only swallow the following character up if it's a '}'.
        pos ++;
    } else {
//        error(Diagnostics::Unterminated_Unicode_escape_sequence);
        isInvalidExtendedEscape = true;
    }

    if (isInvalidExtendedEscape) {
        return "";
    }

    return fromCharCode(escapedValue);
//    return utf16EncodeAsString(escapedValue);
}

string Scanner::scanEscapeSequence(bool isTaggedTemplate) {
    auto start = pos;
    pos ++;
    if (pos >= end) {
//        error(Diagnostics::Unexpected_end_of_text);
        return "";
    }
    auto ch = charCodeAt(text, pos);
    pos ++;
    switch (ch.code) {
        case CharacterCodes::_0:
            // '\01'
            if (isTaggedTemplate && pos < end && isDigit(charCodeAt(text, pos))) {
                pos ++;
                tokenFlags |= TokenFlags::ContainsInvalidEscape;
                return substr(text, start, pos);
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
                for (auto escapePos = pos; escapePos < pos + 4; escapePos ++) {
                    if (escapePos < end && ! isHexDigit(charCodeAt(text, escapePos)) &&
                        charCodeAt(text, escapePos).code != CharacterCodes::openBrace) {
                        pos = escapePos;
                        tokenFlags |= TokenFlags::ContainsInvalidEscape;
                        return substr(text, start, pos);
                    }
                }
            }
            // '\u{DDDDDDDD}'
            if (pos < end && charCodeAt(text, pos).code == CharacterCodes::openBrace) {
                pos ++;

                // '\u{'
                if (isTaggedTemplate && ! isHexDigit(charCodeAt(text, pos))) {
                    tokenFlags |= TokenFlags::ContainsInvalidEscape;
                    return substr(text, start, pos);
                }

                if (isTaggedTemplate) {
                    auto savePos = pos;
                    auto escapedValueString = scanMinimumNumberOfHexDigits(1, /*canHaveSeparators*/ false);

                    try {
                        auto escapedValue = ! escapedValueString.empty() ? stoi(escapedValueString, nullptr, 16) : - 1;

                        // '\u{Not Code Point' or '\u{CodePoint'
                        if (charCodeAt(text, pos).code != CharacterCodes::closeBrace) {
                            tokenFlags |= TokenFlags::ContainsInvalidEscape;
                            return substr(text, start, pos);
                        } else {
                            pos = savePos;
                        }
                    } catch (invalid_argument &error) {
                        tokenFlags |= TokenFlags::ContainsInvalidEscape;
                        return substr(text, start, pos);
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
                if (! isHexDigit(charCodeAt(text, pos))) {
                    tokenFlags |= TokenFlags::ContainsInvalidEscape;
                    return substr(text, start, pos);
                } else if (! isHexDigit(charCodeAt(text, pos + 1))) {
                    pos ++;
                    tokenFlags |= TokenFlags::ContainsInvalidEscape;
                    return substr(text, start, pos);
                }
            }
            // '\xDD'
            return scanHexadecimalEscape(/*numDigits*/ 2);

            // when encountering a LineContinuation (i.e. a backslash and a line terminator sequence),
            // the line terminator is interpreted to be "the empty code unit sequence".
        case CharacterCodes::carriageReturn:
            if (pos < end && charCodeAt(text, pos).code == CharacterCodes::lineFeed) {
                pos ++;
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
    auto startedWithBacktick = charCodeAt(text, pos).code == CharacterCodes::backtick;

    pos ++;
    auto start = pos;
    string contents;
    SyntaxKind resultingToken;

    while (true) {
        if (pos >= end) {
            contents += substr(text, start, pos);
            tokenFlags |= TokenFlags::Unterminated;
//            error(Diagnostics::Unterminated_template_literal);
            resultingToken = startedWithBacktick ? SyntaxKind::NoSubstitutionTemplateLiteral : SyntaxKind::TemplateTail;
            break;
        }

        auto currChar = charCodeAt(text, pos);

        // '`'
        if (currChar.code == CharacterCodes::backtick) {
            contents += substr(text, start, pos);
            pos ++;
            resultingToken = startedWithBacktick ? SyntaxKind::NoSubstitutionTemplateLiteral : SyntaxKind::TemplateTail;
            break;
        }

        // '${'
        if (currChar.code == CharacterCodes::$ && pos + 1 < end && charCodeAt(text, pos + 1).code == CharacterCodes::openBrace) {
            contents += substr(text, start, pos);
            pos += 2;
            resultingToken = startedWithBacktick ? SyntaxKind::TemplateHead : SyntaxKind::TemplateMiddle;
            break;
        }

        // Escape character
        if (currChar.code == CharacterCodes::backslash) {
            contents += substr(text, start, pos);
            contents += scanEscapeSequence(isTaggedTemplate);
            start = pos;
            continue;
        }

        // Speculated ECMAScript 6 Spec 11.8.6.1:
        // <CR><LF> and <CR> LineTerminatorSequences are normalized to <LF> for Template Values
        if (currChar.code == CharacterCodes::carriageReturn) {
            contents += substr(text, start, pos);
            pos ++;

            if (pos < end && charCodeAt(text, pos).code == CharacterCodes::lineFeed) {
                pos ++;
            }

            contents += "\n";
            start = pos;
            continue;
        }

        pos ++;
    }

//    Debug.assert(resultingToken !== undefined);

    tokenValue = contents;
    return resultingToken;
}

string Scanner::scanNumberFragment() {
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
                result += substr(text, start, pos);
            } else if (isPreviousTokenSeparator) {
//                error(Diagnostics::Multiple_consecutive_numeric_separators_are_not_permitted, pos, 1);
            } else {
//                error(Diagnostics::Numeric_separators_are_not_allowed_here, pos, 1);
            }
            pos ++;
            start = pos;
            continue;
        }
        if (isDigit(ch)) {
            allowSeparator = true;
            isPreviousTokenSeparator = false;
            pos ++;
            continue;
        }
        break;
    }
    if (charCodeAt(text, pos - 1).code == CharacterCodes::_) {
//        error(Diagnostics::Numeric_separators_are_not_allowed_here, pos - 1, 1);
    }
    return result + substr(text, start, pos);
}

map<string, SyntaxKind> textToKeyword{
        {"abstract",    SyntaxKind::AbstractKeyword},
        {"any",         SyntaxKind::AnyKeyword},
        {"as",          SyntaxKind::AsKeyword},
        {"asserts",     SyntaxKind::AssertsKeyword},
        {"assert",      SyntaxKind::AssertKeyword},
        {"bigint",      SyntaxKind::BigIntKeyword},
        {"boolean",     SyntaxKind::BooleanKeyword},
        {"break",       SyntaxKind::BreakKeyword},
        {"case",        SyntaxKind::CaseKeyword},
        {"catch",       SyntaxKind::CatchKeyword},
        {"class",       SyntaxKind::ClassKeyword},
        {"continue",    SyntaxKind::ContinueKeyword},
        {"const",       SyntaxKind::ConstKeyword},
        {"constructor", SyntaxKind::ConstructorKeyword},
        {"debugger",    SyntaxKind::DebuggerKeyword},
        {"declare",     SyntaxKind::DeclareKeyword},
        {"default",     SyntaxKind::DefaultKeyword},
        {"delete",      SyntaxKind::DeleteKeyword},
        {"do",          SyntaxKind::DoKeyword},
        {"else",        SyntaxKind::ElseKeyword},
        {"enum",        SyntaxKind::EnumKeyword},
        {"export",      SyntaxKind::ExportKeyword},
        {"extends",     SyntaxKind::ExtendsKeyword},
        {"false",       SyntaxKind::FalseKeyword},
        {"finally",     SyntaxKind::FinallyKeyword},
        {"for",         SyntaxKind::ForKeyword},
        {"from",        SyntaxKind::FromKeyword},
        {"function",    SyntaxKind::FunctionKeyword},
        {"get",         SyntaxKind::GetKeyword},
        {"if",          SyntaxKind::IfKeyword},
        {"implements",  SyntaxKind::ImplementsKeyword},
        {"import",      SyntaxKind::ImportKeyword},
        {"in",          SyntaxKind::InKeyword},
        {"infer",       SyntaxKind::InferKeyword},
        {"instanceof",  SyntaxKind::InstanceOfKeyword},
        {"interface",   SyntaxKind::InterfaceKeyword},
        {"intrinsic",   SyntaxKind::IntrinsicKeyword},
        {"is",          SyntaxKind::IsKeyword},
        {"keyof",       SyntaxKind::KeyOfKeyword},
        {"let",         SyntaxKind::LetKeyword},
        {"module",      SyntaxKind::ModuleKeyword},
        {"namespace",   SyntaxKind::NamespaceKeyword},
        {"never",       SyntaxKind::NeverKeyword},
        {"new",         SyntaxKind::NewKeyword},
        {"null",        SyntaxKind::NullKeyword},
        {"number",      SyntaxKind::NumberKeyword},
        {"object",      SyntaxKind::ObjectKeyword},
        {"package",     SyntaxKind::PackageKeyword},
        {"private",     SyntaxKind::PrivateKeyword},
        {"protected",   SyntaxKind::ProtectedKeyword},
        {"public",      SyntaxKind::PublicKeyword},
        {"override",    SyntaxKind::OverrideKeyword},
        {"out",         SyntaxKind::OutKeyword},
        {"readonly",    SyntaxKind::ReadonlyKeyword},
        {"require",     SyntaxKind::RequireKeyword},
        {"global",      SyntaxKind::GlobalKeyword},
        {"return",      SyntaxKind::ReturnKeyword},
        {"set",         SyntaxKind::SetKeyword},
        {"static",      SyntaxKind::StaticKeyword},
        {"string",      SyntaxKind::StringKeyword},
        {"super",       SyntaxKind::SuperKeyword},
        {"switch",      SyntaxKind::SwitchKeyword},
        {"symbol",      SyntaxKind::SymbolKeyword},
        {"this",        SyntaxKind::ThisKeyword},
        {"throw",       SyntaxKind::ThrowKeyword},
        {"true",        SyntaxKind::TrueKeyword},
        {"try",         SyntaxKind::TryKeyword},
        {"type",        SyntaxKind::TypeKeyword},
        {"typeof",      SyntaxKind::TypeOfKeyword},
        {"undefined",   SyntaxKind::UndefinedKeyword},
        {"unique",      SyntaxKind::UniqueKeyword},
        {"unknown",     SyntaxKind::UnknownKeyword},
        {"var",         SyntaxKind::VarKeyword},
        {"void",        SyntaxKind::VoidKeyword},
        {"while",       SyntaxKind::WhileKeyword},
        {"with",        SyntaxKind::WithKeyword},
        {"yield",       SyntaxKind::YieldKeyword},
        {"async",       SyntaxKind::AsyncKeyword},
        {"await",       SyntaxKind::AwaitKeyword},
        {"of",          SyntaxKind::OfKeyword},
};

/*
    As per ECMAScript Language Specification 3th Edition, Section 7.6: Identifiers
    IdentifierStart ::
        Can contain Unicode 3.0.0 categories:
        Uppercase letter (Lu),
        Lowercase letter (Ll),
        Titlecase letter (Lt),
        Modifier letter (Lm),
        Other letter (Lo), or
        Letter number (Nl).
    IdentifierPart :: =
        Can contain IdentifierStart + Unicode 3.0.0 categories:
        Non-spacing mark (Mn),
        Combining spacing mark (Mc),
        Decimal number (Nd), or
        Connector punctuation (Pc).

    Codepoint ranges for ES3 Identifiers are extracted from the Unicode 3.0.0 specification at:
    http://www.unicode.org/Public/3.0-Update/UnicodeData-3.0.0.txt
*/
vector<int> unicodeES3IdentifierStart = {170, 170, 181, 181, 186, 186, 192, 214, 216, 246, 248, 543, 546, 563, 592, 685, 688, 696, 699, 705, 720, 721, 736, 740, 750, 750, 890,
                                         890, 902, 902, 904, 906, 908, 908, 910, 929, 931, 974, 976, 983, 986, 1011, 1024, 1153, 1164, 1220, 1223, 1224, 1227, 1228, 1232,
                                         1269, 1272, 1273, 1329, 1366, 1369, 1369, 1377, 1415, 1488, 1514, 1520, 1522, 1569, 1594, 1600, 1610, 1649, 1747, 1749, 1749, 1765,
                                         1766, 1786, 1788, 1808, 1808, 1810, 1836, 1920, 1957, 2309, 2361, 2365, 2365, 2384, 2384, 2392, 2401, 2437, 2444, 2447, 2448, 2451,
                                         2472, 2474, 2480, 2482, 2482, 2486, 2489, 2524, 2525, 2527, 2529, 2544, 2545, 2565, 2570, 2575, 2576, 2579, 2600, 2602, 2608, 2610,
                                         2611, 2613, 2614, 2616, 2617, 2649, 2652, 2654, 2654, 2674, 2676, 2693, 2699, 2701, 2701, 2703, 2705, 2707, 2728, 2730, 2736, 2738,
                                         2739, 2741, 2745, 2749, 2749, 2768, 2768, 2784, 2784, 2821, 2828, 2831, 2832, 2835, 2856, 2858, 2864, 2866, 2867, 2870, 2873, 2877,
                                         2877, 2908, 2909, 2911, 2913, 2949, 2954, 2958, 2960, 2962, 2965, 2969, 2970, 2972, 2972, 2974, 2975, 2979, 2980, 2984, 2986, 2990,
                                         2997, 2999, 3001, 3077, 3084, 3086, 3088, 3090, 3112, 3114, 3123, 3125, 3129, 3168, 3169, 3205, 3212, 3214, 3216, 3218, 3240, 3242,
                                         3251, 3253, 3257, 3294, 3294, 3296, 3297, 3333, 3340, 3342, 3344, 3346, 3368, 3370, 3385, 3424, 3425, 3461, 3478, 3482, 3505, 3507,
                                         3515, 3517, 3517, 3520, 3526, 3585, 3632, 3634, 3635, 3648, 3654, 3713, 3714, 3716, 3716, 3719, 3720, 3722, 3722, 3725, 3725, 3732,
                                         3735, 3737, 3743, 3745, 3747, 3749, 3749, 3751, 3751, 3754, 3755, 3757, 3760, 3762, 3763, 3773, 3773, 3776, 3780, 3782, 3782, 3804,
                                         3805, 3840, 3840, 3904, 3911, 3913, 3946, 3976, 3979, 4096, 4129, 4131, 4135, 4137, 4138, 4176, 4181, 4256, 4293, 4304, 4342, 4352,
                                         4441, 4447, 4514, 4520, 4601, 4608, 4614, 4616, 4678, 4680, 4680, 4682, 4685, 4688, 4694, 4696, 4696, 4698, 4701, 4704, 4742, 4744,
                                         4744, 4746, 4749, 4752, 4782, 4784, 4784, 4786, 4789, 4792, 4798, 4800, 4800, 4802, 4805, 4808, 4814, 4816, 4822, 4824, 4846, 4848,
                                         4878, 4880, 4880, 4882, 4885, 4888, 4894, 4896, 4934, 4936, 4954, 5024, 5108, 5121, 5740, 5743, 5750, 5761, 5786, 5792, 5866, 6016,
                                         6067, 6176, 6263, 6272, 6312, 7680, 7835, 7840, 7929, 7936, 7957, 7960, 7965, 7968, 8005, 8008, 8013, 8016, 8023, 8025, 8025, 8027,
                                         8027, 8029, 8029, 8031, 8061, 8064, 8116, 8118, 8124, 8126, 8126, 8130, 8132, 8134, 8140, 8144, 8147, 8150, 8155, 8160, 8172, 8178,
                                         8180, 8182, 8188, 8319, 8319, 8450, 8450, 8455, 8455, 8458, 8467, 8469, 8469, 8473, 8477, 8484, 8484, 8486, 8486, 8488, 8488, 8490,
                                         8493, 8495, 8497, 8499, 8505, 8544, 8579, 12293, 12295, 12321, 12329, 12337, 12341, 12344, 12346, 12353, 12436, 12445, 12446, 12449,
                                         12538, 12540, 12542, 12549, 12588, 12593, 12686, 12704, 12727, 13312, 19893, 19968, 40869, 40960, 42124, 44032, 55203, 63744, 64045,
                                         64256, 64262, 64275, 64279, 64285, 64285, 64287, 64296, 64298, 64310, 64312, 64316, 64318, 64318, 64320, 64321, 64323, 64324, 64326,
                                         64433, 64467, 64829, 64848, 64911, 64914, 64967, 65008, 65019, 65136, 65138, 65140, 65140, 65142, 65276, 65313, 65338, 65345, 65370,
                                         65382, 65470, 65474, 65479, 65482, 65487, 65490, 65495, 65498, 65500,};
vector<int> unicodeES3IdentifierPart = {170, 170, 181, 181, 186, 186, 192, 214, 216, 246, 248, 543, 546, 563, 592, 685, 688, 696, 699, 705, 720, 721, 736, 740, 750, 750, 768,
                                        846, 864, 866, 890, 890, 902, 902, 904, 906, 908, 908, 910, 929, 931, 974, 976, 983, 986, 1011, 1024, 1153, 1155, 1158, 1164, 1220,
                                        1223, 1224, 1227, 1228, 1232, 1269, 1272, 1273, 1329, 1366, 1369, 1369, 1377, 1415, 1425, 1441, 1443, 1465, 1467, 1469, 1471, 1471,
                                        1473, 1474, 1476, 1476, 1488, 1514, 1520, 1522, 1569, 1594, 1600, 1621, 1632, 1641, 1648, 1747, 1749, 1756, 1759, 1768, 1770, 1773,
                                        1776, 1788, 1808, 1836, 1840, 1866, 1920, 1968, 2305, 2307, 2309, 2361, 2364, 2381, 2384, 2388, 2392, 2403, 2406, 2415, 2433, 2435,
                                        2437, 2444, 2447, 2448, 2451, 2472, 2474, 2480, 2482, 2482, 2486, 2489, 2492, 2492, 2494, 2500, 2503, 2504, 2507, 2509, 2519, 2519,
                                        2524, 2525, 2527, 2531, 2534, 2545, 2562, 2562, 2565, 2570, 2575, 2576, 2579, 2600, 2602, 2608, 2610, 2611, 2613, 2614, 2616, 2617,
                                        2620, 2620, 2622, 2626, 2631, 2632, 2635, 2637, 2649, 2652, 2654, 2654, 2662, 2676, 2689, 2691, 2693, 2699, 2701, 2701, 2703, 2705,
                                        2707, 2728, 2730, 2736, 2738, 2739, 2741, 2745, 2748, 2757, 2759, 2761, 2763, 2765, 2768, 2768, 2784, 2784, 2790, 2799, 2817, 2819,
                                        2821, 2828, 2831, 2832, 2835, 2856, 2858, 2864, 2866, 2867, 2870, 2873, 2876, 2883, 2887, 2888, 2891, 2893, 2902, 2903, 2908, 2909,
                                        2911, 2913, 2918, 2927, 2946, 2947, 2949, 2954, 2958, 2960, 2962, 2965, 2969, 2970, 2972, 2972, 2974, 2975, 2979, 2980, 2984, 2986,
                                        2990, 2997, 2999, 3001, 3006, 3010, 3014, 3016, 3018, 3021, 3031, 3031, 3047, 3055, 3073, 3075, 3077, 3084, 3086, 3088, 3090, 3112,
                                        3114, 3123, 3125, 3129, 3134, 3140, 3142, 3144, 3146, 3149, 3157, 3158, 3168, 3169, 3174, 3183, 3202, 3203, 3205, 3212, 3214, 3216,
                                        3218, 3240, 3242, 3251, 3253, 3257, 3262, 3268, 3270, 3272, 3274, 3277, 3285, 3286, 3294, 3294, 3296, 3297, 3302, 3311, 3330, 3331,
                                        3333, 3340, 3342, 3344, 3346, 3368, 3370, 3385, 3390, 3395, 3398, 3400, 3402, 3405, 3415, 3415, 3424, 3425, 3430, 3439, 3458, 3459,
                                        3461, 3478, 3482, 3505, 3507, 3515, 3517, 3517, 3520, 3526, 3530, 3530, 3535, 3540, 3542, 3542, 3544, 3551, 3570, 3571, 3585, 3642,
                                        3648, 3662, 3664, 3673, 3713, 3714, 3716, 3716, 3719, 3720, 3722, 3722, 3725, 3725, 3732, 3735, 3737, 3743, 3745, 3747, 3749, 3749,
                                        3751, 3751, 3754, 3755, 3757, 3769, 3771, 3773, 3776, 3780, 3782, 3782, 3784, 3789, 3792, 3801, 3804, 3805, 3840, 3840, 3864, 3865,
                                        3872, 3881, 3893, 3893, 3895, 3895, 3897, 3897, 3902, 3911, 3913, 3946, 3953, 3972, 3974, 3979, 3984, 3991, 3993, 4028, 4038, 4038,
                                        4096, 4129, 4131, 4135, 4137, 4138, 4140, 4146, 4150, 4153, 4160, 4169, 4176, 4185, 4256, 4293, 4304, 4342, 4352, 4441, 4447, 4514,
                                        4520, 4601, 4608, 4614, 4616, 4678, 4680, 4680, 4682, 4685, 4688, 4694, 4696, 4696, 4698, 4701, 4704, 4742, 4744, 4744, 4746, 4749,
                                        4752, 4782, 4784, 4784, 4786, 4789, 4792, 4798, 4800, 4800, 4802, 4805, 4808, 4814, 4816, 4822, 4824, 4846, 4848, 4878, 4880, 4880,
                                        4882, 4885, 4888, 4894, 4896, 4934, 4936, 4954, 4969, 4977, 5024, 5108, 5121, 5740, 5743, 5750, 5761, 5786, 5792, 5866, 6016, 6099,
                                        6112, 6121, 6160, 6169, 6176, 6263, 6272, 6313, 7680, 7835, 7840, 7929, 7936, 7957, 7960, 7965, 7968, 8005, 8008, 8013, 8016, 8023,
                                        8025, 8025, 8027, 8027, 8029, 8029, 8031, 8061, 8064, 8116, 8118, 8124, 8126, 8126, 8130, 8132, 8134, 8140, 8144, 8147, 8150, 8155,
                                        8160, 8172, 8178, 8180, 8182, 8188, 8255, 8256, 8319, 8319, 8400, 8412, 8417, 8417, 8450, 8450, 8455, 8455, 8458, 8467, 8469, 8469,
                                        8473, 8477, 8484, 8484, 8486, 8486, 8488, 8488, 8490, 8493, 8495, 8497, 8499, 8505, 8544, 8579, 12293, 12295, 12321, 12335, 12337,
                                        12341, 12344, 12346, 12353, 12436, 12441, 12442, 12445, 12446, 12449, 12542, 12549, 12588, 12593, 12686, 12704, 12727, 13312, 19893,
                                        19968, 40869, 40960, 42124, 44032, 55203, 63744, 64045, 64256, 64262, 64275, 64279, 64285, 64296, 64298, 64310, 64312, 64316, 64318,
                                        64318, 64320, 64321, 64323, 64324, 64326, 64433, 64467, 64829, 64848, 64911, 64914, 64967, 65008, 65019, 65056, 65059, 65075, 65076,
                                        65101, 65103, 65136, 65138, 65140, 65140, 65142, 65276, 65296, 65305, 65313, 65338, 65343, 65343, 65345, 65370, 65381, 65470, 65474,
                                        65479, 65482, 65487, 65490, 65495, 65498, 65500,};

/*
    As per ECMAScript Language Specification 5th Edition, Section 7.6: ISyntaxToken Names and Identifiers
    IdentifierStart ::
        Can contain Unicode 6.2 categories:
        Uppercase letter (Lu),
        Lowercase letter (Ll),
        Titlecase letter (Lt),
        Modifier letter (Lm),
        Other letter (Lo), or
        Letter number (Nl).
    IdentifierPart ::
        Can contain IdentifierStart + Unicode 6.2 categories:
        Non-spacing mark (Mn),
        Combining spacing mark (Mc),
        Decimal number (Nd),
        Connector punctuation (Pc),
        <ZWNJ>, or
        <ZWJ>.

    Codepoint ranges for ES5 Identifiers are extracted from the Unicode 6.2 specification at:
    http://www.unicode.org/Public/6.2.0/ucd/UnicodeData.txt
*/
vector<int> unicodeES5IdentifierStart = {170, 170, 181, 181, 186, 186, 192, 214, 216, 246, 248, 705, 710, 721, 736, 740, 748, 748, 750, 750, 880, 884, 886, 887, 890, 893, 902,
                                         902, 904, 906, 908, 908, 910, 929, 931, 1013, 1015, 1153, 1162, 1319, 1329, 1366, 1369, 1369, 1377, 1415, 1488, 1514, 1520, 1522,
                                         1568, 1610, 1646, 1647, 1649, 1747, 1749, 1749, 1765, 1766, 1774, 1775, 1786, 1788, 1791, 1791, 1808, 1808, 1810, 1839, 1869, 1957,
                                         1969, 1969, 1994, 2026, 2036, 2037, 2042, 2042, 2048, 2069, 2074, 2074, 2084, 2084, 2088, 2088, 2112, 2136, 2208, 2208, 2210, 2220,
                                         2308, 2361, 2365, 2365, 2384, 2384, 2392, 2401, 2417, 2423, 2425, 2431, 2437, 2444, 2447, 2448, 2451, 2472, 2474, 2480, 2482, 2482,
                                         2486, 2489, 2493, 2493, 2510, 2510, 2524, 2525, 2527, 2529, 2544, 2545, 2565, 2570, 2575, 2576, 2579, 2600, 2602, 2608, 2610, 2611,
                                         2613, 2614, 2616, 2617, 2649, 2652, 2654, 2654, 2674, 2676, 2693, 2701, 2703, 2705, 2707, 2728, 2730, 2736, 2738, 2739, 2741, 2745,
                                         2749, 2749, 2768, 2768, 2784, 2785, 2821, 2828, 2831, 2832, 2835, 2856, 2858, 2864, 2866, 2867, 2869, 2873, 2877, 2877, 2908, 2909,
                                         2911, 2913, 2929, 2929, 2947, 2947, 2949, 2954, 2958, 2960, 2962, 2965, 2969, 2970, 2972, 2972, 2974, 2975, 2979, 2980, 2984, 2986,
                                         2990, 3001, 3024, 3024, 3077, 3084, 3086, 3088, 3090, 3112, 3114, 3123, 3125, 3129, 3133, 3133, 3160, 3161, 3168, 3169, 3205, 3212,
                                         3214, 3216, 3218, 3240, 3242, 3251, 3253, 3257, 3261, 3261, 3294, 3294, 3296, 3297, 3313, 3314, 3333, 3340, 3342, 3344, 3346, 3386,
                                         3389, 3389, 3406, 3406, 3424, 3425, 3450, 3455, 3461, 3478, 3482, 3505, 3507, 3515, 3517, 3517, 3520, 3526, 3585, 3632, 3634, 3635,
                                         3648, 3654, 3713, 3714, 3716, 3716, 3719, 3720, 3722, 3722, 3725, 3725, 3732, 3735, 3737, 3743, 3745, 3747, 3749, 3749, 3751, 3751,
                                         3754, 3755, 3757, 3760, 3762, 3763, 3773, 3773, 3776, 3780, 3782, 3782, 3804, 3807, 3840, 3840, 3904, 3911, 3913, 3948, 3976, 3980,
                                         4096, 4138, 4159, 4159, 4176, 4181, 4186, 4189, 4193, 4193, 4197, 4198, 4206, 4208, 4213, 4225, 4238, 4238, 4256, 4293, 4295, 4295,
                                         4301, 4301, 4304, 4346, 4348, 4680, 4682, 4685, 4688, 4694, 4696, 4696, 4698, 4701, 4704, 4744, 4746, 4749, 4752, 4784, 4786, 4789,
                                         4792, 4798, 4800, 4800, 4802, 4805, 4808, 4822, 4824, 4880, 4882, 4885, 4888, 4954, 4992, 5007, 5024, 5108, 5121, 5740, 5743, 5759,
                                         5761, 5786, 5792, 5866, 5870, 5872, 5888, 5900, 5902, 5905, 5920, 5937, 5952, 5969, 5984, 5996, 5998, 6000, 6016, 6067, 6103, 6103,
                                         6108, 6108, 6176, 6263, 6272, 6312, 6314, 6314, 6320, 6389, 6400, 6428, 6480, 6509, 6512, 6516, 6528, 6571, 6593, 6599, 6656, 6678,
                                         6688, 6740, 6823, 6823, 6917, 6963, 6981, 6987, 7043, 7072, 7086, 7087, 7098, 7141, 7168, 7203, 7245, 7247, 7258, 7293, 7401, 7404,
                                         7406, 7409, 7413, 7414, 7424, 7615, 7680, 7957, 7960, 7965, 7968, 8005, 8008, 8013, 8016, 8023, 8025, 8025, 8027, 8027, 8029, 8029,
                                         8031, 8061, 8064, 8116, 8118, 8124, 8126, 8126, 8130, 8132, 8134, 8140, 8144, 8147, 8150, 8155, 8160, 8172, 8178, 8180, 8182, 8188,
                                         8305, 8305, 8319, 8319, 8336, 8348, 8450, 8450, 8455, 8455, 8458, 8467, 8469, 8469, 8473, 8477, 8484, 8484, 8486, 8486, 8488, 8488,
                                         8490, 8493, 8495, 8505, 8508, 8511, 8517, 8521, 8526, 8526, 8544, 8584, 11264, 11310, 11312, 11358, 11360, 11492, 11499, 11502, 11506,
                                         11507, 11520, 11557, 11559, 11559, 11565, 11565, 11568, 11623, 11631, 11631, 11648, 11670, 11680, 11686, 11688, 11694, 11696, 11702,
                                         11704, 11710, 11712, 11718, 11720, 11726, 11728, 11734, 11736, 11742, 11823, 11823, 12293, 12295, 12321, 12329, 12337, 12341, 12344,
                                         12348, 12353, 12438, 12445, 12447, 12449, 12538, 12540, 12543, 12549, 12589, 12593, 12686, 12704, 12730, 12784, 12799, 13312, 19893,
                                         19968, 40908, 40960, 42124, 42192, 42237, 42240, 42508, 42512, 42527, 42538, 42539, 42560, 42606, 42623, 42647, 42656, 42735, 42775,
                                         42783, 42786, 42888, 42891, 42894, 42896, 42899, 42912, 42922, 43000, 43009, 43011, 43013, 43015, 43018, 43020, 43042, 43072, 43123,
                                         43138, 43187, 43250, 43255, 43259, 43259, 43274, 43301, 43312, 43334, 43360, 43388, 43396, 43442, 43471, 43471, 43520, 43560, 43584,
                                         43586, 43588, 43595, 43616, 43638, 43642, 43642, 43648, 43695, 43697, 43697, 43701, 43702, 43705, 43709, 43712, 43712, 43714, 43714,
                                         43739, 43741, 43744, 43754, 43762, 43764, 43777, 43782, 43785, 43790, 43793, 43798, 43808, 43814, 43816, 43822, 43968, 44002, 44032,
                                         55203, 55216, 55238, 55243, 55291, 63744, 64109, 64112, 64217, 64256, 64262, 64275, 64279, 64285, 64285, 64287, 64296, 64298, 64310,
                                         64312, 64316, 64318, 64318, 64320, 64321, 64323, 64324, 64326, 64433, 64467, 64829, 64848, 64911, 64914, 64967, 65008, 65019, 65136,
                                         65140, 65142, 65276, 65313, 65338, 65345, 65370, 65382, 65470, 65474, 65479, 65482, 65487, 65490, 65495, 65498, 65500,};
vector<int> unicodeES5IdentifierPart = {170, 170, 181, 181, 186, 186, 192, 214, 216, 246, 248, 705, 710, 721, 736, 740, 748, 748, 750, 750, 768, 884, 886, 887, 890, 893, 902,
                                        902, 904, 906, 908, 908, 910, 929, 931, 1013, 1015, 1153, 1155, 1159, 1162, 1319, 1329, 1366, 1369, 1369, 1377, 1415, 1425, 1469, 1471,
                                        1471, 1473, 1474, 1476, 1477, 1479, 1479, 1488, 1514, 1520, 1522, 1552, 1562, 1568, 1641, 1646, 1747, 1749, 1756, 1759, 1768, 1770,
                                        1788, 1791, 1791, 1808, 1866, 1869, 1969, 1984, 2037, 2042, 2042, 2048, 2093, 2112, 2139, 2208, 2208, 2210, 2220, 2276, 2302, 2304,
                                        2403, 2406, 2415, 2417, 2423, 2425, 2431, 2433, 2435, 2437, 2444, 2447, 2448, 2451, 2472, 2474, 2480, 2482, 2482, 2486, 2489, 2492,
                                        2500, 2503, 2504, 2507, 2510, 2519, 2519, 2524, 2525, 2527, 2531, 2534, 2545, 2561, 2563, 2565, 2570, 2575, 2576, 2579, 2600, 2602,
                                        2608, 2610, 2611, 2613, 2614, 2616, 2617, 2620, 2620, 2622, 2626, 2631, 2632, 2635, 2637, 2641, 2641, 2649, 2652, 2654, 2654, 2662,
                                        2677, 2689, 2691, 2693, 2701, 2703, 2705, 2707, 2728, 2730, 2736, 2738, 2739, 2741, 2745, 2748, 2757, 2759, 2761, 2763, 2765, 2768,
                                        2768, 2784, 2787, 2790, 2799, 2817, 2819, 2821, 2828, 2831, 2832, 2835, 2856, 2858, 2864, 2866, 2867, 2869, 2873, 2876, 2884, 2887,
                                        2888, 2891, 2893, 2902, 2903, 2908, 2909, 2911, 2915, 2918, 2927, 2929, 2929, 2946, 2947, 2949, 2954, 2958, 2960, 2962, 2965, 2969,
                                        2970, 2972, 2972, 2974, 2975, 2979, 2980, 2984, 2986, 2990, 3001, 3006, 3010, 3014, 3016, 3018, 3021, 3024, 3024, 3031, 3031, 3046,
                                        3055, 3073, 3075, 3077, 3084, 3086, 3088, 3090, 3112, 3114, 3123, 3125, 3129, 3133, 3140, 3142, 3144, 3146, 3149, 3157, 3158, 3160,
                                        3161, 3168, 3171, 3174, 3183, 3202, 3203, 3205, 3212, 3214, 3216, 3218, 3240, 3242, 3251, 3253, 3257, 3260, 3268, 3270, 3272, 3274,
                                        3277, 3285, 3286, 3294, 3294, 3296, 3299, 3302, 3311, 3313, 3314, 3330, 3331, 3333, 3340, 3342, 3344, 3346, 3386, 3389, 3396, 3398,
                                        3400, 3402, 3406, 3415, 3415, 3424, 3427, 3430, 3439, 3450, 3455, 3458, 3459, 3461, 3478, 3482, 3505, 3507, 3515, 3517, 3517, 3520,
                                        3526, 3530, 3530, 3535, 3540, 3542, 3542, 3544, 3551, 3570, 3571, 3585, 3642, 3648, 3662, 3664, 3673, 3713, 3714, 3716, 3716, 3719,
                                        3720, 3722, 3722, 3725, 3725, 3732, 3735, 3737, 3743, 3745, 3747, 3749, 3749, 3751, 3751, 3754, 3755, 3757, 3769, 3771, 3773, 3776,
                                        3780, 3782, 3782, 3784, 3789, 3792, 3801, 3804, 3807, 3840, 3840, 3864, 3865, 3872, 3881, 3893, 3893, 3895, 3895, 3897, 3897, 3902,
                                        3911, 3913, 3948, 3953, 3972, 3974, 3991, 3993, 4028, 4038, 4038, 4096, 4169, 4176, 4253, 4256, 4293, 4295, 4295, 4301, 4301, 4304,
                                        4346, 4348, 4680, 4682, 4685, 4688, 4694, 4696, 4696, 4698, 4701, 4704, 4744, 4746, 4749, 4752, 4784, 4786, 4789, 4792, 4798, 4800,
                                        4800, 4802, 4805, 4808, 4822, 4824, 4880, 4882, 4885, 4888, 4954, 4957, 4959, 4992, 5007, 5024, 5108, 5121, 5740, 5743, 5759, 5761,
                                        5786, 5792, 5866, 5870, 5872, 5888, 5900, 5902, 5908, 5920, 5940, 5952, 5971, 5984, 5996, 5998, 6000, 6002, 6003, 6016, 6099, 6103,
                                        6103, 6108, 6109, 6112, 6121, 6155, 6157, 6160, 6169, 6176, 6263, 6272, 6314, 6320, 6389, 6400, 6428, 6432, 6443, 6448, 6459, 6470,
                                        6509, 6512, 6516, 6528, 6571, 6576, 6601, 6608, 6617, 6656, 6683, 6688, 6750, 6752, 6780, 6783, 6793, 6800, 6809, 6823, 6823, 6912,
                                        6987, 6992, 7001, 7019, 7027, 7040, 7155, 7168, 7223, 7232, 7241, 7245, 7293, 7376, 7378, 7380, 7414, 7424, 7654, 7676, 7957, 7960,
                                        7965, 7968, 8005, 8008, 8013, 8016, 8023, 8025, 8025, 8027, 8027, 8029, 8029, 8031, 8061, 8064, 8116, 8118, 8124, 8126, 8126, 8130,
                                        8132, 8134, 8140, 8144, 8147, 8150, 8155, 8160, 8172, 8178, 8180, 8182, 8188, 8204, 8205, 8255, 8256, 8276, 8276, 8305, 8305, 8319,
                                        8319, 8336, 8348, 8400, 8412, 8417, 8417, 8421, 8432, 8450, 8450, 8455, 8455, 8458, 8467, 8469, 8469, 8473, 8477, 8484, 8484, 8486,
                                        8486, 8488, 8488, 8490, 8493, 8495, 8505, 8508, 8511, 8517, 8521, 8526, 8526, 8544, 8584, 11264, 11310, 11312, 11358, 11360, 11492,
                                        11499, 11507, 11520, 11557, 11559, 11559, 11565, 11565, 11568, 11623, 11631, 11631, 11647, 11670, 11680, 11686, 11688, 11694, 11696,
                                        11702, 11704, 11710, 11712, 11718, 11720, 11726, 11728, 11734, 11736, 11742, 11744, 11775, 11823, 11823, 12293, 12295, 12321, 12335,
                                        12337, 12341, 12344, 12348, 12353, 12438, 12441, 12442, 12445, 12447, 12449, 12538, 12540, 12543, 12549, 12589, 12593, 12686, 12704,
                                        12730, 12784, 12799, 13312, 19893, 19968, 40908, 40960, 42124, 42192, 42237, 42240, 42508, 42512, 42539, 42560, 42607, 42612, 42621,
                                        42623, 42647, 42655, 42737, 42775, 42783, 42786, 42888, 42891, 42894, 42896, 42899, 42912, 42922, 43000, 43047, 43072, 43123, 43136,
                                        43204, 43216, 43225, 43232, 43255, 43259, 43259, 43264, 43309, 43312, 43347, 43360, 43388, 43392, 43456, 43471, 43481, 43520, 43574,
                                        43584, 43597, 43600, 43609, 43616, 43638, 43642, 43643, 43648, 43714, 43739, 43741, 43744, 43759, 43762, 43766, 43777, 43782, 43785,
                                        43790, 43793, 43798, 43808, 43814, 43816, 43822, 43968, 44010, 44012, 44013, 44016, 44025, 44032, 55203, 55216, 55238, 55243, 55291,
                                        63744, 64109, 64112, 64217, 64256, 64262, 64275, 64279, 64285, 64296, 64298, 64310, 64312, 64316, 64318, 64318, 64320, 64321, 64323,
                                        64324, 64326, 64433, 64467, 64829, 64848, 64911, 64914, 64967, 65008, 65019, 65024, 65039, 65056, 65062, 65075, 65076, 65101, 65103,
                                        65136, 65140, 65142, 65276, 65296, 65305, 65313, 65338, 65343, 65343, 65345, 65370, 65382, 65470, 65474, 65479, 65482, 65487, 65490,
                                        65495, 65498, 65500,};

/**
 * Generated by scripts/regenerate-unicode-identifier-parts.js on node v12.4.0 with unicode 12.1
 * based on http://www.unicode.org/reports/tr31/ and https://www.ecma-international.org/ecma-262/6.0/#sec-names-and-keywords
 * unicodeESNextIdentifierStart corresponds to the ID_Start and Other_ID_Start property, and
 * unicodeESNextIdentifierPart corresponds to ID_Continue, Other_ID_Continue, plus ID_Start and Other_ID_Start
 */
vector<int> unicodeESNextIdentifierStart = {65, 90, 97, 122, 170, 170, 181, 181, 186, 186, 192, 214, 216, 246, 248, 705, 710, 721, 736, 740, 748, 748, 750, 750, 880, 884, 886,
                                            887, 890, 893, 895, 895, 902, 902, 904, 906, 908, 908, 910, 929, 931, 1013, 1015, 1153, 1162, 1327, 1329, 1366, 1369, 1369, 1376,
                                            1416, 1488, 1514, 1519, 1522, 1568, 1610, 1646, 1647, 1649, 1747, 1749, 1749, 1765, 1766, 1774, 1775, 1786, 1788, 1791, 1791, 1808,
                                            1808, 1810, 1839, 1869, 1957, 1969, 1969, 1994, 2026, 2036, 2037, 2042, 2042, 2048, 2069, 2074, 2074, 2084, 2084, 2088, 2088, 2112,
                                            2136, 2144, 2154, 2208, 2228, 2230, 2237, 2308, 2361, 2365, 2365, 2384, 2384, 2392, 2401, 2417, 2432, 2437, 2444, 2447, 2448, 2451,
                                            2472, 2474, 2480, 2482, 2482, 2486, 2489, 2493, 2493, 2510, 2510, 2524, 2525, 2527, 2529, 2544, 2545, 2556, 2556, 2565, 2570, 2575,
                                            2576, 2579, 2600, 2602, 2608, 2610, 2611, 2613, 2614, 2616, 2617, 2649, 2652, 2654, 2654, 2674, 2676, 2693, 2701, 2703, 2705, 2707,
                                            2728, 2730, 2736, 2738, 2739, 2741, 2745, 2749, 2749, 2768, 2768, 2784, 2785, 2809, 2809, 2821, 2828, 2831, 2832, 2835, 2856, 2858,
                                            2864, 2866, 2867, 2869, 2873, 2877, 2877, 2908, 2909, 2911, 2913, 2929, 2929, 2947, 2947, 2949, 2954, 2958, 2960, 2962, 2965, 2969,
                                            2970, 2972, 2972, 2974, 2975, 2979, 2980, 2984, 2986, 2990, 3001, 3024, 3024, 3077, 3084, 3086, 3088, 3090, 3112, 3114, 3129, 3133,
                                            3133, 3160, 3162, 3168, 3169, 3200, 3200, 3205, 3212, 3214, 3216, 3218, 3240, 3242, 3251, 3253, 3257, 3261, 3261, 3294, 3294, 3296,
                                            3297, 3313, 3314, 3333, 3340, 3342, 3344, 3346, 3386, 3389, 3389, 3406, 3406, 3412, 3414, 3423, 3425, 3450, 3455, 3461, 3478, 3482,
                                            3505, 3507, 3515, 3517, 3517, 3520, 3526, 3585, 3632, 3634, 3635, 3648, 3654, 3713, 3714, 3716, 3716, 3718, 3722, 3724, 3747, 3749,
                                            3749, 3751, 3760, 3762, 3763, 3773, 3773, 3776, 3780, 3782, 3782, 3804, 3807, 3840, 3840, 3904, 3911, 3913, 3948, 3976, 3980, 4096,
                                            4138, 4159, 4159, 4176, 4181, 4186, 4189, 4193, 4193, 4197, 4198, 4206, 4208, 4213, 4225, 4238, 4238, 4256, 4293, 4295, 4295, 4301,
                                            4301, 4304, 4346, 4348, 4680, 4682, 4685, 4688, 4694, 4696, 4696, 4698, 4701, 4704, 4744, 4746, 4749, 4752, 4784, 4786, 4789, 4792,
                                            4798, 4800, 4800, 4802, 4805, 4808, 4822, 4824, 4880, 4882, 4885, 4888, 4954, 4992, 5007, 5024, 5109, 5112, 5117, 5121, 5740, 5743,
                                            5759, 5761, 5786, 5792, 5866, 5870, 5880, 5888, 5900, 5902, 5905, 5920, 5937, 5952, 5969, 5984, 5996, 5998, 6000, 6016, 6067, 6103,
                                            6103, 6108, 6108, 6176, 6264, 6272, 6312, 6314, 6314, 6320, 6389, 6400, 6430, 6480, 6509, 6512, 6516, 6528, 6571, 6576, 6601, 6656,
                                            6678, 6688, 6740, 6823, 6823, 6917, 6963, 6981, 6987, 7043, 7072, 7086, 7087, 7098, 7141, 7168, 7203, 7245, 7247, 7258, 7293, 7296,
                                            7304, 7312, 7354, 7357, 7359, 7401, 7404, 7406, 7411, 7413, 7414, 7418, 7418, 7424, 7615, 7680, 7957, 7960, 7965, 7968, 8005, 8008,
                                            8013, 8016, 8023, 8025, 8025, 8027, 8027, 8029, 8029, 8031, 8061, 8064, 8116, 8118, 8124, 8126, 8126, 8130, 8132, 8134, 8140, 8144,
                                            8147, 8150, 8155, 8160, 8172, 8178, 8180, 8182, 8188, 8305, 8305, 8319, 8319, 8336, 8348, 8450, 8450, 8455, 8455, 8458, 8467, 8469,
                                            8469, 8472, 8477, 8484, 8484, 8486, 8486, 8488, 8488, 8490, 8505, 8508, 8511, 8517, 8521, 8526, 8526, 8544, 8584, 11264, 11310,
                                            11312, 11358, 11360, 11492, 11499, 11502, 11506, 11507, 11520, 11557, 11559, 11559, 11565, 11565, 11568, 11623, 11631, 11631,
                                            11648, 11670, 11680, 11686, 11688, 11694, 11696, 11702, 11704, 11710, 11712, 11718, 11720, 11726, 11728, 11734, 11736, 11742,
                                            12293, 12295, 12321, 12329, 12337, 12341, 12344, 12348, 12353, 12438, 12443, 12447, 12449, 12538, 12540, 12543, 12549, 12591,
                                            12593, 12686, 12704, 12730, 12784, 12799, 13312, 19893, 19968, 40943, 40960, 42124, 42192, 42237, 42240, 42508, 42512, 42527,
                                            42538, 42539, 42560, 42606, 42623, 42653, 42656, 42735, 42775, 42783, 42786, 42888, 42891, 42943, 42946, 42950, 42999, 43009,
                                            43011, 43013, 43015, 43018, 43020, 43042, 43072, 43123, 43138, 43187, 43250, 43255, 43259, 43259, 43261, 43262, 43274, 43301,
                                            43312, 43334, 43360, 43388, 43396, 43442, 43471, 43471, 43488, 43492, 43494, 43503, 43514, 43518, 43520, 43560, 43584, 43586,
                                            43588, 43595, 43616, 43638, 43642, 43642, 43646, 43695, 43697, 43697, 43701, 43702, 43705, 43709, 43712, 43712, 43714, 43714,
                                            43739, 43741, 43744, 43754, 43762, 43764, 43777, 43782, 43785, 43790, 43793, 43798, 43808, 43814, 43816, 43822, 43824, 43866,
                                            43868, 43879, 43888, 44002, 44032, 55203, 55216, 55238, 55243, 55291, 63744, 64109, 64112, 64217, 64256, 64262, 64275, 64279,
                                            64285, 64285, 64287, 64296, 64298, 64310, 64312, 64316, 64318, 64318, 64320, 64321, 64323, 64324, 64326, 64433, 64467, 64829,
                                            64848, 64911, 64914, 64967, 65008, 65019, 65136, 65140, 65142, 65276, 65313, 65338, 65345, 65370, 65382, 65470, 65474, 65479,
                                            65482, 65487, 65490, 65495, 65498, 65500, 65536, 65547, 65549, 65574, 65576, 65594, 65596, 65597, 65599, 65613, 65616, 65629,
                                            65664, 65786, 65856, 65908, 66176, 66204, 66208, 66256, 66304, 66335, 66349, 66378, 66384, 66421, 66432, 66461, 66464, 66499,
                                            66504, 66511, 66513, 66517, 66560, 66717, 66736, 66771, 66776, 66811, 66816, 66855, 66864, 66915, 67072, 67382, 67392, 67413,
                                            67424, 67431, 67584, 67589, 67592, 67592, 67594, 67637, 67639, 67640, 67644, 67644, 67647, 67669, 67680, 67702, 67712, 67742,
                                            67808, 67826, 67828, 67829, 67840, 67861, 67872, 67897, 67968, 68023, 68030, 68031, 68096, 68096, 68112, 68115, 68117, 68119,
                                            68121, 68149, 68192, 68220, 68224, 68252, 68288, 68295, 68297, 68324, 68352, 68405, 68416, 68437, 68448, 68466, 68480, 68497,
                                            68608, 68680, 68736, 68786, 68800, 68850, 68864, 68899, 69376, 69404, 69415, 69415, 69424, 69445, 69600, 69622, 69635, 69687,
                                            69763, 69807, 69840, 69864, 69891, 69926, 69956, 69956, 69968, 70002, 70006, 70006, 70019, 70066, 70081, 70084, 70106, 70106,
                                            70108, 70108, 70144, 70161, 70163, 70187, 70272, 70278, 70280, 70280, 70282, 70285, 70287, 70301, 70303, 70312, 70320, 70366,
                                            70405, 70412, 70415, 70416, 70419, 70440, 70442, 70448, 70450, 70451, 70453, 70457, 70461, 70461, 70480, 70480, 70493, 70497,
                                            70656, 70708, 70727, 70730, 70751, 70751, 70784, 70831, 70852, 70853, 70855, 70855, 71040, 71086, 71128, 71131, 71168, 71215,
                                            71236, 71236, 71296, 71338, 71352, 71352, 71424, 71450, 71680, 71723, 71840, 71903, 71935, 71935, 72096, 72103, 72106, 72144,
                                            72161, 72161, 72163, 72163, 72192, 72192, 72203, 72242, 72250, 72250, 72272, 72272, 72284, 72329, 72349, 72349, 72384, 72440,
                                            72704, 72712, 72714, 72750, 72768, 72768, 72818, 72847, 72960, 72966, 72968, 72969, 72971, 73008, 73030, 73030, 73056, 73061,
                                            73063, 73064, 73066, 73097, 73112, 73112, 73440, 73458, 73728, 74649, 74752, 74862, 74880, 75075, 77824, 78894, 82944, 83526,
                                            92160, 92728, 92736, 92766, 92880, 92909, 92928, 92975, 92992, 92995, 93027, 93047, 93053, 93071, 93760, 93823, 93952, 94026,
                                            94032, 94032, 94099, 94111, 94176, 94177, 94179, 94179, 94208, 100343, 100352, 101106, 110592, 110878, 110928, 110930, 110948,
                                            110951, 110960, 111355, 113664, 113770, 113776, 113788, 113792, 113800, 113808, 113817, 119808, 119892, 119894, 119964, 119966,
                                            119967, 119970, 119970, 119973, 119974, 119977, 119980, 119982, 119993, 119995, 119995, 119997, 120003, 120005, 120069, 120071,
                                            120074, 120077, 120084, 120086, 120092, 120094, 120121, 120123, 120126, 120128, 120132, 120134, 120134, 120138, 120144, 120146,
                                            120485, 120488, 120512, 120514, 120538, 120540, 120570, 120572, 120596, 120598, 120628, 120630, 120654, 120656, 120686, 120688,
                                            120712, 120714, 120744, 120746, 120770, 120772, 120779, 123136, 123180, 123191, 123197, 123214, 123214, 123584, 123627, 124928,
                                            125124, 125184, 125251, 125259, 125259, 126464, 126467, 126469, 126495, 126497, 126498, 126500, 126500, 126503, 126503, 126505,
                                            126514, 126516, 126519, 126521, 126521, 126523, 126523, 126530, 126530, 126535, 126535, 126537, 126537, 126539, 126539, 126541,
                                            126543, 126545, 126546, 126548, 126548, 126551, 126551, 126553, 126553, 126555, 126555, 126557, 126557, 126559, 126559, 126561,
                                            126562, 126564, 126564, 126567, 126570, 126572, 126578, 126580, 126583, 126585, 126588, 126590, 126590, 126592, 126601, 126603,
                                            126619, 126625, 126627, 126629, 126633, 126635, 126651, 131072, 173782, 173824, 177972, 177984, 178205, 178208, 183969, 183984,
                                            191456, 194560, 195101};
vector<int> unicodeESNextIdentifierPart = {48, 57, 65, 90, 95, 95, 97, 122, 170, 170, 181, 181, 183, 183, 186, 186, 192, 214, 216, 246, 248, 705, 710, 721, 736, 740, 748, 748,
                                           750, 750, 768, 884, 886, 887, 890, 893, 895, 895, 902, 906, 908, 908, 910, 929, 931, 1013, 1015, 1153, 1155, 1159, 1162, 1327, 1329,
                                           1366, 1369, 1369, 1376, 1416, 1425, 1469, 1471, 1471, 1473, 1474, 1476, 1477, 1479, 1479, 1488, 1514, 1519, 1522, 1552, 1562, 1568,
                                           1641, 1646, 1747, 1749, 1756, 1759, 1768, 1770, 1788, 1791, 1791, 1808, 1866, 1869, 1969, 1984, 2037, 2042, 2042, 2045, 2045, 2048,
                                           2093, 2112, 2139, 2144, 2154, 2208, 2228, 2230, 2237, 2259, 2273, 2275, 2403, 2406, 2415, 2417, 2435, 2437, 2444, 2447, 2448, 2451,
                                           2472, 2474, 2480, 2482, 2482, 2486, 2489, 2492, 2500, 2503, 2504, 2507, 2510, 2519, 2519, 2524, 2525, 2527, 2531, 2534, 2545, 2556,
                                           2556, 2558, 2558, 2561, 2563, 2565, 2570, 2575, 2576, 2579, 2600, 2602, 2608, 2610, 2611, 2613, 2614, 2616, 2617, 2620, 2620, 2622,
                                           2626, 2631, 2632, 2635, 2637, 2641, 2641, 2649, 2652, 2654, 2654, 2662, 2677, 2689, 2691, 2693, 2701, 2703, 2705, 2707, 2728, 2730,
                                           2736, 2738, 2739, 2741, 2745, 2748, 2757, 2759, 2761, 2763, 2765, 2768, 2768, 2784, 2787, 2790, 2799, 2809, 2815, 2817, 2819, 2821,
                                           2828, 2831, 2832, 2835, 2856, 2858, 2864, 2866, 2867, 2869, 2873, 2876, 2884, 2887, 2888, 2891, 2893, 2902, 2903, 2908, 2909, 2911,
                                           2915, 2918, 2927, 2929, 2929, 2946, 2947, 2949, 2954, 2958, 2960, 2962, 2965, 2969, 2970, 2972, 2972, 2974, 2975, 2979, 2980, 2984,
                                           2986, 2990, 3001, 3006, 3010, 3014, 3016, 3018, 3021, 3024, 3024, 3031, 3031, 3046, 3055, 3072, 3084, 3086, 3088, 3090, 3112, 3114,
                                           3129, 3133, 3140, 3142, 3144, 3146, 3149, 3157, 3158, 3160, 3162, 3168, 3171, 3174, 3183, 3200, 3203, 3205, 3212, 3214, 3216, 3218,
                                           3240, 3242, 3251, 3253, 3257, 3260, 3268, 3270, 3272, 3274, 3277, 3285, 3286, 3294, 3294, 3296, 3299, 3302, 3311, 3313, 3314, 3328,
                                           3331, 3333, 3340, 3342, 3344, 3346, 3396, 3398, 3400, 3402, 3406, 3412, 3415, 3423, 3427, 3430, 3439, 3450, 3455, 3458, 3459, 3461,
                                           3478, 3482, 3505, 3507, 3515, 3517, 3517, 3520, 3526, 3530, 3530, 3535, 3540, 3542, 3542, 3544, 3551, 3558, 3567, 3570, 3571, 3585,
                                           3642, 3648, 3662, 3664, 3673, 3713, 3714, 3716, 3716, 3718, 3722, 3724, 3747, 3749, 3749, 3751, 3773, 3776, 3780, 3782, 3782, 3784,
                                           3789, 3792, 3801, 3804, 3807, 3840, 3840, 3864, 3865, 3872, 3881, 3893, 3893, 3895, 3895, 3897, 3897, 3902, 3911, 3913, 3948, 3953,
                                           3972, 3974, 3991, 3993, 4028, 4038, 4038, 4096, 4169, 4176, 4253, 4256, 4293, 4295, 4295, 4301, 4301, 4304, 4346, 4348, 4680, 4682,
                                           4685, 4688, 4694, 4696, 4696, 4698, 4701, 4704, 4744, 4746, 4749, 4752, 4784, 4786, 4789, 4792, 4798, 4800, 4800, 4802, 4805, 4808,
                                           4822, 4824, 4880, 4882, 4885, 4888, 4954, 4957, 4959, 4969, 4977, 4992, 5007, 5024, 5109, 5112, 5117, 5121, 5740, 5743, 5759, 5761,
                                           5786, 5792, 5866, 5870, 5880, 5888, 5900, 5902, 5908, 5920, 5940, 5952, 5971, 5984, 5996, 5998, 6000, 6002, 6003, 6016, 6099, 6103,
                                           6103, 6108, 6109, 6112, 6121, 6155, 6157, 6160, 6169, 6176, 6264, 6272, 6314, 6320, 6389, 6400, 6430, 6432, 6443, 6448, 6459, 6470,
                                           6509, 6512, 6516, 6528, 6571, 6576, 6601, 6608, 6618, 6656, 6683, 6688, 6750, 6752, 6780, 6783, 6793, 6800, 6809, 6823, 6823, 6832,
                                           6845, 6912, 6987, 6992, 7001, 7019, 7027, 7040, 7155, 7168, 7223, 7232, 7241, 7245, 7293, 7296, 7304, 7312, 7354, 7357, 7359, 7376,
                                           7378, 7380, 7418, 7424, 7673, 7675, 7957, 7960, 7965, 7968, 8005, 8008, 8013, 8016, 8023, 8025, 8025, 8027, 8027, 8029, 8029, 8031,
                                           8061, 8064, 8116, 8118, 8124, 8126, 8126, 8130, 8132, 8134, 8140, 8144, 8147, 8150, 8155, 8160, 8172, 8178, 8180, 8182, 8188, 8255,
                                           8256, 8276, 8276, 8305, 8305, 8319, 8319, 8336, 8348, 8400, 8412, 8417, 8417, 8421, 8432, 8450, 8450, 8455, 8455, 8458, 8467, 8469,
                                           8469, 8472, 8477, 8484, 8484, 8486, 8486, 8488, 8488, 8490, 8505, 8508, 8511, 8517, 8521, 8526, 8526, 8544, 8584, 11264, 11310,
                                           11312, 11358, 11360, 11492, 11499, 11507, 11520, 11557, 11559, 11559, 11565, 11565, 11568, 11623, 11631, 11631, 11647, 11670, 11680,
                                           11686, 11688, 11694, 11696, 11702, 11704, 11710, 11712, 11718, 11720, 11726, 11728, 11734, 11736, 11742, 11744, 11775, 12293, 12295,
                                           12321, 12335, 12337, 12341, 12344, 12348, 12353, 12438, 12441, 12447, 12449, 12538, 12540, 12543, 12549, 12591, 12593, 12686, 12704,
                                           12730, 12784, 12799, 13312, 19893, 19968, 40943, 40960, 42124, 42192, 42237, 42240, 42508, 42512, 42539, 42560, 42607, 42612, 42621,
                                           42623, 42737, 42775, 42783, 42786, 42888, 42891, 42943, 42946, 42950, 42999, 43047, 43072, 43123, 43136, 43205, 43216, 43225, 43232,
                                           43255, 43259, 43259, 43261, 43309, 43312, 43347, 43360, 43388, 43392, 43456, 43471, 43481, 43488, 43518, 43520, 43574, 43584, 43597,
                                           43600, 43609, 43616, 43638, 43642, 43714, 43739, 43741, 43744, 43759, 43762, 43766, 43777, 43782, 43785, 43790, 43793, 43798, 43808,
                                           43814, 43816, 43822, 43824, 43866, 43868, 43879, 43888, 44010, 44012, 44013, 44016, 44025, 44032, 55203, 55216, 55238, 55243, 55291,
                                           63744, 64109, 64112, 64217, 64256, 64262, 64275, 64279, 64285, 64296, 64298, 64310, 64312, 64316, 64318, 64318, 64320, 64321, 64323,
                                           64324, 64326, 64433, 64467, 64829, 64848, 64911, 64914, 64967, 65008, 65019, 65024, 65039, 65056, 65071, 65075, 65076, 65101, 65103,
                                           65136, 65140, 65142, 65276, 65296, 65305, 65313, 65338, 65343, 65343, 65345, 65370, 65382, 65470, 65474, 65479, 65482, 65487, 65490,
                                           65495, 65498, 65500, 65536, 65547, 65549, 65574, 65576, 65594, 65596, 65597, 65599, 65613, 65616, 65629, 65664, 65786, 65856, 65908,
                                           66045, 66045, 66176, 66204, 66208, 66256, 66272, 66272, 66304, 66335, 66349, 66378, 66384, 66426, 66432, 66461, 66464, 66499, 66504,
                                           66511, 66513, 66517, 66560, 66717, 66720, 66729, 66736, 66771, 66776, 66811, 66816, 66855, 66864, 66915, 67072, 67382, 67392, 67413,
                                           67424, 67431, 67584, 67589, 67592, 67592, 67594, 67637, 67639, 67640, 67644, 67644, 67647, 67669, 67680, 67702, 67712, 67742, 67808,
                                           67826, 67828, 67829, 67840, 67861, 67872, 67897, 67968, 68023, 68030, 68031, 68096, 68099, 68101, 68102, 68108, 68115, 68117, 68119,
                                           68121, 68149, 68152, 68154, 68159, 68159, 68192, 68220, 68224, 68252, 68288, 68295, 68297, 68326, 68352, 68405, 68416, 68437, 68448,
                                           68466, 68480, 68497, 68608, 68680, 68736, 68786, 68800, 68850, 68864, 68903, 68912, 68921, 69376, 69404, 69415, 69415, 69424, 69456,
                                           69600, 69622, 69632, 69702, 69734, 69743, 69759, 69818, 69840, 69864, 69872, 69881, 69888, 69940, 69942, 69951, 69956, 69958, 69968,
                                           70003, 70006, 70006, 70016, 70084, 70089, 70092, 70096, 70106, 70108, 70108, 70144, 70161, 70163, 70199, 70206, 70206, 70272, 70278,
                                           70280, 70280, 70282, 70285, 70287, 70301, 70303, 70312, 70320, 70378, 70384, 70393, 70400, 70403, 70405, 70412, 70415, 70416, 70419,
                                           70440, 70442, 70448, 70450, 70451, 70453, 70457, 70459, 70468, 70471, 70472, 70475, 70477, 70480, 70480, 70487, 70487, 70493, 70499,
                                           70502, 70508, 70512, 70516, 70656, 70730, 70736, 70745, 70750, 70751, 70784, 70853, 70855, 70855, 70864, 70873, 71040, 71093, 71096,
                                           71104, 71128, 71133, 71168, 71232, 71236, 71236, 71248, 71257, 71296, 71352, 71360, 71369, 71424, 71450, 71453, 71467, 71472, 71481,
                                           71680, 71738, 71840, 71913, 71935, 71935, 72096, 72103, 72106, 72151, 72154, 72161, 72163, 72164, 72192, 72254, 72263, 72263, 72272,
                                           72345, 72349, 72349, 72384, 72440, 72704, 72712, 72714, 72758, 72760, 72768, 72784, 72793, 72818, 72847, 72850, 72871, 72873, 72886,
                                           72960, 72966, 72968, 72969, 72971, 73014, 73018, 73018, 73020, 73021, 73023, 73031, 73040, 73049, 73056, 73061, 73063, 73064, 73066,
                                           73102, 73104, 73105, 73107, 73112, 73120, 73129, 73440, 73462, 73728, 74649, 74752, 74862, 74880, 75075, 77824, 78894, 82944, 83526,
                                           92160, 92728, 92736, 92766, 92768, 92777, 92880, 92909, 92912, 92916, 92928, 92982, 92992, 92995, 93008, 93017, 93027, 93047, 93053,
                                           93071, 93760, 93823, 93952, 94026, 94031, 94087, 94095, 94111, 94176, 94177, 94179, 94179, 94208, 100343, 100352, 101106, 110592,
                                           110878, 110928, 110930, 110948, 110951, 110960, 111355, 113664, 113770, 113776, 113788, 113792, 113800, 113808, 113817, 113821,
                                           113822, 119141, 119145, 119149, 119154, 119163, 119170, 119173, 119179, 119210, 119213, 119362, 119364, 119808, 119892, 119894,
                                           119964, 119966, 119967, 119970, 119970, 119973, 119974, 119977, 119980, 119982, 119993, 119995, 119995, 119997, 120003, 120005,
                                           120069, 120071, 120074, 120077, 120084, 120086, 120092, 120094, 120121, 120123, 120126, 120128, 120132, 120134, 120134, 120138,
                                           120144, 120146, 120485, 120488, 120512, 120514, 120538, 120540, 120570, 120572, 120596, 120598, 120628, 120630, 120654, 120656,
                                           120686, 120688, 120712, 120714, 120744, 120746, 120770, 120772, 120779, 120782, 120831, 121344, 121398, 121403, 121452, 121461,
                                           121461, 121476, 121476, 121499, 121503, 121505, 121519, 122880, 122886, 122888, 122904, 122907, 122913, 122915, 122916, 122918,
                                           122922, 123136, 123180, 123184, 123197, 123200, 123209, 123214, 123214, 123584, 123641, 124928, 125124, 125136, 125142, 125184,
                                           125259, 125264, 125273, 126464, 126467, 126469, 126495, 126497, 126498, 126500, 126500, 126503, 126503, 126505, 126514, 126516,
                                           126519, 126521, 126521, 126523, 126523, 126530, 126530, 126535, 126535, 126537, 126537, 126539, 126539, 126541, 126543, 126545,
                                           126546, 126548, 126548, 126551, 126551, 126553, 126553, 126555, 126555, 126557, 126557, 126559, 126559, 126561, 126562, 126564,
                                           126564, 126567, 126570, 126572, 126578, 126580, 126583, 126585, 126588, 126590, 126590, 126592, 126601, 126603, 126619, 126625,
                                           126627, 126629, 126633, 126635, 126651, 131072, 173782, 173824, 177972, 177984, 178205, 178208, 183969, 183984, 191456, 194560,
                                           195101, 917760, 917999};

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

bool isConflictMarkerTrivia(const string text, int pos) {
    assert(pos >= 0);

    // Conflict markers must be at the start of a line.
    if (pos == 0 || isLineBreak(charCodeAt(text, pos - 1))) {
        auto ch = charCodeAt(text, pos);

        if ((pos + mergeConflictMarkerLength) < text.size()) {
            for (int i = 0; i < mergeConflictMarkerLength; i ++) {
                if (charCodeAt(text, pos + i).code != ch.code) {
                    return false;
                }
            }

            return ch.code == CharacterCodes::equals || charCodeAt(text, pos + mergeConflictMarkerLength).code == CharacterCodes::space;
        }
    }

    return false;
}

int Scanner::error(DiagnosticMessage message, int errPos, int length) {
    if (errPos == - 1) errPos = pos;

    cout << "Error: " << message.code << ": " << message.message << " at " << errPos << "\n";

    if (onError) {
        (*onError)(message, length);
    }
}

int scanConflictMarkerTrivia(string &text, int pos) {
    auto ch = charCodeAt(text, pos);
    auto len = text.size();

    if (ch.code == CharacterCodes::lessThan || ch.code == CharacterCodes::greaterThan) {
        while (pos < len && ! isLineBreak(charCodeAt(text, pos))) {
            pos ++;
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

            pos ++;
        }
    }

    return pos;
}

int Scanner::scanConflictMarkerTrivia(string &text, int pos) {
    error(Diagnostics::Merge_conflict_marker_encountered, pos, mergeConflictMarkerLength);
    return scanConflictMarkerTrivia(text, pos);
}

bool lookupInUnicodeMap(CharCode code, vector<int> &map) {
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
           lookupInUnicodeMap(code, unicodeESNextIdentifierStart) :
           languageVersion == ScriptTarget::ES5 ? lookupInUnicodeMap(code, unicodeES5IdentifierStart) :
           lookupInUnicodeMap(code, unicodeES3IdentifierStart);
}

bool isUnicodeIdentifierPart(CharCode ch, ScriptTarget languageVersion) {
    return languageVersion >= ScriptTarget::ES2015 ?
           lookupInUnicodeMap(ch, unicodeESNextIdentifierPart) :
           languageVersion == ScriptTarget::ES5 ? lookupInUnicodeMap(ch, unicodeES5IdentifierPart) :
           lookupInUnicodeMap(ch, unicodeES3IdentifierPart);
}

bool isIdentifierStart(CharCode ch, ScriptTarget languageVersion) {
    return (ch.code >= CharacterCodes::A && ch.code <= CharacterCodes::Z) || (ch.code >= CharacterCodes::a && ch.code <= CharacterCodes::z) ||
           ch.code == CharacterCodes::$ || ch.code == CharacterCodes::_ ||
           (ch.code > CharacterCodes::maxAsciiCharacter && isUnicodeIdentifierStart(ch, languageVersion));
}

bool isIdentifierPart(CharCode ch, ScriptTarget languageVersion, LanguageVariant identifierVariant = LanguageVariant::Standard) {
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

bool isWhiteSpaceLike(CharCode ch) {
    return isWhiteSpaceSingleLine(ch) || isLineBreak(ch);
}

/* @internal */
int skipTrivia(string &text, int pos, optional<bool> stopAfterLineBreak, optional<bool> stopAtComments, optional<bool> inJSDoc) {
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
        auto escapedValue = escapedValueString.size() ? stoi(escapedValueString, nullptr, 16) : - 1;
        pos = start;
        return {escapedValue, 1};
    }
    return {- 1, 0};
}

CharCode Scanner::peekUnicodeEscape() {
    if (pos + 5 < end && charCodeAt(text, pos + 1).code == CharacterCodes::u) {
        auto start = pos;
        pos += 2;
        auto value = scanExactNumberOfHexDigits(4, /*canHaveSeparators*/ false);
        pos = start;
        return {value, 1};
    }
    return {- 1, 0};
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
            if (! (ch.code >= 0 && isIdentifierPart(ch, languageVersion))) {
                break;
            }
            tokenFlags |= TokenFlags::UnicodeEscape;
            result += substr(text, start, pos);
            result += fromCharCode(ch.code);
            // Valid Unicode escape is always six characters
            pos += 6;
            start = pos;
        } else {
            break;
        }
    }
    result += substr(text, start, pos);
    return result;
}

void Scanner::checkForIdentifierStartAfterNumericLiteral(int numericStart, bool isScientific) {
    if (! isIdentifierStart(charCodeAt(text, pos), languageVersion)) {
        return;
    }

    auto identifierStart = pos;
    auto parts = scanIdentifierParts();

    if (parts.size() == 1 && text[identifierStart] == 'n') {
        if (isScientific) {
            error(Diagnostics::A_bigint_literal_cannot_use_exponential_notation, numericStart, identifierStart - numericStart + 1);
        } else {
            error(Diagnostics::A_bigint_literal_must_be_an_integer, numericStart, identifierStart - numericStart + 1);
        }
    } else {
        error(Diagnostics::An_identifier_or_keyword_cannot_immediately_follow_a_numeric_literal, identifierStart, parts.size());
        pos = identifierStart;
    }
}

SyntaxKind Scanner::checkBigIntSuffix() {
    if (charCodeAt(text, pos).code == CharacterCodes::n) {
        tokenValue += "n";
        // Use base 10 instead of base 2 or base 8 for shorter literals
        if (tokenFlags & TokenFlags::BinaryOrOctalSpecifier) {
            tokenValue = parsePseudoBigInt(tokenValue) + "n";
        }
        pos ++;
        return SyntaxKind::BigIntLiteral;
    } else {
        // not a bigint, so can convert to number in simplified form
        // Number() may not support 0b or 0o, so use parseInt() instead
        auto numericValue = tokenFlags & TokenFlags::BinarySpecifier
                            ? stoi(substr(tokenValue, 2), 0, 2) // skip "0b"
                            : tokenFlags & TokenFlags::OctalSpecifier
                              ? stoi(substr(tokenValue, 2), 0, 8) // skip "0o"
                              : stoi(tokenValue);
        tokenValue = "" + std::to_string(numericValue);
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
                pos ++;
                continue;
            } else if (ch.code == CharacterCodes::colon && ! namespaceSeparator) {
                tokenValue += ":";
                pos ++;
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
        if (substr(tokenValue, - 1) == ":") {
            tokenValue = substr(tokenValue, 0, - 1);
            pos --;
        }
        return getIdentifierToken();
    }
    return token;
}

SyntaxKind Scanner::scanJsxToken(bool allowMultilineJsxText) {
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
        pos ++;
        return token = SyntaxKind::LessThanToken;
    }

    if (charCode.code == CharacterCodes::openBrace) {
        pos ++;
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
            error(Diagnostics::Unexpected_token_Did_you_mean_or_gt, pos, 1);
        }
        if (charCode.code == CharacterCodes::closeBrace) {
            error(Diagnostics::Unexpected_token_Did_you_mean_or_rbrace, pos, 1);
        }

        // FirstNonWhitespace is 0, then we only see whitespaces so far. If we see a linebreak, we want to ignore that whitespaces.
        // i.e (- : whitespace)
        //      <div>----
        //      </div> becomes <div></div>
        //
        //      <div>----</div> becomes <div>----</div>
        if (isLineBreak(charCode) && firstNonWhitespace == 0) {
            firstNonWhitespace = - 1;
        } else if (! allowMultilineJsxText && isLineBreak(charCode) && firstNonWhitespace > 0) {
            // Stop JsxText on each line during formatting. This allows the formatter to
            // indent each line correctly.
            break;
        } else if (! isWhiteSpaceLike(charCode)) {
            firstNonWhitespace = pos;
        }

        pos ++;
    }

    tokenValue = substr(text, startPos, pos);

    return firstNonWhitespace == - 1 ? SyntaxKind::JsxTextAllWhiteSpaces : SyntaxKind::JsxText;
}

vector<CommentDirective> Scanner::appendIfCommentDirective(
        vector<CommentDirective> &commentDirectives,
        const string &text,
        const regex &commentDirectiveRegEx,
        int lineStart
) {
    auto type = getDirectiveFromComment(trimStringStart(text), commentDirectiveRegEx);
    if (! type.has_value()) {
        return commentDirectives;
    }

    commentDirectives.push_back(CommentDirective{.range =  {.pos =  lineStart, .end = pos}, .type = type.value()});
    return commentDirectives;
}

ScanNumber Scanner::scanNumber() {
    auto start = pos;
    auto mainFragment = scanNumberFragment();
    string decimalFragment;
    bool decimalFragmentSet = false;
    string scientificFragment;
    bool scientificFragmentSet = false;
    if (charCodeAt(text, pos).code == CharacterCodes::dot) {
        pos ++;
        decimalFragment = scanNumberFragment();
        decimalFragmentSet = true;
    }
    auto end = pos;
    if (charCodeAt(text, pos).code == CharacterCodes::E || charCodeAt(text, pos).code == CharacterCodes::e) {
        pos ++;
        tokenFlags |= TokenFlags::Scientific;
        if (charCodeAt(text, pos).code == CharacterCodes::plus || charCodeAt(text, pos).code == CharacterCodes::minus) pos ++;
        auto preNumericPart = pos;
        auto finalFragment = scanNumberFragment();
        if (! finalFragment.size()) {
//            error(Diagnostics::Digit_expected);
        } else {
            scientificFragment = substr(text, end, preNumericPart) + finalFragment;
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
        result = substr(text, start, end); // No need to use all the fragments; no _ removal needed
    }

    if (decimalFragmentSet || tokenFlags & TokenFlags::Scientific) {
        checkForIdentifierStartAfterNumericLiteral(start, ! decimalFragmentSet && ! ! (tokenFlags & TokenFlags::Scientific));
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
//                error(Diagnostics::Multiple_consecutive_numeric_separators_are_not_permitted, pos, 1);
            } else {
//                error(Diagnostics::Numeric_separators_are_not_allowed_here, pos, 1);
            }
            pos ++;
            continue;
        }
        separatorAllowed = true;
        if (! isDigit(ch) || ch.code - CharacterCodes::_0 >= base) {
            break;
        }
        value += text[pos];
        pos ++;
        isPreviousTokenSeparator = false;
    }
    if (charCodeAt(text, pos - 1).code == CharacterCodes::_) {
        // Literal ends with underscore - not allowed
//        error(Diagnostics::Numeric_separators_are_not_allowed_here, pos - 1, 1);
    }
    return value;
}

SyntaxKind Scanner::getIdentifierToken() {
    // Reserved words are between 2 and 12 characters long and start with a lowercase letter
    auto len = tokenValue.size();
    if (len >= 2 && len <= 12) {
        auto ch = charCodeAt(tokenValue, 0);
        if (ch.code >= CharacterCodes::a && ch.code <= CharacterCodes::z) {
            auto it = textToKeyword.find(tokenValue);
            if (it != textToKeyword.end()) {
                return token = it->second;
            }
        }
    }
    return token = SyntaxKind::Identifier;
}

SyntaxKind Scanner::scanIdentifier(CharCode startCharacter, ScriptTarget languageVersion) {
    auto ch = startCharacter;
    if (isIdentifierStart(ch, languageVersion)) {
        pos += ch.length;
        while (pos < end && isIdentifierPart(ch = charCodeAt(text, pos), languageVersion)) pos += ch.length;
        tokenValue = substr(text, tokenPos, pos);
        if (ch.code == CharacterCodes::backslash) {
            tokenValue += scanIdentifierParts();
        }
        return getIdentifierToken();
    }
    return SyntaxKind::Unknown;
}

SyntaxKind Scanner::scan() {
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
        if (ch.code == CharacterCodes::hash && pos == 0 && isShebangTrivia(text, pos)) {
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
                    pos ++;
                    continue;
                } else {
                    if (ch.code == CharacterCodes::carriageReturn && pos + 1 < end &&
                        charCodeAt(text, pos + 1).code == CharacterCodes::lineFeed) {
                        // consume both CR and LF
                        pos += 2;
                    } else {
                        pos ++;
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
                    pos ++;
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
                pos ++;
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
                pos ++;
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
                pos ++;
                return token = SyntaxKind::AmpersandToken;
            case CharacterCodes::openParen:
                pos ++;
                return token = SyntaxKind::OpenParenToken;
            case CharacterCodes::closeParen:
                pos ++;
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
                pos ++;
                if (inJSDocType && ! asteriskSeen && (tokenFlags & TokenFlags::PrecedingLineBreak)) {
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
                pos ++;
                return token = SyntaxKind::PlusToken;
            case CharacterCodes::comma:
                pos ++;
                return token = SyntaxKind::CommaToken;
            case CharacterCodes::minus:
                if (charCodeAt(text, pos + 1).code == CharacterCodes::minus) {
                    return pos += 2, token = SyntaxKind::MinusMinusToken;
                }
                if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                    return pos += 2, token = SyntaxKind::MinusEqualsToken;
                }
                pos ++;
                return token = SyntaxKind::MinusToken;
            case CharacterCodes::dot:
                if (isDigit(charCodeAt(text, pos + 1))) {
                    tokenValue = scanNumber().value;
                    return token = SyntaxKind::NumericLiteral;
                }
                if (charCodeAt(text, pos + 1).code == CharacterCodes::dot && charCodeAt(text, pos + 2).code == CharacterCodes::dot) {
                    return pos += 3, token = SyntaxKind::DotDotDotToken;
                }
                pos ++;
                return token = SyntaxKind::DotToken;
            case CharacterCodes::slash:
                // Single-line comment
                if (charCodeAt(text, pos + 1).code == CharacterCodes::slash) {
                    pos += 2;

                    while (pos < end) {
                        if (isLineBreak(charCodeAt(text, pos))) {
                            break;
                        }
                        pos ++;
                    }

                    commentDirectives = appendIfCommentDirective(
                            commentDirectives,
                            substr(text, tokenPos, pos),
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

                        pos ++;

                        if (isLineBreak(ch)) {
                            lastLineStart = pos;
                            tokenFlags |= TokenFlags::PrecedingLineBreak;
                        }
                    }

                    commentDirectives = appendIfCommentDirective(commentDirectives, substr(text, lastLineStart, pos),
                                                                 commentDirectiveRegExMultiLine, lastLineStart);

                    if (! commentClosed) {
//                        error(Diagnostics::Asterisk_Slash_expected);
                    }

                    if (skipTrivia) {
                        continue;
                    } else {
                        if (! commentClosed) {
                            tokenFlags |= TokenFlags::Unterminated;
                        }
                        return token = SyntaxKind::MultiLineCommentTrivia;
                    }
                }

                if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                    return pos += 2, token = SyntaxKind::SlashEqualsToken;
                }

                pos ++;
                return token = SyntaxKind::SlashToken;

            case CharacterCodes::_0:
                if (pos + 2 < end && (charCodeAt(text, pos + 1).code ==
                                      CharacterCodes::X || charCodeAt(text, pos + 1).code == CharacterCodes::x)) {
                    pos += 2;
                    tokenValue = scanMinimumNumberOfHexDigits(1, /*canHaveSeparators*/ true);
                    if (tokenValue.empty()) {
//                        error(Diagnostics::Hexadecimal_digit_expected);
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
//                        error(Diagnostics::Binary_digit_expected);
                        tokenValue = "0";
                    }
                    tokenValue = "0b" + tokenValue;
                    tokenFlags |= TokenFlags::BinarySpecifier;
                    return token = checkBigIntSuffix();
                } else if (pos + 2 < end && (charCodeAt(text, pos + 1).code == CharacterCodes::O || charCodeAt(text, pos + 1).code == CharacterCodes::o)) {
                    pos += 2;
                    tokenValue = scanBinaryOrOctalDigits(/* base */ 8);
                    if (tokenValue.empty()) {
//                        error(Diagnostics::Octal_digit_expected);
                        tokenValue = "0";
                    }
                    tokenValue = "0o" + tokenValue;
                    tokenFlags |= TokenFlags::OctalSpecifier;
                    return token = checkBigIntSuffix();
                }
                // Try to parse as an octal
                if (pos + 1 < end && isOctalDigit(charCodeAt(text, pos + 1))) {
                    tokenValue = "" + scanOctalDigits();
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
                pos ++;
                return token = SyntaxKind::ColonToken;
            case CharacterCodes::semicolon:
                pos ++;
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
                pos ++;
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
                pos ++;
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

                pos ++;
                return token = SyntaxKind::GreaterThanToken;
            case CharacterCodes::question:
                if (charCodeAt(text, pos + 1).code == CharacterCodes::dot && ! isDigit(charCodeAt(text, pos + 2))) {
                    return pos += 2, token = SyntaxKind::QuestionDotToken;
                }
                if (charCodeAt(text, pos + 1).code == CharacterCodes::question) {
                    if (charCodeAt(text, pos + 2).code == CharacterCodes::equals) {
                        return pos += 3, token = SyntaxKind::QuestionQuestionEqualsToken;
                    }
                    return pos += 2, token = SyntaxKind::QuestionQuestionToken;
                }
                pos ++;
                return token = SyntaxKind::QuestionToken;
            case CharacterCodes::openBracket:
                pos ++;
                return token = SyntaxKind::OpenBracketToken;
            case CharacterCodes::closeBracket:
                pos ++;
                return token = SyntaxKind::CloseBracketToken;
            case CharacterCodes::caret:
                if (charCodeAt(text, pos + 1).code == CharacterCodes::equals) {
                    return pos += 2, token = SyntaxKind::CaretEqualsToken;
                }
                pos ++;
                return token = SyntaxKind::CaretToken;
            case CharacterCodes::openBrace:
                pos ++;
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
                pos ++;
                return token = SyntaxKind::BarToken;
            case CharacterCodes::closeBrace:
                pos ++;
                return token = SyntaxKind::CloseBraceToken;
            case CharacterCodes::tilde:
                pos ++;
                return token = SyntaxKind::TildeToken;
            case CharacterCodes::at:
                pos ++;
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

                error(Diagnostics::Invalid_character);
                pos ++;
                return token = SyntaxKind::Unknown;
            }
            case CharacterCodes::hash:
                if (pos != 0 && text[pos + 1] == '!') {
                    error(Diagnostics::can_only_be_used_at_the_start_of_a_file);
                    pos ++;
                    return token = SyntaxKind::Unknown;
                }

                if (isIdentifierStart(charCodeAt(text, pos + 1), languageVersion)) {
                    pos ++;
                    scanIdentifier(charCodeAt(text, pos), languageVersion);
                } else {
                    tokenValue = fromCharCode(charCodeAt(text, pos).code);
                    error(Diagnostics::Invalid_character, pos ++, ch.length);
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
                error(Diagnostics::Invalid_character, pos, size);
                pos += size;
                return token = SyntaxKind::Unknown;
            }
        }
    }

    return SyntaxKind::EndOfFileToken;
}

bool Scanner::isOctalDigit(CharCode ch) {
    return ch.code >= CharacterCodes::_0 && ch.code <= CharacterCodes::_7;
}

int Scanner::scanOctalDigits() {
    auto start = pos;
    while (isOctalDigit(charCodeAt(text, pos))) {
        pos ++;
    }
    return stoi(substr(text, start, pos));
}

template<typename T>
T Scanner::lookAhead(function<T()> callback) {
    return speculationHelper<T>(callback, /*isLookahead*/ true);
}

template<typename T>
T Scanner::tryScan(function<T()> callback) {
    return speculationHelper<T>(callback, /*isLookahead*/ false);
}

template<typename T>
T Scanner::speculationHelper(function<T()> callback, bool isLookahead) {
    auto savePos = pos;
    auto saveStartPos = startPos;
    auto saveTokenPos = tokenPos;
    auto saveToken = token;
    auto saveTokenValue = tokenValue;
    auto saveTokenFlags = tokenFlags;
    auto result = callback();

    // If our callback returned something 'falsy' or we're just looking ahead,
    // then unconditionally restore us to where we were.
    if (! result || isLookahead) {
        pos = savePos;
        startPos = saveStartPos;
        tokenPos = saveTokenPos;
        token = saveToken;
        tokenValue = saveTokenValue;
        tokenFlags = saveTokenFlags;
    }
    return result;;
}

SyntaxKind Scanner::reScanJsxToken(bool allowMultilineJsxText) {
    pos = tokenPos = startPos;
    return token = scanJsxToken(allowMultilineJsxText);
};