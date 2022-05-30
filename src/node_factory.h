#pragma once

#include <utility>

#include "types.h"
#include "utilities.h"
#include "scanner.h"
#include "node_test.h"

namespace ts::factory {
    shared<ParenthesizedExpression> createParenthesizedExpression(shared<Expression> expression);
    NodeArray createNodeArray(optional<NodeArray> elements, optional<bool> hasTrailingComma = {});
    NodeArray createNodeArray(vector<shared<Node>> elements, optional<bool> hasTrailingComma = {});
    shared<ParenthesizedTypeNode> createParenthesizedType(shared<TypeNode> type);
    shared<CallExpression> updateCallExpression(shared<CallExpression> node, shared<Expression> expression, optional<NodeArray> typeArguments, NodeArray argumentsArray);
    shared<Expression> restoreOuterExpressions(sharedOpt<Expression> outerExpression, shared<Expression> innerExpression, int kinds = (int) OuterExpressionKinds::All);
}

namespace ts::parenthesizerRules {
    using namespace ts;
    using namespace ts::types;

    template<typename T>
    NodeArray map(optional<NodeArray> array, function<shared<T>(shared<T>, int)> callback) {
        NodeArray result;
        if (array) {
            auto i = 0;
            for (auto &v: array->list) {
                result.list.push_back(callback(dynamic_pointer_cast<T>(v), i++));
            }
        }
        return result;
    }

//
//    function getParenthesizeLeftSideOfBinaryForOperator(operatorKind: BinaryOperator) {
//        binaryLeftOperandParenthesizerCache ||= new Map();
//        let parenthesizerRule = binaryLeftOperandParenthesizerCache.get(operatorKind);
//        if (!parenthesizerRule) {
//            parenthesizerRule = node => parenthesizeLeftSideOfBinary(operatorKind, node);
//            binaryLeftOperandParenthesizerCache.set(operatorKind, parenthesizerRule);
//        }
//        return parenthesizerRule;
//    }
//
//    function getParenthesizeRightSideOfBinaryForOperator(operatorKind: BinaryOperator) {
//        binaryRightOperandParenthesizerCache ||= new Map();
//        let parenthesizerRule = binaryRightOperandParenthesizerCache.get(operatorKind);
//        if (!parenthesizerRule) {
//            parenthesizerRule = node => parenthesizeRightSideOfBinary(operatorKind, /*leftSide*/ {}, node);
//            binaryRightOperandParenthesizerCache.set(operatorKind, parenthesizerRule);
//        }
//        return parenthesizerRule;
//    }
//
//    /**
//     * Determines whether the operand to a BinaryExpression needs to be parenthesized.
//     *
//     * @param binaryOperator The operator for the BinaryExpression.
//     * @param operand The operand for the BinaryExpression.
//     * @param isLeftSideOfBinary A value indicating whether the operand is the left side of the
//     *                           BinaryExpression.
//     */
//    function binaryOperandNeedsParentheses(binaryOperator: SyntaxKind, operand: Expression, isLeftSideOfBinary: boolean, leftOperand: Expression | undefined) {
//        // If the operand has lower precedence, then it needs to be parenthesized to preserve the
//        // intent of the expression. For example, if the operand is `a + b` and the operator is
//        // `*`, then we need to parenthesize the operand to preserve the intended order of
//        // operations: `(a + b) * x`.
//        //
//        // If the operand has higher precedence, then it does not need to be parenthesized. For
//        // example, if the operand is `a * b` and the operator is `+`, then we do not need to
//        // parenthesize to preserve the intended order of operations: `a * b + x`.
//        //
//        // If the operand has the same precedence, then we need to check the associativity of
//        // the operator based on whether this is the left or right operand of the expression.
//        //
//        // For example, if `a / d` is on the right of operator `*`, we need to parenthesize
//        // to preserve the intended order of operations: `x * (a / d)`
//        //
//        // If `a ** d` is on the left of operator `**`, we need to parenthesize to preserve
//        // the intended order of operations: `(a ** b) ** c`
//        auto binaryOperatorPrecedence = getOperatorPrecedence(SyntaxKind::BinaryExpression, binaryOperator);
//        auto binaryOperatorAssociativity = getOperatorAssociativity(SyntaxKind::BinaryExpression, binaryOperator);
//        auto emittedOperand = skipPartiallyEmittedExpressions(operand);
//        if (!isLeftSideOfBinary && operand.kind === SyntaxKind::ArrowFunction && binaryOperatorPrecedence > OperatorPrecedence.Assignment) {
//            // We need to parenthesize arrow functions on the right side to avoid it being
//            // parsed as parenthesized expression: `a && (() => {})`
//            return true;
//        }
//        auto operandPrecedence = getExpressionPrecedence(emittedOperand);
//        switch (compareValues(operandPrecedence, binaryOperatorPrecedence)) {
//            case Comparison.LessThan:
//                // If the operand is the right side of a right-associative binary operation
//                // and is a yield expression, then we do not need parentheses.
//                if (!isLeftSideOfBinary
//                    && binaryOperatorAssociativity === Associativity.Right
//                    && operand.kind === SyntaxKind::YieldExpression) {
//                    return false;
//                }
//
//                return true;
//
//            case Comparison.GreaterThan:
//                return false;
//
//            case Comparison.EqualTo:
//                if (isLeftSideOfBinary) {
//                    // No need to parenthesize the left operand when the binary operator is
//                    // left associative:
//                    //  (a*b)/x    -> a*b/x
//                    //  (a**b)/x   -> a**b/x
//                    //
//                    // Parentheses are needed for the left operand when the binary operator is
//                    // right associative:
//                    //  (a/b)**x   -> (a/b)**x
//                    //  (a**b)**x  -> (a**b)**x
//                    return binaryOperatorAssociativity === Associativity.Right;
//                }
//                else {
//                    if (isBinaryExpression(emittedOperand)
//                        && emittedOperand.operatorToken.kind === binaryOperator) {
//                        // No need to parenthesize the right operand when the binary operator and
//                        // operand are the same and one of the following:
//                        //  x*(a*b)     => x*a*b
//                        //  x|(a|b)     => x|a|b
//                        //  x&(a&b)     => x&a&b
//                        //  x^(a^b)     => x^a^b
//                        if (operatorHasAssociativeProperty(binaryOperator)) {
//                            return false;
//                        }
//
//                        // No need to parenthesize the right operand when the binary operator
//                        // is plus (+) if both the left and right operands consist solely of either
//                        // literals of the same kind or binary plus (+) expressions for literals of
//                        // the same kind (recursively).
//                        //  "a"+(1+2)       => "a"+(1+2)
//                        //  "a"+("b"+"c")   => "a"+"b"+"c"
//                        if (binaryOperator === SyntaxKind::PlusToken) {
//                            auto leftKind = leftOperand ? getLiteralKindOfBinaryPlusOperand(leftOperand) : SyntaxKind::Unknown;
//                            if (isLiteralKind(leftKind) && leftKind === getLiteralKindOfBinaryPlusOperand(emittedOperand)) {
//                                return false;
//                            }
//                        }
//                    }
//
//                    // No need to parenthesize the right operand when the operand is right
//                    // associative:
//                    //  x/(a**b)    -> x/a**b
//                    //  x**(a**b)   -> x**a**b
//                    //
//                    // Parentheses are needed for the right operand when the operand is left
//                    // associative:
//                    //  x/(a*b)     -> x/(a*b)
//                    //  x**(a/b)    -> x**(a/b)
//                    auto operandAssociativity = getExpressionAssociativity(emittedOperand);
//                    return operandAssociativity === Associativity.Left;
//                }
//        }
//    }
//
//    /**
//     * Determines whether a binary operator is mathematically associative.
//     *
//     * @param binaryOperator The binary operator.
//     */
//    function operatorHasAssociativeProperty(binaryOperator: SyntaxKind) {
//        // The following operators are associative in JavaScript:
//        //  (a*b)*c     -> a*(b*c)  -> a*b*c
//        //  (a|b)|c     -> a|(b|c)  -> a|b|c
//        //  (a&b)&c     -> a&(b&c)  -> a&b&c
//        //  (a^b)^c     -> a^(b^c)  -> a^b^c
//        //
//        // While addition is associative in mathematics, JavaScript's `+` is not
//        // guaranteed to be associative as it is overloaded with string concatenation.
//        return binaryOperator === SyntaxKind::AsteriskToken
//            || binaryOperator === SyntaxKind::BarToken
//            || binaryOperator === SyntaxKind::AmpersandToken
//            || binaryOperator === SyntaxKind::CaretToken;
//    }
//
//    /**
//     * This function determines whether an expression consists of a homogeneous set of
//     * literal expressions or binary plus expressions that all share the same literal kind.
//     * It is used to determine whether the right-hand operand of a binary plus expression can be
//     * emitted without parentheses.
//     */
//    function getLiteralKindOfBinaryPlusOperand(node: Expression): SyntaxKind {
//        node = skipPartiallyEmittedExpressions(node);
//
//        if (isLiteralKind(node->kind)) {
//            return node->kind;
//        }
//
//        if (node->kind === SyntaxKind::BinaryExpression && (node as BinaryExpression).operatorToken.kind === SyntaxKind::PlusToken) {
//            if ((node as BinaryPlusExpression).cachedLiteralKind !== undefined) {
//                return (node as BinaryPlusExpression).cachedLiteralKind;
//            }
//
//            auto leftKind = getLiteralKindOfBinaryPlusOperand((node as BinaryExpression).left);
//            auto literalKind = isLiteralKind(leftKind)
//                && leftKind === getLiteralKindOfBinaryPlusOperand((node as BinaryExpression).right)
//                    ? leftKind
//                    : SyntaxKind::Unknown;
//
//            (node as BinaryPlusExpression).cachedLiteralKind = literalKind;
//            return literalKind;
//        }
//
//        return SyntaxKind::Unknown;
//    }
//
//    /**
//     * Wraps the operand to a BinaryExpression in parentheses if they are needed to preserve the intended
//     * order of operations.
//     *
//     * @param binaryOperator The operator for the BinaryExpression.
//     * @param operand The operand for the BinaryExpression.
//     * @param isLeftSideOfBinary A value indicating whether the operand is the left side of the
//     *                           BinaryExpression.
//     */
//    function parenthesizeBinaryOperand(binaryOperator: SyntaxKind, operand: Expression, isLeftSideOfBinary: boolean, leftOperand?: Expression) {
//        auto skipped = skipPartiallyEmittedExpressions(operand);
//
//        // If the resulting expression is already parenthesized, we do not need to do any further processing.
//        if (skipped.kind === SyntaxKind::ParenthesizedExpression) {
//            return operand;
//        }
//
//        return binaryOperandNeedsParentheses(binaryOperator, operand, isLeftSideOfBinary, leftOperand)
//            ? factory::createParenthesizedExpression(operand)
//            : operand;
//    }
//
//
//    function parenthesizeLeftSideOfBinary(binaryOperator: SyntaxKind, leftSide: Expression): Expression {
//        return parenthesizeBinaryOperand(binaryOperator, leftSide, /*isLeftSideOfBinary*/ true);
//    }
//
//    function parenthesizeRightSideOfBinary(binaryOperator: SyntaxKind, leftSide: Expression | undefined, rightSide: Expression): Expression {
//        return parenthesizeBinaryOperand(binaryOperator, rightSide, /*isLeftSideOfBinary*/ false, leftSide);
//    }

    shared<Expression> parenthesizeExpressionOfComputedPropertyName(shared<Expression> expression) {
        return isCommaSequence(expression) ? factory::createParenthesizedExpression(expression) : expression;
    }

    shared<Expression> parenthesizeConditionOfConditionalExpression(shared<Expression> condition) {
        auto conditionalPrecedence = getOperatorPrecedence(SyntaxKind::ConditionalExpression, SyntaxKind::QuestionToken);
        auto emittedCondition = skipPartiallyEmittedExpressions(condition);
        auto conditionPrecedence = getExpressionPrecedence(emittedCondition);
        if (compareValues(conditionPrecedence, conditionalPrecedence) != Comparison::GreaterThan) {
            return factory::createParenthesizedExpression(condition);
        }
        return condition;
    }

    shared<Expression> parenthesizeBranchOfConditionalExpression(shared<Expression> branch) {
        // per ES grammar both 'whenTrue' and 'whenFalse' parts of conditional expression are assignment expressions
        // so in case when comma expression is introduced as a part of previous transformations
        // if should be wrapped in parens since comma operator has the lowest precedence
        auto emittedExpression = skipPartiallyEmittedExpressions(branch);
        return isCommaSequence(emittedExpression)
            ? factory::createParenthesizedExpression(branch)
            : branch;
    }

//    /**
//     *  [Per the spec](https://tc39.github.io/ecma262/#prod-ExportDeclaration), `export default` accepts _AssigmentExpression_ but
//     *  has a lookahead restriction for `function`, `async function`, and `class`.
//     *
//     * Basically, that means we need to parenthesize in the following cases:
//     *
//     * - BinaryExpression of CommaToken
//     * - CommaList (synthetic list of multiple comma expressions)
//     * - FunctionExpression
//     * - ClassExpression
//     */
//    function parenthesizeExpressionOfExportDefault(shared<Expression> expression): Expression {
//        auto check = skipPartiallyEmittedExpressions(expression);
//        let needsParens = isCommaSequence(check);
//        if (!needsParens) {
//            switch (getLeftmostExpression(check, /*stopAtCallExpression*/ false).kind) {
//                case SyntaxKind::ClassExpression:
//                case SyntaxKind::FunctionExpression:
//                    needsParens = true;
//            }
//        }
//        return needsParens ? factory::createParenthesizedExpression(expression) : expression;
//    }
//

    /**
     * Wraps an expression in parentheses if it is needed in order to use the expression for
     * property or element access.
     */
    shared<LeftHandSideExpression> parenthesizeLeftSideOfAccess(shared<Expression> expression) {
        // isLeftHandSideExpression is almost the correct criterion for when it is not necessary
        // to parenthesize the expression before a dot. The known exception is:
        //
        //    NewExpression:
        //       new C.x        -> not the same as (new C).x
        //
        auto emittedExpression = skipPartiallyEmittedExpressions(expression);
        if (isLeftHandSideExpression(emittedExpression)
            && (emittedExpression->kind != SyntaxKind::NewExpression || emittedExpression->to<NewExpression>().arguments)) {
            // TODO(rbuckton): Verify whether this assertion holds.
            return dynamic_pointer_cast<LeftHandSideExpression>(expression);
        }

        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return setTextRange(factory::createParenthesizedExpression(expression), expression);
    }

    /**
     * Wraps an expression in parentheses if it is needed in order to use the expression
     * as the expression of a `NewExpression` node->
     */
    shared<LeftHandSideExpression> parenthesizeExpressionOfNew(shared<Expression> expression) {
        auto leftmostExpr = getLeftmostExpression(expression, /*stopAtCallExpressions*/ true);
        switch (leftmostExpr->kind) {
            case SyntaxKind::CallExpression:
                return factory::createParenthesizedExpression(expression);

            case SyntaxKind::NewExpression:
                return !leftmostExpr->to<NewExpression>().arguments
                       ? factory::createParenthesizedExpression(expression)
                       : dynamic_pointer_cast<LeftHandSideExpression>(expression); // TODO(rbuckton): Verify this assertion holds
        }

        return parenthesizeLeftSideOfAccess(expression);
    }

    shared<LeftHandSideExpression> parenthesizeOperandOfPostfixUnary(shared<Expression> operand) {
        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return isLeftHandSideExpression(operand) ? dynamic_pointer_cast<LeftHandSideExpression>(operand) : setTextRange(factory::createParenthesizedExpression(operand), operand);
    }

    shared<UnaryExpression> parenthesizeOperandOfPrefixUnary(shared<Expression> operand) {
        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return isUnaryExpression(operand) ? dynamic_pointer_cast<UnaryExpression>(operand) : setTextRange(factory::createParenthesizedExpression(operand), operand);
    }

    shared<Expression> parenthesizeExpressionForDisallowedComma(shared<Expression> expression, int = 0) {
        auto emittedExpression = skipPartiallyEmittedExpressions(expression);
        auto expressionPrecedence = getExpressionPrecedence(emittedExpression);
        auto commaPrecedence = getOperatorPrecedence(SyntaxKind::BinaryExpression, SyntaxKind::CommaToken);
        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return expressionPrecedence > commaPrecedence ? expression : setTextRange(factory::createParenthesizedExpression(expression), expression);
    }

    NodeArray parenthesizeExpressionsOfCommaDelimitedList(NodeArray elements) {
        auto result = map < Expression > (elements, parenthesizeExpressionForDisallowedComma);
        return setTextRange(factory::createNodeArray(result, elements.hasTrailingComma), elements);
    }

    shared<Expression> parenthesizeExpressionOfExpressionStatement(shared<Expression> expression) {
        auto emittedExpression = skipPartiallyEmittedExpressions(expression);
        if (auto e = to<CallExpression>(emittedExpression)) {
            auto callee = e->expression;
            auto kind = skipPartiallyEmittedExpressions(callee)->kind;
            if (kind == SyntaxKind::FunctionExpression || kind == SyntaxKind::ArrowFunction) {
                // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
                auto updated = factory::updateCallExpression(
                        e,
                        setTextRange(factory::createParenthesizedExpression(callee), callee),
                        e->typeArguments,
                        e->arguments
                );
                return factory::restoreOuterExpressions(expression, updated, (int) OuterExpressionKinds::PartiallyEmittedExpressions);
            }
        }

        auto leftmostExpressionKind = getLeftmostExpression(to<Expression>(emittedExpression), /*stopAtCallExpressions*/ false)->kind;
        if (leftmostExpressionKind == SyntaxKind::ObjectLiteralExpression || leftmostExpressionKind == SyntaxKind::FunctionExpression) {
            // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
            return setTextRange(factory::createParenthesizedExpression(expression), expression);
        }

        return expression;
    }

//    function parenthesizeConciseBodyOfArrowFunction(body: Expression): Expression;
//    function parenthesizeConciseBodyOfArrowFunction(body: ConciseBody): ConciseBody;
    shared<Node> parenthesizeConciseBodyOfArrowFunction(shared<Node> body) {
        if (isBlock(body)) return body;
        auto e = dynamic_pointer_cast<Expression>(body);
        if (isCommaSequence(body) || getLeftmostExpression(e, /*stopAtCallExpressions*/ false)->kind == SyntaxKind::ObjectLiteralExpression) {
            // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
            return setTextRange(factory::createParenthesizedExpression(e), body);
        }

        return body;
    }

    // Type[Extends] :
    //     FunctionOrConstructorType
    //     ConditionalType[?Extends]

    // ConditionalType[Extends] :
    //     UnionType[?Extends]
    //     [~Extends] UnionType[~Extends] `extends` Type[+Extends] `?` Type[~Extends] `:` Type[~Extends]
    //
    // - The check type (the `UnionType`, above) does not allow function, constructor, or conditional types (they must be parenthesized)
    // - The extends type (the first `Type`, above) does not allow conditional types (they must be parenthesized). Function and constructor types are fine.
    // - The true and false branch types (the second and third `Type` non-terminals, above) allow any type
    shared<TypeNode> parenthesizeCheckTypeOfConditionalType(shared<TypeNode> checkType) {
        switch (checkType->kind) {
            case SyntaxKind::FunctionType:
            case SyntaxKind::ConstructorType:
            case SyntaxKind::ConditionalType:
                return factory::createParenthesizedType(checkType);
        }
        return checkType;
    }

    shared<TypeNode> parenthesizeExtendsTypeOfConditionalType(shared<TypeNode> extendsType) {
        switch (extendsType->kind) {
            case SyntaxKind::ConditionalType:
                return factory::createParenthesizedType(extendsType);
        }
        return extendsType;
    }

    // UnionType[Extends] :
    //     `|`? IntersectionType[?Extends]
    //     UnionType[?Extends] `|` IntersectionType[?Extends]
    //
    // - A union type constituent has the same precedence as the check type of a conditional type
    shared<TypeNode> parenthesizeConstituentTypeOfUnionType(shared<TypeNode> type, int = 0) {
        switch (type->kind) {
            case SyntaxKind::UnionType: // Not strictly necessary, but a union containing a union should have been flattened
            case SyntaxKind::IntersectionType: // Not strictly necessary, but makes generated output more readable and avoids breaks in DT tests
                return factory::createParenthesizedType(type);
        }
        return parenthesizeCheckTypeOfConditionalType(type);
    }

    NodeArray parenthesizeConstituentTypesOfUnionType(NodeArray members) {
        return factory::createNodeArray(map < TypeNode > (members, parenthesizeConstituentTypeOfUnionType));
    }

    // IntersectionType[Extends] :
    //     `&`? TypeOperator[?Extends]
    //     IntersectionType[?Extends] `&` TypeOperator[?Extends]
    //
    // - An intersection type constituent does not allow function, constructor, conditional, or union types (they must be parenthesized)
    shared<TypeNode> parenthesizeConstituentTypeOfIntersectionType(shared<TypeNode> type, int = 0) {
        switch (type->kind) {
            case SyntaxKind::UnionType:
            case SyntaxKind::IntersectionType: // Not strictly necessary, but an intersection containing an intersection should have been flattened
                return factory::createParenthesizedType(type);
        }
        return parenthesizeConstituentTypeOfUnionType(type);
    }

    NodeArray parenthesizeConstituentTypesOfIntersectionType(NodeArray members) {
        return factory::createNodeArray(map < TypeNode > (members, parenthesizeConstituentTypeOfIntersectionType));
    }

    // TypeOperator[Extends] :
    //     PostfixType
    //     InferType[?Extends]
    //     `keyof` TypeOperator[?Extends]
    //     `unique` TypeOperator[?Extends]
    //     `readonly` TypeOperator[?Extends]
    //
    shared<TypeNode> parenthesizeOperandOfTypeOperator(shared<TypeNode> type) {
        switch (type->kind) {
            case SyntaxKind::IntersectionType:
                return factory::createParenthesizedType(type);
        }
        return parenthesizeConstituentTypeOfIntersectionType(type);
    }

    shared<TypeNode> parenthesizeOperandOfReadonlyTypeOperator(shared<TypeNode> type) {
        switch (type->kind) {
            case SyntaxKind::TypeOperator:
                return factory::createParenthesizedType(type);
        }
        return parenthesizeOperandOfTypeOperator(type);
    }

    // PostfixType :
    //     NonArrayType
    //     NonArrayType [no LineTerminator here] `!` // JSDoc
    //     NonArrayType [no LineTerminator here] `?` // JSDoc
    //     IndexedAccessType
    //     ArrayType
    //
    // IndexedAccessType :
    //     NonArrayType `[` Type[~Extends] `]`
    //
    // ArrayType :
    //     NonArrayType `[` `]`
    //
    shared<TypeNode> parenthesizeNonArrayTypeOfPostfixType(shared<TypeNode> type) {
        switch (type->kind) {
            case SyntaxKind::InferType:
            case SyntaxKind::TypeOperator:
            case SyntaxKind::TypeQuery: // Not strictly necessary, but makes generated output more readable and avoids breaks in DT tests
                return factory::createParenthesizedType(type);
        }
        return parenthesizeOperandOfTypeOperator(type);
    }

    shared<TypeNode> parenthesizeElementTypeOfTupleType(shared<TypeNode> type, int = 0) {
//        if (hasJSDocPostfixQuestion(type)) return factory::createParenthesizedType(type);
        return type;
    }

    // TupleType :
    //     `[` Elision? `]`
    //     `[` NamedTupleElementTypes `]`
    //     `[` NamedTupleElementTypes `,` Elision? `]`
    //     `[` TupleElementTypes `]`
    //     `[` TupleElementTypes `,` Elision? `]`
    //
    // NamedTupleElementTypes :
    //     Elision? NamedTupleMember
    //     NamedTupleElementTypes `,` Elision? NamedTupleMember
    //
    // NamedTupleMember :
    //     Identifier `?`? `:` Type[~Extends]
    //     `...` Identifier `:` Type[~Extends]
    //
    // TupleElementTypes :
    //     Elision? TupleElementType
    //     TupleElementTypes `,` Elision? TupleElementType
    //
    // TupleElementType :
    //     Type[~Extends] // NOTE: Needs cover grammar to disallow JSDoc postfix-optional
    //     OptionalType
    //     RestType
    //
    // OptionalType :
    //     Type[~Extends] `?` // NOTE: Needs cover grammar to disallow JSDoc postfix-optional
    //
    // RestType :
    //     `...` Type[~Extends]
    //
    NodeArray parenthesizeElementTypesOfTupleType(NodeArray types) {
        return factory::createNodeArray(map < TypeNode > (types, parenthesizeElementTypeOfTupleType));
    }

//    function hasJSDocPostfixQuestion(shared<TypeNode> type | NamedTupleMember): boolean {
//        if (isJSDocNullableType(type)) return type.postfix;
//        if (isNamedTupleMember(type)) return hasJSDocPostfixQuestion(type.type);
//        if (isFunctionTypeNode(type) || isConstructorTypeNode(type) || isTypeOperatorNode(type)) return hasJSDocPostfixQuestion(type.type);
//        if (isConditionalTypeNode(type)) return hasJSDocPostfixQuestion(type.falseType);
//        if (isUnionTypeNode(type)) return hasJSDocPostfixQuestion(last(type.types));
//        if (isIntersectionTypeNode(type)) return hasJSDocPostfixQuestion(last(type.types));
//        if (isInferTypeNode(type)) return !!type.typeParameter.constraint && hasJSDocPostfixQuestion(type.typeParameter.constraint);
//        return false;
//    }
//
//    function parenthesizeTypeOfOptionalType(shared<TypeNode> type): TypeNode {
//        if (hasJSDocPostfixQuestion(type)) return factory::createParenthesizedType(type);
//        return parenthesizeNonArrayTypeOfPostfixType(type);
//    }
//
//    // function parenthesizeMemberOfElementType(member: TypeNode): TypeNode {
//    //     switch (member.kind) {
//    //         case SyntaxKind::UnionType:
//    //         case SyntaxKind::IntersectionType:
//    //         case SyntaxKind::FunctionType:
//    //         case SyntaxKind::ConstructorType:
//    //             return factory::createParenthesizedType(member);
//    //     }
//    //     return parenthesizeMemberOfConditionalType(member);
//    // }
//
//    // function parenthesizeElementTypeOfArrayType(member: TypeNode): TypeNode {
//    //     switch (member.kind) {
//    //         case SyntaxKind::TypeQuery:
//    //         case SyntaxKind::TypeOperator:
//    //         case SyntaxKind::InferType:
//    //             return factory::createParenthesizedType(member);
//    //     }
//    //     return parenthesizeMemberOfElementType(member);
//    // }
//
    shared<TypeNode> parenthesizeLeadingTypeArgument(shared<TypeNode> node) {
        return isFunctionOrConstructorTypeNode(node) && getTypeParameters(node) ? factory::createParenthesizedType(node) : node;
    }

    shared<TypeNode> parenthesizeOrdinalTypeArgument(shared<TypeNode> node, int i) {
        return i == 0 ? parenthesizeLeadingTypeArgument(node) : node;
    }

    optional<NodeArray> parenthesizeTypeArguments(optional<NodeArray> typeArguments) {
        if (typeArguments && (*typeArguments).empty()) {
            auto m = map < TypeNode > (typeArguments, parenthesizeOrdinalTypeArgument);
            return factory::createNodeArray(m);
        }
    }
}

namespace ts::factory {
    int nextAutoGenerateId = 0;

    using ts::types::TransformFlags;

    TransformFlags getTransformFlagsSubtreeExclusions(SyntaxKind kind);

    /* @internal */
    enum class NodeFactoryFlags {
        None = 0,
        // Disables the parenthesizer rules for the factory::
        NoParenthesizerRules = 1 << 0,
        // Disables the node converters for the factory::
        NoNodeConverters = 1 << 1,
        // Ensures new `PropertyAccessExpression` nodes are created with the `NoIndentation` emit flag set.
        NoIndentationOnFreshPropertyAccess = 1 << 2,
        // Do not set an `original` pointer when updating a node->
        NoOriginalNode = 1 << 3,
    };

    int propagatePropertyNameFlagsOfChild(shared<NodeUnion(PropertyName)> &node, int transformFlags) {
        return transformFlags | (node->transformFlags & (int) TransformFlags::PropertyNamePropagatingFlags);
    }

    int propagateChildFlags(sharedOpt<Node> child) {
        if (!child) return (int) TransformFlags::None;
        auto childFlags = child->transformFlags & ~(int) getTransformFlagsSubtreeExclusions(child->kind);

        auto name = getName(child);
        return name ? propagatePropertyNameFlagsOfChild(name, childFlags) : childFlags;
    }

    int propagateIdentifierNameFlags(shared<Node> node) {
        // An IdentifierName is allowed to be `await`
        return propagateChildFlags(std::move(node)) & ~(int) TransformFlags::ContainsPossibleTopLevelAwait;
    }

    int propagateChildrenFlags(optional<NodeArray> children) {
        return children ? children->transformFlags : (int) TransformFlags::None;
    }

    void aggregateChildrenFlags(NodeArray &children) {
        int subtreeFlags = (int) TransformFlags::None;
        for (auto &child: children.list) {
            subtreeFlags |= propagateChildFlags(child);
        }
        children.transformFlags = subtreeFlags;
    }

    NodeArray createNodeArray(optional<NodeArray> elements, optional<bool> hasTrailingComma) {
        if (elements) {
            if (!hasTrailingComma || elements->hasTrailingComma == hasTrailingComma) {
                // Ensure the transform flags have been aggregated for this NodeArray
                if (elements->transformFlags == (int) types::TransformFlags::None) {
                    aggregateChildrenFlags(*elements);
                }
//                Debug.attachNodeArrayDebugInfo(elements);
                return *elements;
            }
        }

        // This *was* a `NodeArray`, but the `hasTrailingComma` option differs. Recreate the
        // array with the same elements, text range, and transform flags but with the updated
        // value for `hasTrailingComma`
        NodeArray array;
        if (elements) array = *elements;
        array.pos = elements->pos;
        array.end = elements->end;
        array.hasTrailingComma = *hasTrailingComma;
        array.transformFlags = elements->transformFlags;
//            Debug.attachNodeArrayDebugInfo(array);
        return array;
    }

    optional<NodeArray> asNodeArray(optional<NodeArray> array) {
        return array;
    }

    template<class T>
    NodeArray asNodeArray(vector<shared<T>> array) {
        NodeArray nodeArray;
        for (auto node: array) nodeArray.list.push_back(node);
        return nodeArray;
    }

    template<class T>
    optional<NodeArray> asNodeArray(optional<vector<shared<T>>> array) {
        if (!array) return nullopt;

        return asNodeArray(*array);
    }

    // @api
    NodeArray createNodeArray(vector<shared<Node>> elements, optional<bool> hasTrailingComma) {
        // Since the element list of a node array is typically created by starting with an empty array and
        // repeatedly calling push(), the list may not have the optimal memory layout. We invoke slice() for
        // small arrays (1 to 4 elements) to give the VM a chance to allocate an optimal representation.
        return createNodeArray(asNodeArray(elements), hasTrailingComma);
    }

    template<class T>
    shared<T> createBaseNode() {
        auto node = make_shared<T>();
        node->kind = (types::SyntaxKind) T::KIND;
        return node;
    }

    template<class T>
    shared<T> createBaseNode(SyntaxKind kind) {
        auto node = make_shared<T>();
        node->kind = kind;
        return node;
    }

    template<class T>
    shared<T> createBaseToken(SyntaxKind kind) {
        return createBaseNode<T>(kind);
    }

    //
    // Literals
    //
    template<typename T>
    shared<T> createBaseLiteral(SyntaxKind kind, string text) {
        auto node = createBaseToken<T>(kind);
        node->text = text;
        return node;
    }

    // @api
    shared<NumericLiteral> createNumericLiteral(string value, int numericLiteralFlags = (int) types::TokenFlags::None) {
        auto node = createBaseLiteral<NumericLiteral>(SyntaxKind::NumericLiteral, std::move(value));
        node->numericLiteralFlags = numericLiteralFlags;
        if (numericLiteralFlags & TokenFlags::BinaryOrOctalSpecifier) node->transformFlags |= (int) TransformFlags::ContainsES2015;
        return node;
    }

    shared<NumericLiteral> createNumericLiteral(double value, types::TokenFlags numericLiteralFlags = types::TokenFlags::None) {
        return createNumericLiteral(std::to_string(value), numericLiteralFlags);
    }

    // @api
    shared<BigIntLiteral> createBigIntLiteral(variant<string, PseudoBigInt> value) {
        string v = holds_alternative<string>(value) ? get<string>(value) : pseudoBigIntToString(get<PseudoBigInt>(value)) + "n";
        auto node = createBaseLiteral<BigIntLiteral>(SyntaxKind::BigIntLiteral, v);
        node->transformFlags |= (int) TransformFlags::ContainsESNext;
        return node;
    }

