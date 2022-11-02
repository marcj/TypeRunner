#pragma once

#include "types.h"

namespace tr {
    // Literals

    using types::ModifierFlags;
    using types::NodeFlags;
    using types::EmitFlags;

    /**
     * Gets flags that control emit behavior of a node.
     */
    int getEmitFlags(const node<Node> &node);

    bool isCommaSequence(node<Node> node);

    bool isAssignmentOperator(SyntaxKind token);

    /**
     * Gets whether an identifier should only be referred to by its local name.
     */
    bool isLocalName(const node<Node> &node);

    bool hasModifier(const node<Node> &node, SyntaxKind kind);

    ModifierFlags modifierToFlag(SyntaxKind token);

    int modifiersToFlags(optionalNode<NodeArray> modifiers);

    /**
     * Gets the ModifierFlags for syntactic modifiers on the provided node. The modifier flags cache on the node is ignored.
     *
     * NOTE: This function does not use `parent` pointers and will not include modifiers from JSDoc.
     */
    int getSyntacticModifierFlagsNoCache(node<Node> node);

    int getModifierFlagsWorker(node<Node> node, bool includeJSDoc, optional<bool> alwaysIncludeJSDoc = {});

    int getSyntacticModifierFlags(node<Node> node);

    int getSelectedSyntacticModifierFlags(node<Node> node, int flags);

    bool hasSyntacticModifier(node<Node> node, int flags);

    bool hasStaticModifier(node<Node> node);

    node<NodeUnion(PropertyName)> resolveNameToNode(const node<Node> &node);

    bool isFunctionOrConstructorTypeNode(node<Node> node);

    optionalNode<NodeArray> getTypeParameters(node<Node> node);

    node<NodeUnion(JsxTagNameExpression)> getTagName(node<NodeUnion(JsxOpeningElement, JsxOpeningFragment)> node);

    string &getEscapedName(const node<Node> &node);

    optionalNode<NodeUnion(PropertyName)> getName(const node<Node> &node);

    bool isNumericLiteral(const node<Node> &node);

    bool isBigIntLiteral(const node<Node> &node);

    bool isStringLiteral(const node<Node> &node);

    bool isJsxText(const node<Node> &node);

    bool isRegularExpressionLiteral(const node<Node> &node);

    bool isNoSubstitutionTemplateLiteral(const node<Node> &node);

    bool isStringLiteralLike(node<Node> node);

    bool isStringOrNumericLiteralLike(node<Node> node);

    // Pseudo-literals

    bool isTemplateHead(const node<Node> &node);

    bool isTemplateMiddle(const node<Node> &node);

    bool isTemplateTail(const node<Node> &node);

    // Punctuation

    bool isDotDotDotToken(const node<Node> &node);

    /*@internal*/
    bool isCommaToken(const node<Node> &node);

    bool isPlusToken(const node<Node> &node);

    bool isMinusToken(const node<Node> &node);

    bool isAsteriskToken(const node<Node> &node);

    /*@internal*/
    bool isExclamationToken(const node<Node> &node);

    /*@internal*/
    bool isQuestionToken(const node<Node> &node);

    /*@internal*/
    bool isColonToken(const node<Node> &node);

    /*@internal*/
    bool isQuestionDotToken(const node<Node> &node);

    /*@internal*/
    bool isEqualsGreaterThanToken(const node<Node> &node);

    // Identifiers

    bool isIdentifier(const node<Node> &node);

    bool isPrivateIdentifier(const node<Node> &node);

    bool identifierIsThisKeyword(const node<Identifier> &id);

    bool isThisIdentifier(optionalNode<Node> node);

    /**
     * Determines whether a node is a property or element access expression for `super`.
     */
    bool isSuperProperty(node<Node> node);

    // Reserved Words

    /* @internal */
    bool isExportModifier(const node<Node> &node);

    /* @internal */
    bool isAsyncModifier(const node<Node> &node);

    /* @internal */
    bool isAssertsKeyword(const node<Node> &node);

    /* @internal */
    bool isAwaitKeyword(const node<Node> &node);

