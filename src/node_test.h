#pragma once

#include "types.h";

namespace ts {
    // Literals

    bool isNumericLiteral(Node &node) {
        return node.kind == SyntaxKind::NumericLiteral;
    }

    bool isBigIntLiteral(Node &node) {
        return node.kind == SyntaxKind::BigIntLiteral;
    }

    bool isStringLiteral(Node &node) {
        return node.kind == SyntaxKind::StringLiteral;
    }

    bool isJsxText(Node &node) {
        return node.kind == SyntaxKind::JsxText;
    }

    bool isRegularExpressionLiteral(Node &node) {
        return node.kind == SyntaxKind::RegularExpressionLiteral;
    }

    bool isNoSubstitutionTemplateLiteral(Node &node) {
        return node.kind == SyntaxKind::NoSubstitutionTemplateLiteral;
    }

    // Pseudo-literals

    bool isTemplateHead(Node &node) {
        return node.kind == SyntaxKind::TemplateHead;
    }

    bool isTemplateMiddle(Node &node) {
        return node.kind == SyntaxKind::TemplateMiddle;
    }

    bool isTemplateTail(Node &node) {
        return node.kind == SyntaxKind::TemplateTail;
    }

    // Punctuation

    bool isDotDotDotToken(Node &node) {
        return node.kind == SyntaxKind::DotDotDotToken;
    }

    /*@internal*/
    bool isCommaToken(Node &node) {
        return node.kind == SyntaxKind::CommaToken;
    }

    bool isPlusToken(Node &node) {
        return node.kind == SyntaxKind::PlusToken;
    }

    bool isMinusToken(Node &node) {
        return node.kind == SyntaxKind::MinusToken;
    }

    bool isAsteriskToken(Node &node) {
        return node.kind == SyntaxKind::AsteriskToken;
    }

    /*@internal*/
    bool isExclamationToken(Node &node) {
        return node.kind == SyntaxKind::ExclamationToken;
    }

    /*@internal*/
    bool isQuestionToken(Node &node) {
        return node.kind == SyntaxKind::QuestionToken;
    }

    /*@internal*/
    bool isColonToken(Node &node) {
        return node.kind == SyntaxKind::ColonToken;
    }

    /*@internal*/
    bool isQuestionDotToken(Node &node) {
        return node.kind == SyntaxKind::QuestionDotToken;
    }

    /*@internal*/
    bool isEqualsGreaterThanToken(Node &node) {
        return node.kind == SyntaxKind::EqualsGreaterThanToken;
    }

    // Identifiers

    bool isIdentifier(Node &node) {
        return node.kind == SyntaxKind::Identifier;
    }

    bool isPrivateIdentifier(Node &node) {
        return node.kind == SyntaxKind::PrivateIdentifier;
    }

    // Reserved Words

    /* @internal */
    bool isExportModifier(Node &node) {
        return node.kind == SyntaxKind::ExportKeyword;
    }

    /* @internal */
    bool isAsyncModifier(Node &node) {
        return node.kind == SyntaxKind::AsyncKeyword;
    }

    /* @internal */
    bool isAssertsKeyword(Node &node) {
        return node.kind == SyntaxKind::AssertsKeyword;
    }

    /* @internal */
    bool isAwaitKeyword(Node &node) {
        return node.kind == SyntaxKind::AwaitKeyword;
    }

    /* @internal */
    bool isReadonlyKeyword(Node &node) {
        return node.kind == SyntaxKind::ReadonlyKeyword;
    }

    /* @internal */
    bool isStaticModifier(Node &node) {
        return node.kind == SyntaxKind::StaticKeyword;
    }

    /* @internal */
    bool isAbstractModifier(Node &node) {
        return node.kind == SyntaxKind::AbstractKeyword;
    }

    /*@internal*/
    bool isSuperKeyword(Node &node) {
        return node.kind == SyntaxKind::SuperKeyword;
    }

