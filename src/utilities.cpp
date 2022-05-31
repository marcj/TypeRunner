#pragma once

#include "Tracy.hpp"
#include "types.h"
#include "utilities.h"
#include "core.h"

namespace ts {
    LanguageVariant getLanguageVariant(ScriptKind scriptKind) {
        // .tsx and .jsx files are treated as jsx language variant.
        return scriptKind == ScriptKind::TSX || scriptKind == ScriptKind::JSX || scriptKind == ScriptKind::JS || scriptKind == ScriptKind::JSON ? LanguageVariant::JSX : LanguageVariant::Standard;
    }

    sharedOpt<Node> lastOrUndefined(sharedOpt<NodeArray> array) {
        if (!array) return nullptr;
        auto last = lastOrUndefined(array->list);
        if (last) *last;
        return nullptr;
    }

    NodeArray &setTextRangePosEnd(NodeArray &range, int pos, int end) {
        range.pos = pos;
        range.end = end;
        return range;
    }

    /**
     * Gets a custom text range to use when emitting source maps.
     */
    shared<SourceMapRange> getSourceMapRange(shared<Node> node) {
        if (!node->emitNode) return node;
        return node->emitNode->sourceMapRange;
    }
    /**
     * Gets a custom text range to use when emitting comments.
     */
    shared<TextRange> getCommentRange(shared<Node> node) {
        if (!node->emitNode) return node;
        if (!node->emitNode->commentRange) return node;
        return node->emitNode->commentRange;
    }

    optional<vector<SynthesizedComment>> getSyntheticLeadingComments(shared<Node> node) {
        if (!node->emitNode) return nullopt;
        return node->emitNode->leadingComments;
    }

    optional<vector<SynthesizedComment>> getSyntheticTrailingComments(shared<Node> node) {
        if (!node->emitNode) return nullopt;
        return node->emitNode->trailingComments;
    }

    bool positionIsSynthesized(int pos) {
        // This is a fast way of testing the following conditions:
        //  pos === undefined || pos === null || isNaN(pos) || pos < 0;
        return !(pos >= 0);
    }

    bool nodeIsSynthesized(shared<TextRange> range) {
        return positionIsSynthesized(range->pos) || positionIsSynthesized(range->end);
    }

    NodeArray setTextRange(NodeArray range, optional<NodeArray> location) {
        return location ? setTextRangePosEnd(range, location->pos, location->end) : range;
    }