    /* @internal */
    bool isReadonlyKeyword(const node<Node> &node);

    /* @internal */
    bool isStaticModifier(const node<Node> &node);

    /* @internal */
    bool isAbstractModifier(const node<Node> &node);

    /*@internal*/
    bool isSuperKeyword(const node<Node> &node);

    /*@internal*/
    bool isImportKeyword(const node<Node> &node);

    // Names

    bool isQualifiedName(const node<Node> &node);

    bool isComputedPropertyName(const node<Node> &node);

    // Signature elements

    bool isTypeParameterDeclaration(const node<Node> &node);

    // TODO(rbuckton): Rename to 'isParameterDeclaration'
    bool isParameter(const node<Node> &node);

    bool isDecorator(const node<Node> &node);

    // TypeMember

    bool isPropertySignature(const node<Node> &node);

    bool isPropertyDeclaration(const node<Node> &node);

    bool isMethodSignature(const node<Node> &node);

    bool isMethodDeclaration(const node<Node> &node);

    bool isClassStaticBlockDeclaration(const node<Node> &node);

    bool isConstructorDeclaration(const node<Node> &node);

    bool isGetAccessorDeclaration(const node<Node> &node);

    bool isSetAccessorDeclaration(const node<Node> &node);

    bool isCallSignatureDeclaration(const node<Node> &node);

    bool isConstructSignatureDeclaration(const node<Node> &node);

    bool isIndexSignatureDeclaration(const node<Node> &node);

    // Type

    bool isTypePredicateNode(const node<Node> &node);

    bool isTypeReferenceNode(const node<Node> &node);

    bool isFunctionTypeNode(const node<Node> &node);

    bool isConstructorTypeNode(const node<Node> &node);

    bool isTypeQueryNode(const node<Node> &node);

    bool isTypeLiteralNode(const node<Node> &node);

    bool isArrayTypeNode(const node<Node> &node);

    bool isTupleTypeNode(const node<Node> &node);

    bool isNamedTupleMember(const node<Node> &node);

    bool isOptionalTypeNode(const node<Node> &node);

    bool isRestTypeNode(const node<Node> &node);

    bool isUnionTypeNode(const node<Node> &node);

    bool isIntersectionTypeNode(const node<Node> &node);

    bool isConditionalTypeNode(const node<Node> &node);

    bool isInferTypeNode(const node<Node> &node);

    bool isParenthesizedTypeNode(const node<Node> &node);

    bool isThisTypeNode(const node<Node> &node);

    bool isTypeOperatorNode(const node<Node> &node);

    bool isIndexedAccessTypeNode(const node<Node> &node);

    bool isMappedTypeNode(const node<Node> &node);

    bool isLiteralTypeNode(const node<Node> &node);

    bool isImportTypeNode(const node<Node> &node);

    bool isTemplateLiteralTypeSpan(const node<Node> &node);

    bool isTemplateLiteralTypeNode(const node<Node> &node);

    // Binding patterns

    bool isObjectBindingPattern(const node<Node> &node);

    bool isArrayBindingPattern(const node<Node> &node);

    bool isBindingElement(const node<Node> &node);

    // Expression

    bool isArrayLiteralExpression(const node<Node> &node);

    bool isObjectLiteralExpression(const node<Node> &node);

    bool isPropertyAccessExpression(const node<Node> &node);

    bool isElementAccessExpression(const node<Node> &node);

    bool isCallExpression(const node<Node> &node);

    bool isCallChain(node<Node> node);

    bool isNewExpression(const node<Node> &node);

    bool isTaggedTemplateExpression(const node<Node> &node);

    bool isTypeAssertionExpression(const node<Node> &node);

    bool isParenthesizedExpression(const node<Node> &node);

    bool isFunctionExpression(const node<Node> &node);

    bool isArrowFunction(const node<Node> &node);

    bool isDeleteExpression(const node<Node> &node);

    bool isTypeOfExpression(const node<Node> &node);

    bool isVoidExpression(const node<Node> &node);

    bool isAwaitExpression(const node<Node> &node);