    shared<StringLiteral> createBaseStringLiteral(string text, optional<bool> isSingleQuote = {}) {
        auto node = createBaseLiteral<StringLiteral>(SyntaxKind::StringLiteral, std::move(text));
        node->singleQuote = isSingleQuote;
        return node;
    }
    // @api
    shared<StringLiteral> createStringLiteral(string text, optional<bool> isSingleQuote = {}, optional<bool> hasExtendedUnicodeEscape = {}) {
        auto node = createBaseStringLiteral(std::move(text), isSingleQuote);
        node->hasExtendedUnicodeEscape = hasExtendedUnicodeEscape;
        if (hasExtendedUnicodeEscape) node->transformFlags |= (int) TransformFlags::ContainsES2015;
        return node;
    }

//        // @api
//        function createToken(token: SyntaxKind::SuperKeyword): SuperExpression;
//        function createToken(token: SyntaxKind::ThisKeyword): ThisExpression;
//        function createToken(token: SyntaxKind::NullKeyword): NullLiteral;
//        function createToken(token: SyntaxKind::TrueKeyword): TrueLiteral;
//        function createToken(token: SyntaxKind::FalseKeyword): FalseLiteral;
//        function createToken<TKind extends PunctuationSyntaxKind>(token: TKind): PunctuationToken<TKind>;
//        function createToken<TKind extends KeywordTypeSyntaxKind>(token: TKind): KeywordTypeNode<TKind>;
//        function createToken<TKind extends ModifierSyntaxKind>(token: TKind): ModifierToken<TKind>;
//        function createToken<TKind extends KeywordSyntaxKind>(token: TKind): KeywordToken<TKind>;
//        function createToken<TKind extends SyntaxKind::Unknown | SyntaxKind::EndOfFileToken>(token: TKind): Token<TKind>;
//        function createToken<TKind extends SyntaxKind>(token: TKind): Token<TKind>;
    template<class T>
    shared<T> createToken(SyntaxKind token) {
//        Debug::asserts(token >= SyntaxKind::FirstToken && token <= SyntaxKind::LastToken, "Invalid token");
//        Debug::asserts(token <= SyntaxKind::FirstTemplateToken || token >= SyntaxKind::LastTemplateToken, "Invalid token. Use 'createTemplateLiteralLikeNode' to create template literals.");
//        Debug::asserts(token <= SyntaxKind::FirstLiteralToken || token >= SyntaxKind::LastLiteralToken, "Invalid token. Use 'createLiteralLikeNode' to create literals.");
//        Debug::asserts(token != SyntaxKind::Identifier, "Invalid token. Use 'createIdentifier' to create identifiers");
        auto node = createBaseToken<T>(token);
        int transformFlags = (int) TransformFlags::None;
        switch (token) {
            case SyntaxKind::AsyncKeyword:
                // 'async' modifier is ES2017 (async functions) or ES2018 (async generators)
                transformFlags = (int) TransformFlags::ContainsES2017 | (int) TransformFlags::ContainsES2018;
                break;

            case SyntaxKind::PublicKeyword:
            case SyntaxKind::PrivateKeyword:
            case SyntaxKind::ProtectedKeyword:
            case SyntaxKind::ReadonlyKeyword:
            case SyntaxKind::AbstractKeyword:
            case SyntaxKind::DeclareKeyword:
            case SyntaxKind::ConstKeyword:
            case SyntaxKind::AnyKeyword:
            case SyntaxKind::NumberKeyword:
            case SyntaxKind::BigIntKeyword:
            case SyntaxKind::NeverKeyword:
            case SyntaxKind::ObjectKeyword:
            case SyntaxKind::InKeyword:
            case SyntaxKind::OutKeyword:
            case SyntaxKind::OverrideKeyword:
            case SyntaxKind::StringKeyword:
            case SyntaxKind::BooleanKeyword:
            case SyntaxKind::SymbolKeyword:
            case SyntaxKind::VoidKeyword:
            case SyntaxKind::UnknownKeyword:
            case SyntaxKind::UndefinedKeyword: // `undefined` is an Identifier in the expression case.
                transformFlags = (int) TransformFlags::ContainsTypeScript;
                break;
            case SyntaxKind::SuperKeyword:
                transformFlags = (int) TransformFlags::ContainsES2015 | (int) TransformFlags::ContainsLexicalSuper;
                break;
            case SyntaxKind::StaticKeyword:
                transformFlags = (int) TransformFlags::ContainsES2015;
                break;
            case SyntaxKind::ThisKeyword:
                // 'this' indicates a lexical 'this'
                transformFlags = (int) TransformFlags::ContainsLexicalThis;
                break;
        }
        if (transformFlags) {
            node->transformFlags |= transformFlags;
        }
        return node;
    }

    //
    // Reserved words
    //

    // @api
    shared<SuperExpression> createSuper() {
        return createToken<SuperExpression>(SyntaxKind::SuperKeyword);
    }

    // @api
    shared<ThisExpression> createThis() {
        return createToken<ThisExpression>(SyntaxKind::ThisKeyword);
    }

    // @api
    shared<NullLiteral> createNull() {
        return createToken<NullLiteral>(SyntaxKind::NullKeyword);
    }

    // @api
    shared<TrueLiteral> createTrue() {
        return createToken<TrueLiteral>(SyntaxKind::TrueKeyword);
    }

    // @api
    shared<FalseLiteral> createFalse() {
        return createToken<FalseLiteral>(SyntaxKind::FalseKeyword);
    }

    shared<Expression> createBooleanLiteral(bool v) {
        if (v) return createTrue();
        return createFalse();
    }

    shared<Identifier> createIdentifier(string text, optional<NodeArray> typeArguments = {}, optional<SyntaxKind> originalKeywordKind = {});

    using NameType = variant<string, shared<Node>>;

    shared<Node> asName(NameType name = {}) {
        if (holds_alternative<string>(name)) return createIdentifier(get<string>(name));
        return get<shared<Node>>(name);
    }

    sharedOpt<Node> asName(optional<NameType> name = {}) {
        if (!name) return nullptr;
        return asName(*name);
    }

    using ExpressionType = variant<string, int, bool, sharedOpt<Expression>>;

    sharedOpt<Expression> asExpression(ExpressionType value) {
        if (holds_alternative<sharedOpt<Expression>>(value)) return get<sharedOpt<Expression>>(value);
        if (holds_alternative<string>(value)) return createStringLiteral(get<string>(value));
        if (holds_alternative<int>(value)) return createNumericLiteral(get<int>(value));
        if (holds_alternative<bool>(value)) return createBooleanLiteral(get<bool>(value));

        throw runtime_error("Invalid type given");
    }

//        function createBaseNode<T extends Node>(kind: T["kind"]) {
//            return basefactory::createBaseNode(kind) as Mutable<T>;
//        }
//
    template<class T>
    shared<T> createBaseDeclaration(
            SyntaxKind kind,
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers = {}
    ) {
        auto node = createBaseNode<T>(kind);
        node->decorators = asNodeArray(decorators);
        node->modifiers = asNodeArray(modifiers);
        node->transformFlags |=
                propagateChildrenFlags(node->decorators) |
                propagateChildrenFlags(node->modifiers);
        // NOTE: The following properties are commonly set by the binder and are added here to
        // ensure declarations have a stable shape.
//            node->symbol = undefined!; // initialized by binder
//            node->localSymbol = undefined; // initialized by binder
//            node->locals = undefined; // initialized by binder
//            node->nextContainer = undefined; // initialized by binder
        return node;
    }

    void setName(shared<Identifier> &lName, shared<Node> rName) {
        lName = dynamic_pointer_cast<Identifier>(rName);
    }

    void setName(shared<PrivateIdentifier> &lName, shared<Node> rName) {
        lName = dynamic_pointer_cast<PrivateIdentifier>(rName);
    }

    void setName(shared<Node> &lName, shared<Node> rName) {
        lName = rName;
    }

    template<class T>
    shared<T> updateWithoutOriginal(shared<T> updated, shared<T> original) {
        if (updated != original) {
            setTextRange(updated, original);
        }
        return updated;
    }

    template<class T>
    shared<T> update(shared<T> updated, shared<T> original) {
        return updateWithoutOriginal<T>(updated, original);
    }

    template<class T>
    shared<T> createBaseNamedDeclaration(
            SyntaxKind kind,
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            optional<NameType> _name = {}
    ) {
        auto node = createBaseDeclaration<T>(
                kind,
                decorators,
                modifiers
        );
        auto name = asName(_name);
        setName(node->name, name);

        // The PropertyName of a member is allowed to be `await`.
        // We don't need to exclude `await` for type signatures since types
        // don't propagate child flags.
        if (name) {
            switch (node->kind) {
                case SyntaxKind::MethodDeclaration:
                case SyntaxKind::GetAccessor:
                case SyntaxKind::SetAccessor:
                case SyntaxKind::PropertyDeclaration:
                case SyntaxKind::PropertyAssignment:
                    if (isIdentifier(name)) {
                        node->transformFlags |= propagateIdentifierNameFlags(name);
                        break;
                    }
                    // fall through
                default:
                    node->transformFlags |= propagateChildFlags(name);
                    break;
            }
        }
        return node;
    }

    template<class T>
    shared<T> createBaseGenericNamedDeclaration(
            SyntaxKind kind,
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NameType name,
            optional<NodeTypeArray(TypeParameterDeclaration)> typeParameters
    ) {
        auto node = createBaseNamedDeclaration<T>(
                kind,
                decorators,
                modifiers,
                name
        );
        node->typeParameters = asNodeArray(typeParameters);
        node->transformFlags |= propagateChildrenFlags(node->typeParameters);
        if (typeParameters) node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        return node;
    }

