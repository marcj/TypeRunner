#pragma once

#include <string>
#include <vector>
#include <optional>
#include "utf.h"
#include "core.h"
#include "types.h"
#include "diagnostic_messages.h"
#include "node_test.h"
#include "scanner.h"

namespace ts {
    using namespace ts::types;
    using std::vector;
    using std::string;
    using std::optional;
    using ts::types::ScriptKind;
    using ts::types::SyntaxKind;
    using ts::types::Extension;
    using ts::types::LanguageVariant;
    using types::DiagnosticMessage;
    using types::DiagnosticCategory;
    using types::DiagnosticWithDetachedLocation;

    inline vector<const char *> supportedDeclarationExtensions{Extension::Dts, Extension::Dcts, Extension::Dmts};

    extern LanguageVariant getLanguageVariant(ScriptKind scriptKind);

    /* @internal */
    template<typename T>
    T setTextRangePosEnd(T range, int pos, int end) {
        range->pos = pos;
        range->end = end;
        return range;
    }

    template<typename T, typename T2>
    T setTextRange(T range, T2 location) {
        return location ? setTextRangePosEnd<T>(range, location->pos, location->end) : range;
    }

    sharedOpt<Node> lastOrUndefined(sharedOpt<NodeArray> array);

    NodeArray &setTextRangePosEnd(NodeArray &range, int pos, int end);

    /**
     * Gets a custom text range to use when emitting source maps.
     */
    shared<SourceMapRange> getSourceMapRange(shared<Node> node);

    /**
     * Gets a custom text range to use when emitting comments.
     */
    shared<TextRange> getCommentRange(shared<Node> node);

    optional<vector<SynthesizedComment>> getSyntheticLeadingComments(shared<Node> node);

    optional<vector<SynthesizedComment>> getSyntheticTrailingComments(shared<Node> node);

    bool positionIsSynthesized(int pos);

    bool nodeIsSynthesized(shared<TextRange> range);

    NodeArray setTextRange(NodeArray range, optional<NodeArray> location);

    string parsePseudoBigInt(string &stringValue);

    /** Add an extra underscore to identifiers that start with two underscores to avoid issues with magic names like '__proto__' */
    string escapeLeadingUnderscores(string identifier);

    /* @internal */
    bool isParameterPropertyModifier(SyntaxKind kind);

    /* @internal */
    bool isClassMemberModifier(SyntaxKind idToken);

    /* @internal */
    bool isModifierKind(SyntaxKind token);

    bool isCommaSequence(shared<Expression> node);

    bool isLiteralKind(SyntaxKind kind);

    string pseudoBigIntToString(PseudoBigInt bigint);
    // Pseudo-literals

    /* @internal */
    bool isTemplateLiteralKind(SyntaxKind kind);

    bool isTemplateLiteralToken(shared<Node> &node);

    bool isTemplateMiddleOrTemplateTail(shared<Node> &node);

    bool isImportOrExportSpecifier(shared<Node> &node);

    enum class OperatorPrecedence {
        // Expression:
        //     AssignmentExpression
        //     Expression `,` AssignmentExpression
        Comma,

        // NOTE: `Spread` is higher than `Comma` due to how it is parsed in |ElementList|
        // SpreadElement:
        //     `...` AssignmentExpression
        Spread,

        // AssignmentExpression:
        //     ConditionalExpression
        //     YieldExpression
        //     ArrowFunction
        //     AsyncArrowFunction
        //     LeftHandSideExpression `=` AssignmentExpression
        //     LeftHandSideExpression AssignmentOperator AssignmentExpression
        //
        // NOTE: AssignmentExpression is broken down into several precedences due to the requirements
        //       of the parenthesizer rules.

        // AssignmentExpression: YieldExpression
        // YieldExpression:
        //     `yield`
        //     `yield` AssignmentExpression
        //     `yield` `*` AssignmentExpression
        Yield,

        // AssignmentExpression: LeftHandSideExpression `=` AssignmentExpression
        // AssignmentExpression: LeftHandSideExpression AssignmentOperator AssignmentExpression
        // AssignmentOperator: one of
        //     `*=` `/=` `%=` `+=` `-=` `<<=` `>>=` `>>>=` `&=` `^=` `|=` `**=`
        Assignment,

        // NOTE: `Conditional` is considered higher than `Assignment` here, but in reality they have
        //       the same precedence.
        // AssignmentExpression: ConditionalExpression
        // ConditionalExpression:
        //     ShortCircuitExpression
        //     ShortCircuitExpression `?` AssignmentExpression `:` AssignmentExpression
        // ShortCircuitExpression:
        //     LogicalORExpression
        //     CoalesceExpression
        Conditional,

