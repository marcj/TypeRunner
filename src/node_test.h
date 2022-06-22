#pragma once

#include "types.h"

namespace ts {
    // Literals

    using types::ModifierFlags;
    using types::NodeFlags;
    using types::EmitFlags;

    /**
     * Gets flags that control emit behavior of a node.
     */
    int getEmitFlags(const shared<Node> &node);

    bool isCommaSequence(shared<Node> node);

    bool isAssignmentOperator(SyntaxKind token);

    /**
     * Gets whether an identifier should only be referred to by its local name.
     */
    bool isLocalName(const shared<Node> &node);

    bool hasModifier(const shared<Node> &node, SyntaxKind kind);

    ModifierFlags modifierToFlag(SyntaxKind token);

    int modifiersToFlags(sharedOpt<NodeArray> modifiers);

    /**
     * Gets the ModifierFlags for syntactic modifiers on the provided node. The modifier flags cache on the node is ignored.
     *
     * NOTE: This function does not use `parent` pointers and will not include modifiers from JSDoc.
     */
    int getSyntacticModifierFlagsNoCache(shared<Node> node);

    int getModifierFlagsWorker(shared<Node> node, bool includeJSDoc, optional<bool> alwaysIncludeJSDoc = {});

    int getSyntacticModifierFlags(shared<Node> node);

    int getSelectedSyntacticModifierFlags(shared<Node> node, int flags);

    bool hasSyntacticModifier(shared<Node> node, int flags);

    bool hasStaticModifier(shared<Node> node);

    shared<NodeUnion(PropertyName)> resolveNameToNode(const shared<Node> &node);

    bool isFunctionOrConstructorTypeNode(shared<Node> node);

    sharedOpt<NodeArray> getTypeParameters(shared<Node> node);

    shared<NodeUnion(JsxTagNameExpression)> getTagName(shared<NodeUnion(JsxOpeningElement, JsxOpeningFragment)> node);

    string &getEscapedName(const shared<Node> &node);

    sharedOpt<NodeUnion(PropertyName)> getName(const shared<Node> &node);

    bool isNumericLiteral(const shared<Node> &node);

    bool isBigIntLiteral(const shared<Node> &node);

    bool isStringLiteral(const shared<Node> &node);

    bool isJsxText(const shared<Node> &node);

    bool isRegularExpressionLiteral(const shared<Node> &node);

    bool isNoSubstitutionTemplateLiteral(const shared<Node> &node);

    bool isStringLiteralLike(shared<Node> node);

    bool isStringOrNumericLiteralLike(shared<Node> node);

    // Pseudo-literals

    bool isTemplateHead(const shared<Node> &node);

    bool isTemplateMiddle(const shared<Node> &node);

    bool isTemplateTail(const shared<Node> &node);

    // Punctuation

    bool isDotDotDotToken(const shared<Node> &node);

    /*@internal*/
    bool isCommaToken(const shared<Node> &node);

    bool isPlusToken(const shared<Node> &node);

    bool isMinusToken(const shared<Node> &node);

    bool isAsteriskToken(const shared<Node> &node);

    /*@internal*/
    bool isExclamationToken(const shared<Node> &node);

    /*@internal*/
    bool isQuestionToken(const shared<Node> &node);

    /*@internal*/
    bool isColonToken(const shared<Node> &node);

    /*@internal*/
    bool isQuestionDotToken(const shared<Node> &node);

    /*@internal*/
    bool isEqualsGreaterThanToken(const shared<Node> &node);

    // Identifiers

    bool isIdentifier(const shared<Node> &node);

    bool isPrivateIdentifier(const shared<Node> &node);

    bool identifierIsThisKeyword(const shared<Identifier> &id);

    bool isThisIdentifier(sharedOpt<Node> node);

    /**
     * Determines whether a node is a property or element access expression for `super`.
     */
    bool isSuperProperty(shared<Node> node);

    // Reserved Words

    /* @internal */
    bool isExportModifier(const shared<Node> &node);

    /* @internal */
    bool isAsyncModifier(const shared<Node> &node);

    /* @internal */
    bool isAssertsKeyword(const shared<Node> &node);

    /* @internal */
    bool isAwaitKeyword(const shared<Node> &node);