    /*@internal*/
    bool isImportKeyword(Node &node) {
        return node.kind == SyntaxKind::ImportKeyword;
    }

    // Names

    bool isQualifiedName(Node &node) {
        return node.kind == SyntaxKind::QualifiedName;
    }

    bool isComputedPropertyName(Node &node) {
        return node.kind == SyntaxKind::ComputedPropertyName;
    }

    // Signature elements

    bool isTypeParameterDeclaration(Node &node) {
        return node.kind == SyntaxKind::TypeParameter;
    }

    // TODO(rbuckton): Rename to 'isParameterDeclaration'
    bool isParameter(Node &node) {
        return node.kind == SyntaxKind::Parameter;
    }

    bool isDecorator(Node &node) {
        return node.kind == SyntaxKind::Decorator;
    }

    // TypeMember

    bool isPropertySignature(Node &node) {
        return node.kind == SyntaxKind::PropertySignature;
    }

    bool isPropertyDeclaration(Node &node) {
        return node.kind == SyntaxKind::PropertyDeclaration;
    }

    bool isMethodSignature(Node &node) {
        return node.kind == SyntaxKind::MethodSignature;
    }

    bool isMethodDeclaration(Node &node) {
        return node.kind == SyntaxKind::MethodDeclaration;
    }

    bool isClassStaticBlockDeclaration(Node &node) {
        return node.kind == SyntaxKind::ClassStaticBlockDeclaration;
    }

    bool isConstructorDeclaration(Node &node) {
        return node.kind == SyntaxKind::Constructor;
    }

    bool isGetAccessorDeclaration(Node &node) {
        return node.kind == SyntaxKind::GetAccessor;
    }

    bool isSetAccessorDeclaration(Node &node) {
        return node.kind == SyntaxKind::SetAccessor;
    }

    bool isCallSignatureDeclaration(Node &node) {
        return node.kind == SyntaxKind::CallSignature;
    }

    bool isConstructSignatureDeclaration(Node &node) {
        return node.kind == SyntaxKind::ConstructSignature;
    }

    bool isIndexSignatureDeclaration(Node &node) {
        return node.kind == SyntaxKind::IndexSignature;
    }

    // Type

    bool isTypePredicateNode(Node &node) {
        return node.kind == SyntaxKind::TypePredicate;
    }

    bool isTypeReferenceNode(Node &node) {
        return node.kind == SyntaxKind::TypeReference;
    }

    bool isFunctionTypeNode(Node &node) {
        return node.kind == SyntaxKind::FunctionType;
    }

    bool isConstructorTypeNode(Node &node) {
        return node.kind == SyntaxKind::ConstructorType;
    }

    bool isTypeQueryNode(Node &node) {
        return node.kind == SyntaxKind::TypeQuery;
    }

    bool isTypeLiteralNode(Node &node) {
        return node.kind == SyntaxKind::TypeLiteral;
    }

    bool isArrayTypeNode(Node &node) {
        return node.kind == SyntaxKind::ArrayType;
    }

    bool isTupleTypeNode(Node &node) {
        return node.kind == SyntaxKind::TupleType;
    }

    bool isNamedTupleMember(Node &node) {
        return node.kind == SyntaxKind::NamedTupleMember;
    }

    bool isOptionalTypeNode(Node &node) {
        return node.kind == SyntaxKind::OptionalType;
    }

    bool isRestTypeNode(Node &node) {
        return node.kind == SyntaxKind::RestType;
    }

    bool isUnionTypeNode(Node &node) {
        return node.kind == SyntaxKind::UnionType;
    }

    bool isIntersectionTypeNode(Node &node) {
        return node.kind == SyntaxKind::IntersectionType;
    }

    bool isConditionalTypeNode(Node &node) {
        return node.kind == SyntaxKind::ConditionalType;
    }

    bool isInferTypeNode(Node &node) {
        return node.kind == SyntaxKind::InferType;
    }

