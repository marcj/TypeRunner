//
// Created by Marc Schmidt on 30.05.22.
//

#include "parenthesizer.h"
#include "factory.h"

namespace tr {

    template<typename T>
    node<NodeArray> sameMap(Factory *factory, const optionalNode<NodeArray> &array, const function<node<T>(node<T>, int)> &callback) {
        auto result = factory->pool->construct<NodeArray>();
        if (array) {
            auto i = 0;
            for (auto v: *array) {
                result->push(callback(reinterpret_node<T>(v), i++));
            }
        }
        return result;
    }

    node<Expression> Parenthesizer::parenthesizeBinaryOperand(SyntaxKind binaryOperator, node<Expression> operand, bool isLeftSideOfBinary, optionalNode<Expression> leftOperand) {
        auto skipped = skipPartiallyEmittedExpressions(operand);

        // If the resulting expression is already parenthesized, we do not need to do any further processing.
        if (skipped->kind == SyntaxKind::ParenthesizedExpression) {
            return operand;
        }

        return binaryOperandNeedsParentheses(binaryOperator, operand, isLeftSideOfBinary, leftOperand)
               ? factory->createParenthesizedExpression(operand)
               : operand;
    }


    node<Expression> Parenthesizer::parenthesizeLeftSideOfBinary(SyntaxKind binaryOperator, node<Expression> leftSide) {
        return parenthesizeBinaryOperand(binaryOperator, leftSide, /*isLeftSideOfBinary*/ true);
    }

    node<Expression> Parenthesizer::parenthesizeExpressionOfComputedPropertyName(node<Expression> expression) {
        return isCommaSequence(expression) ? factory->createParenthesizedExpression(expression) : expression;
    }

    node<Expression> Parenthesizer::parenthesizeConditionOfConditionalExpression(node<Expression> condition) {
        auto conditionalPrecedence = getOperatorPrecedence(SyntaxKind::ConditionalExpression, SyntaxKind::QuestionToken);
        auto emittedCondition = skipPartiallyEmittedExpressions(condition);
        auto conditionPrecedence = getExpressionPrecedence(emittedCondition);
        if (compareValues(conditionPrecedence, conditionalPrecedence) != Comparison::GreaterThan) {
            return factory->createParenthesizedExpression(condition);
        }
        return condition;
    }

    optionalNode<NodeArray> Parenthesizer::parenthesizeTypeArguments(optionalNode<NodeArray> typeArguments) {
        if (typeArguments && !typeArguments->empty()) {
            auto m = sameMap<TypeNode>(factory, typeArguments, CALLBACK(parenthesizeOrdinalTypeArgument));
            return factory->createNodeArray(m);
        }
        return typeArguments;
    }

    node<TypeNode> Parenthesizer::parenthesizeTypeOfOptionalType(node<TypeNode> type) {
        if (hasJSDocPostfixQuestion(type)) return factory->createParenthesizedType(type);
        return parenthesizeNonArrayTypeOfPostfixType(type);
    }

    node<Expression> Parenthesizer::parenthesizeBranchOfConditionalExpression(node<Expression> branch) {
        // per ES grammar both 'whenTrue' and 'whenFalse' parts of conditional expression are assignment expressions
        // so in case when comma expression is introduced as a part of previous transformations
        // if should be wrapped in parens since comma operator has the lowest precedence
        auto emittedExpression = skipPartiallyEmittedExpressions(branch);
        return isCommaSequence(emittedExpression)
               ? factory->createParenthesizedExpression(branch)
               : branch;
    }
    
    node<NodeArray> Parenthesizer::parenthesizeElementTypesOfTupleType(node<NodeArray> types) {
        return factory->createNodeArray(sameMap<TypeNode>(factory, types, CALLBACK(parenthesizeElementTypeOfTupleType)));
    }
    
    node<TypeNode> Parenthesizer::parenthesizeLeadingTypeArgument(node<TypeNode> node) {
        return isFunctionOrConstructorTypeNode(node) && getTypeParameters(node) ? factory->createParenthesizedType(node) : node;
    }

    node<NodeArray> Parenthesizer::parenthesizeConstituentTypesOfIntersectionType(node<NodeArray> members) {
        return factory->createNodeArray(sameMap<TypeNode>(factory, members, CALLBACK(parenthesizeConstituentTypeOfIntersectionType)));
    }

