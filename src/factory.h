#pragma once

#include <utility>

#include "Tracy.hpp"
#include "types.h"
#include "utilities.h"
#include "scanner.h"
#include "node_test.h"
#include "parenthesizer.h"
#include "pool_band.h"

namespace tr {

    struct Parenthesizer;

    struct Factory {
        PoolBand *pool;

        int nextAutoGenerateId = 0;

        /* @internal */
        enum class NodeFactoryFlags {
            None = 0,
            // Disables the parenthesizer rules for the 
            NoParenthesizerRules = 1 << 0,
            // Disables the node converters for the 
            NoNodeConverters = 1 << 1,
            // Ensures new `PropertyAccessExpression` nodes are created with the `NoIndentation` emit flag set.
            NoIndentationOnFreshPropertyAccess = 1 << 2,
            // Do not set an `original` pointer when updating a node->
            NoOriginalNode = 1 << 3,
        };

        Parenthesizer parenthesizer;

        Factory () {
            parenthesizer.factory = this;
        }

        int propagatePropertyNameFlagsOfChild(node<NodeUnion(PropertyName)> &node, int transformFlags);

        int propagateChildFlags(optionalNode<Node> child);

        int propagateIdentifierNameFlags(node<Node> node);

        int propagateChildrenFlags(const optionalNode<NodeArray> &children);

        void aggregateChildrenFlags(const node<NodeArray> &children);

        node<NodeArray> createNodeArray(const optionalNode<NodeArray> &elements, bool hasTrailingComma = false);

        optionalNode<NodeArray> asNodeArray(optionalNode<NodeArray> elements);

//        template<class T>
//        inline node<NodeArray> asNodeArray(const vector<node<T>> &array) {
//            auto nodeArray = make_shared<NodeArray>();
//            for (auto &&node: array) nodeArray->list.push_back(node);
//            return nodeArray;
//        }

//        template<class T>
//        optionalNode<NodeArray> asNodeArray(optional<vector<node<T>>> &array) {
//            if (!array) return nullptr;
//
//            return asNodeArray(*array);
//        }

        // @api
        template<class T>
        node<NodeArray> createNodeArray(const vector<node<T>> &elements, bool hasTrailingComma = false) {
            // Since the element list of a node array is typically created by starting with an empty array and
            // repeatedly calling push(), the list may not have the optimal memory layout. We invoke slice() for
            // small arrays (1 to 4 elements) to give the VM a chance to allocate an optimal representation.
            return createNodeArray(asNodeArray(elements), hasTrailingComma);
        }

        template<class T>
        node<T> createBaseNode() {
            auto node = pool->construct<T>();
            node->kind = (types::SyntaxKind) T::KIND;
            return node;
        }

        template<class T>
        node<T> createBaseNode(SyntaxKind kind) {
            auto node = pool->construct<T>();
            node->kind = kind;
            return node;
        }

        template<class T>
        node<T> createBaseToken(SyntaxKind kind) {
            return createBaseNode<T>(kind);
        }

        //
        // Literals
        //
        template<typename T>
        node<T> createBaseLiteral(SyntaxKind kind, string text) {
            auto node = createBaseToken<T>(kind);
            node->text = text;
            return node;
        }

        // @api
        node<NumericLiteral> createNumericLiteral(string value, int numericLiteralFlags = (int) types::TokenFlags::None);

        node<NumericLiteral> createNumericLiteral(double value, types::TokenFlags numericLiteralFlags = types::TokenFlags::None);

        // @api
        node<BigIntLiteral> createBigIntLiteral(variant<string, PseudoBigInt> value);

        node<StringLiteral> createBaseStringLiteral(string text, optional<bool> isSingleQuote = {});

