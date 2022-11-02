#include "node_test.h"
#include <fmt/core.h>

namespace tr {
    /**
     * Gets flags that control emit behavior of a node.
     */
    int getEmitFlags(const node<Node> &node) {
        return node->emitNode ? node->emitNode->flags : 0;
    }

    bool isCommaSequence(node<Node> node) {
        return node->kind == SyntaxKind::BinaryExpression && to<BinaryExpression>(node)->operatorToken->kind == SyntaxKind::CommaToken || node->kind == SyntaxKind::CommaListExpression;
    }

    bool isAssignmentOperator(SyntaxKind token) {
        return token >= SyntaxKind::FirstAssignment && token <= SyntaxKind::LastAssignment;
    }

    /**
     * Gets whether an identifier should only be referred to by its local name.
     */
    bool isLocalName(const node<Node> &node) {
        return (getEmitFlags(node) & (int) EmitFlags::LocalName) != 0;
    }

    bool hasModifier(const node<Node> &node, SyntaxKind kind) {
        if (!node->modifiers) return false;
        for (auto v: *node->modifiers) if (v->kind == kind) return true;
        return false;
    }

    ModifierFlags modifierToFlag(SyntaxKind token) {
        switch (token) {
            case SyntaxKind::StaticKeyword:
                return ModifierFlags::Static;
            case SyntaxKind::PublicKeyword:
                return ModifierFlags::Public;
            case SyntaxKind::ProtectedKeyword:
                return ModifierFlags::Protected;
            case SyntaxKind::PrivateKeyword:
                return ModifierFlags::Private;
            case SyntaxKind::AbstractKeyword:
                return ModifierFlags::Abstract;
            case SyntaxKind::ExportKeyword:
                return ModifierFlags::Export;
            case SyntaxKind::DeclareKeyword:
                return ModifierFlags::Ambient;
            case SyntaxKind::ConstKeyword:
                return ModifierFlags::Const;
            case SyntaxKind::DefaultKeyword:
                return ModifierFlags::Default;
            case SyntaxKind::AsyncKeyword:
                return ModifierFlags::Async;
            case SyntaxKind::ReadonlyKeyword:
                return ModifierFlags::Readonly;
            case SyntaxKind::OverrideKeyword:
                return ModifierFlags::Override;
            case SyntaxKind::InKeyword:
                return ModifierFlags::In;
            case SyntaxKind::OutKeyword:
                return ModifierFlags::Out;
        }
        return ModifierFlags::None;
    }

    int modifiersToFlags(node<NodeArray> modifiers) {
        int flags = (int) ModifierFlags::None;
        if (modifiers) {
            for (auto modifier: *modifiers) {
                flags |= (int) modifierToFlag(modifier->kind);
            }
        }
        return flags;
    }

    /**
     * Gets the ModifierFlags for syntactic modifiers on the provided node. The modifier flags cache on the node is ignored.
     *
     * NOTE: This function does not use `parent` pointers and will not include modifiers from JSDoc.
     */
    int getSyntacticModifierFlagsNoCache(node<Node> node) {
        auto flags = modifiersToFlags(node->modifiers);
        if (node->flags & (int) NodeFlags::NestedNamespace || (node->kind == SyntaxKind::Identifier && to<Identifier>(node)->isInJSDocNamespace)) {
            flags |= (int) ModifierFlags::Export;
        }
        return flags;
    }

    int getModifierFlagsWorker(node<Node> node, bool includeJSDoc, optional<bool> alwaysIncludeJSDoc) {
        if (node->kind >= SyntaxKind::FirstToken && node->kind <= SyntaxKind::LastToken) {
            return (int) ModifierFlags::None;
        }

        if (!(node->modifierFlagsCache & (int) ModifierFlags::HasComputedFlags)) {
            node->modifierFlagsCache = getSyntacticModifierFlagsNoCache(node) | (int) ModifierFlags::HasComputedFlags;
        }

        //we don't support JSDoc
//        if (includeJSDoc && !(node->modifierFlagsCache & (int)ModifierFlags::HasComputedJSDocModifiers) && (alwaysIncludeJSDoc || isInJSFile(node)) && node->hasParent()) {
//            node->modifierFlagsCache |= getJSDocModifierFlagsNoCache(node) | (int)ModifierFlags::HasComputedJSDocModifiers;
//        }

        return node->modifierFlagsCache & ~((int) ModifierFlags::HasComputedFlags | (int) ModifierFlags::HasComputedJSDocModifiers);
    }

