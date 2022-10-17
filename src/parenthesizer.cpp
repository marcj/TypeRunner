//
// Created by Marc Schmidt on 30.05.22.
//

#include "parenthesizer.h"
#include "factory.h"

namespace tr {

    template<typename T>
    shared<NodeArray> sameMap(const sharedOpt<NodeArray> &array, const function<shared<T>(shared<T>, int)> &callback) {
        auto result = make_shared<NodeArray>();
        if (array) {
            auto i = 0;
            for (auto &v: array->list) {
                result->list.push_back(callback(reinterpret_pointer_cast<T>(v), i++));
            }
        }
        return result;
    }

    shared<Expression> Parenthesizer::parenthesizeBinaryOperand(SyntaxKind binaryOperator, shared<Expression> operand, bool isLeftSideOfBinary, sharedOpt<Expression> leftOperand) {
        auto skipped = skipPartiallyEmittedExpressions(operand);

        // If the resulting expression is already parenthesized, we do not need to do any further processing.
        if (skipped->kind == SyntaxKind::ParenthesizedExpression) {
            return operand;
        }

        return binaryOperandNeedsParentheses(binaryOperator, operand, isLeftSideOfBinary, leftOperand)
               ? factory->createParenthesizedExpression(operand)
               : operand;
    }


    shared<Expression> Parenthesizer::parenthesizeLeftSideOfBinary(SyntaxKind binaryOperator, shared<Expression> leftSide) {
        return parenthesizeBinaryOperand(binaryOperator, leftSide, /*isLeftSideOfBinary*/ true);
    }

    shared<Expression> Parenthesizer::parenthesizeExpressionOfComputedPropertyName(shared<Expression> expression) {
        return isCommaSequence(expression) ? factory->createParenthesizedExpression(expression) : expression;
    }

    shared<Expression> Parenthesizer::parenthesizeConditionOfConditionalExpression(shared<Expression> condition) {
        auto conditionalPrecedence = getOperatorPrecedence(SyntaxKind::ConditionalExpression, SyntaxKind::QuestionToken);
        auto emittedCondition = skipPartiallyEmittedExpressions(condition);
        auto conditionPrecedence = getExpressionPrecedence(emittedCondition);
        if (compareValues(conditionPrecedence, conditionalPrecedence) != Comparison::GreaterThan) {
            return factory->createParenthesizedExpression(condition);
        }
        return condition;
    }

    sharedOpt<NodeArray> Parenthesizer::parenthesizeTypeArguments(sharedOpt<NodeArray> typeArguments) {
        if (typeArguments && !typeArguments->empty()) {
            auto m = sameMap<TypeNode>(typeArguments, CALLBACK(parenthesizeOrdinalTypeArgument));
            return factory->createNodeArray(m);
        }
        return typeArguments;
    }

    shared<TypeNode> Parenthesizer::parenthesizeTypeOfOptionalType(shared<TypeNode> type) {
        if (hasJSDocPostfixQuestion(type)) return factory->createParenthesizedType(type);
        return parenthesizeNonArrayTypeOfPostfixType(type);
    }

    shared<Expression> Parenthesizer::parenthesizeBranchOfConditionalExpression(shared<Expression> branch) {
        // per ES grammar both 'whenTrue' and 'whenFalse' parts of conditional expression are assignment expressions
        // so in case when comma expression is introduced as a part of previous transformations
        // if should be wrapped in parens since comma operator has the lowest precedence
        auto emittedExpression = skipPartiallyEmittedExpressions(branch);
        return isCommaSequence(emittedExpression)
               ? factory->createParenthesizedExpression(branch)
               : branch;
    }
    
    shared<NodeArray> Parenthesizer::parenthesizeElementTypesOfTupleType(shared<NodeArray> types) {
        return factory->createNodeArray(sameMap<TypeNode>(types, CALLBACK(parenthesizeElementTypeOfTupleType)));
    }
    
    shared<TypeNode> Parenthesizer::parenthesizeLeadingTypeArgument(shared<TypeNode> node) {
        return isFunctionOrConstructorTypeNode(node) && getTypeParameters(node) ? factory->createParenthesizedType(node) : node;
    }

    shared<NodeArray> Parenthesizer::parenthesizeConstituentTypesOfIntersectionType(shared<NodeArray> members) {
        return factory->createNodeArray(sameMap<TypeNode>(members, CALLBACK(parenthesizeConstituentTypeOfIntersectionType)));
    }