    string parsePseudoBigInt(string &stringValue) {
        int log2Base;
        switch (charCodeAt(stringValue, 1).code) { // "x" in "0x123"
            case CharacterCodes::b:
            case CharacterCodes::B: // 0b or 0B
                log2Base = 1;
                break;
            case CharacterCodes::o:
            case CharacterCodes::O: // 0o or 0O
                log2Base = 3;
                break;
            case CharacterCodes::x:
            case CharacterCodes::X: // 0x or 0X
                log2Base = 4;
                break;
            default: // already in decimal; omit trailing "n"
                auto nIndex = stringValue.size() - 1;
                // Skip leading 0s
                auto nonZeroStart = 0;
                while (charCodeAt(stringValue, nonZeroStart).code == CharacterCodes::_0) {
                    nonZeroStart++;
                }
                return stringValue.substr(nonZeroStart, nIndex);
        }

        // Omit leading "0b", "0o", or "0x", and trailing "n"
        auto startIndex = 2;
        auto endIndex = stringValue.size() - 1;
        auto bitsNeeded = (endIndex - startIndex) * log2Base;
        // Stores the value specified by the string as a LE array of 16-bit integers
        // using Uint16 instead of Uint32 so combining steps can use bitwise operators
        //auto segments = new Uint16Array((bitsNeeded >>> 4) + (bitsNeeded & 15 ? 1 : 0));
        vector<int> segments;
        segments.reserve((bitsNeeded >> 4) + (bitsNeeded & 15 ? 1 : 0));

        // Add the digits, one at a time
        for (int i = endIndex - 1, bitOffset = 0; i >= startIndex; i--, bitOffset += log2Base) {
            auto segment = bitOffset >> 4;
            auto digitChar = charCodeAt(stringValue, i).code;
            // Find character range: 0-9 < A-F < a-f
            auto digit = digitChar <= CharacterCodes::_9
                         ? digitChar - CharacterCodes::_0
                         : 10 + digitChar -
                           (digitChar <= CharacterCodes::F ? CharacterCodes::A : CharacterCodes::a);
            auto shiftedDigit = digit << (bitOffset & 15);
            segments[segment] |= shiftedDigit;
            auto residual = shiftedDigit >> 16;
            if (residual) segments[segment + 1] |= residual; // overflows segment
        }
        // Repeatedly divide segments by 10 and add remainder to base10Value
        string base10Value;
        auto firstNonzeroSegment = segments.size() - 1;
        auto segmentsRemaining = true;
        while (segmentsRemaining) {
            auto mod10 = 0;
            segmentsRemaining = false;
            for (unsigned long segment = firstNonzeroSegment; segment >= 0; segment--) {
                auto newSegment = mod10 << 16 | segments[segment];
                auto segmentValue = (newSegment / 10) | 0;
                segments[segment] = segmentValue;
                mod10 = newSegment - segmentValue * 10;
                if (segmentValue && !segmentsRemaining) {
                    firstNonzeroSegment = segment;
                    segmentsRemaining = true;
                }
            }
            base10Value = to_string(mod10) + base10Value;
        }
        return base10Value;
    }

    /** Add an extra underscore to identifiers that start with two underscores to avoid issues with magic names like '__proto__' */
    string escapeLeadingUnderscores(string identifier) {
        return (identifier.size() >= 2 && charCodeAt(identifier, 0).code == CharacterCodes::_ && charCodeAt(identifier, 1).code == CharacterCodes::_ ? "_" + identifier : identifier);
    }

    /* @internal */
    bool isParameterPropertyModifier(SyntaxKind kind) {
        return !!((int) modifierToFlag(kind) & (int) ModifierFlags::ParameterPropertyModifier);
    }

    /* @internal */
    bool isClassMemberModifier(SyntaxKind idToken) {
        return isParameterPropertyModifier(idToken) || idToken == SyntaxKind::StaticKeyword || idToken == SyntaxKind::OverrideKeyword;
    }

    /* @internal */
    bool isModifierKind(SyntaxKind token) {
        switch (token) {
            case SyntaxKind::AbstractKeyword:
            case SyntaxKind::AsyncKeyword:
            case SyntaxKind::ConstKeyword:
            case SyntaxKind::DeclareKeyword:
            case SyntaxKind::DefaultKeyword:
            case SyntaxKind::ExportKeyword:
            case SyntaxKind::InKeyword:
            case SyntaxKind::PublicKeyword:
            case SyntaxKind::PrivateKeyword:
            case SyntaxKind::ProtectedKeyword:
            case SyntaxKind::ReadonlyKeyword:
            case SyntaxKind::StaticKeyword:
            case SyntaxKind::OutKeyword:
            case SyntaxKind::OverrideKeyword:
                return true;
        }
        return false;
    }

    bool isCommaSequence(shared<Expression> node) {
        return (node->kind == SyntaxKind::BinaryExpression && node->to<BinaryExpression>().operatorToken->kind == SyntaxKind::CommaToken) || node->kind == SyntaxKind::CommaListExpression;
    }

    bool isLiteralKind(SyntaxKind kind) {
        return SyntaxKind::FirstLiteralToken <= kind && kind <= SyntaxKind::LastLiteralToken;
    }

    string pseudoBigIntToString(PseudoBigInt bigint) {
        return (bigint.negative && bigint.base10Value != "0" ? "-" : "") + bigint.base10Value;
    }