    /* @internal */
    bool isReadonlyKeyword(const shared<Node> &node);

    /* @internal */
    bool isStaticModifier(const shared<Node> &node);

    /* @internal */
    bool isAbstractModifier(const shared<Node> &node);

    /*@internal*/
    bool isSuperKeyword(const shared<Node> &node);

    /*@internal*/
    bool isImportKeyword(const shared<Node> &node);

    // Names

    bool isQualifiedName(const shared<Node> &node);

    bool isComputedPropertyName(const shared<Node> &node);

    // Signature elements

    bool isTypeParameterDeclaration(const shared<Node> &node);

    // TODO(rbuckton): Rename to 'isParameterDeclaration'
    bool isParameter(const shared<Node> &node);

    bool isDecorator(const shared<Node> &node);

    // TypeMember

    bool isPropertySignature(const shared<Node> &node);

    bool isPropertyDeclaration(const shared<Node> &node);

    bool isMethodSignature(const shared<Node> &node);

    bool isMethodDeclaration(const shared<Node> &node);

    bool isClassStaticBlockDeclaration(const shared<Node> &node);

    bool isConstructorDeclaration(const shared<Node> &node);

    bool isGetAccessorDeclaration(const shared<Node> &node);

    bool isSetAccessorDeclaration(const shared<Node> &node);

    bool isCallSignatureDeclaration(const shared<Node> &node);

    bool isConstructSignatureDeclaration(const shared<Node> &node);

    bool isIndexSignatureDeclaration(const shared<Node> &node);

    // Type

    bool isTypePredicateNode(const shared<Node> &node);

    bool isTypeReferenceNode(const shared<Node> &node);

    bool isFunctionTypeNode(const shared<Node> &node);

    bool isConstructorTypeNode(const shared<Node> &node);

    bool isTypeQueryNode(const shared<Node> &node);

    bool isTypeLiteralNode(const shared<Node> &node);

    bool isArrayTypeNode(const shared<Node> &node);

    bool isTupleTypeNode(const shared<Node> &node);

    bool isNamedTupleMember(const shared<Node> &node);

    bool isOptionalTypeNode(const shared<Node> &node);

    bool isRestTypeNode(const shared<Node> &node);

    bool isUnionTypeNode(const shared<Node> &node);

    bool isIntersectionTypeNode(const shared<Node> &node);

    bool isConditionalTypeNode(const shared<Node> &node);

    bool isInferTypeNode(const shared<Node> &node);

    bool isParenthesizedTypeNode(const shared<Node> &node);

    bool isThisTypeNode(const shared<Node> &node);

    bool isTypeOperatorNode(const shared<Node> &node);

    bool isIndexedAccessTypeNode(const shared<Node> &node);

    bool isMappedTypeNode(const shared<Node> &node);

    bool isLiteralTypeNode(const shared<Node> &node);

    bool isImportTypeNode(const shared<Node> &node);

    bool isTemplateLiteralTypeSpan(const shared<Node> &node);

    bool isTemplateLiteralTypeNode(const shared<Node> &node);

    // Binding patterns

    bool isObjectBindingPattern(const shared<Node> &node);

    bool isArrayBindingPattern(const shared<Node> &node);

    bool isBindingElement(const shared<Node> &node);

    // Expression

    bool isArrayLiteralExpression(const shared<Node> &node);

    bool isObjectLiteralExpression(const shared<Node> &node);

    bool isPropertyAccessExpression(const shared<Node> &node);

    bool isElementAccessExpression(const shared<Node> &node);

    bool isCallExpression(const shared<Node> &node);

    bool isCallChain(shared<Node> node);

    bool isNewExpression(const shared<Node> &node);

    bool isTaggedTemplateExpression(const shared<Node> &node);

    bool isTypeAssertionExpression(const shared<Node> &node);

    bool isParenthesizedExpression(const shared<Node> &node);

    bool isFunctionExpression(const shared<Node> &node);

    bool isArrowFunction(const shared<Node> &node);

    bool isDeleteExpression(const shared<Node> &node);

    bool isTypeOfExpression(const shared<Node> &node);

    bool isVoidExpression(const shared<Node> &node);

    bool isAwaitExpression(const shared<Node> &node);