    bool isPrefixUnaryExpression(const node<Node> &node);

    bool isPostfixUnaryExpression(const node<Node> &node);

    bool isBinaryExpression(const node<Node> &node);

    bool isConditionalExpression(const node<Node> &node);

    bool isTemplateExpression(const node<Node> &node);

    bool isYieldExpression(const node<Node> &node);

    bool isSpreadElement(const node<Node> &node);

    bool isClassExpression(const node<Node> &node);

    bool isOmittedExpression(const node<Node> &node);

    bool isExpressionWithTypeArguments(const node<Node> &node);

    bool isAsExpression(const node<Node> &node);

    bool isNonNullExpression(const node<Node> &node);

    bool isNonNullChain(node<Node> node);

    bool isMetaProperty(const node<Node> &node);

    bool isSyntheticExpression(const node<Node> &node);

    bool isPartiallyEmittedExpression(const node<Node> &node);

    bool isCommaListExpression(const node<Node> &node);

    // Misc

    bool isTemplateSpan(const node<Node> &node);

    bool isSemicolonClassElement(const node<Node> &node);

    // Elements

    bool isBlock(const node<Node> &node);

    bool isVariableStatement(const node<Node> &node);

    bool isEmptyStatement(const node<Node> &node);

    bool isExpressionStatement(const node<Node> &node);

    bool isIfStatement(const node<Node> &node);

    bool isDoStatement(const node<Node> &node);

    bool isWhileStatement(const node<Node> &node);

    bool isForStatement(const node<Node> &node);

    bool isForInStatement(const node<Node> &node);

    bool isForOfStatement(const node<Node> &node);

    bool isContinueStatement(const node<Node> &node);

    bool isBreakStatement(const node<Node> &node);

    bool isReturnStatement(const node<Node> &node);

    bool isWithStatement(const node<Node> &node);

    bool isSwitchStatement(const node<Node> &node);

    bool isLabeledStatement(const node<Node> &node);

    bool isThrowStatement(const node<Node> &node);

    bool isTryStatement(const node<Node> &node);

    bool isDebuggerStatement(const node<Node> &node);

    bool isVariableDeclaration(const node<Node> &node);

    bool isVariableDeclarationList(const node<Node> &node);

    bool isFunctionDeclaration(const node<Node> &node);

    bool isClassDeclaration(const node<Node> &node);

    bool isInterfaceDeclaration(const node<Node> &node);

    bool isTypeAliasDeclaration(const node<Node> &node);

    bool isEnumDeclaration(const node<Node> &node);

    bool isModuleDeclaration(const node<Node> &node);

    bool isModuleBlock(const node<Node> &node);

    bool isCaseBlock(const node<Node> &node);

    bool isNamespaceExportDeclaration(const node<Node> &node);

    bool isImportEqualsDeclaration(const node<Node> &node);

    bool isImportDeclaration(const node<Node> &node);

    bool isImportClause(const node<Node> &node);

    bool isAssertClause(const node<Node> &node);

    bool isAssertEntry(const node<Node> &node);

    bool isNamespaceImport(const node<Node> &node);

    bool isNamespaceExport(const node<Node> &node);

    bool isNamedImports(const node<Node> &node);

    bool isImportSpecifier(const node<Node> &node);

    bool isExportAssignment(const node<Node> &node);

    bool isExportDeclaration(const node<Node> &node);

    bool isNamedExports(const node<Node> &node);

    bool isExportSpecifier(const node<Node> &node);

    bool isMissingDeclaration(const node<Node> &node);

    bool isNotEmittedStatement(const node<Node> &node);

    /* @internal */
    bool isSyntheticReference(const node<Node> &node);

    /* @internal */
    bool isMergeDeclarationMarker(const node<Node> &node);

    /* @internal */
    bool isEndOfDeclarationMarker(const node<Node> &node);

    // Module References

    bool isExternalModuleReference(const node<Node> &node);

    // JSX

    bool isJsxElement(const node<Node> &node);

    bool isJsxSelfClosingElement(const node<Node> &node);