    // Pseudo-literals

    /* @internal */
    bool isTemplateLiteralKind(SyntaxKind kind) {
        return SyntaxKind::FirstTemplateToken <= kind && kind <= SyntaxKind::LastTemplateToken;
    }

    bool isTemplateLiteralToken(shared<Node> &node) {
        return isTemplateLiteralKind(node->kind);
    }

    bool isTemplateMiddleOrTemplateTail(shared<Node> &node) {
        return node->kind == SyntaxKind::TemplateMiddle || node->kind == SyntaxKind::TemplateTail;
    }

    bool isImportOrExportSpecifier(shared<Node> &node) {
        return isImportSpecifier(node) || isExportSpecifier(node);
    }

    int getBinaryOperatorPrecedence(SyntaxKind kind) {
        switch (kind) {
            case SyntaxKind::QuestionQuestionToken:
                return (int) OperatorPrecedence::Coalesce;
            case SyntaxKind::BarBarToken:
                return (int) OperatorPrecedence::LogicalOR;
            case SyntaxKind::AmpersandAmpersandToken:
                return (int) OperatorPrecedence::LogicalAND;
            case SyntaxKind::BarToken:
                return (int) OperatorPrecedence::BitwiseOR;
            case SyntaxKind::CaretToken:
                return (int) OperatorPrecedence::BitwiseXOR;
            case SyntaxKind::AmpersandToken:
                return (int) OperatorPrecedence::BitwiseAND;
            case SyntaxKind::EqualsEqualsToken:
            case SyntaxKind::ExclamationEqualsToken:
            case SyntaxKind::EqualsEqualsEqualsToken:
            case SyntaxKind::ExclamationEqualsEqualsToken:
                return (int) OperatorPrecedence::Equality;
            case SyntaxKind::LessThanToken:
            case SyntaxKind::GreaterThanToken:
            case SyntaxKind::LessThanEqualsToken:
            case SyntaxKind::GreaterThanEqualsToken:
            case SyntaxKind::InstanceOfKeyword:
            case SyntaxKind::InKeyword:
            case SyntaxKind::AsKeyword:
                return (int) OperatorPrecedence::Relational;
            case SyntaxKind::LessThanLessThanToken:
            case SyntaxKind::GreaterThanGreaterThanToken:
            case SyntaxKind::GreaterThanGreaterThanGreaterThanToken:
                return (int) OperatorPrecedence::Shift;
            case SyntaxKind::PlusToken:
            case SyntaxKind::MinusToken:
                return (int) OperatorPrecedence::Additive;
            case SyntaxKind::AsteriskToken:
            case SyntaxKind::SlashToken:
            case SyntaxKind::PercentToken:
                return (int) OperatorPrecedence::Multiplicative;
            case SyntaxKind::AsteriskAsteriskToken:
                return (int) OperatorPrecedence::Exponentiation;
        }
        // -1 is lower than all other precedences.  Returning it will cause binary expression parsing to stop.
        return -1;
    }

    bool isKeyword(types::SyntaxKind token) {
        return SyntaxKind::FirstKeyword <= token && token <= SyntaxKind::LastKeyword;
    }

    bool isContextualKeyword(types::SyntaxKind token) {
        return SyntaxKind::FirstContextualKeyword <= token && token <= SyntaxKind::LastContextualKeyword;
    }

    //unordered_map<string, string> localizedDiagnosticMessages{};

    string getLocaleSpecificMessage(DiagnosticMessage message) {
//        if (has(localizedDiagnosticMessages, message.key)) {
//            return localizedDiagnosticMessages[message.key];
//        }
        return message.message;
    }

    using DiagnosticArg = string;

    string formatStringFromArg(string &text, int i, string &v) {
        return replaceAll(text, format("{%d}", i), v);
//        return text.replace(/{(\d+)}/g, (_match, index: string) => "" + Debug.checkDefined(args[+index + baseIndex]));
    }