    bool isParenthesizedTypeNode(Node &node) {
        return node.kind == SyntaxKind::ParenthesizedType;
    }

    bool isThisTypeNode(Node &node) {
        return node.kind == SyntaxKind::ThisType;
    }

    bool isTypeOperatorNode(Node &node) {
        return node.kind == SyntaxKind::TypeOperator;
    }

    bool isIndexedAccessTypeNode(Node &node) {
        return node.kind == SyntaxKind::IndexedAccessType;
    }

    bool isMappedTypeNode(Node &node) {
        return node.kind == SyntaxKind::MappedType;
    }

    bool isLiteralTypeNode(Node &node) {
        return node.kind == SyntaxKind::LiteralType;
    }

    bool isImportTypeNode(Node &node) {
        return node.kind == SyntaxKind::ImportType;
    }

    bool isTemplateLiteralTypeSpan(Node &node) {
        return node.kind == SyntaxKind::TemplateLiteralTypeSpan;
    }

    bool isTemplateLiteralTypeNode(Node &node) {
        return node.kind == SyntaxKind::TemplateLiteralType;
    }

    // Binding patterns

    bool isObjectBindingPattern(Node &node) {
        return node.kind == SyntaxKind::ObjectBindingPattern;
    }

    bool isArrayBindingPattern(Node &node) {
        return node.kind == SyntaxKind::ArrayBindingPattern;
    }

    bool isBindingElement(Node &node) {
        return node.kind == SyntaxKind::BindingElement;
    }

    // Expression

    bool isArrayLiteralExpression(Node &node) {
        return node.kind == SyntaxKind::ArrayLiteralExpression;
    }

    bool isObjectLiteralExpression(Node &node) {
        return node.kind == SyntaxKind::ObjectLiteralExpression;
    }

    bool isPropertyAccessExpression(Node &node) {
        return node.kind == SyntaxKind::PropertyAccessExpression;
    }

    bool isElementAccessExpression(Node &node) {
        return node.kind == SyntaxKind::ElementAccessExpression;
    }

    bool isCallExpression(Node &node) {
        return node.kind == SyntaxKind::CallExpression;
    }

    bool isNewExpression(Node &node) {
        return node.kind == SyntaxKind::NewExpression;
    }

    bool isTaggedTemplateExpression(Node &node) {
        return node.kind == SyntaxKind::TaggedTemplateExpression;
    }

    bool isTypeAssertionExpression(Node &node) {
        return node.kind == SyntaxKind::TypeAssertionExpression;
    }

    bool isParenthesizedExpression(Node &node) {
        return node.kind == SyntaxKind::ParenthesizedExpression;
    }

    bool isFunctionExpression(Node &node) {
        return node.kind == SyntaxKind::FunctionExpression;
    }

    bool isArrowFunction(Node &node) {
        return node.kind == SyntaxKind::ArrowFunction;
    }

    bool isDeleteExpression(Node &node) {
        return node.kind == SyntaxKind::DeleteExpression;
    }

    bool isTypeOfExpression(Node &node) {
        return node.kind == SyntaxKind::TypeOfExpression;
    }

    bool isVoidExpression(Node &node) {
        return node.kind == SyntaxKind::VoidExpression;
    }

    bool isAwaitExpression(Node &node) {
        return node.kind == SyntaxKind::AwaitExpression;
    }

    bool isPrefixUnaryExpression(Node &node) {
        return node.kind == SyntaxKind::PrefixUnaryExpression;
    }

    bool isPostfixUnaryExpression(Node &node) {
        return node.kind == SyntaxKind::PostfixUnaryExpression;
    }

    bool isBinaryExpression(Node &node) {
        return node.kind == SyntaxKind::BinaryExpression;
    }

    bool isConditionalExpression(Node &node) {
        return node.kind == SyntaxKind::ConditionalExpression;
    }

    bool isTemplateExpression(Node &node) {
        return node.kind == SyntaxKind::TemplateExpression;
    }

