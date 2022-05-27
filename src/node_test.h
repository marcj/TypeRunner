#pragma once

#include "types.h";

namespace ts {
    // Literals

    bool isNumericLiteral(const shared<Node> &node) {
        return node->kind == SyntaxKind::NumericLiteral;
    }

    bool isBigIntLiteral(const shared<Node> &node) {
        return node->kind == SyntaxKind::BigIntLiteral;
    }

    bool isStringLiteral(const shared<Node> &node) {
        return node->kind == SyntaxKind::StringLiteral;
    }

    bool isJsxText(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxText;
    }

    bool isRegularExpressionLiteral(const shared<Node> &node) {
        return node->kind == SyntaxKind::RegularExpressionLiteral;
    }

    bool isNoSubstitutionTemplateLiteral(const shared<Node> &node) {
        return node->kind == SyntaxKind::NoSubstitutionTemplateLiteral;
    }

    // Pseudo-literals

    bool isTemplateHead(const shared<Node> &node) {
        return node->kind == SyntaxKind::TemplateHead;
    }

    bool isTemplateMiddle(const shared<Node> &node) {
        return node->kind == SyntaxKind::TemplateMiddle;
    }

    bool isTemplateTail(const shared<Node> &node) {
        return node->kind == SyntaxKind::TemplateTail;
    }

    // Punctuation

    bool isDotDotDotToken(const shared<Node> &node) {
        return node->kind == SyntaxKind::DotDotDotToken;
    }

    /*@internal*/
    bool isCommaToken(const shared<Node> &node) {
        return node->kind == SyntaxKind::CommaToken;
    }

    bool isPlusToken(const shared<Node> &node) {
        return node->kind == SyntaxKind::PlusToken;
    }

    bool isMinusToken(const shared<Node> &node) {
        return node->kind == SyntaxKind::MinusToken;
    }

    bool isAsteriskToken(const shared<Node> &node) {
        return node->kind == SyntaxKind::AsteriskToken;
    }

    /*@internal*/
    bool isExclamationToken(const shared<Node> &node) {
        return node->kind == SyntaxKind::ExclamationToken;
    }

    /*@internal*/
    bool isQuestionToken(const shared<Node> &node) {
        return node->kind == SyntaxKind::QuestionToken;
    }

    /*@internal*/
    bool isColonToken(const shared<Node> &node) {
        return node->kind == SyntaxKind::ColonToken;
    }

    /*@internal*/
    bool isQuestionDotToken(const shared<Node> &node) {
        return node->kind == SyntaxKind::QuestionDotToken;
    }

    /*@internal*/
    bool isEqualsGreaterThanToken(const shared<Node> &node) {
        return node->kind == SyntaxKind::EqualsGreaterThanToken;
    }

    // Identifiers

    bool isIdentifier(const shared<Node> &node) {
        return node->kind == SyntaxKind::Identifier;
    }

    bool isPrivateIdentifier(const shared<Node> &node) {
        return node->kind == SyntaxKind::PrivateIdentifier;
    }

    // Reserved Words

    /* @internal */
    bool isExportModifier(const shared<Node> &node) {
        return node->kind == SyntaxKind::ExportKeyword;
    }

    /* @internal */
    bool isAsyncModifier(const shared<Node> &node) {
        return node->kind == SyntaxKind::AsyncKeyword;
    }

    /* @internal */
    bool isAssertsKeyword(const shared<Node> &node) {
        return node->kind == SyntaxKind::AssertsKeyword;
    }

    /* @internal */
    bool isAwaitKeyword(const shared<Node> &node) {
        return node->kind == SyntaxKind::AwaitKeyword;
    }

    /* @internal */
    bool isReadonlyKeyword(const shared<Node> &node) {
        return node->kind == SyntaxKind::ReadonlyKeyword;
    }

    /* @internal */
    bool isStaticModifier(const shared<Node> &node) {
        return node->kind == SyntaxKind::StaticKeyword;
    }

    /* @internal */
    bool isAbstractModifier(const shared<Node> &node) {
        return node->kind == SyntaxKind::AbstractKeyword;
    }