    string DiagnosticArgToString(DiagnosticArg &v) {
        return v; //currently only string supported
    }

    DiagnosticWithDetachedLocation createDetachedDiagnostic(string fileName, int start, int length, DiagnosticMessage message, vector<DiagnosticArg> textArg) {
        //    assertDiagnosticLocation(/*file*/ undefined, start, length);
        auto text = getLocaleSpecificMessage(message);

//        if (holds_alternative<vector<DiagnosticArg>>(textArg)) {
//            auto v = get<vector<DiagnosticArg>>(textArg);
//            if (!v.empty()) {
//                for (int i = 0; i < v.size(); i++) {
//                    text = formatStringFromArg(text, i, v[i]);
//                }
//            }
//        } else if (holds_alternative<DiagnosticArg>(textArg)) {
//            auto v = get<DiagnosticArg>(textArg);
//            text = formatStringFromArg(text, 0, v);
//        }

        return DiagnosticWithDetachedLocation{
                {
                        {
                                .messageText = text,
                                .category = message.category,
                                .code = message.code,
                        },
                        .reportsUnnecessary = message.reportsUnnecessary,
                },
                .fileName = fileName,
                .start = start,
                .length = length,
        };
    }

    DiagnosticWithDetachedLocation &addRelatedInfo(DiagnosticWithDetachedLocation &diagnostic, vector<DiagnosticRelatedInformation> relatedInformation) {
        if (!relatedInformation.size()) {
            return diagnostic;
        }
        if (!diagnostic.relatedInformation) {
            diagnostic.relatedInformation = vector<DiagnosticRelatedInformation>();
        }
//        Debug::assert(diagnostic.relatedInformation !== emptyArray, "Diagnostic had empty array singleton for related info, but is still being constructed!");
        for (auto &&v: relatedInformation) (*diagnostic.relatedInformation).push_back(v);
        return diagnostic;
    }

    ScriptKind ensureScriptKind(string fileName, optional<ScriptKind> scriptKind) {
        // Using scriptKind as a condition handles both:
        // - 'scriptKind' is unspecified and thus it is `undefined`
        // - 'scriptKind' is set and it is `Unknown` (0)
        // If the 'scriptKind' is 'undefined' or 'Unknown' then we attempt
        // to get the ScriptKind from the file name. If it cannot be resolved
        // from the file name then the default 'TS' script kind is returned.
        if (scriptKind) return *scriptKind;

        //todo: Find generic way to logicalOrLastValue
//        return getScriptKindFromFileName(fileName) || ScriptKind::TS;
    }
    /** @internal */
    bool hasInvalidEscape(shared<NodeUnion(TemplateLiteral)> templateLiteral) {
        if (isNoSubstitutionTemplateLiteral(templateLiteral)) {
            return !!templateLiteral->to<NoSubstitutionTemplateLiteral>().templateFlags;
        }

        return !!templateLiteral->to<TemplateExpression>().head->templateFlags
               || some<TemplateSpan>(templateLiteral->to<TemplateExpression>().templateSpans, [](shared<TemplateSpan> span) {
            if (span->literal->kind == SyntaxKind::TemplateTail) return !!span->literal->to<TemplateTail>().templateFlags;
            if (span->literal->kind == SyntaxKind::TemplateMiddle) return !!span->literal->to<TemplateMiddle>().templateFlags;
            return false;
        });

//        return templateLiteral && !!(isNoSubstitutionTemplateLiteral(templateLiteral)
//        ? templateLiteral->templateFlags
//        : (template.head.templateFlags || some(templateLiteral->templateSpans, span => !!span.literal.templateFlags)));
    }

    string fileExt(const string &filename) {
        auto idx = filename.rfind('.');
        if (idx == std::string::npos) return "";
        return filename.substr(idx + 1);
    }