    shared<TypeNode> Parenthesizer::parenthesizeNonArrayTypeOfPostfixType(shared<TypeNode> type) {
        switch (type->kind) {
            case SyntaxKind::InferType:
            case SyntaxKind::TypeOperator:
            case SyntaxKind::TypeQuery: // Not strictly necessary, but makes generated output more readable and avoids breaks in DT tests
                return factory->createParenthesizedType(type);
        }
        return parenthesizeOperandOfTypeOperator(type);
    }
    shared<TypeNode> Parenthesizer::parenthesizeOperandOfReadonlyTypeOperator(shared<TypeNode> type) {
        switch (type->kind) {
            case SyntaxKind::TypeOperator:
                return factory->createParenthesizedType(type);
        }
        return parenthesizeOperandOfTypeOperator(type);
    }
    shared<TypeNode> Parenthesizer::parenthesizeOperandOfTypeOperator(shared<TypeNode> type) {
        switch (type->kind) {
            case SyntaxKind::IntersectionType:
                return factory->createParenthesizedType(type);
        }
        return parenthesizeConstituentTypeOfIntersectionType(type);
    }
    shared<TypeNode> Parenthesizer::parenthesizeConstituentTypeOfIntersectionType(shared<TypeNode> type, int) {
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
    shared<LeftHandSideExpression> Parenthesizer::parenthesizeLeftSideOfAccess(shared<Expression> expression) {
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
            return reinterpret_pointer_cast<LeftHandSideExpression>(expression);
        }

        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return setTextRange(factory->createParenthesizedExpression(expression), expression);
    }

    /**
     * Wraps an expression in parentheses if it is needed in order to use the expression
     * as the expression of a `NewExpression` node->
     */
    shared<LeftHandSideExpression> Parenthesizer::parenthesizeExpressionOfNew(shared<Expression> expression) {
        auto leftmostExpr = getLeftmostExpression(expression, /*stopAtCallExpressions*/ true);
        switch (leftmostExpr->kind) {
            case SyntaxKind::CallExpression:
                return factory->createParenthesizedExpression(expression);

            case SyntaxKind::NewExpression:
                return !to<NewExpression>(leftmostExpr)->arguments
                       ? factory->createParenthesizedExpression(expression)
                       : reinterpret_pointer_cast<LeftHandSideExpression>(expression); // TODO(rbuckton): Verify this assertion holds
        }

        return parenthesizeLeftSideOfAccess(expression);
    }

    shared<LeftHandSideExpression> Parenthesizer::parenthesizeOperandOfPostfixUnary(shared<Expression> operand) {
        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return isLeftHandSideExpression(operand) ? reinterpret_pointer_cast<LeftHandSideExpression>(operand) : setTextRange(factory->createParenthesizedExpression(operand), operand);
    }

    shared<UnaryExpression> Parenthesizer::parenthesizeOperandOfPrefixUnary(shared<Expression> operand) {
        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return isUnaryExpression(operand) ? reinterpret_pointer_cast<UnaryExpression>(operand) : setTextRange(factory->createParenthesizedExpression(operand), operand);
    }

    shared<Expression> Parenthesizer::parenthesizeExpressionForDisallowedComma(shared<Expression> expression, int) {
        auto emittedExpression = skipPartiallyEmittedExpressions(expression);
        auto expressionPrecedence = getExpressionPrecedence(emittedExpression);
        auto commaPrecedence = getOperatorPrecedence(SyntaxKind::BinaryExpression, SyntaxKind::CommaToken);
        // TODO(rbuckton): Verifiy whether `setTextRange` is needed.
        return expressionPrecedence > commaPrecedence ? expression : setTextRange(factory->createParenthesizedExpression(expression), expression);
    }

    shared<NodeArray> Parenthesizer::parenthesizeExpressionsOfCommaDelimitedList(shared<NodeArray> elements) {
        auto result = sameMap<Expression>(elements, CALLBACK(parenthesizeExpressionForDisallowedComma));
        return setTextRange(factory->createNodeArray(result, elements->hasTrailingComma), elements);
    }

    shared<Expression> Parenthesizer::parenthesizeExpressionOfExpressionStatement(shared<Expression> expression) {
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
    shared<Node> Parenthesizer::parenthesizeConciseBodyOfArrowFunction(shared<Node> body) {
        if (isBlock(body)) return body;
        auto e = reinterpret_pointer_cast<Expression>(body);
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
    shared<TypeNode> Parenthesizer::parenthesizeCheckTypeOfConditionalType(shared<TypeNode> checkType) {
        switch (checkType->kind) {
            case SyntaxKind::FunctionType:
            case SyntaxKind::ConstructorType:
            case SyntaxKind::ConditionalType:
                return factory->createParenthesizedType(checkType);
        }
        return checkType;
    }

    shared<TypeNode> Parenthesizer::parenthesizeExtendsTypeOfConditionalType(shared<TypeNode> extendsType) {
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
    shared<TypeNode> Parenthesizer::parenthesizeConstituentTypeOfUnionType(shared<TypeNode> type, int) {
        switch (type->kind) {
            case SyntaxKind::UnionType: // Not strictly necessary, but a union containing a union should have been flattened
            case SyntaxKind::IntersectionType: // Not strictly necessary, but makes generated output more readable and avoids breaks in DT tests
                return factory->createParenthesizedType(type);
        }
        return parenthesizeCheckTypeOfConditionalType(type);
    }

    shared<NodeArray> Parenthesizer::parenthesizeConstituentTypesOfUnionType(shared<NodeArray> members) {
        return factory->createNodeArray(sameMap<TypeNode>(members, CALLBACK(parenthesizeConstituentTypeOfUnionType)));
    }
}