    int getSyntacticModifierFlags(node<Node> node) {
        return getModifierFlagsWorker(node, /*includeJSDoc*/ false);
    }

    int getSelectedSyntacticModifierFlags(node<Node> node, int flags) {
        return getSyntacticModifierFlags(node) & flags;
    }

    bool hasSyntacticModifier(node<Node> node, int flags) {
        return !!getSelectedSyntacticModifierFlags(node, flags);
    }

    bool hasStaticModifier(node<Node> node) {
        return hasSyntacticModifier(node, (int) ModifierFlags::Static);
    }

    node<NodeUnion(PropertyName)> resolveNameToNode(const node<Node> &node) {
        switch (node->kind) {
            case SyntaxKind::Identifier:
            case SyntaxKind::StringLiteral:
            case SyntaxKind::NumericLiteral:
            case SyntaxKind::ComputedPropertyName:
            case SyntaxKind::PrivateIdentifier:
                return node;
        }

        throw runtime_error(fmt::format("resolveNamToNode with kind {} no valid name property", (int) node->kind));
    }

    bool isFunctionOrConstructorTypeNode(node<Node> node) {
        switch (node->kind) {
            case SyntaxKind::FunctionType:
            case SyntaxKind::ConstructorType:
                return true;
        }
        return false;
    }

    optionalNode<NodeArray> getTypeParameters(node<Node> node) {
        switch (node->kind) {
            case SyntaxKind::Constructor:
                return to<ConstructorDeclaration>(node)->typeParameters;
            case SyntaxKind::GetAccessor:
                return to<GetAccessorDeclaration>(node)->typeParameters;
            case SyntaxKind::SetAccessor:
                return to<SetAccessorDeclaration>(node)->typeParameters;
            case SyntaxKind::ClassExpression:
                return to<ClassExpression>(node)->typeParameters;
            case SyntaxKind::InterfaceDeclaration:
                return to<InterfaceDeclaration>(node)->typeParameters;
            case SyntaxKind::TypeAliasDeclaration:
                return to<TypeAliasDeclaration>(node)->typeParameters;
            case SyntaxKind::FunctionExpression:
                return to<FunctionExpression>(node)->typeParameters;
            case SyntaxKind::ArrowFunction:
                return to<ArrowFunction>(node)->typeParameters;
            case SyntaxKind::FunctionType:
                return to<FunctionTypeNode>(node)->typeParameters;
            case SyntaxKind::ConstructorType:
                return to<ConstructorTypeNode>(node)->typeParameters;
            case SyntaxKind::CallSignature:
                return to<CallSignatureDeclaration>(node)->typeParameters;
            case SyntaxKind::MethodSignature:
                return to<MethodSignature>(node)->typeParameters;
            case SyntaxKind::IndexSignature:
                return to<IndexSignatureDeclaration>(node)->typeParameters;
            default:
                throw runtime_error(fmt::format("node {} has no typeParameters", node->kind));
        }
    }

    node<NodeUnion(JsxTagNameExpression)> getTagName(node<NodeUnion(JsxOpeningElement, JsxOpeningFragment)> node) {
        switch (node->kind) {
            case SyntaxKind::JsxClosingElement:
                return to<JsxClosingElement>(node)->tagName;
            case SyntaxKind::JsxOpeningElement:
                return to<JsxOpeningElement>(node)->tagName;
            default:
                throw runtime_error(fmt::format("node {} has no tagName", node->kind));
        }
    }

    string &getEscapedName(const node<Node> &node) {
        switch (node->kind) {
            case SyntaxKind::Identifier:
                return reinterpret_cast<Identifier*>(node)->escapedText;
            case SyntaxKind::PrivateIdentifier:
                return reinterpret_cast<PrivateIdentifier*>(node)->escapedText;
        }

        throw runtime_error(fmt::format("getEscapedName with kind {} no valid", (int) node->kind));
    }