        // CoalesceExpression:
        //     CoalesceExpressionHead `??` BitwiseORExpression
        // CoalesceExpressionHead:
        //     CoalesceExpression
        //     BitwiseORExpression
        Coalesce = Conditional, // NOTE: This is wrong

        // LogicalORExpression:
        //     LogicalANDExpression
        //     LogicalORExpression `||` LogicalANDExpression
        LogicalOR,

        // LogicalANDExpression:
        //     BitwiseORExpression
        //     LogicalANDExprerssion `&&` BitwiseORExpression
        LogicalAND,

        // BitwiseORExpression:
        //     BitwiseXORExpression
        //     BitwiseORExpression `^` BitwiseXORExpression
        BitwiseOR,

        // BitwiseXORExpression:
        //     BitwiseANDExpression
        //     BitwiseXORExpression `^` BitwiseANDExpression
        BitwiseXOR,

        // BitwiseANDExpression:
        //     EqualityExpression
        //     BitwiseANDExpression `^` EqualityExpression
        BitwiseAND,

        // EqualityExpression:
        //     RelationalExpression
        //     EqualityExpression `==` RelationalExpression
        //     EqualityExpression `!=` RelationalExpression
        //     EqualityExpression `===` RelationalExpression
        //     EqualityExpression `!==` RelationalExpression
        Equality,

        // RelationalExpression:
        //     ShiftExpression
        //     RelationalExpression `<` ShiftExpression
        //     RelationalExpression `>` ShiftExpression
        //     RelationalExpression `<=` ShiftExpression
        //     RelationalExpression `>=` ShiftExpression
        //     RelationalExpression `instanceof` ShiftExpression
        //     RelationalExpression `in` ShiftExpression
        //     [+TypeScript] RelationalExpression `as` Type
        Relational,

        // ShiftExpression:
        //     AdditiveExpression
        //     ShiftExpression `<<` AdditiveExpression
        //     ShiftExpression `>>` AdditiveExpression
        //     ShiftExpression `>>>` AdditiveExpression
        Shift,

        // AdditiveExpression:
        //     MultiplicativeExpression
        //     AdditiveExpression `+` MultiplicativeExpression
        //     AdditiveExpression `-` MultiplicativeExpression
        Additive,

        // MultiplicativeExpression:
        //     ExponentiationExpression
        //     MultiplicativeExpression MultiplicativeOperator ExponentiationExpression
        // MultiplicativeOperator: one of `*`, `/`, `%`
        Multiplicative,

        // ExponentiationExpression:
        //     UnaryExpression
        //     UpdateExpression `**` ExponentiationExpression
        Exponentiation,

        // UnaryExpression:
        //     UpdateExpression
        //     `delete` UnaryExpression
        //     `void` UnaryExpression
        //     `typeof` UnaryExpression
        //     `+` UnaryExpression
        //     `-` UnaryExpression
        //     `~` UnaryExpression
        //     `!` UnaryExpression
        //     AwaitExpression
        // UpdateExpression:            // TODO: Do we need to investigate the precedence here?
        //     `++` UnaryExpression
        //     `--` UnaryExpression
        Unary,

        // UpdateExpression:
        //     LeftHandSideExpression
        //     LeftHandSideExpression `++`
        //     LeftHandSideExpression `--`
        Update,

        // LeftHandSideExpression:
        //     NewExpression
        //     CallExpression
        // NewExpression:
        //     MemberExpression
        //     `new` NewExpression
        LeftHandSide,

        // CallExpression:
        //     CoverCallExpressionAndAsyncArrowHead
        //     SuperCall
        //     ImportCall
        //     CallExpression Arguments
        //     CallExpression `[` Expression `]`
        //     CallExpression `.` IdentifierName
        //     CallExpression TemplateLiteral
        // MemberExpression:
        //     PrimaryExpression
        //     MemberExpression `[` Expression `]`
        //     MemberExpression `.` IdentifierName
        //     MemberExpression TemplateLiteral
        //     SuperProperty
        //     MetaProperty
        //     `new` MemberExpression Arguments
        Member,

        // TODO: JSXElement?
        // PrimaryExpression:
        //     `this`
        //     IdentifierReference
        //     Literal
        //     ArrayLiteral
        //     ObjectLiteral
        //     FunctionExpression
        //     ClassExpression
        //     GeneratorExpression
        //     AsyncFunctionExpression
        //     AsyncGeneratorExpression
        //     RegularExpressionLiteral
        //     TemplateLiteral
        //     CoverParenthesizedExpressionAndArrowParameterList
        Primary,

        Highest = Primary,
        Lowest = Comma,
        // -1 is lower than all other precedences. Returning it will cause binary expression
        // parsing to stop.
        Invalid = -1,
    };