    node<TypeNode> Parenthesizer::parenthesizeNonArrayTypeOfPostfixType(node<TypeNode> type) {
        switch (type->kind) {
            case SyntaxKind::InferType:
            case SyntaxKind::TypeOperator:
            case SyntaxKind::TypeQuery: // Not strictly necessary, but makes generated output more readable and avoids breaks in DT tests
                return factory->createParenthesizedType(type);
        }
        return parenthesizeOperandOfTypeOperator(type);
    }
    node<TypeNode> Parenthesizer::parenthesizeOperandOfReadonlyTypeOperator(node<TypeNode> type) {
        switch (type->kind) {
            case SyntaxKind::TypeOperator:
                return factory->createParenthesizedType(type);
        }
        return parenthesizeOperandOfTypeOperator(type);
    }
    node<TypeNode> Parenthesizer::parenthesizeOperandOfTypeOperator(node<TypeNode> type) {
        switch (type->kind) {
            case SyntaxKind::IntersectionType:
                return factory->createParenthesizedType(type);
        }
        return parenthesizeConstituentTypeOfIntersectionType(type);
    }
    node<TypeNode> Parenthesizer::parenthesizeConstituentTypeOfIntersectionType(node<TypeNode> type, int) {
        switch (type->kind) {
            case SyntaxKind::UnionType:
            case SyntaxKind::IntersectionType: // Not strictly necessary, but an intersection containing an intersection should have been flattened
                return factory->createParenthesizedType(type);
        }
        return parenthesizeConstituentTypeOfUnionType(type);
    }


    /**
     * Wraps an expression in parentheses if it is needed in order to use the expression for
     * property or element access.
     */
    node<LeftHandSideExpression> Parenthesizer::parenthesizeLeftSideOfAccess(node<Expression> expression) {
        // isLeftHandSideExpression is almost the correct criterion for when it is not necessary
        // to parenthesize the expression before a dot. The known exception is:
        //
        //    NewExpression:
        //       new C.x        -> not the same as (new C).x
        //
        auto emittedExpression = skipPartiallyEmittedExpressions(expression);
        if (isLeftHandSideExpression(emittedExpression)
            && (emittedExpression->kind != SyntaxKind::NewExpression || to<NewExpression>(emittedExpression)->arguments)) {
            // TODO(rbuckton): Verify whether this assertion holds.
            return reinterpret_node<LeftHandSideExpression>(expression);
        }

        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return setTextRange(factory->createParenthesizedExpression(expression), expression);
    }

    /**
     * Wraps an expression in parentheses if it is needed in order to use the expression
     * as the expression of a `NewExpression` node->
     */
    node<LeftHandSideExpression> Parenthesizer::parenthesizeExpressionOfNew(node<Expression> expression) {
        auto leftmostExpr = getLeftmostExpression(expression, /*stopAtCallExpressions*/ true);
        switch (leftmostExpr->kind) {
            case SyntaxKind::CallExpression:
                return factory->createParenthesizedExpression(expression);

            case SyntaxKind::NewExpression:
                return !to<NewExpression>(leftmostExpr)->arguments
                       ? factory->createParenthesizedExpression(expression)
                       : reinterpret_node<LeftHandSideExpression>(expression); // TODO(rbuckton): Verify this assertion holds
        }

        return parenthesizeLeftSideOfAccess(expression);
    }

    node<LeftHandSideExpression> Parenthesizer::parenthesizeOperandOfPostfixUnary(node<Expression> operand) {
        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return isLeftHandSideExpression(operand) ? reinterpret_node<LeftHandSideExpression>(operand) : setTextRange(factory->createParenthesizedExpression(operand), operand);
    }

    node<UnaryExpression> Parenthesizer::parenthesizeOperandOfPrefixUnary(node<Expression> operand) {
        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return isUnaryExpression(operand) ? reinterpret_node<UnaryExpression>(operand) : setTextRange(factory->createParenthesizedExpression(operand), operand);
    }