    bool isYieldExpression(Node &node) {
        return node.kind == SyntaxKind::YieldExpression;
    }

    bool isSpreadElement(Node &node) {
        return node.kind == SyntaxKind::SpreadElement;
    }

    bool isClassExpression(Node &node) {
        return node.kind == SyntaxKind::ClassExpression;
    }

    bool isOmittedExpression(Node &node) {
        return node.kind == SyntaxKind::OmittedExpression;
    }

    bool isExpressionWithTypeArguments(Node &node) {
        return node.kind == SyntaxKind::ExpressionWithTypeArguments;
    }

    bool isAsExpression(Node &node) {
        return node.kind == SyntaxKind::AsExpression;
    }

    bool isNonNullExpression(Node &node) {
        return node.kind == SyntaxKind::NonNullExpression;
    }

    bool isMetaProperty(Node &node) {
        return node.kind == SyntaxKind::MetaProperty;
    }

    bool isSyntheticExpression(Node &node) {
        return node.kind == SyntaxKind::SyntheticExpression;
    }

    bool isPartiallyEmittedExpression(Node &node) {
        return node.kind == SyntaxKind::PartiallyEmittedExpression;
    }

    bool isCommaListExpression(Node &node) {
        return node.kind == SyntaxKind::CommaListExpression;
    }

    // Misc

    bool isTemplateSpan(Node &node) {
        return node.kind == SyntaxKind::TemplateSpan;
    }

    bool isSemicolonClassElement(Node &node) {
        return node.kind == SyntaxKind::SemicolonClassElement;
    }

    // Elements

    bool isBlock(Node &node) {
        return node.kind == SyntaxKind::Block;
    }

    bool isVariableStatement(Node &node) {
        return node.kind == SyntaxKind::VariableStatement;
    }

    bool isEmptyStatement(Node &node) {
        return node.kind == SyntaxKind::EmptyStatement;
    }

    bool isExpressionStatement(Node &node) {
        return node.kind == SyntaxKind::ExpressionStatement;
    }

    bool isIfStatement(Node &node) {
        return node.kind == SyntaxKind::IfStatement;
    }

    bool isDoStatement(Node &node) {
        return node.kind == SyntaxKind::DoStatement;
    }

    bool isWhileStatement(Node &node) {
        return node.kind == SyntaxKind::WhileStatement;
    }

    bool isForStatement(Node &node) {
        return node.kind == SyntaxKind::ForStatement;
    }

    bool isForInStatement(Node &node) {
        return node.kind == SyntaxKind::ForInStatement;
    }

    bool isForOfStatement(Node &node) {
        return node.kind == SyntaxKind::ForOfStatement;
    }

    bool isContinueStatement(Node &node) {
        return node.kind == SyntaxKind::ContinueStatement;
    }

    bool isBreakStatement(Node &node) {
        return node.kind == SyntaxKind::BreakStatement;
    }

    bool isReturnStatement(Node &node) {
        return node.kind == SyntaxKind::ReturnStatement;
    }

    bool isWithStatement(Node &node) {
        return node.kind == SyntaxKind::WithStatement;
    }

    bool isSwitchStatement(Node &node) {
        return node.kind == SyntaxKind::SwitchStatement;
    }

    bool isLabeledStatement(Node &node) {
        return node.kind == SyntaxKind::LabeledStatement;
    }

    bool isThrowStatement(Node &node) {
        return node.kind == SyntaxKind::ThrowStatement;
    }

    bool isTryStatement(Node &node) {
        return node.kind == SyntaxKind::TryStatement;
    }

    bool isDebuggerStatement(Node &node) {
        return node.kind == SyntaxKind::DebuggerStatement;
    }

    bool isVariableDeclaration(Node &node) {
        return node.kind == SyntaxKind::VariableDeclaration;
    }

    bool isVariableDeclarationList(Node &node) {
        return node.kind == SyntaxKind::VariableDeclarationList;
    }