        // @api
        node<StringLiteral> createStringLiteral(string text, optional<bool> isSingleQuote = {}, optional<bool> hasExtendedUnicodeEscape = {});

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
        node<T> createToken(SyntaxKind token) {
            ZoneScoped
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
        node<SuperExpression> createSuper();

        // @api
        node<ThisExpression> createThis();

        // @api
        node<NullLiteral> createNull();

        // @api
        node<TrueLiteral> createTrue();

        // @api
        node<FalseLiteral> createFalse();

        node<Expression> createBooleanLiteral(bool v);

        using NameType = variant<string, node<Node>>;

        node<Node> asName(NameType name = {});

        optionalNode<Node> asName(optional<NameType> name = {});

        using ExpressionType = variant<string, int, bool, optionalNode<Expression>>;

        optionalNode<Expression> asExpression(ExpressionType value);

//        function createBaseNode<T extends Node>(kind: T["kind"]) {
//            return basecreateBaseNode(kind) as Mutable<T>;
//        }
//
        template<class T>
        node<T> createBaseDeclaration(
                SyntaxKind kind,
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers
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

        void setName(node<Identifier> &lName, node<Node> rName);

        void setName(node<PrivateIdentifier> &lName, node<Node> rName);

        void setName(node<Node> &lName, node<Node> rName);

        template<class T>
        node<T> updateWithoutOriginal(node<T> updated, node<T> original) {
            if (updated != original) {
                setTextRange(updated, original);
            }
            return updated;
        }

        template<class T>
        node<T> update(node<T> updated, node<T> original) {
            return updateWithoutOriginal<T>(updated, original);
        }

        template<class T>
        node<T> createBaseNamedDeclaration(
                SyntaxKind kind,
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                optional<NameType> _name
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
        node<T> createBaseGenericNamedDeclaration(
                SyntaxKind kind,
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                NameType name,
                optionalNode<NodeTypeArray(TypeParameterDeclaration)> typeParameters
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
        node<T> createBaseSignatureDeclaration(
                SyntaxKind kind,
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                NameType name,
                optionalNode<NodeTypeArray(TypeParameterDeclaration)> typeParameters,
                optionalNode<NodeTypeArray(ParameterDeclaration)> parameters,
                optionalNode<TypeNode> type
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
        node<T> createBaseFunctionLikeDeclaration(
                SyntaxKind kind,
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                NameType name,
                optionalNode<NodeArray> typeParameters,
                optionalNode<NodeArray> parameters,
                optionalNode<TypeNode> type,
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
        node<T> createBaseInterfaceOrClassLikeDeclaration(
                SyntaxKind kind,
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                NameType name,
                optionalNode<NodeArray> typeParameters,
                optionalNode<NodeArray> heritageClauses
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
        node<T> createBaseClassLikeDeclaration(
                SyntaxKind kind,
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                NameType name,
                optionalNode<NodeArray> typeParameters,
                optionalNode<NodeArray> heritageClauses,
                node<NodeArray> members
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
        node<T> createBaseBindingLikeDeclaration(
                SyntaxKind kind,
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                optional<NameType> name,
                optionalNode<Expression> initializer
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
        node<T> createBaseVariableLikeDeclaration(
                SyntaxKind kind,
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                optional<NameType> name,
                optionalNode<TypeNode> type,
                optionalNode<Expression> initializer
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
        node<RegularExpressionLiteral> createRegularExpressionLiteral(string text);

        // @api
        node<JsxText> createJsxText(string text, optional<bool> containsOnlyTriviaWhiteSpaces = {});

        // @api
        node<TemplateLiteralLike> createTemplateLiteralLikeNode(SyntaxKind kind, string text, optional<string> rawText = {}, optional<int> templateFlags = {});

        // @api
        node<LiteralLike> createLiteralLikeNode(SyntaxKind kind, string text);

//        //
//        // Identifiers
//        //

        node<Identifier> createBaseIdentifier(string text, optional<SyntaxKind> originalKeywordKind);

        node<Identifier> createBaseGeneratedIdentifier(string text, GeneratedIdentifierFlags autoGenerateFlags);

        // @api
        node<Identifier> createIdentifier(string text, optionalNode<NodeArray> typeArguments = {}, optional<SyntaxKind> originalKeywordKind = {});
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
        node<PrivateIdentifier> createPrivateIdentifier(string text);

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
        node<QualifiedName> createQualifiedName(node<Node> left, NameType right);

//        // @api
//        function updateQualifiedName(node: QualifiedName, left: EntityName, right: Identifier) {
//            return node->left != left
//                || node->right != right
//                ? update(createQualifiedName(left, right), node)
//                : node;
//        }
//
        // @api
        node<ComputedPropertyName> createComputedPropertyName(node<Expression> expression);

//        // @api
//        function updateComputedPropertyName(node: ComputedPropertyName, node<Expression> expression) {
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
//        function createTypeParameterDeclaration(optionalNode<NodeArray> modifiers, NameType name, constraint?: TypeNode, defaultType?: TypeNode): TypeParameterDeclaration;
//        /** @deprecated */
//        function createTypeParameterDeclaration(NameType name, constraint?: TypeNode, defaultType?: TypeNode): TypeParameterDeclaration;
       node<TypeParameterDeclaration> createTypeParameterDeclaration(
                optional<variant<node<NodeArray>, node<Node>, string>> modifiersOrName,
                optional<variant<node<TypeNode>, node<Identifier>, string>> nameOrConstraint,
                optionalNode<TypeNode> constraintOrDefault,
                optionalNode<TypeNode> defaultType = nullptr
        );

//        // @api
//        function updateTypeParameterDeclaration(node: TypeParameterDeclaration, optionalNode<NodeArray> modifiers, name: Identifier, constraint: TypeNode | undefined, defaultsharedOpt<TypeNode> type): TypeParameterDeclaration;
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
        node<ParameterDeclaration> createParameterDeclaration(
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                optionalNode<DotDotDotToken> dotDotDotToken = nullptr,
                NameType name = "",
                optionalNode<QuestionToken> questionToken = nullptr,
                optionalNode<TypeNode> type = nullptr,
                optionalNode<Expression> initializer = nullptr
        );

//        // @api
//        function updateParameterDeclaration(
//            node: ParameterDeclaration,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            dotDotDotToken: DotDotDotToken | undefined,
//            name: string | BindingName,
//            optionalNode<QuestionToken> questionToken,
//            optionalNode<TypeNode> type,
//            optionalNode<Expression> initializer
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
        node<Decorator> createDecorator(node<Expression> expression);

//        // @api
//        function updateDecorator(node: Decorator, node<Expression> expression) {
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
        node<PropertySignature> createPropertySignature(
                optionalNode<NodeArray> modifiers,
                NameType name,
                optionalNode<QuestionToken> questionToken,
                optionalNode<TypeNode> type
        );

//        // @api
//        function updatePropertySignature(
//            node: PropertySignature,
//            optionalNode<NodeArray> modifiers,
//            name: PropertyName,
//            optionalNode<QuestionToken> questionToken,
//            optionalNode<TypeNode> type
//        ) {
//            return node->modifiers != modifiers
//                || node->name != name
//                || node->questionToken != questionToken
//                || node->type != type
//                ? update(createPropertySignature(modifiers, name, questionToken, type), node)
//                : node;
//        }

        // @api
        node<PropertyDeclaration> createPropertyDeclaration(
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                NameType name,
                optionalNode<Node> questionOrExclamationToken,
                optionalNode<TypeNode> type,
                optionalNode<Expression> initializer
        );

//        // @api
//        function updatePropertyDeclaration(
//            node: PropertyDeclaration,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            NameType name,
//            questionOrExclamationToken: QuestionToken | ExclamationToken | undefined,
//            optionalNode<TypeNode> type,
//            optionalNode<Expression> initializer
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
        node<MethodSignature> createMethodSignature(
                optionalNode<NodeArray> modifiers,
                NameType name,
                optionalNode<QuestionToken> questionToken,
                optionalNode<NodeArray> typeParameters,
                node<NodeArray> parameters,
                optionalNode<TypeNode> type
        );

//        // @api
//        function updateMethodSignature(
//            node: MethodSignature,
//            optionalNode<NodeArray> modifiers,
//            name: PropertyName,
//            optionalNode<QuestionToken> questionToken,
//            typeParameters: NodeArray<TypeParameterDeclaration> | undefined,
//            parameters: NodeArray<ParameterDeclaration>,
//            optionalNode<TypeNode> type
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
        node<MethodDeclaration> createMethodDeclaration(
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                optionalNode<AsteriskToken> asteriskToken,
                NameType name,
                optionalNode<QuestionToken> questionToken,
                optionalNode<NodeArray> typeParameters,
                node<NodeArray> parameters,
                optionalNode<TypeNode> type,
                optionalNode<Block> body
        );

//        // @api
//        function updateMethodDeclaration(
//            node: MethodDeclaration,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            optionalNode<AsteriskToken> asteriskToken,
//            name: PropertyName,
//            optionalNode<QuestionToken> questionToken,
//            optionalNode<NodeArray> typeParameters,
//            node<NodeArray> parameters,
//            optionalNode<TypeNode> type,
//           optionalNode<Block> body
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
        node<ClassStaticBlockDeclaration> createClassStaticBlockDeclaration(
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                node<Block> body
        );
//
//        // @api
//        function updateClassStaticBlockDeclaration(
//            node: ClassStaticBlockDeclaration,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            node<Block> block
//        ): ClassStaticBlockDeclaration {
//            return node->decorators != decorators
//                || node->modifier != modifiers
//                || node->body != body
//                ? update(createClassStaticBlockDeclaration(decorators, modifiers, body), node)
//                : node;
//        }

        // @api
        node<ConstructorDeclaration> createConstructorDeclaration(
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                node<NodeArray> parameters,
                optionalNode<Block> body
        );

//        // @api
//        function updateConstructorDeclaration(
//            node: ConstructorDeclaration,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            node<NodeArray> parameters,
//           optionalNode<Block> body
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
        node<GetAccessorDeclaration> createGetAccessorDeclaration(
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                NameType name,
                node<NodeArray> parameters,
                optionalNode<TypeNode> type,
                optionalNode<Block> body
        );

//        // @api
//        function updateGetAccessorDeclaration(
//            node: GetAccessorDeclaration,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            name: PropertyName,
//            node<NodeArray> parameters,
//            optionalNode<TypeNode> type,
//           optionalNode<Block> body
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
        node<SetAccessorDeclaration> createSetAccessorDeclaration(
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                NameType name,
                node<NodeArray> parameters,
                optionalNode<Block> body
        );

//        // @api
//        function updateSetAccessorDeclaration(
//            node: SetAccessorDeclaration,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            name: PropertyName,
//            node<NodeArray> parameters,
//           optionalNode<Block> body
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
        node<CallSignatureDeclaration> createCallSignature(
                optionalNode<NodeTypeArray(TypeParameterDeclaration)> typeParameters,
                node<NodeTypeArray(ParameterDeclaration)> parameters,
                optionalNode<TypeNode> type
        );

//        // @api
//        function updateCallSignature(
//            node: CallSignatureDeclaration,
//            typeParameters: NodeArray<TypeParameterDeclaration> | undefined,
//            parameters: NodeArray<ParameterDeclaration>,
//            optionalNode<TypeNode> type
//        ) {
//            return node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                ? updateBaseSignatureDeclaration(createCallSignature(typeParameters, parameters, type), node)
//                : node;
//        }
//
        // @api
        node<ConstructSignatureDeclaration> createConstructSignature(
                optionalNode<NodeArray> typeParameters,
                node<NodeArray> parameters,
                optionalNode<TypeNode> type
        );
//
//        // @api
//        function updateConstructSignature(
//            node: ConstructSignatureDeclaration,
//            typeParameters: NodeArray<TypeParameterDeclaration> | undefined,
//            parameters: NodeArray<ParameterDeclaration>,
//            optionalNode<TypeNode> type
//        ) {
//            return node->typeParameters != typeParameters
//                || node->parameters != parameters
//                || node->type != type
//                ? updateBaseSignatureDeclaration(createConstructSignature(typeParameters, parameters, type), node)
//                : node;
//        }
//
        // @api
        node<IndexSignatureDeclaration> createIndexSignature(
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                node<NodeArray> parameters,
                optionalNode<TypeNode> type
        );

//        // @api
//        function updateIndexSignature(
//            node: IndexSignatureDeclaration,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            node<NodeArray> parameters,
//            node<TypeNode> type
//        ) {
//            return node->parameters != parameters
//                || node->type != type
//                || node->decorators != decorators
//                || node->modifiers != modifiers
//                ? updateBaseSignatureDeclaration(createIndexSignature(decorators, modifiers, parameters, type), node)
//                : node;
//        }

        // @api
        node<TemplateLiteralTypeSpan> createTemplateLiteralTypeSpan(node<TypeNode> type, node<NodeUnion(TemplateMiddle, TemplateTail)> literal);

//        // @api
//        function updateTemplateLiteralTypeSpan(node: TemplateLiteralTypeSpan, node<TypeNode> type, literal: TemplateMiddle | TemplateTail) {
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
        node<TypePredicateNode> createTypePredicateNode(optionalNode<AssertsKeyword> assertsModifier, NameType parameterName, optionalNode<TypeNode> type);

//        // @api
//        function updateTypePredicateNode(node: TypePredicateNode, assertsModifier: AssertsKeyword | undefined, parameterName: Identifier | ThisTypeNode, optionalNode<TypeNode> type) {
//            return node->assertsModifier != assertsModifier
//                || node->parameterName != parameterName
//                || node->type != type
//                ? update(createTypePredicateNode(assertsModifier, parameterName, type), node)
//                : node;
//        }

        // @api
        node<TypeReferenceNode> createTypeReferenceNode(NameType typeName, optionalNode<NodeArray> typeArguments);

//        // @api
//        function updateTypeReferenceNode(node: TypeReferenceNode, typeName: EntityName, typeArguments: NodeArray<TypeNode> | undefined) {
//            return node->typeName != typeName
//                || node->typeArguments != typeArguments
//                ? update(createTypeReferenceNode(typeName, typeArguments), node)
//                : node;
//        }

        // @api
        node<FunctionTypeNode> createFunctionTypeNode(
                optionalNode<NodeArray> typeParameters,
                node<NodeArray> parameters,
                optionalNode<TypeNode> type
        );

//        // @api
//        function updateFunctionTypeNode(
//            node: FunctionTypeNode,
//            typeParameters: NodeArray<TypeParameterDeclaration> | undefined,
//            parameters: NodeArray<ParameterDeclaration>,
//            optionalNode<TypeNode> type
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

        node<ConstructorTypeNode> createConstructorTypeNode1(
                optionalNode<NodeArray> modifiers,
                optionalNode<NodeArray> typeParameters,
                node<NodeArray> parameters,
                optionalNode<TypeNode> type
        );

//        /** @deprecated */
//        function createConstructorTypeNode2(
//            optionalNode<NodeArray> typeParameters,
//            node<NodeArray> parameters,
//            optionalNode<TypeNode> type
//        ): ConstructorTypeNode {
//            return createConstructorTypeNode1(/*modifiers*/ nullptr, typeParameters, parameters, type);
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
//            optionalNode<NodeArray> modifiers,
//            typeParameters: NodeArray<TypeParameterDeclaration> | undefined,
//            parameters: NodeArray<ParameterDeclaration>,
//            optionalNode<TypeNode> type
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
//            optionalNode<TypeNode> type
//        ) {
//            return updateConstructorTypeNode1(node, node->modifiers, typeParameters, parameters, type);
//        }
//
        // @api
        node<TypeQueryNode> createTypeQueryNode(node<NodeUnion(EntityName)> exprName, optionalNode<NodeArray> typeArguments);

//        // @api
//        function updateTypeQueryNode(node: TypeQueryNode, exprName: EntityName, typeArguments?: readonly TypeNode[]) {
//            return node->exprName != exprName
//                || node->typeArguments != typeArguments
//                ? update(createTypeQueryNode(exprName, typeArguments), node)
//                : node;
//        }
//
        // @api
        node<TypeLiteralNode> createTypeLiteralNode(optionalNode<NodeArray> members);
//
//        // @api
//        function updateTypeLiteralNode(node: TypeLiteralNode, members: NodeArray<TypeElement>) {
//            return node->members != members
//                ? update(createTypeLiteralNode(members), node)
//                : node;
//        }

        // @api
        node<ArrayTypeNode> createArrayTypeNode(node<TypeNode> elementType);

//        // @api
//        function updateArrayTypeNode(node: ArrayTypeNode, node<TypeNode> elementType): ArrayTypeNode {
//            return node->elementType != elementType
//                ? update(createArrayTypeNode(elementType), node)
//                : node;
//        }
//
        // @api
        node<TupleTypeNode> createTupleTypeNode(node<NodeArray> elements);

//        // @api
//        function updateTupleTypeNode(node: TupleTypeNode, elements: readonly (TypeNode | NamedTupleMember)[]) {
//            return node->elements != elements
//                ? update(createTupleTypeNode(elements), node)
//                : node;
//        }
//
        // @api
        node<NamedTupleMember> createNamedTupleMember(optionalNode<DotDotDotToken> dotDotDotToken, node<Identifier> name, optionalNode<QuestionToken> questionToken, node<TypeNode> type);

//        // @api
//        function updateNamedTupleMember(node: NamedTupleMember, dotDotDotToken: DotDotDotToken | undefined, name: Identifier, optionalNode<QuestionToken> questionToken, node<TypeNode> type) {
//            return node->dotDotDotToken != dotDotDotToken
//                || node->name != name
//                || node->questionToken != questionToken
//                || node->type != type
//                ? update(createNamedTupleMember(dotDotDotToken, name, questionToken, type), node)
//                : node;
//        }
//
        // @api
        node<OptionalTypeNode> createOptionalTypeNode(node<TypeNode> type) {
            auto node = createBaseNode<OptionalTypeNode>(SyntaxKind::OptionalType);
            node->type = parenthesizer.parenthesizeTypeOfOptionalType(type);
            node->transformFlags = (int)TransformFlags::ContainsTypeScript;
            return node;
        }

//        // @api
//        function updateOptionalTypeNode(node: OptionalTypeNode, node<TypeNode> type): OptionalTypeNode {
//            return node->type != type
//                ? update(createOptionalTypeNode(type), node)
//                : node;
//        }
//
        // @api
        node<RestTypeNode> createRestTypeNode(node<TypeNode> type);

//        // @api
//        function updateRestTypeNode(node: RestTypeNode, node<TypeNode> type): RestTypeNode {
//            return node->type != type
//                ? update(createRestTypeNode(type), node)
//                : node;
//        }

        template<class T>
        node<T> createUnionOrIntersectionTypeNode(SyntaxKind kind, node<NodeArray> types, function<node<NodeArray>(node<NodeArray>)> parenthesize) {
            auto node = createBaseNode<T>(kind);
            node->types = createNodeArray(parenthesize(types));
            node->transformFlags = (int) TransformFlags::ContainsTypeScript;
            return node;
        }

//        function updateUnionOrIntersectionTypeNode<T extends UnionOrIntersectionTypeNode>(node: T, types: NodeArray<TypeNode>, parenthesize: (nodes: readonly TypeNode[]) => readonly TypeNode[]): T {
//            return node->types != types
//                ? update(createUnionOrIntersectionTypeNode(node->kind, types, parenthesize) as T, node)
//                : node;
//        }

        // @api
        node<UnionTypeNode> createUnionTypeNode(node<NodeArray> types);

//        // @api
//        function updateUnionTypeNode(node: UnionTypeNode, types: NodeArray<TypeNode>) {
//            return updateUnionOrIntersectionTypeNode(node, types, parenthesizer.parenthesizeConstituentTypesOfUnionType);
//        }

        // @api
        node<IntersectionTypeNode> createIntersectionTypeNode(node<NodeArray> types);

//        // @api
//        function updateIntersectionTypeNode(node: IntersectionTypeNode, types: NodeArray<TypeNode>) {
//            return updateUnionOrIntersectionTypeNode(node, types, parenthesizer.parenthesizeConstituentTypesOfIntersectionType);
//        }

        // @api
        node<ConditionalTypeNode> createConditionalTypeNode(node<TypeNode> checkType, node<TypeNode> extendsType, node<TypeNode> trueType, node<TypeNode> falseType);

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
        node<InferTypeNode> createInferTypeNode(node<TypeParameterDeclaration> typeParameter);

//        // @api
//        function updateInferTypeNode(node: InferTypeNode, typeParameter: TypeParameterDeclaration) {
//            return node->typeParameter != typeParameter
//                ? update(createInferTypeNode(typeParameter), node)
//                : node;
//        }

        // @api
        node<TemplateLiteralTypeNode> createTemplateLiteralType(node<TemplateHead> head, node<NodeArray> templateSpans);

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
        node<ImportTypeNode> createImportTypeNode(
                node<TypeNode> argument,
                optional<variant<node<NodeUnion(EntityName)>, node<ImportTypeAssertionContainer>>> qualifierOrAssertions,
                optional<variant<node<NodeUnion(EntityName)>, node<NodeArray>>> typeArgumentsOrQualifier,
                optional<variant<bool, node<NodeArray>>> isTypeOfOrTypeArguments,
                optional<bool> isTypeOf
        );

//        // @api
//        function updateImportTypeNode(node: ImportTypeNode, argument: TypeNode, qualifier: EntityName | undefined, optionalNode<NodeArray> typeArguments, isTypeOf?: boolean | undefined): ImportTypeNode;
//        function updateImportTypeNode(node: ImportTypeNode, argument: TypeNode, assertions: ImportTypeAssertionContainer | undefined, qualifier: EntityName | undefined, optionalNode<NodeArray> typeArguments, isTypeOf?: boolean | undefined): ImportTypeNode;
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
        node<ParenthesizedTypeNode> createParenthesizedType(node<TypeNode> type);

//        // @api
//        function updateParenthesizedType(node: ParenthesizedTypeNode, node<TypeNode> type) {
//            return node->type != type
//                ? update(createParenthesizedType(type), node)
//                : node;
//        }

        // @api
        node<ThisTypeNode> createThisTypeNode();

        // @api
        node<TypeOperatorNode> createTypeOperatorNode(SyntaxKind operatorKind, node<TypeNode> type);

//        // @api
//        function updateTypeOperatorNode(node: TypeOperatorNode, node<TypeNode> type) {
//            return node->type != type
//                ? update(createTypeOperatorNode(node->operator, type), node)
//                : node;
//        }

        // @api
        node<IndexedAccessTypeNode> createIndexedAccessTypeNode(node<TypeNode> objectType, node<TypeNode> indexType);

//        // @api
//        function updateIndexedAccessTypeNode(node: IndexedAccessTypeNode, objectType: TypeNode, indexType: TypeNode) {
//            return node->objectType != objectType
//                || node->indexType != indexType
//                ? update(createIndexedAccessTypeNode(objectType, indexType), node)
//                : node;
//        }

        // @api
        node<MappedTypeNode> createMappedTypeNode(
                optionalNode<Node> readonlyToken, //: ReadonlyKeyword | PlusToken | MinusToken | undefined,
                node<TypeParameterDeclaration> typeParameter,
                optionalNode<TypeNode> nameType,
                optionalNode<Node> questionToken, //: QuestionToken | PlusToken | MinusToken | undefined,
                optionalNode<TypeNode> type,
                optionalNode<NodeArray> members
        );

//        // @api
//        function updateMappedTypeNode(node: MappedTypeNode, readonlyToken: ReadonlyKeyword | PlusToken | MinusToken | undefined, typeParameter: TypeParameterDeclaration, namesharedOpt<TypeNode> type, questionToken: QuestionToken | PlusToken | MinusToken | undefined, optionalNode<TypeNode> type, optionalNode<NodeArray> members): MappedTypeNode {
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
        node<LiteralTypeNode> createLiteralTypeNode(node<Expression> literal);

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
        node<ObjectBindingPattern> createObjectBindingPattern(node<NodeArray> elements);

//        // @api
//        function updateObjectBindingPattern(node: ObjectBindingPattern, elements: readonly BindingElement[]) {
//            return node->elements != elements
//                ? update(createObjectBindingPattern(elements), node)
//                : node;
//        }
//
        // @api
        node<ArrayBindingPattern> createArrayBindingPattern(node<NodeArray> elements);

//        // @api
//        function updateArrayBindingPattern(node: ArrayBindingPattern, elements: readonly ArrayBindingElement[]) {
//            return node->elements != elements
//                ? update(createArrayBindingPattern(elements), node)
//                : node;
//        }
//
        // @api
        node<BindingElement> createBindingElement(
                optionalNode<DotDotDotToken> dotDotDotToken,
                optional<variant<string, node<NodeUnion(PropertyName)>>> propertyName = {},
                variant<string, node<NodeUnion(BindingName)>> name = "",
                optionalNode<Expression> initializer = {}
        );

//        // @api
//        function updateBindingElement(node: BindingElement, dotDotDotToken: DotDotDotToken | undefined, propertyName: PropertyName | undefined, name: BindingName, optionalNode<Expression> initializer) {
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
        node<T> createBaseExpression(SyntaxKind kind) {
            ZoneScoped;
            auto node = createBaseNode<T>(kind);
            // the following properties are commonly set by the checker/binder
            return node;
        }

        // @api
        node<ArrayLiteralExpression> createArrayLiteralExpression(optionalNode<NodeArray> elements, bool multiLine);

//        // @api
//        function updateArrayLiteralExpression(node: ArrayLiteralExpression, elements: readonly Expression[]) {
//            return node->elements != elements
//                ? update(createArrayLiteralExpression(elements, node->multiLine), node)
//                : node;
//        }

        // @api
        node<ObjectLiteralExpression> createObjectLiteralExpression(optionalNode<NodeArray> properties, bool multiLine);

//        // @api
//        function updateObjectLiteralExpression(node: ObjectLiteralExpression, properties: readonly ObjectLiteralElementLike[]) {
//            return node->properties != properties
//                ? update(createObjectLiteralExpression(properties, node->multiLine), node)
//                : node;
//        }
//
        // @api
        node<PropertyAccessExpression> createPropertyAccessExpression(node<Expression> expression, NameType _name);

        // @api
        node<PropertyAccessExpression> createPropertyAccessChain(node<Expression> expression, optionalNode<QuestionDotToken> questionDotToken, NameType name);

//        // @api
//        function updatePropertyAccessExpression(node: PropertyAccessExpression, node<Expression> expression, name: Identifier | PrivateIdentifier) {
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
//        function updatePropertyAccessChain(node: PropertyAccessChain, node<Expression> expression, optionalNode<QuestionDotToken> questionDotToken, name: Identifier | PrivateIdentifier) {
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
        node<ElementAccessExpression> createElementAccessExpression(node<Expression> expression, ExpressionType index);
//
//        // @api
//        function updateElementAccessExpression(node: ElementAccessExpression, node<Expression> expression, argumentExpression: Expression) {
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
        node<ElementAccessExpression> createElementAccessChain(node<Expression> expression, optionalNode<QuestionDotToken> questionDotToken, ExpressionType index);

//        // @api
//        function updateElementAccessChain(node: ElementAccessChain, node<Expression> expression, optionalNode<QuestionDotToken> questionDotToken, argumentExpression: Expression) {
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
        node<CallExpression> createCallExpression(node<Expression> expression, optionalNode<NodeArray> typeArguments, optionalNode<NodeArray> argumentsArray);

        // @api
        node<CallChain> createCallChain(node<Expression> expression, optionalNode<QuestionDotToken> questionDotToken, optionalNode<NodeArray> typeArguments, optionalNode<NodeArray> argumentsArray);

        // @api
        node<CallChain> updateCallChain(node<CallChain> node, tr::node<Expression> expression, optionalNode<QuestionDotToken> questionDotToken, optionalNode<NodeArray> typeArguments, tr::node<NodeArray> argumentsArray);

        // @api
        node<CallExpression> updateCallExpression(node<CallExpression> node, tr::node<Expression> expression, optionalNode<NodeArray> typeArguments, tr::node<NodeArray> argumentsArray);

        // @api
        node<NewExpression> createNewExpression(node<Expression> expression, optionalNode<NodeArray> typeArguments, optionalNode<NodeArray> argumentsArray);

//        // @api
//        function updateNewExpression(node: NewExpression, node<Expression> expression, optionalNode<NodeArray> typeArguments, optionalNode<NodeArray> argumentsArray) {
//            return node->expression != expression
//                || node->typeArguments != typeArguments
//                || node->arguments != argumentsArray
//                ? update(createNewExpression(expression, typeArguments, argumentsArray), node)
//                : node;
//        }

        // @api
        node<TaggedTemplateExpression> createTaggedTemplateExpression(node<Expression> tag, optionalNode<NodeArray> typeArguments, node<NodeUnion(TemplateLiteralTypes)> templateLiteral);
//
//        // @api
//        function updateTaggedTemplateExpression(node: TaggedTemplateExpression, tag: Expression, optionalNode<NodeArray> typeArguments, template: TemplateLiteral) {
//            return node->tag != tag
//                || node->typeArguments != typeArguments
//                || node->template != template
//                ? update(createTaggedTemplateExpression(tag, typeArguments, template), node)
//                : node;
//        }

        // @api
        node<TypeAssertion> createTypeAssertion(node<TypeNode> type, node<Expression> expression);

        // @api
        node<TypeAssertion> updateTypeAssertion(node<TypeAssertion> node, tr::node<TypeNode> type, tr::node<Expression> expression);

// @api
        node<ParenthesizedExpression> createParenthesizedExpression(node<Expression> expression);

        // @api
        node<ParenthesizedExpression> updateParenthesizedExpression(node<ParenthesizedExpression> node, tr::node<Expression> expression);

        // @api
        node<FunctionExpression> createFunctionExpression(
                optionalNode<NodeArray> modifiers,
                optionalNode<AsteriskToken> asteriskToken,
                NameType name,
                optionalNode<NodeArray> typeParameters,
                optionalNode<NodeArray> parameters,
                optionalNode<TypeNode> type,
                node<Block> body
        );

//        // @api
//        function updateFunctionExpression(
//            node: FunctionExpression,
//            optionalNode<NodeArray> modifiers,
//            optionalNode<AsteriskToken> asteriskToken,
//            name: Identifier | undefined,
//            optionalNode<NodeArray> typeParameters,
//            node<NodeArray> parameters,
//            optionalNode<TypeNode> type,
//            node<Block> block
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
        node<ArrowFunction> createArrowFunction(
                optionalNode<NodeArray> modifiers,
                optionalNode<NodeArray> typeParameters,
                node<NodeArray> parameters,
                optionalNode<TypeNode> type,
                optionalNode<EqualsGreaterThanToken> equalsGreaterThanToken,
                node<NodeUnion(ConciseBody)> body
        );

//        // @api
//        function updateArrowFunction(
//            node: ArrowFunction,
//            optionalNode<NodeArray> modifiers,
//            optionalNode<NodeArray> typeParameters,
//            node<NodeArray> parameters,
//            optionalNode<TypeNode> type,
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
        node<DeleteExpression> createDeleteExpression(node<Expression> expression);

//        // @api
//        function updateDeleteExpression(node: DeleteExpression, node<Expression> expression) {
//            return node->expression != expression
//                ? update(createDeleteExpression(expression), node)
//                : node;
//        }

        // @api
        node<TypeOfExpression> createTypeOfExpression(node<Expression> expression);

//        // @api
//        function updateTypeOfExpression(node: TypeOfExpression, node<Expression> expression) {
//            return node->expression != expression
//                ? update(createTypeOfExpression(expression), node)
//                : node;
//        }

        // @api
        node<VoidExpression> createVoidExpression(node<Expression> expression);

//        // @api
//        function updateVoidExpression(node: VoidExpression, node<Expression> expression) {
//            return node->expression != expression
//                ? update(createVoidExpression(expression), node)
//                : node;
//        }

        // @api
       node<AwaitExpression> createAwaitExpression(node<Expression> expression);

//        // @api
//        function updateAwaitExpression(node: AwaitExpression, node<Expression> expression) {
//            return node->expression != expression
//                ? update(createAwaitExpression(expression), node)
//                : node;
//        }
//
// @api
        node<PrefixUnaryExpression> createPrefixUnaryExpression(SyntaxKind operatorKind, node<Expression> operand);

//        // @api
//        function updatePrefixUnaryExpression(node: PrefixUnaryExpression, operand: Expression) {
//            return node->operand != operand
//                ? update(createPrefixUnaryExpression(node->operator, operand), node)
//                : node;
//        }

        // @api
       node<PostfixUnaryExpression> createPostfixUnaryExpression(node<Expression> operand, SyntaxKind operatorKind);

//        // @api
//        function updatePostfixUnaryExpression(node: PostfixUnaryExpression, operand: Expression) {
//            return node->operand != operand
//                ? update(createPostfixUnaryExpression(operand, node->operator), node)
//                : node;
//        }
//
// @api
        node<BinaryExpression> createBinaryExpression(node<Expression> left, node<Node> operatorNode, node<Expression> right);

        TransformFlags propagateAssignmentPatternFlags(node<Node> node);
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
       node<ConditionalExpression> createConditionalExpression(node<Expression> condition, optionalNode<QuestionToken> questionToken, node<Expression> whenTrue, optionalNode<ColonToken> colonToken, node<Expression> whenFalse);

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
        node<TemplateExpression> createTemplateExpression(node<TemplateHead> head, node<NodeArray> templateSpans);

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
        node<YieldExpression> createYieldExpression(optionalNode<AsteriskToken> asteriskToken, optionalNode<Expression> expression);

//        // @api
//        function updateYieldExpression(node: YieldExpression, optionalNode<AsteriskToken> asteriskToken, node<Expression> expression) {
//            return node->expression != expression
//                || node->asteriskToken != asteriskToken
//                ? update(createYieldExpression(asteriskToken, expression), node)
//                : node;
//        }

        // @api
       node<SpreadElement> createSpreadElement(node<Expression> expression);

//        // @api
//        function updateSpreadElement(node: SpreadElement, node<Expression> expression) {
//            return node->expression != expression
//                ? update(createSpreadElement(expression), node)
//                : node;
//        }

        // @api
       node<ClassExpression> createClassExpression(
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                NameType name,
                optionalNode<NodeArray> typeParameters,
                optionalNode<NodeArray> heritageClauses,
                node<NodeArray> members
        );

//        // @api
//        function updateClassExpression(
//            node: ClassExpression,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            name: Identifier | undefined,
//            optionalNode<NodeArray> typeParameters,
//            optionalNode<NodeArray> heritageClauses,
//            node<NodeArray> members
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
        node<OmittedExpression> createOmittedExpression();

        // @api
        node<ExpressionWithTypeArguments> createExpressionWithTypeArguments(node<Expression> expression, optionalNode<NodeArray> typeArguments);

//        // @api
//        function updateExpressionWithTypeArguments(node: ExpressionWithTypeArguments, node<Expression> expression, optionalNode<NodeArray> typeArguments) {
//            return node->expression != expression
//                || node->typeArguments != typeArguments
//                ? update(createExpressionWithTypeArguments(expression, typeArguments), node)
//                : node;
//        }

        // @api
       node<AsExpression> createAsExpression(node<Expression> expression, node<TypeNode> type);

        // @api
        auto updateAsExpression(node<AsExpression> node, tr::node<Expression> expression, tr::node<TypeNode> type);

        // @api
        node<NonNullExpression> createNonNullExpression(node<Expression> expression);

        // @api
       node<NonNullChain> createNonNullChain(node<Expression> expression);

        // @api
        auto updateNonNullChain(node<NonNullChain> node, tr::node<Expression> expression);

        // @api
        auto updateNonNullExpression(node<NonNullExpression> node, tr::node<Expression> expression);

        // @api
       node<MetaProperty> createMetaProperty(SyntaxKind keywordToken, node<Identifier> name);

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
        node<TemplateSpan> createTemplateSpan(node<Expression> expression, node<NodeUnion(TemplateMiddle, TemplateTail)> literal);

//        // @api
//        function updateTemplateSpan(node: TemplateSpan, node<Expression> expression, literal: TemplateMiddle | TemplateTail) {
//            return node->expression != expression
//                || node->literal != literal
//                ? update(createTemplateSpan(expression, literal), node)
//                : node;
//        }
//
        // @api
       node<SemicolonClassElement> createSemicolonClassElement();

        //
        // Element
        //

        // @api
        node<Block> createBlock(node<NodeArray> statements, bool multiLine);

        // @api
       node<VariableDeclarationList> createVariableDeclarationList(node<NodeArray> declarations, int flags = (int) NodeFlags::None);

//        // @api
//        function updateBlock(node: Block, statements: readonly Statement[]) {
//            return node->statements != statements
//                ? update(createBlock(statements, node->multiLine), node)
//                : node;
//        }

        // @api
        node<VariableStatement> createVariableStatement(optionalNode<NodeArray> modifiers, node<VariableDeclarationList> declarationList);

//       node<VariableStatement> createVariableStatement(optionalNode<NodeArray> modifiers, variant<node<VariableDeclarationList>, vector<node<VariableDeclaration>>> declarationList);

//        // @api
//        function updateVariableStatement(node: VariableStatement, optionalNode<NodeArray> modifiers, declarationList: VariableDeclarationList) {
//            return node->modifiers != modifiers
//                || node->declarationList != declarationList
//                ? update(createVariableStatement(modifiers, declarationList), node)
//                : node;
//        }

        // @api
       node<EmptyStatement> createEmptyStatement();

        node<EmitNode> mergeEmitNode(node<EmitNode> sourceEmitNode, optionalNode<EmitNode> destEmitNode);

        template<class T>
        T setOriginalNode(T node, optionalNode<Node> original) {
            node->original = original;
            if (original) {
                auto emitNode = original->emitNode;
                if (emitNode) node->emitNode = mergeEmitNode(emitNode, node->emitNode);
            }
            return node;
        }

        template<class T>
        optionalNode<T> asEmbeddedStatement(optionalNode<T> statement) {
            return statement && isNotEmittedStatement(statement) ? setTextRange(setOriginalNode(createEmptyStatement(), statement), statement) : statement;
        }

//        // @api
//        function updateExpressionStatement(node: ExpressionStatement, node<Expression> expression) {
//            return node->expression != expression
//                ? update(createExpressionStatement(expression), node)
//                : node;
//        }
//
        // @api
        node<IfStatement> createIfStatement(node<Expression> expression, node<Statement> thenStatement, optionalNode<Statement> elseStatement = nullptr) {
            auto node = createBaseNode<IfStatement>(SyntaxKind::IfStatement);
            node->expression = expression;
            node->thenStatement = asEmbeddedStatement(thenStatement);
            node->elseStatement = asEmbeddedStatement(elseStatement);
            node->transformFlags |=
                propagateChildFlags(node->expression) |
                propagateChildFlags(node->thenStatement) |
                propagateChildFlags(node->elseStatement);
            return node;
        }

//        // @api
//        function updateIfStatement(node: IfStatement, node<Expression> expression, thenStatement: Statement, elseStatement: Statement | undefined) {
//            return node->expression != expression
//                || node->thenStatement != thenStatement
//                || node->elseStatement != elseStatement
//                ? update(createIfStatement(expression, thenStatement, elseStatement), node)
//                : node;
//        }
//
//        // @api
//        function createDoStatement(statement: Statement, node<Expression> expression) {
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
//        function updateDoStatement(node: DoStatement, statement: Statement, node<Expression> expression) {
//            return node->statement != statement
//                || node->expression != expression
//                ? update(createDoStatement(statement, expression), node)
//                : node;
//        }
//
//        // @api
//        function createWhileStatement(node<Expression> expression, statement: Statement) {
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
//        function updateWhileStatement(node: WhileStatement, node<Expression> expression, statement: Statement) {
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
//        function createForInStatement(initializer: ForInitializer, node<Expression> expression, statement: Statement) {
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
//        function updateForInStatement(node: ForInStatement, initializer: ForInitializer, node<Expression> expression, statement: Statement) {
//            return node->initializer != initializer
//                || node->expression != expression
//                || node->statement != statement
//                ? update(createForInStatement(initializer, expression, statement), node)
//                : node;
//        }
//
//        // @api
//        function createForOfStatement(awaitModifier: AwaitKeyword | undefined, initializer: ForInitializer, node<Expression> expression, statement: Statement) {
//            auto node = createBaseNode<ForOfStatement>(SyntaxKind::ForOfStatement);
//            node->awaitModifier = awaitModifier;
//            node->initializer = initializer;
//            node->expression = parenthesizer.parenthesizeExpressionForDisallowedComma(expression);
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
//        function updateForOfStatement(node: ForOfStatement, awaitModifier: AwaitKeyword | undefined, initializer: ForInitializer, node<Expression> expression, statement: Statement) {
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
        // @api
        node<ReturnStatement> createReturnStatement(optionalNode<Expression> expression) {
            auto node = createBaseNode<ReturnStatement>(SyntaxKind::ReturnStatement);
            node->expression = expression;
            // return in an ES2018 async generator must be awaited
            node->transformFlags |=
                propagateChildFlags(node->expression) |
                (int)TransformFlags::ContainsES2018 |
                (int)TransformFlags::ContainsHoistedDeclarationOrCompletion;
            return node;
        }

//        // @api
//        function updateReturnStatement(node: ReturnStatement, optionalNode<Expression> expression) {
//            return node->expression != expression
//                ? update(createReturnStatement(expression), node)
//                : node;
//        }
//
//        // @api
//        function createWithStatement(node<Expression> expression, statement: Statement) {
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
//        function updateWithStatement(node: WithStatement, node<Expression> expression, statement: Statement) {
//            return node->expression != expression
//                || node->statement != statement
//                ? update(createWithStatement(expression, statement), node)
//                : node;
//        }
//
//        // @api
//        function createSwitchStatement(node<Expression> expression, caseBlock: CaseBlock): SwitchStatement {
//            auto node = createBaseNode<SwitchStatement>(SyntaxKind::SwitchStatement);
//            node->expression = parenthesizer.parenthesizeExpressionForDisallowedComma(expression);
//            node->caseBlock = caseBlock;
//            node->transformFlags |=
//                propagateChildFlags(node->expression) |
//                propagateChildFlags(node->caseBlock);
//            return node;
//        }
//
//        // @api
//        function updateSwitchStatement(node: SwitchStatement, node<Expression> expression, caseBlock: CaseBlock) {
//            return node->expression != expression
//                || node->caseBlock != caseBlock
//                ? update(createSwitchStatement(expression, caseBlock), node)
//                : node;
//        }
//
        // @api
       node<LabeledStatement> createLabeledStatement(NameType label, node<Statement> statement);

        // @api
       node<ExpressionStatement> createExpressionStatement(node<Expression> expression);

//        // @api
//        function updateLabeledStatement(node: LabeledStatement, label: Identifier, statement: Statement) {
//            return node->label != label
//                || node->statement != statement
//                ? update(createLabeledStatement(label, statement), node)
//                : node;
//        }
//
//        // @api
//        function createThrowStatement(node<Expression> expression) {
//            auto node = createBaseNode<ThrowStatement>(SyntaxKind::ThrowStatement);
//            node->expression = expression;
//            node->transformFlags |= propagateChildFlags(node->expression);
//            return node;
//        }
//
//        // @api
//        function updateThrowStatement(node: ThrowStatement, node<Expression> expression) {
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
        // @api
       node<VariableDeclaration> createVariableDeclaration(NameType name, optionalNode<ExclamationToken> exclamationToken, optionalNode<TypeNode> type, optionalNode<Expression> initializer);

//        // @api
//        function updateVariableDeclaration(node: VariableDeclaration, name: BindingName, exclamationToken: ExclamationToken | undefined, optionalNode<TypeNode> type, optionalNode<Expression> initializer) {
//            return node->name != name
//                || node->type != type
//                || node->exclamationToken != exclamationToken
//                || node->initializer != initializer
//                ? update(createVariableDeclaration(name, exclamationToken, type, initializer), node)
//                : node;
//        }
//
//
//        // @api
//        function updateVariableDeclarationList(node: VariableDeclarationList, declarations: readonly VariableDeclaration[]) {
//            return node->declarations != declarations
//                ? update(createVariableDeclarationList(declarations, node->flags), node)
//                : node;
//        }

        // @api
       node<FunctionDeclaration> createFunctionDeclaration(
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                optionalNode<AsteriskToken> asteriskToken,
                NameType name,
                optionalNode<NodeArray> typeParameters,
                node<NodeArray> parameters,
                optionalNode<TypeNode> type,
                optionalNode<Block> body
        );

//        // @api
//        function updateFunctionDeclaration(
//            node: FunctionDeclaration,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            optionalNode<AsteriskToken> asteriskToken,
//            name: Identifier | undefined,
//            optionalNode<NodeArray> typeParameters,
//            node<NodeArray> parameters,
//            optionalNode<TypeNode> type,
//           optionalNode<Block> body
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
       node<ClassDeclaration> createClassDeclaration(
                optionalNode<NodeArray> decorators,
                optionalNode<NodeArray> modifiers,
                NameType name,
                optionalNode<NodeArray> typeParameters,
                optionalNode<NodeArray> heritageClauses,
                node<NodeArray> members
        );

//        // @api
//        function updateClassDeclaration(
//            node: ClassDeclaration,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            name: Identifier | undefined,
//            optionalNode<NodeArray> typeParameters,
//            optionalNode<NodeArray> heritageClauses,
//            node<NodeArray> members
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            NameType name,
//            optionalNode<NodeArray> typeParameters,
//            optionalNode<NodeArray> heritageClauses,
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            name: Identifier,
//            optionalNode<NodeArray> typeParameters,
//            optionalNode<NodeArray> heritageClauses,
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
        // @api
        node<TypeAliasDeclaration> createTypeAliasDeclaration(
            optionalNode<NodeArray> decorators,
            optionalNode<NodeArray> modifiers,
            NameType name,
            optionalNode<NodeArray> typeParameters,
            node<TypeNode> type
        ) {
            auto node = createBaseGenericNamedDeclaration<TypeAliasDeclaration>(
                SyntaxKind::TypeAliasDeclaration,
                decorators,
                modifiers,
                name,
                typeParameters
            );
            node->type = type;
            node->transformFlags = (int)TransformFlags::ContainsTypeScript;
            return node;
        }

//        // @api
//        function updateTypeAliasDeclaration(
//            node: TypeAliasDeclaration,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            name: Identifier,
//            optionalNode<NodeArray> typeParameters,
//            node<TypeNode> type
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            NameType name,
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
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
//        function createNamespaceExportDeclaration(NameType name) {
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            isTypeOnly: boolean,
//            NameType name,
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
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
       node<AssertClause> createAssertClause(node<NodeArray> elements, bool multiLine);

//        // @api
//        function updateAssertClause(node: AssertClause, elements: readonly AssertEntry[], multiLine?: boolean): AssertClause {
//            return node->elements != elements
//                || node->multiLine != multiLine
//                ? update(createAssertClause(elements, multiLine), node)
//                : node;
//        }
//
        // @api
       node<AssertEntry> createAssertEntry(node<NodeUnion(AssertionKey)> name, node<Expression> value);

//        // @api
//        function updateAssertEntry(node: AssertEntry, name: AssertionKey, value: Expression): AssertEntry {
//            return node->name != name
//                || node->value != value
//                ? update(createAssertEntry(name, value), node)
//                : node;
//        }

        // @api
       node<ImportTypeAssertionContainer> createImportTypeAssertionContainer(node<AssertClause> clause, bool multiLine);

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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            isExportEquals: boolean | undefined,
//            node<Expression> expression
//        ) {
//            auto node = createBaseDeclaration<ExportAssignment>(
//                SyntaxKind::ExportAssignment,
//                decorators,
//                modifiers
//            );
//            node->isExportEquals = isExportEquals;
//            node->expression = isExportEquals
//                ? parenthesizer.parenthesizeRightSideOfBinary(SyntaxKind::EqualsToken, /*leftSide*/ {}, expression)
//                : parenthesizer.parenthesizeExpressionOfExportDefault(expression);
//            node->transformFlags |= propagateChildFlags(node->expression);
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateExportAssignment(
//            node: ExportAssignment,
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
//            node<Expression> expression
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
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
//            optionalNode<NodeArray> decorators,
//            optionalNode<NodeArray> modifiers,
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
//        function createExportSpecifier(isTypeOnly: boolean, NameType propertyName, NameType name) {
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
        node<MissingDeclaration> createMissingDeclaration();

//        //
//        // Module references
//        //
//
//        // @api
//        function createExternalModuleReference(node<Expression> expression) {
//            auto node = createBaseNode<ExternalModuleReference>(SyntaxKind::ExternalModuleReference);
//            node->expression = expression;
//            node->transformFlags |= propagateChildFlags(node->expression);
//            node->transformFlags &= ~TransformFlags::ContainsPossibleTopLevelAwait; // always parsed in an Await context
//            return node;
//        }
//
//        // @api
//        function updateExternalModuleReference(node: ExternalModuleReference, node<Expression> expression) {
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
        // @api
        node<JSDocNonNullableType> createJSDocNonNullableType(node<TypeNode> type, bool postfix = false) {
            auto node = createBaseNode<JSDocNonNullableType>();
            node->type = postfix ? type ? parenthesizer.parenthesizeNonArrayTypeOfPostfixType(type) : nullptr : type;
            node->postfix = postfix;
            return node;
        }

        node<JSDocNullableType> createJSDocNullableType(node<TypeNode> type, bool postfix = false) {
            auto node = createBaseNode<JSDocNullableType>();
            node->type = postfix ? type ? parenthesizer.parenthesizeNonArrayTypeOfPostfixType(type) : nullptr : type;
            node->postfix = postfix;
            return node;
        }

        // @api
        // createJSDocOptionalType
        // createJSDocVariadicType
        // createJSDocNamepathType
//        node<Node> createJSDocUnaryTypeWorker(SyntaxKind kind, node<TypeNode> type) {
//            auto node = createBaseNode<T>(kind);
//            node->type = type;
//            return node;
//        }
//
//        // @api
//        // updateJSDocNonNullableType
//        // updateJSDocNullableType
//        function updateJSDocPrePostfixUnaryTypeWorker<T extends JSDocType & { readonly optionalNode<TypeNode> type; readonly postfix: boolean; }>(SyntaxKind kind, node: T, type: T["type"]): T {
//            return node->type != type
//            ? update(createJSDocPrePostfixUnaryTypeWorker(kind, type, node->postfix), node)
//            : node;
//        }
//
//        // @api
//        // updateJSDocOptionalType
//        // updateJSDocVariadicType
//        // updateJSDocNamepathType
//        function updateJSDocUnaryTypeWorker<T extends JSDocType & { readonly optionalNode<TypeNode> type; }>(SyntaxKind kind, node: T, type: T["type"]): T {
//            return node->type != type
//                ? update(createJSDocUnaryTypeWorker(kind, type), node)
//                : node;
//        }
//
//        // @api
//        function createJSDocFunctionType(node<NodeArray> parameters, optionalNode<TypeNode> type): JSDocFunctionType {
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
//        function updateJSDocFunctionType(node: JSDocFunctionType, node<NodeArray> parameters, optionalNode<TypeNode> type): JSDocFunctionType {
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
//        function createJSDocTypeExpression(node<TypeNode> type): JSDocTypeExpression {
//            auto node = createBaseNode<JSDocTypeExpression>(SyntaxKind::JSDocTypeExpression);
//            node->type = type;
//            return node;
//        }
//
//        // @api
//        function updateJSDocTypeExpression(node: JSDocTypeExpression, node<TypeNode> type): JSDocTypeExpression {
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
        node<JsxElement> createJsxElement(node<JsxOpeningElement> openingElement, node<NodeArray> children, node<JsxClosingElement> closingElement);

//        // @api
//        function updateJsxElement(node: JsxElement, openingElement: JsxOpeningElement, children: readonly JsxChild[], closingElement: JsxClosingElement) {
//            return node->openingElement != openingElement
//                || node->children != children
//                || node->closingElement != closingElement
//                ? update(createJsxElement(openingElement, children, closingElement), node)
//                : node;
//        }

// @api
        node<JsxSelfClosingElement> createJsxSelfClosingElement(node<NodeUnion(JsxTagNameExpression)> tagName, optionalNode<NodeArray> typeArguments, node<JsxAttributes> attributes);
//
//        // @api
//        function updateJsxSelfClosingElement(node: JsxSelfClosingElement, tagName: JsxTagNameExpression, optionalNode<NodeArray> typeArguments, attributes: JsxAttributes) {
//            return node->tagName != tagName
//                || node->typeArguments != typeArguments
//                || node->attributes != attributes
//                ? update(createJsxSelfClosingElement(tagName, typeArguments, attributes), node)
//                : node;
//        }
//
// @api
        node<JsxOpeningElement> createJsxOpeningElement(node<NodeUnion(JsxTagNameExpression)> tagName, optionalNode<NodeArray> typeArguments, node<JsxAttributes> attributes);
//
//        // @api
//        function updateJsxOpeningElement(node: JsxOpeningElement, tagName: JsxTagNameExpression, optionalNode<NodeArray> typeArguments, attributes: JsxAttributes) {
//            return node->tagName != tagName
//                || node->typeArguments != typeArguments
//                || node->attributes != attributes
//                ? update(createJsxOpeningElement(tagName, typeArguments, attributes), node)
//                : node;
//        }
//
// @api
        node<JsxClosingElement> createJsxClosingElement(node<NodeUnion(JsxTagNameExpression)> tagName);

//        // @api
//        function updateJsxClosingElement(node: JsxClosingElement, tagName: JsxTagNameExpression) {
//            return node->tagName != tagName
//                ? update(createJsxClosingElement(tagName), node)
//                : node;
//        }

// @api
        node<JsxFragment> createJsxFragment(node<JsxOpeningFragment> openingFragment, node<NodeArray> children, node<JsxClosingFragment> closingFragment);

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
        node<JsxOpeningFragment> createJsxOpeningFragment();

// @api
        node<JsxClosingFragment> createJsxJsxClosingFragment();

// @api
        node<JsxAttribute> createJsxAttribute(node<Identifier> name, optionalNode<NodeUnion(JsxAttributeValue)> initializer = {});

//        // @api
//        function updateJsxAttribute(node: JsxAttribute, name: Identifier, initializer: JsxAttributeValue | undefined) {
//            return node->name != name
//                || node->initializer != initializer
//                ? update(createJsxAttribute(name, initializer), node)
//                : node;
//        }
//
// @api
        node<JsxAttributes> createJsxAttributes(node<NodeArray> properties);
//
//        // @api
//        function updateJsxAttributes(node: JsxAttributes, properties: readonly JsxAttributeLike[]) {
//            return node->properties != properties
//                ? update(createJsxAttributes(properties), node)
//                : node;
//        }

// @api
        node<JsxSpreadAttribute> createJsxSpreadAttribute(node<Expression> expression);

//        // @api
//        function updateJsxSpreadAttribute(node: JsxSpreadAttribute, node<Expression> expression) {
//            return node->expression != expression
//                ? update(createJsxSpreadAttribute(expression), node)
//                : node;
//        }
//
// @api
        node<JsxExpression> createJsxExpression(optionalNode<DotDotDotToken> dotDotDotToken, optionalNode<Expression> expression);

//        // @api
//        function updateJsxExpression(node: JsxExpression, optionalNode<Expression> expression) {
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
//        function createCaseClause(node<Expression> expression, statements: readonly Statement[]) {
//            auto node = createBaseNode<CaseClause>(SyntaxKind::CaseClause);
//            node->expression = parenthesizer.parenthesizeExpressionForDisallowedComma(expression);
//            node->statements = createNodeArray(statements);
//            node->transformFlags |=
//                propagateChildFlags(node->expression) |
//                propagateChildrenFlags(node->statements);
//            return node;
//        }
//
//        // @api
//        function updateCaseClause(node: CaseClause, node<Expression> expression, statements: readonly Statement[]) {
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
       node<HeritageClause> createHeritageClause(SyntaxKind token, node<NodeArray> types);

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
       node<PropertyAssignment> createPropertyAssignment(NameType name, node<Expression> initializer);

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
       node<ShorthandPropertyAssignment> createShorthandPropertyAssignment(NameType name, optionalNode<Expression> objectAssignmentInitializer);
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
       node<SpreadAssignment> createSpreadAssignment(node<Expression> expression);

//        // @api
//        function updateSpreadAssignment(node: SpreadAssignment, node<Expression> expression) {
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
//            node->initializer = initializer && parenthesizer.parenthesizeExpressionForDisallowedComma(initializer);
//            node->transformFlags |=
//                propagateChildFlags(node->name) |
//                propagateChildFlags(node->initializer) |
//                (int)TransformFlags::ContainsTypeScript;
//            return node;
//        }
//
//        // @api
//        function updateEnumMember(node: EnumMember, name: PropertyName, optionalNode<Expression> initializer) {
//            return node->name != name
//                || node->initializer != initializer
//                ? update(createEnumMember(name, initializer), node)
//                : node;
//        }

        // @api
        shared_ptr<SourceFile> createSourceFile(
            node<NodeArray> statements,
            node<EndOfFileToken> endOfFileToken,
            int flags
        ) {
            auto node = make_shared<SourceFile>();
            node->statements = createNodeArray(statements);
            node->endOfFileToken = endOfFileToken;
            node->flags |= (int)flags;
            node->fileName = "";
            node->text = "";
            node->languageVersion = types::ScriptTarget::ES3;
            node->languageVariant = types::LanguageVariant::Standard;
            node->scriptKind = types::ScriptKind::Unknown;
            node->isDeclarationFile = false;
            node->hasNoDefaultLib = false;
            node->transformFlags |=
                propagateChildrenFlags(node->statements) |
                propagateChildFlags(node->endOfFileToken);
            return node;
        }

//        function cloneSourceFileWithChanges(
//            source: SourceFile,
//            statements: readonly Statement[],
//            isDeclarationFile: boolean,
//            referencedFiles: readonly FileReference[],
//            typeReferences: readonly FileReference[],
//            hasNoDefaultLib: boolean,
//            libReferences: readonly FileReference[]
//        ) {
//            auto node = (source.redirectInfo ? Object.create(source.redirectInfo.redirectTarget) : basecreateBaseSourceFileNode(SyntaxKind::SourceFile)) as Mutable<SourceFile>;
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
       node<PartiallyEmittedExpression> createPartiallyEmittedExpression(node<Expression> expression, optionalNode<Node> original);

        // @api
        auto updatePartiallyEmittedExpression(node<PartiallyEmittedExpression> node, tr::node<Expression> expression);

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
//        function createSyntheticReferenceExpression(node<Expression> expression, thisArg: Expression) {
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
//        function updateSyntheticReferenceExpression(node: SyntheticReferenceExpression, node<Expression> expression, thisArg: Expression) {
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
//                isSourceFile(node) ? basecreateBaseSourceFileNode(SyntaxKind::SourceFile) as T :
//                isIdentifier(node) ? basecreateBaseIdentifierNode(SyntaxKind::Identifier) as T :
//                isPrivateIdentifier(node) ? basecreateBasePrivateIdentifierNode(SyntaxKind::PrivateIdentifier) as T :
//                !isNodeKind(node->kind) ? basecreateBaseTokenNode(node->kind) as T :
//                basecreateBaseNode(node->kind) as T;
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
//        function createExportDefault(node<Expression> expression) {
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
//                ? createStrictEquality(value, createVoidZero())
//                : createStrictEquality(createTypeOfExpression(value), createStringLiteral(tag));
//        }
//
//        function createMethodCall(object: Expression, methodNameType name, argumentsList: readonly Expression[]) {
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
//        function tryAddPropertyAssignment(properties: Push<PropertyAssignment>, propertyName: string, optionalNode<Expression> expression) {
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

        node<Expression> updateOuterExpression(node<OuterExpression> outerExpression, node<Expression> expression);

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
        bool isIgnorableParen(node<Expression> node);

        node<Expression> restoreOuterExpressions(optionalNode<Expression> outerExpression, node<Expression> innerExpression, int kinds = (int)OuterExpressionKinds::All);

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
//        function createCallBinding(node<Expression> expression, recordTempVariable: (temp: Identifier) => void, languageVersion?: ScriptTarget, cacheIdentifiers = false): CallBinding {
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
//                target = parenthesizer.parenthesizeLeftSideOfAccess(callee);
//            }
//            else if (isPropertyAccessExpression(callee)) {
//                if (shouldBeCapturedInTempVariable(callee.expression, cacheIdentifiers)) {
//                    // for `a.b()` target is `(_a = a).b` and thisArg is `_a`
//                    thisArg = createTempVariable(recordTempVariable);
//                    target = createPropertyAccessExpression(
//                        setTextRange(
//                            createAssignment(
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
//                            createAssignment(
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
//                target = parenthesizer.parenthesizeLeftSideOfAccess(expression);
//            }
//
//            return { target, thisArg };
//        }
//
//        function createAssignmentTargetWrapper(paramName: Identifier, node<Expression> expression): LeftHandSideExpression {
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
//                : reduceLeft(expressions, createComma)!;
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
//         * Lifts a node<NodeArray> containing only Statement nodes to a block.
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
        TransformFlags getTransformFlagsSubtreeExclusions(SyntaxKind kind);
//
//    auto baseFactory = createBaseNodeFactory();
//
//    function makeSynthetic(node: Node) {
//        (node as Mutable<Node>).flags |= NodeFlags::Synthesized;
//        return node;
//    }
//
//    auto syntheticFactory: BaseNodeFactory = {
//        createBaseSourceFileNode: kind => makeSynthetic(basecreateBaseSourceFileNode(kind)),
//        createBaseIdentifierNode: kind => makeSynthetic(basecreateBaseIdentifierNode(kind)),
//        createBasePrivateIdentifierNode: kind => makeSynthetic(basecreateBasePrivateIdentifierNode(kind)),
//        createBaseTokenNode: kind => makeSynthetic(basecreateBaseTokenNode(kind)),
//        createBaseNode: kind => makeSynthetic(basecreateBaseNode(kind)),
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
//                    prologues = append(prologues, setTextRange(createUnparsedPrologue(section.data), section));
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
//                            prependTexts = append(prependTexts, setTextRange(createUnparsedTextLike(text.data, text.kind == BundleFileSectionKind.Internal), text));
//                        }
//                    }
//                    prependChildren = addRange(prependChildren, prependTexts);
//                    texts = append(texts, createUnparsedPrepend(section.data, prependTexts ?? emptyArray));
//                    break;
//                case BundleFileSectionKind.Internal:
//                    if (stripInternal) {
//                        if (!texts) texts = [];
//                        break;
//                    }
//                    // falls through
//
//                case BundleFileSectionKind.Text:
//                    texts = append(texts, setTextRange(createUnparsedTextLike(section.data, section.kind == BundleFileSectionKind.Internal), section));
//                    break;
//                default:
//                    Debug.assertNever(section);
//            }
//        }
//
//        if (!texts) {
//            auto textNode = createUnparsedTextLike(/*data*/ {}, /*internal*/ false);
//            setTextRangePosWidth(textNode, 0, typeof length == "function" ? length() : length);
//            texts = [textNode];
//        }
//
//        auto node = parseNodecreateUnparsedSource(prologues ?? emptyArray, /*syntheticReferences*/ {}, texts);
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
//                    texts = append(texts, setTextRange(createUnparsedTextLike(section.data, section.kind == BundleFileSectionKind.Internal), section));
//                    break;
//
//                case BundleFileSectionKind.NoDefaultLib:
//                case BundleFileSectionKind.Reference:
//                case BundleFileSectionKind.Type:
//                case BundleFileSectionKind.TypeResolutionModeImport:
//                case BundleFileSectionKind.TypeResolutionModeRequire:
//                case BundleFileSectionKind.Lib:
//                    syntheticReferences = append(syntheticReferences, setTextRange(createUnparsedSyntheticReference(section), section));
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
//        auto node = createUnparsedSource(emptyArray, syntheticReferences, texts ?? emptyArray);
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
//        auto node = parseNodecreateInputFiles();
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
    };
}