    bool isPrefixUnaryExpression(const shared<Node> &node);

    bool isPostfixUnaryExpression(const shared<Node> &node);

    bool isBinaryExpression(const shared<Node> &node);

    bool isConditionalExpression(const shared<Node> &node);

    bool isTemplateExpression(const shared<Node> &node);

    bool isYieldExpression(const shared<Node> &node);

    bool isSpreadElement(const shared<Node> &node);

    bool isClassExpression(const shared<Node> &node);

    bool isOmittedExpression(const shared<Node> &node);

    bool isExpressionWithTypeArguments(const shared<Node> &node);

    bool isAsExpression(const shared<Node> &node);

    bool isNonNullExpression(const shared<Node> &node);

    bool isNonNullChain(shared<Node> node);

    bool isMetaProperty(const shared<Node> &node);

    bool isSyntheticExpression(const shared<Node> &node);

    bool isPartiallyEmittedExpression(const shared<Node> &node);

    bool isCommaListExpression(const shared<Node> &node);

    // Misc

    bool isTemplateSpan(const shared<Node> &node);

    bool isSemicolonClassElement(const shared<Node> &node);

    // Elements

    bool isBlock(const shared<Node> &node);

    bool isVariableStatement(const shared<Node> &node);

    bool isEmptyStatement(const shared<Node> &node);

    bool isExpressionStatement(const shared<Node> &node);

    bool isIfStatement(const shared<Node> &node);

    bool isDoStatement(const shared<Node> &node);

    bool isWhileStatement(const shared<Node> &node);

    bool isForStatement(const shared<Node> &node);

    bool isForInStatement(const shared<Node> &node);

    bool isForOfStatement(const shared<Node> &node);

    bool isContinueStatement(const shared<Node> &node);

    bool isBreakStatement(const shared<Node> &node);

    bool isReturnStatement(const shared<Node> &node);

    bool isWithStatement(const shared<Node> &node);

    bool isSwitchStatement(const shared<Node> &node);

    bool isLabeledStatement(const shared<Node> &node);

    bool isThrowStatement(const shared<Node> &node);

    bool isTryStatement(const shared<Node> &node);

    bool isDebuggerStatement(const shared<Node> &node);

    bool isVariableDeclaration(const shared<Node> &node);

    bool isVariableDeclarationList(const shared<Node> &node);

    bool isFunctionDeclaration(const shared<Node> &node);

    bool isClassDeclaration(const shared<Node> &node);

    bool isInterfaceDeclaration(const shared<Node> &node);

    bool isTypeAliasDeclaration(const shared<Node> &node);

    bool isEnumDeclaration(const shared<Node> &node);

    bool isModuleDeclaration(const shared<Node> &node);

    bool isModuleBlock(const shared<Node> &node);

    bool isCaseBlock(const shared<Node> &node);

    bool isNamespaceExportDeclaration(const shared<Node> &node);

    bool isImportEqualsDeclaration(const shared<Node> &node);

    bool isImportDeclaration(const shared<Node> &node);

    bool isImportClause(const shared<Node> &node);

    bool isAssertClause(const shared<Node> &node);

    bool isAssertEntry(const shared<Node> &node);

    bool isNamespaceImport(const shared<Node> &node);

    bool isNamespaceExport(const shared<Node> &node);

    bool isNamedImports(const shared<Node> &node);

    bool isImportSpecifier(const shared<Node> &node);

    bool isExportAssignment(const shared<Node> &node);

    bool isExportDeclaration(const shared<Node> &node);

    bool isNamedExports(const shared<Node> &node);

    bool isExportSpecifier(const shared<Node> &node);

    bool isMissingDeclaration(const shared<Node> &node);

    bool isNotEmittedStatement(const shared<Node> &node);

    /* @internal */
    bool isSyntheticReference(const shared<Node> &node);

    /* @internal */
    bool isMergeDeclarationMarker(const shared<Node> &node);

    /* @internal */
    bool isEndOfDeclarationMarker(const shared<Node> &node);

    // Module References

    bool isExternalModuleReference(const shared<Node> &node);

    // JSX

    bool isJsxElement(const shared<Node> &node);

    bool isJsxSelfClosingElement(const shared<Node> &node);