    bool isFunctionDeclaration(Node &node) {
        return node.kind == SyntaxKind::FunctionDeclaration;
    }

    bool isClassDeclaration(Node &node) {
        return node.kind == SyntaxKind::ClassDeclaration;
    }

    bool isInterfaceDeclaration(Node &node) {
        return node.kind == SyntaxKind::InterfaceDeclaration;
    }

    bool isTypeAliasDeclaration(Node &node) {
        return node.kind == SyntaxKind::TypeAliasDeclaration;
    }

    bool isEnumDeclaration(Node &node) {
        return node.kind == SyntaxKind::EnumDeclaration;
    }

    bool isModuleDeclaration(Node &node) {
        return node.kind == SyntaxKind::ModuleDeclaration;
    }

    bool isModuleBlock(Node &node) {
        return node.kind == SyntaxKind::ModuleBlock;
    }

    bool isCaseBlock(Node &node) {
        return node.kind == SyntaxKind::CaseBlock;
    }

    bool isNamespaceExportDeclaration(Node &node) {
        return node.kind == SyntaxKind::NamespaceExportDeclaration;
    }

    bool isImportEqualsDeclaration(Node &node) {
        return node.kind == SyntaxKind::ImportEqualsDeclaration;
    }

    bool isImportDeclaration(Node &node) {
        return node.kind == SyntaxKind::ImportDeclaration;
    }

    bool isImportClause(Node &node) {
        return node.kind == SyntaxKind::ImportClause;
    }

    bool isAssertClause(Node &node) {
        return node.kind == SyntaxKind::AssertClause;
    }

    bool isAssertEntry(Node &node) {
        return node.kind == SyntaxKind::AssertEntry;
    }

    bool isNamespaceImport(Node &node) {
        return node.kind == SyntaxKind::NamespaceImport;
    }

    bool isNamespaceExport(Node &node) {
        return node.kind == SyntaxKind::NamespaceExport;
    }

    bool isNamedImports(Node &node) {
        return node.kind == SyntaxKind::NamedImports;
    }

    bool isImportSpecifier(Node &node) {
        return node.kind == SyntaxKind::ImportSpecifier;
    }

    bool isExportAssignment(Node &node) {
        return node.kind == SyntaxKind::ExportAssignment;
    }

    bool isExportDeclaration(Node &node) {
        return node.kind == SyntaxKind::ExportDeclaration;
    }

    bool isNamedExports(Node &node) {
        return node.kind == SyntaxKind::NamedExports;
    }

    bool isExportSpecifier(Node &node) {
        return node.kind == SyntaxKind::ExportSpecifier;
    }

    bool isMissingDeclaration(Node &node) {
        return node.kind == SyntaxKind::MissingDeclaration;
    }

    bool isNotEmittedStatement(Node &node) {
        return node.kind == SyntaxKind::NotEmittedStatement;
    }

    /* @internal */
    bool isSyntheticReference(Node &node) {
        return node.kind == SyntaxKind::SyntheticReferenceExpression;
    }

    /* @internal */
    bool isMergeDeclarationMarker(Node &node) {
        return node.kind == SyntaxKind::MergeDeclarationMarker;
    }

    /* @internal */
    bool isEndOfDeclarationMarker(Node &node) {
        return node.kind == SyntaxKind::EndOfDeclarationMarker;
    }

    // Module References

    bool isExternalModuleReference(Node &node) {
        return node.kind == SyntaxKind::ExternalModuleReference;
    }

    bool isExternalModuleReference(BaseUnion &node) {
        return node.kind() == SyntaxKind::ExternalModuleReference;
    }

    // JSX

    bool isJsxElement(Node &node) {
        return node.kind == SyntaxKind::JsxElement;
    }

    bool isJsxSelfClosingElement(Node &node) {
        return node.kind == SyntaxKind::JsxSelfClosingElement;
    }