    ScriptKind getScriptKindFromFileName(const string &fileName) {
        auto ext = fileExt(fileName);

        switch (const_hash(ext)) {
            case const_hash(Extension::Js):
            case const_hash(Extension::Cjs):
            case const_hash(Extension::Mjs):
                return ScriptKind::JS;
            case const_hash(Extension::Jsx):
                return ScriptKind::JSX;
            case const_hash(Extension::Ts):
            case const_hash(Extension::Cts):
            case const_hash(Extension::Mts):
                return ScriptKind::TS;
            case const_hash(Extension::Tsx):
                return ScriptKind::TSX;
            case const_hash(Extension::Json):
                return ScriptKind::JSON;
            default:
                return ScriptKind::Unknown;
        }
    }

    bool isOuterExpression(sharedOpt<Node> node, int kinds) {
        if (!node) return false;
        switch (node->kind) {
            case SyntaxKind::ParenthesizedExpression:
//                if (kinds & (int)OuterExpressionKinds::ExcludeJSDocTypeAssertion && isJSDocTypeAssertion(node)) {
//                    return false;
//                }
                return (kinds & (int) OuterExpressionKinds::Parentheses) != 0;
            case SyntaxKind::TypeAssertionExpression:
            case SyntaxKind::AsExpression:
                return (kinds & (int) OuterExpressionKinds::TypeAssertions) != 0;
            case SyntaxKind::NonNullExpression:
                return (kinds & (int) OuterExpressionKinds::NonNullAssertions) != 0;
            case SyntaxKind::PartiallyEmittedExpression:
                return (kinds & (int) OuterExpressionKinds::PartiallyEmittedExpressions) != 0;
        }
        return false;
    }

    sharedOpt<Expression> getExpression(sharedOpt<Node> node) {
        if (!node) return nullptr;
        switch (node->kind) {
            case SyntaxKind::ParenthesizedExpression:
                return node->to<ParenthesizedExpression>().expression;
            case SyntaxKind::TypeAssertionExpression:
                return node->to<TypeAssertion>().expression;
            case SyntaxKind::AsExpression:
                return node->to<AsExpression>().expression;
            case SyntaxKind::ElementAccessExpression:
                return node->to<ElementAccessExpression>().expression;
            case SyntaxKind::PropertyAccessExpression:
                return node->to<PropertyAccessExpression>().expression;
            case SyntaxKind::NonNullExpression:
                return node->to<NonNullExpression>().expression;
            case SyntaxKind::CallExpression:
                return node->to<CallExpression>().expression;
            case SyntaxKind::PartiallyEmittedExpression:
                return node->to<PartiallyEmittedExpression>().expression;
        }
        throw runtime_error(format("No expression found in type %d", (int) node->kind));
    }

    shared<Node> skipOuterExpressions(shared<Node> node, int kinds) {
        while (isOuterExpression(node, kinds)) {
            node = getExpression(node);
        }
        return node;
    }

    shared<Node> skipPartiallyEmittedExpressions(shared<Node> node) {
        ZoneScoped;
        return skipOuterExpressions(node, (int) OuterExpressionKinds::PartiallyEmittedExpressions);
    }

