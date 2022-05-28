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
    using ErrorCallback = function<
    void(DiagnosticMessage
    message,
    int length
    )>;

    /** @internal */
    static unordered_map<string, SyntaxKind> textToKeywordObj{
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

    static unordered_map<string, SyntaxKind> textToTokenBase{
            {"{",    SyntaxKind::OpenBraceToken},
            {"}",    SyntaxKind::CloseBraceToken},
            {"(",    SyntaxKind::OpenParenToken},
            {")",    SyntaxKind::CloseParenToken},
            {"[",    SyntaxKind::OpenBracketToken},
            {"]",    SyntaxKind::CloseBracketToken},
            {".",    SyntaxKind::DotToken},
            {"...",  SyntaxKind::DotDotDotToken},
            {";",    SyntaxKind::SemicolonToken},
            {"},",   SyntaxKind::CommaToken},
            {"<",    SyntaxKind::LessThanToken},
            {">",    SyntaxKind::GreaterThanToken},
            {"<=",   SyntaxKind::LessThanEqualsToken},
            {">=",   SyntaxKind::GreaterThanEqualsToken},
            {"==",   SyntaxKind::EqualsEqualsToken},
            {"!=",   SyntaxKind::ExclamationEqualsToken},
            {"===",  SyntaxKind::EqualsEqualsEqualsToken},
            {"!==",  SyntaxKind::ExclamationEqualsEqualsToken},
            {"=>",   SyntaxKind::EqualsGreaterThanToken},
            {"+",    SyntaxKind::PlusToken},
            {"-",    SyntaxKind::MinusToken},
            {"**",   SyntaxKind::AsteriskAsteriskToken},
            {"*",    SyntaxKind::AsteriskToken},
            {"/",    SyntaxKind::SlashToken},
            {"%",    SyntaxKind::PercentToken},
            {"++",   SyntaxKind::PlusPlusToken},
            {"--",   SyntaxKind::MinusMinusToken},
            {"<<",   SyntaxKind::LessThanLessThanToken},
            {"</",   SyntaxKind::LessThanSlashToken},
            {">>",   SyntaxKind::GreaterThanGreaterThanToken},
            {">>>",  SyntaxKind::GreaterThanGreaterThanGreaterThanToken},
            {"&",    SyntaxKind::AmpersandToken},
            {"|",    SyntaxKind::BarToken},
            {"^",    SyntaxKind::CaretToken},
            {"!",    SyntaxKind::ExclamationToken},
            {"~",    SyntaxKind::TildeToken},
            {"&&",   SyntaxKind::AmpersandAmpersandToken},
            {"||",   SyntaxKind::BarBarToken},
            {"?",    SyntaxKind::QuestionToken},
            {"??",   SyntaxKind::QuestionQuestionToken},
            {"?.",   SyntaxKind::QuestionDotToken},
            {":",    SyntaxKind::ColonToken},
            {"=",    SyntaxKind::EqualsToken},
            {"+=",   SyntaxKind::PlusEqualsToken},
            {"-=",   SyntaxKind::MinusEqualsToken},
            {"*=",   SyntaxKind::AsteriskEqualsToken},
            {"**=",  SyntaxKind::AsteriskAsteriskEqualsToken},
            {"/=",   SyntaxKind::SlashEqualsToken},
            {"%=",   SyntaxKind::PercentEqualsToken},
            {"<<=",  SyntaxKind::LessThanLessThanEqualsToken},
            {">>=",  SyntaxKind::GreaterThanGreaterThanEqualsToken},
            {">>>=", SyntaxKind::GreaterThanGreaterThanGreaterThanEqualsToken},
            {"&=",   SyntaxKind::AmpersandEqualsToken},
            {"|=",   SyntaxKind::BarEqualsToken},
            {"^=",   SyntaxKind::CaretEqualsToken},
            {"||=",  SyntaxKind::BarBarEqualsToken},
            {"&&=",  SyntaxKind::AmpersandAmpersandEqualsToken},
            {"??=",  SyntaxKind::QuestionQuestionEqualsToken},
            {"@",    SyntaxKind::AtToken},
            {"#",    SyntaxKind::HashToken},
            {"`",    SyntaxKind::BacktickToken},
    };

    static auto textToToken = combine<SyntaxKind>(textToKeywordObj, textToTokenBase);

    /* @internal */
    static optional<SyntaxKind> stringToToken(string s) {
        return get(textToToken, s);
    }

    /* @internal */
    static bool tokenIsIdentifierOrKeyword(SyntaxKind token) {
        return token >= SyntaxKind::Identifier;
    }

    /* @internal */
    static bool tokenIsIdentifierOrKeywordOrGreaterThan(SyntaxKind token) {
        return token == SyntaxKind::GreaterThanToken || tokenIsIdentifierOrKeyword(token);
    }

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

        bool hasPrecedingJSDocComment() {
            //todo
            return false;
        }

        string getTokenValue() {
            return tokenValue;
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

        bool isReservedWord() {
            return token >= SyntaxKind::FirstReservedWord && token <= SyntaxKind::LastReservedWord;
        }

        string getTokenText() {
            return text.substr(tokenPos, pos);
        }

        SyntaxKind reScanInvalidIdentifier() {
//            Debug.assert(token === SyntaxKind::Unknown, "'reScanInvalidIdentifier' should only be called when the current token is 'SyntaxKind::Unknown'.");
            pos = tokenPos = startPos;
            tokenFlags = 0;
            auto ch = charCodeAt(text, pos);
            auto identifierKind = scanIdentifier(ch, ScriptTarget::ESNext);
            if (identifierKind) {
                return token = identifierKind;
            }
            pos += ch.length;
            return token; // Still `SyntaKind.Unknown`
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