    node<Expression> Parenthesizer::parenthesizeExpressionForDisallowedComma(node<Expression> expression, int) {
        auto emittedExpression = skipPartiallyEmittedExpressions(expression);
        auto expressionPrecedence = getExpressionPrecedence(emittedExpression);
        auto commaPrecedence = getOperatorPrecedence(SyntaxKind::BinaryExpression, SyntaxKind::CommaToken);
        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return expressionPrecedence > commaPrecedence ? expression : setTextRange(factory->createParenthesizedExpression(expression), expression);
    }

    node<NodeArray> Parenthesizer::parenthesizeExpressionsOfCommaDelimitedList(node<NodeArray> elements) {
        auto result = sameMap<Expression>(factory, elements, CALLBACK(parenthesizeExpressionForDisallowedComma));
        return setArrayTextRange(factory->createNodeArray(result, elements->hasTrailingComma), elements);
    }

    node<Expression> Parenthesizer::parenthesizeExpressionOfExpressionStatement(node<Expression> expression) {
        auto emittedExpression = skipPartiallyEmittedExpressions(expression);
        if (auto e = to<CallExpression>(emittedExpression)) {
            auto callee = e->expression;
            auto kind = skipPartiallyEmittedExpressions(callee)->kind;
            if (kind == SyntaxKind::FunctionExpression || kind == SyntaxKind::ArrowFunction) {
                // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
                auto updated = factory->updateCallExpression(
                        e,
                        setTextRange(factory->createParenthesizedExpression(callee), callee),
                        e->typeArguments,
                        e->arguments
                );
                return factory->restoreOuterExpressions(expression, updated, (int) OuterExpressionKinds::PartiallyEmittedExpressions);
            }
        }

        auto leftmostExpressionKind = getLeftmostExpression(to<Expression>(emittedExpression), /*stopAtCallExpressions*/ false)->kind;
        if (leftmostExpressionKind == SyntaxKind::ObjectLiteralExpression || leftmostExpressionKind == SyntaxKind::FunctionExpression) {
            // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
            return setTextRange(factory->createParenthesizedExpression(expression), expression);
        }

        return expression;
    }

//    function parenthesizeConciseBodyOfArrowFunction(body: Expression): Expression;
//    function parenthesizeConciseBodyOfArrowFunction(body: ConciseBody): ConciseBody;
    node<Node> Parenthesizer::parenthesizeConciseBodyOfArrowFunction(node<Node> body) {
        if (isBlock(body)) return body;
        auto e = reinterpret_node<Expression>(body);
        if (isCommaSequence(body) || getLeftmostExpression(e, /*stopAtCallExpressions*/ false)->kind == SyntaxKind::ObjectLiteralExpression) {
            // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
            return setTextRange(factory->createParenthesizedExpression(e), body);
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
    node<TypeNode> Parenthesizer::parenthesizeCheckTypeOfConditionalType(node<TypeNode> checkType) {
        switch (checkType->kind) {
            case SyntaxKind::FunctionType:
            case SyntaxKind::ConstructorType:
            case SyntaxKind::ConditionalType:
                return factory->createParenthesizedType(checkType);
        }
        return checkType;
    }

    node<TypeNode> Parenthesizer::parenthesizeExtendsTypeOfConditionalType(node<TypeNode> extendsType) {
        switch (extendsType->kind) {
            case SyntaxKind::ConditionalType:
                return factory->createParenthesizedType(extendsType);
        }
        return extendsType;
    }

    // UnionType[Extends] :
    //     `|`? IntersectionType[?Extends]
    //     UnionType[?Extends] `|` IntersectionType[?Extends]
    //
    // - A union type constituent has the same precedence as the check type of a conditional type
    node<TypeNode> Parenthesizer::parenthesizeConstituentTypeOfUnionType(node<TypeNode> type, int) {
        switch (type->kind) {
            case SyntaxKind::UnionType: // Not strictly necessary, but a union containing a union should have been flattened
            case SyntaxKind::IntersectionType: // Not strictly necessary, but makes generated output more readable and avoids breaks in DT tests
                return factory->createParenthesizedType(type);
        }
        return parenthesizeCheckTypeOfConditionalType(type);
    }

    node<NodeArray> Parenthesizer::parenthesizeConstituentTypesOfUnionType(node<NodeArray> members) {
        return factory->createNodeArray(sameMap<TypeNode>(factory, members, CALLBACK(parenthesizeConstituentTypeOfUnionType)));
    }
}
