#pragma once

#include <utility>
#include "types.h"
#include "utilities.h"
#include "scanner.h"
#include "node_test.h"

namespace ts {
    using namespace ts::types;

    struct Factory;

    struct Parenthesizer {
        Factory *factory;

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
//            ? factory->createParenthesizedExpression(operand)
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

        shared <Expression> parenthesizeExpressionOfComputedPropertyName(shared <Expression> expression);

        shared <Expression> parenthesizeConditionOfConditionalExpression(shared <Expression> condition);

        shared <Expression> parenthesizeBranchOfConditionalExpression(shared <Expression> branch);

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
//        return needsParens ? factory->createParenthesizedExpression(expression) : expression;
//    }
//

        /**
         * Wraps an expression in parentheses if it is needed in order to use the expression for
         * property or element access.
         */
        shared <LeftHandSideExpression> parenthesizeLeftSideOfAccess(shared <Expression> expression);

        /**
         * Wraps an expression in parentheses if it is needed in order to use the expression
         * as the expression of a `NewExpression` node->
         */
        shared <LeftHandSideExpression> parenthesizeExpressionOfNew(shared <Expression> expression);

        shared <LeftHandSideExpression> parenthesizeOperandOfPostfixUnary(shared <Expression> operand);

        shared <UnaryExpression> parenthesizeOperandOfPrefixUnary(shared <Expression> operand);

        shared <Expression> parenthesizeExpressionForDisallowedComma(shared <Expression> expression, int = 0);

        NodeArray parenthesizeExpressionsOfCommaDelimitedList(NodeArray elements);

        shared <Expression> parenthesizeExpressionOfExpressionStatement(shared <Expression> expression);

//    function parenthesizeConciseBodyOfArrowFunction(body: Expression): Expression;
//    function parenthesizeConciseBodyOfArrowFunction(body: ConciseBody): ConciseBody;
        shared <Node> parenthesizeConciseBodyOfArrowFunction(shared <Node> body);

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
        shared <TypeNode> parenthesizeCheckTypeOfConditionalType(shared <TypeNode> checkType);

        shared <TypeNode> parenthesizeExtendsTypeOfConditionalType(shared <TypeNode> extendsType);

        // UnionType[Extends] :
        //     `|`? IntersectionType[?Extends]
        //     UnionType[?Extends] `|` IntersectionType[?Extends]
        //
        // - A union type constituent has the same precedence as the check type of a conditional type
        shared <TypeNode> parenthesizeConstituentTypeOfUnionType(shared <TypeNode> type, int = 0);

        NodeArray parenthesizeConstituentTypesOfUnionType(NodeArray members);

        // IntersectionType[Extends] :
        //     `&`? TypeOperator[?Extends]
        //     IntersectionType[?Extends] `&` TypeOperator[?Extends]
        //
        // - An intersection type constituent does not allow function, constructor, conditional, or union types (they must be parenthesized)
        shared <TypeNode> parenthesizeConstituentTypeOfIntersectionType(shared <TypeNode> type, int = 0);

        NodeArray parenthesizeConstituentTypesOfIntersectionType(NodeArray members);

        // TypeOperator[Extends] :
        //     PostfixType
        //     InferType[?Extends]
        //     `keyof` TypeOperator[?Extends]
        //     `unique` TypeOperator[?Extends]
        //     `readonly` TypeOperator[?Extends]
        //
        shared <TypeNode> parenthesizeOperandOfTypeOperator(shared <TypeNode> type);

        shared <TypeNode> parenthesizeOperandOfReadonlyTypeOperator(shared <TypeNode> type);

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
        shared <TypeNode> parenthesizeNonArrayTypeOfPostfixType(shared <TypeNode> type);

        shared <TypeNode> parenthesizeElementTypeOfTupleType(shared <TypeNode> type, int = 0) {
//        if (hasJSDocPostfixQuestion(type)) return factory->createParenthesizedType(type);
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
        NodeArray parenthesizeElementTypesOfTupleType(NodeArray types);

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
//        if (hasJSDocPostfixQuestion(type)) return factory->createParenthesizedType(type);
//        return parenthesizeNonArrayTypeOfPostfixType(type);
//    }
//
//    // function parenthesizeMemberOfElementType(member: TypeNode): TypeNode {
//    //     switch (member.kind) {
//    //         case SyntaxKind::UnionType:
//    //         case SyntaxKind::IntersectionType:
//    //         case SyntaxKind::FunctionType:
//    //         case SyntaxKind::ConstructorType:
//    //             return factory->createParenthesizedType(member);
//    //     }
//    //     return parenthesizeMemberOfConditionalType(member);
//    // }
//
//    // function parenthesizeElementTypeOfArrayType(member: TypeNode): TypeNode {
//    //     switch (member.kind) {
//    //         case SyntaxKind::TypeQuery:
//    //         case SyntaxKind::TypeOperator:
//    //         case SyntaxKind::InferType:
//    //             return factory->createParenthesizedType(member);
//    //     }
//    //     return parenthesizeMemberOfElementType(member);
//    // }
//
        shared <TypeNode> parenthesizeLeadingTypeArgument(shared <TypeNode> node);

        shared <TypeNode> parenthesizeOrdinalTypeArgument(shared <TypeNode> node, int i) {
            return i == 0 ? parenthesizeLeadingTypeArgument(node) : node;
        }

        optional<NodeArray> parenthesizeTypeArguments(optional<NodeArray> typeArguments);
    };
}