    optionalNode<NodeUnion(PropertyName)> getName(const node<Node> &node) {
        //[x] Check all with `name`
        //[x] Check child of NamedDeclaration
        //[x] Check child of SignatureDeclarationBase
        //[x] Check child of ClassLikeDeclarationBase
        //[x] Check child of DeclarationStatement
        //[x] Check child of FunctionOrConstructorTypeNodeBase

        switch (node->kind) {
            case SyntaxKind::FunctionType:
                return resolveNameToNode(to<FunctionTypeNode>(node)->name);
            case SyntaxKind::ConstructorType:
                return resolveNameToNode(to<ConstructorTypeNode>(node)->name);
            case SyntaxKind::PropertyAccessExpression:
                return resolveNameToNode(to<PropertyAccessExpression>(node)->name);
            case SyntaxKind::MetaProperty:
                return resolveNameToNode(to<MetaProperty>(node)->name);
            case SyntaxKind::ShorthandPropertyAssignment:
                return resolveNameToNode(to<ShorthandPropertyAssignment>(node)->name);
            case SyntaxKind::Parameter:
                return resolveNameToNode(to<ParameterDeclaration>(node)->name);
            case SyntaxKind::MissingDeclaration:
                return resolveNameToNode(to<MissingDeclaration>(node)->name);
            case SyntaxKind::InterfaceDeclaration:
                return resolveNameToNode(to<InterfaceDeclaration>(node)->name);
            case SyntaxKind::ClassDeclaration:
                return resolveNameToNode(to<ClassDeclaration>(node)->name);
            case SyntaxKind::ClassExpression:
                return resolveNameToNode(to<ClassExpression>(node)->name);
            case SyntaxKind::TypeAliasDeclaration:
                return resolveNameToNode(to<TypeAliasDeclaration>(node)->name);
            case SyntaxKind::EnumDeclaration:
                return resolveNameToNode(to<EnumDeclaration>(node)->name);
            case SyntaxKind::EnumMember:
                return resolveNameToNode(to<EnumMember>(node)->name);
            case SyntaxKind::PropertySignature:
                return resolveNameToNode(to<PropertySignature>(node)->name);
            case SyntaxKind::ModuleDeclaration:
                return resolveNameToNode(to<ModuleDeclaration>(node)->name);
            case SyntaxKind::ImportEqualsDeclaration:
                return resolveNameToNode(to<ImportEqualsDeclaration>(node)->name);
            case SyntaxKind::NamespaceImport:
                return resolveNameToNode(to<NamespaceImport>(node)->name);
            case SyntaxKind::ImportSpecifier:
                return resolveNameToNode(to<ImportSpecifier>(node)->name);
            case SyntaxKind::AssertEntry:
                return resolveNameToNode(to<AssertEntry>(node)->name);
            case SyntaxKind::ImportClause:
                return resolveNameToNode(to<ImportClause>(node)->name);
            case SyntaxKind::NamespaceExport:
                return resolveNameToNode(to<NamespaceExport>(node)->name);
            case SyntaxKind::ExportSpecifier:
                return resolveNameToNode(to<ExportSpecifier>(node)->name);
            case SyntaxKind::ExportDeclaration:
                return resolveNameToNode(to<ExportDeclaration>(node)->name);
            case SyntaxKind::NamespaceExportDeclaration:
                return resolveNameToNode(to<NamespaceExportDeclaration>(node)->name);
            case SyntaxKind::MethodSignature:
                return resolveNameToNode(to<MethodSignature>(node)->name);
            case SyntaxKind::FunctionDeclaration:
                return resolveNameToNode(to<FunctionDeclaration>(node)->name);
            case SyntaxKind::MethodDeclaration:
                return resolveNameToNode(to<MethodDeclaration>(node)->name);
            case SyntaxKind::GetAccessor:
                return resolveNameToNode(to<GetAccessorDeclaration>(node)->name);
            case SyntaxKind::SetAccessor:
                return resolveNameToNode(to<SetAccessorDeclaration>(node)->name);
            case SyntaxKind::FunctionExpression:
                return resolveNameToNode(to<FunctionExpression>(node)->name);
            case SyntaxKind::NamedTupleMember:
                return resolveNameToNode(to<NamedTupleMember>(node)->name);
            case SyntaxKind::JsxAttribute:
                return resolveNameToNode(to<JsxAttribute>(node)->name);
            case SyntaxKind::TypeParameter:
                return resolveNameToNode(to<TypeParameterDeclaration>(node)->name);
            case SyntaxKind::BindingElement:
                return resolveNameToNode(to<BindingElement>(node)->name);
            case SyntaxKind::VariableDeclaration:
                return resolveNameToNode(to<VariableDeclaration>(node)->name);
            case SyntaxKind::IndexSignature:
                return resolveNameToNode(to<IndexSignatureDeclaration>(node)->name);
            case SyntaxKind::CallSignature:
                return resolveNameToNode(to<CallSignatureDeclaration>(node)->name);
            case SyntaxKind::ConstructSignature:
                return resolveNameToNode(to<ConstructSignatureDeclaration>(node)->name);
            case SyntaxKind::ExportAssignment:
                return resolveNameToNode(to<ExportAssignment>(node)->name);
            case SyntaxKind::Identifier:
            case SyntaxKind::StringLiteral:
            case SyntaxKind::NumericLiteral:
            case SyntaxKind::ComputedPropertyName:
            case SyntaxKind::PrivateIdentifier:
                return node;
        }

        return nullptr;
    }