    bool isJsxOpeningElement(Node &node) {
        return node.kind == SyntaxKind::JsxOpeningElement;
    }

    bool isJsxClosingElement(Node &node) {
        return node.kind == SyntaxKind::JsxClosingElement;
    }

    bool isJsxFragment(Node &node) {
        return node.kind == SyntaxKind::JsxFragment;
    }

    bool isJsxOpeningFragment(Node &node) {
        return node.kind == SyntaxKind::JsxOpeningFragment;
    }

    bool isJsxClosingFragment(Node &node) {
        return node.kind == SyntaxKind::JsxClosingFragment;
    }

    bool isJsxAttribute(Node &node) {
        return node.kind == SyntaxKind::JsxAttribute;
    }

    bool isJsxAttributes(Node &node) {
        return node.kind == SyntaxKind::JsxAttributes;
    }

    bool isJsxSpreadAttribute(Node &node) {
        return node.kind == SyntaxKind::JsxSpreadAttribute;
    }

    bool isJsxExpression(Node &node) {
        return node.kind == SyntaxKind::JsxExpression;
    }

    // Clauses

    bool isCaseClause(Node &node) {
        return node.kind == SyntaxKind::CaseClause;
    }

    bool isDefaultClause(Node &node) {
        return node.kind == SyntaxKind::DefaultClause;
    }

    bool isHeritageClause(Node &node) {
        return node.kind == SyntaxKind::HeritageClause;
    }

    bool isCatchClause(Node &node) {
        return node.kind == SyntaxKind::CatchClause;
    }

    // Property assignments

    bool isPropertyAssignment(Node &node) {
        return node.kind == SyntaxKind::PropertyAssignment;
    }

    bool isShorthandPropertyAssignment(Node &node) {
        return node.kind == SyntaxKind::ShorthandPropertyAssignment;
    }

    bool isSpreadAssignment(Node &node) {
        return node.kind == SyntaxKind::SpreadAssignment;
    }

    // Enum

    bool isEnumMember(Node &node) {
        return node.kind == SyntaxKind::EnumMember;
    }

    // Unparsed

    // TODO(rbuckton): isUnparsedPrologue

    bool isUnparsedPrepend(Node &node) {
        return node.kind == SyntaxKind::UnparsedPrepend;
    }

    // TODO(rbuckton): isUnparsedText
    // TODO(rbuckton): isUnparsedInternalText
    // TODO(rbuckton): isUnparsedSyntheticReference

    // Top-level nodes
    bool isSourceFile(Node &node) {
        return node.kind == SyntaxKind::SourceFile;
    }

    bool isBundle(Node &node) {
        return node.kind == SyntaxKind::Bundle;
    }

    bool isUnparsedSource(Node &node) {
        return node.kind == SyntaxKind::UnparsedSource;
    }

    // TODO(rbuckton): isInputFiles

    // JSDoc Elements

    bool isJSDocTypeExpression(Node &node) {
        return node.kind == SyntaxKind::JSDocTypeExpression;
    }

    bool isJSDocNameReference(Node &node) {
        return node.kind == SyntaxKind::JSDocNameReference;
    }

    bool isJSDocMemberName(Node &node) {
        return node.kind == SyntaxKind::JSDocMemberName;
    }

    bool isJSDocLink(Node &node) {
        return node.kind == SyntaxKind::JSDocLink;
    }

    bool isJSDocLinkCode(Node &node) {
        return node.kind == SyntaxKind::JSDocLinkCode;
    }

    bool isJSDocLinkPlain(Node &node) {
        return node.kind == SyntaxKind::JSDocLinkPlain;
    }

    bool isJSDocAllType(Node &node) {
        return node.kind == SyntaxKind::JSDocAllType;
    }

    bool isJSDocUnknownType(Node &node) {
        return node.kind == SyntaxKind::JSDocUnknownType;
    }

    bool isJSDocNullableType(Node &node) {
        return node.kind == SyntaxKind::JSDocNullableType;
    }