    bool isJsxOpeningElement(const shared<Node> &node);

    bool isJsxClosingElement(const shared<Node> &node);

    bool isJsxFragment(const shared<Node> &node);

    bool isJsxOpeningFragment(const shared<Node> &node);

    bool isJsxClosingFragment(const shared<Node> &node);

    bool isJsxAttribute(const shared<Node> &node);

    bool isJsxAttributes(const shared<Node> &node);

    bool isJsxSpreadAttribute(const shared<Node> &node);

    bool isJsxExpression(const shared<Node> &node);

    // Clauses

    bool isCaseClause(const shared<Node> &node);

    bool isDefaultClause(const shared<Node> &node);

    bool isHeritageClause(const shared<Node> &node);
    bool isCatchClause(const shared<Node> &node);

    // Property assignments

    bool isPropertyAssignment(const shared<Node> &node);

    bool isShorthandPropertyAssignment(const shared<Node> &node);

    bool isSpreadAssignment(const shared<Node> &node);

    // Enum

    bool isEnumMember(const shared<Node> &node);

    // Unparsed

    // TODO(rbuckton): isUnparsedPrologue

    bool isUnparsedPrepend(const shared<Node> &node);
    // TODO(rbuckton): isUnparsedText
    // TODO(rbuckton): isUnparsedInternalText
    // TODO(rbuckton): isUnparsedSyntheticReference

    // Top-level nodes
    bool isSourceFile(const shared<Node> &node);

    bool isBundle(const shared<Node> &node);

    bool isUnparsedSource(const shared<Node> &node);

    // TODO(rbuckton): isInputFiles

    // JSDoc Elements

    bool isJSDocTypeExpression(const shared<Node> &node);

    bool isJSDocNameReference(const shared<Node> &node);

    bool isJSDocMemberName(const shared<Node> &node);

    bool isJSDocLink(const shared<Node> &node);

    bool isJSDocLinkCode(const shared<Node> &node);

    bool isJSDocLinkPlain(const shared<Node> &node);

    bool isJSDocAllType(const shared<Node> &node);

    bool isJSDocUnknownType(const shared<Node> &node);

    bool isJSDocNullableType(const shared<Node> &node);

    bool isJSDocNonNullableType(const shared<Node> &node);

    bool isJSDocOptionalType(const shared<Node> &node);

    bool isJSDocFunctionType(const shared<Node> &node);

    bool isJSDocVariadicType(const shared<Node> &node);

    bool isJSDocNamepathType(const shared<Node> &node);

    bool isJSDoc(const shared<Node> &node);

    bool isJSDocTypeLiteral(const shared<Node> &node);

    bool isJSDocSignature(const shared<Node> &node);

    // JSDoc Tags

    bool isJSDocAugmentsTag(const shared<Node> &node);

    bool isJSDocAuthorTag(const shared<Node> &node);

    bool isJSDocClassTag(const shared<Node> &node);

    bool isJSDocCallbackTag(const shared<Node> &node);

    bool isJSDocPublicTag(const shared<Node> &node);

    bool isJSDocPrivateTag(const shared<Node> &node);

    bool isJSDocProtectedTag(const shared<Node> &node);
    bool isJSDocReadonlyTag(const shared<Node> &node);

    bool isJSDocOverrideTag(const shared<Node> &node);

    bool isJSDocDeprecatedTag(const shared<Node> &node);

    bool isJSDocSeeTag(const shared<Node> &node);

    bool isJSDocEnumTag(const shared<Node> &node);

    bool isJSDocParameterTag(const shared<Node> &node);

    bool isJSDocReturnTag(const shared<Node> &node);

    bool isJSDocThisTag(const shared<Node> &node);

    bool isJSDocTypeTag(const shared<Node> &node);

    bool isJSDocTemplateTag(const shared<Node> &node);
    bool isJSDocTypedefTag(const shared<Node> &node);

    bool isJSDocUnknownTag(const shared<Node> &node);

    bool isJSDocPropertyTag(const shared<Node> &node);

    bool isJSDocImplementsTag(const shared<Node> &node);

    // Synthesized list

    /* @internal */
    bool isSyntaxList(const shared<Node> &node);
}