    bool isLeftHandSideExpressionKind(SyntaxKind kind) {
        switch (kind) {
            case SyntaxKind::PropertyAccessExpression:
            case SyntaxKind::ElementAccessExpression:
            case SyntaxKind::NewExpression:
            case SyntaxKind::CallExpression:
            case SyntaxKind::JsxElement:
            case SyntaxKind::JsxSelfClosingElement:
            case SyntaxKind::JsxFragment:
            case SyntaxKind::TaggedTemplateExpression:
            case SyntaxKind::ArrayLiteralExpression:
            case SyntaxKind::ParenthesizedExpression:
            case SyntaxKind::ObjectLiteralExpression:
            case SyntaxKind::ClassExpression:
            case SyntaxKind::FunctionExpression:
            case SyntaxKind::Identifier:
            case SyntaxKind::PrivateIdentifier: // technically this is only an Expression if it's in a `#field in expr` BinaryExpression
            case SyntaxKind::RegularExpressionLiteral:
            case SyntaxKind::NumericLiteral:
            case SyntaxKind::BigIntLiteral:
            case SyntaxKind::StringLiteral:
            case SyntaxKind::NoSubstitutionTemplateLiteral:
            case SyntaxKind::TemplateExpression:
            case SyntaxKind::FalseKeyword:
            case SyntaxKind::NullKeyword:
            case SyntaxKind::ThisKeyword:
            case SyntaxKind::TrueKeyword:
            case SyntaxKind::SuperKeyword:
            case SyntaxKind::NonNullExpression:
            case SyntaxKind::ExpressionWithTypeArguments:
            case SyntaxKind::MetaProperty:
            case SyntaxKind::ImportKeyword: // technically this is only an Expression if it's in a CallExpression
                return true;
            default:
                return false;
        }
    }

    bool isUnaryExpressionKind(SyntaxKind kind) {
        switch (kind) {
            case SyntaxKind::PrefixUnaryExpression:
            case SyntaxKind::PostfixUnaryExpression:
            case SyntaxKind::DeleteExpression:
            case SyntaxKind::TypeOfExpression:
            case SyntaxKind::VoidExpression:
            case SyntaxKind::AwaitExpression:
            case SyntaxKind::TypeAssertionExpression:
                return true;
            default:
                return isLeftHandSideExpressionKind(kind);
        }
    }

    /* @internal */
    bool isUnaryExpression(shared<Node> node) {
        return isUnaryExpressionKind(skipPartiallyEmittedExpressions(node)->kind);
    }

    int getOperatorPrecedence(SyntaxKind nodeKind, SyntaxKind operatorKind, optional<bool> hasArguments) {
        switch (nodeKind) {
            case SyntaxKind::CommaListExpression:
                return (int) OperatorPrecedence::Comma;

            case SyntaxKind::SpreadElement:
                return (int) OperatorPrecedence::Spread;

            case SyntaxKind::YieldExpression:
                return (int) OperatorPrecedence::Yield;

            case SyntaxKind::ConditionalExpression:
                return (int) OperatorPrecedence::Conditional;

            case SyntaxKind::BinaryExpression:
                switch (operatorKind) {
                    case SyntaxKind::CommaToken:
                        return (int) OperatorPrecedence::Comma;

                    case SyntaxKind::EqualsToken:
                    case SyntaxKind::PlusEqualsToken:
                    case SyntaxKind::MinusEqualsToken:
                    case SyntaxKind::AsteriskAsteriskEqualsToken:
                    case SyntaxKind::AsteriskEqualsToken:
                    case SyntaxKind::SlashEqualsToken:
                    case SyntaxKind::PercentEqualsToken:
                    case SyntaxKind::LessThanLessThanEqualsToken:
                    case SyntaxKind::GreaterThanGreaterThanEqualsToken:
                    case SyntaxKind::GreaterThanGreaterThanGreaterThanEqualsToken:
                    case SyntaxKind::AmpersandEqualsToken:
                    case SyntaxKind::CaretEqualsToken:
                    case SyntaxKind::BarEqualsToken:
                    case SyntaxKind::BarBarEqualsToken:
                    case SyntaxKind::AmpersandAmpersandEqualsToken:
                    case SyntaxKind::QuestionQuestionEqualsToken:
                        return (int) OperatorPrecedence::Assignment;

                    default:
                        return getBinaryOperatorPrecedence(operatorKind);
                }

                // TODO: Should prefix `++` and `--` be moved to the `Update` precedence?
            case SyntaxKind::TypeAssertionExpression:
            case SyntaxKind::NonNullExpression:
            case SyntaxKind::PrefixUnaryExpression:
            case SyntaxKind::TypeOfExpression:
            case SyntaxKind::VoidExpression:
            case SyntaxKind::DeleteExpression:
            case SyntaxKind::AwaitExpression:
                return (int) OperatorPrecedence::Unary;

            case SyntaxKind::PostfixUnaryExpression:
                return (int) OperatorPrecedence::Update;

            case SyntaxKind::CallExpression:
                return (int) OperatorPrecedence::LeftHandSide;

            case SyntaxKind::NewExpression:
                return hasArguments && *hasArguments ? (int) OperatorPrecedence::Member : (int) OperatorPrecedence::LeftHandSide;

            case SyntaxKind::TaggedTemplateExpression:
            case SyntaxKind::PropertyAccessExpression:
            case SyntaxKind::ElementAccessExpression:
            case SyntaxKind::MetaProperty:
                return (int) OperatorPrecedence::Member;

            case SyntaxKind::AsExpression:
                return (int) OperatorPrecedence::Relational;

            case SyntaxKind::ThisKeyword:
            case SyntaxKind::SuperKeyword:
            case SyntaxKind::Identifier:
            case SyntaxKind::PrivateIdentifier:
            case SyntaxKind::NullKeyword:
            case SyntaxKind::TrueKeyword:
            case SyntaxKind::FalseKeyword:
            case SyntaxKind::NumericLiteral:
            case SyntaxKind::BigIntLiteral:
            case SyntaxKind::StringLiteral:
            case SyntaxKind::ArrayLiteralExpression:
            case SyntaxKind::ObjectLiteralExpression:
            case SyntaxKind::FunctionExpression:
            case SyntaxKind::ArrowFunction:
            case SyntaxKind::ClassExpression:
            case SyntaxKind::RegularExpressionLiteral:
            case SyntaxKind::NoSubstitutionTemplateLiteral:
            case SyntaxKind::TemplateExpression:
            case SyntaxKind::ParenthesizedExpression:
            case SyntaxKind::OmittedExpression:
            case SyntaxKind::JsxElement:
            case SyntaxKind::JsxSelfClosingElement:
            case SyntaxKind::JsxFragment:
                return (int) OperatorPrecedence::Primary;

            default:
                return (int) OperatorPrecedence::Invalid;
        }
    }