    bool isNumericLiteral(const node<Node> &node) {
        return node->kind == SyntaxKind::NumericLiteral;
    }

    bool isBigIntLiteral(const node<Node> &node) {
        return node->kind == SyntaxKind::BigIntLiteral;
    }

    bool isStringLiteral(const node<Node> &node) {
        return node->kind == SyntaxKind::StringLiteral;
    }

    bool isJsxText(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxText;
    }

    bool isRegularExpressionLiteral(const node<Node> &node) {
        return node->kind == SyntaxKind::RegularExpressionLiteral;
    }

    bool isNoSubstitutionTemplateLiteral(const node<Node> &node) {
        return node->kind == SyntaxKind::NoSubstitutionTemplateLiteral;
    }

    bool isStringLiteralLike(node<Node> node) {
        return node->kind == SyntaxKind::StringLiteral || node->kind == SyntaxKind::NoSubstitutionTemplateLiteral;
    }

    bool isStringOrNumericLiteralLike(node<Node> node) {
        return isStringLiteralLike(node) || isNumericLiteral(node);
    }

    // Pseudo-literals

    bool isTemplateHead(const node<Node> &node) {
        return node->kind == SyntaxKind::TemplateHead;
    }

    bool isTemplateMiddle(const node<Node> &node) {
        return node->kind == SyntaxKind::TemplateMiddle;
    }

    bool isTemplateTail(const node<Node> &node) {
        return node->kind == SyntaxKind::TemplateTail;
    }

    // Punctuation

    bool isDotDotDotToken(const node<Node> &node) {
        return node->kind == SyntaxKind::DotDotDotToken;
    }

    /*@internal*/
    bool isCommaToken(const node<Node> &node) {
        return node->kind == SyntaxKind::CommaToken;
    }

    bool isPlusToken(const node<Node> &node) {
        return node->kind == SyntaxKind::PlusToken;
    }

    bool isMinusToken(const node<Node> &node) {
        return node->kind == SyntaxKind::MinusToken;
    }

    bool isAsteriskToken(const node<Node> &node) {
        return node->kind == SyntaxKind::AsteriskToken;
    }

    /*@internal*/
    bool isExclamationToken(const node<Node> &node) {
        return node->kind == SyntaxKind::ExclamationToken;
    }

    /*@internal*/
    bool isQuestionToken(const node<Node> &node) {
        return node->kind == SyntaxKind::QuestionToken;
    }

    /*@internal*/
    bool isColonToken(const node<Node> &node) {
        return node->kind == SyntaxKind::ColonToken;
    }

    /*@internal*/
    bool isQuestionDotToken(const node<Node> &node) {
        return node->kind == SyntaxKind::QuestionDotToken;
    }

    /*@internal*/
    bool isEqualsGreaterThanToken(const node<Node> &node) {
        return node->kind == SyntaxKind::EqualsGreaterThanToken;
    }

    // Identifiers

    bool isIdentifier(const node<Node> &node) {
        return node->kind == SyntaxKind::Identifier;
    }

    bool isPrivateIdentifier(const node<Node> &node) {
        return node->kind == SyntaxKind::PrivateIdentifier;
    }

    bool identifierIsThisKeyword(const node<Identifier> &id) {
        return id->originalKeywordKind == SyntaxKind::ThisKeyword;
    }

    bool isThisIdentifier(optionalNode<Node> node) {
        if (!node) return false;
        return node->kind == SyntaxKind::Identifier && identifierIsThisKeyword(to<Identifier>(node));
    }

    /**
     * Determines whether a node is a property or element access expression for `super`.
     */
    bool isSuperProperty(node<Node> node) {
        if (node->is<PropertyAccessExpression>()) return to<PropertyAccessExpression>(node)->expression->kind == SyntaxKind::SuperKeyword;
        if (node->is<ElementAccessExpression>()) return to<ElementAccessExpression>(node)->expression->kind == SyntaxKind::SuperKeyword;
        return false;
    }