    int getBinaryOperatorPrecedence(SyntaxKind kind) ;

    bool isKeyword(types::SyntaxKind token);

    bool isContextualKeyword(types::SyntaxKind token) ;

    //unordered_map<string, string> localizedDiagnosticMessages{};

    string getLocaleSpecificMessage(DiagnosticMessage message);

    using DiagnosticArg = string;

    string formatStringFromArg(string &text, int i, string &v);

    string DiagnosticArgToString(DiagnosticArg &v) ;

    DiagnosticWithDetachedLocation createDetachedDiagnostic(string fileName, int start, int length, DiagnosticMessage message, vector<DiagnosticArg> textArg = {});

    DiagnosticWithDetachedLocation &addRelatedInfo(DiagnosticWithDetachedLocation &diagnostic, vector<DiagnosticRelatedInformation> relatedInformation) ;

    ScriptKind ensureScriptKind(string fileName, optional<ScriptKind> scriptKind);

    /** @internal */
    bool hasInvalidEscape(shared<NodeUnion(TemplateLiteral)> templateLiteral);

    string fileExt(const string &filename);

    ScriptKind getScriptKindFromFileName(const string &fileName);

    bool isOuterExpression(sharedOpt<Node> node, int kinds = (int) OuterExpressionKinds::All);

    sharedOpt<Expression> getExpression(sharedOpt<Node> node);

    shared<Node> skipOuterExpressions(shared<Node> node, int kinds = (int) OuterExpressionKinds::All);

    shared<Node> skipPartiallyEmittedExpressions(shared<Node> node);

    bool isLeftHandSideExpressionKind(SyntaxKind kind) ;

    bool isUnaryExpressionKind(SyntaxKind kind);

    /* @internal */
    bool isUnaryExpression(shared<Node> node);

    int getOperatorPrecedence(SyntaxKind nodeKind, SyntaxKind operatorKind, optional<bool> hasArguments = {});

    SyntaxKind getOperator(shared<Node> expression);

    /* @internal */
    bool isGeneratedIdentifier(shared<Node> node);

    int getExpressionPrecedence(shared<Node> expression);

    bool isLeftHandSideExpression(shared<Node> node);

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
    bool nodeIsMissing(sharedOpt<Node> node);

    bool nodeIsPresent(sharedOpt<Node> node);

    string getTextOfNodeFromSourceText(string &sourceText, shared<Node> node, bool includeTrivia = false);

    int getFullWidth(shared<Node> node);

    /**
     * Remove extra underscore from escaped identifier text content.
     *
     * @param identifier The escaped identifier text.
     * @returns The unescaped identifier text.
     */
    string unescapeLeadingUnderscores(__String id);

    string idText(shared<NodeUnion(Identifier | PrivateIdentifier)> identifierOrPrivateName);

    shared<Expression> getLeftmostExpression(shared<Expression> node, bool stopAtCallExpressions);

    Comparison compareComparableValues(optional<double> a, optional<double> b) ;

    Comparison compareComparableValues(optional<string> a, optional<string> b) ;

    /**
     * Compare two numeric values for their order relative to each other.
     * To compare strings, use any of the `compareStrings` functions.
     */
    Comparison compareValues(optional<double> a, optional<double> b) ;

    /**
     * Gets the actual offset into an array for a relative offset. Negative offsets indicate a
     * position offset from the end of the array.
     */
    template<typename T>
    int toOffset(vector<T> array, int offset) {
        return offset < 0 ? array.size() + offset : offset;
    }

    /**
     * Appends a range of value to an array, returning the array.
     *
     * @param to The array to which `value` is to be appended. If `to` is `undefined`, a new array
     * is created if `value` was appended.
     * @param from The values to append to the array. If `from` is `undefined`, nothing is
     * appended. If an element of `from` is `undefined`, that element is not appended.
     * @param start The offset in `from` at which to start copying values.
     * @param end The offset in `from` at which to stop copying values (non-inclusive).
     */
//    export function addRange<T>(to: T[], from: readonly T[] | undefined, start?: number, end?: number): T[];
//    export function addRange<T>(to: T[] | undefined, from: readonly T[] | undefined, start?: number, end?: number): T[] | undefined;
    template<typename T>
    optional<vector<T>> addRange(optional<vector<T>> to, optional<vector<T>> from, int start = 0, int end = 0) {
        if (!from || from->empty()) return to;
        if (!to) return slice(from, start, end);
        start = start == 0 ? 0 : toOffset(*from, start);
        end = end == 0 ? from->size() : toOffset(*from, end);
        for (int i = start; i < end && i < from->size(); i++) {
//            if (!(bool) (*from)[i]) {
            to->push_back((*from)[i]);
//            }
        }
        return to;
    }
}