    SyntaxKind getOperator(shared<Node> expression) {
        if (expression->kind == SyntaxKind::BinaryExpression) {
            return expression->to<BinaryExpression>().operatorToken->kind;
        } else if (expression->kind == SyntaxKind::PrefixUnaryExpression) {
            return expression->to<PrefixUnaryExpression>().operatorKind;
        } else if (expression->kind == SyntaxKind::PostfixUnaryExpression) {
            return expression->to<PostfixUnaryExpression>().operatorKind;
        } else {
            return expression->kind;
        }
    }

    /* @internal */
    bool isGeneratedIdentifier(shared<Node> node) {
        return node->kind == SyntaxKind::Identifier && (defaultTo<int>(node->to<Identifier>().autoGenerateFlags, 0) & (int) GeneratedIdentifierFlags::KindMask) > (int) GeneratedIdentifierFlags::None;
    }

    int getExpressionPrecedence(shared<Node> expression) {
        auto operatorNode = getOperator(expression);
        bool hasArguments = expression->is<NewExpression>() ? !!expression->to<NewExpression>().arguments : false;
        return getOperatorPrecedence(expression->kind, operatorNode, hasArguments);
    }

    bool isLeftHandSideExpression(shared<Node> node) {
        ZoneScoped;
        return isLeftHandSideExpressionKind(skipPartiallyEmittedExpressions(node)->kind);
    }