    bool isJsxOpeningElement(const node<Node> &node);

    bool isJsxClosingElement(const node<Node> &node);

    bool isJsxFragment(const node<Node> &node);

    bool isJsxOpeningFragment(const node<Node> &node);

    bool isJsxClosingFragment(const node<Node> &node);

    bool isJsxAttribute(const node<Node> &node);

    bool isJsxAttributes(const node<Node> &node);

    bool isJsxSpreadAttribute(const node<Node> &node);

    bool isJsxExpression(const node<Node> &node);

    // Clauses

    bool isCaseClause(const node<Node> &node);

    bool isDefaultClause(const node<Node> &node);

    bool isHeritageClause(const node<Node> &node);
    bool isCatchClause(const node<Node> &node);

    // Property assignments

    bool isPropertyAssignment(const node<Node> &node);

    bool isShorthandPropertyAssignment(const node<Node> &node);

    bool isSpreadAssignment(const node<Node> &node);

    // Enum

    bool isEnumMember(const node<Node> &node);

    // Unparsed

    // TODO(rbuckton): isUnparsedPrologue

    bool isUnparsedPrepend(const node<Node> &node);
    // TODO(rbuckton): isUnparsedText
    // TODO(rbuckton): isUnparsedInternalText
    // TODO(rbuckton): isUnparsedSyntheticReference

    // Top-level nodes
    bool isSourceFile(const node<Node> &node);

    bool isBundle(const node<Node> &node);

    bool isUnparsedSource(const node<Node> &node);

    // TODO(rbuckton): isInputFiles

    // JSDoc Elements

    bool isJSDocTypeExpression(const node<Node> &node);

    bool isJSDocNameReference(const node<Node> &node);

    bool isJSDocMemberName(const node<Node> &node);

    bool isJSDocLink(const node<Node> &node);

    bool isJSDocLinkCode(const node<Node> &node);

    bool isJSDocLinkPlain(const node<Node> &node);

    bool isJSDocAllType(const node<Node> &node);

    bool isJSDocUnknownType(const node<Node> &node);

    bool isJSDocNullableType(const node<Node> &node);

    bool isJSDocNonNullableType(const node<Node> &node);

    bool isJSDocOptionalType(const node<Node> &node);

    bool isJSDocFunctionType(const node<Node> &node);

    bool isJSDocVariadicType(const node<Node> &node);

    bool isJSDocNamepathType(const node<Node> &node);

    bool isJSDoc(const node<Node> &node);

    bool isJSDocTypeLiteral(const node<Node> &node);

    bool isJSDocSignature(const node<Node> &node);

    // JSDoc Tags

    bool isJSDocAugmentsTag(const node<Node> &node);

    bool isJSDocAuthorTag(const node<Node> &node);

    bool isJSDocClassTag(const node<Node> &node);

    bool isJSDocCallbackTag(const node<Node> &node);

    bool isJSDocPublicTag(const node<Node> &node);

    bool isJSDocPrivateTag(const node<Node> &node);

    bool isJSDocProtectedTag(const node<Node> &node);
    bool isJSDocReadonlyTag(const node<Node> &node);

    bool isJSDocOverrideTag(const node<Node> &node);

    bool isJSDocDeprecatedTag(const node<Node> &node);

    bool isJSDocSeeTag(const node<Node> &node);

    bool isJSDocEnumTag(const node<Node> &node);

    bool isJSDocParameterTag(const node<Node> &node);

    bool isJSDocReturnTag(const node<Node> &node);

    bool isJSDocThisTag(const node<Node> &node);

    bool isJSDocTypeTag(const node<Node> &node);

    bool isJSDocTemplateTag(const node<Node> &node);
    bool isJSDocTypedefTag(const node<Node> &node);

    bool isJSDocUnknownTag(const node<Node> &node);

    bool isJSDocPropertyTag(const node<Node> &node);

    bool isJSDocImplementsTag(const node<Node> &node);

    // Synthesized list

    /* @internal */
    bool isSyntaxList(const node<Node> &node);
}