    // Reserved Words

    /* @internal */
    bool isExportModifier(const node<Node> &node) {
        return node->kind == SyntaxKind::ExportKeyword;
    }

    /* @internal */
    bool isAsyncModifier(const node<Node> &node) {
        return node->kind == SyntaxKind::AsyncKeyword;
    }

    /* @internal */
    bool isAssertsKeyword(const node<Node> &node) {
        return node->kind == SyntaxKind::AssertsKeyword;
    }

    /* @internal */
    bool isAwaitKeyword(const node<Node> &node) {
        return node->kind == SyntaxKind::AwaitKeyword;
    }

    /* @internal */
    bool isReadonlyKeyword(const node<Node> &node) {
        return node->kind == SyntaxKind::ReadonlyKeyword;
    }

    /* @internal */
    bool isStaticModifier(const node<Node> &node) {
        return node->kind == SyntaxKind::StaticKeyword;
    }

    /* @internal */
    bool isAbstractModifier(const node<Node> &node) {
        return node->kind == SyntaxKind::AbstractKeyword;
    }

    /*@internal*/
    bool isSuperKeyword(const node<Node> &node) {
        return node->kind == SyntaxKind::SuperKeyword;
    }

    /*@internal*/
    bool isImportKeyword(const node<Node> &node) {
        return node->kind == SyntaxKind::ImportKeyword;
    }

    // Names

    bool isQualifiedName(const node<Node> &node) {
        return node->kind == SyntaxKind::QualifiedName;
    }

    bool isComputedPropertyName(const node<Node> &node) {
        return node->kind == SyntaxKind::ComputedPropertyName;
    }

    // Signature elements

