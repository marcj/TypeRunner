#pragma once

#include <functional>
#include "types.h"

using namespace ts;
using namespace ts::types;

inline bool forEachChild(Node *node, function<bool(Node&)> cbNode,function<bool(NodeArray&)> cbNodes) {
    if (node == nullptr || node->kind <= SyntaxKind::LastToken) {
        return false;
    }
    switch (node->kind) {
        case SyntaxKind::QualifiedName:
            return visitNode(cbNode, (node as QualifiedName).left) ||
                visitNode(cbNode, (node as QualifiedName).right);
        case SyntaxKind::TypeParameter:
            return visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as TypeParameterDeclaration).name) ||
                visitNode(cbNode, (node as TypeParameterDeclaration).constraint) ||
                visitNode(cbNode, (node as TypeParameterDeclaration).default) ||
                visitNode(cbNode, (node as TypeParameterDeclaration).expression);
        case SyntaxKind::ShorthandPropertyAssignment:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as ShorthandPropertyAssignment).name) ||
                visitNode(cbNode, (node as ShorthandPropertyAssignment).questionToken) ||
                visitNode(cbNode, (node as ShorthandPropertyAssignment).exclamationToken) ||
                visitNode(cbNode, (node as ShorthandPropertyAssignment).equalsToken) ||
                visitNode(cbNode, (node as ShorthandPropertyAssignment).objectAssignmentInitializer);
        case SyntaxKind::SpreadAssignment:
            return visitNode(cbNode, (node as SpreadAssignment).expression);
        case SyntaxKind::Parameter:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as ParameterDeclaration).dotDotDotToken) ||
                visitNode(cbNode, (node as ParameterDeclaration).name) ||
                visitNode(cbNode, (node as ParameterDeclaration).questionToken) ||
                visitNode(cbNode, (node as ParameterDeclaration).type) ||
                visitNode(cbNode, (node as ParameterDeclaration).initializer);
        case SyntaxKind::PropertyDeclaration:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as PropertyDeclaration).name) ||
                visitNode(cbNode, (node as PropertyDeclaration).questionToken) ||
                visitNode(cbNode, (node as PropertyDeclaration).exclamationToken) ||
                visitNode(cbNode, (node as PropertyDeclaration).type) ||
                visitNode(cbNode, (node as PropertyDeclaration).initializer);
        case SyntaxKind::PropertySignature:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as PropertySignature).name) ||
                visitNode(cbNode, (node as PropertySignature).questionToken) ||
                visitNode(cbNode, (node as PropertySignature).type) ||
                visitNode(cbNode, (node as PropertySignature).initializer);
        case SyntaxKind::PropertyAssignment:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as PropertyAssignment).name) ||
                visitNode(cbNode, (node as PropertyAssignment).questionToken) ||
                visitNode(cbNode, (node as PropertyAssignment).initializer);
        case SyntaxKind::VariableDeclaration:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as VariableDeclaration).name) ||
                visitNode(cbNode, (node as VariableDeclaration).exclamationToken) ||
                visitNode(cbNode, (node as VariableDeclaration).type) ||
                visitNode(cbNode, (node as VariableDeclaration).initializer);
        case SyntaxKind::BindingElement:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as BindingElement).dotDotDotToken) ||
                visitNode(cbNode, (node as BindingElement).propertyName) ||
                visitNode(cbNode, (node as BindingElement).name) ||
                visitNode(cbNode, (node as BindingElement).initializer);
        case SyntaxKind::FunctionType:
        case SyntaxKind::ConstructorType:
        case SyntaxKind::CallSignature:
        case SyntaxKind::ConstructSignature:
        case SyntaxKind::IndexSignature:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNodes(cbNode, cbNodes, (node as SignatureDeclaration).typeParameters) ||
                visitNodes(cbNode, cbNodes, (node as SignatureDeclaration).parameters) ||
                visitNode(cbNode, (node as SignatureDeclaration).type);
        case SyntaxKind::MethodDeclaration:
        case SyntaxKind::MethodSignature:
        case SyntaxKind::Constructor:
        case SyntaxKind::GetAccessor:
        case SyntaxKind::SetAccessor:
        case SyntaxKind::FunctionExpression:
        case SyntaxKind::FunctionDeclaration:
        case SyntaxKind::ArrowFunction:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as FunctionLikeDeclaration).asteriskToken) ||
                visitNode(cbNode, (node as FunctionLikeDeclaration).name) ||
                visitNode(cbNode, (node as FunctionLikeDeclaration).questionToken) ||
                visitNode(cbNode, (node as FunctionLikeDeclaration).exclamationToken) ||
                visitNodes(cbNode, cbNodes, (node as FunctionLikeDeclaration).typeParameters) ||
                visitNodes(cbNode, cbNodes, (node as FunctionLikeDeclaration).parameters) ||
                visitNode(cbNode, (node as FunctionLikeDeclaration).type) ||
                visitNode(cbNode, (node as ArrowFunction).equalsGreaterThanToken) ||
                visitNode(cbNode, (node as FunctionLikeDeclaration).body);
        case SyntaxKind::ClassStaticBlockDeclaration:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as ClassStaticBlockDeclaration).body);
        case SyntaxKind::TypeReference:
            return visitNode(cbNode, (node as TypeReferenceNode).typeName) ||
                visitNodes(cbNode, cbNodes, (node as TypeReferenceNode).typeArguments);
        case SyntaxKind::TypePredicate:
            return visitNode(cbNode, (node as TypePredicateNode).assertsModifier) ||
                visitNode(cbNode, (node as TypePredicateNode).parameterName) ||
                visitNode(cbNode, (node as TypePredicateNode).type);
        case SyntaxKind::TypeQuery:
            return visitNode(cbNode, (node as TypeQueryNode).exprName) ||
                visitNodes(cbNode, cbNodes, (node as TypeQueryNode).typeArguments);
        case SyntaxKind::TypeLiteral:
            return visitNodes(cbNode, cbNodes, (node as TypeLiteralNode).members);
        case SyntaxKind::ArrayType:
            return visitNode(cbNode, (node as ArrayTypeNode).elementType);
        case SyntaxKind::TupleType:
            return visitNodes(cbNode, cbNodes, (node as TupleTypeNode).elements);
        case SyntaxKind::UnionType:
        case SyntaxKind::IntersectionType:
            return visitNodes(cbNode, cbNodes, (node as UnionOrIntersectionTypeNode).types);
        case SyntaxKind::ConditionalType:
            return visitNode(cbNode, (node as ConditionalTypeNode).checkType) ||
                visitNode(cbNode, (node as ConditionalTypeNode).extendsType) ||
                visitNode(cbNode, (node as ConditionalTypeNode).trueType) ||
                visitNode(cbNode, (node as ConditionalTypeNode).falseType);
        case SyntaxKind::InferType:
            return visitNode(cbNode, (node as InferTypeNode).typeParameter);
        case SyntaxKind::ImportType:
            return visitNode(cbNode, (node as ImportTypeNode).argument) ||
                visitNode(cbNode, (node as ImportTypeNode).assertions) ||
                visitNode(cbNode, (node as ImportTypeNode).qualifier) ||
                visitNodes(cbNode, cbNodes, (node as ImportTypeNode).typeArguments);
        case SyntaxKind::ImportTypeAssertionContainer:
            return visitNode(cbNode, (node as ImportTypeAssertionContainer).assertClause);
        case SyntaxKind::ParenthesizedType:
        case SyntaxKind::TypeOperator:
            return visitNode(cbNode, (node as ParenthesizedTypeNode | TypeOperatorNode).type);
        case SyntaxKind::IndexedAccessType:
            return visitNode(cbNode, (node as IndexedAccessTypeNode).objectType) ||
                visitNode(cbNode, (node as IndexedAccessTypeNode).indexType);
        case SyntaxKind::MappedType:
            return visitNode(cbNode, (node as MappedTypeNode).readonlyToken) ||
                visitNode(cbNode, (node as MappedTypeNode).typeParameter) ||
                visitNode(cbNode, (node as MappedTypeNode).nameType) ||
                visitNode(cbNode, (node as MappedTypeNode).questionToken) ||
                visitNode(cbNode, (node as MappedTypeNode).type) ||
                visitNodes(cbNode, cbNodes, (node as MappedTypeNode).members);
        case SyntaxKind::LiteralType:
            return visitNode(cbNode, (node as LiteralTypeNode).literal);
        case SyntaxKind::NamedTupleMember:
            return visitNode(cbNode, (node as NamedTupleMember).dotDotDotToken) ||
                visitNode(cbNode, (node as NamedTupleMember).name) ||
                visitNode(cbNode, (node as NamedTupleMember).questionToken) ||
                visitNode(cbNode, (node as NamedTupleMember).type);
        case SyntaxKind::ObjectBindingPattern:
        case SyntaxKind::ArrayBindingPattern:
            return visitNodes(cbNode, cbNodes, (node as BindingPattern).elements);
        case SyntaxKind::ArrayLiteralExpression:
            return visitNodes(cbNode, cbNodes, (node as ArrayLiteralExpression).elements);
        case SyntaxKind::ObjectLiteralExpression:
            return visitNodes(cbNode, cbNodes, (node as ObjectLiteralExpression).properties);
        case SyntaxKind::PropertyAccessExpression:
            return visitNode(cbNode, (node as PropertyAccessExpression).expression) ||
                visitNode(cbNode, (node as PropertyAccessExpression).questionDotToken) ||
                visitNode(cbNode, (node as PropertyAccessExpression).name);
        case SyntaxKind::ElementAccessExpression:
            return visitNode(cbNode, (node as ElementAccessExpression).expression) ||
                visitNode(cbNode, (node as ElementAccessExpression).questionDotToken) ||
                visitNode(cbNode, (node as ElementAccessExpression).argumentExpression);
        case SyntaxKind::CallExpression:
        case SyntaxKind::NewExpression:
            return visitNode(cbNode, (node as CallExpression).expression) ||
                visitNode(cbNode, (node as CallExpression).questionDotToken) ||
                visitNodes(cbNode, cbNodes, (node as CallExpression).typeArguments) ||
                visitNodes(cbNode, cbNodes, (node as CallExpression).arguments);
        case SyntaxKind::TaggedTemplateExpression:
            return visitNode(cbNode, (node as TaggedTemplateExpression).tag) ||
                visitNode(cbNode, (node as TaggedTemplateExpression).questionDotToken) ||
                visitNodes(cbNode, cbNodes, (node as TaggedTemplateExpression).typeArguments) ||
                visitNode(cbNode, (node as TaggedTemplateExpression).template);
        case SyntaxKind::TypeAssertionExpression:
            return visitNode(cbNode, (node as TypeAssertion).type) ||
                visitNode(cbNode, (node as TypeAssertion).expression);
        case SyntaxKind::ParenthesizedExpression:
            return visitNode(cbNode, (node as ParenthesizedExpression).expression);
        case SyntaxKind::DeleteExpression:
            return visitNode(cbNode, (node as DeleteExpression).expression);
        case SyntaxKind::TypeOfExpression:
            return visitNode(cbNode, (node as TypeOfExpression).expression);
        case SyntaxKind::VoidExpression:
            return visitNode(cbNode, (node as VoidExpression).expression);
        case SyntaxKind::PrefixUnaryExpression:
            return visitNode(cbNode, (node as PrefixUnaryExpression).operand);
        case SyntaxKind::YieldExpression:
            return visitNode(cbNode, (node as YieldExpression).asteriskToken) ||
                visitNode(cbNode, (node as YieldExpression).expression);
        case SyntaxKind::AwaitExpression:
            return visitNode(cbNode, (node as AwaitExpression).expression);
        case SyntaxKind::PostfixUnaryExpression:
            return visitNode(cbNode, (node as PostfixUnaryExpression).operand);
        case SyntaxKind::BinaryExpression:
            return visitNode(cbNode, (node as BinaryExpression).left) ||
                visitNode(cbNode, (node as BinaryExpression).operatorToken) ||
                visitNode(cbNode, (node as BinaryExpression).right);
        case SyntaxKind::AsExpression:
            return visitNode(cbNode, (node as AsExpression).expression) ||
                visitNode(cbNode, (node as AsExpression).type);
        case SyntaxKind::NonNullExpression:
            return visitNode(cbNode, (node as NonNullExpression).expression);
        case SyntaxKind::MetaProperty:
            return visitNode(cbNode, (node as MetaProperty).name);
        case SyntaxKind::ConditionalExpression:
            return visitNode(cbNode, (node as ConditionalExpression).condition) ||
                visitNode(cbNode, (node as ConditionalExpression).questionToken) ||
                visitNode(cbNode, (node as ConditionalExpression).whenTrue) ||
                visitNode(cbNode, (node as ConditionalExpression).colonToken) ||
                visitNode(cbNode, (node as ConditionalExpression).whenFalse);
        case SyntaxKind::SpreadElement:
            return visitNode(cbNode, (node as SpreadElement).expression);
        case SyntaxKind::Block:
        case SyntaxKind::ModuleBlock:
            return visitNodes(cbNode, cbNodes, (node as Block).statements);
        case SyntaxKind::SourceFile:
            return visitNodes(cbNode, cbNodes, (node as SourceFile).statements) ||
                visitNode(cbNode, (node as SourceFile).endOfFileToken);
        case SyntaxKind::VariableStatement:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as VariableStatement).declarationList);
        case SyntaxKind::VariableDeclarationList:
            return visitNodes(cbNode, cbNodes, (node as VariableDeclarationList).declarations);
        case SyntaxKind::ExpressionStatement:
            return visitNode(cbNode, (node as ExpressionStatement).expression);
        case SyntaxKind::IfStatement:
            return visitNode(cbNode, (node as IfStatement).expression) ||
                visitNode(cbNode, (node as IfStatement).thenStatement) ||
                visitNode(cbNode, (node as IfStatement).elseStatement);
        case SyntaxKind::DoStatement:
            return visitNode(cbNode, (node as DoStatement).statement) ||
                visitNode(cbNode, (node as DoStatement).expression);
        case SyntaxKind::WhileStatement:
            return visitNode(cbNode, (node as WhileStatement).expression) ||
                visitNode(cbNode, (node as WhileStatement).statement);
        case SyntaxKind::ForStatement:
            return visitNode(cbNode, (node as ForStatement).initializer) ||
                visitNode(cbNode, (node as ForStatement).condition) ||
                visitNode(cbNode, (node as ForStatement).incrementor) ||
                visitNode(cbNode, (node as ForStatement).statement);
        case SyntaxKind::ForInStatement:
            return visitNode(cbNode, (node as ForInStatement).initializer) ||
                visitNode(cbNode, (node as ForInStatement).expression) ||
                visitNode(cbNode, (node as ForInStatement).statement);
        case SyntaxKind::ForOfStatement:
            return visitNode(cbNode, (node as ForOfStatement).awaitModifier) ||
                visitNode(cbNode, (node as ForOfStatement).initializer) ||
                visitNode(cbNode, (node as ForOfStatement).expression) ||
                visitNode(cbNode, (node as ForOfStatement).statement);
        case SyntaxKind::ContinueStatement:
        case SyntaxKind::BreakStatement:
            return visitNode(cbNode, (node as BreakOrContinueStatement).label);
        case SyntaxKind::ReturnStatement:
            return visitNode(cbNode, (node as ReturnStatement).expression);
        case SyntaxKind::WithStatement:
            return visitNode(cbNode, (node as WithStatement).expression) ||
                visitNode(cbNode, (node as WithStatement).statement);
        case SyntaxKind::SwitchStatement:
            return visitNode(cbNode, (node as SwitchStatement).expression) ||
                visitNode(cbNode, (node as SwitchStatement).caseBlock);
        case SyntaxKind::CaseBlock:
            return visitNodes(cbNode, cbNodes, (node as CaseBlock).clauses);
        case SyntaxKind::CaseClause:
            return visitNode(cbNode, (node as CaseClause).expression) ||
                visitNodes(cbNode, cbNodes, (node as CaseClause).statements);
        case SyntaxKind::DefaultClause:
            return visitNodes(cbNode, cbNodes, (node as DefaultClause).statements);
        case SyntaxKind::LabeledStatement:
            return visitNode(cbNode, (node as LabeledStatement).label) ||
                visitNode(cbNode, (node as LabeledStatement).statement);
        case SyntaxKind::ThrowStatement:
            return visitNode(cbNode, (node as ThrowStatement).expression);
        case SyntaxKind::TryStatement:
            return visitNode(cbNode, (node as TryStatement).tryBlock) ||
                visitNode(cbNode, (node as TryStatement).catchClause) ||
                visitNode(cbNode, (node as TryStatement).finallyBlock);
        case SyntaxKind::CatchClause:
            return visitNode(cbNode, (node as CatchClause).variableDeclaration) ||
                visitNode(cbNode, (node as CatchClause).block);
        case SyntaxKind::Decorator:
            return visitNode(cbNode, (node as Decorator).expression);
        case SyntaxKind::ClassDeclaration:
        case SyntaxKind::ClassExpression:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as ClassLikeDeclaration).name) ||
                visitNodes(cbNode, cbNodes, (node as ClassLikeDeclaration).typeParameters) ||
                visitNodes(cbNode, cbNodes, (node as ClassLikeDeclaration).heritageClauses) ||
                visitNodes(cbNode, cbNodes, (node as ClassLikeDeclaration).members);
        case SyntaxKind::InterfaceDeclaration:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as InterfaceDeclaration).name) ||
                visitNodes(cbNode, cbNodes, (node as InterfaceDeclaration).typeParameters) ||
                visitNodes(cbNode, cbNodes, (node as ClassDeclaration).heritageClauses) ||
                visitNodes(cbNode, cbNodes, (node as InterfaceDeclaration).members);
        case SyntaxKind::TypeAliasDeclaration:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as TypeAliasDeclaration).name) ||
                visitNodes(cbNode, cbNodes, (node as TypeAliasDeclaration).typeParameters) ||
                visitNode(cbNode, (node as TypeAliasDeclaration).type);
        case SyntaxKind::EnumDeclaration:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as EnumDeclaration).name) ||
                visitNodes(cbNode, cbNodes, (node as EnumDeclaration).members);
        case SyntaxKind::EnumMember:
            return visitNode(cbNode, (node as EnumMember).name) ||
                visitNode(cbNode, (node as EnumMember).initializer);
        case SyntaxKind::ModuleDeclaration:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as ModuleDeclaration).name) ||
                visitNode(cbNode, (node as ModuleDeclaration).body);
        case SyntaxKind::ImportEqualsDeclaration:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as ImportEqualsDeclaration).name) ||
                visitNode(cbNode, (node as ImportEqualsDeclaration).moduleReference);
        case SyntaxKind::ImportDeclaration:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as ImportDeclaration).importClause) ||
                visitNode(cbNode, (node as ImportDeclaration).moduleSpecifier) ||
                visitNode(cbNode, (node as ImportDeclaration).assertClause);
        case SyntaxKind::ImportClause:
            return visitNode(cbNode, (node as ImportClause).name) ||
                visitNode(cbNode, (node as ImportClause).namedBindings);
        case SyntaxKind::AssertClause:
            return visitNodes(cbNode, cbNodes, (node as AssertClause).elements);
        case SyntaxKind::AssertEntry:
            return visitNode(cbNode, (node as AssertEntry).name) ||
                visitNode(cbNode, (node as AssertEntry).value);
        case SyntaxKind::NamespaceExportDeclaration:
            return visitNode(cbNode, (node as NamespaceExportDeclaration).name);
        case SyntaxKind::NamespaceImport:
            return visitNode(cbNode, (node as NamespaceImport).name);
        case SyntaxKind::NamespaceExport:
            return visitNode(cbNode, (node as NamespaceExport).name);
        case SyntaxKind::NamedImports:
        case SyntaxKind::NamedExports:
            return visitNodes(cbNode, cbNodes, (node as NamedImportsOrExports).elements);
        case SyntaxKind::ExportDeclaration:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as ExportDeclaration).exportClause) ||
                visitNode(cbNode, (node as ExportDeclaration).moduleSpecifier) ||
                visitNode(cbNode, (node as ExportDeclaration).assertClause);
        case SyntaxKind::ImportSpecifier:
        case SyntaxKind::ExportSpecifier:
            return visitNode(cbNode, (node as ImportOrExportSpecifier).propertyName) ||
                visitNode(cbNode, (node as ImportOrExportSpecifier).name);
        case SyntaxKind::ExportAssignment:
            return visitNodes(cbNode, cbNodes, node.decorators) ||
                visitNodes(cbNode, cbNodes, node.modifiers) ||
                visitNode(cbNode, (node as ExportAssignment).expression);
        case SyntaxKind::TemplateExpression:
            return visitNode(cbNode, (node as TemplateExpression).head) || visitNodes(cbNode, cbNodes, (node as TemplateExpression).templateSpans);
        case SyntaxKind::TemplateSpan:
            return visitNode(cbNode, (node as TemplateSpan).expression) || visitNode(cbNode, (node as TemplateSpan).literal);
        case SyntaxKind::TemplateLiteralType:
            return visitNode(cbNode, (node as TemplateLiteralTypeNode).head) || visitNodes(cbNode, cbNodes, (node as TemplateLiteralTypeNode).templateSpans);
        case SyntaxKind::TemplateLiteralTypeSpan:
            return visitNode(cbNode, (node as TemplateLiteralTypeSpan).type) || visitNode(cbNode, (node as TemplateLiteralTypeSpan).literal);
        case SyntaxKind::ComputedPropertyName:
            return visitNode(cbNode, (node as ComputedPropertyName).expression);
        case SyntaxKind::HeritageClause:
            return visitNodes(cbNode, cbNodes, (node as HeritageClause).types);
        case SyntaxKind::ExpressionWithTypeArguments:
            return visitNode(cbNode, (node as ExpressionWithTypeArguments).expression) ||
                visitNodes(cbNode, cbNodes, (node as ExpressionWithTypeArguments).typeArguments);
        case SyntaxKind::ExternalModuleReference:
            return visitNode(cbNode, (node as ExternalModuleReference).expression);
        case SyntaxKind::MissingDeclaration:
            return visitNodes(cbNode, cbNodes, node.decorators);
        case SyntaxKind::CommaListExpression:
            return visitNodes(cbNode, cbNodes, (node as CommaListExpression).elements);

        case SyntaxKind::JsxElement:
            return visitNode(cbNode, (node as JsxElement).openingElement) ||
                visitNodes(cbNode, cbNodes, (node as JsxElement).children) ||
                visitNode(cbNode, (node as JsxElement).closingElement);
        case SyntaxKind::JsxFragment:
            return visitNode(cbNode, (node as JsxFragment).openingFragment) ||
                visitNodes(cbNode, cbNodes, (node as JsxFragment).children) ||
                visitNode(cbNode, (node as JsxFragment).closingFragment);
        case SyntaxKind::JsxSelfClosingElement:
        case SyntaxKind::JsxOpeningElement:
            return visitNode(cbNode, (node as JsxOpeningLikeElement).tagName) ||
                visitNodes(cbNode, cbNodes, (node as JsxOpeningLikeElement).typeArguments) ||
                visitNode(cbNode, (node as JsxOpeningLikeElement).attributes);
        case SyntaxKind::JsxAttributes:
            return visitNodes(cbNode, cbNodes, (node as JsxAttributes).properties);
        case SyntaxKind::JsxAttribute:
            return visitNode(cbNode, (node as JsxAttribute).name) ||
                visitNode(cbNode, (node as JsxAttribute).initializer);
        case SyntaxKind::JsxSpreadAttribute:
            return visitNode(cbNode, (node as JsxSpreadAttribute).expression);
        case SyntaxKind::JsxExpression:
            return visitNode(cbNode, (node as JsxExpression).dotDotDotToken) ||
                visitNode(cbNode, (node as JsxExpression).expression);
        case SyntaxKind::JsxClosingElement:
            return visitNode(cbNode, (node as JsxClosingElement).tagName);

        case SyntaxKind::OptionalType:
        case SyntaxKind::RestType:
        case SyntaxKind::JSDocTypeExpression:
        case SyntaxKind::JSDocNonNullableType:
        case SyntaxKind::JSDocNullableType:
        case SyntaxKind::JSDocOptionalType:
        case SyntaxKind::JSDocVariadicType:
            return visitNode(cbNode, (node as OptionalTypeNode | RestTypeNode | JSDocTypeExpression | JSDocTypeReferencingNode).type);
        case SyntaxKind::JSDocFunctionType:
            return visitNodes(cbNode, cbNodes, (node as JSDocFunctionType).parameters) ||
                visitNode(cbNode, (node as JSDocFunctionType).type);
        case SyntaxKind::JSDoc:
            return (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined))
                || visitNodes(cbNode, cbNodes, (node as JSDoc).tags);
        case SyntaxKind::JSDocSeeTag:
            return visitNode(cbNode, (node as JSDocSeeTag).tagName) ||
                visitNode(cbNode, (node as JSDocSeeTag).name) ||
                (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined));
        case SyntaxKind::JSDocNameReference:
            return visitNode(cbNode, (node as JSDocNameReference).name);
        case SyntaxKind::JSDocMemberName:
            return visitNode(cbNode, (node as JSDocMemberName).left) ||
                visitNode(cbNode, (node as JSDocMemberName).right);
        case SyntaxKind::JSDocParameterTag:
        case SyntaxKind::JSDocPropertyTag:
            return visitNode(cbNode, (node as JSDocTag).tagName) ||
                ((node as JSDocPropertyLikeTag).isNameFirst
                    ? visitNode(cbNode, (node as JSDocPropertyLikeTag).name) ||
                        visitNode(cbNode, (node as JSDocPropertyLikeTag).typeExpression) ||
                        (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined))
                    : visitNode(cbNode, (node as JSDocPropertyLikeTag).typeExpression) ||
                        visitNode(cbNode, (node as JSDocPropertyLikeTag).name) ||
                        (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined)));
        case SyntaxKind::JSDocAuthorTag:
            return visitNode(cbNode, (node as JSDocTag).tagName) ||
                (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined));
        case SyntaxKind::JSDocImplementsTag:
            return visitNode(cbNode, (node as JSDocTag).tagName) ||
                visitNode(cbNode, (node as JSDocImplementsTag).class) ||
                (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined));
        case SyntaxKind::JSDocAugmentsTag:
            return visitNode(cbNode, (node as JSDocTag).tagName) ||
                visitNode(cbNode, (node as JSDocAugmentsTag).class) ||
                (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined));
        case SyntaxKind::JSDocTemplateTag:
            return visitNode(cbNode, (node as JSDocTag).tagName) ||
                visitNode(cbNode, (node as JSDocTemplateTag).constraint) ||
                visitNodes(cbNode, cbNodes, (node as JSDocTemplateTag).typeParameters) ||
                (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined));
        case SyntaxKind::JSDocTypedefTag:
            return visitNode(cbNode, (node as JSDocTag).tagName) ||
                ((node as JSDocTypedefTag).typeExpression &&
                    (node as JSDocTypedefTag).typeExpression!.kind === SyntaxKind::JSDocTypeExpression
                    ? visitNode(cbNode, (node as JSDocTypedefTag).typeExpression) ||
                        visitNode(cbNode, (node as JSDocTypedefTag).fullName) ||
                        (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined))
                    : visitNode(cbNode, (node as JSDocTypedefTag).fullName) ||
                        visitNode(cbNode, (node as JSDocTypedefTag).typeExpression) ||
                        (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined)));
        case SyntaxKind::JSDocCallbackTag:
            return visitNode(cbNode, (node as JSDocTag).tagName) ||
                visitNode(cbNode, (node as JSDocCallbackTag).fullName) ||
                visitNode(cbNode, (node as JSDocCallbackTag).typeExpression) ||
                (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined));
        case SyntaxKind::JSDocReturnTag:
        case SyntaxKind::JSDocTypeTag:
        case SyntaxKind::JSDocThisTag:
        case SyntaxKind::JSDocEnumTag:
            return visitNode(cbNode, (node as JSDocTag).tagName) ||
                visitNode(cbNode, (node as JSDocReturnTag | JSDocTypeTag | JSDocThisTag | JSDocEnumTag).typeExpression) ||
                (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined));
        case SyntaxKind::JSDocSignature:
            return forEach((node as JSDocSignature).typeParameters, cbNode) ||
                forEach((node as JSDocSignature).parameters, cbNode) ||
                visitNode(cbNode, (node as JSDocSignature).type);
        case SyntaxKind::JSDocLink:
        case SyntaxKind::JSDocLinkCode:
        case SyntaxKind::JSDocLinkPlain:
            return visitNode(cbNode, (node as JSDocLink | JSDocLinkCode | JSDocLinkPlain).name);
        case SyntaxKind::JSDocTypeLiteral:
            return forEach((node as JSDocTypeLiteral).jsDocPropertyTags, cbNode);
        case SyntaxKind::JSDocTag:
        case SyntaxKind::JSDocClassTag:
        case SyntaxKind::JSDocPublicTag:
        case SyntaxKind::JSDocPrivateTag:
        case SyntaxKind::JSDocProtectedTag:
        case SyntaxKind::JSDocReadonlyTag:
        case SyntaxKind::JSDocDeprecatedTag:
            return visitNode(cbNode, (node as JSDocTag).tagName)
             || (typeof (node as JSDoc).comment === "string" ? undefined : visitNodes(cbNode, cbNodes, (node as JSDoc).comment as NodeArray<JSDocComment> | undefined));
        case SyntaxKind::PartiallyEmittedExpression:
            return visitNode(cbNode, (node as PartiallyEmittedExpression).expression);
    }
}