    template<class T>
    shared<T> createBaseSignatureDeclaration(
            SyntaxKind kind,
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NameType name,
            optional<NodeTypeArray(TypeParameterDeclaration)> typeParameters,
            optional<NodeTypeArray(ParameterDeclaration)> parameters,
            sharedOpt<TypeNode> type
    ) {
        auto node = createBaseGenericNamedDeclaration<T>(
                kind,
                decorators,
                modifiers,
                name,
                typeParameters
        );
        node->parameters = createNodeArray(parameters);
        node->type = type;
        node->transformFlags |=
                propagateChildrenFlags(node->parameters) |
                propagateChildFlags(node->type);
        if (type) node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        function updateBaseSignatureDeclaration<T extends SignatureDeclarationBase>(updated: Mutable<T>, original: T) {
//            // copy children used only for error reporting
//            if (original.typeArguments) updated.typeArguments = original.typeArguments;
//            return update(updated, original);
//        }

    template<class T>
    shared<T> createBaseFunctionLikeDeclaration(
            SyntaxKind kind,
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NameType name,
            optional<NodeArray> typeParameters,
            optional<NodeArray> parameters,
            sharedOpt<TypeNode> type,
            decltype(declval<T>().body) body
    ) {
        auto node = createBaseSignatureDeclaration<T>(
                kind,
                decorators,
                modifiers,
                name,
                typeParameters,
                parameters,
                type
        );
        node->body = body;
        node->transformFlags |= propagateChildFlags(node->body) & ~(int) TransformFlags::ContainsPossibleTopLevelAwait;
        if (!body) node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        function updateBaseFunctionLikeDeclaration<T extends FunctionLikeDeclaration>(updated: Mutable<T>, original: T) {
//            // copy children used only for error reporting
//            if (original.exclamationToken) updated.exclamationToken = original.exclamationToken;
//            if (original.typeArguments) updated.typeArguments = original.typeArguments;
//            return updateBaseSignatureDeclaration(updated, original);
//        }

    template<class T>
    shared<T> createBaseInterfaceOrClassLikeDeclaration(
            SyntaxKind kind,
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NameType name,
            optional<NodeArray> typeParameters,
            optional<NodeArray> heritageClauses
    ) {
        auto node = createBaseGenericNamedDeclaration<T>(
                kind,
                decorators,
                modifiers,
                name,
                typeParameters
        );
        node->heritageClauses = asNodeArray(heritageClauses);
        node->transformFlags |= propagateChildrenFlags(node->heritageClauses);
        return node;
    }

    template<class T>
    shared<T> createBaseClassLikeDeclaration(
            SyntaxKind kind,
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NameType name,
            optional<NodeArray> typeParameters,
            optional<NodeArray> heritageClauses,
            NodeArray members
    ) {
        auto node = createBaseInterfaceOrClassLikeDeclaration<T>(
                kind,
                decorators,
                modifiers,
                name,
                typeParameters,
                heritageClauses
        );
        node->members = createNodeArray(members);
        node->transformFlags |= propagateChildrenFlags(node->members);
        return node;
    }

    template<class T>
    shared<T> createBaseBindingLikeDeclaration(
            SyntaxKind kind,
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            optional<NameType> name = {},
            sharedOpt<Expression> initializer = {}
    ) {
        auto node = createBaseNamedDeclaration<T>(
                kind,
                decorators,
                modifiers,
                name
        );
        node->initializer = initializer;
        node->transformFlags |= propagateChildFlags(node->initializer);
        return node;
    }

    template<class T>
    shared<T> createBaseVariableLikeDeclaration(
            SyntaxKind kind,
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            optional<NameType> name = {},
            sharedOpt<TypeNode> type = nullptr,
            sharedOpt<Expression> initializer = nullptr
    ) {
        auto node = createBaseBindingLikeDeclaration<T>(
                kind,
                decorators,
                modifiers,
                name,
                initializer
        );
        node->type = type;
        node->transformFlags |= propagateChildFlags(type);
        if (type) node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function createStringLiteralFromNode(sourceNode: PropertyNameLiteral): StringLiteral {
//            auto node = createBaseStringLiteral(getTextOfIdentifierOrLiteral(sourceNode), /*isSingleQuote*/ undefined);
//            node->textSourceNode = sourceNode;
//            return node;
//        }
//
    // @api
    shared<RegularExpressionLiteral> createRegularExpressionLiteral(string text) {
        auto node = createBaseLiteral<RegularExpressionLiteral>(SyntaxKind::RegularExpressionLiteral, text);
        return node;
    }

    // @api
    shared<JsxText> createJsxText(string text, optional<bool> containsOnlyTriviaWhiteSpaces = {}) {
        auto node = createBaseNode<JsxText>(SyntaxKind::JsxText);
        node->text = text;
        node->containsOnlyTriviaWhiteSpaces = containsOnlyTriviaWhiteSpaces ? *containsOnlyTriviaWhiteSpaces : false;
        node->transformFlags |= (int) TransformFlags::ContainsJsx;
        return node;
    }

    // @api
    shared<TemplateLiteralLike> createTemplateLiteralLikeNode(SyntaxKind kind, string text, optional<string> rawText = {}, optional<int> templateFlags = {}) {
        auto node = createBaseToken<TemplateLiteralLike>(kind);
        node->text = std::move(text);
        node->rawText = std::move(rawText);
        node->templateFlags = (templateFlags ? *templateFlags : 0) & (int) TokenFlags::TemplateLiteralLikeFlags;
        node->transformFlags |= (int) TransformFlags::ContainsES2015;
        if (node->templateFlags) {
            node->transformFlags |= (int) TransformFlags::ContainsES2018;
        }
        return node;
    }

    // @api
    shared<LiteralLike> createLiteralLikeNode(SyntaxKind kind, string text) {
        switch (kind) {
            case SyntaxKind::NumericLiteral:
                return createNumericLiteral(text, /*numericLiteralFlags*/ 0);
            case SyntaxKind::BigIntLiteral:
                return createBigIntLiteral(text);
            case SyntaxKind::StringLiteral:
                return createStringLiteral(text, /*isSingleQuote*/ {});
            case SyntaxKind::JsxText:
                return createJsxText(text, /*containsOnlyTriviaWhiteSpaces*/ false);
            case SyntaxKind::JsxTextAllWhiteSpaces:
                return createJsxText(text, /*containsOnlyTriviaWhiteSpaces*/ true);
            case SyntaxKind::RegularExpressionLiteral:
                return createRegularExpressionLiteral(text);
            case SyntaxKind::NoSubstitutionTemplateLiteral:
                return createTemplateLiteralLikeNode(kind, text, /*rawText*/ {}, /*templateFlags*/ 0);
        }
    }

//        //
//        // Identifiers
//        //

    shared<Identifier> createBaseIdentifier(string text, optional<SyntaxKind> originalKeywordKind) {
        if (!originalKeywordKind && !text.empty()) {
            originalKeywordKind = stringToToken(text);
        }
        if (originalKeywordKind == SyntaxKind::Identifier) {
            originalKeywordKind.reset();
        }
        auto node = createBaseNode<Identifier>();
        node->originalKeywordKind = originalKeywordKind;
        node->escapedText = escapeLeadingUnderscores(text);
        return node;
    }

    shared<Identifier> createBaseGeneratedIdentifier(string text, GeneratedIdentifierFlags autoGenerateFlags) {
        auto node = createBaseIdentifier(text, /*originalKeywordKind*/ {});
        node->autoGenerateFlags = (int) autoGenerateFlags;
        node->autoGenerateId = nextAutoGenerateId;
        nextAutoGenerateId++;
        return node;
    }

    // @api
    shared<Identifier> createIdentifier(string text, optional<NodeArray> typeArguments, optional<SyntaxKind> originalKeywordKind) {
        auto node = createBaseIdentifier(std::move(text), originalKeywordKind);
        if (typeArguments) {
            // NOTE: we do not use `setChildren` here because typeArguments in an identifier do not contribute to transformations
            node->typeArguments = createNodeArray(*typeArguments);
        }
        if (node->originalKeywordKind == SyntaxKind::AwaitKeyword) {
            node->transformFlags |= (int) TransformFlags::ContainsPossibleTopLevelAwait;
        }
        return node;
    }
//
//        // @api
//        function updateIdentifier(node: Identifier, typeArguments?: NodeArray<TypeNode | TypeParameterDeclaration> | undefined): Identifier {
//            return node->typeArguments != typeArguments
//                ? update(createIdentifier(idText(node), typeArguments), node)
//                : node;
//        }
//
//        // @api
//        function createTempVariable(recordTempVariable: ((node: Identifier) => void) | undefined, reservedInNestedScopes?: boolean): GeneratedIdentifier {
//            let flags = GeneratedIdentifierFlags.Auto;
//            if (reservedInNestedScopes) flags |= GeneratedIdentifierFlags.ReservedInNestedScopes;
//            auto name = createBaseGeneratedIdentifier("", flags);
//            if (recordTempVariable) {
//                recordTempVariable(name);
//            }
//            return name;
//        }
//
//        /** Create a unique temporary variable for use in a loop. */
//        // @api
//        function createLoopVariable(reservedInNestedScopes?: boolean): Identifier {
//            let flags = GeneratedIdentifierFlags.Loop;
//            if (reservedInNestedScopes) flags |= GeneratedIdentifierFlags.ReservedInNestedScopes;
//            return createBaseGeneratedIdentifier("", flags);
//        }
//
//        /** Create a unique name based on the supplied text. */
//        // @api
//        function createUniqueName(text: string, flags: GeneratedIdentifierFlags = GeneratedIdentifierFlags.None): Identifier {
//            Debug::asserts(!(flags & GeneratedIdentifierFlags.KindMask), "Argument out of range: flags");
//            Debug::asserts((flags & (GeneratedIdentifierFlags.Optimistic | GeneratedIdentifierFlags.FileLevel)) != GeneratedIdentifierFlags.FileLevel, "GeneratedIdentifierFlags.FileLevel cannot be set without also setting GeneratedIdentifierFlags.Optimistic");
//            return createBaseGeneratedIdentifier(text, GeneratedIdentifierFlags.Unique | flags);
//        }
//
//        /** Create a unique name generated for a node-> */
//        // @api
//        function getGeneratedNameForNode(node: Node | undefined, flags: GeneratedIdentifierFlags = 0): Identifier {
//            Debug::asserts(!(flags & GeneratedIdentifierFlags.KindMask), "Argument out of range: flags");
//            auto name = createBaseGeneratedIdentifier(node && isIdentifier(node) ? idText(node) : "", GeneratedIdentifierFlags.Node | flags);
//            name.original = node;
//            return name;
//        }
//
    // @api
    shared<PrivateIdentifier> createPrivateIdentifier(string text) {
        if (!startsWith(text, "#")) throw runtime_error("First character of private identifier must be #: " + text);
        auto node = createBaseNode<PrivateIdentifier>(SyntaxKind::PrivateIdentifier);
        node->escapedText = escapeLeadingUnderscores(text);
        node->transformFlags |= (int) TransformFlags::ContainsClassFields;
        return node;
    }

//        //
//        // Punctuation
//        //

//
//        //
//        // Modifiers
//        //
//
//        // @api
//        function createModifier<T extends ModifierSyntaxKind>(kind: T) {
//            return createToken(kind);
//        }
//
//        // @api
//        function createModifiersFromModifierFlags(flags: ModifierFlags) {
//            auto result: Modifier[] = [];
//            if (flags & ModifierFlags::Export) result.push(createModifier(SyntaxKind::ExportKeyword));
//            if (flags & ModifierFlags::Ambient) result.push(createModifier(SyntaxKind::DeclareKeyword));
//            if (flags & ModifierFlags::Default) result.push(createModifier(SyntaxKind::DefaultKeyword));
//            if (flags & ModifierFlags::Const) result.push(createModifier(SyntaxKind::ConstKeyword));
//            if (flags & ModifierFlags::Public) result.push(createModifier(SyntaxKind::PublicKeyword));
//            if (flags & ModifierFlags::Private) result.push(createModifier(SyntaxKind::PrivateKeyword));
//            if (flags & ModifierFlags::Protected) result.push(createModifier(SyntaxKind::ProtectedKeyword));
//            if (flags & ModifierFlags::Abstract) result.push(createModifier(SyntaxKind::AbstractKeyword));
//            if (flags & ModifierFlags::Static) result.push(createModifier(SyntaxKind::StaticKeyword));
//            if (flags & ModifierFlags::Override) result.push(createModifier(SyntaxKind::OverrideKeyword));
//            if (flags & ModifierFlags::Readonly) result.push(createModifier(SyntaxKind::ReadonlyKeyword));
//            if (flags & ModifierFlags::Async) result.push(createModifier(SyntaxKind::AsyncKeyword));
//            if (flags & ModifierFlags::In) result.push(createModifier(SyntaxKind::InKeyword));
//            if (flags & ModifierFlags::Out) result.push(createModifier(SyntaxKind::OutKeyword));
//            return result.length ? result : undefined;
//        }
//
//        //
//        // Names
//        //
//
    // @api
    auto createQualifiedName(shared<Node> left, NameType right) {
        auto node = createBaseNode<QualifiedName>(SyntaxKind::QualifiedName);
        node->left = left;
        node->right = dynamic_pointer_cast<Identifier>(asName(right));
        node->transformFlags |=
                propagateChildFlags(node->left) |
                propagateIdentifierNameFlags(node->right);
        return node;
    }

//        // @api
//        function updateQualifiedName(node: QualifiedName, left: EntityName, right: Identifier) {
//            return node->left != left
//                || node->right != right
//                ? update(createQualifiedName(left, right), node)
//                : node;
//        }
//
    // @api
    shared<ComputedPropertyName> createComputedPropertyName(shared<Expression> expression) {
        auto node = createBaseNode<ComputedPropertyName>(SyntaxKind::ComputedPropertyName);
        node->expression = parenthesizerRules::parenthesizeExpressionOfComputedPropertyName(expression);
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                (int) TransformFlags::ContainsES2015 |
                (int) TransformFlags::ContainsComputedPropertyName;
        return node;
    }

//        // @api
//        function updateComputedPropertyName(node: ComputedPropertyName, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createComputedPropertyName(expression), node)
//                : node;
//        }
//
//        //
//        // Signature elements
//        //
//
//        // @api
//        function createTypeParameterDeclaration(optional<NodeArray> modifiers, name: string | Identifier, constraint?: TypeNode, defaultType?: TypeNode): TypeParameterDeclaration;
//        /** @deprecated */
//        function createTypeParameterDeclaration(name: string | Identifier, constraint?: TypeNode, defaultType?: TypeNode): TypeParameterDeclaration;
    auto createTypeParameterDeclaration(
            optional<variant<NodeArray, shared<Node>, string>> modifiersOrName,
            optional<variant<shared<TypeNode>, shared<Identifier>, string>> nameOrConstraint,
            sharedOpt<TypeNode> constraintOrDefault,
            sharedOpt<TypeNode> defaultType = nullptr
    ) {
        NameType name = "";
        optional<NodeArray> modifiers;
        sharedOpt<TypeNode> constraint = constraintOrDefault;

        if (modifiersOrName) {
            if (holds_alternative<NodeArray>(*modifiersOrName)) {
                modifiers = get<NodeArray>(*modifiersOrName);
            } else if (holds_alternative<string>(*modifiersOrName)) {
                name = get<string>(*modifiersOrName);
            } else if (holds_alternative<shared<Node>>(*modifiersOrName)) {
                name = get<shared<Node>>(*modifiersOrName);
            }
        }

        if (nameOrConstraint) {
            if (holds_alternative<shared<TypeNode>>(*nameOrConstraint)) {
                constraint = get<shared<TypeNode>>(*nameOrConstraint);
            } else if (holds_alternative<shared<Identifier>>(*nameOrConstraint)) {
                name = get<shared<Identifier>>(*nameOrConstraint);
            } else if (holds_alternative<string>(*nameOrConstraint)) {
                name = get<string>(*nameOrConstraint);
            }
        }

//            if (!modifiersOrName || isArray(modifiersOrName)) {
//                modifiers = modifiersOrName;
//                name = nameOrConstraint as string | Identifier;
//                constraint = constraintOrDefault;
//            }
//            else {
//                modifiers = undefined;
//                name = modifiersOrName;
//                constraint = nameOrConstraint as TypeNode | undefined;
//            }
        auto node = createBaseNamedDeclaration<TypeParameterDeclaration>(
                SyntaxKind::TypeParameter,
                /*decorators*/ {},
                modifiers,
                name
        );
        node->constraint = constraint;
        node->defaultType = defaultType;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateTypeParameterDeclaration(node: TypeParameterDeclaration, optional<NodeArray> modifiers, name: Identifier, constraint: TypeNode | undefined, defaultsharedOpt<TypeNode> type): TypeParameterDeclaration;
//        /** @deprecated */
//        function updateTypeParameterDeclaration(node: TypeParameterDeclaration, name: Identifier, constraint: TypeNode | undefined, defaultsharedOpt<TypeNode> type): TypeParameterDeclaration;
//        function updateTypeParameterDeclaration(node: TypeParameterDeclaration, modifiersOrName: readonly Modifier[] | Identifier | undefined, nameOrConstraint: Identifier | TypeNode | undefined, constraintOrDefault: TypeNode | undefined, defaultType?: TypeNode | undefined) {
//            let name;
//            let modifiers;
//            let constraint;
//            if (modifiersOrName == undefined || isArray(modifiersOrName)) {
//                modifiers = modifiersOrName;
//                name = nameOrConstraint as Identifier;
//                constraint = constraintOrDefault;
//            }
//            else {
//                modifiers = undefined;
//                name = modifiersOrName;
//                constraint = nameOrConstraint as TypeNode | undefined;
//            }
//            return node->modifiers != modifiers
//                || node->name != name
//                || node->constraint != constraint
//                || node->default != defaultType
//                ? update(createTypeParameterDeclaration(modifiers, name, constraint, defaultType), node)
//                : node;
//        }

    // @api
    shared<ParameterDeclaration> createParameterDeclaration(
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            sharedOpt<DotDotDotToken> dotDotDotToken = nullptr,
            NameType name = "",
            sharedOpt<QuestionToken> questionToken = nullptr,
            sharedOpt<TypeNode> type = nullptr,
            sharedOpt<Expression> initializer = nullptr
    ) {
        auto node = createBaseVariableLikeDeclaration<ParameterDeclaration>(
                SyntaxKind::Parameter,
                decorators,
                modifiers,
                name,
                type,
                initializer ? parenthesizerRules::parenthesizeExpressionForDisallowedComma(initializer) : nullptr
        );
        node->dotDotDotToken = dotDotDotToken;
        node->questionToken = questionToken;
        if (isThisIdentifier(node->name)) {
            node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        } else {
            node->transformFlags |=
                    propagateChildFlags(node->dotDotDotToken) |
                    propagateChildFlags(node->questionToken);
            if (questionToken) node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
            if (modifiersToFlags(node->modifiers) & (int) ModifierFlags::ParameterPropertyModifier) node->transformFlags |= (int) TransformFlags::ContainsTypeScriptClassSyntax;
            if (!!initializer || !!dotDotDotToken) node->transformFlags |= (int) TransformFlags::ContainsES2015;
        }
        return node;
    }

//        // @api
//        function updateParameterDeclaration(
//            node: ParameterDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            dotDotDotToken: DotDotDotToken | undefined,
//            name: string | BindingName,
//            sharedOpt<QuestionToken> questionToken,
//            sharedOpt<TypeNode> type,
//            sharedOpt<Expression> initializer
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->dotDotDotToken != dotDotDotToken
//                || node->name != name
//                || node->questionToken != questionToken
//                || node->type != type
//                || node->initializer != initializer
//                ? update(createParameterDeclaration(decorators, modifiers, dotDotDotToken, name, questionToken, type, initializer), node)
//                : node;
//        }
//
    // @api
    shared<Decorator> createDecorator(shared<Expression> expression) {
        auto node = createBaseNode<Decorator>(SyntaxKind::Decorator);
        node->expression = parenthesizerRules::parenthesizeLeftSideOfAccess(expression);
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                (int) TransformFlags::ContainsTypeScript |
                (int) TransformFlags::ContainsTypeScriptClassSyntax;
        return node;
    }

//        // @api
//        function updateDecorator(node: Decorator, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createDecorator(expression), node)
//                : node;
//        }
//
//        //
//        // Type Elements
//        //
//
    // @api
    auto createPropertySignature(
            optional<NodeArray> modifiers,
            NameType name,
            sharedOpt<QuestionToken> questionToken,
            sharedOpt<TypeNode> type
    ) {
        auto node = createBaseNamedDeclaration<PropertySignature>(
                SyntaxKind::PropertySignature,
                /*decorators*/ {},
                modifiers,
                name
        );
        node->type = type;
        node->questionToken = questionToken;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updatePropertySignature(
//            node: PropertySignature,
//            optional<NodeArray> modifiers,
//            name: PropertyName,
//            sharedOpt<QuestionToken> questionToken,
//            sharedOpt<TypeNode> type
//        ) {
//            return node->modifiers != modifiers
//                || node->name != name
//                || node->questionToken != questionToken
//                || node->type != type
//                ? update(createPropertySignature(modifiers, name, questionToken, type), node)
//                : node;
//        }

    // @api
    auto createPropertyDeclaration(
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NameType name,
            sharedOpt<Node> questionOrExclamationToken,
            sharedOpt<TypeNode> type,
            sharedOpt<Expression> initializer
    ) {
        auto node = createBaseVariableLikeDeclaration<PropertyDeclaration>(
                SyntaxKind::PropertyDeclaration,
                decorators,
                modifiers,
                name,
                type,
                initializer
        );
        node->questionToken = questionOrExclamationToken && isQuestionToken(questionOrExclamationToken) ? dynamic_pointer_cast<QuestionToken>(questionOrExclamationToken) : nullptr;
        node->exclamationToken = questionOrExclamationToken && isExclamationToken(questionOrExclamationToken) ? dynamic_pointer_cast<ExclamationToken>(questionOrExclamationToken) : nullptr;
        node->transformFlags |=
                propagateChildFlags(node->questionToken) |
                propagateChildFlags(node->exclamationToken) |
                (int) TransformFlags::ContainsClassFields;
        if (isComputedPropertyName(node->name) || (hasStaticModifier(node) && node->initializer)) {
            node->transformFlags |= (int) TransformFlags::ContainsTypeScriptClassSyntax;
        }
        if (questionOrExclamationToken || modifiersToFlags(node->modifiers) & (int) ModifierFlags::Ambient) {
            node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        }
        return node;
    }

//        // @api
//        function updatePropertyDeclaration(
//            node: PropertyDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            NameType name,
//            questionOrExclamationToken: QuestionToken | ExclamationToken | undefined,
//            sharedOpt<TypeNode> type,
//            sharedOpt<Expression> initializer
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->name != name
//                || node->questionToken != (questionOrExclamationToken != undefined && isQuestionToken(questionOrExclamationToken) ? questionOrExclamationToken : undefined)
//                || node->exclamationToken != (questionOrExclamationToken != undefined && isExclamationToken(questionOrExclamationToken) ? questionOrExclamationToken : undefined)
//                || node->type != type
//                || node->initializer != initializer
//                ? update(createPropertyDeclaration(decorators, modifiers, name, questionOrExclamationToken, type, initializer), node)
//                : node;
//        }
//
    // @api
    auto createMethodSignature(
            optional<NodeArray> modifiers,
            NameType name,
            sharedOpt<QuestionToken> questionToken,
            optional<NodeArray> typeParameters,
            NodeArray parameters,
            sharedOpt<TypeNode> type
    ) {
        auto node = createBaseSignatureDeclaration<MethodSignature>(
                SyntaxKind::MethodSignature,
                /*decorators*/ {},
                modifiers,
                name,
                typeParameters,
                parameters,
                type
        );
        node->questionToken = questionToken;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateMethodSignature(
//            node: MethodSignature,
//            optional<NodeArray> modifiers,
//            name: PropertyName,
//            sharedOpt<QuestionToken> questionToken,
//            typeParameters: NodeArray<TypeParameterDeclaration> | undefined,
//            parameters: NodeArray<ParameterDeclaration>,
//            sharedOpt<TypeNode> type
//        ) {
//            return node->modifiers != modifiers
//                || node->name != name
//                || node->questionToken != questionToken
//                || node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                ? updateBaseSignatureDeclaration(createMethodSignature(modifiers, name, questionToken, typeParameters, parameters, type), node)
//                : node;
//        }
//
    // @api
    auto createMethodDeclaration(
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            sharedOpt<AsteriskToken> asteriskToken,
            NameType name,
            sharedOpt<QuestionToken> questionToken,
            optional<NodeArray> typeParameters,
            NodeArray parameters,
            sharedOpt<TypeNode> type,
            sharedOpt<Block> body
    ) {
        auto node = createBaseFunctionLikeDeclaration<MethodDeclaration>(
                SyntaxKind::MethodDeclaration,
                decorators,
                modifiers,
                name,
                typeParameters,
                parameters,
                type,
                body
        );
        node->asteriskToken = asteriskToken;
        node->questionToken = questionToken;
        node->transformFlags |=
                propagateChildFlags(node->asteriskToken) |
                propagateChildFlags(node->questionToken) |
                (int) TransformFlags::ContainsES2015;
        if (questionToken) {
            node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        }
        if (modifiersToFlags(node->modifiers) & (int) ModifierFlags::Async) {
            if (asteriskToken) {
                node->transformFlags |= (int) TransformFlags::ContainsES2018;
            } else {
                node->transformFlags |= (int) TransformFlags::ContainsES2017;
            }
        } else if (asteriskToken) {
            node->transformFlags |= (int) TransformFlags::ContainsGenerator;
        }
        return node;
    }

//        // @api
//        function updateMethodDeclaration(
//            node: MethodDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            sharedOpt<AsteriskToken> asteriskToken,
//            name: PropertyName,
//            sharedOpt<QuestionToken> questionToken,
//            optional<NodeArray> typeParameters,
//            NodeArray parameters,
//            sharedOpt<TypeNode> type,
//           sharedOpt<Block> body
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->asteriskToken != asteriskToken
//                || node->name != name
//                || node->questionToken != questionToken
//                || node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                || node->body != body
//                ? updateBaseFunctionLikeDeclaration(createMethodDeclaration(decorators, modifiers, asteriskToken, name, questionToken, typeParameters, parameters, type, body), node)
//                : node;
//        }
//
    // @api
    auto createClassStaticBlockDeclaration(
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            shared<Block> body
    ) {
        auto node = createBaseNamedDeclaration<ClassStaticBlockDeclaration>(
                SyntaxKind::ClassStaticBlockDeclaration,
                decorators,
                modifiers,
                /*name*/ {}
        );
        node->body = body;
        node->transformFlags = propagateChildFlags(body) | (int) TransformFlags::ContainsClassFields;
        return node;
    }
//
//        // @api
//        function updateClassStaticBlockDeclaration(
//            node: ClassStaticBlockDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            shared<Block> block
//        ): ClassStaticBlockDeclaration {
//            return node->decorators != decorators
//                || node->modifier != modifiers
//                || node->body != body
//                ? update(createClassStaticBlockDeclaration(decorators, modifiers, body), node)
//                : node;
//        }

    // @api
    auto createConstructorDeclaration(
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NodeArray parameters,
            sharedOpt<Block> body
    ) {
        auto node = createBaseFunctionLikeDeclaration<ConstructorDeclaration>(
                SyntaxKind::Constructor,
                decorators,
                modifiers,
                /*name*/ {},
                /*typeParameters*/ {},
                parameters,
                /*type*/ {},
                body
        );
        node->transformFlags |= (int) TransformFlags::ContainsES2015;
        return node;
    }

//        // @api
//        function updateConstructorDeclaration(
//            node: ConstructorDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            NodeArray parameters,
//           sharedOpt<Block> body
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->parameters != parameters
//                || node->body != body
//                ? updateBaseFunctionLikeDeclaration(createConstructorDeclaration(decorators, modifiers, parameters, body), node)
//                : node;
//        }
//
    // @api
    shared<GetAccessorDeclaration> createGetAccessorDeclaration(
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NameType name,
            NodeArray parameters,
            sharedOpt<TypeNode> type,
            sharedOpt<Block> body
    ) {
        return createBaseFunctionLikeDeclaration<GetAccessorDeclaration>(
                SyntaxKind::GetAccessor,
                decorators,
                modifiers,
                name,
                /*typeParameters*/ {},
                parameters,
                type,
                body
        );
    }

//        // @api
//        function updateGetAccessorDeclaration(
//            node: GetAccessorDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: PropertyName,
//            NodeArray parameters,
//            sharedOpt<TypeNode> type,
//           sharedOpt<Block> body
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->name != name
//                || node->parameters != parameters
//                || node->type != type
//                || node->body != body
//                ? updateBaseFunctionLikeDeclaration(createGetAccessorDeclaration(decorators, modifiers, name, parameters, type, body), node)
//                : node;
//        }

    // @api
    shared<SetAccessorDeclaration> createSetAccessorDeclaration(
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NameType name,
            NodeArray parameters,
            sharedOpt<Block> body
    ) {
        return createBaseFunctionLikeDeclaration<SetAccessorDeclaration>(
                SyntaxKind::SetAccessor,
                decorators,
                modifiers,
                name,
                /*typeParameters*/ {},
                parameters,
                /*type*/ {},
                body
        );
    }

//        // @api
//        function updateSetAccessorDeclaration(
//            node: SetAccessorDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: PropertyName,
//            NodeArray parameters,
//           sharedOpt<Block> body
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->name != name
//                || node->parameters != parameters
//                || node->body != body
//                ? updateBaseFunctionLikeDeclaration(createSetAccessorDeclaration(decorators, modifiers, name, parameters, body), node)
//                : node;
//        }
//
    // @api
    shared<CallSignatureDeclaration> createCallSignature(
            optional<NodeTypeArray(TypeParameterDeclaration)> typeParameters,
            NodeTypeArray(ParameterDeclaration) parameters,
            sharedOpt<TypeNode> type
    ) {
        auto node = createBaseSignatureDeclaration<CallSignatureDeclaration>(
                SyntaxKind::CallSignature,
                /*decorators*/ {},
                /*modifiers*/ {},
                /*name*/ {},
                typeParameters,
                parameters,
                type
        );
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateCallSignature(
//            node: CallSignatureDeclaration,
//            typeParameters: NodeArray<TypeParameterDeclaration> | undefined,
//            parameters: NodeArray<ParameterDeclaration>,
//            sharedOpt<TypeNode> type
//        ) {
//            return node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                ? updateBaseSignatureDeclaration(createCallSignature(typeParameters, parameters, type), node)
//                : node;
//        }
//
    // @api
    shared<ConstructSignatureDeclaration> createConstructSignature(
            optional<NodeArray> typeParameters,
            NodeArray parameters,
            sharedOpt<TypeNode> type
    ) {
        auto node = createBaseSignatureDeclaration<ConstructSignatureDeclaration>(
                SyntaxKind::ConstructSignature,
                /*decorators*/ {},
                /*modifiers*/ {},
                /*name*/ {},
                typeParameters,
                parameters,
                type
        );
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }
//
//        // @api
//        function updateConstructSignature(
//            node: ConstructSignatureDeclaration,
//            typeParameters: NodeArray<TypeParameterDeclaration> | undefined,
//            parameters: NodeArray<ParameterDeclaration>,
//            sharedOpt<TypeNode> type
//        ) {
//            return node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                ? updateBaseSignatureDeclaration(createConstructSignature(typeParameters, parameters, type), node)
//                : node;
//        }
//
    // @api
    shared<IndexSignatureDeclaration> createIndexSignature(
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NodeArray parameters,
            sharedOpt<TypeNode> type
    ) {
        auto node = createBaseSignatureDeclaration<IndexSignatureDeclaration>(
                SyntaxKind::IndexSignature,
                decorators,
                modifiers,
                /*name*/ {},
                /*typeParameters*/ {},
                parameters,
                type
        );
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateIndexSignature(
//            node: IndexSignatureDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            NodeArray parameters,
//            shared<TypeNode> type
//        ) {
//            return node->parameters != parameters
//                || node->type != type
//                || node->decorators != decorators
//                || node->modifiers != modifiers
//                ? updateBaseSignatureDeclaration(createIndexSignature(decorators, modifiers, parameters, type), node)
//                : node;
//        }

    // @api
    auto createTemplateLiteralTypeSpan(shared<TypeNode> type, shared<NodeUnion(TemplateMiddle, TemplateTail)> literal) {
        auto node = createBaseNode<TemplateLiteralTypeSpan>(SyntaxKind::TemplateLiteralTypeSpan);
        node->type = type;
        node->literal = literal;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateTemplateLiteralTypeSpan(node: TemplateLiteralTypeSpan, shared<TypeNode> type, literal: TemplateMiddle | TemplateTail) {
//            return node->type != type
//                || node->literal != literal
//                ? update(createTemplateLiteralTypeSpan(type, literal), node)
//                : node;
//        }
//
//        //
//        // Types
//        //
//
//        // @api
//        function createKeywordTypeNode<TKind extends KeywordTypeSyntaxKind>(kind: TKind) {
//            return createToken(kind);
//        }
//
    // @api
    shared<TypePredicateNode> createTypePredicateNode(sharedOpt<AssertsKeyword> assertsModifier, NameType parameterName, sharedOpt<TypeNode> type) {
        auto node = createBaseNode<TypePredicateNode>(SyntaxKind::TypePredicate);
        node->assertsModifier = assertsModifier;
        node->parameterName = asName(parameterName);
        node->type = type;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateTypePredicateNode(node: TypePredicateNode, assertsModifier: AssertsKeyword | undefined, parameterName: Identifier | ThisTypeNode, sharedOpt<TypeNode> type) {
//            return node->assertsModifier != assertsModifier
//                || node->parameterName != parameterName
//                || node->type != type
//                ? update(createTypePredicateNode(assertsModifier, parameterName, type), node)
//                : node;
//        }

    // @api
    auto createTypeReferenceNode(NameType typeName, optional<NodeArray> typeArguments) {
        auto node = createBaseNode<TypeReferenceNode>(SyntaxKind::TypeReference);
        node->typeName = asName(typeName);
        if (typeArguments) node->typeArguments = parenthesizerRules::parenthesizeTypeArguments(createNodeArray(typeArguments));
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateTypeReferenceNode(node: TypeReferenceNode, typeName: EntityName, typeArguments: NodeArray<TypeNode> | undefined) {
//            return node->typeName != typeName
//                || node->typeArguments != typeArguments
//                ? update(createTypeReferenceNode(typeName, typeArguments), node)
//                : node;
//        }

    // @api
    auto createFunctionTypeNode(
            optional<NodeArray> typeParameters,
            NodeArray parameters,
            sharedOpt<TypeNode> type
    ) {
        auto node = createBaseSignatureDeclaration<FunctionTypeNode>(
                SyntaxKind::FunctionType,
                /*decorators*/ {},
                /*modifiers*/ {},
                /*name*/ {},
                typeParameters,
                parameters,
                type
        );
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateFunctionTypeNode(
//            node: FunctionTypeNode,
//            typeParameters: NodeArray<TypeParameterDeclaration> | undefined,
//            parameters: NodeArray<ParameterDeclaration>,
//            sharedOpt<TypeNode> type
//        ) {
//            return node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                ? updateBaseSignatureDeclaration(createFunctionTypeNode(typeParameters, parameters, type), node)
//                : node;
//        }
//
//        // @api
//        function createConstructorTypeNode(...args: Parameters<typeof createConstructorTypeNode1 | typeof createConstructorTypeNode2>) {
//            return args.length == 4 ? createConstructorTypeNode1(...args) :
//                args.length == 3 ? createConstructorTypeNode2(...args) :
//                Debug.fail("Incorrect number of arguments specified.");
//        }

    auto createConstructorTypeNode1(
            optional<NodeArray> modifiers,
            optional<NodeArray> typeParameters,
            NodeArray parameters,
            sharedOpt<TypeNode> type
    ) {
        auto node = createBaseSignatureDeclaration<ConstructorTypeNode>(
                SyntaxKind::ConstructorType,
                /*decorators*/ {},
                modifiers,
                /*name*/ {},
                typeParameters,
                parameters,
                type
        );
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        /** @deprecated */
//        function createConstructorTypeNode2(
//            optional<NodeArray> typeParameters,
//            NodeArray parameters,
//            sharedOpt<TypeNode> type
//        ): ConstructorTypeNode {
//            return createConstructorTypeNode1(/*modifiers*/ {}, typeParameters, parameters, type);
//        }
//
//        // @api
//        function updateConstructorTypeNode(...args: Parameters<typeof updateConstructorTypeNode1 | typeof updateConstructorTypeNode2>) {
//            return args.length == 5 ? updateConstructorTypeNode1(...args) :
//                args.length == 4 ? updateConstructorTypeNode2(...args) :
//                Debug.fail("Incorrect number of arguments specified.");
//        }
//
//        function updateConstructorTypeNode1(
//            node: ConstructorTypeNode,
//            optional<NodeArray> modifiers,
//            typeParameters: NodeArray<TypeParameterDeclaration> | undefined,
//            parameters: NodeArray<ParameterDeclaration>,
//            sharedOpt<TypeNode> type
//        ) {
//            return node->modifiers != modifiers
//                || node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                ? updateBaseSignatureDeclaration(createConstructorTypeNode(modifiers, typeParameters, parameters, type), node)
//                : node;
//        }
//
//        /** @deprecated */
//        function updateConstructorTypeNode2(
//            node: ConstructorTypeNode,
//            typeParameters: NodeArray<TypeParameterDeclaration> | undefined,
//            parameters: NodeArray<ParameterDeclaration>,
//            sharedOpt<TypeNode> type
//        ) {
//            return updateConstructorTypeNode1(node, node->modifiers, typeParameters, parameters, type);
//        }
//
    // @api
    auto createTypeQueryNode(shared<NodeUnion(EntityName)> exprName, optional<NodeArray> typeArguments) {
        auto node = createBaseNode<TypeQueryNode>(SyntaxKind::TypeQuery);
        node->exprName = exprName;
        if (typeArguments) node->typeArguments = parenthesizerRules::parenthesizeTypeArguments(typeArguments);
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateTypeQueryNode(node: TypeQueryNode, exprName: EntityName, typeArguments?: readonly TypeNode[]) {
//            return node->exprName != exprName
//                || node->typeArguments != typeArguments
//                ? update(createTypeQueryNode(exprName, typeArguments), node)
//                : node;
//        }
//
    // @api
    auto createTypeLiteralNode(optional<NodeArray> members) {
        auto node = createBaseNode<TypeLiteralNode>(SyntaxKind::TypeLiteral);
        node->members = createNodeArray(members);
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }
//
//        // @api
//        function updateTypeLiteralNode(node: TypeLiteralNode, members: NodeArray<TypeElement>) {
//            return node->members != members
//                ? update(createTypeLiteralNode(members), node)
//                : node;
//        }

    // @api
    auto createArrayTypeNode(shared<TypeNode> elementType) {
        auto node = createBaseNode<ArrayTypeNode>(SyntaxKind::ArrayType);
        node->elementType = parenthesizerRules::parenthesizeNonArrayTypeOfPostfixType(elementType);
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateArrayTypeNode(node: ArrayTypeNode, shared<TypeNode> elementType): ArrayTypeNode {
//            return node->elementType != elementType
//                ? update(createArrayTypeNode(elementType), node)
//                : node;
//        }
//
    // @api
    auto createTupleTypeNode(NodeArray elements) {
        auto node = createBaseNode<TupleTypeNode>(SyntaxKind::TupleType);
        node->elements = createNodeArray(parenthesizerRules::parenthesizeElementTypesOfTupleType(elements));
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateTupleTypeNode(node: TupleTypeNode, elements: readonly (TypeNode | NamedTupleMember)[]) {
//            return node->elements != elements
//                ? update(createTupleTypeNode(elements), node)
//                : node;
//        }
//
    // @api
    auto createNamedTupleMember(sharedOpt<DotDotDotToken> dotDotDotToken, shared<Identifier> name, sharedOpt<QuestionToken> questionToken, shared<TypeNode> type) {
        auto node = createBaseNode<NamedTupleMember>(SyntaxKind::NamedTupleMember);
        node->dotDotDotToken = dotDotDotToken;
        node->name = name;
        node->questionToken = questionToken;
        node->type = type;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateNamedTupleMember(node: NamedTupleMember, dotDotDotToken: DotDotDotToken | undefined, name: Identifier, sharedOpt<QuestionToken> questionToken, shared<TypeNode> type) {
//            return node->dotDotDotToken != dotDotDotToken
//                || node->name != name
//                || node->questionToken != questionToken
//                || node->type != type
//                ? update(createNamedTupleMember(dotDotDotToken, name, questionToken, type), node)
//                : node;
//        }
//
//        // @api
//        function createOptionalTypeNode(shared<TypeNode> type) {
//            auto node = createBaseNode<OptionalTypeNode>(SyntaxKind::OptionalType);
//            node->type = parenthesizerRules::parenthesizeTypeOfOptionalType(type);
//            node->transformFlags = (int)TransformFlags::ContainsTypeScript;
//            return node;
//        }
//
//        // @api
//        function updateOptionalTypeNode(node: OptionalTypeNode, shared<TypeNode> type): OptionalTypeNode {
//            return node->type != type
//                ? update(createOptionalTypeNode(type), node)
//                : node;
//        }
//
    // @api
    auto createRestTypeNode(shared<TypeNode> type) {
        auto node = createBaseNode<RestTypeNode>(SyntaxKind::RestType);
        node->type = type;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateRestTypeNode(node: RestTypeNode, shared<TypeNode> type): RestTypeNode {
//            return node->type != type
//                ? update(createRestTypeNode(type), node)
//                : node;
//        }

    template<class T>
    shared<T> createUnionOrIntersectionTypeNode(SyntaxKind kind, NodeArray types, function<NodeArray(NodeArray)> parenthesize) {
        auto node = createBaseNode<T>(kind);
        node->types = factory::createNodeArray(parenthesize(types));
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        function updateUnionOrIntersectionTypeNode<T extends UnionOrIntersectionTypeNode>(node: T, types: NodeArray<TypeNode>, parenthesize: (nodes: readonly TypeNode[]) => readonly TypeNode[]): T {
//            return node->types != types
//                ? update(createUnionOrIntersectionTypeNode(node->kind, types, parenthesize) as T, node)
//                : node;
//        }

    // @api
    auto createUnionTypeNode(NodeArray types) {
        return createUnionOrIntersectionTypeNode<UnionTypeNode>(SyntaxKind::UnionType, types, parenthesizerRules::parenthesizeConstituentTypesOfUnionType);
    }

//        // @api
//        function updateUnionTypeNode(node: UnionTypeNode, types: NodeArray<TypeNode>) {
//            return updateUnionOrIntersectionTypeNode(node, types, parenthesizerRules::parenthesizeConstituentTypesOfUnionType);
//        }

    // @api
    auto createIntersectionTypeNode(NodeArray types) {
        return createUnionOrIntersectionTypeNode<IntersectionTypeNode>(SyntaxKind::IntersectionType, types, parenthesizerRules::parenthesizeConstituentTypesOfIntersectionType);
    }

//        // @api
//        function updateIntersectionTypeNode(node: IntersectionTypeNode, types: NodeArray<TypeNode>) {
//            return updateUnionOrIntersectionTypeNode(node, types, parenthesizerRules::parenthesizeConstituentTypesOfIntersectionType);
//        }

    // @api
    auto createConditionalTypeNode(shared<TypeNode> checkType, shared<TypeNode> extendsType, shared<TypeNode> trueType, shared<TypeNode> falseType) {
        auto node = createBaseNode<ConditionalTypeNode>(SyntaxKind::ConditionalType);
        node->checkType = parenthesizerRules::parenthesizeCheckTypeOfConditionalType(checkType);
        node->extendsType = parenthesizerRules::parenthesizeExtendsTypeOfConditionalType(extendsType);
        node->trueType = trueType;
        node->falseType = falseType;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateConditionalTypeNode(node: ConditionalTypeNode, checkType: TypeNode, extendsType: TypeNode, trueType: TypeNode, falseType: TypeNode) {
//            return node->checkType != checkType
//                || node->extendsType != extendsType
//                || node->trueType != trueType
//                || node->falseType != falseType
//                ? update(createConditionalTypeNode(checkType, extendsType, trueType, falseType), node)
//                : node;
//        }
//
    // @api
    auto createInferTypeNode(shared<TypeParameterDeclaration> typeParameter) {
        auto node = createBaseNode<InferTypeNode>(SyntaxKind::InferType);
        node->typeParameter = typeParameter;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateInferTypeNode(node: InferTypeNode, typeParameter: TypeParameterDeclaration) {
//            return node->typeParameter != typeParameter
//                ? update(createInferTypeNode(typeParameter), node)
//                : node;
//        }

    // @api
    auto createTemplateLiteralType(shared<TemplateHead> head, NodeArray templateSpans) {
        auto node = createBaseNode<TemplateLiteralTypeNode>(SyntaxKind::TemplateLiteralType);
        node->head = head;
        node->templateSpans = createNodeArray(templateSpans);
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateTemplateLiteralType(node: TemplateLiteralTypeNode, head: TemplateHead, templateSpans: readonly TemplateLiteralTypeSpan[]) {
//            return node->head != head
//                || node->templateSpans != templateSpans
//                ? update(createTemplateLiteralType(head, templateSpans), node)
//                : node;
//        }
//
//        // @api
//        function createImportTypeNode(argument: TypeNode, qualifier?: EntityName, typeArguments?: readonly TypeNode[], isTypeOf?: boolean): ImportTypeNode;
//        function createImportTypeNode(argument: TypeNode, assertions?: ImportTypeAssertionContainer, qualifier?: EntityName, typeArguments?: readonly TypeNode[], isTypeOf?: boolean): ImportTypeNode;
    auto createImportTypeNode(
            shared<TypeNode> argument,
            optional<variant<shared<NodeUnion(EntityName)>, shared<ImportTypeAssertionContainer>>> qualifierOrAssertions,
            optional<variant<shared<NodeUnion(EntityName)>, NodeArray>> typeArgumentsOrQualifier,
            optional<variant<bool, NodeArray>> isTypeOfOrTypeArguments,
            optional<bool> isTypeOf
    ) {
        sharedOpt<ImportTypeAssertionContainer> assertion;
        sharedOpt<NodeUnion(EntityName)> qualifier;
        optional<NodeArray> typeArguments;
        if (qualifierOrAssertions) {
            if (holds_alternative<shared<ImportTypeAssertionContainer>>(*qualifierOrAssertions)) {
                assertion = get<shared<ImportTypeAssertionContainer>>(*qualifierOrAssertions);
            } else if (holds_alternative<shared<NodeUnion(EntityName)>>(*qualifierOrAssertions)) {
                qualifier = get<shared<NodeUnion(EntityName)>>(*qualifierOrAssertions);
            }
        }

        if (typeArgumentsOrQualifier) {
            if (holds_alternative<NodeArray>(*typeArgumentsOrQualifier)) {
                typeArguments = get<NodeArray>(*typeArgumentsOrQualifier);
            } else if (holds_alternative<shared<NodeUnion(EntityName)>>(*typeArgumentsOrQualifier)) {
                qualifier = get<shared<NodeUnion(EntityName)>>(*typeArgumentsOrQualifier);
            }
        }

        if (isTypeOfOrTypeArguments) {
            if (holds_alternative<NodeArray>(*isTypeOfOrTypeArguments)) {
                typeArguments = get<NodeArray>(*isTypeOfOrTypeArguments);
            } else if (holds_alternative<bool>(*isTypeOfOrTypeArguments)) {
                isTypeOf = get<bool>(*isTypeOfOrTypeArguments);
            }
        }

//            auto assertion = qualifierOrAssertions && qualifierOrAssertions.kind == SyntaxKind::ImportTypeAssertionContainer ? qualifierOrAssertions : undefined;
//            auto qualifier = qualifierOrAssertions && isEntityName(qualifierOrAssertions) ? qualifierOrAssertions
//                : typeArgumentsOrQualifier && !isArray(typeArgumentsOrQualifier) ? typeArgumentsOrQualifier : undefined;
//            auto typeArguments = isArray(typeArgumentsOrQualifier) ? typeArgumentsOrQualifier : isArray(isTypeOfOrTypeArguments) ? isTypeOfOrTypeArguments : undefined;
//            isTypeOf = typeof isTypeOfOrTypeArguments == "boolean" ? isTypeOfOrTypeArguments : typeof isTypeOf == "boolean" ? isTypeOf : false;

        auto node = createBaseNode<ImportTypeNode>(SyntaxKind::ImportType);
        node->argument = argument;
        node->assertions = assertion;
        node->qualifier = qualifier;
        if (typeArguments) node->typeArguments = parenthesizerRules::parenthesizeTypeArguments(*typeArguments);
        node->isTypeOf = isTrue(isTypeOf);
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateImportTypeNode(node: ImportTypeNode, argument: TypeNode, qualifier: EntityName | undefined, optional<NodeArray> typeArguments, isTypeOf?: boolean | undefined): ImportTypeNode;
//        function updateImportTypeNode(node: ImportTypeNode, argument: TypeNode, assertions: ImportTypeAssertionContainer | undefined, qualifier: EntityName | undefined, optional<NodeArray> typeArguments, isTypeOf?: boolean | undefined): ImportTypeNode;
//        function updateImportTypeNode(
//            node: ImportTypeNode,
//            argument: TypeNode,
//            qualifierOrAssertions: EntityName | ImportTypeAssertionContainer | undefined,
//            typeArgumentsOrQualifier: readonly TypeNode[] | EntityName | undefined,
//            isTypeOfOrTypeArguments: boolean | readonly TypeNode[] | undefined,
//            isTypeOf?: boolean | undefined
//        ) {
//            auto assertion = qualifierOrAssertions && qualifierOrAssertions.kind == SyntaxKind::ImportTypeAssertionContainer ? qualifierOrAssertions : undefined;
//            auto qualifier = qualifierOrAssertions && isEntityName(qualifierOrAssertions) ? qualifierOrAssertions
//                : typeArgumentsOrQualifier && !isArray(typeArgumentsOrQualifier) ? typeArgumentsOrQualifier : undefined;
//            auto typeArguments = isArray(typeArgumentsOrQualifier) ? typeArgumentsOrQualifier : isArray(isTypeOfOrTypeArguments) ? isTypeOfOrTypeArguments : undefined;
//            isTypeOf = typeof isTypeOfOrTypeArguments == "boolean" ? isTypeOfOrTypeArguments : typeof isTypeOf == "boolean" ? isTypeOf : node->isTypeOf;
//
//            return node->argument != argument
//                || node->assertions != assertion
//                || node->qualifier != qualifier
//                || node->typeArguments != typeArguments
//                || node->isTypeOf != isTypeOf
//                ? update(createImportTypeNode(argument, assertion, qualifier, typeArguments, isTypeOf), node)
//                : node;
//        }

    // @api
    shared<ParenthesizedTypeNode> createParenthesizedType(shared<TypeNode> type) {
        auto node = createBaseNode<ParenthesizedTypeNode>(SyntaxKind::ParenthesizedType);
        node->type = type;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateParenthesizedType(node: ParenthesizedTypeNode, shared<TypeNode> type) {
//            return node->type != type
//                ? update(createParenthesizedType(type), node)
//                : node;
//        }

    // @api
    auto createThisTypeNode() {
        auto node = createBaseNode<ThisTypeNode>(SyntaxKind::ThisType);
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

    // @api
    auto createTypeOperatorNode(SyntaxKind operatorKind, shared<TypeNode> type) {
        auto node = createBaseNode<TypeOperatorNode>(SyntaxKind::TypeOperator);
        node->operatorKind = operatorKind;
        node->type = operatorKind == SyntaxKind::ReadonlyKeyword ?
                     parenthesizerRules::parenthesizeOperandOfReadonlyTypeOperator(type) :
                     parenthesizerRules::parenthesizeOperandOfTypeOperator(type);
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateTypeOperatorNode(node: TypeOperatorNode, shared<TypeNode> type) {
//            return node->type != type
//                ? update(createTypeOperatorNode(node->operator, type), node)
//                : node;
//        }

    // @api
    auto createIndexedAccessTypeNode(shared<TypeNode> objectType, shared<TypeNode> indexType) {
        auto node = createBaseNode<IndexedAccessTypeNode>(SyntaxKind::IndexedAccessType);
        node->objectType = parenthesizerRules::parenthesizeNonArrayTypeOfPostfixType(objectType);
        node->indexType = indexType;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateIndexedAccessTypeNode(node: IndexedAccessTypeNode, objectType: TypeNode, indexType: TypeNode) {
//            return node->objectType != objectType
//                || node->indexType != indexType
//                ? update(createIndexedAccessTypeNode(objectType, indexType), node)
//                : node;
//        }

    // @api
    auto createMappedTypeNode(
            sharedOpt<Node> readonlyToken, //: ReadonlyKeyword | PlusToken | MinusToken | undefined,
            shared<TypeParameterDeclaration> typeParameter,
            sharedOpt<TypeNode> nameType,
            sharedOpt<Node> questionToken, //: QuestionToken | PlusToken | MinusToken | undefined,
            sharedOpt<TypeNode> type,
            optional<NodeArray> members
    ) {
        auto node = createBaseNode<MappedTypeNode>(SyntaxKind::MappedType);
        node->readonlyToken = readonlyToken;
        node->typeParameter = typeParameter;
        node->nameType = nameType;
        node->questionToken = questionToken;
        node->type = type;
        if (members) node->members = createNodeArray(members);
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateMappedTypeNode(node: MappedTypeNode, readonlyToken: ReadonlyKeyword | PlusToken | MinusToken | undefined, typeParameter: TypeParameterDeclaration, namesharedOpt<TypeNode> type, questionToken: QuestionToken | PlusToken | MinusToken | undefined, sharedOpt<TypeNode> type, optional<NodeArray> members): MappedTypeNode {
//            return node->readonlyToken != readonlyToken
//                || node->typeParameter != typeParameter
//                || node->nameType != nameType
//                || node->questionToken != questionToken
//                || node->type != type
//                || node->members != members
//                ? update(createMappedTypeNode(readonlyToken, typeParameter, nameType, questionToken, type, members), node)
//                : node;
//        }
//
    // @api
    auto createLiteralTypeNode(shared<Expression> literal) {
        auto node = createBaseNode<LiteralTypeNode>(SyntaxKind::LiteralType);
        node->literal = literal;
        node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        return node;
    }

//        // @api
//        function updateLiteralTypeNode(node: LiteralTypeNode, literal: LiteralTypeNode["literal"]) {
//            return node->literal != literal
//                ? update(createLiteralTypeNode(literal), node)
//                : node;
//        }
//
//        //
//        // Binding Patterns
//        //
//
    // @api
    shared<ObjectBindingPattern> createObjectBindingPattern(NodeArray elements) {
        auto node = createBaseNode<ObjectBindingPattern>(SyntaxKind::ObjectBindingPattern);
        node->elements = createNodeArray(elements);
        node->transformFlags |=
                propagateChildrenFlags(node->elements) |
                (int) TransformFlags::ContainsES2015 |
                (int) TransformFlags::ContainsBindingPattern;
        if (node->transformFlags & (int) TransformFlags::ContainsRestOrSpread) {
            node->transformFlags |=
                    (int) TransformFlags::ContainsES2018 |
                    (int) TransformFlags::ContainsObjectRestOrSpread;
        }
        return node;
    }
//
//        // @api
//        function updateObjectBindingPattern(node: ObjectBindingPattern, elements: readonly BindingElement[]) {
//            return node->elements != elements
//                ? update(createObjectBindingPattern(elements), node)
//                : node;
//        }
//
    // @api
    shared<ArrayBindingPattern> createArrayBindingPattern(NodeArray elements) {
        auto node = createBaseNode<ArrayBindingPattern>(SyntaxKind::ArrayBindingPattern);
        node->elements = createNodeArray(elements);
        node->transformFlags |=
                propagateChildrenFlags(node->elements) |
                (int) TransformFlags::ContainsES2015 |
                (int) TransformFlags::ContainsBindingPattern;
        return node;
    }

//        // @api
//        function updateArrayBindingPattern(node: ArrayBindingPattern, elements: readonly ArrayBindingElement[]) {
//            return node->elements != elements
//                ? update(createArrayBindingPattern(elements), node)
//                : node;
//        }
//
    // @api
    shared<BindingElement> createBindingElement(
            sharedOpt<DotDotDotToken> dotDotDotToken,
            optional<variant<string, shared<NodeUnion(PropertyName)>>> propertyName = {},
            variant<string, shared<NodeUnion(BindingName)>> name = "",
            sharedOpt<Expression> initializer = {}
    ) {
        auto node = createBaseBindingLikeDeclaration<BindingElement>(
                SyntaxKind::BindingElement,
                /*decorators*/ {},
                /*modifiers*/ {},
                name,
                initializer ? parenthesizerRules::parenthesizeExpressionForDisallowedComma(initializer) : nullptr
        );
        node->propertyName = asName(*propertyName);
        node->dotDotDotToken = dotDotDotToken;
        node->transformFlags |= propagateChildFlags(node->dotDotDotToken) | (int) TransformFlags::ContainsES2015;
        if (node->propertyName) {
            node->transformFlags |= isIdentifier(node->propertyName) ?
                                    propagateIdentifierNameFlags(node->propertyName) :
                                    propagateChildFlags(node->propertyName);
        }
        if (dotDotDotToken) node->transformFlags |= (int) TransformFlags::ContainsRestOrSpread;
        return node;
    }

//        // @api
//        function updateBindingElement(node: BindingElement, dotDotDotToken: DotDotDotToken | undefined, propertyName: PropertyName | undefined, name: BindingName, sharedOpt<Expression> initializer) {
//            return node->propertyName != propertyName
//                || node->dotDotDotToken != dotDotDotToken
//                || node->name != name
//                || node->initializer != initializer
//                ? update(createBindingElement(dotDotDotToken, propertyName, name, initializer), node)
//                : node;
//        }
//
//        //
//        // Expression
//        //

    template<typename T>
    shared<T> createBaseExpression(SyntaxKind kind) {
        auto node = createBaseNode<T>(kind);
        // the following properties are commonly set by the checker/binder
        return node;
    }

    // @api
    auto createArrayLiteralExpression(optional<NodeArray> elements, bool multiLine) {
        auto node = createBaseExpression<ArrayLiteralExpression>(SyntaxKind::ArrayLiteralExpression);
        // Ensure we add a trailing comma for something like `[NumericLiteral(1), NumericLiteral(2), OmittedExpresion]` so that
        // we end up with `[1, 2, ,]` instead of `[1, 2, ]` otherwise the `OmittedExpression` will just end up being treated like
        // a trailing comma.
        sharedOpt<Node> lastElement = elements ? lastOrUndefined(elements) : nullptr;
        auto elementsArray = createNodeArray(elements, lastElement && isOmittedExpression(lastElement) ? true : false);
        node->elements = parenthesizerRules::parenthesizeExpressionsOfCommaDelimitedList(elementsArray);
        node->multiLine = multiLine;
        node->transformFlags |= propagateChildrenFlags(node->elements);
        return node;
    }

//        // @api
//        function updateArrayLiteralExpression(node: ArrayLiteralExpression, elements: readonly Expression[]) {
//            return node->elements != elements
//                ? update(createArrayLiteralExpression(elements, node->multiLine), node)
//                : node;
//        }

    // @api
    auto createObjectLiteralExpression(optional<NodeArray> properties, bool multiLine) {
        auto node = createBaseExpression<ObjectLiteralExpression>(SyntaxKind::ObjectLiteralExpression);
        node->properties = createNodeArray(properties);
        node->multiLine = multiLine;
        node->transformFlags |= propagateChildrenFlags(node->properties);
        return node;
    }

//        // @api
//        function updateObjectLiteralExpression(node: ObjectLiteralExpression, properties: readonly ObjectLiteralElementLike[]) {
//            return node->properties != properties
//                ? update(createObjectLiteralExpression(properties, node->multiLine), node)
//                : node;
//        }
//
    // @api
    shared<PropertyAccessExpression> createPropertyAccessExpression(shared<Expression> expression, NameType _name) {
        auto node = createBaseExpression<PropertyAccessExpression>(SyntaxKind::PropertyAccessExpression);
        node->expression = parenthesizerRules::parenthesizeLeftSideOfAccess(expression);
        node->name = asName(_name);
        node->transformFlags =
                propagateChildFlags(node->expression) |
                (isIdentifier(node->name) ?
                 propagateIdentifierNameFlags(node->name) :
                 propagateChildFlags(node->name));
        if (isSuperKeyword(expression)) {
            // super method calls require a lexical 'this'
            // super method calls require 'super' hoisting in ES2017 and ES2018 async functions and async generators
            node->transformFlags |=
                    (int) TransformFlags::ContainsES2017 |
                    (int) TransformFlags::ContainsES2018;
        }
        return node;
    }

    // @api
    shared<PropertyAccessExpression> createPropertyAccessChain(shared<Expression> expression, sharedOpt<QuestionDotToken> questionDotToken, NameType name) {
        auto node = createBaseExpression<PropertyAccessExpression>(SyntaxKind::PropertyAccessExpression);
        node->flags |= (int) NodeFlags::OptionalChain;
        node->expression = parenthesizerRules::parenthesizeLeftSideOfAccess(expression);
        node->questionDotToken = questionDotToken;
        node->name = asName(name);
        node->transformFlags |=
                (int) TransformFlags::ContainsES2020 |
                propagateChildFlags(node->expression) |
                propagateChildFlags(node->questionDotToken) |
                (isIdentifier(node->name) ?
                 propagateIdentifierNameFlags(node->name) :
                 propagateChildFlags(node->name));
        return node;
    }

//        // @api
//        function updatePropertyAccessExpression(node: PropertyAccessExpression, shared<Expression> expression, name: Identifier | PrivateIdentifier) {
//            if (isPropertyAccessChain(node)) {
//                return updatePropertyAccessChain(node, expression, node->questionDotToken, cast(name, isIdentifier));
//            }
//            return node->expression != expression
//                || node->name != name
//                ? update(createPropertyAccessExpression(expression, name), node)
//                : node;
//        }
//
//
//        // @api
//        function updatePropertyAccessChain(node: PropertyAccessChain, shared<Expression> expression, sharedOpt<QuestionDotToken> questionDotToken, name: Identifier | PrivateIdentifier) {
//            Debug::asserts(!!(node->flags & NodeFlags::OptionalChain), "Cannot update a PropertyAccessExpression using updatePropertyAccessChain. Use updatePropertyAccess instead.");
//            // Because we are updating an existing PropertyAccessChain we want to inherit its emitFlags
//            // instead of using the default from createPropertyAccess
//            return node->expression != expression
//                || node->questionDotToken != questionDotToken
//                || node->name != name
//                ? update(createPropertyAccessChain(expression, questionDotToken, name), node)
//                : node;
//        }
//
    // @api
    shared<ElementAccessExpression> createElementAccessExpression(shared<Expression> expression, ExpressionType index) {
        auto node = createBaseExpression<ElementAccessExpression>(SyntaxKind::ElementAccessExpression);
        node->expression = parenthesizerRules::parenthesizeLeftSideOfAccess(expression);
        node->argumentExpression = asExpression(index);
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                propagateChildFlags(node->argumentExpression);
        if (isSuperKeyword(expression)) {
            // super method calls require a lexical 'this'
            // super method calls require 'super' hoisting in ES2017 and ES2018 async functions and async generators
            node->transformFlags |=
                    (int) TransformFlags::ContainsES2017 |
                    (int) TransformFlags::ContainsES2018;
        }
        return node;
    }
//
//        // @api
//        function updateElementAccessExpression(node: ElementAccessExpression, shared<Expression> expression, argumentExpression: Expression) {
//            if (isElementAccessChain(node)) {
//                return updateElementAccessChain(node, expression, node->questionDotToken, argumentExpression);
//            }
//            return node->expression != expression
//                || node->argumentExpression != argumentExpression
//                ? update(createElementAccessExpression(expression, argumentExpression), node)
//                : node;
//        }
//
    // @api
    shared<ElementAccessExpression> createElementAccessChain(shared<Expression> expression, sharedOpt<QuestionDotToken> questionDotToken, ExpressionType index) {
        auto node = createBaseExpression<ElementAccessExpression>(SyntaxKind::ElementAccessExpression);
        node->flags |= (int) NodeFlags::OptionalChain;
        node->expression = parenthesizerRules::parenthesizeLeftSideOfAccess(expression);
        node->questionDotToken = questionDotToken;
        node->argumentExpression = asExpression(index);
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                propagateChildFlags(node->questionDotToken) |
                propagateChildFlags(node->argumentExpression) |
                (int) TransformFlags::ContainsES2020;
        return node;
    }

//        // @api
//        function updateElementAccessChain(node: ElementAccessChain, shared<Expression> expression, sharedOpt<QuestionDotToken> questionDotToken, argumentExpression: Expression) {
//            Debug::asserts(!!(node->flags & NodeFlags::OptionalChain), "Cannot update a ElementAccessExpression using updateElementAccessChain. Use updateElementAccess instead.");
//            // Because we are updating an existing ElementAccessChain we want to inherit its emitFlags
//            // instead of using the default from createElementAccess
//            return node->expression != expression
//                || node->questionDotToken != questionDotToken
//                || node->argumentExpression != argumentExpression
//                ? update(createElementAccessChain(expression, questionDotToken, argumentExpression), node)
//                : node;
//        }
//
    // @api
    auto createCallExpression(shared<Expression> expression, optional<NodeArray> typeArguments, optional<NodeArray> argumentsArray) {
        auto node = createBaseExpression<CallExpression>(SyntaxKind::CallExpression);
        node->expression = parenthesizerRules::parenthesizeLeftSideOfAccess(expression);
        node->typeArguments = asNodeArray(typeArguments);
        node->arguments = parenthesizerRules::parenthesizeExpressionsOfCommaDelimitedList(createNodeArray(argumentsArray));
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                propagateChildrenFlags(node->typeArguments) |
                propagateChildrenFlags(node->arguments);
        if (node->typeArguments) {
            node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        }
        if (isImportKeyword(node->expression)) {
            node->transformFlags |= (int) TransformFlags::ContainsDynamicImport;
        } else if (isSuperProperty(node->expression)) {
            node->transformFlags |= (int) TransformFlags::ContainsLexicalThis;
        }
        return node;
    }

    // @api
    auto createCallChain(shared<Expression> expression, sharedOpt<QuestionDotToken> questionDotToken, optional<NodeArray> typeArguments, optional<NodeArray> argumentsArray) {
        auto node = createBaseExpression<CallChain>(SyntaxKind::CallExpression);
        node->flags |= (int) NodeFlags::OptionalChain;
        node->expression = parenthesizerRules::parenthesizeLeftSideOfAccess(expression);
        node->questionDotToken = questionDotToken;
        node->typeArguments = asNodeArray(typeArguments);
        node->arguments = parenthesizerRules::parenthesizeExpressionsOfCommaDelimitedList(createNodeArray(argumentsArray));
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                propagateChildFlags(node->questionDotToken) |
                propagateChildrenFlags(node->typeArguments) |
                propagateChildrenFlags(node->arguments) |
                (int) TransformFlags::ContainsES2020;
        if (node->typeArguments) {
            node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        }
        if (isSuperProperty(node->expression)) {
            node->transformFlags |= (int) TransformFlags::ContainsLexicalThis;
        }
        return node;
    }

    // @api
    auto updateCallChain(shared<CallChain> node, shared<Expression> expression, sharedOpt<QuestionDotToken> questionDotToken, optional<NodeArray> typeArguments, NodeArray argumentsArray) {
        Debug::asserts(!!(node->flags & (int) NodeFlags::OptionalChain), "Cannot update a CallExpression using updateCallChain. Use updateCall instead.");
        return node->expression != expression
               || node->questionDotToken != questionDotToken
               || node->typeArguments != typeArguments
               || node->arguments != argumentsArray
               ? update(createCallChain(expression, questionDotToken, typeArguments, argumentsArray), node)
               : node;
    }

    // @api
    shared<CallExpression> updateCallExpression(shared<CallExpression> node, shared<Expression> expression, optional<NodeArray> typeArguments, NodeArray argumentsArray) {
        if (isCallChain(node)) {
            return updateCallChain(node, expression, node->questionDotToken, typeArguments, argumentsArray);
        }
        return node->expression != expression
               || node->typeArguments != typeArguments
               || node->arguments != argumentsArray
               ? update(createCallExpression(expression, typeArguments, argumentsArray), node)
               : node;
    }

    // @api
    auto createNewExpression(shared<Expression> expression, optional<NodeArray> typeArguments, optional<NodeArray> argumentsArray) {
        auto node = createBaseExpression<NewExpression>(SyntaxKind::NewExpression);
        node->expression = parenthesizerRules::parenthesizeExpressionOfNew(expression);
        node->typeArguments = asNodeArray(typeArguments);
        if (argumentsArray) node->arguments = parenthesizerRules::parenthesizeExpressionsOfCommaDelimitedList(*argumentsArray);
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                propagateChildrenFlags(node->typeArguments) |
                propagateChildrenFlags(node->arguments) |
                (int) TransformFlags::ContainsES2020;
        if (node->typeArguments) {
            node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        }
        return node;
    }

//        // @api
//        function updateNewExpression(node: NewExpression, shared<Expression> expression, optional<NodeArray> typeArguments, optional<NodeArray> argumentsArray) {
//            return node->expression != expression
//                || node->typeArguments != typeArguments
//                || node->arguments != argumentsArray
//                ? update(createNewExpression(expression, typeArguments, argumentsArray), node)
//                : node;
//        }

    // @api
    shared<TaggedTemplateExpression> createTaggedTemplateExpression(shared<Expression> tag, optional<NodeArray> typeArguments, shared<NodeUnion(TemplateLiteral)> templateLiteral) {
        auto node = createBaseExpression<TaggedTemplateExpression>(SyntaxKind::TaggedTemplateExpression);
        node->tag = parenthesizerRules::parenthesizeLeftSideOfAccess(tag);
        node->typeArguments = asNodeArray(typeArguments);
        node->templateLiteral = templateLiteral;
        node->transformFlags |=
                propagateChildFlags(node->tag) |
                propagateChildrenFlags(node->typeArguments) |
                propagateChildFlags(node->templateLiteral) |
                (int) TransformFlags::ContainsES2015;
        if (node->typeArguments) {
            node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        }
        if (hasInvalidEscape(node->templateLiteral)) {
            node->transformFlags |= (int) TransformFlags::ContainsES2018;
        }
        return node;
    }
//
//        // @api
//        function updateTaggedTemplateExpression(node: TaggedTemplateExpression, tag: Expression, optional<NodeArray> typeArguments, template: TemplateLiteral) {
//            return node->tag != tag
//                || node->typeArguments != typeArguments
//                || node->template != template
//                ? update(createTaggedTemplateExpression(tag, typeArguments, template), node)
//                : node;
//        }

    // @api
    auto createTypeAssertion(shared<TypeNode> type, shared<Expression> expression) {
        auto node = createBaseExpression<TypeAssertion>(SyntaxKind::TypeAssertionExpression);
        node->expression = parenthesizerRules::parenthesizeOperandOfPrefixUnary(expression);
        node->type = type;
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                propagateChildFlags(node->type) |
                (int) TransformFlags::ContainsTypeScript;
        return node;
    }

    // @api
    auto updateTypeAssertion(shared<TypeAssertion> node, shared<TypeNode> type, shared<Expression> expression) {
        return node->type != type
               || node->expression != expression
               ? update(createTypeAssertion(type, expression), node)
               : node;
    }

// @api
    shared<ParenthesizedExpression> createParenthesizedExpression(shared<Expression> expression) {
        auto node = createBaseExpression<ParenthesizedExpression>(SyntaxKind::ParenthesizedExpression);
        node->expression = expression;
        node->transformFlags = propagateChildFlags(node->expression);
        return node;
    }

    // @api
    auto updateParenthesizedExpression(shared<ParenthesizedExpression> node, shared<Expression> expression) {
        return node->expression != expression
               ? update(createParenthesizedExpression(expression), node)
               : node;
    }

    // @api
    auto createFunctionExpression(
            optional<NodeArray> modifiers,
            sharedOpt<AsteriskToken> asteriskToken,
            NameType name,
            optional<NodeArray> typeParameters,
            optional<NodeArray> parameters,
            sharedOpt<TypeNode> type,
            shared<Block> body
    ) {
        auto node = createBaseFunctionLikeDeclaration<FunctionExpression>(
                SyntaxKind::FunctionExpression,
                /*decorators*/ {},
                modifiers,
                name,
                typeParameters,
                parameters,
                type,
                body
        );
        node->asteriskToken = asteriskToken;
        node->transformFlags |= propagateChildFlags(node->asteriskToken);
        if (node->typeParameters) {
            node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        }
        if (modifiersToFlags(node->modifiers) & (int) ModifierFlags::Async) {
            if (node->asteriskToken) {
                node->transformFlags |= (int) TransformFlags::ContainsES2018;
            } else {
                node->transformFlags |= (int) TransformFlags::ContainsES2017;
            }
        } else if (node->asteriskToken) {
            node->transformFlags |= (int) TransformFlags::ContainsGenerator;
        }
        return node;
    }

//        // @api
//        function updateFunctionExpression(
//            node: FunctionExpression,
//            optional<NodeArray> modifiers,
//            sharedOpt<AsteriskToken> asteriskToken,
//            name: Identifier | undefined,
//            optional<NodeArray> typeParameters,
//            NodeArray parameters,
//            sharedOpt<TypeNode> type,
//            shared<Block> block
//        ) {
//            return node->name != name
//                || node->modifiers != modifiers
//                || node->asteriskToken != asteriskToken
//                || node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                || node->body != body
//                ? updateBaseFunctionLikeDeclaration(createFunctionExpression(modifiers, asteriskToken, name, typeParameters, parameters, type, body), node)
//                : node;
//        }
    // @api
    auto createArrowFunction(
            optional<NodeArray> modifiers,
            optional<NodeArray> typeParameters,
            NodeArray parameters,
            sharedOpt<TypeNode> type,
            sharedOpt<EqualsGreaterThanToken> equalsGreaterThanToken,
            shared<NodeUnion(ConciseBody)> body
    ) {
        auto node = createBaseFunctionLikeDeclaration<ArrowFunction>(
                SyntaxKind::ArrowFunction,
                /*decorators*/ {},
                modifiers,
                /*name*/ {},
                typeParameters,
                parameters,
                type,
                parenthesizerRules::parenthesizeConciseBodyOfArrowFunction(body)
        );
        node->equalsGreaterThanToken = equalsGreaterThanToken ? equalsGreaterThanToken : createToken<EqualsGreaterThanToken>(SyntaxKind::EqualsGreaterThanToken);
        node->transformFlags |=
                propagateChildFlags(node->equalsGreaterThanToken) |
                (int) TransformFlags::ContainsES2015;
        if (modifiersToFlags(node->modifiers) & (int) ModifierFlags::Async) {
            node->transformFlags |= (int) TransformFlags::ContainsES2017 | (int) TransformFlags::ContainsLexicalThis;
        }
        return node;
    }

//        // @api
//        function updateArrowFunction(
//            node: ArrowFunction,
//            optional<NodeArray> modifiers,
//            optional<NodeArray> typeParameters,
//            NodeArray parameters,
//            sharedOpt<TypeNode> type,
//            equalsGreaterThanToken: EqualsGreaterThanToken,
//            body: ConciseBody
//        ): ArrowFunction {
//            return node->modifiers != modifiers
//                || node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                || node->equalsGreaterThanToken != equalsGreaterThanToken
//                || node->body != body
//                ? updateBaseFunctionLikeDeclaration(createArrowFunction(modifiers, typeParameters, parameters, type, equalsGreaterThanToken, body), node)
//                : node;
//        }
//
    // @api
    auto createDeleteExpression(shared<Expression> expression) {
        auto node = createBaseExpression<DeleteExpression>(SyntaxKind::DeleteExpression);
        node->expression = parenthesizerRules::parenthesizeOperandOfPrefixUnary(expression);
        node->transformFlags |= propagateChildFlags(node->expression);
        return node;
    }

//        // @api
//        function updateDeleteExpression(node: DeleteExpression, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createDeleteExpression(expression), node)
//                : node;
//        }

    // @api
    auto createTypeOfExpression(shared<Expression> expression) {
        auto node = createBaseExpression<TypeOfExpression>(SyntaxKind::TypeOfExpression);
        node->expression = parenthesizerRules::parenthesizeOperandOfPrefixUnary(expression);
        node->transformFlags |= propagateChildFlags(node->expression);
        return node;
    }

//        // @api
//        function updateTypeOfExpression(node: TypeOfExpression, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createTypeOfExpression(expression), node)
//                : node;
//        }

    // @api
    auto createVoidExpression(shared<Expression> expression) {
        auto node = createBaseExpression<VoidExpression>(SyntaxKind::VoidExpression);
        node->expression = parenthesizerRules::parenthesizeOperandOfPrefixUnary(expression);
        node->transformFlags |= propagateChildFlags(node->expression);
        return node;
    }

//        // @api
//        function updateVoidExpression(node: VoidExpression, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createVoidExpression(expression), node)
//                : node;
//        }

    // @api
    auto createAwaitExpression(shared<Expression> expression) {
        auto node = createBaseExpression<AwaitExpression>(SyntaxKind::AwaitExpression);
        node->expression = parenthesizerRules::parenthesizeOperandOfPrefixUnary(expression);
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                (int) TransformFlags::ContainsES2017 |
                (int) TransformFlags::ContainsES2018 |
                (int) TransformFlags::ContainsAwait;
        return node;
    }

//        // @api
//        function updateAwaitExpression(node: AwaitExpression, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createAwaitExpression(expression), node)
//                : node;
//        }
//
// @api
    shared<PrefixUnaryExpression> createPrefixUnaryExpression(SyntaxKind operatorKind, shared<Expression> operand) {
        auto node = createBaseExpression<PrefixUnaryExpression>(SyntaxKind::PrefixUnaryExpression);
        node->operatorKind = operatorKind;
        node->operand = parenthesizerRules::parenthesizeOperandOfPrefixUnary(operand);
        node->transformFlags |= propagateChildFlags(node->operand);
        // Only set this flag for non-generated identifiers and non-"local" names. See the
        // comment in `visitPreOrPostfixUnaryExpression` in module.ts
        if ((operatorKind == SyntaxKind::PlusPlusToken || operatorKind == SyntaxKind::MinusMinusToken) &&
            isIdentifier(node->operand) &&
            !isGeneratedIdentifier(node->operand)) {
            //!isLocalName(node->operand)) { //<- this is always false as its set by transformer/emit stuff
            node->transformFlags |= (int) TransformFlags::ContainsUpdateExpressionForIdentifier;
        }
        return node;
    }

//        // @api
//        function updatePrefixUnaryExpression(node: PrefixUnaryExpression, operand: Expression) {
//            return node->operand != operand
//                ? update(createPrefixUnaryExpression(node->operator, operand), node)
//                : node;
//        }

    // @api
    auto createPostfixUnaryExpression(shared<Expression> operand, SyntaxKind operatorKind) {
        auto node = createBaseExpression<PostfixUnaryExpression>(SyntaxKind::PostfixUnaryExpression);
        node->operatorKind = operatorKind;
        node->operand = parenthesizerRules::parenthesizeOperandOfPostfixUnary(operand);
        node->transformFlags |= propagateChildFlags(node->operand);
        // Only set this flag for non-generated identifiers and non-"local" names. See the
        // comment in `visitPreOrPostfixUnaryExpression` in module.ts
        if (isIdentifier(node->operand) &&
            !isGeneratedIdentifier(node->operand) &&
            !isLocalName(node->operand)) {
            node->transformFlags |= (int) TransformFlags::ContainsUpdateExpressionForIdentifier;
        }
        return node;
    }

//        // @api
//        function updatePostfixUnaryExpression(node: PostfixUnaryExpression, operand: Expression) {
//            return node->operand != operand
//                ? update(createPostfixUnaryExpression(operand, node->operator), node)
//                : node;
//        }
//
// @api
    shared<BinaryExpression> createBinaryExpression(shared<Expression> left, shared<Node> operatorNode, shared<Expression> right) {
        auto node = createBaseExpression<BinaryExpression>(SyntaxKind::BinaryExpression);
//            auto operatorToken = asToken(operatorNode);
//            auto operatorKind = operatorToken.kind;
//            node->left = parenthesizerRules::parenthesizeLeftSideOfBinary(operatorKind, left);
//            node->operatorToken = operatorToken;
//            node->right = parenthesizerRules::parenthesizeRightSideOfBinary(operatorKind, node->left, right);
//            node->transformFlags |=
//                propagateChildFlags(node->left) |
//                propagateChildFlags(node->operatorToken) |
//                propagateChildFlags(node->right);
//            if (operatorKind == SyntaxKind::QuestionQuestionToken) {
//                node->transformFlags |= (int)TransformFlags::ContainsES2020;
//            }
//            else if (operatorKind == SyntaxKind::EqualsToken) {
//                if (isObjectLiteralExpression(node->left)) {
//                    node->transformFlags |=
//                        (int)TransformFlags::ContainsES2015 |
//                        (int)TransformFlags::ContainsES2018 |
//                        (int)TransformFlags::ContainsDestructuringAssignment |
//                        propagateAssignmentPatternFlags(node->left);
//                }
//                else if (isArrayLiteralExpression(node->left)) {
//                    node->transformFlags |=
//                        (int)TransformFlags::ContainsES2015 |
//                        (int)TransformFlags::ContainsDestructuringAssignment |
//                        propagateAssignmentPatternFlags(node->left);
//                }
//            }
//            else if (operatorKind == SyntaxKind::AsteriskAsteriskToken || operatorKind == SyntaxKind::AsteriskAsteriskEqualsToken) {
//                node->transformFlags |= (int)TransformFlags::ContainsES2016;
//            }
//            else if (isLogicalOrCoalescingAssignmentOperator(operatorKind)) {
//                node->transformFlags |= (int)TransformFlags::ContainsES2021;
//            }
        return node;
    }

//        function propagateAssignmentPatternFlags(node: AssignmentPattern): TransformFlags {
//            if (node->transformFlags & TransformFlags::ContainsObjectRestOrSpread) return TransformFlags::ContainsObjectRestOrSpread;
//            if (node->transformFlags & TransformFlags::ContainsES2018) {
//                // check for nested spread assignments, otherwise '{ x: { a, ...b } = foo } = c'
//                // will not be correctly interpreted by the ES2018 transformer
//                for (auto element of getElementsOfBindingOrAssignmentPattern(node)) {
//                    auto target = getTargetOfBindingOrAssignmentElement(element);
//                    if (target && isAssignmentPattern(target)) {
//                        if (target.transformFlags & TransformFlags::ContainsObjectRestOrSpread) {
//                            return TransformFlags::ContainsObjectRestOrSpread;
//                        }
//                        if (target.transformFlags & TransformFlags::ContainsES2018) {
//                            auto flags = propagateAssignmentPatternFlags(target);
//                            if (flags) return flags;
//                        }
//                    }
//                }
//            }
//            return TransformFlags::None;
//        }
//
//        // @api
//        function updateBinaryExpression(node: BinaryExpression, left: Expression, operator: BinaryOperatorToken, right: Expression) {
//            return node->left != left
//                || node->operatorToken != operator
//                || node->right != right
//                ? update(createBinaryExpression(left, operator, right), node)
//                : node;
//        }

        // @api
        auto createConditionalExpression(shared<Expression> condition, sharedOpt<QuestionToken> questionToken, shared<Expression> whenTrue, sharedOpt<ColonToken> colonToken, shared<Expression> whenFalse) {
            auto node = createBaseExpression<ConditionalExpression>(SyntaxKind::ConditionalExpression);
            node->condition = parenthesizerRules::parenthesizeConditionOfConditionalExpression(condition);
            node->questionToken = questionToken ? questionToken: createToken<QuestionToken>(SyntaxKind::QuestionToken);
            node->whenTrue = parenthesizerRules::parenthesizeBranchOfConditionalExpression(whenTrue);
            node->colonToken = colonToken ? colonToken : createToken<ColonToken>(SyntaxKind::ColonToken);
            node->whenFalse = parenthesizerRules::parenthesizeBranchOfConditionalExpression(whenFalse);
            node->transformFlags |=
                propagateChildFlags(node->condition) |
                propagateChildFlags(node->questionToken) |
                propagateChildFlags(node->whenTrue) |
                propagateChildFlags(node->colonToken) |
                propagateChildFlags(node->whenFalse);
            return node;
        }

//        // @api
//        function updateConditionalExpression(
//            node: ConditionalExpression,
//            condition: Expression,
//            questionToken: Token<SyntaxKind::QuestionToken>,
//            whenTrue: Expression,
//            colonToken: Token<SyntaxKind::ColonToken>,
//            whenFalse: Expression
//        ): ConditionalExpression {
//            return node->condition != condition
//                || node->questionToken != questionToken
//                || node->whenTrue != whenTrue
//                || node->colonToken != colonToken
//                || node->whenFalse != whenFalse
//                ? update(createConditionalExpression(condition, questionToken, whenTrue, colonToken, whenFalse), node)
//                : node;
//        }

    // @api
    shared<TemplateExpression> createTemplateExpression(shared<TemplateHead> head, NodeArray templateSpans) {
        auto node = createBaseExpression<TemplateExpression>(SyntaxKind::TemplateExpression);
        node->head = head;
        node->templateSpans = createNodeArray(templateSpans);
        node->transformFlags |=
                propagateChildFlags(node->head) |
                propagateChildrenFlags(node->templateSpans) |
                (int) TransformFlags::ContainsES2015;
        return node;
    }

//        // @api
//        function updateTemplateExpression(node: TemplateExpression, head: TemplateHead, templateSpans: readonly TemplateSpan[]) {
//            return node->head != head
//                || node->templateSpans != templateSpans
//                ? update(createTemplateExpression(head, templateSpans), node)
//                : node;
//        }
//
//        function createTemplateLiteralLikeNodeChecked(kind: TemplateLiteralToken["kind"], text: string | undefined, rawText: string | undefined, templateFlags = TokenFlags::None) {
//            Debug::asserts(!(templateFlags & ~TokenFlags::TemplateLiteralLikeFlags), "Unsupported template flags.");
//            // NOTE: without the assignment to `undefined`, we don't narrow the initial type of `cooked`.
//            // eslint-disable-next-line no-undef-init
//            let cooked: string | object | undefined = undefined;
//            if (rawText != undefined && rawText != text) {
//                cooked = getCookedText(kind, rawText);
//                if (typeof cooked == "object") {
//                    return Debug.fail("Invalid raw text");
//                }
//            }
//            if (text == undefined) {
//                if (cooked == undefined) {
//                    return Debug.fail("Arguments 'text' and 'rawText' may not both be undefined.");
//                }
//                text = cooked;
//            }
//            else if (cooked != undefined) {
//                Debug::asserts(text == cooked, "Expected argument 'text' to be the normalized (i.e. 'cooked') version of argument 'rawText'.");
//            }
//            return createTemplateLiteralLikeNode(kind, text, rawText, templateFlags);
//        }
//
//        // @api
//        function createTemplateHead(text: string | undefined, rawText?: string, templateFlags?: TokenFlags) {
//            return createTemplateLiteralLikeNodeChecked(SyntaxKind::TemplateHead, text, rawText, templateFlags) as TemplateHead;
//        }
//
//        // @api
//        function createTemplateMiddle(text: string | undefined, rawText?: string, templateFlags?: TokenFlags) {
//            return createTemplateLiteralLikeNodeChecked(SyntaxKind::TemplateMiddle, text, rawText, templateFlags) as TemplateMiddle;
//        }
//
//        // @api
//        function createTemplateTail(text: string | undefined, rawText?: string, templateFlags?: TokenFlags) {
//            return createTemplateLiteralLikeNodeChecked(SyntaxKind::TemplateTail, text, rawText, templateFlags) as TemplateTail;
//        }
//
//        // @api
//        function createNoSubstitutionTemplateLiteral(text: string | undefined, rawText?: string, templateFlags?: TokenFlags) {
//            return createTemplateLiteralLikeNodeChecked(SyntaxKind::NoSubstitutionTemplateLiteral, text, rawText, templateFlags) as NoSubstitutionTemplateLiteral;
//        }

    // @api
    shared<YieldExpression> createYieldExpression(sharedOpt<AsteriskToken> asteriskToken, sharedOpt<Expression> expression) {
        Debug::asserts(!asteriskToken || !!expression, "A `YieldExpression` with an asteriskToken must have an expression.");
        auto node = createBaseExpression<YieldExpression>(SyntaxKind::YieldExpression);
        if (expression) node->expression = parenthesizerRules::parenthesizeExpressionForDisallowedComma(expression);
        node->asteriskToken = asteriskToken;
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                propagateChildFlags(node->asteriskToken) |
                (int) TransformFlags::ContainsES2015 |
                (int) TransformFlags::ContainsES2018 |
                (int) TransformFlags::ContainsYield;
        return node;
    }

//        // @api
//        function updateYieldExpression(node: YieldExpression, sharedOpt<AsteriskToken> asteriskToken, shared<Expression> expression) {
//            return node->expression != expression
//                || node->asteriskToken != asteriskToken
//                ? update(createYieldExpression(asteriskToken, expression), node)
//                : node;
//        }

    // @api
    auto createSpreadElement(shared<Expression> expression) {
        auto node = createBaseExpression<SpreadElement>(SyntaxKind::SpreadElement);
        node->expression = parenthesizerRules::parenthesizeExpressionForDisallowedComma(expression);
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                (int) TransformFlags::ContainsES2015 |
                (int) TransformFlags::ContainsRestOrSpread;
        return node;
    }

//        // @api
//        function updateSpreadElement(node: SpreadElement, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createSpreadElement(expression), node)
//                : node;
//        }

    // @api
    auto createClassExpression(
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NameType name,
            optional<NodeArray> typeParameters,
            optional<NodeArray> heritageClauses,
            NodeArray members
    ) {
        auto node = createBaseClassLikeDeclaration<ClassExpression>(
                SyntaxKind::ClassExpression,
                decorators,
                modifiers,
                name,
                typeParameters,
                heritageClauses,
                members
        );
        node->transformFlags |= (int) TransformFlags::ContainsES2015;
        return node;
    }

//        // @api
//        function updateClassExpression(
//            node: ClassExpression,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: Identifier | undefined,
//            optional<NodeArray> typeParameters,
//            optional<NodeArray> heritageClauses,
//            NodeArray members
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->name != name
//                || node->typeParameters != typeParameters
//                || node->heritageClauses != heritageClauses
//                || node->members != members
//                ? update(createClassExpression(decorators, modifiers, name, typeParameters, heritageClauses, members), node)
//                : node;
//        }

    // @api
    shared<OmittedExpression> createOmittedExpression() {
        return createBaseExpression<OmittedExpression>(SyntaxKind::OmittedExpression);
    }

    // @api
    shared<ExpressionWithTypeArguments> createExpressionWithTypeArguments(shared<Expression> expression, optional<NodeArray> typeArguments) {
        auto node = createBaseNode<ExpressionWithTypeArguments>(SyntaxKind::ExpressionWithTypeArguments);
        node->expression = parenthesizerRules::parenthesizeLeftSideOfAccess(expression);
        if (typeArguments) node->typeArguments = parenthesizerRules::parenthesizeTypeArguments(typeArguments);
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                propagateChildrenFlags(node->typeArguments) |
                (int) TransformFlags::ContainsES2015;
        return node;
    }

//        // @api
//        function updateExpressionWithTypeArguments(node: ExpressionWithTypeArguments, shared<Expression> expression, optional<NodeArray> typeArguments) {
//            return node->expression != expression
//                || node->typeArguments != typeArguments
//                ? update(createExpressionWithTypeArguments(expression, typeArguments), node)
//                : node;
//        }

    // @api
    auto createAsExpression(shared<Expression> expression, shared<TypeNode> type) {
        auto node = createBaseExpression<AsExpression>(SyntaxKind::AsExpression);
        node->expression = expression;
        node->type = type;
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                propagateChildFlags(node->type) |
                (int) TransformFlags::ContainsTypeScript;
        return node;
    }

    // @api
    auto updateAsExpression(shared<AsExpression> node, shared<Expression> expression, shared<TypeNode> type) {
        return node->expression != expression
               || node->type != type
               ? update(createAsExpression(expression, type), node)
               : node;
    }

    // @api
    shared<NonNullExpression> createNonNullExpression(shared<Expression> expression) {
        auto node = createBaseExpression<NonNullExpression>(SyntaxKind::NonNullExpression);
        node->expression = parenthesizerRules::parenthesizeLeftSideOfAccess(expression);
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                (int) TransformFlags::ContainsTypeScript;
        return node;
    }

    // @api
    auto createNonNullChain(shared<Expression> expression) {
        auto node = createBaseExpression<NonNullChain>(SyntaxKind::NonNullExpression);
        node->flags |= (int) NodeFlags::OptionalChain;
        node->expression = parenthesizerRules::parenthesizeLeftSideOfAccess(expression);
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                (int) TransformFlags::ContainsTypeScript;
        return node;
    }

    // @api
    auto updateNonNullChain(shared<NonNullChain> node, shared<Expression> expression) {
        Debug::asserts(!!(node->flags & (int) NodeFlags::OptionalChain), "Cannot update a NonNullExpression using updateNonNullChain. Use updateNonNullExpression instead.");
        return node->expression != expression
               ? update(createNonNullChain(expression), node)
               : node;
    }

    // @api
    auto updateNonNullExpression(shared<NonNullExpression> node, shared<Expression> expression) {
        if (isNonNullChain(node)) {
            return updateNonNullChain(node, expression);
        }
        return node->expression != expression
               ? update(createNonNullExpression(expression), node)
               : node;
    }

    // @api
    auto createMetaProperty(SyntaxKind keywordToken, shared<Identifier> name) {
        auto node = createBaseExpression<MetaProperty>(SyntaxKind::MetaProperty);
        node->keywordToken = keywordToken;
        node->name = name;
        node->transformFlags |= propagateChildFlags(node->name);
        switch (keywordToken) {
            case SyntaxKind::NewKeyword:
                node->transformFlags |= (int) TransformFlags::ContainsES2015;
                break;
            case SyntaxKind::ImportKeyword:
                node->transformFlags |= (int) TransformFlags::ContainsESNext;
                break;
            default:
                throw runtime_error(format("invalid keyword token %d", keywordToken));
        }
        return node;
    }

//        // @api
//        function updateMetaProperty(node: MetaProperty, name: Identifier) {
//            return node->name != name
//                ? update(createMetaProperty(node->keywordToken, name), node)
//                : node;
//        }
//
//        //
//        // Misc
//        //

    // @api
    shared<TemplateSpan> createTemplateSpan(shared<Expression> expression, shared<NodeUnion(TemplateMiddle, TemplateTail)> literal) {
        auto node = createBaseNode<TemplateSpan>(SyntaxKind::TemplateSpan);
        node->expression = expression;
        node->literal = literal;
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                propagateChildFlags(node->literal) |
                (int) TransformFlags::ContainsES2015;
        return node;
    }

//        // @api
//        function updateTemplateSpan(node: TemplateSpan, shared<Expression> expression, literal: TemplateMiddle | TemplateTail) {
//            return node->expression != expression
//                || node->literal != literal
//                ? update(createTemplateSpan(expression, literal), node)
//                : node;
//        }
//
    // @api
    auto createSemicolonClassElement() {
        auto node = createBaseNode<SemicolonClassElement>(SyntaxKind::SemicolonClassElement);
        node->transformFlags |= (int) TransformFlags::ContainsES2015;
        return node;
    }

    //
    // Element
    //

    // @api
    shared<Block> createBlock(NodeArray statements, bool multiLine) {
        auto node = createBaseNode<Block>(SyntaxKind::Block);
        node->statements = createNodeArray(statements);
        node->multiLine = multiLine;
        node->transformFlags |= propagateChildrenFlags(node->statements);
        return node;
    }

//        // @api
//        function updateBlock(node: Block, statements: readonly Statement[]) {
//            return node->statements != statements
//                ? update(createBlock(statements, node->multiLine), node)
//                : node;
//        }
//
//        // @api
//        function createVariableStatement(optional<NodeArray> modifiers, declarationList: VariableDeclarationList | readonly VariableDeclaration[]) {
//            auto node = createBaseDeclaration<VariableStatement>(SyntaxKind::VariableStatement, /*decorators*/ {}, modifiers);
//            node->declarationList = isArray(declarationList) ? createVariableDeclarationList(declarationList) : declarationList;
//            node->transformFlags |=
//                propagateChildFlags(node->declarationList);
//            if (modifiersToFlags(node->modifiers) & ModifierFlags::Ambient) {
//                node->transformFlags = (int)TransformFlags::ContainsTypeScript;
//            }
//            return node;
//        }
//
//        // @api
//        function updateVariableStatement(node: VariableStatement, optional<NodeArray> modifiers, declarationList: VariableDeclarationList) {
//            return node->modifiers != modifiers
//                || node->declarationList != declarationList
//                ? update(createVariableStatement(modifiers, declarationList), node)
//                : node;
//        }

    // @api
    auto createEmptyStatement() {
        return createBaseNode<EmptyStatement>(SyntaxKind::EmptyStatement);
    }

    shared<EmitNode> mergeEmitNode(shared<EmitNode> sourceEmitNode, sharedOpt<EmitNode> destEmitNode) {
        if (!destEmitNode) destEmitNode = make_shared<EmitNode>();
        // We are using `.slice()` here in case `destEmitNode.leadingComments` is pushed to later.
        if (sourceEmitNode->leadingComments) destEmitNode->leadingComments = addRange<SynthesizedComment>(slice(sourceEmitNode->leadingComments), destEmitNode->leadingComments);
        if (sourceEmitNode->trailingComments) destEmitNode->trailingComments = addRange<SynthesizedComment>(slice(sourceEmitNode->trailingComments), destEmitNode->trailingComments);
        if (sourceEmitNode->flags) destEmitNode->flags = sourceEmitNode->flags & ~(int)EmitFlags::Immutable;
        if (sourceEmitNode->commentRange) destEmitNode->commentRange = sourceEmitNode->commentRange;
        if (sourceEmitNode->sourceMapRange) destEmitNode->sourceMapRange = sourceEmitNode->sourceMapRange;
//        if (sourceEmitNode->tokenSourceMapRanges) destEmitNode->tokenSourceMapRanges = mergeTokenSourceMapRanges(sourceEmitNode->tokenSourceMapRanges, destEmitNode->tokenSourceMapRanges!);
//        if (sourceEmitNode->constantValue != false) destEmitNode->constantValue = constantValue;
//        if (sourceEmitNode->helpers) {
//            for (auto helper of helpers) {
//                destEmitNode.helpers = appendIfUnique(destEmitNode.helpers, helper);
//            }
//        }
//        if (startsOnNewLine != undefined) destEmitNode.startsOnNewLine = startsOnNewLine;
        return destEmitNode;
    }

    template<class T>
    T setOriginalNode(T node, sharedOpt<Node> original) {
        node->original = original;
        if (original) {
            auto emitNode = original->emitNode;
            if (emitNode) node->emitNode = mergeEmitNode(emitNode, node->emitNode);
        }
        return node;
    }

    template<class T>
    sharedOpt<T> asEmbeddedStatement(sharedOpt<T> statement) {
        return statement && isNotEmittedStatement(statement) ? setTextRange(setOriginalNode(createEmptyStatement(), statement), statement) : statement;
    }

    // @api
    auto createExpressionStatement(shared<Expression> expression) {
        auto node = createBaseNode<ExpressionStatement>(SyntaxKind::ExpressionStatement);
        node->expression = parenthesizerRules::parenthesizeExpressionOfExpressionStatement(expression);
        node->transformFlags |= propagateChildFlags(node->expression);
        return node;
    }

//        // @api
//        function updateExpressionStatement(node: ExpressionStatement, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createExpressionStatement(expression), node)
//                : node;
//        }
//
//        // @api
//        function createIfStatement(shared<Expression> expression, thenStatement: Statement, elseStatement?: Statement) {
//            auto node = createBaseNode<IfStatement>(SyntaxKind::IfStatement);
//            node->expression = expression;
//            node->thenStatement = asEmbeddedStatement(thenStatement);
//            node->elseStatement = asEmbeddedStatement(elseStatement);
//            node->transformFlags |=
//                propagateChildFlags(node->expression) |
//                propagateChildFlags(node->thenStatement) |
//                propagateChildFlags(node->elseStatement);
//            return node;
//        }
//
//        // @api
//        function updateIfStatement(node: IfStatement, shared<Expression> expression, thenStatement: Statement, elseStatement: Statement | undefined) {
//            return node->expression != expression
//                || node->thenStatement != thenStatement
//                || node->elseStatement != elseStatement
//                ? update(createIfStatement(expression, thenStatement, elseStatement), node)
//                : node;
//        }
//
//        // @api
//        function createDoStatement(statement: Statement, shared<Expression> expression) {
//            auto node = createBaseNode<DoStatement>(SyntaxKind::DoStatement);
//            node->statement = asEmbeddedStatement(statement);
//            node->expression = expression;
//            node->transformFlags |=
//                propagateChildFlags(node->statement) |
//                propagateChildFlags(node->expression);
//            return node;
//        }
//
//        // @api
//        function updateDoStatement(node: DoStatement, statement: Statement, shared<Expression> expression) {
//            return node->statement != statement
//                || node->expression != expression
//                ? update(createDoStatement(statement, expression), node)
//                : node;
//        }
//
//        // @api
//        function createWhileStatement(shared<Expression> expression, statement: Statement) {
//            auto node = createBaseNode<WhileStatement>(SyntaxKind::WhileStatement);
//            node->expression = expression;
//            node->statement = asEmbeddedStatement(statement);
//            node->transformFlags |=
//                propagateChildFlags(node->expression) |
//                propagateChildFlags(node->statement);
//            return node;
//        }
//
//        // @api
//        function updateWhileStatement(node: WhileStatement, shared<Expression> expression, statement: Statement) {
//            return node->expression != expression
//                || node->statement != statement
//                ? update(createWhileStatement(expression, statement), node)
//                : node;
//        }
//
//        // @api
//        function createForStatement(initializer: ForInitializer | undefined, condition: Expression | undefined, incrementor: Expression | undefined, statement: Statement) {
//            auto node = createBaseNode<ForStatement>(SyntaxKind::ForStatement);
//            node->initializer = initializer;
//            node->condition = condition;
//            node->incrementor = incrementor;
//            node->statement = asEmbeddedStatement(statement);
//            node->transformFlags |=
//                propagateChildFlags(node->initializer) |
//                propagateChildFlags(node->condition) |
//                propagateChildFlags(node->incrementor) |
//                propagateChildFlags(node->statement);
//            return node;
//        }
//
//        // @api
//        function updateForStatement(node: ForStatement, initializer: ForInitializer | undefined, condition: Expression | undefined, incrementor: Expression | undefined, statement: Statement) {
//            return node->initializer != initializer
//                || node->condition != condition
//                || node->incrementor != incrementor
//                || node->statement != statement
//                ? update(createForStatement(initializer, condition, incrementor, statement), node)
//                : node;
//        }
//
//        // @api
//        function createForInStatement(initializer: ForInitializer, shared<Expression> expression, statement: Statement) {
//            auto node = createBaseNode<ForInStatement>(SyntaxKind::ForInStatement);
//            node->initializer = initializer;
//            node->expression = expression;
//            node->statement = asEmbeddedStatement(statement);
//            node->transformFlags |=
//                propagateChildFlags(node->initializer) |
//                propagateChildFlags(node->expression) |
//                propagateChildFlags(node->statement);
//            return node;
//        }
//
//        // @api
//        function updateForInStatement(node: ForInStatement, initializer: ForInitializer, shared<Expression> expression, statement: Statement) {
//            return node->initializer != initializer
//                || node->expression != expression
//                || node->statement != statement
//                ? update(createForInStatement(initializer, expression, statement), node)
//                : node;
//        }
//
//        // @api
//        function createForOfStatement(awaitModifier: AwaitKeyword | undefined, initializer: ForInitializer, shared<Expression> expression, statement: Statement) {
//            auto node = createBaseNode<ForOfStatement>(SyntaxKind::ForOfStatement);
//            node->awaitModifier = awaitModifier;
//            node->initializer = initializer;
//            node->expression = parenthesizerRules::parenthesizeExpressionForDisallowedComma(expression);
//            node->statement = asEmbeddedStatement(statement);
//            node->transformFlags |=
//                propagateChildFlags(node->awaitModifier) |
//                propagateChildFlags(node->initializer) |
//                propagateChildFlags(node->expression) |
//                propagateChildFlags(node->statement) |
//                (int)TransformFlags::ContainsES2015;
//            if (awaitModifier) node->transformFlags |= (int)TransformFlags::ContainsES2018;
//            return node;
//        }
//
//        // @api
//        function updateForOfStatement(node: ForOfStatement, awaitModifier: AwaitKeyword | undefined, initializer: ForInitializer, shared<Expression> expression, statement: Statement) {
//            return node->awaitModifier != awaitModifier
//                || node->initializer != initializer
//                || node->expression != expression
//                || node->statement != statement
//                ? update(createForOfStatement(awaitModifier, initializer, expression, statement), node)
//                : node;
//        }
//
//        // @api
//        function createContinueStatement(label?: string | Identifier): ContinueStatement {
//            auto node = createBaseNode<ContinueStatement>(SyntaxKind::ContinueStatement);
//            node->label = asName(label);
//            node->transformFlags |=
//                propagateChildFlags(node->label) |
//                (int)TransformFlags::ContainsHoistedDeclarationOrCompletion;
//            return node;
//        }
//
//        // @api
//        function updateContinueStatement(node: ContinueStatement, label: Identifier | undefined) {
//            return node->label != label
//                ? update(createContinueStatement(label), node)
//                : node;
//        }
//
//        // @api
//        function createBreakStatement(label?: string | Identifier): BreakStatement {
//            auto node = createBaseNode<BreakStatement>(SyntaxKind::BreakStatement);
//            node->label = asName(label);
//            node->transformFlags |=
//                propagateChildFlags(node->label) |
//                (int)TransformFlags::ContainsHoistedDeclarationOrCompletion;
//            return node;
//        }
//
//        // @api
//        function updateBreakStatement(node: BreakStatement, label: Identifier | undefined) {
//            return node->label != label
//                ? update(createBreakStatement(label), node)
//                : node;
//        }
//
//        // @api
//        function createReturnStatement(expression?: Expression): ReturnStatement {
//            auto node = createBaseNode<ReturnStatement>(SyntaxKind::ReturnStatement);
//            node->expression = expression;
//            // return in an ES2018 async generator must be awaited
//            node->transformFlags |=
//                propagateChildFlags(node->expression) |
//                (int)TransformFlags::ContainsES2018 |
//                (int)TransformFlags::ContainsHoistedDeclarationOrCompletion;
//            return node;
//        }
//
//        // @api
//        function updateReturnStatement(node: ReturnStatement, sharedOpt<Expression> expression) {
//            return node->expression != expression
//                ? update(createReturnStatement(expression), node)
//                : node;
//        }
//
//        // @api
//        function createWithStatement(shared<Expression> expression, statement: Statement) {
//            auto node = createBaseNode<WithStatement>(SyntaxKind::WithStatement);
//            node->expression = expression;
//            node->statement = asEmbeddedStatement(statement);
//            node->transformFlags |=
//                propagateChildFlags(node->expression) |
//                propagateChildFlags(node->statement);
//            return node;
//        }
//
//        // @api
//        function updateWithStatement(node: WithStatement, shared<Expression> expression, statement: Statement) {
//            return node->expression != expression
//                || node->statement != statement
//                ? update(createWithStatement(expression, statement), node)
//                : node;
//        }
//
//        // @api
//        function createSwitchStatement(shared<Expression> expression, caseBlock: CaseBlock): SwitchStatement {
//            auto node = createBaseNode<SwitchStatement>(SyntaxKind::SwitchStatement);
//            node->expression = parenthesizerRules::parenthesizeExpressionForDisallowedComma(expression);
//            node->caseBlock = caseBlock;
//            node->transformFlags |=
//                propagateChildFlags(node->expression) |
//                propagateChildFlags(node->caseBlock);
//            return node;
//        }
//
//        // @api
//        function updateSwitchStatement(node: SwitchStatement, shared<Expression> expression, caseBlock: CaseBlock) {
//            return node->expression != expression
//                || node->caseBlock != caseBlock
//                ? update(createSwitchStatement(expression, caseBlock), node)
//                : node;
//        }
//
        // @api
        auto createLabeledStatement(NameType label, shared<Statement> statement) {
            auto node = createBaseNode<LabeledStatement>(SyntaxKind::LabeledStatement);
            node->label = to<Identifier>(asName(label));
            node->statement = asEmbeddedStatement(statement);
            node->transformFlags |=
                propagateChildFlags(node->label) |
                propagateChildFlags(node->statement);
            return node;
        }

//        // @api
//        function updateLabeledStatement(node: LabeledStatement, label: Identifier, statement: Statement) {
//            return node->label != label
//                || node->statement != statement
//                ? update(createLabeledStatement(label, statement), node)
//                : node;
//        }
//
//        // @api
//        function createThrowStatement(shared<Expression> expression) {
//            auto node = createBaseNode<ThrowStatement>(SyntaxKind::ThrowStatement);
//            node->expression = expression;
//            node->transformFlags |= propagateChildFlags(node->expression);
//            return node;
//        }
//
//        // @api
//        function updateThrowStatement(node: ThrowStatement, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createThrowStatement(expression), node)
//                : node;
//        }
//
//        // @api
//        function createTryStatement(tryBlock: Block, catchClause: CatchClause | undefined, finallyBlock: Block | undefined) {
//            auto node = createBaseNode<TryStatement>(SyntaxKind::TryStatement);
//            node->tryBlock = tryBlock;
//            node->catchClause = catchClause;
//            node->finallyBlock = finallyBlock;
//            node->transformFlags |=
//                propagateChildFlags(node->tryBlock) |
//                propagateChildFlags(node->catchClause) |
//                propagateChildFlags(node->finallyBlock);
//            return node;
//        }
//
//        // @api
//        function updateTryStatement(node: TryStatement, tryBlock: Block, catchClause: CatchClause | undefined, finallyBlock: Block | undefined) {
//            return node->tryBlock != tryBlock
//                || node->catchClause != catchClause
//                || node->finallyBlock != finallyBlock
//                ? update(createTryStatement(tryBlock, catchClause, finallyBlock), node)
//                : node;
//        }
//
//        // @api
//        function createDebuggerStatement() {
//            return createBaseNode<DebuggerStatement>(SyntaxKind::DebuggerStatement);
//        }
//
//        // @api
//        function createVariableDeclaration(name: string | BindingName, exclamationToken: ExclamationToken | undefined, sharedOpt<TypeNode> type, sharedOpt<Expression> initializer) {
//            auto node = createBaseVariableLikeDeclaration<VariableDeclaration>(
//                SyntaxKind::VariableDeclaration,
//                /*decorators*/ {},
//                /*modifiers*/ {},
//                name,
//                type,
//                initializer && parenthesizerRules::parenthesizeExpressionForDisallowedComma(initializer)
//            );
//            node->exclamationToken = exclamationToken;
//            node->transformFlags |= propagateChildFlags(node->exclamationToken);
//            if (exclamationToken) {
//                node->transformFlags |= (int)TransformFlags::ContainsTypeScript;
//            }
//            return node;
//        }
//
//        // @api
//        function updateVariableDeclaration(node: VariableDeclaration, name: BindingName, exclamationToken: ExclamationToken | undefined, sharedOpt<TypeNode> type, sharedOpt<Expression> initializer) {
//            return node->name != name
//                || node->type != type
//                || node->exclamationToken != exclamationToken
//                || node->initializer != initializer
//                ? update(createVariableDeclaration(name, exclamationToken, type, initializer), node)
//                : node;
//        }
//
//        // @api
//        function createVariableDeclarationList(declarations: readonly VariableDeclaration[], flags = NodeFlags::None) {
//            auto node = createBaseNode<VariableDeclarationList>(SyntaxKind::VariableDeclarationList);
//            node->flags |= flags & NodeFlags::BlockScoped;
//            node->declarations = createNodeArray(declarations);
//            node->transformFlags |=
//                propagateChildrenFlags(node->declarations) |
//                (int)TransformFlags::ContainsHoistedDeclarationOrCompletion;
//            if (flags & NodeFlags::BlockScoped) {
//                node->transformFlags |=
//                    (int)TransformFlags::ContainsES2015 |
//                    (int)TransformFlags::ContainsBlockScopedBinding;
//            }
//            return node;
//        }
//
//        // @api
//        function updateVariableDeclarationList(node: VariableDeclarationList, declarations: readonly VariableDeclaration[]) {
//            return node->declarations != declarations
//                ? update(createVariableDeclarationList(declarations, node->flags), node)
//                : node;
//        }
//
//        // @api
//        function createFunctionDeclaration(
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            sharedOpt<AsteriskToken> asteriskToken,
//            name: string | Identifier | undefined,
//            optional<NodeArray> typeParameters,
//            NodeArray parameters,
//            sharedOpt<TypeNode> type,
//           sharedOpt<Block> body
//        ) {
//            auto node = createBaseFunctionLikeDeclaration<FunctionDeclaration>(
//                SyntaxKind::FunctionDeclaration,
//                decorators,
//                modifiers,
//                name,
//                typeParameters,
//                parameters,
//                type,
//                body
//            );
//            node->asteriskToken = asteriskToken;
//            if (!node->body || modifiersToFlags(node->modifiers) & ModifierFlags::Ambient) {
//                node->transformFlags = (int)TransformFlags::ContainsTypeScript;
//            }
//            else {
//                node->transformFlags |=
//                    propagateChildFlags(node->asteriskToken) |
//                    (int)TransformFlags::ContainsHoistedDeclarationOrCompletion;
//                if (modifiersToFlags(node->modifiers) & ModifierFlags::Async) {
//                    if (node->asteriskToken) {
//                        node->transformFlags |= (int)TransformFlags::ContainsES2018;
//                    }
//                    else {
//                        node->transformFlags |= (int)TransformFlags::ContainsES2017;
//                    }
//                }
//                else if (node->asteriskToken) {
//                    node->transformFlags |= (int)TransformFlags::ContainsGenerator;
//                }
//            }
//            return node;
//        }
//
//        // @api
//        function updateFunctionDeclaration(
//            node: FunctionDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            sharedOpt<AsteriskToken> asteriskToken,
//            name: Identifier | undefined,
//            optional<NodeArray> typeParameters,
//            NodeArray parameters,
//            sharedOpt<TypeNode> type,
//           sharedOpt<Block> body
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->asteriskToken != asteriskToken
//                || node->name != name
//                || node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                || node->body != body
//                ? updateBaseFunctionLikeDeclaration(createFunctionDeclaration(decorators, modifiers, asteriskToken, name, typeParameters, parameters, type, body), node)
//                : node;
//        }
//
    // @api
    auto createClassDeclaration(
            optional<NodeArray> decorators,
            optional<NodeArray> modifiers,
            NameType name,
            optional<NodeArray> typeParameters,
            optional<NodeArray> heritageClauses,
            NodeArray members
    ) {
        auto node = createBaseClassLikeDeclaration<ClassDeclaration>(
                SyntaxKind::ClassDeclaration,
                decorators,
                modifiers,
                name,
                typeParameters,
                heritageClauses,
                members
        );
        if (modifiersToFlags(node->modifiers) & (int) ModifierFlags::Ambient) {
            node->transformFlags = (int) TransformFlags::ContainsTypeScript;
        } else {
            node->transformFlags |= (int) TransformFlags::ContainsES2015;
            if (node->transformFlags & (int) TransformFlags::ContainsTypeScriptClassSyntax) {
                node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
            }
        }
        return node;
    }

//        // @api
//        function updateClassDeclaration(
//            node: ClassDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: Identifier | undefined,
//            optional<NodeArray> typeParameters,
//            optional<NodeArray> heritageClauses,
//            NodeArray members
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->name != name
//                || node->typeParameters != typeParameters
//                || node->heritageClauses != heritageClauses
//                || node->members != members
//                ? update(createClassDeclaration(decorators, modifiers, name, typeParameters, heritageClauses, members), node)
//                : node;
//        }
//
//        // @api
//        function createInterfaceDeclaration(
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: string | Identifier,
//            optional<NodeArray> typeParameters,
//            optional<NodeArray> heritageClauses,
//            members: readonly TypeElement[]
//        ) {
//            auto node = createBaseInterfaceOrClassLikeDeclaration<InterfaceDeclaration>(
//                SyntaxKind::InterfaceDeclaration,
//                decorators,
//                modifiers,
//                name,
//                typeParameters,
//                heritageClauses
//            );
//            node->members = createNodeArray(members);
//            node->transformFlags = (int)TransformFlags::ContainsTypeScript;
//            return node;
//        }
//
//        // @api
//        function updateInterfaceDeclaration(
//            node: InterfaceDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: Identifier,
//            optional<NodeArray> typeParameters,
//            optional<NodeArray> heritageClauses,
//            members: readonly TypeElement[]
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->name != name
//                || node->typeParameters != typeParameters
//                || node->heritageClauses != heritageClauses
//                || node->members != members
//                ? update(createInterfaceDeclaration(decorators, modifiers, name, typeParameters, heritageClauses, members), node)
//                : node;
//        }
//
//        // @api
//        function createTypeAliasDeclaration(
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: string | Identifier,
//            optional<NodeArray> typeParameters,
//            shared<TypeNode> type
//        ) {
//            auto node = createBaseGenericNamedDeclaration<TypeAliasDeclaration>(
//                SyntaxKind::TypeAliasDeclaration,
//                decorators,
//                modifiers,
//                name,
//                typeParameters
//            );
//            node->type = type;
//            node->transformFlags = (int)TransformFlags::ContainsTypeScript;
//            return node;
//        }
//
//        // @api
//        function updateTypeAliasDeclaration(
//            node: TypeAliasDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: Identifier,
//            optional<NodeArray> typeParameters,
//            shared<TypeNode> type
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->name != name
//                || node->typeParameters != typeParameters
//                || node->type != type
//                ? update(createTypeAliasDeclaration(decorators, modifiers, name, typeParameters, type), node)
//                : node;
//        }
//
//        // @api
//        function createEnumDeclaration(
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: string | Identifier,
//            members: readonly EnumMember[]
//        ) {
//            auto node = createBaseNamedDeclaration<EnumDeclaration>(
//                SyntaxKind::EnumDeclaration,
//                decorators,
//                modifiers,
//                name
//            );
//            node->members = createNodeArray(members);
//            node->transformFlags |=
//                propagateChildrenFlags(node->members) |
//                (int)TransformFlags::ContainsTypeScript;
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // Enum declarations cannot contain `await`
//            return node;
//        }
//
//        // @api
//        function updateEnumDeclaration(
//            node: EnumDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: Identifier,
//            members: readonly EnumMember[]) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->name != name
//                || node->members != members
//                ? update(createEnumDeclaration(decorators, modifiers, name, members), node)
//                : node;
//        }
//
//        // @api
//        function createModuleDeclaration(
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: ModuleName,
//            body: ModuleBody | undefined,
//            flags = NodeFlags::None
//        ) {
//            auto node = createBaseDeclaration<ModuleDeclaration>(
//                SyntaxKind::ModuleDeclaration,
//                decorators,
//                modifiers
//            );
//            node->flags |= flags & (NodeFlags::Namespace | NodeFlags::NestedNamespace | NodeFlags::GlobalAugmentation);
//            node->name = name;
//            node->body = body;
//            if (modifiersToFlags(node->modifiers) & ModifierFlags::Ambient) {
//                node->transformFlags = (int)TransformFlags::ContainsTypeScript;
//            }
//            else {
//                node->transformFlags |=
//                    propagateChildFlags(node->name) |
//                    propagateChildFlags(node->body) |
//                    (int)TransformFlags::ContainsTypeScript;
//            }
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // Module declarations cannot contain `await`.
//            return node;
//        }
//
//        // @api
//        function updateModuleDeclaration(
//            node: ModuleDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            name: ModuleName,
//            body: ModuleBody | undefined
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->name != name
//                || node->body != body
//                ? update(createModuleDeclaration(decorators, modifiers, name, body, node->flags), node)
//                : node;
//        }
//
//        // @api
//        function createModuleBlock(statements: readonly Statement[]) {
//            auto node = createBaseNode<ModuleBlock>(SyntaxKind::ModuleBlock);
//            node->statements = createNodeArray(statements);
//            node->transformFlags |= propagateChildrenFlags(node->statements);
//            return node;
//        }
//
//        // @api
//        function updateModuleBlock(node: ModuleBlock, statements: readonly Statement[]) {
//            return node->statements != statements
//                ? update(createModuleBlock(statements), node)
//                : node;
//        }
//
//        // @api
//        function createCaseBlock(clauses: readonly CaseOrDefaultClause[]): CaseBlock {
//            auto node = createBaseNode<CaseBlock>(SyntaxKind::CaseBlock);
//            node->clauses = createNodeArray(clauses);
//            node->transformFlags |= propagateChildrenFlags(node->clauses);
//            return node;
//        }
//
//        // @api
//        function updateCaseBlock(node: CaseBlock, clauses: readonly CaseOrDefaultClause[]) {
//            return node->clauses != clauses
//                ? update(createCaseBlock(clauses), node)
//                : node;
//        }
//
//        // @api
//        function createNamespaceExportDeclaration(name: string | Identifier) {
//            auto node = createBaseNamedDeclaration<NamespaceExportDeclaration>(
//                SyntaxKind::NamespaceExportDeclaration,
//                /*decorators*/ {},
//                /*modifiers*/ {},
//                name
//            );
//            node->transformFlags = (int)TransformFlags::ContainsTypeScript;
//            return node;
//        }
//
//        // @api
//        function updateNamespaceExportDeclaration(node: NamespaceExportDeclaration, name: Identifier) {
//            return node->name != name
//                ? update(createNamespaceExportDeclaration(name), node)
//                : node;
//        }
//
//        // @api
//        function createImportEqualsDeclaration(
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            isTypeOnly: boolean,
//            name: string | Identifier,
//            moduleReference: ModuleReference
//        ) {
//            auto node = createBaseNamedDeclaration<ImportEqualsDeclaration>(
//                SyntaxKind::ImportEqualsDeclaration,
//                decorators,
//                modifiers,
//                name
//            );
//            node->isTypeOnly = isTypeOnly;
//            node->moduleReference = moduleReference;
//            node->transformFlags |= propagateChildFlags(node->moduleReference);
//            if (!isExternalModuleReference(node->moduleReference)) node->transformFlags |= (int)TransformFlags::ContainsTypeScript;
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // Import= declaration is always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateImportEqualsDeclaration(
//            node: ImportEqualsDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            isTypeOnly: boolean,
//            name: Identifier,
//            moduleReference: ModuleReference
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->isTypeOnly != isTypeOnly
//                || node->name != name
//                || node->moduleReference != moduleReference
//                ? update(createImportEqualsDeclaration(decorators, modifiers, isTypeOnly, name, moduleReference), node)
//                : node;
//        }
//
//        // @api
//        function createImportDeclaration(
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            importClause: ImportClause | undefined,
//            moduleSpecifier: Expression,
//            assertClause: AssertClause | undefined
//        ): ImportDeclaration {
//            auto node = createBaseDeclaration<ImportDeclaration>(
//                SyntaxKind::ImportDeclaration,
//                decorators,
//                modifiers
//            );
//            node->importClause = importClause;
//            node->moduleSpecifier = moduleSpecifier;
//            node->assertClause = assertClause;
//            node->transformFlags |=
//                propagateChildFlags(node->importClause) |
//                propagateChildFlags(node->moduleSpecifier);
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateImportDeclaration(
//            node: ImportDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            importClause: ImportClause | undefined,
//            moduleSpecifier: Expression,
//            assertClause: AssertClause | undefined
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->importClause != importClause
//                || node->moduleSpecifier != moduleSpecifier
//                || node->assertClause != assertClause
//                ? update(createImportDeclaration(decorators, modifiers, importClause, moduleSpecifier, assertClause), node)
//                : node;
//        }
//
//        // @api
//        function createImportClause(isTypeOnly: boolean, name: Identifier | undefined, namedBindings: NamedImportBindings | undefined): ImportClause {
//            auto node = createBaseNode<ImportClause>(SyntaxKind::ImportClause);
//            node->isTypeOnly = isTypeOnly;
//            node->name = name;
//            node->namedBindings = namedBindings;
//            node->transformFlags |=
//                propagateChildFlags(node->name) |
//                propagateChildFlags(node->namedBindings);
//            if (isTypeOnly) {
//                node->transformFlags |= (int)TransformFlags::ContainsTypeScript;
//            }
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateImportClause(node: ImportClause, isTypeOnly: boolean, name: Identifier | undefined, namedBindings: NamedImportBindings | undefined) {
//            return node->isTypeOnly != isTypeOnly
//                || node->name != name
//                || node->namedBindings != namedBindings
//                ? update(createImportClause(isTypeOnly, name, namedBindings), node)
//                : node;
//        }
//
    // @api
    auto createAssertClause(NodeArray elements, bool multiLine) {
        auto node = createBaseNode<AssertClause>(SyntaxKind::AssertClause);
        node->elements = createNodeArray(elements);
        node->multiLine = multiLine;
        node->transformFlags |= (int) TransformFlags::ContainsESNext;
        return node;
    }

//        // @api
//        function updateAssertClause(node: AssertClause, elements: readonly AssertEntry[], multiLine?: boolean): AssertClause {
//            return node->elements != elements
//                || node->multiLine != multiLine
//                ? update(createAssertClause(elements, multiLine), node)
//                : node;
//        }
//
    // @api
    auto createAssertEntry(shared<NodeUnion(AssertionKey)> name, shared<Expression> value) {
        auto node = createBaseNode<AssertEntry>(SyntaxKind::AssertEntry);
        node->name = name;
        node->value = value;
        node->transformFlags |= (int) TransformFlags::ContainsESNext;
        return node;
    }

//        // @api
//        function updateAssertEntry(node: AssertEntry, name: AssertionKey, value: Expression): AssertEntry {
//            return node->name != name
//                || node->value != value
//                ? update(createAssertEntry(name, value), node)
//                : node;
//        }

    // @api
    auto createImportTypeAssertionContainer(shared<AssertClause> clause, bool multiLine) {
        auto node = createBaseNode<ImportTypeAssertionContainer>(SyntaxKind::ImportTypeAssertionContainer);
        node->assertClause = clause;
        node->multiLine = multiLine;
        return node;
    }

//        // @api
//        function updateImportTypeAssertionContainer(node: ImportTypeAssertionContainer, clause: AssertClause, multiLine?: boolean): ImportTypeAssertionContainer {
//            return node->assertClause != clause
//                || node->multiLine != multiLine
//                ? update(createImportTypeAssertionContainer(clause, multiLine), node)
//                : node;
//        }
//
//        // @api
//        function createNamespaceImport(name: Identifier): NamespaceImport {
//            auto node = createBaseNode<NamespaceImport>(SyntaxKind::NamespaceImport);
//            node->name = name;
//            node->transformFlags |= propagateChildFlags(node->name);
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateNamespaceImport(node: NamespaceImport, name: Identifier) {
//            return node->name != name
//                ? update(createNamespaceImport(name), node)
//                : node;
//        }
//
//        // @api
//        function createNamespaceExport(name: Identifier): NamespaceExport {
//            auto node = createBaseNode<NamespaceExport>(SyntaxKind::NamespaceExport);
//            node->name = name;
//            node->transformFlags |=
//                propagateChildFlags(node->name) |
//                (int)TransformFlags::ContainsESNext;
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateNamespaceExport(node: NamespaceExport, name: Identifier) {
//            return node->name != name
//                ? update(createNamespaceExport(name), node)
//                : node;
//        }
//
//        // @api
//        function createNamedImports(elements: readonly ImportSpecifier[]): NamedImports {
//            auto node = createBaseNode<NamedImports>(SyntaxKind::NamedImports);
//            node->elements = createNodeArray(elements);
//            node->transformFlags |= propagateChildrenFlags(node->elements);
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateNamedImports(node: NamedImports, elements: readonly ImportSpecifier[]) {
//            return node->elements != elements
//                ? update(createNamedImports(elements), node)
//                : node;
//        }
//
//        // @api
//        function createImportSpecifier(isTypeOnly: boolean, propertyName: Identifier | undefined, name: Identifier) {
//            auto node = createBaseNode<ImportSpecifier>(SyntaxKind::ImportSpecifier);
//            node->isTypeOnly = isTypeOnly;
//            node->propertyName = propertyName;
//            node->name = name;
//            node->transformFlags |=
//                propagateChildFlags(node->propertyName) |
//                propagateChildFlags(node->name);
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateImportSpecifier(node: ImportSpecifier, isTypeOnly: boolean, propertyName: Identifier | undefined, name: Identifier) {
//            return node->isTypeOnly != isTypeOnly
//                || node->propertyName != propertyName
//                || node->name != name
//                ? update(createImportSpecifier(isTypeOnly, propertyName, name), node)
//                : node;
//        }
//
//        // @api
//        function createExportAssignment(
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            isExportEquals: boolean | undefined,
//            shared<Expression> expression
//        ) {
//            auto node = createBaseDeclaration<ExportAssignment>(
//                SyntaxKind::ExportAssignment,
//                decorators,
//                modifiers
//            );
//            node->isExportEquals = isExportEquals;
//            node->expression = isExportEquals
//                ? parenthesizerRules::parenthesizeRightSideOfBinary(SyntaxKind::EqualsToken, /*leftSide*/ {}, expression)
//                : parenthesizerRules::parenthesizeExpressionOfExportDefault(expression);
//            node->transformFlags |= propagateChildFlags(node->expression);
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateExportAssignment(
//            node: ExportAssignment,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            shared<Expression> expression
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->expression != expression
//                ? update(createExportAssignment(decorators, modifiers, node->isExportEquals, expression), node)
//                : node;
//        }
//
//        // @api
//        function createExportDeclaration(
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            isTypeOnly: boolean,
//            exportClause: NamedExportBindings | undefined,
//            moduleSpecifier?: Expression,
//            assertClause?: AssertClause
//        ) {
//            auto node = createBaseDeclaration<ExportDeclaration>(
//                SyntaxKind::ExportDeclaration,
//                decorators,
//                modifiers
//            );
//            node->isTypeOnly = isTypeOnly;
//            node->exportClause = exportClause;
//            node->moduleSpecifier = moduleSpecifier;
//            node->assertClause = assertClause;
//            node->transformFlags |=
//                propagateChildFlags(node->exportClause) |
//                propagateChildFlags(node->moduleSpecifier);
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateExportDeclaration(
//            node: ExportDeclaration,
//            optional<NodeArray> decorators,
//            optional<NodeArray> modifiers,
//            isTypeOnly: boolean,
//            exportClause: NamedExportBindings | undefined,
//            moduleSpecifier: Expression | undefined,
//            assertClause: AssertClause | undefined
//        ) {
//            return node->decorators != decorators
//                || node->modifiers != modifiers
//                || node->isTypeOnly != isTypeOnly
//                || node->exportClause != exportClause
//                || node->moduleSpecifier != moduleSpecifier
//                || node->assertClause != assertClause
//                ? update(createExportDeclaration(decorators, modifiers, isTypeOnly, exportClause, moduleSpecifier, assertClause), node)
//                : node;
//        }
//
//        // @api
//        function createNamedExports(elements: readonly ExportSpecifier[]) {
//            auto node = createBaseNode<NamedExports>(SyntaxKind::NamedExports);
//            node->elements = createNodeArray(elements);
//            node->transformFlags |= propagateChildrenFlags(node->elements);
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateNamedExports(node: NamedExports, elements: readonly ExportSpecifier[]) {
//            return node->elements != elements
//                ? update(createNamedExports(elements), node)
//                : node;
//        }
//
//        // @api
//        function createExportSpecifier(isTypeOnly: boolean, propertyName: string | Identifier | undefined, name: string | Identifier) {
//            auto node = createBaseNode<ExportSpecifier>(SyntaxKind::ExportSpecifier);
//            node->isTypeOnly = isTypeOnly;
//            node->propertyName = asName(propertyName);
//            node->name = asName(name);
//            node->transformFlags |=
//                propagateChildFlags(node->propertyName) |
//                propagateChildFlags(node->name);
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateExportSpecifier(node: ExportSpecifier, isTypeOnly: boolean, propertyName: Identifier | undefined, name: Identifier) {
//            return node->isTypeOnly != isTypeOnly
//                || node->propertyName != propertyName
//                || node->name != name
//                ? update(createExportSpecifier(isTypeOnly, propertyName, name), node)
//                : node;
//        }
//
// @api
    shared<MissingDeclaration> createMissingDeclaration() {
        auto node = createBaseDeclaration<MissingDeclaration>(
                SyntaxKind::MissingDeclaration,
                /*decorators*/ {},
                /*modifiers*/ {}
        );
        return node;
    }

//        //
//        // Module references
//        //
//
//        // @api
//        function createExternalModuleReference(shared<Expression> expression) {
//            auto node = createBaseNode<ExternalModuleReference>(SyntaxKind::ExternalModuleReference);
//            node->expression = expression;
//            node->transformFlags |= propagateChildFlags(node->expression);
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateExternalModuleReference(node: ExternalModuleReference, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createExternalModuleReference(expression), node)
//                : node;
//        }
//
//        //
//        // JSDoc
//        //
//
//        // @api
//        // createJSDocAllType
//        // createJSDocUnknownType
//        function createJSDocPrimaryTypeWorker<T extends JSDocType>(kind: T["kind"]) {
//            return createBaseNode(kind);
//        }
//
//        // @api
//        // createJSDocNullableType
//        // createJSDocNonNullableType
//        function createJSDocPrePostfixUnaryTypeWorker<T extends JSDocType & { readonly sharedOpt<TypeNode> type; readonly postfix: boolean }>(SyntaxKind kind, type: T["type"], postfix = false): T {
//            auto node = createJSDocUnaryTypeWorker(
//                kind,
//                postfix ? type && parenthesizerRules::parenthesizeNonArrayTypeOfPostfixType(type) : type
//            ) as Mutable<T>;
//            node->postfix = postfix;
//            return node;
//        }
//
//        // @api
//        // createJSDocOptionalType
//        // createJSDocVariadicType
//        // createJSDocNamepathType
//        function createJSDocUnaryTypeWorker<T extends JSDocType & { readonly sharedOpt<TypeNode> type; }>(SyntaxKind kind, type: T["type"]): T {
//            auto node = createBaseNode<T>(kind);
//            node->type = type;
//            return node;
//        }
//
//        // @api
//        // updateJSDocNonNullableType
//        // updateJSDocNullableType
//        function updateJSDocPrePostfixUnaryTypeWorker<T extends JSDocType & { readonly sharedOpt<TypeNode> type; readonly postfix: boolean; }>(SyntaxKind kind, node: T, type: T["type"]): T {
//            return node->type != type
//            ? update(createJSDocPrePostfixUnaryTypeWorker(kind, type, node->postfix), node)
//            : node;
//        }
//
//        // @api
//        // updateJSDocOptionalType
//        // updateJSDocVariadicType
//        // updateJSDocNamepathType
//        function updateJSDocUnaryTypeWorker<T extends JSDocType & { readonly sharedOpt<TypeNode> type; }>(SyntaxKind kind, node: T, type: T["type"]): T {
//            return node->type != type
//                ? update(createJSDocUnaryTypeWorker(kind, type), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocFunctionType(NodeArray parameters, sharedOpt<TypeNode> type): JSDocFunctionType {
//            auto node = createBaseSignatureDeclaration<JSDocFunctionType>(
//                SyntaxKind::JSDocFunctionType,
//                /*decorators*/ {},
//                /*modifiers*/ {},
//                /*name*/ {},
//                /*typeParameters*/ {},
//                parameters,
//                type
//            );
//            return node;
//        }
//
//        // @api
//        function updateJSDocFunctionType(node: JSDocFunctionType, NodeArray parameters, sharedOpt<TypeNode> type): JSDocFunctionType {
//            return node->parameters != parameters
//                || node->type != type
//                ? update(createJSDocFunctionType(parameters, type), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocTypeLiteral(propertyTags?: readonly JSDocPropertyLikeTag[], isArrayType = false): JSDocTypeLiteral {
//            auto node = createBaseNode<JSDocTypeLiteral>(SyntaxKind::JSDocTypeLiteral);
//            node->jsDocPropertyTags = asNodeArray(propertyTags);
//            node->isArrayType = isArrayType;
//            return node;
//        }
//
//        // @api
//        function updateJSDocTypeLiteral(node: JSDocTypeLiteral, propertyTags: readonly JSDocPropertyLikeTag[] | undefined, isArrayType: boolean): JSDocTypeLiteral {
//            return node->jsDocPropertyTags != propertyTags
//                || node->isArrayType != isArrayType
//                ? update(createJSDocTypeLiteral(propertyTags, isArrayType), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocTypeExpression(shared<TypeNode> type): JSDocTypeExpression {
//            auto node = createBaseNode<JSDocTypeExpression>(SyntaxKind::JSDocTypeExpression);
//            node->type = type;
//            return node;
//        }
//
//        // @api
//        function updateJSDocTypeExpression(node: JSDocTypeExpression, shared<TypeNode> type): JSDocTypeExpression {
//            return node->type != type
//                ? update(createJSDocTypeExpression(type), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocSignature(typeParameters: readonly JSDocTemplateTag[] | undefined, parameters: readonly JSDocParameterTag[], type?: JSDocReturnTag): JSDocSignature {
//            auto node = createBaseNode<JSDocSignature>(SyntaxKind::JSDocSignature);
//            node->typeParameters = asNodeArray(typeParameters);
//            node->parameters = createNodeArray(parameters);
//            node->type = type;
//            return node;
//        }
//
//        // @api
//        function updateJSDocSignature(node: JSDocSignature, typeParameters: readonly JSDocTemplateTag[] | undefined, parameters: readonly JSDocParameterTag[], type: JSDocReturnTag | undefined): JSDocSignature {
//            return node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                ? update(createJSDocSignature(typeParameters, parameters, type), node)
//                : node;
//        }
//
//        function getDefaultTagName(node: JSDocTag) {
//            auto defaultTagName = getDefaultTagNameForKind(node->kind);
//            return node->tagName.escapedText == escapeLeadingUnderscores(defaultTagName)
//                ? node->tagName
//                : createIdentifier(defaultTagName);
//        }
//
//        // @api
//        function createBaseJSDocTag<T extends JSDocTag>(SyntaxKind kind, tagName: Identifier, comment: string | NodeArray<JSDocComment> | undefined) {
//            auto node = createBaseNode<T>(kind);
//            node->tagName = tagName;
//            node->comment = comment;
//            return node;
//        }
//
//        // @api
//        function createJSDocTemplateTag(tagName: Identifier | undefined, constraint: JSDocTypeExpression | undefined, typeParameters: readonly TypeParameterDeclaration[], comment?: string | NodeArray<JSDocComment>): JSDocTemplateTag {
//            auto node = createBaseJSDocTag<JSDocTemplateTag>(SyntaxKind::JSDocTemplateTag, tagName ?? createIdentifier("template"), comment);
//            node->constraint = constraint;
//            node->typeParameters = createNodeArray(typeParameters);
//            return node;
//        }
//
//        // @api
//        function updateJSDocTemplateTag(node: JSDocTemplateTag, tagName: Identifier = getDefaultTagName(node), constraint: JSDocTypeExpression | undefined, typeParameters: readonly TypeParameterDeclaration[], comment: string | NodeArray<JSDocComment> | undefined): JSDocTemplateTag {
//            return node->tagName != tagName
//                || node->constraint != constraint
//                || node->typeParameters != typeParameters
//                || node->comment != comment
//                ? update(createJSDocTemplateTag(tagName, constraint, typeParameters, comment), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocTypedefTag(tagName: Identifier | undefined, typeExpression?: JSDocTypeExpression, fullName?: Identifier | JSDocNamespaceDeclaration, comment?: string | NodeArray<JSDocComment>): JSDocTypedefTag {
//            auto node = createBaseJSDocTag<JSDocTypedefTag>(SyntaxKind::JSDocTypedefTag, tagName ?? createIdentifier("typedef"), comment);
//            node->typeExpression = typeExpression;
//            node->fullName = fullName;
//            node->name = getJSDocTypeAliasName(fullName);
//            return node;
//        }
//
//        // @api
//        function updateJSDocTypedefTag(node: JSDocTypedefTag, tagName: Identifier = getDefaultTagName(node), typeExpression: JSDocTypeExpression | undefined, fullName: Identifier | JSDocNamespaceDeclaration | undefined, comment: string | NodeArray<JSDocComment> | undefined): JSDocTypedefTag {
//            return node->tagName != tagName
//                || node->typeExpression != typeExpression
//                || node->fullName != fullName
//                || node->comment != comment
//                ? update(createJSDocTypedefTag(tagName, typeExpression, fullName, comment), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocParameterTag(tagName: Identifier | undefined, name: EntityName, isBracketed: boolean, typeExpression?: JSDocTypeExpression, isNameFirst?: boolean, comment?: string | NodeArray<JSDocComment>): JSDocParameterTag {
//            auto node = createBaseJSDocTag<JSDocParameterTag>(SyntaxKind::JSDocParameterTag, tagName ?? createIdentifier("param"), comment);
//            node->typeExpression = typeExpression;
//            node->name = name;
//            node->isNameFirst = !!isNameFirst;
//            node->isBracketed = isBracketed;
//            return node;
//        }
//
//        // @api
//        function updateJSDocParameterTag(node: JSDocParameterTag, tagName: Identifier = getDefaultTagName(node), name: EntityName, isBracketed: boolean, typeExpression: JSDocTypeExpression | undefined, isNameFirst: boolean, comment: string | NodeArray<JSDocComment> | undefined): JSDocParameterTag {
//            return node->tagName != tagName
//                || node->name != name
//                || node->isBracketed != isBracketed
//                || node->typeExpression != typeExpression
//                || node->isNameFirst != isNameFirst
//                || node->comment != comment
//                ? update(createJSDocParameterTag(tagName, name, isBracketed, typeExpression, isNameFirst, comment), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocPropertyTag(tagName: Identifier | undefined, name: EntityName, isBracketed: boolean, typeExpression?: JSDocTypeExpression, isNameFirst?: boolean, comment?: string | NodeArray<JSDocComment>): JSDocPropertyTag {
//            auto node = createBaseJSDocTag<JSDocPropertyTag>(SyntaxKind::JSDocPropertyTag, tagName ?? createIdentifier("prop"), comment);
//            node->typeExpression = typeExpression;
//            node->name = name;
//            node->isNameFirst = !!isNameFirst;
//            node->isBracketed = isBracketed;
//            return node;
//        }
//
//        // @api
//        function updateJSDocPropertyTag(node: JSDocPropertyTag, tagName: Identifier = getDefaultTagName(node), name: EntityName, isBracketed: boolean, typeExpression: JSDocTypeExpression | undefined, isNameFirst: boolean, comment: string | NodeArray<JSDocComment> | undefined): JSDocPropertyTag {
//            return node->tagName != tagName
//                || node->name != name
//                || node->isBracketed != isBracketed
//                || node->typeExpression != typeExpression
//                || node->isNameFirst != isNameFirst
//                || node->comment != comment
//                ? update(createJSDocPropertyTag(tagName, name, isBracketed, typeExpression, isNameFirst, comment), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocCallbackTag(tagName: Identifier | undefined, typeExpression: JSDocSignature, fullName?: Identifier | JSDocNamespaceDeclaration, comment?: string | NodeArray<JSDocComment>): JSDocCallbackTag {
//            auto node = createBaseJSDocTag<JSDocCallbackTag>(SyntaxKind::JSDocCallbackTag, tagName ?? createIdentifier("callback"), comment);
//            node->typeExpression = typeExpression;
//            node->fullName = fullName;
//            node->name = getJSDocTypeAliasName(fullName);
//            return node;
//        }
//
//        // @api
//        function updateJSDocCallbackTag(node: JSDocCallbackTag, tagName: Identifier = getDefaultTagName(node), typeExpression: JSDocSignature, fullName: Identifier | JSDocNamespaceDeclaration | undefined, comment: string | NodeArray<JSDocComment> | undefined): JSDocCallbackTag {
//            return node->tagName != tagName
//                || node->typeExpression != typeExpression
//                || node->fullName != fullName
//                || node->comment != comment
//                ? update(createJSDocCallbackTag(tagName, typeExpression, fullName, comment), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocAugmentsTag(tagName: Identifier | undefined, className: JSDocAugmentsTag["class"], comment?: string | NodeArray<JSDocComment>): JSDocAugmentsTag {
//            auto node = createBaseJSDocTag<JSDocAugmentsTag>(SyntaxKind::JSDocAugmentsTag, tagName ?? createIdentifier("augments"), comment);
//            node->class = className;
//            return node;
//        }
//
//        // @api
//        function updateJSDocAugmentsTag(node: JSDocAugmentsTag, tagName: Identifier = getDefaultTagName(node), className: JSDocAugmentsTag["class"], comment: string | NodeArray<JSDocComment> | undefined): JSDocAugmentsTag {
//            return node->tagName != tagName
//                || node->class != className
//                || node->comment != comment
//                ? update(createJSDocAugmentsTag(tagName, className, comment), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocImplementsTag(tagName: Identifier | undefined, className: JSDocImplementsTag["class"], comment?: string | NodeArray<JSDocComment>): JSDocImplementsTag {
//            auto node = createBaseJSDocTag<JSDocImplementsTag>(SyntaxKind::JSDocImplementsTag, tagName ?? createIdentifier("implements"), comment);
//            node->class = className;
//            return node;
//        }
//
//        // @api
//        function createJSDocSeeTag(tagName: Identifier | undefined, name: JSDocNameReference | undefined, comment?: string | NodeArray<JSDocComment>): JSDocSeeTag {
//            auto node = createBaseJSDocTag<JSDocSeeTag>(SyntaxKind::JSDocSeeTag, tagName ?? createIdentifier("see"), comment);
//            node->name = name;
//            return node;
//        }
//
//        // @api
//        function updateJSDocSeeTag(node: JSDocSeeTag, tagName: Identifier | undefined, name: JSDocNameReference | undefined, comment?: string | NodeArray<JSDocComment>): JSDocSeeTag {
//            return node->tagName != tagName
//                || node->name != name
//                || node->comment != comment
//                ? update(createJSDocSeeTag(tagName, name, comment), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocNameReference(name: EntityName | JSDocMemberName): JSDocNameReference {
//            auto node = createBaseNode<JSDocNameReference>(SyntaxKind::JSDocNameReference);
//            node->name = name;
//            return node;
//        }
//
//        // @api
//        function updateJSDocNameReference(node: JSDocNameReference, name: EntityName | JSDocMemberName): JSDocNameReference {
//            return node->name != name
//                ? update(createJSDocNameReference(name), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocMemberName(left: EntityName | JSDocMemberName, right: Identifier) {
//            auto node = createBaseNode<JSDocMemberName>(SyntaxKind::JSDocMemberName);
//            node->left = left;
//            node->right = right;
//            node->transformFlags |=
//                propagateChildFlags(node->left) |
//                propagateChildFlags(node->right);
//            return node;
//        }
//
//        // @api
//        function updateJSDocMemberName(node: JSDocMemberName, left: EntityName | JSDocMemberName, right: Identifier) {
//            return node->left != left
//                || node->right != right
//                ? update(createJSDocMemberName(left, right), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocLink(name: EntityName | JSDocMemberName | undefined, text: string): JSDocLink {
//            auto node = createBaseNode<JSDocLink>(SyntaxKind::JSDocLink);
//            node->name = name;
//            node->text = text;
//            return node;
//        }
//
//        // @api
//        function updateJSDocLink(node: JSDocLink, name: EntityName | JSDocMemberName | undefined, text: string): JSDocLink {
//            return node->name != name
//                ? update(createJSDocLink(name, text), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocLinkCode(name: EntityName | JSDocMemberName | undefined, text: string): JSDocLinkCode {
//            auto node = createBaseNode<JSDocLinkCode>(SyntaxKind::JSDocLinkCode);
//            node->name = name;
//            node->text = text;
//            return node;
//        }
//
//        // @api
//        function updateJSDocLinkCode(node: JSDocLinkCode, name: EntityName | JSDocMemberName | undefined, text: string): JSDocLinkCode {
//            return node->name != name
//                ? update(createJSDocLinkCode(name, text), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocLinkPlain(name: EntityName | JSDocMemberName | undefined, text: string): JSDocLinkPlain {
//            auto node = createBaseNode<JSDocLinkPlain>(SyntaxKind::JSDocLinkPlain);
//            node->name = name;
//            node->text = text;
//            return node;
//        }
//
//        // @api
//        function updateJSDocLinkPlain(node: JSDocLinkPlain, name: EntityName | JSDocMemberName | undefined, text: string): JSDocLinkPlain {
//            return node->name != name
//                ? update(createJSDocLinkPlain(name, text), node)
//                : node;
//        }
//
//        // @api
//        function updateJSDocImplementsTag(node: JSDocImplementsTag, tagName: Identifier = getDefaultTagName(node), className: JSDocImplementsTag["class"], comment: string | NodeArray<JSDocComment> | undefined): JSDocImplementsTag {
//            return node->tagName != tagName
//                || node->class != className
//                || node->comment != comment
//                ? update(createJSDocImplementsTag(tagName, className, comment), node)
//                : node;
//        }
//
//        // @api
//        // createJSDocAuthorTag
//        // createJSDocClassTag
//        // createJSDocPublicTag
//        // createJSDocPrivateTag
//        // createJSDocProtectedTag
//        // createJSDocReadonlyTag
//        // createJSDocDeprecatedTag
//        function createJSDocSimpleTagWorker<T extends JSDocTag>(SyntaxKind kind, tagName: Identifier | undefined, comment?: string | NodeArray<JSDocComment>) {
//            auto node = createBaseJSDocTag<T>(kind, tagName ?? createIdentifier(getDefaultTagNameForKind(kind)), comment);
//            return node;
//        }
//
//        // @api
//        // updateJSDocAuthorTag
//        // updateJSDocClassTag
//        // updateJSDocPublicTag
//        // updateJSDocPrivateTag
//        // updateJSDocProtectedTag
//        // updateJSDocReadonlyTag
//        // updateJSDocDeprecatedTag
//        function updateJSDocSimpleTagWorker<T extends JSDocTag>(SyntaxKind kind, node: T, tagName: Identifier = getDefaultTagName(node), comment: string | NodeArray<JSDocComment> | undefined) {
//            return node->tagName != tagName
//                || node->comment != comment
//                ? update(createJSDocSimpleTagWorker(kind, tagName, comment), node) :
//                node;
//        }
//
//        // @api
//        // createJSDocTypeTag
//        // createJSDocReturnTag
//        // createJSDocThisTag
//        // createJSDocEnumTag
//        function createJSDocTypeLikeTagWorker<T extends JSDocTag & { typeExpression?: JSDocTypeExpression }>(SyntaxKind kind, tagName: Identifier | undefined, typeExpression?: JSDocTypeExpression, comment?: string | NodeArray<JSDocComment>) {
//            auto node = createBaseJSDocTag<T>(kind, tagName ?? createIdentifier(getDefaultTagNameForKind(kind)), comment);
//            node->typeExpression = typeExpression;
//            return node;
//        }
//
//        // @api
//        // updateJSDocTypeTag
//        // updateJSDocReturnTag
//        // updateJSDocThisTag
//        // updateJSDocEnumTag
//        function updateJSDocTypeLikeTagWorker<T extends JSDocTag & { typeExpression?: JSDocTypeExpression }>(SyntaxKind kind, node: T, tagName: Identifier = getDefaultTagName(node), typeExpression: JSDocTypeExpression | undefined, comment: string | NodeArray<JSDocComment> | undefined) {
//            return node->tagName != tagName
//                || node->typeExpression != typeExpression
//                || node->comment != comment
//                ? update(createJSDocTypeLikeTagWorker(kind, tagName, typeExpression, comment), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocUnknownTag(tagName: Identifier, comment?: string | NodeArray<JSDocComment>): JSDocUnknownTag {
//            auto node = createBaseJSDocTag<JSDocUnknownTag>(SyntaxKind::JSDocTag, tagName, comment);
//            return node;
//        }
//
//        // @api
//        function updateJSDocUnknownTag(node: JSDocUnknownTag, tagName: Identifier, comment: string | NodeArray<JSDocComment> | undefined): JSDocUnknownTag {
//            return node->tagName != tagName
//                || node->comment != comment
//                ? update(createJSDocUnknownTag(tagName, comment), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocText(text: string): JSDocText {
//            auto node = createBaseNode<JSDocText>(SyntaxKind::JSDocText);
//            node->text = text;
//            return node;
//        }
//
//        // @api
//        function updateJSDocText(node: JSDocText, text: string): JSDocText {
//            return node->text != text
//                ? update(createJSDocText(text), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocComment(comment?: string | NodeArray<JSDocComment> | undefined, tags?: readonly JSDocTag[] | undefined) {
//            auto node = createBaseNode<JSDoc>(SyntaxKind::JSDoc);
//            node->comment = comment;
//            node->tags = asNodeArray(tags);
//            return node;
//        }
//
//        // @api
//        function updateJSDocComment(node: JSDoc, comment: string | NodeArray<JSDocComment> | undefined, tags: readonly JSDocTag[] | undefined) {
//            return node->comment != comment
//                || node->tags != tags
//                ? update(createJSDocComment(comment, tags), node)
//                : node;
//        }
//
//        //
//        // JSX
//        //
//
// @api
    shared<JsxElement> createJsxElement(shared<JsxOpeningElement> openingElement, NodeArray children, shared<JsxClosingElement> closingElement) {
        auto node = createBaseNode<JsxElement>(SyntaxKind::JsxElement);
        node->openingElement = openingElement;
        node->children = createNodeArray(children);
        node->closingElement = closingElement;
        node->transformFlags |=
                propagateChildFlags(node->openingElement) |
                propagateChildrenFlags(node->children) |
                propagateChildFlags(node->closingElement) |
                (int) TransformFlags::ContainsJsx;
        return node;
    }

//        // @api
//        function updateJsxElement(node: JsxElement, openingElement: JsxOpeningElement, children: readonly JsxChild[], closingElement: JsxClosingElement) {
//            return node->openingElement != openingElement
//                || node->children != children
//                || node->closingElement != closingElement
//                ? update(createJsxElement(openingElement, children, closingElement), node)
//                : node;
//        }

// @api
    shared<JsxSelfClosingElement> createJsxSelfClosingElement(shared<NodeUnion(JsxTagNameExpression)> tagName, optional<NodeArray> typeArguments, shared<JsxAttributes> attributes) {
        auto node = createBaseNode<JsxSelfClosingElement>(SyntaxKind::JsxSelfClosingElement);
        node->tagName = tagName;
        node->typeArguments = asNodeArray(typeArguments);
        node->attributes = attributes;
        node->transformFlags |=
                propagateChildFlags(node->tagName) |
                propagateChildrenFlags(node->typeArguments) |
                propagateChildFlags(node->attributes) |
                (int) TransformFlags::ContainsJsx;
        if (node->typeArguments) {
            node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        }
        return node;
    }
//
//        // @api
//        function updateJsxSelfClosingElement(node: JsxSelfClosingElement, tagName: JsxTagNameExpression, optional<NodeArray> typeArguments, attributes: JsxAttributes) {
//            return node->tagName != tagName
//                || node->typeArguments != typeArguments
//                || node->attributes != attributes
//                ? update(createJsxSelfClosingElement(tagName, typeArguments, attributes), node)
//                : node;
//        }
//
// @api
    shared<JsxOpeningElement> createJsxOpeningElement(shared<NodeUnion(JsxTagNameExpression)> tagName, optional<NodeArray> typeArguments, shared<JsxAttributes> attributes) {
        auto node = createBaseNode<JsxOpeningElement>(SyntaxKind::JsxOpeningElement);
        node->tagName = tagName;
        node->typeArguments = asNodeArray(typeArguments);
        node->attributes = attributes;
        node->transformFlags |=
                propagateChildFlags(node->tagName) |
                propagateChildrenFlags(node->typeArguments) |
                propagateChildFlags(node->attributes) |
                (int) TransformFlags::ContainsJsx;
        if (typeArguments) {
            node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
        }
        return node;
    }
//
//        // @api
//        function updateJsxOpeningElement(node: JsxOpeningElement, tagName: JsxTagNameExpression, optional<NodeArray> typeArguments, attributes: JsxAttributes) {
//            return node->tagName != tagName
//                || node->typeArguments != typeArguments
//                || node->attributes != attributes
//                ? update(createJsxOpeningElement(tagName, typeArguments, attributes), node)
//                : node;
//        }
//
// @api
    shared<JsxClosingElement> createJsxClosingElement(shared<NodeUnion(JsxTagNameExpression)> tagName) {
        auto node = createBaseNode<JsxClosingElement>(SyntaxKind::JsxClosingElement);
        node->tagName = tagName;
        node->transformFlags |=
                propagateChildFlags(node->tagName) |
                (int) TransformFlags::ContainsJsx;
        return node;
    }

//        // @api
//        function updateJsxClosingElement(node: JsxClosingElement, tagName: JsxTagNameExpression) {
//            return node->tagName != tagName
//                ? update(createJsxClosingElement(tagName), node)
//                : node;
//        }

// @api
    shared<JsxFragment> createJsxFragment(shared<JsxOpeningFragment> openingFragment, NodeArray children, shared<JsxClosingFragment> closingFragment) {
        auto node = createBaseNode<JsxFragment>(SyntaxKind::JsxFragment);
        node->openingFragment = openingFragment;
        node->children = createNodeArray(children);
        node->closingFragment = closingFragment;
        node->transformFlags |=
                propagateChildFlags(node->openingFragment) |
                propagateChildrenFlags(node->children) |
                propagateChildFlags(node->closingFragment) |
                (int) TransformFlags::ContainsJsx;
        return node;
    }

//        // @api
//        function updateJsxFragment(node: JsxFragment, openingFragment: JsxOpeningFragment, children: readonly JsxChild[], closingFragment: JsxClosingFragment) {
//            return node->openingFragment != openingFragment
//                || node->children != children
//                || node->closingFragment != closingFragment
//                ? update(createJsxFragment(openingFragment, children, closingFragment), node)
//                : node;
//        }
//
//
//        // @api
//        function updateJsxText(node: JsxText, text: string, containsOnlyTriviaWhiteSpaces?: boolean) {
//            return node->text != text
//                || node->containsOnlyTriviaWhiteSpaces != containsOnlyTriviaWhiteSpaces
//                ? update(createJsxText(text, containsOnlyTriviaWhiteSpaces), node)
//                : node;
//        }
//
// @api
    shared<JsxOpeningFragment> createJsxOpeningFragment() {
        auto node = createBaseNode<JsxOpeningFragment>(SyntaxKind::JsxOpeningFragment);
        node->transformFlags |= (int) TransformFlags::ContainsJsx;
        return node;
    }

// @api
    shared<JsxClosingFragment> createJsxJsxClosingFragment() {
        auto node = createBaseNode<JsxClosingFragment>(SyntaxKind::JsxClosingFragment);
        node->transformFlags |= (int) TransformFlags::ContainsJsx;
        return node;
    }

// @api
    shared<JsxAttribute> createJsxAttribute(shared<Identifier> name, sharedOpt<NodeUnion(JsxAttributeValue)> initializer = {}) {
        auto node = createBaseNode<JsxAttribute>(SyntaxKind::JsxAttribute);
        node->name = name;
        node->initializer = initializer;
        node->transformFlags |=
                propagateChildFlags(node->name) |
                propagateChildFlags(node->initializer) |
                (int) TransformFlags::ContainsJsx;
        return node;
    }

//        // @api
//        function updateJsxAttribute(node: JsxAttribute, name: Identifier, initializer: JsxAttributeValue | undefined) {
//            return node->name != name
//                || node->initializer != initializer
//                ? update(createJsxAttribute(name, initializer), node)
//                : node;
//        }
//
// @api
    shared<JsxAttributes> createJsxAttributes(NodeArray properties) {
        auto node = createBaseNode<JsxAttributes>(SyntaxKind::JsxAttributes);
        node->properties = createNodeArray(properties);
        node->transformFlags |=
                propagateChildrenFlags(node->properties) |
                (int) TransformFlags::ContainsJsx;
        return node;
    }
//
//        // @api
//        function updateJsxAttributes(node: JsxAttributes, properties: readonly JsxAttributeLike[]) {
//            return node->properties != properties
//                ? update(createJsxAttributes(properties), node)
//                : node;
//        }

// @api
    shared<JsxSpreadAttribute> createJsxSpreadAttribute(shared<Expression> expression) {
        auto node = createBaseNode<JsxSpreadAttribute>(SyntaxKind::JsxSpreadAttribute);
        node->expression = expression;
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                (int) TransformFlags::ContainsJsx;
        return node;
    }

//        // @api
//        function updateJsxSpreadAttribute(node: JsxSpreadAttribute, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createJsxSpreadAttribute(expression), node)
//                : node;
//        }
//
// @api
    shared<JsxExpression> createJsxExpression(sharedOpt<DotDotDotToken> dotDotDotToken, sharedOpt<Expression> expression) {
        auto node = createBaseNode<JsxExpression>(SyntaxKind::JsxExpression);
        node->dotDotDotToken = dotDotDotToken;
        node->expression = expression;
        node->transformFlags |=
                propagateChildFlags(node->dotDotDotToken) |
                propagateChildFlags(node->expression) |
                (int) TransformFlags::ContainsJsx;
        return node;
    }

//        // @api
//        function updateJsxExpression(node: JsxExpression, sharedOpt<Expression> expression) {
//            return node->expression != expression
//                ? update(createJsxExpression(node->dotDotDotToken, expression), node)
//                : node;
//        }
//
//        //
//        // Clauses
//        //
//
//        // @api
//        function createCaseClause(shared<Expression> expression, statements: readonly Statement[]) {
//            auto node = createBaseNode<CaseClause>(SyntaxKind::CaseClause);
//            node->expression = parenthesizerRules::parenthesizeExpressionForDisallowedComma(expression);
//            node->statements = createNodeArray(statements);
//            node->transformFlags |=
//                propagateChildFlags(node->expression) |
//                propagateChildrenFlags(node->statements);
//            return node;
//        }
//
//        // @api
//        function updateCaseClause(node: CaseClause, shared<Expression> expression, statements: readonly Statement[]) {
//            return node->expression != expression
//                || node->statements != statements
//                ? update(createCaseClause(expression, statements), node)
//                : node;
//        }
//
//        // @api
//        function createDefaultClause(statements: readonly Statement[]) {
//            auto node = createBaseNode<DefaultClause>(SyntaxKind::DefaultClause);
//            node->statements = createNodeArray(statements);
//            node->transformFlags = propagateChildrenFlags(node->statements);
//            return node;
//        }
//
//        // @api
//        function updateDefaultClause(node: DefaultClause, statements: readonly Statement[]) {
//            return node->statements != statements
//                ? update(createDefaultClause(statements), node)
//                : node;
//        }

    // @api
    auto createHeritageClause(SyntaxKind token, NodeArray types) {
        auto node = createBaseNode<HeritageClause>(SyntaxKind::HeritageClause);
        node->token = token;
        node->types = createNodeArray(types);
        node->transformFlags |= propagateChildrenFlags(node->types);
        switch (token) {
            case SyntaxKind::ExtendsKeyword:
                node->transformFlags |= (int) TransformFlags::ContainsES2015;
                break;
            case SyntaxKind::ImplementsKeyword:
                node->transformFlags |= (int) TransformFlags::ContainsTypeScript;
                break;
            default:
                throw runtime_error("invalid token");
        }
        return node;
    }

//        // @api
//        function updateHeritageClause(node: HeritageClause, types: readonly ExpressionWithTypeArguments[]) {
//            return node->types != types
//                ? update(createHeritageClause(node->token, types), node)
//                : node;
//        }
//
//        // @api
//        function createCatchClause(variableDeclaration: string | BindingName | VariableDeclaration | undefined, block: Block) {
//            auto node = createBaseNode<CatchClause>(SyntaxKind::CatchClause);
//            if (typeof variableDeclaration == "string" || variableDeclaration && !isVariableDeclaration(variableDeclaration)) {
//                variableDeclaration = createVariableDeclaration(
//                    variableDeclaration,
//                    /*exclamationToken*/ {},
//                    /*type*/ {},
//                    /*initializer*/ undefined
//                );
//            }
//            node->variableDeclaration = variableDeclaration;
//            node->block = block;
//            node->transformFlags |=
//                propagateChildFlags(node->variableDeclaration) |
//                propagateChildFlags(node->block);
//            if (!variableDeclaration) node->transformFlags |= (int)TransformFlags::ContainsES2019;
//            return node;
//        }
//
//        // @api
//        function updateCatchClause(node: CatchClause, variableDeclaration: VariableDeclaration | undefined, block: Block) {
//            return node->variableDeclaration != variableDeclaration
//                || node->block != block
//                ? update(createCatchClause(variableDeclaration, block), node)
//                : node;
//        }
//
//        //
//        // Property assignments
//        //
//
    // @api
    auto createPropertyAssignment(NameType name, shared<Expression> initializer) {
        auto node = createBaseNamedDeclaration<PropertyAssignment>(
                SyntaxKind::PropertyAssignment,
                /*decorators*/ {},
                /*modifiers*/ {},
                name
        );
        node->initializer = parenthesizerRules::parenthesizeExpressionForDisallowedComma(initializer);
        node->transformFlags |=
                propagateChildFlags(node->name) |
                propagateChildFlags(node->initializer);
        return node;
    }

//        function finishUpdatePropertyAssignment(updated: Mutable<PropertyAssignment>, original: PropertyAssignment) {
//            // copy children used only for error reporting
//            if (original.decorators) updated.decorators = original.decorators;
//            if (original.modifiers) updated.modifiers = original.modifiers;
//            if (original.questionToken) updated.questionToken = original.questionToken;
//            if (original.exclamationToken) updated.exclamationToken = original.exclamationToken;
//            return update(updated, original);
//        }
//
//        // @api
//        function updatePropertyAssignment(node: PropertyAssignment, name: PropertyName, initializer: Expression) {
//            return node->name != name
//                || node->initializer != initializer
//                ? finishUpdatePropertyAssignment(createPropertyAssignment(name, initializer), node)
//                : node;
//        }
//
    // @api
    auto createShorthandPropertyAssignment(NameType name, sharedOpt<Expression> objectAssignmentInitializer) {
        auto node = createBaseNamedDeclaration<ShorthandPropertyAssignment>(
                SyntaxKind::ShorthandPropertyAssignment,
                /*decorators*/ {},
                /*modifiers*/ {},
                name
        );
        node->objectAssignmentInitializer = objectAssignmentInitializer ? parenthesizerRules::parenthesizeExpressionForDisallowedComma(objectAssignmentInitializer) : nullptr;
        node->transformFlags |=
                propagateChildFlags(node->objectAssignmentInitializer) |
                (int) TransformFlags::ContainsES2015;
        return node;
    }
//
//        function finishUpdateShorthandPropertyAssignment(updated: Mutable<ShorthandPropertyAssignment>, original: ShorthandPropertyAssignment) {
//            // copy children used only for error reporting
//            if (original.decorators) updated.decorators = original.decorators;
//            if (original.modifiers) updated.modifiers = original.modifiers;
//            if (original.equalsToken) updated.equalsToken = original.equalsToken;
//            if (original.questionToken) updated.questionToken = original.questionToken;
//            if (original.exclamationToken) updated.exclamationToken = original.exclamationToken;
//            return update(updated, original);
//        }
//
//        // @api
//        function updateShorthandPropertyAssignment(node: ShorthandPropertyAssignment, name: Identifier, objectAssignmentInitializer: Expression | undefined) {
//            return node->name != name
//                || node->objectAssignmentInitializer != objectAssignmentInitializer
//                ? finishUpdateShorthandPropertyAssignment(createShorthandPropertyAssignment(name, objectAssignmentInitializer), node)
//                : node;
//        }
//
    // @api
    auto createSpreadAssignment(shared<Expression> expression) {
        auto node = createBaseNode<SpreadAssignment>(SyntaxKind::SpreadAssignment);
        node->expression = parenthesizerRules::parenthesizeExpressionForDisallowedComma(expression);
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                (int) TransformFlags::ContainsES2018 |
                (int) TransformFlags::ContainsObjectRestOrSpread;
        return node;
    }

//        // @api
//        function updateSpreadAssignment(node: SpreadAssignment, shared<Expression> expression) {
//            return node->expression != expression
//                ? update(createSpreadAssignment(expression), node)
//                : node;
//        }
//
//        //
//        // Enum
//        //
//
//        // @api
//        function createEnumMember(NameType name, initializer?: Expression) {
//            auto node = createBaseNode<EnumMember>(SyntaxKind::EnumMember);
//            node->name = asName(name);
//            node->initializer = initializer && parenthesizerRules::parenthesizeExpressionForDisallowedComma(initializer);
//            node->transformFlags |=
//                propagateChildFlags(node->name) |
//                propagateChildFlags(node->initializer) |
//                (int)TransformFlags::ContainsTypeScript;
//            return node;
//        }
//
//        // @api
//        function updateEnumMember(node: EnumMember, name: PropertyName, sharedOpt<Expression> initializer) {
//            return node->name != name
//                || node->initializer != initializer
//                ? update(createEnumMember(name, initializer), node)
//                : node;
//        }
//
//        //
//        // Top-level nodes
//        //
//
//        // @api
//        function createSourceFile(
//            statements: readonly Statement[],
//            endOfFileToken: EndOfFileToken,
//            flags: NodeFlags
//        ) {
//            auto node = basefactory::createBaseSourceFileNode(SyntaxKind::SourceFile) as Mutable<SourceFile>;
//            node->statements = createNodeArray(statements);
//            node->endOfFileToken = endOfFileToken;
//            node->flags |= flags;
//            node->fileName = "";
//            node->text = "";
//            node->languageVersion = 0;
//            node->languageVariant = 0;
//            node->scriptKind = 0;
//            node->isDeclarationFile = false;
//            node->hasNoDefaultLib = false;
//            node->transformFlags |=
//                propagateChildrenFlags(node->statements) |
//                propagateChildFlags(node->endOfFileToken);
//            return node;
//        }
//
//        function cloneSourceFileWithChanges(
//            source: SourceFile,
//            statements: readonly Statement[],
//            isDeclarationFile: boolean,
//            referencedFiles: readonly FileReference[],
//            typeReferences: readonly FileReference[],
//            hasNoDefaultLib: boolean,
//            libReferences: readonly FileReference[]
//        ) {
//            auto node = (source.redirectInfo ? Object.create(source.redirectInfo.redirectTarget) : basefactory::createBaseSourceFileNode(SyntaxKind::SourceFile)) as Mutable<SourceFile>;
//            for (auto p in source) {
//                if (p == "emitNode" || hasProperty(node, p) || !hasProperty(source, p)) continue;
//                (node as any)[p] = (source as any)[p];
//            }
//            node->flags |= source.flags;
//            node->statements = createNodeArray(statements);
//            node->endOfFileToken = source.endOfFileToken;
//            node->isDeclarationFile = isDeclarationFile;
//            node->referencedFiles = referencedFiles;
//            node->typeReferenceDirectives = typeReferences;
//            node->hasNoDefaultLib = hasNoDefaultLib;
//            node->libReferenceDirectives = libReferences;
//            node->transformFlags =
//                propagateChildrenFlags(node->statements) |
//                propagateChildFlags(node->endOfFileToken);
//            node->impliedNodeFormat = source.impliedNodeFormat;
//            return node;
//        }
//
//        // @api
//        function updateSourceFile(
//            node: SourceFile,
//            statements: readonly Statement[],
//            isDeclarationFile = node->isDeclarationFile,
//            referencedFiles = node->referencedFiles,
//            typeReferenceDirectives = node->typeReferenceDirectives,
//            hasNoDefaultLib = node->hasNoDefaultLib,
//            libReferenceDirectives = node->libReferenceDirectives
//        ) {
//            return node->statements != statements
//                || node->isDeclarationFile != isDeclarationFile
//                || node->referencedFiles != referencedFiles
//                || node->typeReferenceDirectives != typeReferenceDirectives
//                || node->hasNoDefaultLib != hasNoDefaultLib
//                || node->libReferenceDirectives != libReferenceDirectives
//                ? update(cloneSourceFileWithChanges(node, statements, isDeclarationFile, referencedFiles, typeReferenceDirectives, hasNoDefaultLib, libReferenceDirectives), node)
//                : node;
//        }
//
//        // @api
//        function createBundle(sourceFiles: readonly SourceFile[], prepends: readonly (UnparsedSource | InputFiles)[] = emptyArray) {
//            auto node = createBaseNode<Bundle>(SyntaxKind::Bundle);
//            node->prepends = prepends;
//            node->sourceFiles = sourceFiles;
//            return node;
//        }
//
//        // @api
//        function updateBundle(node: Bundle, sourceFiles: readonly SourceFile[], prepends: readonly (UnparsedSource | InputFiles)[] = emptyArray) {
//            return node->sourceFiles != sourceFiles
//                || node->prepends != prepends
//                ? update(createBundle(sourceFiles, prepends), node)
//                : node;
//        }
//
//        // @api
//        function createUnparsedSource(prologues: readonly UnparsedPrologue[], syntheticReferences: readonly UnparsedSyntheticReference[] | undefined, texts: readonly UnparsedSourceText[]) {
//            auto node = createBaseNode<UnparsedSource>(SyntaxKind::UnparsedSource);
//            node->prologues = prologues;
//            node->syntheticReferences = syntheticReferences;
//            node->texts = texts;
//            node->fileName = "";
//            node->text = "";
//            node->referencedFiles = emptyArray;
//            node->libReferenceDirectives = emptyArray;
//            node->getLineAndCharacterOfPosition = pos => getLineAndCharacterOfPosition(node, pos);
//            return node;
//        }
//
//        function createBaseUnparsedNode<T extends UnparsedNode>(SyntaxKind kind, data?: string) {
//            auto node = createBaseNode(kind);
//            node->data = data;
//            return node;
//        }
//
//        // @api
//        function createUnparsedPrologue(data?: string): UnparsedPrologue {
//            return createBaseUnparsedNode(SyntaxKind::UnparsedPrologue, data);
//        }
//
//        // @api
//        function createUnparsedPrepend(data: string | undefined, texts: readonly UnparsedTextLike[]): UnparsedPrepend {
//            auto node = createBaseUnparsedNode<UnparsedPrepend>(SyntaxKind::UnparsedPrepend, data);
//            node->texts = texts;
//            return node;
//        }
//
//        // @api
//        function createUnparsedTextLike(data: string | undefined, internal: boolean): UnparsedTextLike {
//            return createBaseUnparsedNode(internal ? SyntaxKind::UnparsedInternalText : SyntaxKind::UnparsedText, data);
//        }
//
//        // @api
//        function createUnparsedSyntheticReference(section: BundleFileHasNoDefaultLib | BundleFileReference): UnparsedSyntheticReference {
//            auto node = createBaseNode<UnparsedSyntheticReference>(SyntaxKind::UnparsedSyntheticReference);
//            node->data = section.data;
//            node->section = section;
//            return node;
//        }
//
//        // @api
//        function createInputFiles(): InputFiles {
//            auto node = createBaseNode<InputFiles>(SyntaxKind::InputFiles);
//            node->javascriptText = "";
//            node->declarationText = "";
//            return node;
//        }
//
//        //
//        // Synthetic Nodes (used by checker)
//        //
//
//        // @api
//        function createSyntheticExpression(type: Type, isSpread = false, tupleNameSource?: ParameterDeclaration | NamedTupleMember) {
//            auto node = createBaseNode<SyntheticExpression>(SyntaxKind::SyntheticExpression);
//            node->type = type;
//            node->isSpread = isSpread;
//            node->tupleNameSource = tupleNameSource;
//            return node;
//        }
//
//        // @api
//        function createSyntaxList(children: Node[]) {
//            auto node = createBaseNode<SyntaxList>(SyntaxKind::SyntaxList);
//            node->_children = children;
//            return node;
//        }
//
//        //
//        // Transformation nodes
//        //
//
//        /**
//         * Creates a synthetic statement to act as a placeholder for a not-emitted statement in
//         * order to preserve comments.
//         *
//         * @param original The original statement.
//         */
//        // @api
//        function createNotEmittedStatement(original: Node) {
//            auto node = createBaseNode<NotEmittedStatement>(SyntaxKind::NotEmittedStatement);
//            node->original = original;
//            setTextRange(node, original);
//            return node;
//        }

    /**
     * Creates a synthetic expression to act as a placeholder for a not-emitted expression in
     * order to preserve comments or sourcemap positions.
     *
     * @param expression The inner expression to emit.
     * @param original The original outer expression.
     */
    // @api
    auto createPartiallyEmittedExpression(shared<Expression> expression, sharedOpt<Node> original) {
        auto node = createBaseNode<PartiallyEmittedExpression>(SyntaxKind::PartiallyEmittedExpression);
        node->expression = expression;
        node->original = original;
        node->transformFlags |=
                propagateChildFlags(node->expression) |
                (int) TransformFlags::ContainsTypeScript;
        setTextRange(node, original);
        return node;
    }

    // @api
    auto updatePartiallyEmittedExpression(shared<PartiallyEmittedExpression> node, shared<Expression> expression) {
        return node->expression != expression
               ? update(createPartiallyEmittedExpression(expression, node->original), node)
               : node;
    }

//        function flattenCommaElements(node: Expression): Expression | readonly Expression[] {
//            if (nodeIsSynthesized(node) && !isParseTreeNode(node) && !node->original && !node->emitNode && !node->id) {
//                if (isCommaListExpression(node)) {
//                    return node->elements;
//                }
//                if (isBinaryExpression(node) && isCommaToken(node->operatorToken)) {
//                    return [node->left, node->right];
//                }
//            }
//            return node;
//        }
//
//        // @api
//        function createCommaListExpression(elements: readonly Expression[]) {
//            auto node = createBaseNode<CommaListExpression>(SyntaxKind::CommaListExpression);
//            node->elements = createNodeArray(sameFlatMap(elements, flattenCommaElements));
//            node->transformFlags |= propagateChildrenFlags(node->elements);
//            return node;
//        }
//
//        // @api
//        function updateCommaListExpression(node: CommaListExpression, elements: readonly Expression[]) {
//            return node->elements != elements
//                ? update(createCommaListExpression(elements), node)
//                : node;
//        }
//
//        /**
//         * Creates a synthetic element to act as a placeholder for the end of an emitted declaration in
//         * order to properly emit exports.
//         */
//        // @api
//        function createEndOfDeclarationMarker(original: Node) {
//            auto node = createBaseNode<EndOfDeclarationMarker>(SyntaxKind::EndOfDeclarationMarker);
//            node->emitNode = {} as EmitNode;
//            node->original = original;
//            return node;
//        }
//
//        /**
//         * Creates a synthetic element to act as a placeholder for the beginning of a merged declaration in
//         * order to properly emit exports.
//         */
//        // @api
//        function createMergeDeclarationMarker(original: Node) {
//            auto node = createBaseNode<MergeDeclarationMarker>(SyntaxKind::MergeDeclarationMarker);
//            node->emitNode = {} as EmitNode;
//            node->original = original;
//            return node;
//        }
//
//        // @api
//        function createSyntheticReferenceExpression(shared<Expression> expression, thisArg: Expression) {
//            auto node = createBaseNode<SyntheticReferenceExpression>(SyntaxKind::SyntheticReferenceExpression);
//            node->expression = expression;
//            node->thisArg = thisArg;
//            node->transformFlags |=
//                propagateChildFlags(node->expression) |
//                propagateChildFlags(node->thisArg);
//            return node;
//        }
//
//        // @api
//        function updateSyntheticReferenceExpression(node: SyntheticReferenceExpression, shared<Expression> expression, thisArg: Expression) {
//            return node->expression != expression
//                || node->thisArg != thisArg
//                ? update(createSyntheticReferenceExpression(expression, thisArg), node)
//                : node;
//        }
//
//        // @api
//        function cloneNode<T extends Node | undefined>(node: T): T;
//        function cloneNode<T extends Node>(node: T) {
//            // We don't use "clone" from core.ts here, as we need to preserve the prototype chain of
//            // the original node-> We also need to exclude specific properties and only include own-
//            // properties (to skip members already defined on the shared prototype).
//            if (node == undefined) {
//                return node;
//            }
//
//            auto clone =
//                isSourceFile(node) ? basefactory::createBaseSourceFileNode(SyntaxKind::SourceFile) as T :
//                isIdentifier(node) ? basefactory::createBaseIdentifierNode(SyntaxKind::Identifier) as T :
//                isPrivateIdentifier(node) ? basefactory::createBasePrivateIdentifierNode(SyntaxKind::PrivateIdentifier) as T :
//                !isNodeKind(node->kind) ? basefactory::createBaseTokenNode(node->kind) as T :
//                basefactory::createBaseNode(node->kind) as T;
//
//            (clone as Mutable<T>).flags |= (node->flags & ~NodeFlags::Synthesized);
//            (clone as Mutable<T>).transformFlags = node->transformFlags;
//            setOriginalNode(clone, node);
//
//            for (auto key in node) {
//                if (clone.hasOwnProperty(key) || !node->hasOwnProperty(key)) {
//                    continue;
//                }
//
//                clone[key] = node[key];
//            }
//
//            return clone;
//        }
//
//        // compound nodes
//        function createImmediatelyInvokedFunctionExpression(statements: readonly Statement[]): CallExpression;
//        function createImmediatelyInvokedFunctionExpression(statements: readonly Statement[], param: ParameterDeclaration, paramValue: Expression): CallExpression;
//        function createImmediatelyInvokedFunctionExpression(statements: readonly Statement[], param?: ParameterDeclaration, paramValue?: Expression) {
//            return createCallExpression(
//                createFunctionExpression(
//                    /*modifiers*/ {},
//                    /*asteriskToken*/ {},
//                    /*name*/ {},
//                    /*typeParameters*/ {},
//                    /*parameters*/ param ? [param] : [],
//                    /*type*/ {},
//                    createBlock(statements, /*multiLine*/ true)
//                ),
//                /*typeArguments*/ {},
//                /*argumentsArray*/ paramValue ? [paramValue] : []
//            );
//        }
//
//        function createImmediatelyInvokedArrowFunction(statements: readonly Statement[]): CallExpression;
//        function createImmediatelyInvokedArrowFunction(statements: readonly Statement[], param: ParameterDeclaration, paramValue: Expression): CallExpression;
//        function createImmediatelyInvokedArrowFunction(statements: readonly Statement[], param?: ParameterDeclaration, paramValue?: Expression) {
//            return createCallExpression(
//                createArrowFunction(
//                    /*modifiers*/ {},
//                    /*typeParameters*/ {},
//                    /*parameters*/ param ? [param] : [],
//                    /*type*/ {},
//                    /*equalsGreaterThanToken*/ {},
//                    createBlock(statements, /*multiLine*/ true)
//                ),
//                /*typeArguments*/ {},
//                /*argumentsArray*/ paramValue ? [paramValue] : []
//            );
//        }
//
//        function createVoidZero() {
//            return createVoidExpression(createNumericLiteral("0"));
//        }
//
//        function createExportDefault(shared<Expression> expression) {
//            return createExportAssignment(
//                /*decorators*/ {},
//                /*modifiers*/ {},
//                /*isExportEquals*/ false,
//                expression);
//        }
//
//        function createExternalModuleExport(exportName: Identifier) {
//            return createExportDeclaration(
//                /*decorators*/ {},
//                /*modifiers*/ {},
//                /*isTypeOnly*/ false,
//                createNamedExports([
//                    createExportSpecifier(/*isTypeOnly*/ false, /*propertyName*/ {}, exportName)
//                ])
//            );
//        }
//
//        //
//        // Utilities
//        //
//
//        function createTypeCheck(value: Expression, tag: TypeOfTag) {
//            return tag == "undefined"
//                ? factory::createStrictEquality(value, createVoidZero())
//                : factory::createStrictEquality(createTypeOfExpression(value), createStringLiteral(tag));
//        }
//
//        function createMethodCall(object: Expression, methodName: string | Identifier, argumentsList: readonly Expression[]) {
//            // Preserve the optionality of `object`.
//            if (isCallChain(object)) {
//                return createCallChain(
//                    createPropertyAccessChain(object, /*questionDotToken*/ {}, methodName),
//                    /*questionDotToken*/ {},
//                    /*typeArguments*/ {},
//                    argumentsList
//                );
//            }
//            return createCallExpression(
//                createPropertyAccessExpression(object, methodName),
//                /*typeArguments*/ {},
//                argumentsList
//            );
//        }
//
//        function createFunctionBindCall(target: Expression, thisArg: Expression, argumentsList: readonly Expression[]) {
//            return createMethodCall(target, "bind", [thisArg, ...argumentsList]);
//        }
//
//        function createFunctionCallCall(target: Expression, thisArg: Expression, argumentsList: readonly Expression[]) {
//            return createMethodCall(target, "call", [thisArg, ...argumentsList]);
//        }
//
//        function createFunctionApplyCall(target: Expression, thisArg: Expression, argumentsExpression: Expression) {
//            return createMethodCall(target, "apply", [thisArg, argumentsExpression]);
//        }
//
//        function createGlobalMethodCall(globalObjectName: string, methodName: string, argumentsList: readonly Expression[]) {
//            return createMethodCall(createIdentifier(globalObjectName), methodName, argumentsList);
//        }
//
//        function createArraySliceCall(array: Expression, start?: number | Expression) {
//            return createMethodCall(array, "slice", start == undefined ? [] : [asExpression(start)]);
//        }
//
//        function createArrayConcatCall(array: Expression, argumentsList: readonly Expression[]) {
//            return createMethodCall(array, "concat", argumentsList);
//        }
//
//        function createObjectDefinePropertyCall(target: Expression, propertyName: string | Expression, attributes: Expression) {
//            return createGlobalMethodCall("Object", "defineProperty", [target, asExpression(propertyName), attributes]);
//        }
//
//        function createReflectGetCall(target: Expression, propertyKey: Expression, receiver?: Expression): CallExpression {
//            return createGlobalMethodCall("Reflect", "get", receiver ? [target, propertyKey, receiver] : [target, propertyKey]);
//        }
//
//        function createReflectSetCall(target: Expression, propertyKey: Expression, value: Expression, receiver?: Expression): CallExpression {
//            return createGlobalMethodCall("Reflect", "set", receiver ? [target, propertyKey, value, receiver] : [target, propertyKey, value]);
//        }
//
//        function tryAddPropertyAssignment(properties: Push<PropertyAssignment>, propertyName: string, sharedOpt<Expression> expression) {
//            if (expression) {
//                properties.push(createPropertyAssignment(propertyName, expression));
//                return true;
//            }
//            return false;
//        }
//
//        function createPropertyDescriptor(attributes: PropertyDescriptorAttributes, singleLine?: boolean) {
//            auto properties: PropertyAssignment[] = [];
//            tryAddPropertyAssignment(properties, "enumerable", asExpression(attributes.enumerable));
//            tryAddPropertyAssignment(properties, "configurable", asExpression(attributes.configurable));
//
//            let isData = tryAddPropertyAssignment(properties, "writable", asExpression(attributes.writable));
//            isData = tryAddPropertyAssignment(properties, "value", attributes.value) || isData;
//
//            let isAccessor = tryAddPropertyAssignment(properties, "get", attributes.get);
//            isAccessor = tryAddPropertyAssignment(properties, "set", attributes.set) || isAccessor;
//
//            Debug::asserts(!(isData && isAccessor), "A PropertyDescriptor may not be both an accessor descriptor and a data descriptor.");
//            return createObjectLiteralExpression(properties, !singleLine);
//        }

    shared<Expression> updateOuterExpression(shared<OuterExpression> outerExpression, shared<Expression> expression) {
        switch (outerExpression->kind) {
            case SyntaxKind::ParenthesizedExpression:
                return updateParenthesizedExpression(to<ParenthesizedExpression>(outerExpression), expression);
            case SyntaxKind::TypeAssertionExpression:
                return updateTypeAssertion(to<TypeAssertion>(outerExpression), to<TypeAssertion>(outerExpression)->type, expression);
            case SyntaxKind::AsExpression:
                return updateAsExpression(to<AsExpression>(outerExpression), expression, to<AsExpression>(outerExpression)->type);
            case SyntaxKind::NonNullExpression:
                return updateNonNullExpression(to<NonNullExpression>(outerExpression), expression);
            case SyntaxKind::PartiallyEmittedExpression:
                return updatePartiallyEmittedExpression(to<PartiallyEmittedExpression>(outerExpression), expression);
        }
        throw runtime_error("Invalid OuterExpression passed");
    }

    /**
     * Determines whether a node is a parenthesized expression that can be ignored when recreating outer expressions.
     *
     * A parenthesized expression can be ignored when all of the following are true:
     *
     * - It's `pos` and `end` are not -1
     * - It does not have a custom source map range
     * - It does not have a custom comment range
     * - It does not have synthetic leading or trailing comments
     *
     * If an outermost parenthesized expression is ignored, but the containing expression requires a parentheses around
     * the expression to maintain precedence, a new parenthesized expression should be created automatically when
     * the containing expression is created/updated.
     */
    bool isIgnorableParen(shared<Expression> node) {
        return isParenthesizedExpression(node)
               && nodeIsSynthesized(node)
               && nodeIsSynthesized(getSourceMapRange(node))
               && nodeIsSynthesized(getCommentRange(node))
               && !some(getSyntheticLeadingComments(node))
               && !some(getSyntheticTrailingComments(node));
    }

    shared<Expression> restoreOuterExpressions(sharedOpt<Expression> outerExpression, shared<Expression> innerExpression, int kinds) {
        if (isOuterExpression(outerExpression, kinds) && !isIgnorableParen(outerExpression)) {
            return updateOuterExpression(
                    outerExpression,
                    restoreOuterExpressions(getExpression(outerExpression), innerExpression)
            );
        }
        return innerExpression;
    }

//        function restoreEnclosingLabel(node: Statement, outermostLabeledStatement: LabeledStatement | undefined, afterRestoreLabelCallback?: (node: LabeledStatement) => void): Statement {
//            if (!outermostLabeledStatement) {
//                return node;
//            }
//            auto updated = updateLabeledStatement(
//                outermostLabeledStatement,
//                outermostLabeledStatement.label,
//                isLabeledStatement(outermostLabeledStatement.statement)
//                    ? restoreEnclosingLabel(node, outermostLabeledStatement.statement)
//                    : node
//            );
//            if (afterRestoreLabelCallback) {
//                afterRestoreLabelCallback(outermostLabeledStatement);
//            }
//            return updated;
//        }
//
//        function shouldBeCapturedInTempVariable(node: Expression, cacheIdentifiers: boolean): boolean {
//            auto target = skipParentheses(node);
//            switch (target.kind) {
//                case SyntaxKind::Identifier:
//                    return cacheIdentifiers;
//                case SyntaxKind::ThisKeyword:
//                case SyntaxKind::NumericLiteral:
//                case SyntaxKind::BigIntLiteral:
//                case SyntaxKind::StringLiteral:
//                    return false;
//                case SyntaxKind::ArrayLiteralExpression:
//                    auto elements = (target as ArrayLiteralExpression).elements;
//                    if (elements.length == 0) {
//                        return false;
//                    }
//                    return true;
//                case SyntaxKind::ObjectLiteralExpression:
//                    return (target as ObjectLiteralExpression).properties.length > 0;
//                default:
//                    return true;
//            }
//        }
//
//        function createCallBinding(shared<Expression> expression, recordTempVariable: (temp: Identifier) => void, languageVersion?: ScriptTarget, cacheIdentifiers = false): CallBinding {
//            auto callee = skipOuterExpressions(expression, OuterExpressionKinds::All);
//            let thisArg: Expression;
//            let target: LeftHandSideExpression;
//            if (isSuperProperty(callee)) {
//                thisArg = createThis();
//                target = callee;
//            }
//            else if (isSuperKeyword(callee)) {
//                thisArg = createThis();
//                target = languageVersion != undefined && languageVersion < ScriptTarget.ES2015
//                    ? setTextRange(createIdentifier("_super"), callee)
//                    : callee as PrimaryExpression;
//            }
//            else if (getEmitFlags(callee) & EmitFlags.HelperName) {
//                thisArg = createVoidZero();
//                target = parenthesizerRules::parenthesizeLeftSideOfAccess(callee);
//            }
//            else if (isPropertyAccessExpression(callee)) {
//                if (shouldBeCapturedInTempVariable(callee.expression, cacheIdentifiers)) {
//                    // for `a.b()` target is `(_a = a).b` and thisArg is `_a`
//                    thisArg = createTempVariable(recordTempVariable);
//                    target = createPropertyAccessExpression(
//                        setTextRange(
//                            factory::createAssignment(
//                                thisArg,
//                                callee.expression
//                            ),
//                            callee.expression
//                        ),
//                        callee.name
//                    );
//                    setTextRange(target, callee);
//                }
//                else {
//                    thisArg = callee.expression;
//                    target = callee;
//                }
//            }
//            else if (isElementAccessExpression(callee)) {
//                if (shouldBeCapturedInTempVariable(callee.expression, cacheIdentifiers)) {
//                    // for `a[b]()` target is `(_a = a)[b]` and thisArg is `_a`
//                    thisArg = createTempVariable(recordTempVariable);
//                    target = createElementAccessExpression(
//                        setTextRange(
//                            factory::createAssignment(
//                                thisArg,
//                                callee.expression
//                            ),
//                            callee.expression
//                        ),
//                        callee.argumentExpression
//                    );
//                    setTextRange(target, callee);
//                }
//                else {
//                    thisArg = callee.expression;
//                    target = callee;
//                }
//            }
//            else {
//                // for `a()` target is `a` and thisArg is `void 0`
//                thisArg = createVoidZero();
//                target = parenthesizerRules::parenthesizeLeftSideOfAccess(expression);
//            }
//
//            return { target, thisArg };
//        }
//
//        function createAssignmentTargetWrapper(paramName: Identifier, shared<Expression> expression): LeftHandSideExpression {
//            return createPropertyAccessExpression(
//                // Explicit parens required because of v8 regression (https://bugs.chromium.org/p/v8/issues/detail?id=9560)
//                createParenthesizedExpression(
//                    createObjectLiteralExpression([
//                        createSetAccessorDeclaration(
//                            /*decorators*/ {},
//                            /*modifiers*/ {},
//                            "value",
//                            [createParameterDeclaration(
//                                /*decorators*/ {},
//                                /*modifiers*/ {},
//                                /*dotDotDotToken*/ {},
//                                paramName,
//                                /*questionToken*/ {},
//                                /*type*/ {},
//                                /*initializer*/ undefined
//                            )],
//                            createBlock([
//                                createExpressionStatement(expression)
//                            ])
//                        )
//                    ])
//                ),
//                "value"
//            );
//        }
//
//        function inlineExpressions(expressions: readonly Expression[]) {
//            // Avoid deeply nested comma expressions as traversing them during emit can result in "Maximum call
//            // stack size exceeded" errors.
//            return expressions.length > 10
//                ? createCommaListExpression(expressions)
//                : reduceLeft(expressions, factory::createComma)!;
//        }
//
//        function getName(node: Declaration | undefined, allowComments?: boolean, allowSourceMaps?: boolean, emitFlags: EmitFlags = 0) {
//            auto nodeName = getNameOfDeclaration(node);
//            if (nodeName && isIdentifier(nodeName) && !isGeneratedIdentifier(nodeName)) {
//                // TODO(rbuckton): Does this need to be parented?
//                auto name = setParent(setTextRange(cloneNode(nodeName), nodeName), nodeName.parent);
//                emitFlags |= getEmitFlags(nodeName);
//                if (!allowSourceMaps) emitFlags |= EmitFlags.NoSourceMap;
//                if (!allowComments) emitFlags |= EmitFlags.NoComments;
//                if (emitFlags) setEmitFlags(name, emitFlags);
//                return name;
//            }
//            return getGeneratedNameForNode(node);
//        }
//
//        /**
//         * Gets the internal name of a declaration. This is primarily used for declarations that can be
//         * referred to by name in the body of an ES5 class function body. An internal name will *never*
//         * be prefixed with an module or namespace export modifier like "exports." when emitted as an
//         * expression. An internal name will also *never* be renamed due to a collision with a block
//         * scoped variable.
//         *
//         * @param node The declaration.
//         * @param allowComments A value indicating whether comments may be emitted for the name.
//         * @param allowSourceMaps A value indicating whether source maps may be emitted for the name.
//         */
//        function getInternalName(node: Declaration, allowComments?: boolean, allowSourceMaps?: boolean) {
//            return getName(node, allowComments, allowSourceMaps, EmitFlags.LocalName | EmitFlags.InternalName);
//        }
//
//        /**
//         * Gets the local name of a declaration. This is primarily used for declarations that can be
//         * referred to by name in the declaration's immediate scope (classes, enums, namespaces). A
//         * local name will *never* be prefixed with an module or namespace export modifier like
//         * "exports." when emitted as an expression.
//         *
//         * @param node The declaration.
//         * @param allowComments A value indicating whether comments may be emitted for the name.
//         * @param allowSourceMaps A value indicating whether source maps may be emitted for the name.
//         */
//        function getLocalName(node: Declaration, allowComments?: boolean, allowSourceMaps?: boolean) {
//            return getName(node, allowComments, allowSourceMaps, EmitFlags.LocalName);
//        }
//
//        /**
//         * Gets the export name of a declaration. This is primarily used for declarations that can be
//         * referred to by name in the declaration's immediate scope (classes, enums, namespaces). An
//         * export name will *always* be prefixed with an module or namespace export modifier like
//         * `"exports."` when emitted as an expression if the name points to an exported symbol.
//         *
//         * @param node The declaration.
//         * @param allowComments A value indicating whether comments may be emitted for the name.
//         * @param allowSourceMaps A value indicating whether source maps may be emitted for the name.
//         */
//        function getExportName(node: Declaration, allowComments?: boolean, allowSourceMaps?: boolean): Identifier {
//            return getName(node, allowComments, allowSourceMaps, EmitFlags.ExportName);
//        }
//
//        /**
//         * Gets the name of a declaration for use in declarations.
//         *
//         * @param node The declaration.
//         * @param allowComments A value indicating whether comments may be emitted for the name.
//         * @param allowSourceMaps A value indicating whether source maps may be emitted for the name.
//         */
//        function getDeclarationName(node: Declaration | undefined, allowComments?: boolean, allowSourceMaps?: boolean) {
//            return getName(node, allowComments, allowSourceMaps);
//        }
//
//        /**
//         * Gets a namespace-qualified name for use in expressions.
//         *
//         * @param ns The namespace identifier.
//         * @param name The name.
//         * @param allowComments A value indicating whether comments may be emitted for the name.
//         * @param allowSourceMaps A value indicating whether source maps may be emitted for the name.
//         */
//        function getNamespaceMemberName(ns: Identifier, name: Identifier, allowComments?: boolean, allowSourceMaps?: boolean): PropertyAccessExpression {
//            auto qualifiedName = createPropertyAccessExpression(ns, nodeIsSynthesized(name) ? name : cloneNode(name));
//            setTextRange(qualifiedName, name);
//            let emitFlags: EmitFlags = 0;
//            if (!allowSourceMaps) emitFlags |= EmitFlags.NoSourceMap;
//            if (!allowComments) emitFlags |= EmitFlags.NoComments;
//            if (emitFlags) setEmitFlags(qualifiedName, emitFlags);
//            return qualifiedName;
//        }
//
//        /**
//         * Gets the exported name of a declaration for use in expressions.
//         *
//         * An exported name will *always* be prefixed with an module or namespace export modifier like
//         * "exports." if the name points to an exported symbol.
//         *
//         * @param ns The namespace identifier.
//         * @param node The declaration.
//         * @param allowComments A value indicating whether comments may be emitted for the name.
//         * @param allowSourceMaps A value indicating whether source maps may be emitted for the name.
//         */
//        function getExternalModuleOrNamespaceExportName(ns: Identifier | undefined, node: Declaration, allowComments?: boolean, allowSourceMaps?: boolean): Identifier | PropertyAccessExpression {
//            if (ns && hasSyntacticModifier(node, ModifierFlags::Export)) {
//                return getNamespaceMemberName(ns, getName(node), allowComments, allowSourceMaps);
//            }
//            return getExportName(node, allowComments, allowSourceMaps);
//        }
//
//        /**
//         * Copies any necessary standard and custom prologue-directives into target array.
//         * @param source origin statements array
//         * @param target result statements array
//         * @param ensureUseStrict boolean determining whether the function need to add prologue-directives
//         * @param visitor Optional callback used to visit any custom prologue directives.
//         */
//        function copyPrologue(source: readonly Statement[], target: Push<Statement>, ensureUseStrict?: boolean, visitor?: (node: Node) => VisitResult<Node>): number {
//            auto offset = copyStandardPrologue(source, target, 0, ensureUseStrict);
//            return copyCustomPrologue(source, target, offset, visitor);
//        }
//
//        function isUseStrictPrologue(node: ExpressionStatement): boolean {
//            return isStringLiteral(node->expression) && node->expression.text == "use strict";
//        }
//
//        function createUseStrictPrologue() {
//            return startOnNewLine(createExpressionStatement(createStringLiteral("use strict"))) as PrologueDirective;
//        }
//
//        /**
//         * Copies only the standard (string-expression) prologue-directives into the target statement-array.
//         * @param source origin statements array
//         * @param target result statements array
//         * @param statementOffset The offset at which to begin the copy.
//         * @param ensureUseStrict boolean determining whether the function need to add prologue-directives
//         * @returns Count of how many directive statements were copied.
//         */
//        function copyStandardPrologue(source: readonly Statement[], target: Push<Statement>, statementOffset = 0, ensureUseStrict?: boolean): number {
//            Debug::asserts(target.length == 0, "Prologue directives should be at the first statement in the target statements array");
//            let foundUseStrict = false;
//            auto numStatements = source.length;
//            while (statementOffset < numStatements) {
//                auto statement = source[statementOffset];
//                if (isPrologueDirective(statement)) {
//                    if (isUseStrictPrologue(statement)) {
//                        foundUseStrict = true;
//                    }
//                    target.push(statement);
//                }
//                else {
//                    break;
//                }
//                statementOffset++;
//            }
//            if (ensureUseStrict && !foundUseStrict) {
//                target.push(createUseStrictPrologue());
//            }
//            return statementOffset;
//        }
//
//        /**
//         * Copies only the custom prologue-directives into target statement-array.
//         * @param source origin statements array
//         * @param target result statements array
//         * @param statementOffset The offset at which to begin the copy.
//         * @param visitor Optional callback used to visit any custom prologue directives.
//         */
//        function copyCustomPrologue(source: readonly Statement[], target: Push<Statement>, statementOffset: number, visitor?: (node: Node) => VisitResult<Node>, filter?: (node: Node) => boolean): number;
//        function copyCustomPrologue(source: readonly Statement[], target: Push<Statement>, statementOffset: number | undefined, visitor?: (node: Node) => VisitResult<Node>, filter?: (node: Node) => boolean): number | undefined;
//        function copyCustomPrologue(source: readonly Statement[], target: Push<Statement>, statementOffset: number | undefined, visitor?: (node: Node) => VisitResult<Node>, filter: (node: Node) => boolean = returnTrue): number | undefined {
//            auto numStatements = source.length;
//            while (statementOffset != undefined && statementOffset < numStatements) {
//                auto statement = source[statementOffset];
//                if (getEmitFlags(statement) & EmitFlags.CustomPrologue && filter(statement)) {
//                    append(target, visitor ? visitNode(statement, visitor, isStatement) : statement);
//                }
//                else {
//                    break;
//                }
//                statementOffset++;
//            }
//            return statementOffset;
//        }
//
//        /**
//         * Ensures "use strict" directive is added
//         *
//         * @param statements An array of statements
//         */
//        function ensureUseStrict(statements: NodeArray<Statement>): NodeArray<Statement> {
//            auto foundUseStrict = findUseStrictPrologue(statements);
//
//            if (!foundUseStrict) {
//                return setTextRange(createNodeArray<Statement>([createUseStrictPrologue(), ...statements]), statements);
//            }
//
//            return statements;
//        }
//
//        /**
//         * Lifts a NodeArray containing only Statement nodes to a block.
//         *
//         * @param nodes The NodeArray.
//         */
//        function liftToBlock(nodes: readonly Node[]): Statement {
//            Debug::asserts(every(nodes, isStatementOrBlock), "Cannot lift nodes to a Block.");
//            return singleOrUndefined(nodes) as Statement || createBlock(nodes as readonly Statement[]);
//        }
//
//        function findSpanEnd<T>(array: readonly T[], test: (value: T) => boolean, start: number) {
//            let i = start;
//            while (i < array.length && test(array[i])) {
//                i++;
//            }
//            return i;
//        }
//
//        function mergeLexicalEnvironment(statements: NodeArray<Statement>, declarations: readonly Statement[] | undefined): NodeArray<Statement>;
//        function mergeLexicalEnvironment(statements: Statement[], declarations: readonly Statement[] | undefined): Statement[];
//        function mergeLexicalEnvironment(statements: Statement[] | NodeArray<Statement>, declarations: readonly Statement[] | undefined) {
//            if (!some(declarations)) {
//                return statements;
//            }
//
//            // When we merge new lexical statements into an existing statement list, we merge them in the following manner:
//            //
//            // Given:
//            //
//            // | Left                               | Right                               |
//            // |------------------------------------|-------------------------------------|
//            // | [standard prologues (left)]        | [standard prologues (right)]        |
//            // | [hoisted functions (left)]         | [hoisted functions (right)]         |
//            // | [hoisted variables (left)]         | [hoisted variables (right)]         |
//            // | [lexical init statements (left)]   | [lexical init statements (right)]   |
//            // | [other statements (left)]          |                                     |
//            //
//            // The resulting statement list will be:
//            //
//            // | Result                              |
//            // |-------------------------------------|
//            // | [standard prologues (right)]        |
//            // | [standard prologues (left)]         |
//            // | [hoisted functions (right)]         |
//            // | [hoisted functions (left)]          |
//            // | [hoisted variables (right)]         |
//            // | [hoisted variables (left)]          |
//            // | [lexical init statements (right)]   |
//            // | [lexical init statements (left)]    |
//            // | [other statements (left)]           |
//            //
//            // NOTE: It is expected that new lexical init statements must be evaluated before existing lexical init statements,
//            // as the prior transformation may depend on the evaluation of the lexical init statements to be in the correct state.
//
//            // find standard prologues on left in the following order: standard directives, hoisted functions, hoisted variables, other custom
//            auto leftStandardPrologueEnd = findSpanEnd(statements, isPrologueDirective, 0);
//            auto leftHoistedFunctionsEnd = findSpanEnd(statements, isHoistedFunction, leftStandardPrologueEnd);
//            auto leftHoistedVariablesEnd = findSpanEnd(statements, isHoistedVariableStatement, leftHoistedFunctionsEnd);
//
//            // find standard prologues on right in the following order: standard directives, hoisted functions, hoisted variables, other custom
//            auto rightStandardPrologueEnd = findSpanEnd(declarations, isPrologueDirective, 0);
//            auto rightHoistedFunctionsEnd = findSpanEnd(declarations, isHoistedFunction, rightStandardPrologueEnd);
//            auto rightHoistedVariablesEnd = findSpanEnd(declarations, isHoistedVariableStatement, rightHoistedFunctionsEnd);
//            auto rightCustomPrologueEnd = findSpanEnd(declarations, isCustomPrologue, rightHoistedVariablesEnd);
//            Debug::asserts(rightCustomPrologueEnd == declarations.length, "Expected declarations to be valid standard or custom prologues");
//
//            // splice prologues from the right into the left. We do this in reverse order
//            // so that we don't need to recompute the index on the left when we insert items.
//            auto left = isNodeArray(statements) ? statements.slice() : statements;
//
//            // splice other custom prologues from right into left
//            if (rightCustomPrologueEnd > rightHoistedVariablesEnd) {
//                left.splice(leftHoistedVariablesEnd, 0, ...declarations.slice(rightHoistedVariablesEnd, rightCustomPrologueEnd));
//            }
//
//            // splice hoisted variables from right into left
//            if (rightHoistedVariablesEnd > rightHoistedFunctionsEnd) {
//                left.splice(leftHoistedFunctionsEnd, 0, ...declarations.slice(rightHoistedFunctionsEnd, rightHoistedVariablesEnd));
//            }
//
//            // splice hoisted functions from right into left
//            if (rightHoistedFunctionsEnd > rightStandardPrologueEnd) {
//                left.splice(leftStandardPrologueEnd, 0, ...declarations.slice(rightStandardPrologueEnd, rightHoistedFunctionsEnd));
//            }
//
//            // splice standard prologues from right into left (that are not already in left)
//            if (rightStandardPrologueEnd > 0) {
//                if (leftStandardPrologueEnd == 0) {
//                    left.splice(0, 0, ...declarations.slice(0, rightStandardPrologueEnd));
//                }
//                else {
//                    auto leftPrologues = new Map<string, boolean>();
//                    for (let i = 0; i < leftStandardPrologueEnd; i++) {
//                        auto leftPrologue = statements[i] as PrologueDirective;
//                        leftPrologues.set(leftPrologue.expression.text, true);
//                    }
//                    for (let i = rightStandardPrologueEnd - 1; i >= 0; i--) {
//                        auto rightPrologue = declarations[i] as PrologueDirective;
//                        if (!leftPrologues.has(rightPrologue.expression.text)) {
//                            left.unshift(rightPrologue);
//                        }
//                    }
//                }
//            }
//
//            if (isNodeArray(statements)) {
//                return setTextRange(createNodeArray(left, statements.hasTrailingComma), statements);
//            }
//
//            return statements;
//        }
//
//        function updateModifiers<T extends HasModifiers>(node: T, modifiers: readonly Modifier[] | ModifierFlags): T;
//        function updateModifiers(node: HasModifiers, modifiers: readonly Modifier[] | ModifierFlags) {
//            let modifierArray;
//            if (typeof modifiers == "number") {
//                modifierArray = createModifiersFromModifierFlags(modifiers);
//            }
//            else {
//                modifierArray = modifiers;
//            }
//            return isParameter(node) ? updateParameterDeclaration(node, node->decorators, modifierArray, node->dotDotDotToken, node->name, node->questionToken, node->type, node->initializer) :
//                isPropertySignature(node) ? updatePropertySignature(node, modifierArray, node->name, node->questionToken, node->type) :
//                isPropertyDeclaration(node) ? updatePropertyDeclaration(node, node->decorators, modifierArray, node->name, node->questionToken ?? node->exclamationToken, node->type, node->initializer) :
//                isMethodSignature(node) ? updateMethodSignature(node, modifierArray, node->name, node->questionToken, node->typeParameters, node->parameters, node->type) :
//                isMethodDeclaration(node) ? updateMethodDeclaration(node, node->decorators, modifierArray, node->asteriskToken, node->name, node->questionToken, node->typeParameters, node->parameters, node->type, node->body) :
//                isConstructorDeclaration(node) ? updateConstructorDeclaration(node, node->decorators, modifierArray, node->parameters, node->body) :
//                isGetAccessorDeclaration(node) ? updateGetAccessorDeclaration(node, node->decorators, modifierArray, node->name, node->parameters, node->type, node->body) :
//                isSetAccessorDeclaration(node) ? updateSetAccessorDeclaration(node, node->decorators, modifierArray, node->name, node->parameters, node->body) :
//                isIndexSignatureDeclaration(node) ? updateIndexSignature(node, node->decorators, modifierArray, node->parameters, node->type) :
//                isFunctionExpression(node) ? updateFunctionExpression(node, modifierArray, node->asteriskToken, node->name, node->typeParameters, node->parameters, node->type, node->body) :
//                isArrowFunction(node) ? updateArrowFunction(node, modifierArray, node->typeParameters, node->parameters, node->type, node->equalsGreaterThanToken, node->body) :
//                isClassExpression(node) ? updateClassExpression(node, node->decorators, modifierArray, node->name, node->typeParameters, node->heritageClauses, node->members) :
//                isVariableStatement(node) ? updateVariableStatement(node, modifierArray, node->declarationList) :
//                isFunctionDeclaration(node) ? updateFunctionDeclaration(node, node->decorators, modifierArray, node->asteriskToken, node->name, node->typeParameters, node->parameters, node->type, node->body) :
//                isClassDeclaration(node) ? updateClassDeclaration(node, node->decorators, modifierArray, node->name, node->typeParameters, node->heritageClauses, node->members) :
//                isInterfaceDeclaration(node) ? updateInterfaceDeclaration(node, node->decorators, modifierArray, node->name, node->typeParameters, node->heritageClauses, node->members) :
//                isTypeAliasDeclaration(node) ? updateTypeAliasDeclaration(node, node->decorators, modifierArray, node->name, node->typeParameters, node->type) :
//                isEnumDeclaration(node) ? updateEnumDeclaration(node, node->decorators, modifierArray, node->name, node->members) :
//                isModuleDeclaration(node) ? updateModuleDeclaration(node, node->decorators, modifierArray, node->name, node->body) :
//                isImportEqualsDeclaration(node) ? updateImportEqualsDeclaration(node, node->decorators, modifierArray, node->isTypeOnly, node->name, node->moduleReference) :
//                isImportDeclaration(node) ? updateImportDeclaration(node, node->decorators, modifierArray, node->importClause, node->moduleSpecifier, node->assertClause) :
//                isExportAssignment(node) ? updateExportAssignment(node, node->decorators, modifierArray, node->expression) :
//                isExportDeclaration(node) ? updateExportDeclaration(node, node->decorators, modifierArray, node->isTypeOnly, node->exportClause, node->moduleSpecifier, node->assertClause) :
//                Debug.assertNever(node);
//        }
//
//        function asNodeArray<T extends Node>(array: readonly T[]): NodeArray<T>;
//        function asNodeArray<T extends Node>(array: readonly T[] | undefined): NodeArray<T> | undefined;
//        function asNodeArray<T extends Node>(array: readonly T[] | undefined): NodeArray<T> | undefined {
//            return array ? createNodeArray(array) : undefined;
//        }
//
//        function asToken<TKind extends SyntaxKind>(value: TKind | Token<TKind>): Token<TKind> {
//            return typeof value == "number" ? createToken(value) : value;
//        }
//
//        function asEmbeddedStatement<T extends Node>(statement: T): T | EmptyStatement;
//        function asEmbeddedStatement<T extends Node>(statement: T | undefined): T | EmptyStatement | undefined;

//    }

//    function updateWithOriginal<T extends Node>(updated: T, original: T): T {
//        if (updated != original) {
//            setOriginalNode(updated, original);
//            setTextRange(updated, original);
//        }
//        return updated;
//    }
//
//    function getDefaultTagNameForKind(kind: JSDocTag["kind"]): string {
//        switch (kind) {
//            case SyntaxKind::JSDocTypeTag: return "type";
//            case SyntaxKind::JSDocReturnTag: return "returns";
//            case SyntaxKind::JSDocThisTag: return "this";
//            case SyntaxKind::JSDocEnumTag: return "enum";
//            case SyntaxKind::JSDocAuthorTag: return "author";
//            case SyntaxKind::JSDocClassTag: return "class";
//            case SyntaxKind::JSDocPublicTag: return "public";
//            case SyntaxKind::JSDocPrivateTag: return "private";
//            case SyntaxKind::JSDocProtectedTag: return "protected";
//            case SyntaxKind::JSDocReadonlyTag: return "readonly";
//            case SyntaxKind::JSDocOverrideTag: return "override";
//            case SyntaxKind::JSDocTemplateTag: return "template";
//            case SyntaxKind::JSDocTypedefTag: return "typedef";
//            case SyntaxKind::JSDocParameterTag: return "param";
//            case SyntaxKind::JSDocPropertyTag: return "prop";
//            case SyntaxKind::JSDocCallbackTag: return "callback";
//            case SyntaxKind::JSDocAugmentsTag: return "augments";
//            case SyntaxKind::JSDocImplementsTag: return "implements";
//            default:
//                return Debug.fail(`Unsupported kind: ${Debug.formatSyntaxKind(kind)}`);
//        }
//    }
//
//    let rawTextScanner: Scanner | undefined;
//    auto invalidValueSentinel: object = { };
//
//    function getCookedText(kind: TemplateLiteralToken["kind"], rawText: string) {
//        if (!rawTextScanner) {
//            rawTextScanner = createScanner(ScriptTarget.Latest, /*skipTrivia*/ false, LanguageVariant.Standard);
//        }
//        switch (kind) {
//            case SyntaxKind::NoSubstitutionTemplateLiteral:
//                rawTextScanner.setText("`" + rawText + "`");
//                break;
//            case SyntaxKind::TemplateHead:
//                // tslint:disable-next-line no-invalid-template-strings
//                rawTextScanner.setText("`" + rawText + "${");
//                break;
//            case SyntaxKind::TemplateMiddle:
//                // tslint:disable-next-line no-invalid-template-strings
//                rawTextScanner.setText("}" + rawText + "${");
//                break;
//            case SyntaxKind::TemplateTail:
//                rawTextScanner.setText("}" + rawText + "`");
//                break;
//        }
//
//        let token = rawTextScanner.scan();
//        if (token == SyntaxKind::CloseBraceToken) {
//            token = rawTextScanner.reScanTemplateToken(/*isTaggedTemplate*/ false);
//        }
//
//        if (rawTextScanner.isUnterminated()) {
//            rawTextScanner.setText(undefined);
//            return invalidValueSentinel;
//        }
//
//        let tokenValue: string | undefined;
//        switch (token) {
//            case SyntaxKind::NoSubstitutionTemplateLiteral:
//            case SyntaxKind::TemplateHead:
//            case SyntaxKind::TemplateMiddle:
//            case SyntaxKind::TemplateTail:
//                tokenValue = rawTextScanner.getTokenValue();
//                break;
//        }
//
//        if (tokenValue == undefined || rawTextScanner.scan() != SyntaxKind::EndOfFileToken) {
//            rawTextScanner.setText(undefined);
//            return invalidValueSentinel;
//        }
//
//        rawTextScanner.setText(undefined);
//        return tokenValue;
//    }

/**
 * Gets the transform flags to exclude when unioning the transform flags of a subtree.
 */
/* @internal */
    TransformFlags getTransformFlagsSubtreeExclusions(SyntaxKind kind) {
        if (kind >= SyntaxKind::FirstTypeNode && kind <= SyntaxKind::LastTypeNode) {
            return TransformFlags::TypeExcludes;
        }

        switch (kind) {
            case SyntaxKind::CallExpression:
            case SyntaxKind::NewExpression:
            case SyntaxKind::ArrayLiteralExpression:
                return TransformFlags::ArrayLiteralOrCallOrNewExcludes;
            case SyntaxKind::ModuleDeclaration:
                return TransformFlags::ModuleExcludes;
            case SyntaxKind::Parameter:
                return TransformFlags::ParameterExcludes;
            case SyntaxKind::ArrowFunction:
                return TransformFlags::ArrowFunctionExcludes;
            case SyntaxKind::FunctionExpression:
            case SyntaxKind::FunctionDeclaration:
                return TransformFlags::FunctionExcludes;
            case SyntaxKind::VariableDeclarationList:
                return TransformFlags::VariableDeclarationListExcludes;
            case SyntaxKind::ClassDeclaration:
            case SyntaxKind::ClassExpression:
                return TransformFlags::ClassExcludes;
            case SyntaxKind::Constructor:
                return TransformFlags::ConstructorExcludes;
            case SyntaxKind::PropertyDeclaration:
                return TransformFlags::PropertyExcludes;
            case SyntaxKind::MethodDeclaration:
            case SyntaxKind::GetAccessor:
            case SyntaxKind::SetAccessor:
                return TransformFlags::MethodOrAccessorExcludes;
            case SyntaxKind::AnyKeyword:
            case SyntaxKind::NumberKeyword:
            case SyntaxKind::BigIntKeyword:
            case SyntaxKind::NeverKeyword:
            case SyntaxKind::StringKeyword:
            case SyntaxKind::ObjectKeyword:
            case SyntaxKind::BooleanKeyword:
            case SyntaxKind::SymbolKeyword:
            case SyntaxKind::VoidKeyword:
            case SyntaxKind::TypeParameter:
            case SyntaxKind::PropertySignature:
            case SyntaxKind::MethodSignature:
            case SyntaxKind::CallSignature:
            case SyntaxKind::ConstructSignature:
            case SyntaxKind::IndexSignature:
            case SyntaxKind::InterfaceDeclaration:
            case SyntaxKind::TypeAliasDeclaration:
                return TransformFlags::TypeExcludes;
            case SyntaxKind::ObjectLiteralExpression:
                return TransformFlags::ObjectLiteralExcludes;
            case SyntaxKind::CatchClause:
                return TransformFlags::CatchClauseExcludes;
            case SyntaxKind::ObjectBindingPattern:
            case SyntaxKind::ArrayBindingPattern:
                return TransformFlags::BindingPatternExcludes;
            case SyntaxKind::TypeAssertionExpression:
            case SyntaxKind::AsExpression:
            case SyntaxKind::PartiallyEmittedExpression:
            case SyntaxKind::ParenthesizedExpression:
            case SyntaxKind::SuperKeyword:
                return TransformFlags::OuterExpressionExcludes;
            case SyntaxKind::PropertyAccessExpression:
            case SyntaxKind::ElementAccessExpression:
                return TransformFlags::PropertyAccessExcludes;
            default:
                return TransformFlags::NodeExcludes;
        }
    }
//
//    auto baseFactory = createBaseNodeFactory();
//
//    function makeSynthetic(node: Node) {
//        (node as Mutable<Node>).flags |= NodeFlags::Synthesized;
//        return node;
//    }
//
//    auto syntheticFactory: BaseNodeFactory = {
//        createBaseSourceFileNode: kind => makeSynthetic(basefactory::createBaseSourceFileNode(kind)),
//        createBaseIdentifierNode: kind => makeSynthetic(basefactory::createBaseIdentifierNode(kind)),
//        createBasePrivateIdentifierNode: kind => makeSynthetic(basefactory::createBasePrivateIdentifierNode(kind)),
//        createBaseTokenNode: kind => makeSynthetic(basefactory::createBaseTokenNode(kind)),
//        createBaseNode: kind => makeSynthetic(basefactory::createBaseNode(kind)),
//    };
//
//    export auto factory = createNodeFactory(NodeFactoryFlags.NoIndentationOnFreshPropertyAccess, syntheticFactory);
//
//    export function createUnparsedSourceFile(text: string): UnparsedSource;
//    export function createUnparsedSourceFile(inputFile: InputFiles, type: "js" | "dts", stripInternal?: boolean): UnparsedSource;
//    export function createUnparsedSourceFile(text: string, mapPath: string | undefined, map: string | undefined): UnparsedSource;
//    export function createUnparsedSourceFile(textOrInputFiles: string | InputFiles, mapPathOrType?: string, mapTextOrStripInternal?: string | boolean): UnparsedSource {
//        let stripInternal: boolean | undefined;
//        let bundleFileInfo: BundleFileInfo | undefined;
//        let fileName: string;
//        let text: string | undefined;
//        let length: number | (() => number);
//        let sourceMapPath: string | undefined;
//        let sourceMapText: string | undefined;
//        let getText: (() => string) | undefined;
//        let getSourceMapText: (() => string | undefined) | undefined;
//        let oldFileOfCurrentEmit: boolean | undefined;
//
//        if (!isString(textOrInputFiles)) {
//            Debug::asserts(mapPathOrType == "js" || mapPathOrType == "dts");
//            fileName = (mapPathOrType == "js" ? textOrInputFiles.javascriptPath : textOrInputFiles.declarationPath) || "";
//            sourceMapPath = mapPathOrType == "js" ? textOrInputFiles.javascriptMapPath : textOrInputFiles.declarationMapPath;
//            getText = () => mapPathOrType == "js" ? textOrInputFiles.javascriptText : textOrInputFiles.declarationText;
//            getSourceMapText = () => mapPathOrType == "js" ? textOrInputFiles.javascriptMapText : textOrInputFiles.declarationMapText;
//            length = () => getText!().length;
//            if (textOrInputFiles.buildInfo && textOrInputFiles.buildInfo.bundle) {
//                Debug::asserts(mapTextOrStripInternal == undefined || typeof mapTextOrStripInternal == "boolean");
//                stripInternal = mapTextOrStripInternal;
//                bundleFileInfo = mapPathOrType == "js" ? textOrInputFiles.buildInfo.bundle.js : textOrInputFiles.buildInfo.bundle.dts;
//                oldFileOfCurrentEmit = textOrInputFiles.oldFileOfCurrentEmit;
//            }
//        }
//        else {
//            fileName = "";
//            text = textOrInputFiles;
//            length = textOrInputFiles.length;
//            sourceMapPath = mapPathOrType;
//            sourceMapText = mapTextOrStripInternal as string;
//        }
//        auto node = oldFileOfCurrentEmit ?
//            parseOldFileOfCurrentEmit(Debug.checkDefined(bundleFileInfo)) :
//            parseUnparsedSourceFile(bundleFileInfo, stripInternal, length);
//        node->fileName = fileName;
//        node->sourceMapPath = sourceMapPath;
//        node->oldFileOfCurrentEmit = oldFileOfCurrentEmit;
//        if (getText && getSourceMapText) {
//            Object.defineProperty(node, "text", { get: getText });
//            Object.defineProperty(node, "sourceMapText", { get: getSourceMapText });
//        }
//        else {
//            Debug::asserts(!oldFileOfCurrentEmit);
//            node->text = text ?? "";
//            node->sourceMapText = sourceMapText;
//        }
//
//        return node;
//    }
//
//    function parseUnparsedSourceFile(bundleFileInfo: BundleFileInfo | undefined, stripInternal: boolean | undefined, length: number | (() => number)) {
//        let prologues: UnparsedPrologue[] | undefined;
//        let helpers: UnscopedEmitHelper[] | undefined;
//        let referencedFiles: FileReference[] | undefined;
//        let typeReferenceDirectives: FileReference[] | undefined;
//        let libReferenceDirectives: FileReference[] | undefined;
//        let prependChildren: UnparsedTextLike[] | undefined;
//        let texts: UnparsedSourceText[] | undefined;
//        let hasNoDefaultLib: boolean | undefined;
//
//        for (auto section of bundleFileInfo ? bundleFileInfo.sections : emptyArray) {
//            switch (section.kind) {
//                case BundleFileSectionKind.Prologue:
//                    prologues = append(prologues, setTextRange(factory::createUnparsedPrologue(section.data), section));
//                    break;
//                case BundleFileSectionKind.EmitHelpers:
//                    helpers = append(helpers, getAllUnscopedEmitHelpers().get(section.data)!);
//                    break;
//                case BundleFileSectionKind.NoDefaultLib:
//                    hasNoDefaultLib = true;
//                    break;
//                case BundleFileSectionKind.Reference:
//                    referencedFiles = append(referencedFiles, { pos: -1, end: -1, fileName: section.data });
//                    break;
//                case BundleFileSectionKind.Type:
//                    typeReferenceDirectives = append(typeReferenceDirectives, { pos: -1, end: -1, fileName: section.data });
//                    break;
//                case BundleFileSectionKind.TypeResolutionModeImport:
//                    typeReferenceDirectives = append(typeReferenceDirectives, { pos: -1, end: -1, fileName: section.data, resolutionMode: ModuleKind.ESNext });
//                    break;
//                case BundleFileSectionKind.TypeResolutionModeRequire:
//                    typeReferenceDirectives = append(typeReferenceDirectives, { pos: -1, end: -1, fileName: section.data, resolutionMode: ModuleKind.CommonJS });
//                    break;
//                case BundleFileSectionKind.Lib:
//                    libReferenceDirectives = append(libReferenceDirectives, { pos: -1, end: -1, fileName: section.data });
//                    break;
//                case BundleFileSectionKind.Prepend:
//                    let prependTexts: UnparsedTextLike[] | undefined;
//                    for (auto text of section.texts) {
//                        if (!stripInternal || text.kind != BundleFileSectionKind.Internal) {
//                            prependTexts = append(prependTexts, setTextRange(factory::createUnparsedTextLike(text.data, text.kind == BundleFileSectionKind.Internal), text));
//                        }
//                    }
//                    prependChildren = addRange(prependChildren, prependTexts);
//                    texts = append(texts, factory::createUnparsedPrepend(section.data, prependTexts ?? emptyArray));
//                    break;
//                case BundleFileSectionKind.Internal:
//                    if (stripInternal) {
//                        if (!texts) texts = [];
//                        break;
//                    }
//                    // falls through
//
//                case BundleFileSectionKind.Text:
//                    texts = append(texts, setTextRange(factory::createUnparsedTextLike(section.data, section.kind == BundleFileSectionKind.Internal), section));
//                    break;
//                default:
//                    Debug.assertNever(section);
//            }
//        }
//
//        if (!texts) {
//            auto textNode = factory::createUnparsedTextLike(/*data*/ {}, /*internal*/ false);
//            setTextRangePosWidth(textNode, 0, typeof length == "function" ? length() : length);
//            texts = [textNode];
//        }
//
//        auto node = parseNodefactory::createUnparsedSource(prologues ?? emptyArray, /*syntheticReferences*/ {}, texts);
//        setEachParent(prologues, node);
//        setEachParent(texts, node);
//        setEachParent(prependChildren, node);
//        node->hasNoDefaultLib = hasNoDefaultLib;
//        node->helpers = helpers;
//        node->referencedFiles = referencedFiles || emptyArray;
//        node->typeReferenceDirectives = typeReferenceDirectives;
//        node->libReferenceDirectives = libReferenceDirectives || emptyArray;
//        return node;
//    }
//
//    function parseOldFileOfCurrentEmit(bundleFileInfo: BundleFileInfo) {
//        let texts: UnparsedTextLike[] | undefined;
//        let syntheticReferences: UnparsedSyntheticReference[] | undefined;
//        for (auto section of bundleFileInfo.sections) {
//            switch (section.kind) {
//                case BundleFileSectionKind.Internal:
//                case BundleFileSectionKind.Text:
//                    texts = append(texts, setTextRange(factory::createUnparsedTextLike(section.data, section.kind == BundleFileSectionKind.Internal), section));
//                    break;
//
//                case BundleFileSectionKind.NoDefaultLib:
//                case BundleFileSectionKind.Reference:
//                case BundleFileSectionKind.Type:
//                case BundleFileSectionKind.TypeResolutionModeImport:
//                case BundleFileSectionKind.TypeResolutionModeRequire:
//                case BundleFileSectionKind.Lib:
//                    syntheticReferences = append(syntheticReferences, setTextRange(factory::createUnparsedSyntheticReference(section), section));
//                    break;
//
//                // Ignore
//                case BundleFileSectionKind.Prologue:
//                case BundleFileSectionKind.EmitHelpers:
//                case BundleFileSectionKind.Prepend:
//                    break;
//
//                default:
//                    Debug.assertNever(section);
//            }
//        }
//
//        auto node = factory::createUnparsedSource(emptyArray, syntheticReferences, texts ?? emptyArray);
//        setEachParent(syntheticReferences, node);
//        setEachParent(texts, node);
//        node->helpers = map(bundleFileInfo.sources && bundleFileInfo.sources.helpers, name => getAllUnscopedEmitHelpers().get(name)!);
//        return node;
//    }
//
//    // TODO(rbuckton): Move part of this to factory
//    export function createInputFiles(
//        javascriptText: string,
//        declarationText: string
//    ): InputFiles;
//    export function createInputFiles(
//        readFileText: (path: string) => string | undefined,
//        javascriptPath: string,
//        javascriptMapPath: string | undefined,
//        declarationPath: string,
//        declarationMapPath: string | undefined,
//        buildInfoPath: string | undefined
//    ): InputFiles;
//    export function createInputFiles(
//        javascriptText: string,
//        declarationText: string,
//        javascriptMapPath: string | undefined,
//        javascriptMapText: string | undefined,
//        declarationMapPath: string | undefined,
//        declarationMapText: string | undefined
//    ): InputFiles;
//    /*@internal*/
//    export function createInputFiles(
//        javascriptText: string,
//        declarationText: string,
//        javascriptMapPath: string | undefined,
//        javascriptMapText: string | undefined,
//        declarationMapPath: string | undefined,
//        declarationMapText: string | undefined,
//        javascriptPath: string | undefined,
//        declarationPath: string | undefined,
//        buildInfoPath?: string | undefined,
//        buildInfo?: BuildInfo,
//        oldFileOfCurrentEmit?: boolean
//    ): InputFiles;
//    export function createInputFiles(
//        javascriptTextOrReadFileText: string | ((path: string) => string | undefined),
//        declarationTextOrJavascriptPath: string,
//        javascriptMapPath?: string,
//        javascriptMapTextOrDeclarationPath?: string,
//        declarationMapPath?: string,
//        declarationMapTextOrBuildInfoPath?: string,
//        javascriptPath?: string | undefined,
//        declarationPath?: string | undefined,
//        buildInfoPath?: string | undefined,
//        buildInfo?: BuildInfo,
//        oldFileOfCurrentEmit?: boolean
//    ): InputFiles {
//        auto node = parseNodefactory::createInputFiles();
//        if (!isString(javascriptTextOrReadFileText)) {
//            auto cache = new Map<string, string | false>();
//            auto textGetter = (path: string | undefined) => {
//                if (path == undefined) return undefined;
//                let value = cache.get(path);
//                if (value == undefined) {
//                    value = javascriptTextOrReadFileText(path);
//                    cache.set(path, value != undefined ? value : false);
//                }
//                return value != false ? value as string : undefined;
//            };
//            auto definedTextGetter = (path: string) => {
//                auto result = textGetter(path);
//                return result != undefined ? result : "/* Input file ${path} was missing */\r\n";
//            };
//            let buildInfo: BuildInfo | false;
//            auto getAndCacheBuildInfo = (getText: () => string | undefined) => {
//                if (buildInfo == undefined) {
//                    auto result = getText();
//                    buildInfo = result != undefined ? getBuildInfo(result) : false;
//                }
//                return buildInfo || undefined;
//            };
//            node->javascriptPath = declarationTextOrJavascriptPath;
//            node->javascriptMapPath = javascriptMapPath;
//            node->declarationPath = Debug.checkDefined(javascriptMapTextOrDeclarationPath);
//            node->declarationMapPath = declarationMapPath;
//            node->buildInfoPath = declarationMapTextOrBuildInfoPath;
//            Object.defineProperties(node, {
//                javascriptText: { get() { return definedTextGetter(declarationTextOrJavascriptPath); } },
//                javascriptMapText: { get() { return textGetter(javascriptMapPath); } }, // TODO:: if there is inline sourceMap in jsFile, use that
//                declarationText: { get() { return definedTextGetter(Debug.checkDefined(javascriptMapTextOrDeclarationPath)); } },
//                declarationMapText: { get() { return textGetter(declarationMapPath); } }, // TODO:: if there is inline sourceMap in dtsFile, use that
//                buildInfo: { get() { return getAndCacheBuildInfo(() => textGetter(declarationMapTextOrBuildInfoPath)); } }
//            });
//        }
//        else {
//            node->javascriptText = javascriptTextOrReadFileText;
//            node->javascriptMapPath = javascriptMapPath;
//            node->javascriptMapText = javascriptMapTextOrDeclarationPath;
//            node->declarationText = declarationTextOrJavascriptPath;
//            node->declarationMapPath = declarationMapPath;
//            node->declarationMapText = declarationMapTextOrBuildInfoPath;
//            node->javascriptPath = javascriptPath;
//            node->declarationPath = declarationPath;
//            node->buildInfoPath = buildInfoPath;
//            node->buildInfo = buildInfo;
//            node->oldFileOfCurrentEmit = oldFileOfCurrentEmit;
//        }
//        return node;
//    }
//
//    // tslint:disable-next-line variable-name
//    let SourceMapSource: new (fileName: string, text: string, skipTrivia?: (pos: number) => number) => SourceMapSource;
//
//    /**
//     * Create an external source map source file reference
//     */
//    export function createSourceMapSource(fileName: string, text: string, skipTrivia?: (pos: number) => number): SourceMapSource {
//        return new (SourceMapSource || (SourceMapSource = objectAllocator.getSourceMapSourceConstructor()))(fileName, text, skipTrivia);
//    }
//
//    // Utilities
//
//    function mergeTokenSourceMapRanges(sourceRanges: (TextRange | undefined)[], destRanges: (TextRange | undefined)[]) {
//        if (!destRanges) destRanges = [];
//        for (auto key in sourceRanges) {
//            destRanges[key] = sourceRanges[key];
//        }
//        return destRanges;
//    }
}