    bool isTypeParameterDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::TypeParameter;
    }

    // TODO(rbuckton): Rename to 'isParameterDeclaration'
    bool isParameter(const node<Node> &node) {
        return node->kind == SyntaxKind::Parameter;
    }

    bool isDecorator(const node<Node> &node) {
        return node->kind == SyntaxKind::Decorator;
    }

    // TypeMember

    bool isPropertySignature(const node<Node> &node) {
        return node->kind == SyntaxKind::PropertySignature;
    }

    bool isPropertyDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::PropertyDeclaration;
    }

    bool isMethodSignature(const node<Node> &node) {
        return node->kind == SyntaxKind::MethodSignature;
    }

    bool isMethodDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::MethodDeclaration;
    }

    bool isClassStaticBlockDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::ClassStaticBlockDeclaration;
    }

    bool isConstructorDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::Constructor;
    }

    bool isGetAccessorDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::GetAccessor;
    }

    bool isSetAccessorDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::SetAccessor;
    }

    bool isCallSignatureDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::CallSignature;
    }

    bool isConstructSignatureDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::ConstructSignature;
    }

    bool isIndexSignatureDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::IndexSignature;
    }

    // Type

    bool isTypePredicateNode(const node<Node> &node) {
        return node->kind == SyntaxKind::TypePredicate;
    }

    bool isTypeReferenceNode(const node<Node> &node) {
        return node->kind == SyntaxKind::TypeReference;
    }

    bool isFunctionTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::FunctionType;
    }

    bool isConstructorTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::ConstructorType;
    }

    bool isTypeQueryNode(const node<Node> &node) {
        return node->kind == SyntaxKind::TypeQuery;
    }

    bool isTypeLiteralNode(const node<Node> &node) {
        return node->kind == SyntaxKind::TypeLiteral;
    }

    bool isArrayTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::ArrayType;
    }

    bool isTupleTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::TupleType;
    }

    bool isNamedTupleMember(const node<Node> &node) {
        return node->kind == SyntaxKind::NamedTupleMember;
    }

    bool isOptionalTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::OptionalType;
    }

    bool isRestTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::RestType;
    }

    bool isUnionTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::UnionType;
    }

    bool isIntersectionTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::IntersectionType;
    }

    bool isConditionalTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::ConditionalType;
    }

    bool isInferTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::InferType;
    }

    bool isParenthesizedTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::ParenthesizedType;
    }

    bool isThisTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::ThisType;
    }

    bool isTypeOperatorNode(const node<Node> &node) {
        return node->kind == SyntaxKind::TypeOperator;
    }

    bool isIndexedAccessTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::IndexedAccessType;
    }

    bool isMappedTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::MappedType;
    }

    bool isLiteralTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::LiteralType;
    }

    bool isImportTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::ImportType;
    }

    bool isTemplateLiteralTypeSpan(const node<Node> &node) {
        return node->kind == SyntaxKind::TemplateLiteralTypeSpan;
    }

    bool isTemplateLiteralTypeNode(const node<Node> &node) {
        return node->kind == SyntaxKind::TemplateLiteralType;
    }

    // Binding patterns

    bool isObjectBindingPattern(const node<Node> &node) {
        return node->kind == SyntaxKind::ObjectBindingPattern;
    }

    bool isArrayBindingPattern(const node<Node> &node) {
        return node->kind == SyntaxKind::ArrayBindingPattern;
    }

    bool isBindingElement(const node<Node> &node) {
        return node->kind == SyntaxKind::BindingElement;
    }

    // Expression

    bool isArrayLiteralExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::ArrayLiteralExpression;
    }

    bool isObjectLiteralExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::ObjectLiteralExpression;
    }

    bool isPropertyAccessExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::PropertyAccessExpression;
    }

    bool isElementAccessExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::ElementAccessExpression;
    }

    bool isCallExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::CallExpression;
    }

    bool isCallChain(node<Node> node) {
        return isCallExpression(node) && !!(node->flags & (int)NodeFlags::OptionalChain);
    }

    bool isNewExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::NewExpression;
    }

    bool isTaggedTemplateExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::TaggedTemplateExpression;
    }

    bool isTypeAssertionExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::TypeAssertionExpression;
    }

    bool isParenthesizedExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::ParenthesizedExpression;
    }

    bool isFunctionExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::FunctionExpression;
    }

    bool isArrowFunction(const node<Node> &node) {
        return node->kind == SyntaxKind::ArrowFunction;
    }

    bool isDeleteExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::DeleteExpression;
    }

    bool isTypeOfExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::TypeOfExpression;
    }

    bool isVoidExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::VoidExpression;
    }

    bool isAwaitExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::AwaitExpression;
    }

    bool isPrefixUnaryExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::PrefixUnaryExpression;
    }

    bool isPostfixUnaryExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::PostfixUnaryExpression;
    }

    bool isBinaryExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::BinaryExpression;
    }

    bool isConditionalExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::ConditionalExpression;
    }

    bool isTemplateExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::TemplateExpression;
    }

    bool isYieldExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::YieldExpression;
    }

    bool isSpreadElement(const node<Node> &node) {
        return node->kind == SyntaxKind::SpreadElement;
    }

    bool isClassExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::ClassExpression;
    }

    bool isOmittedExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::OmittedExpression;
    }

    bool isExpressionWithTypeArguments(const node<Node> &node) {
        return node->kind == SyntaxKind::ExpressionWithTypeArguments;
    }

    bool isAsExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::AsExpression;
    }

    bool isNonNullExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::NonNullExpression;
    }

    bool isNonNullChain(node<Node> node) {
        return isNonNullExpression(node) && !!(node->flags & (int)NodeFlags::OptionalChain);
    }

    bool isMetaProperty(const node<Node> &node) {
        return node->kind == SyntaxKind::MetaProperty;
    }

    bool isSyntheticExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::SyntheticExpression;
    }

    bool isPartiallyEmittedExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::PartiallyEmittedExpression;
    }

    bool isCommaListExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::CommaListExpression;
    }

    // Misc

    bool isTemplateSpan(const node<Node> &node) {
        return node->kind == SyntaxKind::TemplateSpan;
    }

    bool isSemicolonClassElement(const node<Node> &node) {
        return node->kind == SyntaxKind::SemicolonClassElement;
    }

    // Elements

    bool isBlock(const node<Node> &node) {
        return node->kind == SyntaxKind::Block;
    }

    bool isVariableStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::VariableStatement;
    }

    bool isEmptyStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::EmptyStatement;
    }

    bool isExpressionStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::ExpressionStatement;
    }

    bool isIfStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::IfStatement;
    }

    bool isDoStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::DoStatement;
    }

    bool isWhileStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::WhileStatement;
    }

    bool isForStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::ForStatement;
    }

    bool isForInStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::ForInStatement;
    }

    bool isForOfStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::ForOfStatement;
    }

    bool isContinueStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::ContinueStatement;
    }

    bool isBreakStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::BreakStatement;
    }

    bool isReturnStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::ReturnStatement;
    }

    bool isWithStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::WithStatement;
    }

    bool isSwitchStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::SwitchStatement;
    }

    bool isLabeledStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::LabeledStatement;
    }

    bool isThrowStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::ThrowStatement;
    }

    bool isTryStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::TryStatement;
    }

    bool isDebuggerStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::DebuggerStatement;
    }

    bool isVariableDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::VariableDeclaration;
    }

    bool isVariableDeclarationList(const node<Node> &node) {
        return node->kind == SyntaxKind::VariableDeclarationList;
    }

    bool isFunctionDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::FunctionDeclaration;
    }

    bool isClassDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::ClassDeclaration;
    }

    bool isInterfaceDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::InterfaceDeclaration;
    }

    bool isTypeAliasDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::TypeAliasDeclaration;
    }

    bool isEnumDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::EnumDeclaration;
    }

    bool isModuleDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::ModuleDeclaration;
    }

    bool isModuleBlock(const node<Node> &node) {
        return node->kind == SyntaxKind::ModuleBlock;
    }

    bool isCaseBlock(const node<Node> &node) {
        return node->kind == SyntaxKind::CaseBlock;
    }

    bool isNamespaceExportDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::NamespaceExportDeclaration;
    }

    bool isImportEqualsDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::ImportEqualsDeclaration;
    }

    bool isImportDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::ImportDeclaration;
    }

    bool isImportClause(const node<Node> &node) {
        return node->kind == SyntaxKind::ImportClause;
    }

    bool isAssertClause(const node<Node> &node) {
        return node->kind == SyntaxKind::AssertClause;
    }

    bool isAssertEntry(const node<Node> &node) {
        return node->kind == SyntaxKind::AssertEntry;
    }

    bool isNamespaceImport(const node<Node> &node) {
        return node->kind == SyntaxKind::NamespaceImport;
    }

    bool isNamespaceExport(const node<Node> &node) {
        return node->kind == SyntaxKind::NamespaceExport;
    }

    bool isNamedImports(const node<Node> &node) {
        return node->kind == SyntaxKind::NamedImports;
    }

    bool isImportSpecifier(const node<Node> &node) {
        return node->kind == SyntaxKind::ImportSpecifier;
    }

    bool isExportAssignment(const node<Node> &node) {
        return node->kind == SyntaxKind::ExportAssignment;
    }

    bool isExportDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::ExportDeclaration;
    }

    bool isNamedExports(const node<Node> &node) {
        return node->kind == SyntaxKind::NamedExports;
    }

    bool isExportSpecifier(const node<Node> &node) {
        return node->kind == SyntaxKind::ExportSpecifier;
    }

    bool isMissingDeclaration(const node<Node> &node) {
        return node->kind == SyntaxKind::MissingDeclaration;
    }

    bool isNotEmittedStatement(const node<Node> &node) {
        return node->kind == SyntaxKind::NotEmittedStatement;
    }

    /* @internal */
    bool isSyntheticReference(const node<Node> &node) {
        return node->kind == SyntaxKind::SyntheticReferenceExpression;
    }

    /* @internal */
    bool isMergeDeclarationMarker(const node<Node> &node) {
        return node->kind == SyntaxKind::MergeDeclarationMarker;
    }

    /* @internal */
    bool isEndOfDeclarationMarker(const node<Node> &node) {
        return node->kind == SyntaxKind::EndOfDeclarationMarker;
    }

    // Module References

    bool isExternalModuleReference(const node<Node> &node) {
        return node->kind == SyntaxKind::ExternalModuleReference;
    }

    // JSX

    bool isJsxElement(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxElement;
    }

    bool isJsxSelfClosingElement(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxSelfClosingElement;
    }

    bool isJsxOpeningElement(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxOpeningElement;
    }

    bool isJsxClosingElement(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxClosingElement;
    }

    bool isJsxFragment(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxFragment;
    }

    bool isJsxOpeningFragment(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxOpeningFragment;
    }

    bool isJsxClosingFragment(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxClosingFragment;
    }

    bool isJsxAttribute(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxAttribute;
    }

    bool isJsxAttributes(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxAttributes;
    }

    bool isJsxSpreadAttribute(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxSpreadAttribute;
    }

    bool isJsxExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::JsxExpression;
    }

    // Clauses

    bool isCaseClause(const node<Node> &node) {
        return node->kind == SyntaxKind::CaseClause;
    }

    bool isDefaultClause(const node<Node> &node) {
        return node->kind == SyntaxKind::DefaultClause;
    }

    bool isHeritageClause(const node<Node> &node) {
        return node->kind == SyntaxKind::HeritageClause;
    }

    bool isCatchClause(const node<Node> &node) {
        return node->kind == SyntaxKind::CatchClause;
    }

    // Property assignments

    bool isPropertyAssignment(const node<Node> &node) {
        return node->kind == SyntaxKind::PropertyAssignment;
    }

    bool isShorthandPropertyAssignment(const node<Node> &node) {
        return node->kind == SyntaxKind::ShorthandPropertyAssignment;
    }

    bool isSpreadAssignment(const node<Node> &node) {
        return node->kind == SyntaxKind::SpreadAssignment;
    }

    // Enum

    bool isEnumMember(const node<Node> &node) {
        return node->kind == SyntaxKind::EnumMember;
    }

    // Unparsed

    // TODO(rbuckton): isUnparsedPrologue

    bool isUnparsedPrepend(const node<Node> &node) {
        return node->kind == SyntaxKind::UnparsedPrepend;
    }

    // TODO(rbuckton): isUnparsedText
    // TODO(rbuckton): isUnparsedInternalText
    // TODO(rbuckton): isUnparsedSyntheticReference

    // Top-level nodes
    bool isSourceFile(const node<Node> &node) {
        return node->kind == SyntaxKind::SourceFile;
    }

    bool isBundle(const node<Node> &node) {
        return node->kind == SyntaxKind::Bundle;
    }

    bool isUnparsedSource(const node<Node> &node) {
        return node->kind == SyntaxKind::UnparsedSource;
    }

    // TODO(rbuckton): isInputFiles

    // JSDoc Elements

    bool isJSDocTypeExpression(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocTypeExpression;
    }

    bool isJSDocNameReference(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocNameReference;
    }

    bool isJSDocMemberName(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocMemberName;
    }

    bool isJSDocLink(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocLink;
    }

    bool isJSDocLinkCode(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocLinkCode;
    }

    bool isJSDocLinkPlain(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocLinkPlain;
    }

    bool isJSDocAllType(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocAllType;
    }

    bool isJSDocUnknownType(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocUnknownType;
    }

    bool isJSDocNullableType(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocNullableType;
    }

    bool isJSDocNonNullableType(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocNonNullableType;
    }

    bool isJSDocOptionalType(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocOptionalType;
    }

    bool isJSDocFunctionType(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocFunctionType;
    }

    bool isJSDocVariadicType(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocVariadicType;
    }

    bool isJSDocNamepathType(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocNamepathType;
    }

    bool isJSDoc(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDoc;
    }

    bool isJSDocTypeLiteral(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocTypeLiteral;
    }

    bool isJSDocSignature(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocSignature;
    }

    // JSDoc Tags

    bool isJSDocAugmentsTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocAugmentsTag;
    }

    bool isJSDocAuthorTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocAuthorTag;
    }

    bool isJSDocClassTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocClassTag;
    }

    bool isJSDocCallbackTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocCallbackTag;
    }

    bool isJSDocPublicTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocPublicTag;
    }

    bool isJSDocPrivateTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocPrivateTag;
    }

    bool isJSDocProtectedTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocProtectedTag;
    }

    bool isJSDocReadonlyTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocReadonlyTag;
    }

    bool isJSDocOverrideTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocOverrideTag;
    }

    bool isJSDocDeprecatedTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocDeprecatedTag;
    }

    bool isJSDocSeeTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocSeeTag;
    }

    bool isJSDocEnumTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocEnumTag;
    }

    bool isJSDocParameterTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocParameterTag;
    }

    bool isJSDocReturnTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocReturnTag;
    }

    bool isJSDocThisTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocThisTag;
    }

    bool isJSDocTypeTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocTypeTag;
    }

    bool isJSDocTemplateTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocTemplateTag;
    }

    bool isJSDocTypedefTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocTypedefTag;
    }

    bool isJSDocUnknownTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocTag;
    }

    bool isJSDocPropertyTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocPropertyTag;
    }

    bool isJSDocImplementsTag(const node<Node> &node) {
        return node->kind == SyntaxKind::JSDocImplementsTag;
    }

    // Synthesized list

    /* @internal */
    bool isSyntaxList(const node<Node> &node) {
        return node->kind == SyntaxKind::SyntaxList;
    }
}