    // Returns true if this node is missing from the actual source code. A 'missing' node is different
    // from 'undefined/defined'. When a node is undefined (which can happen for optional nodes
    // in the tree), it is definitely missing. However, a node may be defined, but still be
    // missing.  This happens whenever the parser knows it needs to parse something, but can't
    // get anything in the source code that it expects at that location. For example:
    //
    //          let a: ;
    //
    // Here, the Type in the Type-Annotation is not-optional (as there is a colon in the source
    // code). So the parser will attempt to parse out a type, and will create an actual node.
    // However, this node will be 'missing' in the sense that no actual source-code/tokens are
    // contained within it.
    bool nodeIsMissing(sharedOpt<Node> node) {
        if (!node) {
            return true;
        }

        return node->pos == node->end && node->pos >= 0 && node->kind != SyntaxKind::EndOfFileToken;
    }

    bool nodeIsPresent(sharedOpt<Node> node) {
        return !nodeIsMissing(node);
    }

    string getTextOfNodeFromSourceText(string &sourceText, shared<Node> node, bool includeTrivia) {
        if (nodeIsMissing(node)) {
            return "";
        }

        auto text = substr(sourceText, includeTrivia ? node->pos : skipTrivia(sourceText, node->pos), node->end);

        //we don't care about JSDoc
//        if (isJSDocTypeExpressionOrChild(node)) {
//            // strip space + asterisk at line start
//            //todo:
////            text = text.split(/\r\n|\n|\r/).map(line => trimStringStart(line.replace(/^\s*\*/, ""))).join("\n");
//        }

        return text;
    }

    int getFullWidth(shared<Node> node) {
        return node->end - node->pos;
    }

    /**
     * Remove extra underscore from escaped identifier text content.
     *
     * @param identifier The escaped identifier text.
     * @returns The unescaped identifier text.
     */
    string unescapeLeadingUnderscores(__String id) {
        return id.size() >= 3 && charCodeAt(id, 0).code == CharacterCodes::_ && charCodeAt(id, 1).code == CharacterCodes::_ && charCodeAt(id, 2).code == CharacterCodes::_ ? substr(id, 1) : id;
    }

    string idText(shared<NodeUnion(Identifier | PrivateIdentifier)> identifierOrPrivateName) {
        return unescapeLeadingUnderscores(getEscapedName(identifierOrPrivateName));
    }

    shared<Expression> getLeftmostExpression(shared<Expression> node, bool stopAtCallExpressions) {
        while (true) {
            switch (node->kind) {
                case SyntaxKind::PostfixUnaryExpression:
                    node = node->to<PostfixUnaryExpression>().operand;
                    continue;

                case SyntaxKind::BinaryExpression:
                    node = node->to<BinaryExpression>().left;
                    continue;

                case SyntaxKind::ConditionalExpression:
                    node = node->to<ConditionalExpression>().condition;
                    continue;

                case SyntaxKind::TaggedTemplateExpression:
                    node = node->to<TaggedTemplateExpression>().tag;
                    continue;

                case SyntaxKind::CallExpression:
                    if (stopAtCallExpressions) {
                        return node;
                    }
                    // falls through
                case SyntaxKind::AsExpression:
                case SyntaxKind::ElementAccessExpression:
                case SyntaxKind::PropertyAccessExpression:
                case SyntaxKind::NonNullExpression:
                case SyntaxKind::PartiallyEmittedExpression:
                    node = getExpression(node);
                    continue;
            }

            return node;
        }
    }

    Comparison compareComparableValues(optional<double> a, optional<double> b) {
        return a == b ? Comparison::EqualTo :
               a == false ? Comparison::LessThan :
               b == false ? Comparison::GreaterThan :
               a < b ? Comparison::LessThan :
               Comparison::GreaterThan;
    }

    Comparison compareComparableValues(optional<string> a, optional<string> b) {
        if (!a) return Comparison::LessThan;
        if (!b) return Comparison::GreaterThan;

        return compareComparableValues(std::stod(*a), std::stod(*b));
    }

    /**
     * Compare two numeric values for their order relative to each other.
     * To compare strings, use any of the `compareStrings` functions.
     */
    Comparison compareValues(optional<double> a, optional<double> b) {
        return compareComparableValues(a, b);
    }
}