    /*@internal*/
    bool isSuperKeyword(const shared<Node> &node) {
        return node->kind == SyntaxKind::SuperKeyword;
    }

    /*@internal*/
    bool isImportKeyword(const shared<Node> &node) {
        return node->kind == SyntaxKind::ImportKeyword;
    }

    // Names

    bool isQualifiedName(const shared<Node> &node) {
        return node->kind == SyntaxKind::QualifiedName;
    }

    bool isComputedPropertyName(const shared<Node> &node) {
        return node->kind == SyntaxKind::ComputedPropertyName;
    }

    // Signature elements

    bool isTypeParameterDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::TypeParameter;
    }

    // TODO(rbuckton): Rename to 'isParameterDeclaration'
    bool isParameter(const shared<Node> &node) {
        return node->kind == SyntaxKind::Parameter;
    }

    bool isDecorator(const shared<Node> &node) {
        return node->kind == SyntaxKind::Decorator;
    }

    // TypeMember

    bool isPropertySignature(const shared<Node> &node) {
        return node->kind == SyntaxKind::PropertySignature;
    }

    bool isPropertyDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::PropertyDeclaration;
    }

    bool isMethodSignature(const shared<Node> &node) {
        return node->kind == SyntaxKind::MethodSignature;
    }

    bool isMethodDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::MethodDeclaration;
    }

    bool isClassStaticBlockDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::ClassStaticBlockDeclaration;
    }

    bool isConstructorDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::Constructor;
    }

    bool isGetAccessorDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::GetAccessor;
    }

    bool isSetAccessorDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::SetAccessor;
    }

    bool isCallSignatureDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::CallSignature;
    }

    bool isConstructSignatureDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::ConstructSignature;
    }

    bool isIndexSignatureDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::IndexSignature;
    }

    // Type

    bool isTypePredicateNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::TypePredicate;
    }

    bool isTypeReferenceNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::TypeReference;
    }

    bool isFunctionTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::FunctionType;
    }

    bool isConstructorTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::ConstructorType;
    }

    bool isTypeQueryNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::TypeQuery;
    }

    bool isTypeLiteralNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::TypeLiteral;
    }

    bool isArrayTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::ArrayType;
    }

    bool isTupleTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::TupleType;
    }

    bool isNamedTupleMember(const shared<Node> &node) {
        return node->kind == SyntaxKind::NamedTupleMember;
    }

    bool isOptionalTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::OptionalType;
    }

    bool isRestTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::RestType;
    }

    bool isUnionTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::UnionType;
    }

    bool isIntersectionTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::IntersectionType;
    }

    bool isConditionalTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::ConditionalType;
    }

    bool isInferTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::InferType;
    }

    bool isParenthesizedTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::ParenthesizedType;
    }

    bool isThisTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::ThisType;
    }

    bool isTypeOperatorNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::TypeOperator;
    }

    bool isIndexedAccessTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::IndexedAccessType;
    }

    bool isMappedTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::MappedType;
    }

    bool isLiteralTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::LiteralType;
    }

    bool isImportTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::ImportType;
    }

    bool isTemplateLiteralTypeSpan(const shared<Node> &node) {
        return node->kind == SyntaxKind::TemplateLiteralTypeSpan;
    }

    bool isTemplateLiteralTypeNode(const shared<Node> &node) {
        return node->kind == SyntaxKind::TemplateLiteralType;
    }

    // Binding patterns

    bool isObjectBindingPattern(const shared<Node> &node) {
        return node->kind == SyntaxKind::ObjectBindingPattern;
    }

    bool isArrayBindingPattern(const shared<Node> &node) {
        return node->kind == SyntaxKind::ArrayBindingPattern;
    }

    bool isBindingElement(const shared<Node> &node) {
        return node->kind == SyntaxKind::BindingElement;
    }

    // Expression

    bool isArrayLiteralExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::ArrayLiteralExpression;
    }

    bool isObjectLiteralExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::ObjectLiteralExpression;
    }

    bool isPropertyAccessExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::PropertyAccessExpression;
    }

    bool isElementAccessExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::ElementAccessExpression;
    }

    bool isCallExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::CallExpression;
    }

    bool isNewExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::NewExpression;
    }

    bool isTaggedTemplateExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::TaggedTemplateExpression;
    }

    bool isTypeAssertionExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::TypeAssertionExpression;
    }

    bool isParenthesizedExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::ParenthesizedExpression;
    }

    bool isFunctionExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::FunctionExpression;
    }

    bool isArrowFunction(const shared<Node> &node) {
        return node->kind == SyntaxKind::ArrowFunction;
    }

    bool isDeleteExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::DeleteExpression;
    }

    bool isTypeOfExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::TypeOfExpression;
    }

    bool isVoidExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::VoidExpression;
    }

    bool isAwaitExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::AwaitExpression;
    }

    bool isPrefixUnaryExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::PrefixUnaryExpression;
    }

    bool isPostfixUnaryExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::PostfixUnaryExpression;
    }

    bool isBinaryExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::BinaryExpression;
    }

    bool isConditionalExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::ConditionalExpression;
    }

    bool isTemplateExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::TemplateExpression;
    }

    bool isYieldExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::YieldExpression;
    }

    bool isSpreadElement(const shared<Node> &node) {
        return node->kind == SyntaxKind::SpreadElement;
    }

    bool isClassExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::ClassExpression;
    }

    bool isOmittedExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::OmittedExpression;
    }

    bool isExpressionWithTypeArguments(const shared<Node> &node) {
        return node->kind == SyntaxKind::ExpressionWithTypeArguments;
    }

    bool isAsExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::AsExpression;
    }

    bool isNonNullExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::NonNullExpression;
    }

    bool isMetaProperty(const shared<Node> &node) {
        return node->kind == SyntaxKind::MetaProperty;
    }

    bool isSyntheticExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::SyntheticExpression;
    }

    bool isPartiallyEmittedExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::PartiallyEmittedExpression;
    }

    bool isCommaListExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::CommaListExpression;
    }

    // Misc

    bool isTemplateSpan(const shared<Node> &node) {
        return node->kind == SyntaxKind::TemplateSpan;
    }

    bool isSemicolonClassElement(const shared<Node> &node) {
        return node->kind == SyntaxKind::SemicolonClassElement;
    }

    // Elements

    bool isBlock(const shared<Node> &node) {
        return node->kind == SyntaxKind::Block;
    }

    bool isVariableStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::VariableStatement;
    }

    bool isEmptyStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::EmptyStatement;
    }

    bool isExpressionStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::ExpressionStatement;
    }

    bool isIfStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::IfStatement;
    }

    bool isDoStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::DoStatement;
    }

    bool isWhileStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::WhileStatement;
    }

    bool isForStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::ForStatement;
    }

    bool isForInStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::ForInStatement;
    }

    bool isForOfStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::ForOfStatement;
    }

    bool isContinueStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::ContinueStatement;
    }

    bool isBreakStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::BreakStatement;
    }

    bool isReturnStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::ReturnStatement;
    }

    bool isWithStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::WithStatement;
    }

    bool isSwitchStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::SwitchStatement;
    }

    bool isLabeledStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::LabeledStatement;
    }

    bool isThrowStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::ThrowStatement;
    }

    bool isTryStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::TryStatement;
    }

    bool isDebuggerStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::DebuggerStatement;
    }

    bool isVariableDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::VariableDeclaration;
    }

    bool isVariableDeclarationList(const shared<Node> &node) {
        return node->kind == SyntaxKind::VariableDeclarationList;
    }

    bool isFunctionDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::FunctionDeclaration;
    }

    bool isClassDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::ClassDeclaration;
    }

    bool isInterfaceDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::InterfaceDeclaration;
    }

    bool isTypeAliasDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::TypeAliasDeclaration;
    }

    bool isEnumDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::EnumDeclaration;
    }

    bool isModuleDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::ModuleDeclaration;
    }

    bool isModuleBlock(const shared<Node> &node) {
        return node->kind == SyntaxKind::ModuleBlock;
    }

    bool isCaseBlock(const shared<Node> &node) {
        return node->kind == SyntaxKind::CaseBlock;
    }

    bool isNamespaceExportDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::NamespaceExportDeclaration;
    }

    bool isImportEqualsDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::ImportEqualsDeclaration;
    }

    bool isImportDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::ImportDeclaration;
    }

    bool isImportClause(const shared<Node> &node) {
        return node->kind == SyntaxKind::ImportClause;
    }

    bool isAssertClause(const shared<Node> &node) {
        return node->kind == SyntaxKind::AssertClause;
    }

    bool isAssertEntry(const shared<Node> &node) {
        return node->kind == SyntaxKind::AssertEntry;
    }

    bool isNamespaceImport(const shared<Node> &node) {
        return node->kind == SyntaxKind::NamespaceImport;
    }

    bool isNamespaceExport(const shared<Node> &node) {
        return node->kind == SyntaxKind::NamespaceExport;
    }

    bool isNamedImports(const shared<Node> &node) {
        return node->kind == SyntaxKind::NamedImports;
    }

    bool isImportSpecifier(const shared<Node> &node) {
        return node->kind == SyntaxKind::ImportSpecifier;
    }

    bool isExportAssignment(const shared<Node> &node) {
        return node->kind == SyntaxKind::ExportAssignment;
    }

    bool isExportDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::ExportDeclaration;
    }

    bool isNamedExports(const shared<Node> &node) {
        return node->kind == SyntaxKind::NamedExports;
    }

    bool isExportSpecifier(const shared<Node> &node) {
        return node->kind == SyntaxKind::ExportSpecifier;
    }

    bool isMissingDeclaration(const shared<Node> &node) {
        return node->kind == SyntaxKind::MissingDeclaration;
    }

    bool isNotEmittedStatement(const shared<Node> &node) {
        return node->kind == SyntaxKind::NotEmittedStatement;
    }

    /* @internal */
    bool isSyntheticReference(const shared<Node> &node) {
        return node->kind == SyntaxKind::SyntheticReferenceExpression;
    }

    /* @internal */
    bool isMergeDeclarationMarker(const shared<Node> &node) {
        return node->kind == SyntaxKind::MergeDeclarationMarker;
    }

    /* @internal */
    bool isEndOfDeclarationMarker(const shared<Node> &node) {
        return node->kind == SyntaxKind::EndOfDeclarationMarker;
    }

    // Module References

    bool isExternalModuleReference(const shared<Node> &node) {
        return node->kind == SyntaxKind::ExternalModuleReference;
    }

    // JSX

    bool isJsxElement(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxElement;
    }

    bool isJsxSelfClosingElement(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxSelfClosingElement;
    }

    bool isJsxOpeningElement(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxOpeningElement;
    }

    bool isJsxClosingElement(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxClosingElement;
    }

    bool isJsxFragment(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxFragment;
    }

    bool isJsxOpeningFragment(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxOpeningFragment;
    }

    bool isJsxClosingFragment(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxClosingFragment;
    }

    bool isJsxAttribute(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxAttribute;
    }

    bool isJsxAttributes(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxAttributes;
    }

    bool isJsxSpreadAttribute(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxSpreadAttribute;
    }

    bool isJsxExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::JsxExpression;
    }

    // Clauses

    bool isCaseClause(const shared<Node> &node) {
        return node->kind == SyntaxKind::CaseClause;
    }

    bool isDefaultClause(const shared<Node> &node) {
        return node->kind == SyntaxKind::DefaultClause;
    }

    bool isHeritageClause(const shared<Node> &node) {
        return node->kind == SyntaxKind::HeritageClause;
    }

    bool isCatchClause(const shared<Node> &node) {
        return node->kind == SyntaxKind::CatchClause;
    }

    // Property assignments

    bool isPropertyAssignment(const shared<Node> &node) {
        return node->kind == SyntaxKind::PropertyAssignment;
    }

    bool isShorthandPropertyAssignment(const shared<Node> &node) {
        return node->kind == SyntaxKind::ShorthandPropertyAssignment;
    }

    bool isSpreadAssignment(const shared<Node> &node) {
        return node->kind == SyntaxKind::SpreadAssignment;
    }

    // Enum

    bool isEnumMember(const shared<Node> &node) {
        return node->kind == SyntaxKind::EnumMember;
    }

    // Unparsed

    // TODO(rbuckton): isUnparsedPrologue

    bool isUnparsedPrepend(const shared<Node> &node) {
        return node->kind == SyntaxKind::UnparsedPrepend;
    }

    // TODO(rbuckton): isUnparsedText
    // TODO(rbuckton): isUnparsedInternalText
    // TODO(rbuckton): isUnparsedSyntheticReference

    // Top-level nodes
    bool isSourceFile(const shared<Node> &node) {
        return node->kind == SyntaxKind::SourceFile;
    }

    bool isBundle(const shared<Node> &node) {
        return node->kind == SyntaxKind::Bundle;
    }

    bool isUnparsedSource(const shared<Node> &node) {
        return node->kind == SyntaxKind::UnparsedSource;
    }

    // TODO(rbuckton): isInputFiles

    // JSDoc Elements

    bool isJSDocTypeExpression(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocTypeExpression;
    }

    bool isJSDocNameReference(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocNameReference;
    }

    bool isJSDocMemberName(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocMemberName;
    }

    bool isJSDocLink(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocLink;
    }

    bool isJSDocLinkCode(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocLinkCode;
    }

    bool isJSDocLinkPlain(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocLinkPlain;
    }

    bool isJSDocAllType(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocAllType;
    }

    bool isJSDocUnknownType(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocUnknownType;
    }

    bool isJSDocNullableType(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocNullableType;
    }

    bool isJSDocNonNullableType(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocNonNullableType;
    }

    bool isJSDocOptionalType(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocOptionalType;
    }

    bool isJSDocFunctionType(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocFunctionType;
    }

    bool isJSDocVariadicType(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocVariadicType;
    }

    bool isJSDocNamepathType(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocNamepathType;
    }

    bool isJSDoc(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDoc;
    }

    bool isJSDocTypeLiteral(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocTypeLiteral;
    }

    bool isJSDocSignature(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocSignature;
    }

    // JSDoc Tags

    bool isJSDocAugmentsTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocAugmentsTag;
    }

    bool isJSDocAuthorTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocAuthorTag;
    }

    bool isJSDocClassTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocClassTag;
    }

    bool isJSDocCallbackTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocCallbackTag;
    }

    bool isJSDocPublicTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocPublicTag;
    }

    bool isJSDocPrivateTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocPrivateTag;
    }

    bool isJSDocProtectedTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocProtectedTag;
    }

    bool isJSDocReadonlyTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocReadonlyTag;
    }

    bool isJSDocOverrideTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocOverrideTag;
    }

    bool isJSDocDeprecatedTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocDeprecatedTag;
    }

    bool isJSDocSeeTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocSeeTag;
    }

    bool isJSDocEnumTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocEnumTag;
    }

    bool isJSDocParameterTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocParameterTag;
    }

    bool isJSDocReturnTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocReturnTag;
    }

    bool isJSDocThisTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocThisTag;
    }

    bool isJSDocTypeTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocTypeTag;
    }

    bool isJSDocTemplateTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocTemplateTag;
    }

    bool isJSDocTypedefTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocTypedefTag;
    }

    bool isJSDocUnknownTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocTag;
    }

    bool isJSDocPropertyTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocPropertyTag;
    }

    bool isJSDocImplementsTag(const shared<Node> &node) {
        return node->kind == SyntaxKind::JSDocImplementsTag;
    }

    // Synthesized list

    /* @internal */
    bool isSyntaxList(const shared<Node> &node){
        return node->kind == SyntaxKind::SyntaxList;
    }
}