    bool isJSDocNonNullableType(Node &node) {
        return node.kind == SyntaxKind::JSDocNonNullableType;
    }

    bool isJSDocOptionalType(Node &node) {
        return node.kind == SyntaxKind::JSDocOptionalType;
    }

    bool isJSDocFunctionType(Node &node) {
        return node.kind == SyntaxKind::JSDocFunctionType;
    }

    bool isJSDocVariadicType(Node &node) {
        return node.kind == SyntaxKind::JSDocVariadicType;
    }

    bool isJSDocNamepathType(Node &node) {
        return node.kind == SyntaxKind::JSDocNamepathType;
    }

    bool isJSDoc(Node &node) {
        return node.kind == SyntaxKind::JSDoc;
    }

    bool isJSDocTypeLiteral(Node &node) {
        return node.kind == SyntaxKind::JSDocTypeLiteral;
    }

    bool isJSDocSignature(Node &node) {
        return node.kind == SyntaxKind::JSDocSignature;
    }

    // JSDoc Tags

    bool isJSDocAugmentsTag(Node &node) {
        return node.kind == SyntaxKind::JSDocAugmentsTag;
    }

    bool isJSDocAuthorTag(Node &node) {
        return node.kind == SyntaxKind::JSDocAuthorTag;
    }

    bool isJSDocClassTag(Node &node) {
        return node.kind == SyntaxKind::JSDocClassTag;
    }

    bool isJSDocCallbackTag(Node &node) {
        return node.kind == SyntaxKind::JSDocCallbackTag;
    }

    bool isJSDocPublicTag(Node &node) {
        return node.kind == SyntaxKind::JSDocPublicTag;
    }

    bool isJSDocPrivateTag(Node &node) {
        return node.kind == SyntaxKind::JSDocPrivateTag;
    }

    bool isJSDocProtectedTag(Node &node) {
        return node.kind == SyntaxKind::JSDocProtectedTag;
    }

    bool isJSDocReadonlyTag(Node &node) {
        return node.kind == SyntaxKind::JSDocReadonlyTag;
    }

    bool isJSDocOverrideTag(Node &node) {
        return node.kind == SyntaxKind::JSDocOverrideTag;
    }

    bool isJSDocDeprecatedTag(Node &node) {
        return node.kind == SyntaxKind::JSDocDeprecatedTag;
    }

    bool isJSDocSeeTag(Node &node) {
        return node.kind == SyntaxKind::JSDocSeeTag;
    }

    bool isJSDocEnumTag(Node &node) {
        return node.kind == SyntaxKind::JSDocEnumTag;
    }

    bool isJSDocParameterTag(Node &node) {
        return node.kind == SyntaxKind::JSDocParameterTag;
    }

    bool isJSDocReturnTag(Node &node) {
        return node.kind == SyntaxKind::JSDocReturnTag;
    }

    bool isJSDocThisTag(Node &node) {
        return node.kind == SyntaxKind::JSDocThisTag;
    }

    bool isJSDocTypeTag(Node &node) {
        return node.kind == SyntaxKind::JSDocTypeTag;
    }

    bool isJSDocTemplateTag(Node &node) {
        return node.kind == SyntaxKind::JSDocTemplateTag;
    }

    bool isJSDocTypedefTag(Node &node) {
        return node.kind == SyntaxKind::JSDocTypedefTag;
    }

    bool isJSDocUnknownTag(Node &node) {
        return node.kind == SyntaxKind::JSDocTag;
    }

    bool isJSDocPropertyTag(Node &node) {
        return node.kind == SyntaxKind::JSDocPropertyTag;
    }

    bool isJSDocImplementsTag(Node &node) {
        return node.kind == SyntaxKind::JSDocImplementsTag;
    }

    // Synthesized list

    /* @internal */
    bool isSyntaxList(Node &node){
        return node.kind == SyntaxKind::SyntaxList;
    }
}
