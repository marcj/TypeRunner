#pragma once

#include "Tracy.hpp"
#include <functional>
#include <variant>
#include <optional>
#include <unordered_map>
#include <set>
#include <string>
#include <vector>
#include "types.h"
#include "core.h"
#include "path.h"
#include "scanner.h"
#include "node_test.h"
#include "factory.h"
#include "utilities.h"
#include "diagnostic_messages.h"
#include <fmt/core.h>

using namespace ts::types;

namespace ts {
    using std::string;
    using std::vector;
    using std::function;
    using std::optional;
    using std::variant;

    enum class SignatureFlags {
        None = 0,
        Yield = 1 << 0,
        Await = 1 << 1,
        Type = 1 << 2,
        IgnoreMissingOpenBrace = 1 << 4,
        JSDoc = 1 << 5,
    };

    enum class SpeculationKind {
        TryParse,
        Lookahead,
        Reparse
    };

    enum class ParsingContext {
        SourceElements,            // Elements in source file
        BlockStatements,           // Statements in block
        SwitchClauses,             // Clauses in switch statement
        SwitchClauseStatements,    // Statements in switch clause
        TypeMembers,               // Members in interface or type literal
        ClassMembers,              // Members in class declaration
        EnumMembers,               // Members in enum declaration
        HeritageClauseElement,     // Elements in a heritage clause
        VariableDeclarations,      // Variable declarations in variable statement
        ObjectBindingElements,     // Binding elements in object binding list
        ArrayBindingElements,      // Binding elements in array binding list
        ArgumentExpressions,       // Expressions in argument list
        ObjectLiteralMembers,      // Members in object literal
        JsxAttributes,             // Attributes in jsx element
        JsxChildren,               // Things between opening and closing JSX tags
        ArrayLiteralMembers,       // Members in array literal
        Parameters,                // Parameters in parameter list
        JSDocParameters,           // JSDoc parameters in parameter list of JSDoc function type
        RestProperties,            // Property names in a rest type list
        TypeParameters,            // Type parameters in type parameter list
        TypeArguments,             // Type arguments in type argument list
        TupleElementTypes,         // Element types in tuple element type list
        HeritageClauses,           // Heritage clauses for a class or interface declaration.
        ImportOrExportSpecifiers,  // Named import clause's import specifier list,
        AssertEntries,               // Import entries list.
        Count                      // Number of parsing contexts
    };

    enum class Tristate {
        False,
        True,
        Unknown
    };


////    auto NodeConstructor: new (kind: SyntaxKind, pos?: number, end?: number) => Node;
////    auto TokenConstructor: new (kind: SyntaxKind, pos?: number, end?: number) => Node;
////    auto IdentifierConstructor: new (kind: SyntaxKind, pos?: number, end?: number) => Node;
////    auto PrivateIdentifierConstructor: new (kind: SyntaxKind, pos?: number, end?: number) => Node;
////    auto SourceFileConstructor: new (kind: SyntaxKind, pos?: number, end?: number) => Node;
//
//    /**
//     * NOTE: You should not use this, it is only exported to support `createNode` in `~/src/deprecatedCompat/deprecations.ts`.
//     */
////    /* @internal */
////    export const parseBaseNodeFactory: BaseNodeFactory = {
////        createBaseSourceFileNode: kind => new (SourceFileConstructor || (SourceFileConstructor = objectAllocator.getSourceFileConstructor()))(kind, -1, -1),
////        createBaseIdentifierNode: kind => new (IdentifierConstructor || (IdentifierConstructor = objectAllocator.getIdentifierConstructor()))(kind, -1, -1),
////        createBasePrivateIdentifierNode: kind => new (PrivateIdentifierConstructor || (PrivateIdentifierConstructor = objectAllocator.getPrivateIdentifierConstructor()))(kind, -1, -1),
////        createBaseTokenNode: kind => new (TokenConstructor || (TokenConstructor = objectAllocator.getTokenConstructor()))(kind, -1, -1),
////        createBaseNode: kind => new (NodeConstructor || (NodeConstructor = objectAllocator.getNodeConstructor()))(kind, -1, -1),
////    };
////
////    /* @internal */
////    export const parseNodeFactory = createNodeFactory(NodeFactoryFlags.NoParenthesizerRules, parseBaseNodeFactory);

    inline sharedOpt<Node> visitNode(const function<sharedOpt<Node>(shared<Node>)> &cbNode, const sharedOpt<Node> &node) {
        if (node) cbNode(node);
        return nullptr;
    }

//    Node *visitNode(function<Node*(Node &)> cbNode, Node &node) {
//        return cbNode(node);
//    }

    inline sharedOpt<Node> noop(const shared<NodeArray> &) {
        return nullptr;
    };

    inline sharedOpt<Node> forEachChild(const shared<Node> &node, const function<sharedOpt<Node>(shared<Node>)> &cbNode, const function<sharedOpt<Node>(shared<NodeArray>)> &cbNodes = noop);

    inline sharedOpt<Node> visitNodes(
            const function<sharedOpt<Node>(shared<Node>)> &cbNode,
            const function<sharedOpt<Node>(shared<NodeArray>)> &cbNodes = noop,
            const sharedOpt<NodeArray> &nodes = nullptr
    ) {
        if (nodes) {
            if (cbNodes) {
                return cbNodes(nodes);
            }
            for (auto &&node: nodes->list) {
                auto result = cbNode(node);
                if (result) {
                    return result;
                }
            }
        }
        return nullptr;
    }

    /**
     * Iterates through 'array' by index and performs the callback on each element of array until the callback
     * returns a truthy value, then returns that value.
     * If no such value is found, the callback is applied to each element of array and undefined is returned.
     */
    inline sharedOpt<Node> forEach(const sharedOpt<NodeArray> &array, const function<sharedOpt<Node>(shared<Node> element, int index)> &callback) {
        if (array) {
            for (int i = 0; i < array->list.size(); i++) {
                auto result = callback(array->list[i], i);
                if (result) {
                    return result;
                }
            }
        }
        return nullptr;
    }

//    /**
//     * Iterates through 'array' by index and performs the callback on each element of array until the callback
//     * returns a truthy value, then returns that value.
//     * If no such value is found, the callback is applied to each element of array and undefined is returned.
//     */
//    bool forEach(sharedOpt<NodeArray>  array, function<bool(shared<Node> element, int index)> callback) {
//        if (array) {
//            for (int i = 0; i < array->list.size(); i ++) {
//                auto result = callback((*array).list[i], i);
//                if (result) {
//                    return result;
//                }
//            }
//        }
//        return false;
//    }

//    inline bool some(const sharedOpt<NodeArray> &array) {
//        if (array) {
//            return array->list.size() > 0;
//        }
//        return false;
//    }

    inline bool some(const sharedOpt<NodeArray> &array, const function<bool(shared<Node> value)> &predicate) {
        if (array) {
            for (auto &&v: array->list) {
                if (predicate(v)) {
                    return true;
                }
            }
        }
        return false;
    }

    /** Do not use hasModifier inside the parser; it relies on parent pointers. Use this instead. */
    inline bool hasModifierOfKind(const shared<Node> &node, SyntaxKind kind) {
        return some<Node>(node->modifiers, [kind](auto m) { return m->kind == kind; });
    }

    inline sharedOpt<Node> isAnExternalModuleIndicatorNode(shared<Node> node, int) {
        if (hasModifierOfKind(node, SyntaxKind::ExportKeyword)
            || (node->is<ImportEqualsDeclaration>() && isExternalModuleReference(node->to<ImportEqualsDeclaration>().moduleReference))
            || isImportDeclaration(node)
            || isExportAssignment(node)
            || isExportDeclaration(node))
            return node;
        return nullptr;
    }

    inline bool isImportMeta(const shared<Node> &node) {
        if (isMetaProperty(node)) {
            auto meta = node->to<MetaProperty>();
            return meta.keywordToken == SyntaxKind::ImportKeyword && meta.name->to<Identifier>().escapedText == "meta";
        }
        return false;
    }

    inline sharedOpt<Node> walkTreeForImportMeta(const shared<Node> &node) {
        if (isImportMeta(node)) return node;
        return forEachChild(node, walkTreeForImportMeta);
    }

    inline sharedOpt<Node> getImportMetaIfNecessary(const shared<SourceFile> &sourceFile) {
        return sourceFile->flags & (int) NodeFlags::PossiblyContainsImportMeta ?
               walkTreeForImportMeta(sourceFile) :
               nullptr;
    }

    /*@internal*/
    inline sharedOpt<Node> isFileProbablyExternalModule(const shared<SourceFile> &sourceFile) {
        // Try to use the first top-level import/export when available, then
        // fall back to looking for an 'import.meta' somewhere in the tree if necessary.
        auto r = forEach(sourceFile->statements, isAnExternalModuleIndicatorNode);
        if (r) return r;

        return getImportMetaIfNecessary(sourceFile);
    }
    /**
     * Invokes a callback for each child of the given node. The 'cbNode' callback is invoked for all child nodes
     * stored in properties. If a 'cbNodes' callback is specified, it is invoked for embedded arrays; otherwise,
     * embedded arrays are flattened and the 'cbNode' callback is invoked for each element. If a callback returns
     * a truthy value, iteration stops and that value is returned. Otherwise, undefined is returned.
     *
     * @param node a given node to visit its children
     * @param cbNode a callback to be invoked for all child nodes
     * @param cbNodes a callback to be invoked for embedded array
     *
     * @remarks `forEachChild` must visit the children of a node in the order
     * that they appear in the source code. The language service depends on this property to locate nodes by position.
     */
    inline sharedOpt<Node> forEachChild(const shared<Node> &node, const function<sharedOpt<Node>(shared<Node>)> &cbNode, const function<sharedOpt<Node>(shared<NodeArray>)> &cbNodes) {
        if (node->kind <= SyntaxKind::LastToken) {
            return nullptr;
        }
        switch (node->kind) {
            case SyntaxKind::QualifiedName:
                //short-circuit evaluation is always boolean ... not last value in c++
                return visitNode(cbNode, node->to<QualifiedName>().left) ||
                       visitNode(cbNode, node->to<QualifiedName>().right);
            case SyntaxKind::TypeParameter:
                return visitNodes(cbNode, cbNodes, node->to<TypeParameterDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<TypeParameterDeclaration>().name) ||
                       visitNode(cbNode, node->to<TypeParameterDeclaration>().constraint) ||
                       visitNode(cbNode, node->to<TypeParameterDeclaration>().defaultType) ||
                       visitNode(cbNode, node->to<TypeParameterDeclaration>().expression);
            case SyntaxKind::ShorthandPropertyAssignment:
                return visitNodes(cbNode, cbNodes, node->to<ShorthandPropertyAssignment>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<ShorthandPropertyAssignment>().modifiers) ||
                       visitNode(cbNode, node->to<ShorthandPropertyAssignment>().name) ||
                       visitNode(cbNode, node->to<ShorthandPropertyAssignment>().questionToken) ||
                       visitNode(cbNode, node->to<ShorthandPropertyAssignment>().exclamationToken) ||
                       visitNode(cbNode, node->to<ShorthandPropertyAssignment>().equalsToken) ||
                       visitNode(cbNode, node->to<ShorthandPropertyAssignment>().objectAssignmentInitializer);
            case SyntaxKind::SpreadAssignment:
                return visitNode(cbNode, node->to<SpreadAssignment>().expression);
            case SyntaxKind::Parameter:
                return visitNodes(cbNode, cbNodes, node->to<ParameterDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<ParameterDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<ParameterDeclaration>().dotDotDotToken) ||
                       visitNode(cbNode, node->to<ParameterDeclaration>().name) ||
                       visitNode(cbNode, node->to<ParameterDeclaration>().questionToken) ||
                       visitNode(cbNode, node->to<ParameterDeclaration>().type) ||
                       visitNode(cbNode, node->to<ParameterDeclaration>().initializer);
            case SyntaxKind::PropertyDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<PropertyDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<PropertyDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<PropertyDeclaration>().name) ||
                       visitNode(cbNode, node->to<PropertyDeclaration>().questionToken) ||
                       visitNode(cbNode, node->to<PropertyDeclaration>().exclamationToken) ||
                       visitNode(cbNode, node->to<PropertyDeclaration>().type) ||
                       visitNode(cbNode, node->to<PropertyDeclaration>().initializer);
            case SyntaxKind::PropertySignature:
                return visitNodes(cbNode, cbNodes, node->to<PropertySignature>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<PropertySignature>().modifiers) ||
                       visitNode(cbNode, node->to<PropertySignature>().name) ||
                       visitNode(cbNode, node->to<PropertySignature>().questionToken) ||
                       visitNode(cbNode, node->to<PropertySignature>().type) ||
                       visitNode(cbNode, node->to<PropertySignature>().initializer);
            case SyntaxKind::PropertyAssignment:
                return visitNodes(cbNode, cbNodes, node->to<PropertyAssignment>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<PropertyAssignment>().modifiers) ||
                       visitNode(cbNode, node->to<PropertyAssignment>().name) ||
                       visitNode(cbNode, node->to<PropertyAssignment>().questionToken) ||
                       visitNode(cbNode, node->to<PropertyAssignment>().initializer);
            case SyntaxKind::VariableDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<VariableDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<VariableDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<VariableDeclaration>().name) ||
                       visitNode(cbNode, node->to<VariableDeclaration>().exclamationToken) ||
                       visitNode(cbNode, node->to<VariableDeclaration>().type) ||
                       visitNode(cbNode, node->to<VariableDeclaration>().initializer);
            case SyntaxKind::BindingElement: {
                return visitNodes(cbNode, cbNodes, node->to<BindingElement>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<BindingElement>().modifiers) ||
                       visitNode(cbNode, node->to<BindingElement>().dotDotDotToken) ||
                       visitNode(cbNode, node->to<BindingElement>().propertyName) ||
                       visitNode(cbNode, node->to<BindingElement>().name) ||
                       visitNode(cbNode, node->to<BindingElement>().initializer);
            }
            case SyntaxKind::FunctionType:
            case SyntaxKind::ConstructorType:
            case SyntaxKind::CallSignature:
            case SyntaxKind::ConstructSignature:
            case SyntaxKind::IndexSignature:
                return visitNodes(cbNode, cbNodes, node->decorators) ||
                       visitNodes(cbNode, cbNodes, node->modifiers) ||
                       visitNodes(cbNode, cbNodes, node->to<IndexSignatureDeclaration>().typeParameters) ||
                       visitNodes(cbNode, cbNodes, node->to<IndexSignatureDeclaration>().parameters) ||
                       visitNode(cbNode, node->to<IndexSignatureDeclaration>().type);
            case SyntaxKind::MethodDeclaration:
            case SyntaxKind::MethodSignature:
            case SyntaxKind::Constructor:
            case SyntaxKind::GetAccessor:
            case SyntaxKind::SetAccessor:
            case SyntaxKind::FunctionExpression:
            case SyntaxKind::FunctionDeclaration:
            case SyntaxKind::ArrowFunction: {
                if (auto b = visitNodes(cbNode, cbNodes, node->decorators)) return b;

                return visitNodes(cbNode, cbNodes, node->decorators) ||
                       visitNodes(cbNode, cbNodes, node->modifiers) ||
                       visitNode(cbNode, node->cast<FunctionLikeDeclarationBase>().asteriskToken) ||
                       visitNode(cbNode, node->cast<FunctionLikeDeclarationBase>().name) ||
                       visitNode(cbNode, node->cast<FunctionLikeDeclarationBase>().questionToken) ||
                       visitNode(cbNode, node->cast<FunctionLikeDeclarationBase>().exclamationToken) ||
                       visitNodes(cbNode, cbNodes, node->cast<FunctionLikeDeclarationBase>().typeParameters) ||
                       visitNodes(cbNode, cbNodes, node->cast<FunctionLikeDeclarationBase>().parameters) ||
                       visitNode(cbNode, node->cast<FunctionLikeDeclarationBase>().type) ||
                       (node->kind == SyntaxKind::ArrowFunction ? visitNode(cbNode, node->to<ArrowFunction>().equalsGreaterThanToken) : shared<Node>(nullptr)) ||
                       visitNode(cbNode, node->cast<FunctionLikeDeclarationBase>().body);
            }
            case SyntaxKind::ClassStaticBlockDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<ClassStaticBlockDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<ClassStaticBlockDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<ClassStaticBlockDeclaration>().body);
            case SyntaxKind::TypeReference:
                return visitNode(cbNode, node->to<TypeReferenceNode>().typeName) ||
                       visitNodes(cbNode, cbNodes, node->to<TypeReferenceNode>().typeArguments);
            case SyntaxKind::TypePredicate:
                return visitNode(cbNode, node->to<TypePredicateNode>().assertsModifier) ||
                       visitNode(cbNode, node->to<TypePredicateNode>().parameterName) ||
                       visitNode(cbNode, node->to<TypePredicateNode>().type);
            case SyntaxKind::TypeQuery:
                return visitNode(cbNode, node->to<TypeQueryNode>().exprName) ||
                       visitNodes(cbNode, cbNodes, node->to<TypeQueryNode>().typeArguments);
            case SyntaxKind::TypeLiteral:
                return visitNodes(cbNode, cbNodes, node->to<TypeLiteralNode>().members);
            case SyntaxKind::ArrayType:
                return visitNode(cbNode, node->to<ArrayTypeNode>().elementType);
            case SyntaxKind::TupleType:
                return visitNodes(cbNode, cbNodes, node->to<TupleTypeNode>().elements);
            case SyntaxKind::UnionType:
                return visitNodes(cbNode, cbNodes, node->to<UnionTypeNode>().types);
            case SyntaxKind::IntersectionType:
                return visitNodes(cbNode, cbNodes, node->to<IntersectionTypeNode>().types);
            case SyntaxKind::ConditionalType:
                return visitNode(cbNode, node->to<ConditionalTypeNode>().checkType) ||
                       visitNode(cbNode, node->to<ConditionalTypeNode>().extendsType) ||
                       visitNode(cbNode, node->to<ConditionalTypeNode>().trueType) ||
                       visitNode(cbNode, node->to<ConditionalTypeNode>().falseType);
            case SyntaxKind::InferType:
                return visitNode(cbNode, node->to<InferTypeNode>().typeParameter);
            case SyntaxKind::ImportType:
                return visitNode(cbNode, node->to<ImportTypeNode>().argument) ||
                       visitNode(cbNode, node->to<ImportTypeNode>().assertions) ||
                       visitNode(cbNode, node->to<ImportTypeNode>().qualifier) ||
                       visitNodes(cbNode, cbNodes, node->to<ImportTypeNode>().typeArguments);
            case SyntaxKind::ImportTypeAssertionContainer:
                return visitNode(cbNode, node->to<ImportTypeAssertionContainer>().assertClause);
            case SyntaxKind::ParenthesizedType:
                return visitNode(cbNode, node->to<ParenthesizedTypeNode>().type);
            case SyntaxKind::TypeOperator:
                return visitNode(cbNode, node->to<TypeOperatorNode>().type);
            case SyntaxKind::IndexedAccessType:
                return visitNode(cbNode, node->to<IndexedAccessTypeNode>().objectType) ||
                       visitNode(cbNode, node->to<IndexedAccessTypeNode>().indexType);
            case SyntaxKind::MappedType:
                return visitNode(cbNode, node->to<MappedTypeNode>().readonlyToken) ||
                       visitNode(cbNode, node->to<MappedTypeNode>().typeParameter) ||
                       visitNode(cbNode, node->to<MappedTypeNode>().nameType) ||
                       visitNode(cbNode, node->to<MappedTypeNode>().questionToken) ||
                       visitNode(cbNode, node->to<MappedTypeNode>().type) ||
                       visitNodes(cbNode, cbNodes, node->to<MappedTypeNode>().members);
            case SyntaxKind::LiteralType:
                return visitNode(cbNode, node->to<LiteralTypeNode>().literal);
            case SyntaxKind::NamedTupleMember:
                return visitNode(cbNode, node->to<NamedTupleMember>().dotDotDotToken) ||
                       visitNode(cbNode, node->to<NamedTupleMember>().name) ||
                       visitNode(cbNode, node->to<NamedTupleMember>().questionToken) ||
                       visitNode(cbNode, node->to<NamedTupleMember>().type);
            case SyntaxKind::ObjectBindingPattern:
                return visitNodes(cbNode, cbNodes, node->to<ObjectBindingPattern>().elements);
            case SyntaxKind::ArrayBindingPattern:
                return visitNodes(cbNode, cbNodes, node->to<ArrayBindingPattern>().elements);
            case SyntaxKind::ArrayLiteralExpression:
                return visitNodes(cbNode, cbNodes, node->to<ArrayLiteralExpression>().elements);
            case SyntaxKind::ObjectLiteralExpression:
                return visitNodes(cbNode, cbNodes, node->to<ObjectLiteralExpression>().properties);
            case SyntaxKind::PropertyAccessExpression:
                return visitNode(cbNode, node->to<PropertyAccessExpression>().expression) ||
                       visitNode(cbNode, node->to<PropertyAccessExpression>().questionDotToken) ||
                       visitNode(cbNode, node->to<PropertyAccessExpression>().name);
            case SyntaxKind::ElementAccessExpression:
                return visitNode(cbNode, node->to<ElementAccessExpression>().expression) ||
                       visitNode(cbNode, node->to<ElementAccessExpression>().questionDotToken) ||
                       visitNode(cbNode, node->to<ElementAccessExpression>().argumentExpression);
            case SyntaxKind::CallExpression:
            case SyntaxKind::NewExpression:
                return visitNode(cbNode, node->to<CallExpression>().expression) ||
                       visitNode(cbNode, node->to<CallExpression>().questionDotToken) ||
                       visitNodes(cbNode, cbNodes, node->to<CallExpression>().typeArguments) ||
                       visitNodes(cbNode, cbNodes, node->to<CallExpression>().arguments);
            case SyntaxKind::TaggedTemplateExpression:
                return visitNode(cbNode, node->to<TaggedTemplateExpression>().tag) ||
                       visitNode(cbNode, node->to<TaggedTemplateExpression>().questionDotToken) ||
                       visitNodes(cbNode, cbNodes, node->to<TaggedTemplateExpression>().typeArguments) ||
                       visitNode(cbNode, node->to<TaggedTemplateExpression>().templateLiteral);
            case SyntaxKind::TypeAssertionExpression:
                return visitNode(cbNode, node->to<TypeAssertion>().type) ||
                       visitNode(cbNode, node->to<TypeAssertion>().expression);
            case SyntaxKind::ParenthesizedExpression:
                return visitNode(cbNode, node->to<ParenthesizedExpression>().expression);
            case SyntaxKind::DeleteExpression:
                return visitNode(cbNode, node->to<DeleteExpression>().expression);
            case SyntaxKind::TypeOfExpression:
                return visitNode(cbNode, node->to<TypeOfExpression>().expression);
            case SyntaxKind::VoidExpression:
                return visitNode(cbNode, node->to<VoidExpression>().expression);
            case SyntaxKind::PrefixUnaryExpression:
                return visitNode(cbNode, node->to<PrefixUnaryExpression>().operand);
            case SyntaxKind::YieldExpression:
                return visitNode(cbNode, node->to<YieldExpression>().asteriskToken) ||
                       visitNode(cbNode, node->to<YieldExpression>().expression);
            case SyntaxKind::AwaitExpression:
                return visitNode(cbNode, node->to<AwaitExpression>().expression);
            case SyntaxKind::PostfixUnaryExpression:
                return visitNode(cbNode, node->to<PostfixUnaryExpression>().operand);
            case SyntaxKind::BinaryExpression:
                return visitNode(cbNode, node->to<BinaryExpression>().left) ||
                       visitNode(cbNode, node->to<BinaryExpression>().operatorToken) ||
                       visitNode(cbNode, node->to<BinaryExpression>().right);
            case SyntaxKind::AsExpression:
                return visitNode(cbNode, node->to<AsExpression>().expression) ||
                       visitNode(cbNode, node->to<AsExpression>().type);
            case SyntaxKind::NonNullExpression:
                return visitNode(cbNode, node->to<NonNullExpression>().expression);
            case SyntaxKind::MetaProperty:
                return visitNode(cbNode, node->to<MetaProperty>().name);
            case SyntaxKind::ConditionalExpression:
                return visitNode(cbNode, node->to<ConditionalExpression>().condition) ||
                       visitNode(cbNode, node->to<ConditionalExpression>().questionToken) ||
                       visitNode(cbNode, node->to<ConditionalExpression>().whenTrue) ||
                       visitNode(cbNode, node->to<ConditionalExpression>().colonToken) ||
                       visitNode(cbNode, node->to<ConditionalExpression>().whenFalse);
            case SyntaxKind::SpreadElement:
                return visitNode(cbNode, node->to<SpreadElement>().expression);
            case SyntaxKind::Block:
            case SyntaxKind::ModuleBlock:
                return visitNodes(cbNode, cbNodes, node->to<Block>().statements);
            case SyntaxKind::SourceFile:
                return visitNodes(cbNode, cbNodes, node->to<SourceFile>().statements) ||
                       visitNode(cbNode, node->to<SourceFile>().endOfFileToken);
            case SyntaxKind::VariableStatement:
                return visitNodes(cbNode, cbNodes, node->to<VariableStatement>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<VariableStatement>().modifiers) ||
                       visitNode(cbNode, node->to<VariableStatement>().declarationList);
            case SyntaxKind::VariableDeclarationList:
                return visitNodes(cbNode, cbNodes, node->to<VariableDeclarationList>().declarations);
            case SyntaxKind::ExpressionStatement:
                return visitNode(cbNode, node->to<ExpressionStatement>().expression);
            case SyntaxKind::IfStatement:
                return visitNode(cbNode, node->to<IfStatement>().expression) ||
                       visitNode(cbNode, node->to<IfStatement>().thenStatement) ||
                       visitNode(cbNode, node->to<IfStatement>().elseStatement);
            case SyntaxKind::DoStatement:
                return visitNode(cbNode, node->to<DoStatement>().statement) ||
                       visitNode(cbNode, node->to<DoStatement>().expression);
            case SyntaxKind::WhileStatement:
                return visitNode(cbNode, node->to<WhileStatement>().expression) ||
                       visitNode(cbNode, node->to<WhileStatement>().statement);
            case SyntaxKind::ForStatement:
                return visitNode(cbNode, node->to<ForStatement>().initializer) ||
                       visitNode(cbNode, node->to<ForStatement>().condition) ||
                       visitNode(cbNode, node->to<ForStatement>().incrementor) ||
                       visitNode(cbNode, node->to<ForStatement>().statement);
            case SyntaxKind::ForInStatement:
                return visitNode(cbNode, node->to<ForInStatement>().initializer) ||
                       visitNode(cbNode, node->to<ForInStatement>().expression) ||
                       visitNode(cbNode, node->to<ForInStatement>().statement);
            case SyntaxKind::ForOfStatement:
                return visitNode(cbNode, node->to<ForOfStatement>().awaitModifier) ||
                       visitNode(cbNode, node->to<ForOfStatement>().initializer) ||
                       visitNode(cbNode, node->to<ForOfStatement>().expression) ||
                       visitNode(cbNode, node->to<ForOfStatement>().statement);
            case SyntaxKind::ContinueStatement:
                return visitNode(cbNode, node->to<ContinueStatement>().label);
            case SyntaxKind::BreakStatement:
                return visitNode(cbNode, node->to<BreakStatement>().label);
            case SyntaxKind::ReturnStatement:
                return visitNode(cbNode, node->to<ReturnStatement>().expression);
            case SyntaxKind::WithStatement:
                return visitNode(cbNode, node->to<WithStatement>().expression) ||
                       visitNode(cbNode, node->to<WithStatement>().statement);
            case SyntaxKind::SwitchStatement:
                return visitNode(cbNode, node->to<SwitchStatement>().expression) ||
                       visitNode(cbNode, node->to<SwitchStatement>().caseBlock);
            case SyntaxKind::CaseBlock:
                return visitNodes(cbNode, cbNodes, node->to<CaseBlock>().clauses);
            case SyntaxKind::CaseClause:
                return visitNode(cbNode, node->to<CaseClause>().expression) ||
                       visitNodes(cbNode, cbNodes, node->to<CaseClause>().statements);
            case SyntaxKind::DefaultClause:
                return visitNodes(cbNode, cbNodes, node->to<DefaultClause>().statements);
            case SyntaxKind::LabeledStatement:
                return visitNode(cbNode, node->to<LabeledStatement>().label) ||
                       visitNode(cbNode, node->to<LabeledStatement>().statement);
            case SyntaxKind::ThrowStatement:
                return visitNode(cbNode, node->to<ThrowStatement>().expression);
            case SyntaxKind::TryStatement:
                return visitNode(cbNode, node->to<TryStatement>().tryBlock) ||
                       visitNode(cbNode, node->to<TryStatement>().catchClause) ||
                       visitNode(cbNode, node->to<TryStatement>().finallyBlock);
            case SyntaxKind::CatchClause:
                return visitNode(cbNode, node->to<CatchClause>().variableDeclaration) ||
                       visitNode(cbNode, node->to<CatchClause>().block);
            case SyntaxKind::Decorator:
                return visitNode(cbNode, node->to<Decorator>().expression);
            case SyntaxKind::ClassDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<ClassDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<ClassDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<ClassDeclaration>().name) ||
                       visitNodes(cbNode, cbNodes, node->to<ClassDeclaration>().typeParameters) ||
                       visitNodes(cbNode, cbNodes, node->to<ClassDeclaration>().heritageClauses) ||
                       visitNodes(cbNode, cbNodes, node->to<ClassDeclaration>().members);
            case SyntaxKind::ClassExpression:
                return visitNodes(cbNode, cbNodes, node->to<ClassExpression>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<ClassExpression>().modifiers) ||
                       visitNode(cbNode, node->to<ClassExpression>().name) ||
                       visitNodes(cbNode, cbNodes, node->to<ClassExpression>().typeParameters) ||
                       visitNodes(cbNode, cbNodes, node->to<ClassExpression>().heritageClauses) ||
                       visitNodes(cbNode, cbNodes, node->to<ClassExpression>().members);
            case SyntaxKind::InterfaceDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<InterfaceDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<InterfaceDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<InterfaceDeclaration>().name) ||
                       visitNodes(cbNode, cbNodes, node->to<InterfaceDeclaration>().typeParameters) ||
                       visitNodes(cbNode, cbNodes, node->to<ClassDeclaration>().heritageClauses) ||
                       visitNodes(cbNode, cbNodes, node->to<InterfaceDeclaration>().members);
            case SyntaxKind::TypeAliasDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<TypeAliasDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<TypeAliasDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<TypeAliasDeclaration>().name) ||
                       visitNodes(cbNode, cbNodes, node->to<TypeAliasDeclaration>().typeParameters) ||
                       visitNode(cbNode, node->to<TypeAliasDeclaration>().type);
            case SyntaxKind::EnumDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<EnumDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<EnumDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<EnumDeclaration>().name) ||
                       visitNodes(cbNode, cbNodes, node->to<EnumDeclaration>().members);
            case SyntaxKind::EnumMember:
                return visitNode(cbNode, node->to<EnumMember>().name) ||
                       visitNode(cbNode, node->to<EnumMember>().initializer);
            case SyntaxKind::ModuleDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<ModuleDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<ModuleDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<ModuleDeclaration>().name) ||
                       visitNode(cbNode, node->to<ModuleDeclaration>().body);
            case SyntaxKind::ImportEqualsDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<ImportEqualsDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<ImportEqualsDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<ImportEqualsDeclaration>().name) ||
                       visitNode(cbNode, node->to<ImportEqualsDeclaration>().moduleReference);
            case SyntaxKind::ImportDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<ImportDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<ImportDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<ImportDeclaration>().importClause) ||
                       visitNode(cbNode, node->to<ImportDeclaration>().moduleSpecifier) ||
                       visitNode(cbNode, node->to<ImportDeclaration>().assertClause);
            case SyntaxKind::ImportClause:
                return visitNode(cbNode, node->to<ImportClause>().name) ||
                       visitNode(cbNode, node->to<ImportClause>().namedBindings);
            case SyntaxKind::AssertClause:
                return visitNodes(cbNode, cbNodes, node->to<AssertClause>().elements);
            case SyntaxKind::AssertEntry:
                return visitNode(cbNode, node->to<AssertEntry>().name) ||
                       visitNode(cbNode, node->to<AssertEntry>().value);
            case SyntaxKind::NamespaceExportDeclaration:
                return visitNode(cbNode, node->to<NamespaceExportDeclaration>().name);
            case SyntaxKind::NamespaceImport:
                return visitNode(cbNode, node->to<NamespaceImport>().name);
            case SyntaxKind::NamespaceExport:
                return visitNode(cbNode, node->to<NamespaceExport>().name);
            case SyntaxKind::NamedImports:
                return visitNodes(cbNode, cbNodes, node->to<NamedImports>().elements);
            case SyntaxKind::NamedExports:
                return visitNodes(cbNode, cbNodes, node->to<NamedExports>().elements);
            case SyntaxKind::ExportDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<ExportDeclaration>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<ExportDeclaration>().modifiers) ||
                       visitNode(cbNode, node->to<ExportDeclaration>().exportClause) ||
                       visitNode(cbNode, node->to<ExportDeclaration>().moduleSpecifier) ||
                       visitNode(cbNode, node->to<ExportDeclaration>().assertClause);
            case SyntaxKind::ImportSpecifier:
                return visitNode(cbNode, node->to<ImportSpecifier>().propertyName) ||
                       visitNode(cbNode, node->to<ImportSpecifier>().name);
            case SyntaxKind::ExportSpecifier:
                return visitNode(cbNode, node->to<ExportSpecifier>().propertyName) ||
                       visitNode(cbNode, node->to<ExportSpecifier>().name);
            case SyntaxKind::ExportAssignment:
                return visitNodes(cbNode, cbNodes, node->to<ExportAssignment>().decorators) ||
                       visitNodes(cbNode, cbNodes, node->to<ExportAssignment>().modifiers) ||
                       visitNode(cbNode, node->to<ExportAssignment>().expression);
            case SyntaxKind::TemplateExpression:
                return visitNode(cbNode, node->to<TemplateExpression>().head) || visitNodes(cbNode, cbNodes, node->to<TemplateExpression>().templateSpans);
            case SyntaxKind::TemplateSpan:
                return visitNode(cbNode, node->to<TemplateSpan>().expression) || visitNode(cbNode, node->to<TemplateSpan>().literal);
            case SyntaxKind::TemplateLiteralType:
                return visitNode(cbNode, node->to<TemplateLiteralTypeNode>().head) || visitNodes(cbNode, cbNodes, node->to<TemplateLiteralTypeNode>().templateSpans);
            case SyntaxKind::TemplateLiteralTypeSpan:
                return visitNode(cbNode, node->to<TemplateLiteralTypeSpan>().type) || visitNode(cbNode, node->to<TemplateLiteralTypeSpan>().literal);
            case SyntaxKind::ComputedPropertyName:
                return visitNode(cbNode, node->to<ComputedPropertyName>().expression);
            case SyntaxKind::HeritageClause:
                return visitNodes(cbNode, cbNodes, node->to<HeritageClause>().types);
            case SyntaxKind::ExpressionWithTypeArguments:
                return visitNode(cbNode, node->to<ExpressionWithTypeArguments>().expression) ||
                       visitNodes(cbNode, cbNodes, node->to<ExpressionWithTypeArguments>().typeArguments);
            case SyntaxKind::ExternalModuleReference:
                return visitNode(cbNode, node->to<ExternalModuleReference>().expression);
            case SyntaxKind::MissingDeclaration:
                return visitNodes(cbNode, cbNodes, node->to<MissingDeclaration>().decorators);
            case SyntaxKind::CommaListExpression:
                return visitNodes(cbNode, cbNodes, node->to<CommaListExpression>().elements);

            case SyntaxKind::JsxElement:
                return visitNode(cbNode, node->to<JsxElement>().openingElement) ||
                       visitNodes(cbNode, cbNodes, node->to<JsxElement>().children) ||
                       visitNode(cbNode, node->to<JsxElement>().closingElement);
            case SyntaxKind::JsxFragment:
                return visitNode(cbNode, node->to<JsxFragment>().openingFragment) ||
                       visitNodes(cbNode, cbNodes, node->to<JsxFragment>().children) ||
                       visitNode(cbNode, node->to<JsxFragment>().closingFragment);
            case SyntaxKind::JsxSelfClosingElement:
                return visitNode(cbNode, node->to<JsxSelfClosingElement>().tagName) ||
                       visitNodes(cbNode, cbNodes, node->to<JsxSelfClosingElement>().typeArguments) ||
                       visitNode(cbNode, node->to<JsxSelfClosingElement>().attributes);
            case SyntaxKind::JsxOpeningElement:
                return visitNode(cbNode, node->to<JsxOpeningElement>().tagName) ||
                       visitNodes(cbNode, cbNodes, node->to<JsxOpeningElement>().typeArguments) ||
                       visitNode(cbNode, node->to<JsxOpeningElement>().attributes);
            case SyntaxKind::JsxAttributes:
                return visitNodes(cbNode, cbNodes, node->to<JsxAttributes>().properties);
            case SyntaxKind::JsxAttribute:
                return visitNode(cbNode, node->to<JsxAttribute>().name) ||
                       visitNode(cbNode, node->to<JsxAttribute>().initializer);
            case SyntaxKind::JsxSpreadAttribute:
                return visitNode(cbNode, node->to<JsxSpreadAttribute>().expression);
            case SyntaxKind::JsxExpression:
                return visitNode(cbNode, node->to<JsxExpression>().dotDotDotToken) ||
                       visitNode(cbNode, node->to<JsxExpression>().expression);
            case SyntaxKind::JsxClosingElement:
                return visitNode(cbNode, node->to<JsxClosingElement>().tagName);

            case SyntaxKind::OptionalType:
                return visitNode(cbNode, node->to<OptionalTypeNode>().type);
            case SyntaxKind::RestType:
                return visitNode(cbNode, node->to<RestTypeNode>().type);
            case SyntaxKind::PartiallyEmittedExpression:
                return visitNode(cbNode, node->to<PartiallyEmittedExpression>().expression);
        }
    }

    //@note: not in use inside Parser
//    /** @internal */
//    /**
//     * Invokes a callback for each child of the given node. The 'cbNode' callback is invoked for all child nodes
//     * stored in properties. If a 'cbNodes' callback is specified, it is invoked for embedded arrays; additionally,
//     * unlike `forEachChild`, embedded arrays are flattened and the 'cbNode' callback is invoked for each element.
//     *  If a callback returns a truthy value, iteration stops and that value is returned. Otherwise, undefined is returned.
//     *
//     * @param node a given node to visit its children
//     * @param cbNode a callback to be invoked for all child nodes
//     * @param cbNodes a callback to be invoked for embedded array
//     *
//     * @remarks Unlike `forEachChild`, `forEachChildRecursively` handles recursively invoking the traversal on each child node found,
//     * and while doing so, handles traversing the structure without relying on the callstack to encode the tree structure.
//     */
//    Node *forEachChildRecursively(
//        Node rootNode,
//        cbNode: (node: Node, parent: Node) => T | "skip" | undefined,
//        cbNodes?: (nodes: NodeArray<Node>, parent: Node) => T | "skip" | undefined
//    ) {
//        auto queue: (Node | NodeArray<Node>)[] = gatherPossibleChildren(rootNode);
//        auto parents: Node[] = []; // tracks parent references for elements in queue
//        while (parents.length < queue.length) {
//            parents.push(rootNode);
//        }
//        while (queue.length != 0) {
//            auto current = queue.pop()!;
//            auto parent = parents.pop()!;
//            if (isArray(current)) {
//                if (cbNodes) {
//                    auto res = cbNodes(current, parent);
//                    if (res) {
//                        if (res == "skip") continue;
//                        return res;
//                    }
//                }
//                for (let i = current.length - 1; i >= 0; --i) {
//                    queue.push(current[i]);
//                    parents.push(parent);
//                }
//            }
//            else {
//                auto res = cbNode(current, parent);
//                if (res) {
//                    if (res == "skip") continue;
//                    return res;
//                }
//                if (current.kind >= SyntaxKind::FirstNode) {
//                    // add children in reverse order to the queue, so popping gives the first child
//                    for (const child of gatherPossibleChildren(current)) {
//                        queue.push(child);
//                        parents.push(current);
//                    }
//                }
//            }
//        }
//    }
//    function gatherPossibleChildren(node: Node) {
//        auto children: (Node | NodeArray<Node>)[] = [];
//        forEachChild(node, addWorkItem, addWorkItem); // By using a stack above and `unshift` here, we emulate a depth-first preorder traversal
//        return children;
//
//        function addWorkItem(n: Node | NodeArray<Node>) {
//            children.unshift(n);
//        }
//    }
//
    struct CreateSourceFileOptions {
        ScriptTarget languageVersion;
        /**
         * Controls the format the file is detected as - this can be derived from only the path
         * and files on disk, but needs to be done with a module resolution cache in scope to be performant.
         * This is usually `undefined` for compilations that do not have `moduleResolution` values of `node16` or `nodenext`.
         */
        optional<ModuleKind> impliedNodeFormat; //?: ModuleKind.ESNext | ModuleKind.CommonJS;

        /**
         * Controls how module-y-ness is set for the given file. Usually the result of calling
         * `getSetExternalModuleIndicator` on a valid `CompilerOptions` object. If not present, the default
         * check specified by `isFileProbablyExternalModule` will be used to set the field.
         */
        optional<function<void(shared<SourceFile>)>> setExternalModuleIndicator;
    };

//
//    function setExternalModuleIndicator(sourceFile: SourceFile) {
//        sourceFile.externalModuleIndicator = isFileProbablyExternalModule(sourceFile);
//    }

    inline void setExternalModuleIndicator(const shared<SourceFile> &sourceFile) {
        sourceFile->externalModuleIndicator = isFileProbablyExternalModule(sourceFile);
    }

//    namespace Parser {
//        shared<SourceFile> parseSourceFile(string fileName, string sourceText, ScriptTarget languageVersion, bool setParentNodes = false, optional<ScriptKind> scriptKind = {}, optional<function<void(shared<SourceFile>)>> setExternalModuleIndicatorOverride = {});
//    };

//
//    export function parseIsolatedEntityName(text: string, languageVersion: ScriptTarget): EntityName | undefined {
//        return Parser.parseIsolatedEntityName(text, languageVersion);
//    }
//
//    /**
//     * Parse json text into SyntaxTree and return node and parse errors if any
//     * @param fileName
//     * @param sourceText
//     */
//    export function parseJsonText(fileName: string, sourceText: string): JsonSourceFile {
//        return Parser.parseJsonText(fileName, sourceText);
//    }

    // See also `isExternalOrCommonJsModule` in utilities.ts
    inline bool isExternalModule(const shared<SourceFile> &file) {
        return (bool) file->externalModuleIndicator;
    }

//    // Produces a new SourceFile for the 'newText' provided. The 'textChangeRange' parameter
//    // indicates what changed between the 'text' that this SourceFile has and the 'newText'.
//    // The SourceFile will be created with the compiler attempting to reuse as many nodes from
//    // this file as possible.
//    //
//    // Note: this function mutates nodes from this SourceFile. That means any existing nodes
//    // from this SourceFile that are being held onto may change as a result (including
//    // becoming detached from any SourceFile).  It is recommended that this SourceFile not
//    // be used once 'update' is called on it.
//    export function updateSourceFile(sourceFile: SourceFile, newText: string, textChangeRange: TextChangeRange, aggressiveChecks = false): SourceFile {
//        auto newSourceFile = IncrementalParser.updateSourceFile(sourceFile, newText, textChangeRange, aggressiveChecks);
//        // Because new source file node is created, it may not have the flag PossiblyContainDynamicImport. This is the case if there is no new edit to add dynamic import.
//        // We will manually port the flag to the new source file.
//        (newSourceFile as Mutable<SourceFile>).flags |= (sourceFile.flags & NodeFlags::PermanentlySetIncrementalFlags);
//        return newSourceFile;
//    }
//
//    /* @internal */
//    export function parseIsolatedJSDocComment(content: string, start?: number, length?: number) {
//        auto result = Parser.JSDocParser.parseIsolatedJSDocComment(content, start, length);
//        if (result && result.jsDoc) {
//            // because the jsDocComment was parsed out of the source file, it might
//            // not be covered by the fixupParentReferences.
//            Parser.fixupParentReferences(result.jsDoc);
//        }
//
//        return result;
//    }
//
//    /* @internal */
//    // Exposed only for testing.
//    export function parseJSDocTypeExpressionForTests(content: string, start?: number, length?: number) {
//        return Parser.JSDocParser.parseJSDocTypeExpressionForTests(content, start, length);
//    }
//
//    // Implement the parser as a singleton module.  We do this for perf reasons because creating
//    // parser instances can actually be expensive enough to impact us on projects with many source
//    // files.
    struct Parser {
        // Share a single scanner across all calls to parse a source file.  This helps speed things
        // up by avoiding the cost of creating/compiling scanners over and over again.
        Scanner scanner{ScriptTarget::Latest, /*skipTrivia*/ true};
        Factory factory;

        int disallowInAndDecoratorContext = (int) NodeFlags::DisallowInContext | (int) NodeFlags::DecoratorContext;

//        // capture constructors in 'initializeState' to avoid null checks
//        // tslint:disable variable-name
//        auto NodeConstructor: new (kind: SyntaxKind, int pos, end: number) => Node;
//        auto TokenConstructor: new (kind: SyntaxKind, int pos, end: number) => Node;
//        auto IdentifierConstructor: new (kind: SyntaxKind, int pos, end: number) => Node;
//        auto PrivateIdentifierConstructor: new (kind: SyntaxKind, int pos, end: number) => Node;
//        auto SourceFileConstructor: new (kind: SyntaxKind, int pos, end: number) => Node;
//        // tslint:enable variable-name

        shared<Node> countNode(const shared<Node> &node);

//        // Rather than using `createBaseNodeFactory` here, we establish a `BaseNodeFactory` that closes over the
//        // constructors above, which are reset each time `initializeState` is called.
//        auto baseNodeFactory: BaseNodeFactory = {
//            createBaseSourceFileNode: kind => countNode(new SourceFileConstructor(kind, /*pos*/ 0, /*end*/ 0)),
//            createBaseIdentifierNode: kind => countNode(new IdentifierConstructor(kind, /*pos*/ 0, /*end*/ 0)),
//            createBasePrivateIdentifierNode: kind => countNode(new PrivateIdentifierConstructor(kind, /*pos*/ 0, /*end*/ 0)),
//            createBaseTokenNode: kind => countNode(new TokenConstructor(kind, /*pos*/ 0, /*end*/ 0)),
//            createBaseNode: kind => countNode(new NodeConstructor(kind, /*pos*/ 0, /*end*/ 0))
//        };
//
//        auto factory = createNodeFactory(NodeFactoryFlags.NoParenthesizerRules | NodeFactoryFlags.NoNodeConverters | NodeFactoryFlags.NoOriginalNode, baseNodeFactory);
//
        string fileName = "";
        /*NodeFlags*/ int sourceFlags = 0;
        string sourceText = "";
        ScriptTarget languageVersion;
        ScriptKind scriptKind;
        LanguageVariant languageVariant;
        vector<DiagnosticWithDetachedLocation> parseDiagnostics;
        vector<DiagnosticWithDetachedLocation> jsDocDiagnostics;
//        auto syntaxCursor: IncrementalParser.SyntaxCursor | undefined;

        SyntaxKind currentToken;
        int nodeCount = 0;
        unordered_map<string, string> identifiers;
        unordered_map<string, string> privateIdentifiers;
//        auto privateIdentifiers: ESMap<string, string>;
        int identifierCount = 0;

        /*ParsingContext*/ int parsingContext = 0;

        std::set<int> notParenthesizedArrow;

        // Flags that dictate what parsing context we're in.  For example:
        // Whether or not we are in strict parsing mode.  All that changes in strict parsing mode is
        // that some tokens that would be considered identifiers may be considered keywords.
        //
        // When adding more parser context flags, consider which is the more common case that the
        // flag will be in.  This should be the 'false' state for that flag.  The reason for this is
        // that we don't store data in our nodes unless the value is in the *non-default* state.  So,
        // for example, more often than code 'allows-in' (or doesn't 'disallow-in').  We opt for
        // 'disallow-in' set to 'false'.  Otherwise, if we had 'allowsIn' set to 'true', then almost
        // all nodes would need extra state on them to store this info.
        //
        // Note: 'allowIn' and 'allowYield' track 1:1 with the [in] and [yield] concepts in the ES6
        // grammar specification.
        //
        // An important thing about these context concepts.  By default they are effectively inherited
        // while parsing through every grammar production.  i.e. if you don't change them, then when
        // you parse a sub-production, it will have the same context values as the parent production.
        // This is great most of the time.  After all, consider all the 'expression' grammar productions
        // and how nearly all of them pass along the 'in' and 'yield' context values:
        //
        // EqualityExpression[In, Yield] :
        //      RelationalExpression[?In, ?Yield]
        //      EqualityExpression[?In, ?Yield] == RelationalExpression[?In, ?Yield]
        //      EqualityExpression[?In, ?Yield] != RelationalExpression[?In, ?Yield]
        //      EqualityExpression[?In, ?Yield] == RelationalExpression[?In, ?Yield]
        //      EqualityExpression[?In, ?Yield] != RelationalExpression[?In, ?Yield]
        //
        // Where you have to be careful is then understanding what the points are in the grammar
        // where the values are *not* passed along.  For example:
        //
        // SingleNameBinding[Yield,GeneratorParameter]
        //      [+GeneratorParameter]BindingIdentifier[Yield] Initializer[In]opt
        //      [~GeneratorParameter]BindingIdentifier[?Yield]Initializer[In, ?Yield]opt
        //
        // Here this is saying that if the GeneratorParameter context flag is set, that we should
        // explicitly set the 'yield' context flag to false before calling into the BindingIdentifier
        // and we should explicitly unset the 'yield' context flag before calling into the Initializer.
        // production.  Conversely, if the GeneratorParameter context flag is not set, then we
        // should leave the 'yield' context flag alone.
        //
        // Getting this all correct is tricky and requires careful reading of the grammar to
        // understand when these values should be changed versus when they should be inherited.
        //
        // Note: it should not be necessary to save/restore these flags during speculative/lookahead
        // parsing.  These context flags are naturally stored and restored through normal recursive
        // descent parsing and unwinding.
        /*NodeFlags*/ int contextFlags = 0;

        // Indicates whether we are currently parsing top-level statements.
        bool topLevel = true;

        // Whether or not we've had a parse error since creating the last AST node.  If we have
        // encountered an error, it will be stored on the next AST node we create.  Parse errors
        // can be broken down into three categories:
        //
        // 1) An error that occurred during scanning.  For example, an unterminated literal, or a
        //    character that was completely not understood.
        //
        // 2) A token was expected, but was not present.  This type of error is commonly produced
        //    by the 'parseExpected' function.
        //
        // 3) A token was present that no parsing function was able to consume.  This type of error
        //    only occurs in the 'abortParsingListOrMoveToNextToken' function when the parser
        //    decides to skip the token.
        //
        // In all of these cases, we want to mark the next node as having had an error before it.
        // With this mark, we can know in incremental settings if this node can be reused, or if
        // we have to reparse it.  If we don't keep this information around, we may just reuse the
        // node.  in that event we would then not produce the same errors as we did before, causing
        // significant confusion problems.
        //
        // Note: it is necessary that this value be saved/restored during speculative/lookahead
        // parsing.  During lookahead parsing, we will often create a node.  That node will have
        // this value attached, and then this value will be set back to 'false'.  If we decide to
        // rewind, we must get back to the same value we had prior to the lookahead.
        //
        // Note: any errors at the end of the file that do not precede a regular node, should get
        // attached to the EOF token.
        bool parseErrorBeforeNextFinishedNode = false;

        /** @internal */
        bool isDeclarationFileName(const string &fileName) {
            return fileExtensionIsOneOf(fileName, supportedDeclarationExtensions);
        }

        // Use this function to access the current token instead of reading the currentToken
        // variable. Since function results aren't narrowed in control flow analysis, this ensures
        // that the type checker doesn't make wrong assumptions about the type of the current
        // token (e.g. a call to nextToken() changes the current token but the checker doesn't
        // reason about this side effect).  Mainstream VMs inline simple functions like this, so
        // there is no performance penalty.
        SyntaxKind token();

        SyntaxKind nextTokenWithoutCheck();

        optional<DiagnosticWithDetachedLocation> parseErrorAtPosition(int start, int length, const shared<DiagnosticMessage> &message, DiagnosticArg arg = "");

        optional<DiagnosticWithDetachedLocation> parseErrorAt(int start, int end, const shared<DiagnosticMessage> &message, DiagnosticArg arg = "") {
            return parseErrorAtPosition(start, end - start, message, arg);
        }

        void scanError(const shared<DiagnosticMessage> &message, int length) {
            parseErrorAtPosition(scanner.getTextPos(), length, message);
        }

        SyntaxKind nextToken() {
            ZoneScoped;
            // if the keyword had an escape
            if (isKeyword(currentToken) && (scanner.hasUnicodeEscape() || scanner.hasExtendedUnicodeEscape())) {
                // issue a parse error for the escape
                parseErrorAt(scanner.getTokenPos(), scanner.getTextPos(), Diagnostics::Keywords_cannot_contain_escape_characters);
            }
            return nextTokenWithoutCheck();
        }

        template<typename T>
        T nextTokenAnd(function<T()> func) {
            ZoneScoped;
            nextToken();
            return func();
        }

        void initializeState(string _fileName, string _sourceText, ScriptTarget _languageVersion, ScriptKind _scriptKind) {
            ZoneScoped;
//            NodeConstructor = objectAllocator.getNodeConstructor();
//            TokenConstructor = objectAllocator.getTokenConstructor();
//            IdentifierConstructor = objectAllocator.getIdentifierConstructor();
//            PrivateIdentifierConstructor = objectAllocator.getPrivateIdentifierConstructor();
//            SourceFileConstructor = objectAllocator.getSourceFileConstructor();

            fileName = normalizePath(_fileName);
            sourceText = _sourceText;
            languageVersion = _languageVersion;
            scriptKind = _scriptKind;
            languageVariant = getLanguageVariant(_scriptKind);

            parseDiagnostics.clear();
            parsingContext = 0;
            identifiers.clear();
//            identifiers = new Map<string, string>();
//            privateIdentifiers = new Map<string, string>();
            identifierCount = 0;
            nodeCount = 0;
            sourceFlags = 0;
            topLevel = true;

            switch (scriptKind) {
                case ScriptKind::JS:
                case ScriptKind::JSX:
                    contextFlags = (int) NodeFlags::JavaScriptFile;
                    break;
                case ScriptKind::JSON:
                    contextFlags = (int) NodeFlags::JavaScriptFile | (int) NodeFlags::JsonFile;
                    break;
                default:
                    contextFlags = (int) NodeFlags::None;
                    break;
            }
            parseErrorBeforeNextFinishedNode = false;

            // Initialize and prime the scanner before parsing the source elements.
            scanner.setText(sourceText);
//            scanner.setOnError([this](auto ...a) { scanError(a...); });
            scanner.setScriptTarget(languageVersion);
            scanner.setLanguageVariant(languageVariant);
        }

        void clearState() {
            ZoneScoped;
            // Clear out the text the scanner is pointing at, so it doesn't keep anything alive unnecessarily.
            scanner.clearCommentDirectives();
            scanner.setText("");
            scanner.setOnError(nullopt);

            // Clear any data.  We don't want to accidentally hold onto it for too long.
            sourceText = "";
            languageVersion = ScriptTarget::Latest;
//            syntaxCursor = undefined;
            scriptKind = ScriptKind::Unknown;
            languageVariant = LanguageVariant::Standard;
            sourceFlags = 0;
            parseDiagnostics.clear();
            parsingContext = 0;
//            identifiers = undefined!;
//            notParenthesizedArrow = undefined;
            notParenthesizedArrow.clear();
            topLevel = true;
        }

        int getNodePos() {
            return scanner.getStartPos();
        }

        template<class T>
        shared<T> withJSDoc(const shared<T> &node, bool hasJSDoc) {
            return node;
            //we do not care about JSDoc
//            return hasJSDoc ? addJSDocComment(node) : node;
        }

        bool hasDeprecatedTag = false;

        template<class T>
        shared<T> addJSDocComment(shared<T> node) {
            //no JSDoc support
            return node;
//            Debug::asserts(!node.jsDoc); // Should only be called once per node
//            auto jsDoc = mapDefined(getJSDocCommentRanges(node, sourceText), comment => JSDocParser.parseJSDocComment(node, comment.pos, comment.end - comment.pos));
//            if (jsDoc.length) node.jsDoc = jsDoc;
//            if (hasDeprecatedTag) {
//                hasDeprecatedTag = false;
//                reinterpret_cast<Mutable<T>&>(node).flags |= NodeFlags::Deprecated;
//            }
//            return node;
        }

//        function reparseTopLevelAwait(sourceFile: SourceFile) {
//            auto savedSyntaxCursor = syntaxCursor;
//            auto baseSyntaxCursor = IncrementalParser.createSyntaxCursor(sourceFile);
//            syntaxCursor = { currentNode };
//
//            auto statements: Statement[] = [];
//            auto savedParseDiagnostics = parseDiagnostics;
//
//            parseDiagnostics = [];
//
//            auto pos = 0;
//            auto start = findNextStatementWithAwait(sourceFile.statements, 0);
//            while (start != -1) {
//                // append all statements between pos and start
//                auto prevStatement = sourceFile.statements[pos];
//                auto nextStatement = sourceFile.statements[start];
//                addRange(statements, sourceFile.statements, pos, start);
//                pos = findNextStatementWithoutAwait(sourceFile.statements, start);
//
//                // append all diagnostics associated with the copied range
//                auto diagnosticStart = findIndex(savedParseDiagnostics, diagnostic => diagnostic.start >= prevStatement.pos);
//                auto diagnosticEnd = diagnosticStart >= 0 ? findIndex(savedParseDiagnostics, diagnostic => diagnostic.start >= nextStatement.pos, diagnosticStart) : -1;
//                if (diagnosticStart >= 0) {
//                    addRange(parseDiagnostics, savedParseDiagnostics, diagnosticStart, diagnosticEnd >= 0 ? diagnosticEnd : undefined);
//                }
//
//                // reparse all statements between start and pos. We skip existing diagnostics for the same range and allow the parser to generate new ones.
//                speculationHelper(() => {
//                    auto savedContextFlags = contextFlags;
//                    contextFlags |= NodeFlags::AwaitContext;
//                    scanner.setTextPos(nextStatement.pos);
//                    nextToken();
//
//                    while (token() != SyntaxKind::EndOfFileToken) {
//                        auto startPos = scanner.getStartPos();
//                        auto statement = parseListElement(ParsingContext::SourceElements, parseStatement);
//                        statements.push(statement);
//                        if (startPos == scanner.getStartPos()) {
//                            nextToken();
//                        }
//
//                        if (pos >= 0) {
//                            auto nonAwaitStatement = sourceFile.statements[pos];
//                            if (statement.end == nonAwaitStatement.pos) {
//                                // done reparsing this section
//                                break;
//                            }
//                            if (statement.end > nonAwaitStatement.pos) {
//                                // we ate into the next statement, so we must reparse it.
//                                pos = findNextStatementWithoutAwait(sourceFile.statements, pos + 1);
//                            }
//                        }
//                    }
//
//                    contextFlags = savedContextFlags;
//                }, SpeculationKind::Reparse);
//
//                // find the next statement containing an `await`
//                start = pos >= 0 ? findNextStatementWithAwait(sourceFile.statements, pos) : -1;
//            }
//
//            // append all statements between pos and the end of the list
//            if (pos >= 0) {
//                auto prevStatement = sourceFile.statements[pos];
//                addRange(statements, sourceFile.statements, pos);
//
//                // append all diagnostics associated with the copied range
//                auto diagnosticStart = findIndex(savedParseDiagnostics, diagnostic => diagnostic.start >= prevStatement.pos);
//                if (diagnosticStart >= 0) {
//                    addRange(parseDiagnostics, savedParseDiagnostics, diagnosticStart);
//                }
//            }
//
//            syntaxCursor = savedSyntaxCursor;
//            return factory.updateSourceFile(sourceFile, setTextRange(factory.createNodeArray(statements), sourceFile.statements));
//
//            function containsPossibleTopLevelAwait(node: Node) {
//                return !(node.flags & NodeFlags::AwaitContext)
//                    && !!(node.transformFlags & TransformFlags.ContainsPossibleTopLevelAwait);
//            }
//
//            function findNextStatementWithAwait(statements: NodeArray<Statement>, start: number) {
//                for (let i = start; i < statements.length; i++) {
//                    if (containsPossibleTopLevelAwait(statements[i])) {
//                        return i;
//                    }
//                }
//                return -1;
//            }
//
//            function findNextStatementWithoutAwait(statements: NodeArray<Statement>, start: number) {
//                for (let i = start; i < statements.length; i++) {
//                    if (!containsPossibleTopLevelAwait(statements[i])) {
//                        return i;
//                    }
//                }
//                return -1;
//            }
//
//            function currentNode(position: number) {
//                auto node = baseSyntaxCursor.currentNode(position);
//                if (topLevel && node && containsPossibleTopLevelAwait(node)) {
//                    node.intersectsChange = true;
//                }
//                return node;
//            }
//
//        }
//
//        export function fixupParentReferences(rootNode: Node) {
//            // normally parent references are set during binding. However, for clients that only need
//            // a syntax tree, and no semantic features, then the binding process is an unnecessary
//            // overhead.  This functions allows us to set all the parents, without all the expense of
//            // binding.
//            setParentRecursive(rootNode, /*incremental*/ true);
//        }

        shared<SourceFile> createSourceFile(
                string fileName,
                ScriptTarget languageVersion,
                ScriptKind scriptKind,
                bool isDeclarationFile,
                shared<NodeArray> statements,
                shared<EndOfFileToken> endOfFileToken,
                int flags,
                function<void(shared<SourceFile>)> setExternalModuleIndicator
        ) {
            // code from createNode is inlined here so createNode won't have to deal with special case of creating source files
            // this is quite rare comparing to other nodes and createNode should be as fast as possible
            auto sourceFile = factory.createSourceFile(statements, endOfFileToken, flags);
            setTextRangePosEnd(sourceFile, 0, sourceText.size());
            sourceFile->text = sourceText;

//                sourceFile.bindDiagnostics = [];
//                sourceFile.bindSuggestionDiagnostics = undefined;
            sourceFile->languageVersion = languageVersion;
            sourceFile->fileName = fileName;
            sourceFile->languageVariant = getLanguageVariant(scriptKind);
            sourceFile->isDeclarationFile = isDeclarationFile;
            sourceFile->scriptKind = scriptKind;
            setExternalModuleIndicator(sourceFile);
            sourceFile->setExternalModuleIndicator = setExternalModuleIndicator;

            // If we parsed this as an external module, it may contain top-level await
            if (!isDeclarationFile && isExternalModule(sourceFile) && sourceFile->transformFlags & (int) TransformFlags::ContainsPossibleTopLevelAwait) {
                throw runtime_error("not implemented");
//                sourceFile = reparseTopLevelAwait(sourceFile);

                sourceFile->text = sourceText;
//                sourceFile.bindDiagnostics = [];
//                sourceFile.bindSuggestionDiagnostics = undefined;
                sourceFile->languageVersion = languageVersion;
                sourceFile->fileName = fileName;
                sourceFile->languageVariant = getLanguageVariant(scriptKind);
                sourceFile->isDeclarationFile = isDeclarationFile;
                sourceFile->scriptKind = scriptKind;
                setExternalModuleIndicator(sourceFile);
                sourceFile->setExternalModuleIndicator = setExternalModuleIndicator;
            }

            return sourceFile;
        }

        void setContextFlag(bool val, int flag) {
            if (val) {
                contextFlags |= flag;
            } else {
                contextFlags &= ~flag;
            }
        }
        void setContextFlag(bool val, NodeFlags flag) {
            return setContextFlag(val, (int) flag);
        }

        void setDisallowInContext(bool val) {
            setContextFlag(val, NodeFlags::DisallowInContext);
        }

        void setYieldContext(bool val) {
            setContextFlag(val, NodeFlags::YieldContext);
        }

        void setDecoratorContext(bool val) {
            setContextFlag(val, NodeFlags::DecoratorContext);
        }

        void setAwaitContext(bool val) {
            setContextFlag(val, NodeFlags::AwaitContext);
        }

        optional<DiagnosticWithDetachedLocation> parseErrorAtCurrentToken(const shared<DiagnosticMessage> &message, DiagnosticArg arg = "") {
            return parseErrorAt(scanner.getTokenPos(), scanner.getTextPos(), message, arg);
        }
//
//        function parseErrorAtPosition(start: number, length: number, message: DiagnosticMessage, arg0?: any): DiagnosticWithDetachedLocation | undefined {
//            // Don't report another error if it would just be at the same position as the last error.
//            auto lastError = lastOrUndefined(parseDiagnostics);
//            auto result: DiagnosticWithDetachedLocation | undefined;
//            if (!lastError || start != lastError.start) {
//                result = createDetachedDiagnostic(fileName, start, length, message, arg0);
//                parseDiagnostics.push(result);
//            }
//
//            // Mark that we've encountered an error.  We'll set an appropriate bit on the next
//            // node we finish so that it can't be reused incrementally.
//            parseErrorBeforeNextFinishedNode = true;
//            return result;
//        }

        void parseErrorAtRange(shared<Node> range, const shared<DiagnosticMessage> &message, DiagnosticArg arg = "") {
            parseErrorAt(range->pos, range->end, message, arg);
        }

        bool hasPrecedingJSDocComment() {
            return scanner.hasPrecedingJSDocComment();
        }

        //we don't care about JSDoc
//        SyntaxKind nextTokenJSDoc() {
//            return currentToken = scanner.scanJsDocToken();
//        }

        SyntaxKind reScanGreaterToken() {
            return currentToken = scanner.reScanGreaterToken();
        }

        SyntaxKind reScanSlashToken() {
            return currentToken = scanner.reScanSlashToken();
        }

        SyntaxKind reScanTemplateToken(bool isTaggedTemplate) {
            return currentToken = scanner.reScanTemplateToken(isTaggedTemplate);
        }

        SyntaxKind reScanTemplateHeadOrNoSubstitutionTemplate() {
            return currentToken = scanner.reScanTemplateHeadOrNoSubstitutionTemplate();
        }

        SyntaxKind reScanLessThanToken() {
            return currentToken = scanner.reScanLessThanToken();
        }

        SyntaxKind reScanHashToken() {
            return currentToken = scanner.reScanHashToken();
        }

        SyntaxKind scanJsxIdentifier() {
            return currentToken = scanner.scanJsxIdentifier();
        }

        SyntaxKind scanJsxText() {
            return currentToken = scanner.scanJsxToken();
        }
        template<typename T>
        T speculationHelper(const function<T()> &callback, SpeculationKind speculationKind) {
            ZoneScoped;
            // Keep track of the state we'll need to rollback to if lookahead fails (or if the
            // caller asked us to always reset our state).
            const auto saveToken = currentToken;
            const auto saveParseDiagnosticsLength = parseDiagnostics.size();
            const auto saveParseErrorBeforeNextFinishedNode = parseErrorBeforeNextFinishedNode;

            // Note: it is not actually necessary to save/restore the context flags here.  That's
            // because the saving/restoring of these flags happens naturally through the recursive
            // descent nature of our parser.  However, we still store this here just so we can
            // assert that invariant holds.
            const auto saveContextFlags = contextFlags;

            // If we're only looking ahead, then tell the scanner to only lookahead as well.
            // Otherwise, if we're actually speculatively parsing, then tell the scanner to do the
            // same.
            T result = speculationKind != SpeculationKind::TryParse
                       ? scanner.lookAhead<T>(callback)
                       : scanner.tryScan<T>(callback);

            assert(saveContextFlags == contextFlags);

            // If our callback returned something 'falsy' or we're just looking ahead,
            // then unconditionally restore us to where we were.
            if ((speculationKind != SpeculationKind::TryParse) || !(bool) result) {
                currentToken = saveToken;
                if (speculationKind != SpeculationKind::Reparse) {
                    parseDiagnostics.resize(saveParseDiagnosticsLength);
                }
                parseErrorBeforeNextFinishedNode = saveParseErrorBeforeNextFinishedNode;
            }

            return result;
        }

        /** Invokes the provided callback then unconditionally restores the parser to the state it
         * was in immediately prior to invoking the callback.  The result of invoking the callback
         * is returned from this function.
         */
        template<typename T>
        T lookAhead(const function<T()> &callback) {
            ZoneScoped;
            return speculationHelper<T>(callback, SpeculationKind::Lookahead);
        }

        bool inContext(NodeFlags flags) {
            return (contextFlags & (int) flags) != 0;
        }

        bool inYieldContext() {
            return inContext(NodeFlags::YieldContext);
        }

        bool inAwaitContext() {
            return inContext(NodeFlags::AwaitContext);
        }

        // Ignore strict mode flag because we will report an error in type checker instead.
        bool isIdentifier() {
            if (token() == SyntaxKind::Identifier) {
                return true;
            }

            // If we have a 'yield' keyword, and we're in the [yield] context, then 'yield' is
            // considered a keyword and is not an identifier.
            if (token() == SyntaxKind::YieldKeyword && inYieldContext()) {
                return false;
            }

            // If we have a 'await' keyword, and we're in the [Await] context, then 'await' is
            // considered a keyword and is not an identifier.
            if (token() == SyntaxKind::AwaitKeyword && inAwaitContext()) {
                return false;
            }

            return token() > SyntaxKind::LastReservedWord;
        }
//        auto viableKeywordSuggestions = Object.keys(textToKeywordObj).filter(keyword => keyword.length > 2);


        /**
         * Reports a diagnostic error for the current token being an invalid name.
         *
         * @param blankDiagnostic Diagnostic to report for the case of the name being blank (matched tokenIfBlankName).
         * @param nameDiagnostic Diagnostic to report for all other cases.
         * @param tokenIfBlankName Current token if the name was invalid for being blank (not provided / skipped).
         */
        void parseErrorForInvalidName(shared<DiagnosticMessage> nameDiagnostic, shared<DiagnosticMessage> blankDiagnostic, SyntaxKind tokenIfBlankName) {
            if (token() == tokenIfBlankName) {
                parseErrorAtCurrentToken(blankDiagnostic);
            } else {
                parseErrorAtCurrentToken(nameDiagnostic, scanner.getTokenValue());
            }
        }

        /**
         * Provides a better error message than the generic "';' expected" if possible for
         * known common variants of a missing semicolon, such as from a mispelled names.
         *
         * @param node Node preceding the expected semicolon location.
         */
        void parseErrorForMissingSemicolonAfter(shared<Node> node) {
            // Tagged template literals are sometimes used in places where only simple strings are allowed, i.e.:
            //   module `M1` {
            //   ^^^^^^^^^^^ This block is parsed as a template literal like module`M1`.
            if (isTaggedTemplateExpression(node)) {
                auto n = node->to<TaggedTemplateExpression>();
                parseErrorAt(skipTrivia(sourceText, n.templateLiteral->pos), n.templateLiteral->end, Diagnostics::Module_declaration_names_may_only_use_or_quoted_strings);
                return;
            }

            // Otherwise, if this isn't a well-known keyword-like identifier, give the generic fallback message.
            string expressionText = ts::isIdentifier(node) ? idText(node) : "";
            if (expressionText == "" || !isIdentifierText(expressionText, languageVersion)) {
                parseErrorAtCurrentToken(Diagnostics::_0_expected, tokenToString(SyntaxKind::SemicolonToken));
                return;
            }

            auto pos = skipTrivia(sourceText, node->pos);

            // Some known keywords are likely signs of syntax being used improperly.
            switch (const_hash(expressionText)) {
                case "const"_hash:
                case "let"_hash:
                case "var"_hash:
                    parseErrorAt(pos, node->end, Diagnostics::Variable_declaration_not_allowed_at_this_location);
                    return;

                case "declare"_hash:
                    // If a declared node failed to parse, it would have emitted a diagnostic already.
                    return;

                case "interface"_hash:
                    parseErrorForInvalidName(Diagnostics::Interface_name_cannot_be_0, Diagnostics::Interface_must_be_given_a_name, SyntaxKind::OpenBraceToken);
                    return;

                case "is"_hash:
                    parseErrorAt(pos, scanner.getTextPos(), Diagnostics::A_type_predicate_is_only_allowed_in_return_type_position_for_functions_and_methods);
                    return;

                case "module"_hash:
                case "namespace"_hash:
                    parseErrorForInvalidName(Diagnostics::Namespace_name_cannot_be_0, Diagnostics::Namespace_must_be_given_a_name, SyntaxKind::OpenBraceToken);
                    return;

                case "type"_hash:
                    parseErrorForInvalidName(Diagnostics::Type_alias_name_cannot_be_0, Diagnostics::Type_alias_must_be_given_a_name, SyntaxKind::EqualsToken);
                    return;
            }

            // The user alternatively might have misspelled or forgotten to add a space after a common keyword.
//            auto suggestion = getSpellingSuggestion(expressionText, viableKeywordSuggestions, n => n) ?? getSpaceSuggestion(expressionText);
//            if (suggestion) {
//                parseErrorAt(pos, node.end, Diagnostics::Unknown_keyword_or_identifier_Did_you_mean_0, suggestion);
//                return;
//            }

            // Unknown tokens are handled with their own errors in the scanner
            if (token() == SyntaxKind::Unknown) {
                return;
            }

            // Otherwise, we know this some kind of unknown word, not just a missing expected semicolon.
            parseErrorAt(pos, node->end, Diagnostics::Unexpected_keyword_or_identifier);
        }

//        function getSpaceSuggestion(expressionText: string) {
//            for (const keyword of viableKeywordSuggestions) {
//                if (expressionText.length > keyword.length + 2 && startsWith(expressionText, keyword)) {
//                    return `${keyword} ${expressionText.slice(keyword.length)}`;
//                }
//            }
//
//            return undefined;
//        }

        bool canParseSemicolon() {
            // If there's a real semicolon, then we can always parse it out.
            if (token() == SyntaxKind::SemicolonToken) {
                return true;
            }

            // We can parse out an optional semicolon in ASI cases in the following cases.
            return token() == SyntaxKind::CloseBraceToken || token() == SyntaxKind::EndOfFileToken || scanner.hasPrecedingLineBreak();
        }

        bool tryParseSemicolon() {
            if (!canParseSemicolon()) {
                return false;
            }

            if (token() == SyntaxKind::SemicolonToken) {
                // consume the semicolon if it was explicitly provided.
                nextToken();
            }

            return true;
        }

        void parseSemicolonAfterPropertyName(shared<NodeUnion(PropertyName)> name, sharedOpt<TypeNode> type, sharedOpt<Expression> initializer) {
            if (token() == SyntaxKind::AtToken && !scanner.hasPrecedingLineBreak()) {
                parseErrorAtCurrentToken(Diagnostics::Decorators_must_precede_the_name_and_all_keywords_of_property_declarations);
                return;
            }

            if (token() == SyntaxKind::OpenParenToken) {
                parseErrorAtCurrentToken(Diagnostics::Cannot_start_a_function_call_in_a_type_annotation);
                nextToken();
                return;
            }

            if (type && !canParseSemicolon()) {
                if (initializer) {
                    parseErrorAtCurrentToken(Diagnostics::_0_expected, tokenToString(SyntaxKind::SemicolonToken));
                } else {
                    parseErrorAtCurrentToken(Diagnostics::Expected_for_property_initializer);
                }
                return;
            }

            if (tryParseSemicolon()) {
                return;
            }

            if (initializer) {
                parseErrorAtCurrentToken(Diagnostics::_0_expected, tokenToString(SyntaxKind::SemicolonToken));
                return;
            }

            parseErrorForMissingSemicolonAfter(name);
        }

//        function parseExpectedJSDoc(kind: JSDocSyntaxKind) {
//            if (token() == kind) {
//                nextTokenJSDoc();
//                return true;
//            }
//            parseErrorAtCurrentToken(Diagnostics::_0_expected, tokenToString(kind));
//            return false;
//        }
//
        bool parseOptional(SyntaxKind t) {
            if (token() == t) {
                nextToken();
                return true;
            }
            return false;
        }

        template<class T>
        shared<T> finishNode(shared<T> node, int pos, optional<int> end = {}) {
            ZoneScoped;
            setTextRangePosEnd(node, pos, end ? *end : scanner.getStartPos());
            if (contextFlags) {
                node->flags |= contextFlags;
            }

            // Keep track on the node if we encountered an error while parsing it.  If we did, then
            // we cannot reuse the node incrementally.  Once we've marked this node, clear out the
            // flag so that we don't mark any subsequent nodes.
            if (parseErrorBeforeNextFinishedNode) {
                parseErrorBeforeNextFinishedNode = false;
                node->flags |= (int) NodeFlags::ThisNodeHasError;
            }

            return node;
        }

        template<class T>
        shared<T> parseTokenNode() {
            ZoneScoped;
            auto pos = getNodePos();
            auto kind = token();
            nextToken();
            return finishNode(factory.createToken<T>(kind), pos);
        }

//        function parseOptionalToken<TKind extends SyntaxKind>(t: TKind): Token<TKind>;
        template<class T>
        sharedOpt<T> parseOptionalToken(SyntaxKind t) {
            ZoneScoped;
            if (token() == t) {
                return parseTokenNode<T>();
            }
            return nullptr;
        }

//        function parseOptionalTokenJSDoc<TKind extends JSDocSyntaxKind>(t: TKind): Token<TKind>;
//        function parseOptionalTokenJSDoc(t: JSDocSyntaxKind): Node | undefined {
//            if (token() == t) {
//                return parseTokenNodeJSDoc();
//            }
//            return undefined;
//        }
//
//        function parseExpectedTokenJSDoc<TKind extends JSDocSyntaxKind>(t: TKind): Token<TKind>;
//        function parseExpectedTokenJSDoc(t: JSDocSyntaxKind): Node {
//            return parseOptionalTokenJSDoc(t) ||
//                createMissingNode(t, /*reportAtCurrentPosition*/ false, Diagnostics::_0_expected, tokenToString(t));
//        }
//
//        function parseTokenNodeJSDoc<T extends Node>(): T {
//            auto pos = getNodePos();
//            auto kind = token();
//            nextTokenJSDoc();
//            return finishNode(factory.createToken(kind), pos) as T;
//        }

        bool parseSemicolon() {
            return tryParseSemicolon() || parseExpected(SyntaxKind::SemicolonToken);
        }

        shared<NodeArray> createNodeArray(const shared<NodeArray> &array, int pos, optional<int> end = {}, bool hasTrailingComma = false) {
            if (!array) throw runtime_error("No array passed");
            setTextRangePosEnd(array, pos, end ? *end : scanner.getStartPos());
            return array;
        }
//        shared<NodeArray> createNodeArray(const vector<shared<Node>> &elements, int pos, optional<int> end = {}, bool hasTrailingComma = false) {
//            auto array = factory.createNodeArray(elements, hasTrailingComma);
//            setTextRangePosEnd(array, pos, end ? *end : scanner.getStartPos());
//            return array;
//        }
//
//        //for empty call: createNodeArray(nullptr)
//        shared<NodeArray> createNodeArray(vector<shared<Node>> *elements, int pos, optional<int> end = {}, bool hasTrailingComma = false) {
//            auto array = factory.createNodeArray(elements ? *elements : (vector<shared<Node>>) {}, hasTrailingComma);
//            setTextRangePosEnd(array, pos, end ? *end : scanner.getStartPos());
//            return array;
//        }

//        function createMissingNode<T extends Node>(kind: T["kind"], reportAtCurrentPosition: false, optional<DiagnosticMessage> diagnosticMessage, arg0?: any): T;
//        function createMissingNode<T extends Node>(kind: T["kind"], reportAtCurrentPosition: boolean, diagnosticMessage: DiagnosticMessage, arg0?: any): T;
        template<typename T>
        shared<T> createMissingNode(SyntaxKind kind, bool reportAtCurrentPosition, const sharedOpt<DiagnosticMessage> &diagnosticMessage = nullptr, DiagnosticArg arg = "") {
            if (reportAtCurrentPosition && diagnosticMessage) {
                parseErrorAtPosition(scanner.getStartPos(), 0, diagnosticMessage, arg);
            } else if (diagnosticMessage) {
                parseErrorAtCurrentToken(diagnosticMessage, arg);
            }

            auto pos = getNodePos();
            auto result =
                    kind == SyntaxKind::Identifier ? factory.createIdentifier("", /*typeArguments*/ {}, /*originalKeywordKind*/ {}) :
                    isTemplateLiteralKind(kind) ? factory.createTemplateLiteralLikeNode(kind, "", "", /*templateFlags*/ {}) :
                    kind == SyntaxKind::NumericLiteral ? factory.createNumericLiteral("", /*numericLiteralFlags*/ {}) :
                    kind == SyntaxKind::StringLiteral ? factory.createStringLiteral("", /*isSingleQuote*/ {}) :
                    kind == SyntaxKind::MissingDeclaration ? factory.createMissingDeclaration() :
                    factory.createToken<Node>(kind);
            return to<T>(finishNode(result, pos));
        }


        string internIdentifier(const string &text) {
            //this was used in the JS version as optimization to not reuse text instances.
            //we do not need that.
            return text;
//            auto identifier = get(identifiers, text);
//            if (!identifier) {
//                identifier = text;
////                identifiers[text] = text;
////                set(identifiers, text, *identifier);
//            }
//            return *identifier;
        }

        bool isBindingIdentifier() {
            if (token() == SyntaxKind::Identifier) {
                return true;
            }

            // `let await`/`let yield` in [Yield] or [Await] are allowed here and disallowed in the binder.
            return token() > SyntaxKind::LastReservedWord;
        }

        shared<Identifier> parseBindingIdentifier(const sharedOpt<DiagnosticMessage> &privateIdentifierDiagnosticMessage = nullptr) {
            ZoneScoped;
            return createIdentifier(isBindingIdentifier(), /*diagnosticMessage*/ {}, privateIdentifierDiagnosticMessage);
        }

        shared<Identifier> parseIdentifierName(const sharedOpt<DiagnosticMessage> &diagnosticMessage = nullptr) {
            return createIdentifier(tokenIsIdentifierOrKeyword(token()), diagnosticMessage);
        }

        bool isAssertionKey() {
            return tokenIsIdentifierOrKeyword(token()) || token() == SyntaxKind::StringLiteral;
        }

        /** Invokes the provided callback.  If the callback returns something falsy, then it restores
         * the parser to the state it was in immediately prior to invoking the callback.  If the
         * callback returns something truthy, then the parser state is not rolled back.  The result
         * of invoking the callback is returned from this function.
         */
        template<typename T>
        T tryParse(const function<T()> &callback) {
            return speculationHelper<T>(callback, SpeculationKind::TryParse);
        }

        bool nextTokenIsClassKeywordOnSameLine() {
            nextToken();
            return token() == SyntaxKind::ClassKeyword && !scanner.hasPrecedingLineBreak();
        }

        bool nextTokenIsFunctionKeywordOnSameLine() {
            nextToken();
            return token() == SyntaxKind::FunctionKeyword && !scanner.hasPrecedingLineBreak();
        }

        bool nextTokenCanFollowDefaultKeyword() {
            nextToken();
            return token() == SyntaxKind::ClassKeyword || token() == SyntaxKind::FunctionKeyword ||
                   token() == SyntaxKind::InterfaceKeyword ||
                   (token() == SyntaxKind::AbstractKeyword && lookAhead<bool>(CALLBACK(nextTokenIsClassKeywordOnSameLine)) ||
                    (token() == SyntaxKind::AsyncKeyword && lookAhead<bool>(CALLBACK(nextTokenIsFunctionKeywordOnSameLine))));
        }

        bool isLiteralPropertyName() {
            return tokenIsIdentifierOrKeyword(token()) ||
                   token() == SyntaxKind::StringLiteral ||
                   token() == SyntaxKind::NumericLiteral;
        }

        bool canFollowModifier() {
            return token() == SyntaxKind::OpenBracketToken
                   || token() == SyntaxKind::OpenBraceToken
                   || token() == SyntaxKind::AsteriskToken
                   || token() == SyntaxKind::DotDotDotToken
                   || isLiteralPropertyName();
        }

        bool canFollowExportModifier() {
            return token() != SyntaxKind::AsteriskToken
                   && token() != SyntaxKind::AsKeyword
                   && token() != SyntaxKind::OpenBraceToken
                   && canFollowModifier();
        }

        bool nextTokenCanFollowExportModifier() {
            nextToken();
            return canFollowExportModifier();
        }

        bool nextTokenIsOnSameLineAndCanFollowModifier() {
            nextToken();
            if (scanner.hasPrecedingLineBreak()) {
                return false;
            }
            return canFollowModifier();
        }

        bool nextTokenCanFollowModifier() {
            switch (token()) {
                case SyntaxKind::ConstKeyword:
                    // 'const' is only a modifier if followed by 'enum'.
                    return nextToken() == SyntaxKind::EnumKeyword;
                case SyntaxKind::ExportKeyword:
                    nextToken();
                    if (token() == SyntaxKind::DefaultKeyword) {
                        return lookAhead<bool>(CALLBACK(nextTokenCanFollowDefaultKeyword));
                    }
                    if (token() == SyntaxKind::TypeKeyword) {
                        return lookAhead<bool>(CALLBACK(nextTokenCanFollowExportModifier));
                    }
                    return canFollowExportModifier();
                case SyntaxKind::DefaultKeyword:
                    return nextTokenCanFollowDefaultKeyword();
                case SyntaxKind::StaticKeyword:
                case SyntaxKind::GetKeyword:
                case SyntaxKind::SetKeyword:
                    nextToken();
                    return canFollowModifier();
                default:
                    return nextTokenIsOnSameLineAndCanFollowModifier();
            }
        }

        bool parseContextualModifier(SyntaxKind t) {
            return token() == t && tryParse<bool>(CALLBACK(nextTokenCanFollowModifier));
        }

        bool nextTokenIsOpenParenOrLessThanOrDot() {
            switch (nextToken()) {
                case SyntaxKind::OpenParenToken:
                case SyntaxKind::LessThanToken:
                case SyntaxKind::DotToken:
                    return true;
            }
            return false;
        }

        // EXPRESSIONS
        bool isStartOfLeftHandSideExpression() {
            switch (token()) {
                case SyntaxKind::ThisKeyword:
                case SyntaxKind::SuperKeyword:
                case SyntaxKind::NullKeyword:
                case SyntaxKind::TrueKeyword:
                case SyntaxKind::FalseKeyword:
                case SyntaxKind::NumericLiteral:
                case SyntaxKind::BigIntLiteral:
                case SyntaxKind::StringLiteral:
                case SyntaxKind::NoSubstitutionTemplateLiteral:
                case SyntaxKind::TemplateHead:
                case SyntaxKind::OpenParenToken:
                case SyntaxKind::OpenBracketToken:
                case SyntaxKind::OpenBraceToken:
                case SyntaxKind::FunctionKeyword:
                case SyntaxKind::ClassKeyword:
                case SyntaxKind::NewKeyword:
                case SyntaxKind::SlashToken:
                case SyntaxKind::SlashEqualsToken:
                case SyntaxKind::Identifier:
                    return true;
                case SyntaxKind::ImportKeyword:
                    return lookAhead<bool>(CALLBACK(nextTokenIsOpenParenOrLessThanOrDot));
                default:
                    return isIdentifier();
            }
        }

        bool inDisallowInContext() {
            return inContext(NodeFlags::DisallowInContext);
        }

        bool isBinaryOperator() {
            if (inDisallowInContext() && token() == SyntaxKind::InKeyword) {
                return false;
            }

            return getBinaryOperatorPrecedence(token()) > 0;
        }

        bool isStartOfExpression() {
            if (isStartOfLeftHandSideExpression()) {
                return true;
            }

            switch (token()) {
                case SyntaxKind::PlusToken:
                case SyntaxKind::MinusToken:
                case SyntaxKind::TildeToken:
                case SyntaxKind::ExclamationToken:
                case SyntaxKind::DeleteKeyword:
                case SyntaxKind::TypeOfKeyword:
                case SyntaxKind::VoidKeyword:
                case SyntaxKind::PlusPlusToken:
                case SyntaxKind::MinusMinusToken:
                case SyntaxKind::LessThanToken:
                case SyntaxKind::AwaitKeyword:
                case SyntaxKind::YieldKeyword:
                case SyntaxKind::PrivateIdentifier:
                    // Yield/await always starts an expression.  Either it is an identifier (in which case
                    // it is definitely an expression).  Or it's a keyword (either because we're in
                    // a generator or async function, or in strict mode (or both)) and it started a yield or await expression.
                    return true;
                default:
                    // Error tolerance.  If we see the start of some binary operator, we consider
                    // that the start of an expression.  That way we'll parse out a missing identifier,
                    // give a good message about an identifier being missing, and then consume the
                    // rest of the binary expression.
                    if (isBinaryOperator()) {
                        return true;
                    }

                    return isIdentifier();
            }
        }

        bool isStartOfExpressionStatement() {
            // As per the grammar, none of '{' or 'function' or 'class' can start an expression statement.
            return token() != SyntaxKind::OpenBraceToken &&
                   token() != SyntaxKind::FunctionKeyword &&
                   token() != SyntaxKind::ClassKeyword &&
                   token() != SyntaxKind::AtToken &&
                   isStartOfExpression();
        }

        bool nextTokenIsNumericOrBigIntLiteral() {
            nextToken();
            return token() == SyntaxKind::NumericLiteral || token() == SyntaxKind::BigIntLiteral;
        }

        bool isBindingIdentifierOrPrivateIdentifierOrPattern() {
            return token() == SyntaxKind::OpenBraceToken
                   || token() == SyntaxKind::OpenBracketToken
                   || token() == SyntaxKind::PrivateIdentifier
                   || isBindingIdentifier();
        }

        bool isStartOfParameter(bool isJSDocParameter) {
            return token() == SyntaxKind::DotDotDotToken ||
                   isBindingIdentifierOrPrivateIdentifierOrPattern() ||
                   isModifierKind(token()) ||
                   token() == SyntaxKind::AtToken ||
                   isStartOfType(/*inStartOfParameter*/ !isJSDocParameter);
        }

        bool isStartOfParenthesizedOrFunctionType() {
            nextToken();
            return token() == SyntaxKind::CloseParenToken || isStartOfParameter(/*isJSDocParameter*/ false) || isStartOfType();
        }

        bool isStartOfType(bool inStartOfParameter = false) {
            switch (token()) {
                case SyntaxKind::AnyKeyword:
                case SyntaxKind::UnknownKeyword:
                case SyntaxKind::StringKeyword:
                case SyntaxKind::NumberKeyword:
                case SyntaxKind::BigIntKeyword:
                case SyntaxKind::BooleanKeyword:
                case SyntaxKind::ReadonlyKeyword:
                case SyntaxKind::SymbolKeyword:
                case SyntaxKind::UniqueKeyword:
                case SyntaxKind::VoidKeyword:
                case SyntaxKind::UndefinedKeyword:
                case SyntaxKind::NullKeyword:
                case SyntaxKind::ThisKeyword:
                case SyntaxKind::TypeOfKeyword:
                case SyntaxKind::NeverKeyword:
                case SyntaxKind::OpenBraceToken:
                case SyntaxKind::OpenBracketToken:
                case SyntaxKind::LessThanToken:
                case SyntaxKind::BarToken:
                case SyntaxKind::AmpersandToken:
                case SyntaxKind::NewKeyword:
                case SyntaxKind::StringLiteral:
                case SyntaxKind::NumericLiteral:
                case SyntaxKind::BigIntLiteral:
                case SyntaxKind::TrueKeyword:
                case SyntaxKind::FalseKeyword:
                case SyntaxKind::ObjectKeyword:
                case SyntaxKind::AsteriskToken:
                case SyntaxKind::QuestionToken:
                case SyntaxKind::ExclamationToken:
                case SyntaxKind::DotDotDotToken:
                case SyntaxKind::InferKeyword:
                case SyntaxKind::ImportKeyword:
                case SyntaxKind::AssertsKeyword:
                case SyntaxKind::NoSubstitutionTemplateLiteral:
                case SyntaxKind::TemplateHead:
                    return true;
                case SyntaxKind::FunctionKeyword:
                    return !inStartOfParameter;
                case SyntaxKind::MinusToken:
                    return !inStartOfParameter && lookAhead<bool>(CALLBACK(nextTokenIsNumericOrBigIntLiteral));
                case SyntaxKind::OpenParenToken:
                    // Only consider '(' the start of a type if followed by ')', '...', an identifier, a modifier,
                    // or something that starts a type. We don't want to consider things like '(1)' a type.
                    return !inStartOfParameter && lookAhead<bool>(CALLBACK(isStartOfParenthesizedOrFunctionType));
                default:
                    return isIdentifier();
            }
        }

        sharedOpt<Node> currentNode(ParsingContext parsingContext) {
            // If we don't have a cursor or the parsing context isn't reusable, there's nothing to reuse.
            //
            // If there is an outstanding parse error that we've encountered, but not attached to
            // some node, then we cannot get a node from the old source tree.  This is because we
            // want to mark the next node we encounter as being unusable.
            //
            // Note: This may be too conservative.  Perhaps we could reuse the node and set the bit
            // on it (or its leftmost child) as having the error.  For now though, being conservative
            // is nice and likely won't ever affect perf.
//            if (!syntaxCursor || !isReusableParsingContext(parsingContext) || parseErrorBeforeNextFinishedNode) {
//                return undefined;
//            }

            //I hope we do not need to reimplement syntaxCursor too
            return nullptr;

//            auto node = syntaxCursor.currentNode(scanner.getStartPos());
//
//            // Can't reuse a missing node.
//            // Can't reuse a node that intersected the change range.
//            // Can't reuse a node that contains a parse error.  This is necessary so that we
//            // produce the same set of errors again.
//            if (nodeIsMissing(node) || node.intersectsChange || containsParseError(node)) {
//                return undefined;
//            }
//
//            // We can only reuse a node if it was parsed under the same strict mode that we're
//            // currently in.  i.e. if we originally parsed a node in non-strict mode, but then
//            // the user added 'using strict' at the top of the file, then we can't use that node
//            // again as the presence of strict mode may cause us to parse the tokens in the file
//            // differently.
//            //
//            // Note: we *can* reuse tokens when the strict mode changes.  That's because tokens
//            // are unaffected by strict mode.  It's just the parser will decide what to do with it
//            // differently depending on what mode it is in.
//            //
//            // This also applies to all our other context flags as well.
//            auto nodeContextFlags = node.flags & NodeFlags::ContextFlags;
//            if (nodeContextFlags != contextFlags) {
//                return undefined;
//            }
//
//            // Ok, we have a node that looks like it could be reused.  Now verify that it is valid
//            // in the current list parsing context that we're currently at.
//            if (!canReuseNode(node, parsingContext)) {
//                return undefined;
//            }
//
//            if (reinterpret_cast<JSDocContainer&>(node).jsDocCache) {
//                // jsDocCache may include tags from parent nodes, which might have been modified.
//                reinterpret_cast<JSDocContainer&>(node).jsDocCache = undefined;
//            }
//
//            return node;
        }

        bool isHeritageClause() {
            return token() == SyntaxKind::ExtendsKeyword || token() == SyntaxKind::ImplementsKeyword;
        }

        bool isTypeMemberStart() {
            // Return true if we have the start of a signature member
            if (token() == SyntaxKind::OpenParenToken ||
                token() == SyntaxKind::LessThanToken ||
                token() == SyntaxKind::GetKeyword ||
                token() == SyntaxKind::SetKeyword) {
                return true;
            }
            auto idToken = false;
            // Eat up all modifiers, but hold on to the last one in case it is actually an identifier
            while (isModifierKind(token())) {
                idToken = true;
                nextToken();
            }
            // Index signatures and computed property names are type members
            if (token() == SyntaxKind::OpenBracketToken) {
                return true;
            }
            // Try to get the first property-like token following all modifiers
            if (isLiteralPropertyName()) {
                idToken = true;
                nextToken();
            }
            // If we were able to get any potential identifier, check that it is
            // the start of a member declaration
            if (idToken) {
                return token() == SyntaxKind::OpenParenToken ||
                       token() == SyntaxKind::LessThanToken ||
                       token() == SyntaxKind::QuestionToken ||
                       token() == SyntaxKind::ColonToken ||
                       token() == SyntaxKind::CommaToken ||
                       canParseSemicolon();
            }
            return false;
        }

        bool nextTokenIsIdentifierOnSameLine() {
            nextToken();
            return !scanner.hasPrecedingLineBreak() && isIdentifier();
        }

        bool nextTokenIsIdentifierOrStringLiteralOnSameLine() {
            nextToken();
            return !scanner.hasPrecedingLineBreak() && (isIdentifier() || token() == SyntaxKind::StringLiteral);
        }

        bool isDeclaration() {
            while (true) {
                switch (token()) {
                    case SyntaxKind::VarKeyword:
                    case SyntaxKind::LetKeyword:
                    case SyntaxKind::ConstKeyword:
                    case SyntaxKind::FunctionKeyword:
                    case SyntaxKind::ClassKeyword:
                    case SyntaxKind::EnumKeyword:
                        return true;

                        // 'declare', 'module', 'namespace', 'interface'* and 'type' are all legal JavaScript identifiers;
                        // however, an identifier cannot be followed by another identifier on the same line. This is what we
                        // count on to parse out the respective declarations. For instance, we exploit this to say that
                        //
                        //    namespace n
                        //
                        // can be none other than the beginning of a namespace declaration, but need to respect that JavaScript sees
                        //
                        //    namespace
                        //    n
                        //
                        // as the identifier 'namespace' on one line followed by the identifier 'n' on another.
                        // We need to look one token ahead to see if it permissible to try parsing a declaration.
                        //
                        // *Note*: 'interface' is actually a strict mode reserved word. So while
                        //
                        //   "use strict"
                        //   interface
                        //   I {}
                        //
                        // could be legal, it would add complexity for very little gain.
                    case SyntaxKind::InterfaceKeyword:
                    case SyntaxKind::TypeKeyword:
                        return nextTokenIsIdentifierOnSameLine();
                    case SyntaxKind::ModuleKeyword:
                    case SyntaxKind::NamespaceKeyword:
                        return nextTokenIsIdentifierOrStringLiteralOnSameLine();
                    case SyntaxKind::AbstractKeyword:
                    case SyntaxKind::AsyncKeyword:
                    case SyntaxKind::DeclareKeyword:
                    case SyntaxKind::PrivateKeyword:
                    case SyntaxKind::ProtectedKeyword:
                    case SyntaxKind::PublicKeyword:
                    case SyntaxKind::ReadonlyKeyword:
                        nextToken();
                        // ASI takes effect for this modifier.
                        if (scanner.hasPrecedingLineBreak()) {
                            return false;
                        }
                        continue;

                    case SyntaxKind::GlobalKeyword:
                        nextToken();
                        return token() == SyntaxKind::OpenBraceToken || token() == SyntaxKind::Identifier || token() == SyntaxKind::ExportKeyword;

                    case SyntaxKind::ImportKeyword:
                        nextToken();
                        return token() == SyntaxKind::StringLiteral || token() == SyntaxKind::AsteriskToken ||
                               token() == SyntaxKind::OpenBraceToken || tokenIsIdentifierOrKeyword(token());
                    case SyntaxKind::ExportKeyword: {
                        auto currentToken = nextToken();
                        if (currentToken == SyntaxKind::TypeKeyword) {
                            currentToken = lookAhead<SyntaxKind>(CALLBACK(nextToken));
                        }
                        if (currentToken == SyntaxKind::EqualsToken || currentToken == SyntaxKind::AsteriskToken ||
                            currentToken == SyntaxKind::OpenBraceToken || currentToken == SyntaxKind::DefaultKeyword ||
                            currentToken == SyntaxKind::AsKeyword) {
                            return true;
                        }
                        continue;
                    }
                    case SyntaxKind::StaticKeyword:
                        nextToken();
                        continue;
                    default:
                        return false;
                }
            }
        }

        bool isStartOfDeclaration() {
            return lookAhead<bool>(CALLBACK(isDeclaration));
        }

        bool nextTokenIsIdentifierOrKeywordOnSameLine() {
            nextToken();
            return tokenIsIdentifierOrKeyword(token()) && !scanner.hasPrecedingLineBreak();
        }

        bool isStartOfStatement() {
            switch (token()) {
                case SyntaxKind::AtToken:
                case SyntaxKind::SemicolonToken:
                case SyntaxKind::OpenBraceToken:
                case SyntaxKind::VarKeyword:
                case SyntaxKind::LetKeyword:
                case SyntaxKind::FunctionKeyword:
                case SyntaxKind::ClassKeyword:
                case SyntaxKind::EnumKeyword:
                case SyntaxKind::IfKeyword:
                case SyntaxKind::DoKeyword:
                case SyntaxKind::WhileKeyword:
                case SyntaxKind::ForKeyword:
                case SyntaxKind::ContinueKeyword:
                case SyntaxKind::BreakKeyword:
                case SyntaxKind::ReturnKeyword:
                case SyntaxKind::WithKeyword:
                case SyntaxKind::SwitchKeyword:
                case SyntaxKind::ThrowKeyword:
                case SyntaxKind::TryKeyword:
                case SyntaxKind::DebuggerKeyword:
                    // 'catch' and 'finally' do not actually indicate that the code is part of a statement,
                    // however, we say they are here so that we may gracefully parse them and error later.
                    // falls through
                case SyntaxKind::CatchKeyword:
                case SyntaxKind::FinallyKeyword:
                    return true;

                case SyntaxKind::ImportKeyword:
                    return isStartOfDeclaration() || lookAhead<bool>(CALLBACK(nextTokenIsOpenParenOrLessThanOrDot));

                case SyntaxKind::ConstKeyword:
                case SyntaxKind::ExportKeyword:
                    return isStartOfDeclaration();

                case SyntaxKind::AsyncKeyword:
                case SyntaxKind::DeclareKeyword:
                case SyntaxKind::InterfaceKeyword:
                case SyntaxKind::ModuleKeyword:
                case SyntaxKind::NamespaceKeyword:
                case SyntaxKind::TypeKeyword:
                case SyntaxKind::GlobalKeyword:
                    // When these don't start a declaration, they're an identifier in an expression statement
                    return true;

                case SyntaxKind::PublicKeyword:
                case SyntaxKind::PrivateKeyword:
                case SyntaxKind::ProtectedKeyword:
                case SyntaxKind::StaticKeyword:
                case SyntaxKind::ReadonlyKeyword:
                    // When these don't start a declaration, they may be the start of a class member if an identifier
                    // immediately follows. Otherwise they're an identifier in an expression statement.
                    return isStartOfDeclaration() || !lookAhead<bool>(CALLBACK(nextTokenIsIdentifierOrKeywordOnSameLine));

                default:
                    return isStartOfExpression();
            }
        }

        bool isClassMemberStart() {
            SyntaxKind idToken = SyntaxKind::Unknown;

            if (token() == SyntaxKind::AtToken) {
                return true;
            }

            // Eat up all modifiers, but hold on to the last one in case it is actually an identifier.
            while (isModifierKind(token())) {
                idToken = token();
                // If the idToken is a class modifier (protected, private, public, and static), it is
                // certain that we are starting to parse class member. This allows better error recovery
                // Example:
                //      public foo() ...     // true
                //      public @dec blah ... // true; we will then report an error later
                //      export public ...    // true; we will then report an error later
                if (isClassMemberModifier(idToken)) {
                    return true;
                }

                nextToken();
            }

            if (token() == SyntaxKind::AsteriskToken) {
                return true;
            }

            // Try to get the first property-like token following all modifiers.
            // This can either be an identifier or the 'get' or 'set' keywords.
            if (isLiteralPropertyName()) {
                idToken = token();
                nextToken();
            }

            // Index signatures and computed properties are class members; we can parse.
            if (token() == SyntaxKind::OpenBracketToken) {
                return true;
            }

            // If we were able to get any potential identifier...
            if (idToken != SyntaxKind::Unknown) {
                // If we have a non-keyword identifier, or if we have an accessor, then it's safe to parse.
                if (!isKeyword(idToken) || idToken == SyntaxKind::SetKeyword || idToken == SyntaxKind::GetKeyword) {
                    return true;
                }

                // If it *is* a keyword, but not an accessor, check a little farther along
                // to see if it should actually be parsed as a class member.
                switch (token()) {
                    case SyntaxKind::OpenParenToken:     // Method declaration
                    case SyntaxKind::LessThanToken:      // Generic Method declaration
                    case SyntaxKind::ExclamationToken:   // Non-null assertion on property name
                    case SyntaxKind::ColonToken:         // Type Annotation for declaration
                    case SyntaxKind::EqualsToken:        // Initializer for declaration
                    case SyntaxKind::QuestionToken:      // Not valid, but permitted so that it gets caught later on.
                        return true;
                    default:
                        // Covers
                        //  - Semicolons     (declaration termination)
                        //  - Closing braces (end-of-class, must be declaration)
                        //  - End-of-files   (not valid, but permitted so that it gets caught later on)
                        //  - Line-breaks    (enabling *automatic semicolon insertion*)
                        return canParseSemicolon();
                }
            }

            return false;
        }

        bool isValidHeritageClauseObjectLiteral() {
            assert(token() == SyntaxKind::OpenBraceToken);
            if (nextToken() == SyntaxKind::CloseBraceToken) {
                // if we see "extends {}" then only treat the {} as what we're extending (and not
                // the class body) if we have:
                //
                //      extends {} {
                //      extends {},
                //      extends {} extends
                //      extends {} implements

                auto next = nextToken();
                return next == SyntaxKind::CommaToken || next == SyntaxKind::OpenBraceToken || next == SyntaxKind::ExtendsKeyword || next == SyntaxKind::ImplementsKeyword;
            }

            return true;
        }

        bool nextTokenIsStartOfExpression() {
            nextToken();
            return isStartOfExpression();
        }

        bool isHeritageClauseExtendsOrImplementsKeyword() {
            if (token() == SyntaxKind::ImplementsKeyword ||
                token() == SyntaxKind::ExtendsKeyword) {

                return lookAhead<bool>(CALLBACK(nextTokenIsStartOfExpression));
            }

            return false;
        }

        // True if positioned at the start of a list element
        bool isListElement(ParsingContext parsingContext, bool inErrorRecovery) {
            ZoneScoped;
            auto node = currentNode(parsingContext);
            if (node) {
                return true;
            }

            switch (parsingContext) {
                case ParsingContext::SourceElements:
                case ParsingContext::BlockStatements:
                case ParsingContext::SwitchClauseStatements:
                    // If we're in error recovery, then we don't want to treat ';' as an empty statement.
                    // The problem is that ';' can show up in far too many contexts, and if we see one
                    // and assume it's a statement, then we may bail out inappropriately from whatever
                    // we're parsing.  For example, if we have a semicolon in the middle of a class, then
                    // we really don't want to assume the class is over and we're on a statement in the
                    // outer module.  We just want to consume and move on.
                    return !(token() == SyntaxKind::SemicolonToken && inErrorRecovery) && isStartOfStatement();
                case ParsingContext::SwitchClauses:
                    return token() == SyntaxKind::CaseKeyword || token() == SyntaxKind::DefaultKeyword;
                case ParsingContext::TypeMembers:
                    return lookAhead<bool>(CALLBACK(isTypeMemberStart));
                case ParsingContext::ClassMembers:
                    // We allow semicolons as class elements (as specified by ES6) as long as we're
                    // not in error recovery.  If we're in error recovery, we don't want an errant
                    // semicolon to be treated as a class member (since they're almost always used
                    // for statements.
                    return lookAhead<bool>(CALLBACK(isClassMemberStart)) || (token() == SyntaxKind::SemicolonToken && !inErrorRecovery);
                case ParsingContext::EnumMembers:
                    // Include open bracket computed properties. This technically also lets in indexers,
                    // which would be a candidate for improved error reporting.
                    return token() == SyntaxKind::OpenBracketToken || isLiteralPropertyName();
                case ParsingContext::ObjectLiteralMembers:
                    switch (token()) {
                        case SyntaxKind::OpenBracketToken:
                        case SyntaxKind::AsteriskToken:
                        case SyntaxKind::DotDotDotToken:
                        case SyntaxKind::DotToken: // Not an object literal member, but don't want to close the object (see `tests/cases/fourslash/completionsDotInObjectLiteral.ts`)
                            return true;
                        default:
                            return isLiteralPropertyName();
                    }
                case ParsingContext::RestProperties:
                    return isLiteralPropertyName();
                case ParsingContext::ObjectBindingElements:
                    return token() == SyntaxKind::OpenBracketToken || token() == SyntaxKind::DotDotDotToken || isLiteralPropertyName();
                case ParsingContext::AssertEntries:
                    return isAssertionKey();
                case ParsingContext::HeritageClauseElement:
                    // If we see `{ ... }` then only consume it as an expression if it is followed by `,` or `{`
                    // That way we won't consume the body of a class in its heritage clause.
                    if (token() == SyntaxKind::OpenBraceToken) {
                        return lookAhead<bool>(CALLBACK(isValidHeritageClauseObjectLiteral));
                    }

                    if (!inErrorRecovery) {
                        return isStartOfLeftHandSideExpression() && !isHeritageClauseExtendsOrImplementsKeyword();
                    } else {
                        // If we're in error recovery we tighten up what we're willing to match.
                        // That way we don't treat something like "this" as a valid heritage clause
                        // element during recovery.
                        return isIdentifier() && !isHeritageClauseExtendsOrImplementsKeyword();
                    }
                case ParsingContext::VariableDeclarations:
                    return isBindingIdentifierOrPrivateIdentifierOrPattern();
                case ParsingContext::ArrayBindingElements:
                    return token() == SyntaxKind::CommaToken || token() == SyntaxKind::DotDotDotToken || isBindingIdentifierOrPrivateIdentifierOrPattern();
                case ParsingContext::TypeParameters:
                    return token() == SyntaxKind::InKeyword || isIdentifier();
                case ParsingContext::ArrayLiteralMembers:
                    switch (token()) {
                        case SyntaxKind::CommaToken:
                        case SyntaxKind::DotToken: // Not an array literal member, but don't want to close the array (see `tests/cases/fourslash/completionsDotInArrayLiteralInObjectLiteral.ts`)
                            return true;
                    }
                    // falls through
                case ParsingContext::ArgumentExpressions:
                    return token() == SyntaxKind::DotDotDotToken || isStartOfExpression();
                case ParsingContext::Parameters:
                    return isStartOfParameter(/*isJSDocParameter*/ false);
                case ParsingContext::JSDocParameters:
                    return isStartOfParameter(/*isJSDocParameter*/ true);
                case ParsingContext::TypeArguments:
                case ParsingContext::TupleElementTypes:
                    return token() == SyntaxKind::CommaToken || isStartOfType();
                case ParsingContext::HeritageClauses:
                    return isHeritageClause();
                case ParsingContext::ImportOrExportSpecifiers:
                    return tokenIsIdentifierOrKeyword(token());
                case ParsingContext::JsxAttributes:
                    return tokenIsIdentifierOrKeyword(token()) || token() == SyntaxKind::OpenBraceToken;
                case ParsingContext::JsxChildren:
                    return true;
            }

            throw runtime_error("Non-exhaustive case in 'isListElement'.");
        }

        bool nextTokenIsIdentifier() {
            nextToken();
            return isIdentifier();
        }

        bool nextTokenIsIdentifierOrKeyword() {
            nextToken();
            return tokenIsIdentifierOrKeyword(token());
        }

        bool nextTokenIsIdentifierOrKeywordOrGreaterThan() {
            nextToken();
            return tokenIsIdentifierOrKeywordOrGreaterThan(token());
        }

        bool nextTokenIsStartOfType() {
            nextToken();
            return isStartOfType();
        }

        bool isInOrOfKeyword(SyntaxKind t) {
            return t == SyntaxKind::InKeyword || t == SyntaxKind::OfKeyword;
        }

        bool isVariableDeclaratorListTerminator() {
            // If we can consume a semicolon (either explicitly, or with ASI), then consider us done
            // with parsing the list of variable declarators.
            if (canParseSemicolon()) {
                return true;
            }

            // in the case where we're parsing the variable declarator of a 'for-in' statement, we
            // are done if we see an 'in' keyword in front of us. Same with for-of
            if (isInOrOfKeyword(token())) {
                return true;
            }

            // ERROR RECOVERY TWEAK:
            // For better error recovery, if we see an '=>' then we just stop immediately.  We've got an
            // arrow function here and it's going to be very unlikely that we'll resynchronize and get
            // another variable declaration.
            if (token() == SyntaxKind::EqualsGreaterThanToken) {
                return true;
            }

            // Keep trying to parse out variable declarators.
            return false;
        }

        bool nextTokenIsOpenParen() {
            return nextToken() == SyntaxKind::OpenParenToken;
        }

        bool isExternalModuleReference() {
            return token() == SyntaxKind::RequireKeyword && lookAhead<bool>(CALLBACK(nextTokenIsOpenParen));
        }

        bool nextTokenIsOpenBrace() {
            return nextToken() == SyntaxKind::OpenBraceToken;
        }

        bool nextTokenIsSlash() {
            return nextToken() == SyntaxKind::SlashToken;
        }

        // True if positioned at a list terminator
        bool isListTerminator(ParsingContext kind) {
            if (token() == SyntaxKind::EndOfFileToken) {
                // Being at the end of the file ends all lists.
                return true;
            }

            switch (kind) {
                case ParsingContext::BlockStatements:
                case ParsingContext::SwitchClauses:
                case ParsingContext::TypeMembers:
                case ParsingContext::ClassMembers:
                case ParsingContext::EnumMembers:
                case ParsingContext::ObjectLiteralMembers:
                case ParsingContext::ObjectBindingElements:
                case ParsingContext::ImportOrExportSpecifiers:
                case ParsingContext::AssertEntries:
                    return token() == SyntaxKind::CloseBraceToken;
                case ParsingContext::SwitchClauseStatements:
                    return token() == SyntaxKind::CloseBraceToken || token() == SyntaxKind::CaseKeyword || token() == SyntaxKind::DefaultKeyword;
                case ParsingContext::HeritageClauseElement:
                    return token() == SyntaxKind::OpenBraceToken || token() == SyntaxKind::ExtendsKeyword || token() == SyntaxKind::ImplementsKeyword;
                case ParsingContext::VariableDeclarations:
                    return isVariableDeclaratorListTerminator();
                case ParsingContext::TypeParameters:
                    // Tokens other than '>' are here for better error recovery
                    return token() == SyntaxKind::GreaterThanToken || token() == SyntaxKind::OpenParenToken || token() == SyntaxKind::OpenBraceToken || token() == SyntaxKind::ExtendsKeyword || token() == SyntaxKind::ImplementsKeyword;
                case ParsingContext::ArgumentExpressions:
                    // Tokens other than ')' are here for better error recovery
                    return token() == SyntaxKind::CloseParenToken || token() == SyntaxKind::SemicolonToken;
                case ParsingContext::ArrayLiteralMembers:
                case ParsingContext::TupleElementTypes:
                case ParsingContext::ArrayBindingElements:
                    return token() == SyntaxKind::CloseBracketToken;
                case ParsingContext::JSDocParameters:
                case ParsingContext::Parameters:
                case ParsingContext::RestProperties:
                    // Tokens other than ')' and ']' (the latter for index signatures) are here for better error recovery
                    return token() == SyntaxKind::CloseParenToken || token() == SyntaxKind::CloseBracketToken /*|| token == SyntaxKind::OpenBraceToken*/;
                case ParsingContext::TypeArguments:
                    // All other tokens should cause the type-argument to terminate except comma token
                    return token() != SyntaxKind::CommaToken;
                case ParsingContext::HeritageClauses:
                    return token() == SyntaxKind::OpenBraceToken || token() == SyntaxKind::CloseBraceToken;
                case ParsingContext::JsxAttributes:
                    return token() == SyntaxKind::GreaterThanToken || token() == SyntaxKind::SlashToken;
                case ParsingContext::JsxChildren:
                    return token() == SyntaxKind::LessThanToken && lookAhead<bool>(CALLBACK(nextTokenIsSlash));
                default:
                    return false;
            }
        }

        shared<Node> consumeNode(shared<Node> node) {
            // Move the scanner so it is after the node we just consumed.
            scanner.setTextPos(node->end);
            nextToken();
            return node;
        }

//        function isReusableParsingContext(parsingContext: ParsingContext): boolean {
//            switch (parsingContext) {
//                case ParsingContext::ClassMembers:
//                case ParsingContext::SwitchClauses:
//                case ParsingContext::SourceElements:
//                case ParsingContext::BlockStatements:
//                case ParsingContext::SwitchClauseStatements:
//                case ParsingContext::EnumMembers:
//                case ParsingContext::TypeMembers:
//                case ParsingContext::VariableDeclarations:
//                case ParsingContext::JSDocParameters:
//                case ParsingContext::Parameters:
//                    return true;
//            }
//            return false;
//        }
//
//        function canReuseNode(node: Node, parsingContext: ParsingContext): boolean {
//            switch (parsingContext) {
//                case ParsingContext::ClassMembers:
//                    return isReusableClassMember(node);
//
//                case ParsingContext::SwitchClauses:
//                    return isReusableSwitchClause(node);
//
//                case ParsingContext::SourceElements:
//                case ParsingContext::BlockStatements:
//                case ParsingContext::SwitchClauseStatements:
//                    return isReusableStatement(node);
//
//                case ParsingContext::EnumMembers:
//                    return isReusableEnumMember(node);
//
//                case ParsingContext::TypeMembers:
//                    return isReusableTypeMember(node);
//
//                case ParsingContext::VariableDeclarations:
//                    return isReusableVariableDeclaration(node);
//
//                case ParsingContext::JSDocParameters:
//                case ParsingContext::Parameters:
//                    return isReusableParameter(node);
//
//                // Any other lists we do not care about reusing nodes in.  But feel free to add if
//                // you can do so safely.  Danger areas involve nodes that may involve speculative
//                // parsing.  If speculative parsing is involved with the node, then the range the
//                // parser reached while looking ahead might be in the edited range (see the example
//                // in canReuseVariableDeclaratorNode for a good case of this).
//
//                // case ParsingContext::HeritageClauses:
//                // This would probably be safe to reuse.  There is no speculative parsing with
//                // heritage clauses.
//
//                // case ParsingContext::TypeParameters:
//                // This would probably be safe to reuse.  There is no speculative parsing with
//                // type parameters.  Note that that's because type *parameters* only occur in
//                // unambiguous *type* contexts.  While type *arguments* occur in very ambiguous
//                // *expression* contexts.
//
//                // case ParsingContext::TupleElementTypes:
//                // This would probably be safe to reuse.  There is no speculative parsing with
//                // tuple types.
//
//                // Technically, type argument list types are probably safe to reuse.  While
//                // speculative parsing is involved with them (since type argument lists are only
//                // produced from speculative parsing a < as a type argument list), we only have
//                // the types because speculative parsing succeeded.  Thus, the lookahead never
//                // went past the end of the list and rewound.
//                // case ParsingContext::TypeArguments:
//
//                // Note: these are almost certainly not safe to ever reuse.  Expressions commonly
//                // need a large amount of lookahead, and we should not reuse them as they may
//                // have actually intersected the edit.
//                // case ParsingContext::ArgumentExpressions:
//
//                // This is not safe to reuse for the same reason as the 'AssignmentExpression'
//                // cases.  i.e. a property assignment may end with an expression, and thus might
//                // have lookahead far beyond it's old node.
//                // case ParsingContext::ObjectLiteralMembers:
//
//                // This is probably not safe to reuse.  There can be speculative parsing with
//                // type names in a heritage clause.  There can be generic names in the type
//                // name list, and there can be left hand side expressions (which can have type
//                // arguments.)
//                // case ParsingContext::HeritageClauseElement:
//
//                // Perhaps safe to reuse, but it's unlikely we'd see more than a dozen attributes
//                // on any given element. Same for children.
//                // case ParsingContext::JsxAttributes:
//                // case ParsingContext::JsxChildren:
//
//            }
//
//            return false;
//        }
//
//        function isReusableClassMember(node: Node) {
//            if (node) {
//                switch (node.kind) {
//                    case SyntaxKind::Constructor:
//                    case SyntaxKind::IndexSignature:
//                    case SyntaxKind::GetAccessor:
//                    case SyntaxKind::SetAccessor:
//                    case SyntaxKind::PropertyDeclaration:
//                    case SyntaxKind::SemicolonClassElement:
//                        return true;
//                    case SyntaxKind::MethodDeclaration:
//                        // Method declarations are not necessarily reusable.  An object-literal
//                        // may have a method calls "constructor(...)" and we must reparse that
//                        // into an actual .ConstructorDeclaration.
//                        auto methodDeclaration = node as MethodDeclaration;
//                        auto nameIsConstructor = methodDeclaration.name.kind == SyntaxKind::Identifier &&
//                            methodDeclaration.name.originalKeywordKind == SyntaxKind::ConstructorKeyword;
//
//                        return !nameIsConstructor;
//                }
//            }
//
//            return false;
//        }
//
//        function isReusableSwitchClause(node: Node) {
//            if (node) {
//                switch (node.kind) {
//                    case SyntaxKind::CaseClause:
//                    case SyntaxKind::DefaultClause:
//                        return true;
//                }
//            }
//
//            return false;
//        }
//
//        function isReusableStatement(node: Node) {
//            if (node) {
//                switch (node.kind) {
//                    case SyntaxKind::FunctionDeclaration:
//                    case SyntaxKind::VariableStatement:
//                    case SyntaxKind::Block:
//                    case SyntaxKind::IfStatement:
//                    case SyntaxKind::ExpressionStatement:
//                    case SyntaxKind::ThrowStatement:
//                    case SyntaxKind::ReturnStatement:
//                    case SyntaxKind::SwitchStatement:
//                    case SyntaxKind::BreakStatement:
//                    case SyntaxKind::ContinueStatement:
//                    case SyntaxKind::ForInStatement:
//                    case SyntaxKind::ForOfStatement:
//                    case SyntaxKind::ForStatement:
//                    case SyntaxKind::WhileStatement:
//                    case SyntaxKind::WithStatement:
//                    case SyntaxKind::EmptyStatement:
//                    case SyntaxKind::TryStatement:
//                    case SyntaxKind::LabeledStatement:
//                    case SyntaxKind::DoStatement:
//                    case SyntaxKind::DebuggerStatement:
//                    case SyntaxKind::ImportDeclaration:
//                    case SyntaxKind::ImportEqualsDeclaration:
//                    case SyntaxKind::ExportDeclaration:
//                    case SyntaxKind::ExportAssignment:
//                    case SyntaxKind::ModuleDeclaration:
//                    case SyntaxKind::ClassDeclaration:
//                    case SyntaxKind::InterfaceDeclaration:
//                    case SyntaxKind::EnumDeclaration:
//                    case SyntaxKind::TypeAliasDeclaration:
//                        return true;
//                }
//            }
//
//            return false;
//        }
//
//        function isReusableEnumMember(node: Node) {
//            return node.kind == SyntaxKind::EnumMember;
//        }
//
//        function isReusableTypeMember(node: Node) {
//            if (node) {
//                switch (node.kind) {
//                    case SyntaxKind::ConstructSignature:
//                    case SyntaxKind::MethodSignature:
//                    case SyntaxKind::IndexSignature:
//                    case SyntaxKind::PropertySignature:
//                    case SyntaxKind::CallSignature:
//                        return true;
//                }
//            }
//
//            return false;
//        }
//
//        function isReusableVariableDeclaration(node: Node) {
//            if (node.kind != SyntaxKind::VariableDeclaration) {
//                return false;
//            }
//
//            // Very subtle incremental parsing bug.  Consider the following code:
//            //
//            //      auto v = new List < A, B
//            //
//            // This is actually legal code.  It's a list of variable declarators "v = new List<A"
//            // on one side and "B" on the other. If you then change that to:
//            //
//            //      auto v = new List < A, B >()
//            //
//            // then we have a problem.  "v = new List<A" doesn't intersect the change range, so we
//            // start reparsing at "B" and we completely fail to handle this properly.
//            //
//            // In order to prevent this, we do not allow a variable declarator to be reused if it
//            // has an initializer.
//            auto variableDeclarator = node as VariableDeclaration;
//            return variableDeclarator.initializer == undefined;
//        }
//
//        function isReusableParameter(node: Node) {
//            if (node.kind != SyntaxKind::Parameter) {
//                return false;
//            }
//
//            // See the comment in isReusableVariableDeclaration for why we do this.
//            auto parameter = node as ParameterDeclaration;
//            return parameter.initializer == undefined;
//        }
//
        sharedOpt<DiagnosticMessage> getExpectedCommaDiagnostic(ParsingContext kind) {
            if (kind == ParsingContext::EnumMembers) return Diagnostics::An_enum_member_name_must_be_followed_by_a_or;
            return nullptr;
        }

        bool parseExpected(SyntaxKind kind, const sharedOpt<DiagnosticMessage> &diagnosticMessage = nullptr, bool shouldAdvance = true) {
            if (token() == kind) {
                if (shouldAdvance) {
                    nextToken();
                }
                return true;
            }

            // Report specific message if provided with one.  Otherwise, report generic fallback message.
            if (diagnosticMessage) {
                parseErrorAtCurrentToken(diagnosticMessage);
            } else {
                parseErrorAtCurrentToken(Diagnostics::_0_expected, tokenToString(kind));
            }
            return false;
        }

        optional<DiagnosticWithDetachedLocation> parsingContextErrors(ParsingContext context) {
            switch (context) {
                case ParsingContext::SourceElements:
                    return token() == SyntaxKind::DefaultKeyword
                           ? parseErrorAtCurrentToken(Diagnostics::_0_expected, tokenToString(SyntaxKind::ExportKeyword))
                           : parseErrorAtCurrentToken(Diagnostics::Declaration_or_statement_expected);
                case ParsingContext::BlockStatements:
                    return parseErrorAtCurrentToken(Diagnostics::Declaration_or_statement_expected);
                case ParsingContext::SwitchClauses:
                    return parseErrorAtCurrentToken(Diagnostics::case_or_default_expected);
                case ParsingContext::SwitchClauseStatements:
                    return parseErrorAtCurrentToken(Diagnostics::Statement_expected);
                case ParsingContext::RestProperties: // fallthrough
                case ParsingContext::TypeMembers:
                    return parseErrorAtCurrentToken(Diagnostics::Property_or_signature_expected);
                case ParsingContext::ClassMembers:
                    return parseErrorAtCurrentToken(Diagnostics::Unexpected_token_A_constructor_method_accessor_or_property_was_expected);
                case ParsingContext::EnumMembers:
                    return parseErrorAtCurrentToken(Diagnostics::Enum_member_expected);
                case ParsingContext::HeritageClauseElement:
                    return parseErrorAtCurrentToken(Diagnostics::Expression_expected);
                case ParsingContext::VariableDeclarations:
                    return isKeyword(token())
                           ? parseErrorAtCurrentToken(Diagnostics::_0_is_not_allowed_as_a_variable_declaration_name, tokenToString(token()))
                           : parseErrorAtCurrentToken(Diagnostics::Variable_declaration_expected);
                case ParsingContext::ObjectBindingElements:
                    return parseErrorAtCurrentToken(Diagnostics::Property_destructuring_pattern_expected);
                case ParsingContext::ArrayBindingElements:
                    return parseErrorAtCurrentToken(Diagnostics::Array_element_destructuring_pattern_expected);
                case ParsingContext::ArgumentExpressions:
                    return parseErrorAtCurrentToken(Diagnostics::Argument_expression_expected);
                case ParsingContext::ObjectLiteralMembers:
                    return parseErrorAtCurrentToken(Diagnostics::Property_assignment_expected);
                case ParsingContext::ArrayLiteralMembers:
                    return parseErrorAtCurrentToken(Diagnostics::Expression_or_comma_expected);
                case ParsingContext::JSDocParameters:
                    return parseErrorAtCurrentToken(Diagnostics::Parameter_declaration_expected);
                case ParsingContext::Parameters:
                    return isKeyword(token())
                           ? parseErrorAtCurrentToken(Diagnostics::_0_is_not_allowed_as_a_parameter_name, tokenToString(token()))
                           : parseErrorAtCurrentToken(Diagnostics::Parameter_declaration_expected);
                case ParsingContext::TypeParameters:
                    return parseErrorAtCurrentToken(Diagnostics::Type_parameter_declaration_expected);
                case ParsingContext::TypeArguments:
                    return parseErrorAtCurrentToken(Diagnostics::Type_argument_expected);
                case ParsingContext::TupleElementTypes:
                    return parseErrorAtCurrentToken(Diagnostics::Type_expected);
                case ParsingContext::HeritageClauses:
                    return parseErrorAtCurrentToken(Diagnostics::Unexpected_token_expected);
                case ParsingContext::ImportOrExportSpecifiers:
                    return parseErrorAtCurrentToken(Diagnostics::Identifier_expected);
                case ParsingContext::JsxAttributes:
                    return parseErrorAtCurrentToken(Diagnostics::Identifier_expected);
                case ParsingContext::JsxChildren:
                    return parseErrorAtCurrentToken(Diagnostics::Identifier_expected);
                default:
                    return nullopt; // TODO: GH#18217 `default: Debug::assertsNever(context);`
            }
        }

        // True if positioned at element or terminator of the current list or any enclosing list
        bool isInSomeParsingContext() {
            for (int kind = 0; kind < (int) ParsingContext::Count; kind++) {
                if (parsingContext & (1 << kind)) {
                    if (isListElement((ParsingContext) kind, /*inErrorRecovery*/ true) || isListTerminator((ParsingContext) kind)) {
                        return true;
                    }
                }
            }

            return false;
        }

        // Returns true if we should abort parsing.
        bool abortParsingListOrMoveToNextToken(ParsingContext kind) {
            parsingContextErrors(kind);
            if (isInSomeParsingContext()) {
                return true;
            }

            nextToken();
            return false;
        }

        template<class T>
        sharedOpt<T> parseListElement(ParsingContext parsingContext, function<shared<T>()> parseElement) {
            ZoneScoped;
            auto node = currentNode(parsingContext);
            if (node) {
                return consumeNode(node);
            }

            return parseElement();
        }

        // Parses a list of elements
        shared<NodeArray> parseList(ParsingContext kind, const function<shared<Node>()> &parseElement) {
            ZoneScoped;
            int saveParsingContext = parsingContext;
            parsingContext |= 1 << (int) kind;
            auto list = make_shared<NodeArray>();
            auto listPos = getNodePos();

            while (!isListTerminator(kind)) {
                if (isListElement(kind, /*inErrorRecovery*/ false)) {
                    auto n = parseListElement(kind, parseElement);
                    if (!n) throw runtime_error("No node given");
                    list->push(n);

                    continue;
                }

                if (abortParsingListOrMoveToNextToken(kind)) {
                    break;
                }
            }

            parsingContext = saveParsingContext;
            return createNodeArray(list, listPos);
        }

//        // Parses a comma-delimited list of elements
//        function parseDelimitedList<T extends Node>(kind: ParsingContext, parseElement: () => T, considerSemicolonAsDelimiter?: boolean): NodeArray<T>;
//        function parseDelimitedList<T extends Node | undefined>(kind: ParsingContext, parseElement: () => T, considerSemicolonAsDelimiter?: boolean): NodeArray<NonNullable<T>> | undefined;
        sharedOpt<NodeArray> parseDelimitedList(ParsingContext kind, function<sharedOpt<Node>()> parseElement, optional<bool> considerSemicolonAsDelimiter = {}) {
            ZoneScoped;
            auto saveParsingContext = parsingContext;
            parsingContext |= 1 << (int) kind;
            auto list = make_shared<NodeArray>();
            auto listPos = getNodePos();

            int commaStart = -1; // Meaning the previous token was not a comma
            while (true) {
                if (isListElement(kind, /*inErrorRecovery*/ false)) {
                    auto startPos = scanner.getStartPos();
                    auto result = parseListElement(kind, parseElement);
                    if (!result) {
                        parsingContext = saveParsingContext;
                        return nullptr;
                    }
                    list->push(result);
                    commaStart = scanner.getTokenPos();

                    if (parseOptional(SyntaxKind::CommaToken)) {
                        // No need to check for a zero length node since we know we parsed a comma
                        continue;
                    }

                    commaStart = -1; // Back to the state where the last token was not a comma
                    if (isListTerminator(kind)) {
                        break;
                    }

                    // We didn't get a comma, and the list wasn't terminated, explicitly parse
                    // out a comma so we give a good error message.
                    parseExpected(SyntaxKind::CommaToken, getExpectedCommaDiagnostic(kind));

                    // If the token was a semicolon, and the caller allows that, then skip it and
                    // continue.  This ensures we get back on track and don't result in tons of
                    // parse errors.  For example, this can happen when people do things like use
                    // a semicolon to delimit object literal members.   Note: we'll have already
                    // reported an error when we called parseExpected above.
                    if (considerSemicolonAsDelimiter && token() == SyntaxKind::SemicolonToken && !scanner.hasPrecedingLineBreak()) {
                        nextToken();
                    }
                    if (startPos == scanner.getStartPos()) {
                        // What we're parsing isn't actually remotely recognizable as a element and we've consumed no tokens whatsoever
                        // Consume a token to advance the parser in some way and avoid an infinite loop
                        // This can happen when we're speculatively parsing parenthesized expressions which we think may be arrow functions,
                        // or when a modifier keyword which is disallowed as a parameter name (ie, `static` in strict mode) is supplied
                        nextToken();
                    }
                    continue;
                }

                if (isListTerminator(kind)) {
                    break;
                }

                if (abortParsingListOrMoveToNextToken(kind)) {
                    break;
                }
            }

            parsingContext = saveParsingContext;
            // Recording the trailing comma is deliberately done after the previous
            // loop, and not just if we see a list terminator. This is because the list
            // may have ended incorrectly, but it is still important to know if there
            // was a trailing comma.
            // Check if the last token was a comma.
            // Always preserve a trailing comma by marking it on the NodeArray
            return createNodeArray(list, listPos, /*end*/ {}, commaStart >= 0);
        }

//        interface MissingList<T extends Node> extends NodeArray<T> {
//            isMissingList: true;
//        }

        shared<NodeArray> createMissingList() {
            auto list = createNodeArray(make_shared<NodeArray>(), getNodePos());
            list->isMissingList = true;
            return list;
        }

        bool isMissingList(const shared<NodeArray> &arr) {
            return arr->isMissingList;
        }

        // An identifier that starts with two underscores has an extra underscore character prepended to it to avoid issues
        // with magic property names like '__proto__'. The 'identifiers' object is used to share a single string instance for
        // each identifier in order to reduce memory consumption.
        shared<Identifier> createIdentifier(bool isIdentifier, const sharedOpt<DiagnosticMessage> &diagnosticMessage = nullptr, const sharedOpt<DiagnosticMessage> &privateIdentifierDiagnosticMessage = nullptr) {
            ZoneScoped;
            if (isIdentifier) {
                identifierCount++;
                auto pos = getNodePos();
                // Store original token kind if it is not just an Identifier so we can report appropriate error later in type checker
                auto originalKeywordKind = token();
                auto text = internIdentifier(scanner.getTokenValue());
                nextTokenWithoutCheck();
                return finishNode(factory.createIdentifier(text, /*typeArguments*/ {}, originalKeywordKind), pos);
            }

            if (token() == SyntaxKind::PrivateIdentifier) {
                parseErrorAtCurrentToken(privateIdentifierDiagnosticMessage ? privateIdentifierDiagnosticMessage : Diagnostics::Private_identifiers_are_not_allowed_outside_class_bodies);
                return createIdentifier(/*isIdentifier*/ true);
            }

            if (token() == SyntaxKind::Unknown && scanner.tryScan<bool>([this]() { return scanner.reScanInvalidIdentifier() == SyntaxKind::Identifier; })) {
                // Scanner has already recorded an 'Invalid character' error, so no need to add another from the parser.
                return createIdentifier(/*isIdentifier*/ true);
            }

            identifierCount++;
            // Only for end of file because the error gets reported incorrectly on embedded script tags.
            auto reportAtCurrentPosition = token() == SyntaxKind::EndOfFileToken;

            auto isReservedWord = scanner.isReservedWord();
            auto msgArg = scanner.getTokenText();

            auto defaultMessage = isReservedWord ?
                                  Diagnostics::Identifier_expected_0_is_a_reserved_word_that_cannot_be_used_here :
                                  Diagnostics::Identifier_expected;

            return createMissingNode<Identifier>(SyntaxKind::Identifier, reportAtCurrentPosition, diagnosticMessage ? diagnosticMessage : defaultMessage);
        }

        shared<Identifier> parseIdentifier(const sharedOpt<DiagnosticMessage> &diagnosticMessage = nullptr, const sharedOpt<DiagnosticMessage> &privateIdentifierDiagnosticMessage = nullptr) {
            return createIdentifier(isIdentifier(), diagnosticMessage, privateIdentifierDiagnosticMessage);
        }

        template<typename T>
        T doOutsideOfContext(int context, function<T()> func) {
            // contextFlagsToClear will contain only the context flags that are
            // currently set that we need to temporarily clear
            // We don't just blindly reset to the previous flags to ensure
            // that we do not mutate cached flags for the incremental
            // parser (ThisNodeHasError, ThisNodeOrAnySubNodesHasError, and
            // HasAggregatedChildData).
            auto contextFlagsToClear = context & contextFlags;
            if (contextFlagsToClear) {
                // clear the requested context flags
                setContextFlag(/*val*/ false, contextFlagsToClear);
                auto result = func();
                // restore the context flags we just cleared
                setContextFlag(/*val*/ true, contextFlagsToClear);
                return result;
            }

            // no need to do anything special as we are not in any of the requested contexts
            return func();
        }

        template<typename T>
        T doOutsideOfContext(NodeFlags context, const function<T()> &func) {
            return doOutsideOfContext<T>((int) context, func);
        }

        template<typename T>
        T doInsideOfContext(/*NodeFlags*/int context, function<T()> func) {
            ZoneScoped;
            // contextFlagsToSet will contain only the context flags that
            // are not currently set that we need to temporarily enable.
            // We don't just blindly reset to the previous flags to ensure
            // that we do not mutate cached flags for the incremental
            // parser (ThisNodeHasError, ThisNodeOrAnySubNodesHasError, and
            // HasAggregatedChildData).
            auto contextFlagsToSet = context & ~contextFlags;
            if (contextFlagsToSet) {
                // set the requested context flags
                setContextFlag(/*val*/ true, contextFlagsToSet);
                auto result = func();
                // reset the context flags we just set
                setContextFlag(/*val*/ false, contextFlagsToSet);
                return result;
            }

            // no need to do anything special as we are already in all of the requested contexts
            return func();
        }

        template<typename T>
        T allowInAnd(const function<T()> &func) {
            return doOutsideOfContext<T>((int) NodeFlags::DisallowInContext, func);
        }

        template<typename T>
        T disallowInAnd(const function<T()> &func) {
            return doInsideOfContext<T>((int) NodeFlags::DisallowInContext, func);
        }

        template<typename T>
        T allowConditionalTypesAnd(const function<T()> &func) {
            return doOutsideOfContext<T>((int) NodeFlags::DisallowConditionalTypesContext, func);
        }
        template<typename T>
        T disallowConditionalTypesAnd(const function<T()> &func) {
            return doInsideOfContext<T>((int) NodeFlags::DisallowConditionalTypesContext, func);
        }
        template<typename T>
        T doInYieldContext(const function<T()> &func) {
            return doInsideOfContext<T>((int) NodeFlags::YieldContext, func);
        }
        template<typename T>
        T doInDecoratorContext(const function<T()> &func) {
            return doInsideOfContext<T>((int) NodeFlags::DecoratorContext, func);
        }
        template<typename T>
        T doInAwaitContext(const function<T()> &func) {
            return doInsideOfContext<T>((int) NodeFlags::AwaitContext, func);
        }
        template<typename T>
        T doOutsideOfAwaitContext(const function<T()> &func) {
            return doOutsideOfContext<T>((int) NodeFlags::AwaitContext, func);
        }
        template<typename T>
        T doInYieldAndAwaitContext(const function<T()> &func) {
            return doInsideOfContext<T>((int) NodeFlags::YieldContext | (int) NodeFlags::AwaitContext, func);
        }
        template<typename T>
        T doOutsideOfYieldAndAwaitContext(const function<T()> &func) {
            return doOutsideOfContext<T>((int) NodeFlags::YieldContext | (int) NodeFlags::AwaitContext, func);
        }

        bool inDisallowConditionalTypesContext() {
            return inContext(NodeFlags::DisallowConditionalTypesContext);
        }

        bool inDecoratorContext() {
            return inContext(NodeFlags::DecoratorContext);
        }

//        function createQualifiedName(entity: EntityName, name: Identifier): QualifiedName {
//            return finishNode(factory.createQualifiedName(entity, name), entity.pos);
//        }

        shared<NodeUnion(TemplateMiddle, TemplateTail)> parseTemplateMiddleOrTemplateTail() {
            auto fragment = parseLiteralLikeNode(token());
            Debug::asserts(fragment->kind == SyntaxKind::TemplateMiddle || fragment->kind == SyntaxKind::TemplateTail, "Template fragment has wrong token kind");
            return fragment;
        }

//        function parseExpectedToken<TKind extends SyntaxKind>(t: TKind, optional<DiagnosticMessage> diagnosticMessage, arg0?: any): Token<TKind>;
        template<class T>
        shared<T> parseExpectedToken(SyntaxKind t, const sharedOpt<DiagnosticMessage> &diagnosticMessage = nullptr, DiagnosticArg arg0 = "") {
            if (auto a = parseOptionalToken<T>(t)) return a;
            return createMissingNode<T>(t, /*reportAtCurrentPosition*/ false, diagnosticMessage ? diagnosticMessage : Diagnostics::_0_expected, arg0 != "" ? arg0 : tokenToString(t));
        }

        shared<Node> parseLiteralOfTemplateSpan(bool isTaggedTemplate) {
            if (token() == SyntaxKind::CloseBraceToken) {
                reScanTemplateToken(isTaggedTemplate);
                return parseTemplateMiddleOrTemplateTail();
            } else {
                // TODO(rbuckton): Do we need to call `parseExpectedToken` or can we just call `createMissingNode` directly?
                return parseExpectedToken<TemplateTail>(SyntaxKind::TemplateTail, Diagnostics::_0_expected, tokenToString(SyntaxKind::CloseBraceToken));
            }
        }

        shared<TemplateSpan> parseTemplateSpan(bool isTaggedTemplate) {
            auto pos = getNodePos();
            return finishNode(
                    factory.createTemplateSpan(
                            allowInAnd<shared<Expression>>(CALLBACK(parseExpression)),
                            parseLiteralOfTemplateSpan(isTaggedTemplate)
                    ),
                    pos
            );
        }

        shared<NodeArray> parseTemplateSpans(bool isTaggedTemplate) {
            auto pos = getNodePos();
            auto list = make_shared<NodeArray>();
            sharedOpt<TemplateSpan> node;
            do {
                node = parseTemplateSpan(isTaggedTemplate);
                list->push(node);
            } while (node->literal->kind == SyntaxKind::TemplateMiddle);
            return createNodeArray(list, pos);
        }

        shared<TemplateHead> parseTemplateHead(bool isTaggedTemplate) {
            if (isTaggedTemplate) {
                reScanTemplateHeadOrNoSubstitutionTemplate();
            }
            shared<Node> fragment = parseLiteralLikeNode(token());
//            Debug::asserts(fragment.kind == SyntaxKind::TemplateHead, "Template head has wrong token kind");
            return reinterpret_pointer_cast<TemplateHead>(fragment);
        }

        shared<TemplateExpression> parseTemplateExpression(bool isTaggedTemplate) {
            auto pos = getNodePos();
            return finishNode(
                    factory.createTemplateExpression(
                            parseTemplateHead(isTaggedTemplate),
                            parseTemplateSpans(isTaggedTemplate)
                    ),
                    pos
            );
        }

        shared<Node> parseEntityName(bool allowReservedWords, const sharedOpt<DiagnosticMessage> &diagnosticMessage = nullptr) {
            auto pos = getNodePos();
            shared<Node> entity = allowReservedWords ? parseIdentifierName(diagnosticMessage) : parseIdentifier(diagnosticMessage);
            auto dotPos = getNodePos();
            while (parseOptional(SyntaxKind::DotToken)) {
                if (token() == SyntaxKind::LessThanToken) {
                    // the entity is part of a JSDoc-style generic, so record the trailing dot for later error reporting
                    //we don't support jsdoc
                    //entity.jsdocDotPos = dotPos;
                    break;
                }
                dotPos = getNodePos();
                entity = finishNode(
                        factory.createQualifiedName(
                                entity,
                                parseRightSideOfDot(allowReservedWords, /* allowPrivateIdentifiers */ false)
                        ),
                        pos
                );
            }
            return entity;
        }


        // TYPES

        shared<Node> parseEntityNameOfTypeReference() {
            return parseEntityName(/*allowReservedWords*/ true, Diagnostics::Type_expected);
        }

        sharedOpt<NodeArray> parseTypeArgumentsOfTypeReference() {
            if (!scanner.hasPrecedingLineBreak() && reScanLessThanToken() == SyntaxKind::LessThanToken) {
                return parseBracketedList(ParsingContext::TypeArguments, CALLBACK(parseType), SyntaxKind::LessThanToken, SyntaxKind::GreaterThanToken);
            }
            return nullptr;
        }

        shared<TypeReferenceNode> parseTypeReference() {
            auto pos = getNodePos();
            return finishNode(
                    factory.createTypeReferenceNode(
                            parseEntityNameOfTypeReference(),
                            parseTypeArgumentsOfTypeReference()
                    ),
                    pos
            );
        }

        // If true, we should abort parsing an error function.
        bool typeHasArrowFunctionBlockingParseError(shared<TypeNode> node) {
            switch (node->kind) {
                case SyntaxKind::TypeReference:
                    return nodeIsMissing(node->to<TypeReferenceNode>().typeName);
                case SyntaxKind::FunctionType:
                case SyntaxKind::ConstructorType: {
                    auto f = reinterpret_pointer_cast<FunctionOrConstructorTypeNodeBase>(node);
                    return isMissingList(f->parameters) || typeHasArrowFunctionBlockingParseError(f->type);
                }
                case SyntaxKind::ParenthesizedType:
                    return typeHasArrowFunctionBlockingParseError(reinterpret_cast<ParenthesizedTypeNode &>(node).type);
                default:
                    return false;
            }
        }

        shared<TypePredicateNode> parseThisTypePredicate(shared<ThisTypeNode> lhs) {
            nextToken();
            return finishNode(factory.createTypePredicateNode(/*assertsModifier*/ {}, lhs, parseType()), lhs->pos);
        }

        shared<ThisTypeNode> parseThisTypeNode() {
            auto pos = getNodePos();
            nextToken();
            return finishNode(factory.createThisTypeNode(), pos);
        }

//        function parseJSDocNonNullableType(): TypeNode {
//            auto pos = getNodePos();
//            nextToken();
//            return finishNode(factory.createJSDocNonNullableType(parseNonArrayType(), /*postfix*/ false), pos);
//        }
//
//        function parseJSDocUnknownOrNullableType(): JSDocUnknownType | JSDocNullableType {
//            auto pos = getNodePos();
//            // skip the ?
//            nextToken();
//
//            // Need to lookahead to decide if this is a nullable or unknown type.
//
//            // Here are cases where we'll pick the unknown type:
//            //
//            //      Foo(?,
//            //      { a: ? }
//            //      Foo(?)
//            //      Foo<?>
//            //      Foo(?=
//            //      (?|
//            if (token() == SyntaxKind::CommaToken ||
//                token() == SyntaxKind::CloseBraceToken ||
//                token() == SyntaxKind::CloseParenToken ||
//                token() == SyntaxKind::GreaterThanToken ||
//                token() == SyntaxKind::EqualsToken ||
//                token() == SyntaxKind::BarToken) {
//                return finishNode(factory.createJSDocUnknownType(), pos);
//            }
//            else {
//                return finishNode(factory.createJSDocNullableType(parseType(), /*postfix*/ false), pos);
//            }
//        }
//
//        function parseJSDocFunctionType(): JSDocFunctionType | TypeReferenceNode {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            if (lookAhead(nextTokenIsOpenParen)) {
//                nextToken();
//                auto parameters = parseParameters(SignatureFlags::Type | SignatureFlags::JSDoc);
//                auto type = parseReturnType(SyntaxKind::ColonToken, /*isType*/ false);
//                return withJSDoc(finishNode(factory.createJSDocFunctionType(parameters, type), pos), hasJSDoc);
//            }
//            return finishNode(factory.createTypeReferenceNode(parseIdentifierName(), /*typeArguments*/ undefined), pos);
//        }
//
//        function parseJSDocParameter(): ParameterDeclaration {
//            auto pos = getNodePos();
//            auto name: Identifier | undefined;
//            if (token() == SyntaxKind::ThisKeyword || token() == SyntaxKind::NewKeyword) {
//                name = parseIdentifierName();
//                parseExpected(SyntaxKind::ColonToken);
//            }
//            return finishNode(
//                factory.createParameterDeclaration(
//                    /*decorators*/ {},
//                    /*modifiers*/ {},
//                    /*dotDotDotToken*/ undefined,
//                    // TODO(rbuckton): JSDoc parameters don't have names (except `this`/`new`), should we manufacture an empty identifier?
//                    name!,
//                    /*questionToken*/ undefined,
//                    parseJSDocType(),
//                    /*initializer*/ undefined
//                ),
//                pos
//            );
//        }
//
//        function parseJSDocType(): TypeNode {
//            scanner.setInJSDocType(true);
//            auto pos = getNodePos();
//            if (parseOptional(SyntaxKind::ModuleKeyword)) {
//                // TODO(rbuckton): We never set the type for a JSDocNamepathType. What should we put here?
//                auto moduleTag = factory.createJSDocNamepathType(/*type*/ undefined!);
//                terminate: while (true) {
//                    switch (token()) {
//                        case SyntaxKind::CloseBraceToken:
//                        case SyntaxKind::EndOfFileToken:
//                        case SyntaxKind::CommaToken:
//                        case SyntaxKind::WhitespaceTrivia:
//                            break terminate;
//                        default:
//                            nextTokenJSDoc();
//                    }
//                }
//
//                scanner.setInJSDocType(false);
//                return finishNode(moduleTag, pos);
//            }
//
//            auto hasDotDotDot = parseOptional(SyntaxKind::DotDotDotToken);
//            auto type = parseTypeOrTypePredicate();
//            scanner.setInJSDocType(false);
//            if (hasDotDotDot) {
//                type = finishNode(factory.createJSDocVariadicType(type), pos);
//            }
//            if (token() == SyntaxKind::EqualsToken) {
//                nextToken();
//                return finishNode(factory.createJSDocOptionalType(type), pos);
//            }
//            return type;
//        }

        shared<TypeQueryNode> parseTypeQuery() {
            auto pos = getNodePos();
            parseExpected(SyntaxKind::TypeOfKeyword);
            auto entityName = parseEntityName(/*allowReservedWords*/ true);
            // Make sure we perform ASI to prevent parsing the next line's type arguments as part of an instantiation expression.
            sharedOpt<NodeArray> typeArguments = !scanner.hasPrecedingLineBreak() ? tryParseTypeArguments() : nullptr;
            return finishNode(factory.createTypeQueryNode(entityName, typeArguments), pos);
        }

        bool isTemplateStartOfTaggedTemplate() {
            return token() == SyntaxKind::NoSubstitutionTemplateLiteral || token() == SyntaxKind::TemplateHead;
        }

        bool nextTokenIsIdentifierOrKeywordOrOpenBracketOrTemplate() {
            nextToken();
            return tokenIsIdentifierOrKeyword(token())
                   || token() == SyntaxKind::OpenBracketToken
                   || isTemplateStartOfTaggedTemplate();
        }

        bool isStartOfOptionalPropertyOrElementAccessChain() {
            return token() == SyntaxKind::QuestionDotToken && lookAhead<bool>(CALLBACK(nextTokenIsIdentifierOrKeywordOrOpenBracketOrTemplate));
        }

        bool tryReparseOptionalChain(shared<Expression> node) {
            if (node->flags & (int) NodeFlags::OptionalChain) {
                return true;
            }
            // check for an optional chain in a non-null expression
            if (isNonNullExpression(node)) {
                auto expr = node->to<NonNullExpression>().expression;
                while (isNonNullExpression(expr) && !(expr->flags & (int) NodeFlags::OptionalChain)) {
                    expr = node->to<NonNullExpression>().expression;
                }
                if (expr->flags & (int) NodeFlags::OptionalChain) {
                    // this is part of an optional chain. Walk down from `node` to `expression` and set the flag.
                    while (isNonNullExpression(node)) {
                        auto n = node->to<NonNullExpression>();
                        n.flags |= (int) NodeFlags::OptionalChain;
                        node = n.expression;
                    }
                    return true;
                }
            }
            return false;
        }

        shared<PropertyAccessExpression> parsePropertyAccessExpressionRest(int pos, const shared<LeftHandSideExpression> &expression, const sharedOpt<QuestionDotToken> &questionDotToken) {
            auto name = parseRightSideOfDot(/*allowIdentifierNames*/ true, /*allowPrivateIdentifiers*/ true);
            auto isOptionalChain = questionDotToken || tryReparseOptionalChain(expression);
            auto propertyAccess = isOptionalChain ?
                                  factory.createPropertyAccessChain(expression, questionDotToken, name) :
                                  factory.createPropertyAccessExpression(expression, name);
            if (isOptionalChain && isPrivateIdentifier(propertyAccess->name)) {
                parseErrorAtRange(propertyAccess->name, Diagnostics::An_optional_chain_cannot_contain_private_identifiers);
            }
            return finishNode(propertyAccess, pos);
        }

        shared<ElementAccessExpression> parseElementAccessExpressionRest(int pos, const shared<LeftHandSideExpression> &expression, const sharedOpt<QuestionDotToken> &questionDotToken) {
            sharedOpt<Expression> argumentExpression;
            if (token() == SyntaxKind::CloseBracketToken) {
                argumentExpression = createMissingNode<Identifier>(SyntaxKind::Identifier, /*reportAtCurrentPosition*/ true, Diagnostics::An_element_access_expression_should_take_an_argument);
            } else {
                auto argument = allowInAnd<shared<Expression>>(CALLBACK(parseExpression));
                if (isStringOrNumericLiteralLike(argument)) {
                    reinterpret_pointer_cast<LiteralExpression>(argument)->text = internIdentifier(reinterpret_pointer_cast<LiteralExpression>(argument)->text);
                }
                argumentExpression = argument;
            }

            parseExpected(SyntaxKind::CloseBracketToken);

            if (questionDotToken || tryReparseOptionalChain(expression)) {
                return finishNode(factory.createElementAccessChain(expression, questionDotToken, argumentExpression), pos);
            }
            return finishNode(factory.createElementAccessExpression(expression, argumentExpression), pos);
        }

        shared<TaggedTemplateExpression> parseTaggedTemplateRest(int pos, shared<LeftHandSideExpression> tag, sharedOpt<QuestionDotToken> questionDotToken, sharedOpt<NodeArray> typeArguments = {}) {
            shared<Node> templateLiteral = token() == SyntaxKind::NoSubstitutionTemplateLiteral ?
                                           (shared<Node>) (reScanTemplateHeadOrNoSubstitutionTemplate(), parseLiteralNode()) :
                                           (shared<Node>) parseTemplateExpression(/*isTaggedTemplate*/ true);
            auto tagExpression = factory.createTaggedTemplateExpression(
                    tag,
                    typeArguments,
                    templateLiteral
            );
            if (questionDotToken || tag->flags & (int) NodeFlags::OptionalChain) {
                tagExpression->flags |= (int) NodeFlags::OptionalChain;
            }
            tagExpression->questionDotToken = questionDotToken;
            return finishNode(tagExpression, pos);
        }

        bool canFollowTypeArgumentsInExpression() {
            switch (token()) {
                // These tokens can follow a type argument list in a call expression.
                case SyntaxKind::OpenParenToken:                 // foo<x>(
                case SyntaxKind::NoSubstitutionTemplateLiteral:  // foo<T> `...`
                case SyntaxKind::TemplateHead:                   // foo<T> `...${100}...`
                    return true;
            }
            // Consider something a type argument list only if the following token can't start an expression.
            return !isStartOfExpression();
        }

        sharedOpt<NodeArray> parseTypeArgumentsInExpression() {
            if ((contextFlags & (int) NodeFlags::JavaScriptFile) != 0) {
                // TypeArguments must not be parsed in JavaScript files to avoid ambiguity with binary operators.
                return nullptr;
            }

            if (reScanLessThanToken() != SyntaxKind::LessThanToken) {
                return nullptr;
            }
            nextToken();

            auto typeArguments = parseDelimitedList(ParsingContext::TypeArguments, CALLBACK(parseType));
            if (!parseExpected(SyntaxKind::GreaterThanToken)) {
                // If it doesn't have the closing `>` then it's definitely not an type argument list.
                return nullptr;
            }

            // We successfully parsed a type argument list. The next token determines whether we want to
            // treat it as such. If the type argument list is followed by `(` or a template literal, as in
            // `f<number>(42)`, we favor the type argument interpretation even though JavaScript would view
            // it as a relational expression.
            return typeArguments && canFollowTypeArgumentsInExpression() ? typeArguments : nullptr;
        }

        shared<MemberExpression> parseMemberExpressionRest(int pos, shared<LeftHandSideExpression> expression, bool allowOptionalChain) {
            while (true) {
                sharedOpt<QuestionDotToken> questionDotToken;
                auto isPropertyAccess = false;
                if (allowOptionalChain && isStartOfOptionalPropertyOrElementAccessChain()) {
                    questionDotToken = parseExpectedToken<QuestionDotToken>(SyntaxKind::QuestionDotToken);
                    isPropertyAccess = tokenIsIdentifierOrKeyword(token());
                } else {
                    isPropertyAccess = parseOptional(SyntaxKind::DotToken);
                }

                if (isPropertyAccess) {
                    expression = parsePropertyAccessExpressionRest(pos, expression, questionDotToken);
                    continue;
                }

                // when in the [Decorator] context, we do not parse ElementAccess as it could be part of a ComputedPropertyName
                if ((questionDotToken || !inDecoratorContext()) && parseOptional(SyntaxKind::OpenBracketToken)) {
                    expression = parseElementAccessExpressionRest(pos, expression, questionDotToken);
                    continue;
                }

                if (isTemplateStartOfTaggedTemplate()) {
                    // Absorb type arguments into TemplateExpression when preceding expression is ExpressionWithTypeArguments
                    expression = !questionDotToken && expression->kind == SyntaxKind::ExpressionWithTypeArguments ?
                                 parseTaggedTemplateRest(pos, expression->to<ExpressionWithTypeArguments>().expression, questionDotToken, expression->to<ExpressionWithTypeArguments>().typeArguments) :
                                 parseTaggedTemplateRest(pos, expression, questionDotToken, /*typeArguments*/ {});
                    continue;
                }

                if (!questionDotToken) {
                    if (token() == SyntaxKind::ExclamationToken && !scanner.hasPrecedingLineBreak()) {
                        nextToken();
                        expression = finishNode(factory.createNonNullExpression(expression), pos);
                        continue;
                    }
                    auto typeArguments = tryParse<sharedOpt<NodeArray>>(CALLBACK(parseTypeArgumentsInExpression));
                    if (typeArguments) {
                        expression = finishNode(factory.createExpressionWithTypeArguments(expression, typeArguments), pos);
                        continue;
                    }
                }

                return reinterpret_pointer_cast<MemberExpression>(expression);
            }
        }

        shared<LeftHandSideExpression> parseDecoratorExpression() {
            if (inAwaitContext() && token() == SyntaxKind::AwaitKeyword) {
                // `@await` is is disallowed in an [Await] context, but can cause parsing to go off the rails
                // This simply parses the missing identifier and moves on.
                auto pos = getNodePos();
                auto awaitExpression = parseIdentifier(Diagnostics::Expression_expected);
                nextToken();
                auto memberExpression = parseMemberExpressionRest(pos, awaitExpression, /*allowOptionalChain*/ true);
                return parseCallExpressionRest(pos, memberExpression);
            }
            return parseLeftHandSideExpressionOrHigher();
        }

        sharedOpt<Decorator> tryParseDecorator() {
            auto pos = getNodePos();
            if (!parseOptional(SyntaxKind::AtToken)) {
                return nullptr;
            }
            auto expression = doInDecoratorContext<shared<LeftHandSideExpression>>(CALLBACK(parseDecoratorExpression));
            return finishNode(factory.createDecorator(expression), pos);
        }

        sharedOpt<NodeArray> parseDecorators() {
            ZoneScoped;
            auto pos = getNodePos();
            sharedOpt<Node> decorator;
            sharedOpt<NodeArray> list;
            while (decorator = tryParseDecorator()) {
                if (!list) list = make_shared<NodeArray>();
                list->push(decorator);
            }
            if (!list) return nullptr;
            return createNodeArray(list, pos);
        }

        bool parseAnyContextualModifier() {
            return isModifierKind(token()) && tryParse<bool>(CALLBACK(nextTokenCanFollowModifier));
        }

        sharedOpt<Modifier> tryParseModifier(bool permitInvalidConstAsModifier = false, bool stopOnStartOfClassStaticBlock = false, bool hasSeenStaticModifier = false) {
            ZoneScoped;
            const auto pos = getNodePos();
            const auto kind = token();

            if (token() == SyntaxKind::ConstKeyword && permitInvalidConstAsModifier) {
                // We need to ensure that any subsequent modifiers appear on the same line
                // so that when 'const' is a standalone declaration, we don't issue an error.
                if (!tryParse<bool>(CALLBACK(nextTokenIsOnSameLineAndCanFollowModifier))) {
                    return nullptr;
                }
            } else if (stopOnStartOfClassStaticBlock && token() == SyntaxKind::StaticKeyword && lookAhead<bool>(CALLBACK(nextTokenIsOpenBrace))) {
                return nullptr;
            } else if (hasSeenStaticModifier && token() == SyntaxKind::StaticKeyword) {
                return nullptr;
            } else {
                if (!parseAnyContextualModifier()) {
                    return nullptr;
                }
            }

            return finishNode(factory.createToken<Modifier>(kind), pos);
        }

        /*
         * There are situations in which a modifier like 'const' will appear unexpectedly, such as on a class member.
         * In those situations, if we are entirely sure that 'const' is not valid on its own (such as when ASI takes effect
         * and turns it into a standalone declaration), then it is better to parse it and report an error later.
         *
         * In such situations, 'permitInvalidConstAsModifier' should be set to true.
         */
        sharedOpt<NodeArray> parseModifiers(bool permitInvalidConstAsModifier = false, bool stopOnStartOfClassStaticBlock = false) {
            ZoneScoped;
            const auto pos = getNodePos();
            auto hasSeenStatic = false;
            sharedOpt<NodeArray> list;
            sharedOpt<Node> modifier;
            while ((modifier = tryParseModifier(permitInvalidConstAsModifier, stopOnStartOfClassStaticBlock, hasSeenStatic))) {
                if (modifier->kind == SyntaxKind::StaticKeyword) hasSeenStatic = true;
                if (!list) list = make_shared<NodeArray>();
                list->push(modifier);
            }
            if (list) return createNodeArray(list, pos);
            return nullptr;
        }

        bool isParameterNameStart() {
            // Be permissive about await and yield by calling isBindingIdentifier instead of isIdentifier; disallowing
            // them during a speculative parse leads to many more follow-on errors than allowing the function to parse then later
            // complaining about the use of the keywords.
            return isBindingIdentifier() || token() == SyntaxKind::OpenBracketToken || token() == SyntaxKind::OpenBraceToken;
        }

        shared<Node> parseNameOfParameter(const sharedOpt<NodeArray> &modifiers) {
            ZoneScoped;
            // FormalParameter [Yield,Await]:
            //      BindingElement[?Yield,?Await]
            auto name = parseIdentifierOrPattern(Diagnostics::Private_identifiers_cannot_be_used_as_parameters);
            if (getFullWidth(name) == 0 && !some(modifiers) && isModifierKind(token())) {
                // in cases like
                // 'use strict'
                // function foo(static)
                // isParameter('static') == true, because of isModifier('static')
                // however 'static' is not a legal identifier in a strict mode.
                // so result of this function will be ParameterDeclaration (flags = 0, name = missing, type = undefined, initializer = undefined)
                // and current token will not change => parsing of the enclosing parameter list will last till the end of time (or OOM)
                // to avoid this we'll advance cursor to the next token.
                nextToken();
            }
            return name;
        }

        sharedOpt<Expression> parseInitializer() {
            ZoneScoped;
            return parseOptional(SyntaxKind::EqualsToken) ? parseAssignmentExpressionOrHigher() : nullptr;
        }

//        function parseParameterWorker(inOuterAwaitContext: boolean): ParameterDeclaration;
//        function parseParameterWorker(inOuterAwaitContext: boolean, allowAmbiguity: false): ParameterDeclaration | undefined;
        sharedOpt<ParameterDeclaration> parseParameterWorker(bool inOuterAwaitContext, bool allowAmbiguity = true) {
            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();

            // FormalParameter [Yield,Await]:
            //      BindingElement[?Yield,?Await]

            // Decorators are parsed in the outer [Await] context, the rest of the parameter is parsed in the function's [Await] context.
            auto decorators = inOuterAwaitContext ? doInAwaitContext<sharedOpt<NodeArray>>(CALLBACK(parseDecorators)) : parseDecorators();

            if (token() == SyntaxKind::ThisKeyword) {
                auto node = factory.createParameterDeclaration(
                        decorators,
                        /*modifiers*/ {},
                        /*dotDotDotToken*/ nullptr,
                        createIdentifier(/*isIdentifier*/ true),
                        /*questionToken*/ nullptr,
                        parseTypeAnnotation(),
                        /*initializer*/ nullptr
                );

                if (decorators && !decorators->empty()) {
                    parseErrorAtRange(decorators->list[0], Diagnostics::Decorators_may_not_be_applied_to_this_parameters);
                }

                return withJSDoc(finishNode(node, pos), hasJSDoc);
            }

            auto savedTopLevel = topLevel;
            topLevel = false;

            auto modifiers = parseModifiers();
            auto dotDotDotToken = parseOptionalToken<DotDotDotToken>(SyntaxKind::DotDotDotToken);

            if (!allowAmbiguity && !isParameterNameStart()) {
                return nullptr;
            }

            auto node = withJSDoc(
                    finishNode(
                            factory.createParameterDeclaration(
                                    decorators,
                                    modifiers,
                                    dotDotDotToken,
                                    parseNameOfParameter(modifiers),
                                    parseOptionalToken<QuestionToken>(SyntaxKind::QuestionToken),
                                    parseTypeAnnotation(),
                                    parseInitializer()
                            ),
                            pos
                    ),
                    hasJSDoc
            );
            topLevel = savedTopLevel;
            return node;
        }

        shared<ParameterDeclaration> parseParameter(bool inOuterAwaitContext) {
            return parseParameterWorker(inOuterAwaitContext);
        }

        sharedOpt<ParameterDeclaration> parseParameterForSpeculation(bool inOuterAwaitContext) {
            return parseParameterWorker(inOuterAwaitContext, /*allowAmbiguity*/ false);
        }

        bool shouldParseReturnType(SyntaxKind returnToken, bool isType) {
            if (returnToken == SyntaxKind::EqualsGreaterThanToken) {
                parseExpected(returnToken);
                return true;
            } else if (parseOptional(SyntaxKind::ColonToken)) {
                return true;
            } else if (isType && token() == SyntaxKind::EqualsGreaterThanToken) {
                // This is easy to get backward, especially in type contexts, so parse the type anyway
                parseErrorAtCurrentToken(Diagnostics::_0_expected, tokenToString(SyntaxKind::ColonToken));
                nextToken();
                return true;
            }
            return false;
        }

//        function parseReturnType(returnToken: SyntaxKind::EqualsGreaterThanToken, isType: boolean): TypeNode;
//        function parseReturnType(returnToken: SyntaxKind::ColonToken | SyntaxKind::EqualsGreaterThanToken, isType: boolean): TypeNode | undefined;
        sharedOpt<TypeNode> parseReturnType(SyntaxKind returnToken, bool isType) {
            if (shouldParseReturnType(returnToken, isType)) {
                return allowConditionalTypesAnd<shared<TypeNode>>(CALLBACK(parseTypeOrTypePredicate));
            }
            return nullptr;
        }

        void parseTypeMemberSemicolon() {
            // We allow type members to be separated by commas or (possibly ASI) semicolons.
            // First check if it was a comma.  If so, we're done with the member.
            if (parseOptional(SyntaxKind::CommaToken)) {
                return;
            }

            // Didn't have a comma.  We must have a (possible ASI) semicolon.
            parseSemicolon();
        }

        shared<NodeUnion(ArrayBindingElement)> parseArrayBindingElement() {
            auto pos = getNodePos();
            if (token() == SyntaxKind::CommaToken) {
                return finishNode(factory.createOmittedExpression(), pos);
            }
            auto dotDotDotToken = parseOptionalToken<DotDotDotToken>(SyntaxKind::DotDotDotToken);
            auto name = parseIdentifierOrPattern();
            auto initializer = parseInitializer();
            return finishNode(factory.createBindingElement(dotDotDotToken, /*propertyName*/ {}, name, initializer), pos);
        }

        shared<ArrayBindingPattern> parseArrayBindingPattern() {
            ZoneScoped;
            auto pos = getNodePos();
            parseExpected(SyntaxKind::OpenBracketToken);
            auto elements = parseDelimitedList(ParsingContext::ArrayBindingElements, CALLBACK(parseArrayBindingElement));
            parseExpected(SyntaxKind::CloseBracketToken);
            return finishNode(factory.createArrayBindingPattern(elements), pos);
        }

        string getTemplateLiteralRawText(SyntaxKind kind) {
            auto isLast = kind == SyntaxKind::NoSubstitutionTemplateLiteral || kind == SyntaxKind::TemplateTail;
            auto tokenText = scanner.getTokenText();
            return substring(tokenText, 1, tokenText.size() - (scanner.isUnterminated() ? 0 : isLast ? 1 : 2));
        }

        shared<LiteralLike> parseLiteralLikeNode(SyntaxKind kind) {
            auto pos = getNodePos();
            shared<LiteralLike> node =
                    isTemplateLiteralKind(kind) ? factory.createTemplateLiteralLikeNode(kind, scanner.getTokenValue(), getTemplateLiteralRawText(kind), scanner.getTokenFlags() & (int) TokenFlags::TemplateLiteralLikeFlags) :
                    // Octal literals are not allowed in strict mode or ES5
                    // Note that theoretically the following condition would hold true literals like 009,
                    // which is not octal. But because of how the scanner separates the tokens, we would
                    // never get a token like this. Instead, we would get 00 and 9 as two separate tokens.
                    // We also do not need to check for negatives because any prefix operator would be part of a
                    // parent unary expression.
                    kind == SyntaxKind::NumericLiteral ? factory.createNumericLiteral(scanner.getTokenValue(), scanner.getNumericLiteralFlags()) :
                    kind == SyntaxKind::StringLiteral ? factory.createStringLiteral(scanner.getTokenValue(), /*isSingleQuote*/ {}, scanner.hasExtendedUnicodeEscape()) :
                    isLiteralKind(kind) ? factory.createLiteralLikeNode(kind, scanner.getTokenValue()) :
                    throw runtime_error("Nope");

            if (scanner.hasExtendedUnicodeEscape()) {
                node->hasExtendedUnicodeEscape = true;
            }

            if (scanner.isUnterminated()) {
                node->isUnterminated = true;
            }

            nextToken();
            return finishNode<LiteralLike>(node, pos);
        }

        shared<LiteralLike> parseLiteralNode() {
            return parseLiteralLikeNode(token());
        }

        shared<ComputedPropertyName> parseComputedPropertyName() {
            // PropertyName [Yield]:
            //      LiteralPropertyName
            //      ComputedPropertyName[?Yield]
            auto pos = getNodePos();
            parseExpected(SyntaxKind::OpenBracketToken);
            // We parse any expression (including a comma expression). But the grammar
            // says that only an assignment expression is allowed, so the grammar checker
            // will error if it sees a comma expression.
            auto expression = allowInAnd<shared<Expression>>(CALLBACK(parseExpression));
            parseExpected(SyntaxKind::CloseBracketToken);
            return finishNode(factory.createComputedPropertyName(expression), pos);
        }

        string internPrivateIdentifier(const string &text) {
            auto privateIdentifier = get(privateIdentifiers, text);
            if (!privateIdentifier) {
                privateIdentifier = text;
                set(privateIdentifiers, text, *privateIdentifier);
            }
            return *privateIdentifier;
        }

        shared<PrivateIdentifier> parsePrivateIdentifier() {
            auto pos = getNodePos();
            auto node = factory.createPrivateIdentifier(internPrivateIdentifier(scanner.getTokenText()));
            nextToken();
            return finishNode(node, pos);
        }

        shared<Node> parsePropertyNameWorker(bool allowComputedPropertyNames) {
            if (token() == SyntaxKind::StringLiteral || token() == SyntaxKind::NumericLiteral) {
                auto node = parseLiteralNode();
                node->text = internIdentifier(node->text);
                return node;
            }
            if (allowComputedPropertyNames && token() == SyntaxKind::OpenBracketToken) {
                return parseComputedPropertyName();
            }
            if (token() == SyntaxKind::PrivateIdentifier) {
                return parsePrivateIdentifier();
            }
            return parseIdentifierName();
        }

        shared<Node> parsePropertyName() {
            return parsePropertyNameWorker(/*allowComputedPropertyNames*/ true);
        }

        shared<BindingElement> parseObjectBindingElement() {
            ZoneScoped;
            auto pos = getNodePos();
            auto dotDotDotToken = parseOptionalToken<DotDotDotToken>(SyntaxKind::DotDotDotToken);
            auto tokenIsIdentifier = isBindingIdentifier();
            sharedOpt<Node> propertyName = parsePropertyName();
            sharedOpt<Node> name;
            if (tokenIsIdentifier && token() != SyntaxKind::ColonToken) {
                name = propertyName;
                propertyName = nullptr;
            } else {
                parseExpected(SyntaxKind::ColonToken);
                name = parseIdentifierOrPattern();
            }
            auto initializer = parseInitializer();
            return finishNode(factory.createBindingElement(dotDotDotToken, propertyName, name, initializer), pos);
        }

        shared<ObjectBindingPattern> parseObjectBindingPattern() {
            ZoneScoped;
            auto pos = getNodePos();
            parseExpected(SyntaxKind::OpenBraceToken);
            auto elements = parseDelimitedList(ParsingContext::ObjectBindingElements, CALLBACK(parseObjectBindingElement));
            parseExpected(SyntaxKind::CloseBraceToken);
            return finishNode(factory.createObjectBindingPattern(elements), pos);
        }

        shared<UnionNode<Identifier, BindingPattern>> parseIdentifierOrPattern(const shared<DiagnosticMessage> &privateIdentifierDiagnosticMessage = nullptr) {
            ZoneScoped;
            if (token() == SyntaxKind::OpenBracketToken) {
                return parseArrayBindingPattern();
            }
            if (token() == SyntaxKind::OpenBraceToken) {
                return parseObjectBindingPattern();
            }
            return parseBindingIdentifier(privateIdentifierDiagnosticMessage);
        }

        bool skipParameterStart() {
            if (isModifierKind(token())) {
                // Skip modifiers
                parseModifiers();
            }
            if (isIdentifier() || token() == SyntaxKind::ThisKeyword) {
                nextToken();
                return true;
            }
            if (token() == SyntaxKind::OpenBracketToken || token() == SyntaxKind::OpenBraceToken) {
                // Return true if we can parse an array or object binding pattern with no errors
                auto previousErrorCount = parseDiagnostics.size();
                parseIdentifierOrPattern();
                return previousErrorCount == parseDiagnostics.size();
            }
            return false;
        }

        bool isUnambiguouslyStartOfFunctionType() {
            nextToken();
            if (token() == SyntaxKind::CloseParenToken || token() == SyntaxKind::DotDotDotToken) {
                // ( )
                // ( ...
                return true;
            }
            if (skipParameterStart()) {
                // We successfully skipped modifiers (if any) and an identifier or binding pattern,
                // now see if we have something that indicates a parameter declaration
                if (token() == SyntaxKind::ColonToken || token() == SyntaxKind::CommaToken ||
                    token() == SyntaxKind::QuestionToken || token() == SyntaxKind::EqualsToken) {
                    // ( xxx :
                    // ( xxx ,
                    // ( xxx ?
                    // ( xxx =
                    return true;
                }
                if (token() == SyntaxKind::CloseParenToken) {
                    nextToken();
                    if (token() == SyntaxKind::EqualsGreaterThanToken) {
                        // ( xxx ) =>
                        return true;
                    }
                }
            }
            return false;
        }

        bool nextTokenIsNewKeyword() {
            nextToken();
            return token() == SyntaxKind::NewKeyword;
        }

        bool isStartOfFunctionTypeOrConstructorType() {
            if (token() == SyntaxKind::LessThanToken) {
                return true;
            }
            if (token() == SyntaxKind::OpenParenToken && lookAhead<bool>(CALLBACK(isUnambiguouslyStartOfFunctionType))) {
                return true;
            }
            return token() == SyntaxKind::NewKeyword || (token() == SyntaxKind::AbstractKeyword && lookAhead<bool>(CALLBACK(nextTokenIsNewKeyword)));
        }

        sharedOpt<NodeArray> parseModifiersForConstructorType() {
            sharedOpt<NodeArray> modifiers;
            if (token() == SyntaxKind::AbstractKeyword) {
                auto pos = getNodePos();
                nextToken();
                auto modifier = finishNode(factory.createToken<AbstractKeyword>(SyntaxKind::AbstractKeyword), pos);
                auto list = make_shared<NodeArray>();
                list->push(modifier);
                modifiers = createNodeArray(list, pos);
            }
            return modifiers;
        }

        /**
         * Check if the current token can possibly be an ES7 increment expression.
         *
         * ES7 UpdateExpression:
         *      LeftHandSideExpression[?Yield]
         *      LeftHandSideExpression[?Yield][no LineTerminator here]++
         *      LeftHandSideExpression[?Yield][no LineTerminator here]--
         *      ++LeftHandSideExpression[?Yield]
         *      --LeftHandSideExpression[?Yield]
         */
        bool isUpdateExpression() {
            // This function is called inside parseUnaryExpression to decide
            // whether to call parseSimpleUnaryExpression or call parseUpdateExpression directly
            switch (token()) {
                case SyntaxKind::PlusToken:
                case SyntaxKind::MinusToken:
                case SyntaxKind::TildeToken:
                case SyntaxKind::ExclamationToken:
                case SyntaxKind::DeleteKeyword:
                case SyntaxKind::TypeOfKeyword:
                case SyntaxKind::VoidKeyword:
                case SyntaxKind::AwaitKeyword:
                    return false;
                case SyntaxKind::LessThanToken:
                    // If we are not in JSX context, we are parsing TypeAssertion which is an UnaryExpression
                    if (languageVariant != LanguageVariant::JSX) {
                        return false;
                    }
                    // We are in JSX context and the token is part of JSXElement.
                    // falls through
                default:
                    return true;
            }
        }

        shared<Node> parseRightSideOfDot(bool allowIdentifierNames, bool allowPrivateIdentifiers) {
            // Technically a keyword is valid here as all identifiers and keywords are identifier names.
            // However, often we'll encounter this in error situations when the identifier or keyword
            // is actually starting another valid construct.
            //
            // So, we check for the following specific case:
            //
            //      name.
            //      identifierOrKeyword identifierNameOrKeyword
            //
            // Note: the newlines are important here.  For example, if that above code
            // were rewritten into:
            //
            //      name.identifierOrKeyword
            //      identifierNameOrKeyword
            //
            // Then we would consider it valid.  That's because ASI would take effect and
            // the code would be implicitly: "name.identifierOrKeyword; identifierNameOrKeyword".
            // In the first case though, ASI will not take effect because there is not a
            // line terminator after the identifier or keyword.
            if (scanner.hasPrecedingLineBreak() && tokenIsIdentifierOrKeyword(token())) {
                auto matchesPattern = lookAhead<bool>(CALLBACK(nextTokenIsIdentifierOrKeywordOnSameLine));

                if (matchesPattern) {
                    // Report that we need an identifier.  However, report it right after the dot,
                    // and not on the next token.  This is because the next token might actually
                    // be an identifier and the error would be quite confusing.
                    return createMissingNode<Identifier>(SyntaxKind::Identifier, /*reportAtCurrentPosition*/ true, Diagnostics::Identifier_expected);
                }
            }

            if (token() == SyntaxKind::PrivateIdentifier) {
                auto node = parsePrivateIdentifier();
                return allowPrivateIdentifiers ? (shared<Node>) node : createMissingNode<Identifier>(SyntaxKind::Identifier, /*reportAtCurrentPosition*/ true, Diagnostics::Identifier_expected);
            }

            return allowIdentifierNames ? parseIdentifierName() : parseIdentifier();
        }

        shared<NodeUnion(JsxTagNameExpression)> parseJsxElementName() {
            auto pos = getNodePos();
            scanJsxIdentifier();
            // JsxElement can have name in the form of
            //      propertyAccessExpression
            //      primaryExpression in the form of an identifier and "this" keyword
            // We can't just simply use parseLeftHandSideExpressionOrHigher because then we will start consider class,function etc as a keyword
            // We only want to consider "this" as a primaryExpression
            shared<Expression> expression = token() == SyntaxKind::ThisKeyword ? (shared<Expression>) parseTokenNode<ThisExpression>() : parseIdentifierName();
            while (parseOptional(SyntaxKind::DotToken)) {
                expression = finishNode(factory.createPropertyAccessExpression(expression, parseRightSideOfDot(/*allowIdentifierNames*/ true, /*allowPrivateIdentifiers*/ false)), pos);
            }
            return expression;
        }

        shared<NodeArray> parseBracketedList(ParsingContext kind, function<shared<Node>()> parseElement, SyntaxKind open, SyntaxKind close) {
            if (parseExpected(open)) {
                auto result = parseDelimitedList(kind, parseElement);
                parseExpected(close);
                return result;
            }

            return createMissingList();
        }

        sharedOpt<NodeArray> tryParseTypeArguments() {
            if (token() == SyntaxKind::LessThanToken) return parseBracketedList(ParsingContext::TypeArguments, CALLBACK(parseType), SyntaxKind::LessThanToken, SyntaxKind::GreaterThanToken);
            return nullptr;
        }

        SyntaxKind scanJsxAttributeValue() {
            return currentToken = scanner.scanJsxAttributeValue();
        }

        sharedOpt<JsxExpression> parseJsxExpression(bool inExpressionContext) {
            auto pos = getNodePos();
            if (!parseExpected(SyntaxKind::OpenBraceToken)) {
                return nullptr;
            }

            sharedOpt<DotDotDotToken> dotDotDotToken;
            sharedOpt<Expression> expression;
            if (token() != SyntaxKind::CloseBraceToken) {
                dotDotDotToken = parseOptionalToken<DotDotDotToken>(SyntaxKind::DotDotDotToken);
                // Only an AssignmentExpression is valid here per the JSX spec,
                // but we can unambiguously parse a comma sequence and provide
                // a better error message in grammar checking.
                expression = parseExpression();
            }
            if (inExpressionContext) {
                parseExpected(SyntaxKind::CloseBraceToken);
            } else {
                if (parseExpected(SyntaxKind::CloseBraceToken, /*message*/ {}, /*shouldAdvance*/ false)) {
                    scanJsxText();
                }
            }

            return finishNode(factory.createJsxExpression(dotDotDotToken, expression), pos);
        }

        shared<JsxText> parseJsxText() {
            auto pos = getNodePos();
            auto node = factory.createJsxText(scanner.getTokenValue(), currentToken == SyntaxKind::JsxTextAllWhiteSpaces);
            currentToken = scanner.scanJsxToken();
            return finishNode(node, pos);
        }

        sharedOpt<NodeUnion(JsxChild)> parseJsxChild(shared<NodeUnion(JsxOpeningElement, JsxOpeningFragment)> openingTag, SyntaxKind token) {
            switch (token) {
                case SyntaxKind::EndOfFileToken:
                    // If we hit EOF, issue the error at the tag that lacks the closing element
                    // rather than at the end of the file (which is useless)
                    if (isJsxOpeningFragment(openingTag)) {
                        parseErrorAtRange(openingTag, Diagnostics::JSX_fragment_has_no_corresponding_closing_tag);
                    } else {
                        // We want the error span to cover only 'Foo.Bar' in < Foo.Bar >
                        // or to cover only 'Foo' in < Foo >
                        auto tag = getTagName(openingTag);
                        auto start = skipTrivia(sourceText, tag->pos);
                        parseErrorAt(start, tag->end, Diagnostics::JSX_element_0_has_no_corresponding_closing_tag, getTextOfNodeFromSourceText(sourceText, tag));
                    }
                    return nullptr;
                case SyntaxKind::LessThanSlashToken:
                case SyntaxKind::ConflictMarkerTrivia:
                    return nullptr;
                case SyntaxKind::JsxText:
                case SyntaxKind::JsxTextAllWhiteSpaces:
                    return parseJsxText();
                case SyntaxKind::OpenBraceToken:
                    return parseJsxExpression(/*inExpressionContext*/ false);
                case SyntaxKind::LessThanToken:
                    return parseJsxElementOrSelfClosingElementOrFragment(/*inExpressionContext*/ false, /*topInvalidNodePosition*/ {}, openingTag);
                default:
                    throw runtime_error(fmt::format("Should not reach {}", token));
//                    return Debug::assertsNever(token);
            }
        }

        /** @internal */
        bool tagNamesAreEquivalent(shared<NodeUnion(JsxTagNameExpression)> lhs, shared<NodeUnion(JsxTagNameExpression)> rhs) {
            if (lhs->kind != rhs->kind) {
                return false;
            }

            if (lhs->kind == SyntaxKind::Identifier) {
                if (rhs->kind == SyntaxKind::Identifier) return lhs->to<Identifier>().escapedText == rhs->to<Identifier>().escapedText;
                return false;
            }

            if (lhs->kind == SyntaxKind::ThisKeyword) {
                return true;
            }

            // If we are at this statement then we must have PropertyAccessExpression and because tag name in Jsx element can only
            // take forms of JsxTagNameExpression which includes an identifier, "this" expression, or another propertyAccessExpression
            // it is safe to case the expression property as such. See parseJsxElementName for how we parse tag name in Jsx element
            if (lhs->kind == SyntaxKind::PropertyAccessExpression && rhs->kind == SyntaxKind::PropertyAccessExpression) {
                auto l = lhs->to<PropertyAccessExpression>();
                auto r = rhs->to<PropertyAccessExpression>();
                return getEscapedName(l.name) == getEscapedName(r.name) && tagNamesAreEquivalent(l.expression, r.expression);
            }
            return false;
        }

        shared<NodeArray> parseJsxChildren(shared<NodeUnion(JsxOpeningElement, JsxOpeningFragment)> openingTag) {
            auto list = make_shared<NodeArray>();
            auto listPos = getNodePos();
            auto saveParsingContext = parsingContext;
            parsingContext |= 1 << (int) ParsingContext::JsxChildren;

            while (true) {
                auto child = parseJsxChild(openingTag, currentToken = scanner.reScanJsxToken());
                if (!child) break;
                list->push(child);
                if (isJsxOpeningElement(openingTag)
                    && (child && child->kind == SyntaxKind::JsxElement)
                    && !tagNamesAreEquivalent(getTagName(child->to<JsxElement>().openingElement), getTagName(child->to<JsxElement>().closingElement))
                    && tagNamesAreEquivalent(getTagName(openingTag), getTagName(child->to<JsxElement>().closingElement))) {
                    // stop after parsing a mismatched child like <div>...(<span></div>) in order to reattach the </div> higher
                    break;
                }
            }

            parsingContext = saveParsingContext;
            return createNodeArray(list, listPos);
        }

        shared<JsxClosingElement> parseJsxClosingElement(shared<JsxOpeningElement> open, bool inExpressionContext) {
            auto pos = getNodePos();
            parseExpected(SyntaxKind::LessThanSlashToken);
            auto tagName = parseJsxElementName();
            if (parseExpected(SyntaxKind::GreaterThanToken, /*diagnostic*/ {}, /*shouldAdvance*/ false)) {
                // manually advance the scanner in order to look for jsx text inside jsx
                if (inExpressionContext || !tagNamesAreEquivalent(open->tagName, tagName)) {
                    nextToken();
                } else {
                    scanJsxText();
                }
            }
            return finishNode(factory.createJsxClosingElement(tagName), pos);
        }

        shared<JsxClosingFragment> parseJsxClosingFragment(bool inExpressionContext) {
            auto pos = getNodePos();
            parseExpected(SyntaxKind::LessThanSlashToken);
            if (tokenIsIdentifierOrKeyword(token())) {
                parseErrorAtRange(parseJsxElementName(), Diagnostics::Expected_corresponding_closing_tag_for_JSX_fragment);
            }
            if (parseExpected(SyntaxKind::GreaterThanToken, /*diagnostic*/ {}, /*shouldAdvance*/ false)) {
                // manually advance the scanner in order to look for jsx text inside jsx
                if (inExpressionContext) {
                    nextToken();
                } else {
                    scanJsxText();
                }
            }
            return finishNode(factory.createJsxJsxClosingFragment(), pos);
        }

        shared<Expression>
        parseJsxElementOrSelfClosingElementOrFragment(bool inExpressionContext, optional<int> topInvalidNodePosition = {}, sharedOpt<NodeUnion(JsxOpeningElement, JsxOpeningFragment)> openingTag = {}) {
            auto pos = getNodePos();
            auto _opening = parseJsxOpeningOrSelfClosingElementOrOpeningFragment(inExpressionContext);
            sharedOpt<Expression> result; //JsxElement | JsxSelfClosingElement | JsxFragment
            if (_opening->kind == SyntaxKind::JsxOpeningElement) {
                auto opening = reinterpret_pointer_cast<JsxOpeningElement>(_opening);
                auto children = parseJsxChildren(opening);
                sharedOpt<JsxClosingElement> closingElement; // JsxClosingElement;

                auto lastChild = children->list[children->length() - 1];
                if (lastChild && lastChild->kind == SyntaxKind::JsxElement
                    && !tagNamesAreEquivalent(getTagName(lastChild->to<JsxElement>().openingElement), getTagName(lastChild->to<JsxElement>().closingElement))
                    && tagNamesAreEquivalent(getTagName(opening), getTagName(lastChild->to<JsxElement>().closingElement))) {
                    // when an unclosed JsxOpeningElement incorrectly parses its parent's JsxClosingElement,
                    // restructure (<div>(...<span>...</div>)) --> (<div>(...<span>...</>)</div>)
                    // (no need to error; the parent will error)
                    auto jsxElement = lastChild->to<JsxElement>();
                    auto end = jsxElement.children->end;
                    auto newLast = finishNode(factory.createJsxElement(
                                                      jsxElement.openingElement,
                                                      jsxElement.children,
                                                      finishNode(factory.createJsxClosingElement(finishNode(factory.createIdentifier(""), end, end)), end, end)),
                                              jsxElement.openingElement->pos,
                                              end);

                    auto c = children->slice(0, children->length() - 1);
                    c.push_back(newLast);
                    children = createNodeArray(make_shared<NodeArray>(c), children->pos, end);
                    closingElement = jsxElement.closingElement;
                } else {
                    closingElement = parseJsxClosingElement(opening, inExpressionContext);
                    if (!tagNamesAreEquivalent(getTagName(opening), getTagName(closingElement))) {
                        if (openingTag && isJsxOpeningElement(openingTag) && tagNamesAreEquivalent(getTagName(closingElement), getTagName(openingTag))) {
                            // opening incorrectly matched with its parent's closing -- put error on opening
                            parseErrorAtRange(getTagName(opening), Diagnostics::JSX_element_0_has_no_corresponding_closing_tag, getTextOfNodeFromSourceText(sourceText, getTagName(opening)));
                        } else {
                            // other opening/closing mismatches -- put error on closing
                            parseErrorAtRange(getTagName(closingElement), Diagnostics::Expected_corresponding_JSX_closing_tag_for_0, getTextOfNodeFromSourceText(sourceText, getTagName(opening)));
                        }
                    }
                }
                result = finishNode(factory.createJsxElement(opening, children, closingElement), pos);
            } else if (_opening->kind == SyntaxKind::JsxOpeningFragment) {
                auto opening = reinterpret_pointer_cast<JsxOpeningFragment>(_opening);
                result = finishNode(factory.createJsxFragment(opening, parseJsxChildren(opening), parseJsxClosingFragment(inExpressionContext)), pos);
            } else {
                assert(_opening->kind == SyntaxKind::JsxSelfClosingElement);
                // Nothing else to do for self-closing elements
                result = _opening;
            }

            // If the user writes the invalid code '<div></div><div></div>' in an expression context (i.e. not wrapped in
            // an enclosing tag), we'll naively try to parse   ^ this as a 'less than' operator and the remainder of the tag
            // as garbage, which will cause the formatter to badly mangle the JSX. Perform a speculative parse of a JSX
            // element if we see a < token so that we can wrap it in a synthetic binary expression so the formatter
            // does less damage and we can report a better error.
            // Since JSX elements are invalid < operands anyway, this lookahead parse will only occur in error scenarios
            // of one sort or another.
            if (inExpressionContext && token() == SyntaxKind::LessThanToken) {
//                auto topBadPos = typeof topInvalidNodePosition == "undefined" ? result.pos : topInvalidNodePosition;
//                auto invalidElement = tryParse(() => parseJsxElementOrSelfClosingElementOrFragment(/*inExpressionContext*/ true, topBadPos));
//                if (invalidElement) {
//                    auto operatorToken = createMissingNode(SyntaxKind::CommaToken, /*reportAtCurrentPosition*/ false);
//                    setTextRangePosWidth(operatorToken, invalidElement.pos, 0);
//                    parseErrorAt(skipTrivia(sourceText, topBadPos), invalidElement.end, Diagnostics::JSX_expressions_must_have_one_parent_element);
//                    return finishNode(factory.createBinaryExpression(result, operatorToken as Token<SyntaxKind::CommaToken>, invalidElement), pos) as Node as JsxElement;
//                }
            }

            return result;
        }

        sharedOpt<NodeUnion(JsxAttributeValue)> parseJsxAttributeValue() {
            if (token() == SyntaxKind::EqualsToken) {
                if (scanJsxAttributeValue() == SyntaxKind::StringLiteral) {
                    return parseLiteralNode();
                }
                if (token() == SyntaxKind::OpenBraceToken) {
                    return parseJsxExpression(/*inExpressionContext*/ true);
                }
                if (token() == SyntaxKind::LessThanToken) {
                    return parseJsxElementOrSelfClosingElementOrFragment(/*inExpressionContext*/ true);
                }
                parseErrorAtCurrentToken(Diagnostics::or_JSX_element_expected);
            }
            return nullptr;
        }

        shared<JsxSpreadAttribute> parseJsxSpreadAttribute() {
            auto pos = getNodePos();
            parseExpected(SyntaxKind::OpenBraceToken);
            parseExpected(SyntaxKind::DotDotDotToken);
            auto expression = parseExpression();
            parseExpected(SyntaxKind::CloseBraceToken);
            return finishNode(factory.createJsxSpreadAttribute(expression), pos);
        }

        shared<NodeUnion(JsxAttribute, JsxSpreadAttribute)> parseJsxAttribute() {
            if (token() == SyntaxKind::OpenBraceToken) {
                return parseJsxSpreadAttribute();
            }

            scanJsxIdentifier();
            auto pos = getNodePos();
            return finishNode(factory.createJsxAttribute(parseIdentifierName(), parseJsxAttributeValue()), pos);
        }

        shared<JsxAttributes> parseJsxAttributes() {
            auto pos = getNodePos();
            return finishNode(factory.createJsxAttributes(parseList(ParsingContext::JsxAttributes, CALLBACK(parseJsxAttribute))), pos);
        }

        shared<Expression> parseJsxOpeningOrSelfClosingElementOrOpeningFragment(bool inExpressionContext) {
            auto pos = getNodePos();

            parseExpected(SyntaxKind::LessThanToken);

            if (token() == SyntaxKind::GreaterThanToken) {
                // See below for explanation of scanJsxText
                scanJsxText();
                return finishNode(factory.createJsxOpeningFragment(), pos);
            }
            auto tagName = parseJsxElementName();
            sharedOpt<NodeArray> typeArguments = (contextFlags & (int) NodeFlags::JavaScriptFile) == 0 ? tryParseTypeArguments() : nullptr;
            auto attributes = parseJsxAttributes();

            sharedOpt<Expression> node;

            if (token() == SyntaxKind::GreaterThanToken) {
                // Closing tag, so scan the immediately-following text with the JSX scanning instead
                // of regular scanning to avoid treating illegal characters (e.g. '#') as immediate
                // scanning errors
                scanJsxText();
                node = factory.createJsxOpeningElement(tagName, typeArguments, attributes);
            } else {
                parseExpected(SyntaxKind::SlashToken);
                if (parseExpected(SyntaxKind::GreaterThanToken, /*diagnostic*/ {}, /*shouldAdvance*/ false)) {
                    // manually advance the scanner in order to look for jsx text inside jsx
                    if (inExpressionContext) {
                        nextToken();
                    } else {
                        scanJsxText();
                    }
                }
                node = factory.createJsxSelfClosingElement(tagName, typeArguments, attributes);
            }

            return finishNode(node, pos);
        }

        bool nextTokenIsOpenParenOrLessThan() {
            nextToken();
            return token() == SyntaxKind::OpenParenToken || token() == SyntaxKind::LessThanToken;
        }

        bool nextTokenIsDot() {
            return nextToken() == SyntaxKind::DotToken;
        }

        sharedOpt<NodeArray> parseTypeParameters() {
            if (token() == SyntaxKind::LessThanToken) {
                return parseBracketedList(ParsingContext::TypeParameters, CALLBACK(parseTypeParameter), SyntaxKind::LessThanToken, SyntaxKind::GreaterThanToken);
            }
            return nullptr;
        }

//        function parseParametersWorker(SignatureFlags flags, allowAmbiguity: true): NodeArray<ParameterDeclaration>;
//        function parseParametersWorker(SignatureFlags flags, allowAmbiguity: false): NodeArray<ParameterDeclaration> | undefined;
        sharedOpt<NodeArray> parseParametersWorker(int flags, bool allowAmbiguity) {
            // FormalParameters [Yield,Await]: (modified)
            //      [empty]
            //      FormalParameterList[?Yield,Await]
            //
            // FormalParameter[Yield,Await]: (modified)
            //      BindingElement[?Yield,Await]
            //
            // BindingElement [Yield,Await]: (modified)
            //      SingleNameBinding[?Yield,?Await]
            //      BindingPattern[?Yield,?Await]Initializer [In, ?Yield,?Await] opt
            //
            // SingleNameBinding [Yield,Await]:
            //      BindingIdentifier[?Yield,?Await]Initializer [In, ?Yield,?Await] opt
            auto savedYieldContext = inYieldContext();
            auto savedAwaitContext = inAwaitContext();

            setYieldContext(!!(flags & (int) SignatureFlags::Yield));
            setAwaitContext(!!(flags & (int) SignatureFlags::Await));

            sharedOpt<NodeArray> parameters = flags & (int) SignatureFlags::JSDoc ?
                                              make_shared<NodeArray>() : //we ignore JSDoc, parseDelimitedList(ParsingContext::JSDocParameters, parseJSDocParameter) :
                                              parseDelimitedList(ParsingContext::Parameters, [this, &allowAmbiguity, savedAwaitContext]()->sharedOpt<Node> { return allowAmbiguity ? parseParameter(savedAwaitContext) : parseParameterForSpeculation(savedAwaitContext); });

            setYieldContext(savedYieldContext);
            setAwaitContext(savedAwaitContext);

            return parameters;
        }

        shared<NodeArray> parseParameters(int flags) {
            // FormalParameters [Yield,Await]: (modified)
            //      [empty]
            //      FormalParameterList[?Yield,Await]
            //
            // FormalParameter[Yield,Await]: (modified)
            //      BindingElement[?Yield,Await]
            //
            // BindingElement [Yield,Await]: (modified)
            //      SingleNameBinding[?Yield,?Await]
            //      BindingPattern[?Yield,?Await]Initializer [In, ?Yield,?Await] opt
            //
            // SingleNameBinding [Yield,Await]:
            //      BindingIdentifier[?Yield,?Await]Initializer [In, ?Yield,?Await] opt
            if (!parseExpected(SyntaxKind::OpenParenToken)) {
                return createMissingList();
            }

            auto parameters = parseParametersWorker(flags, /*allowAmbiguity*/ true);
            parseExpected(SyntaxKind::CloseParenToken);
            return parameters;
        }

        shared<NodeArray> parseParameters(SignatureFlags flags) {
            return parseParameters((int) flags);
        }

        shared<NodeUnion(CallSignatureDeclaration, ConstructSignatureDeclaration)> parseSignatureMember(SyntaxKind kind) {
            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();
            if (kind == SyntaxKind::ConstructSignature) {
                parseExpected(SyntaxKind::NewKeyword);
            }

            auto typeParameters = parseTypeParameters();
            auto parameters = parseParameters(SignatureFlags::Type);
            auto type = parseReturnType(SyntaxKind::ColonToken, /*isType*/ true);
            parseTypeMemberSemicolon();
            shared<Node> node = kind == SyntaxKind::CallSignature
                                ? (shared<Node>) factory.createCallSignature(typeParameters, parameters, type)
                                : (shared<Node>) factory.createConstructSignature(typeParameters, parameters, type);
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        void parseExpectedMatchingBrackets(SyntaxKind openKind, SyntaxKind closeKind, bool openParsed, int openPosition) {
            if (token() == closeKind) {
                nextToken();
                return;
            }
            auto lastError = parseErrorAtCurrentToken(Diagnostics::_0_expected, tokenToString(closeKind));
            if (!openParsed) {
                return;
            }
            if (lastError) {
                auto b = createDetachedDiagnostic(fileName, openPosition, 1, Diagnostics::The_parser_expected_to_find_a_1_to_match_the_0_token_here, {tokenToString(openKind), tokenToString(closeKind)});
                addRelatedInfo(
                        *lastError,
                        {b}
                );
            }
        }

        shared<Block> parseBlock(bool ignoreMissingOpenBrace, const sharedOpt<DiagnosticMessage> &diagnosticMessage = nullptr) {
            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();
            auto openBracePosition = scanner.getTokenPos();
            auto openBraceParsed = parseExpected(SyntaxKind::OpenBraceToken, diagnosticMessage);
            if (openBraceParsed || ignoreMissingOpenBrace) {
                auto multiLine = scanner.hasPrecedingLineBreak();
                auto statements = parseList(ParsingContext::BlockStatements, CALLBACK(parseStatement));
                parseExpectedMatchingBrackets(SyntaxKind::OpenBraceToken, SyntaxKind::CloseBraceToken, openBraceParsed, openBracePosition);
                auto result = withJSDoc(finishNode(factory.createBlock(statements, multiLine), pos), hasJSDoc);
                if (token() == SyntaxKind::EqualsToken) {
                    parseErrorAtCurrentToken(Diagnostics::Declaration_or_statement_expected_This_follows_a_block_of_statements_so_if_you_intended_to_write_a_destructuring_assignment_you_might_need_to_wrap_the_the_whole_assignment_in_parentheses);
                    nextToken();
                }

                return result;
            } else {
                auto statements = createMissingList();
                return withJSDoc(finishNode(factory.createBlock(statements, /*multiLine*/ false), pos), hasJSDoc);
            }
        }

        shared<Block> parseFunctionBlock(int flags, const sharedOpt<DiagnosticMessage> &diagnosticMessage = nullptr) {
            auto savedYieldContext = inYieldContext();
            setYieldContext(!!(flags & (int) SignatureFlags::Yield));

            auto savedAwaitContext = inAwaitContext();
            setAwaitContext(!!(flags & (int) SignatureFlags::Await));

            auto savedTopLevel = topLevel;
            topLevel = false;

            // We may be in a [Decorator] context when parsing a function expression or
            // arrow function. The body of the function is not in [Decorator] context.
            auto saveDecoratorContext = inDecoratorContext();
            if (saveDecoratorContext) {
                setDecoratorContext(/*val*/ false);
            }

            auto block = parseBlock(!!(flags & (int) SignatureFlags::IgnoreMissingOpenBrace), diagnosticMessage);

            if (saveDecoratorContext) {
                setDecoratorContext(/*val*/ true);
            }

            topLevel = savedTopLevel;
            setYieldContext(savedYieldContext);
            setAwaitContext(savedAwaitContext);

            return block;
        }

        sharedOpt<Block> parseFunctionBlockOrSemicolon(int flags, const sharedOpt<DiagnosticMessage> &diagnosticMessage = nullptr) {
            if (token() != SyntaxKind::OpenBraceToken && canParseSemicolon()) {
                parseSemicolon();
                return nullptr;
            }

            return parseFunctionBlock(flags, diagnosticMessage);
        }

        shared<Node> parseAccessorDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray> decorators, sharedOpt<NodeArray> modifiers, SyntaxKind kind) {
            auto name = parsePropertyName();
            auto typeParameters = parseTypeParameters();
            auto parameters = parseParameters(SignatureFlags::None);
            auto type = parseReturnType(SyntaxKind::ColonToken, /*isType*/ false);
            auto body = parseFunctionBlockOrSemicolon((int) SignatureFlags::None);
            shared<Node> node = kind == SyntaxKind::GetAccessor
                                ? (shared<Node>) factory.createGetAccessorDeclaration(decorators, modifiers, name, parameters, type, body)
                                : (shared<Node>) factory.createSetAccessorDeclaration(decorators, modifiers, name, parameters, body);
            // Keep track of `typeParameters` (for both) and `type` (for setters) if they were parsed those indicate grammar errors
            if (node->is<GetAccessorDeclaration>()) {
                node->to<GetAccessorDeclaration>().typeParameters = typeParameters;
            } else if (node->is<SetAccessorDeclaration>()) {
                node->to<SetAccessorDeclaration>().typeParameters = typeParameters;
                if (type) {
                    node->to<SetAccessorDeclaration>().type = type;
                }
            }
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        bool isUnambiguouslyIndexSignature() {
            // The only allowed sequence is:
            //
            //   [id:
            //
            // However, for error recovery, we also check the following cases:
            //
            //   [...
            //   [id,
            //   [id?,
            //   [id?:
            //   [id?]
            //   [public id
            //   [private id
            //   [protected id
            //   []
            //
            nextToken();
            if (token() == SyntaxKind::DotDotDotToken || token() == SyntaxKind::CloseBracketToken) {
                return true;
            }

            if (isModifierKind(token())) {
                nextToken();
                if (isIdentifier()) {
                    return true;
                }
            } else if (!isIdentifier()) {
                return false;
            } else {
                // Skip the identifier
                nextToken();
            }

            // A colon signifies a well formed indexer
            // A comma should be a badly formed indexer because comma expressions are not allowed
            // in computed properties.
            if (token() == SyntaxKind::ColonToken || token() == SyntaxKind::CommaToken) {
                return true;
            }

            // Question mark could be an indexer with an optional property,
            // or it could be a conditional expression in a computed property.
            if (token() != SyntaxKind::QuestionToken) {
                return false;
            }

            // If any of the following tokens are after the question mark, it cannot
            // be a conditional expression, so treat it as an indexer.
            nextToken();
            return token() == SyntaxKind::ColonToken || token() == SyntaxKind::CommaToken || token() == SyntaxKind::CloseBracketToken;
        }

        bool isIndexSignature() {
            return token() == SyntaxKind::OpenBracketToken && lookAhead<bool>(CALLBACK(isUnambiguouslyIndexSignature));
        }

        shared<IndexSignatureDeclaration> parseIndexSignatureDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray> decorators, sharedOpt<NodeArray> modifiers) {
            auto parameters = parseBracketedList(ParsingContext::Parameters, [this]()->shared<Node> { return parseParameter(/*inOuterAwaitContext*/ false); }, SyntaxKind::OpenBracketToken, SyntaxKind::CloseBracketToken);
            auto type = parseTypeAnnotation();
            parseTypeMemberSemicolon();
            auto node = factory.createIndexSignature(decorators, modifiers, parameters, type);
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        shared<Node> parsePropertyOrMethodSignature(int pos, bool hasJSDoc, sharedOpt<NodeArray> modifiers) {
            auto name = parsePropertyName();
            auto questionToken = parseOptionalToken<QuestionToken>(SyntaxKind::QuestionToken);
            shared<Node> node;
            if (token() == SyntaxKind::OpenParenToken || token() == SyntaxKind::LessThanToken) {
                // Method signatures don't exist in expression contexts.  So they have neither
                // [Yield] nor [Await]
                auto typeParameters = parseTypeParameters();
                auto parameters = parseParameters(SignatureFlags::Type);
                auto type = parseReturnType(SyntaxKind::ColonToken, /*isType*/ true);
                node = factory.createMethodSignature(modifiers, name, questionToken, typeParameters, parameters, type);
            } else {
                auto type = parseTypeAnnotation();
                auto p = factory.createPropertySignature(modifiers, name, questionToken, type);
                // Although type literal properties cannot not have initializers, we attempt
                // to parse an initializer so we can report in the checker that an interface
                // property or type literal property cannot have an initializer.
                if (token() == SyntaxKind::EqualsToken) p->initializer = parseInitializer();
                node = p;
            }
            parseTypeMemberSemicolon();
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        shared<Node> parseTypeMember() {
            if (token() == SyntaxKind::OpenParenToken || token() == SyntaxKind::LessThanToken) {
                return parseSignatureMember(SyntaxKind::CallSignature);
            }
            if (token() == SyntaxKind::NewKeyword && lookAhead<bool>(CALLBACK(nextTokenIsOpenParenOrLessThan))) {
                return parseSignatureMember(SyntaxKind::ConstructSignature);
            }
            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();
            auto modifiers = parseModifiers();
            if (parseContextualModifier(SyntaxKind::GetKeyword)) {
                return parseAccessorDeclaration(pos, hasJSDoc, /*decorators*/ {}, modifiers, SyntaxKind::GetAccessor);
            }

            if (parseContextualModifier(SyntaxKind::SetKeyword)) {
                return parseAccessorDeclaration(pos, hasJSDoc, /*decorators*/ {}, modifiers, SyntaxKind::SetAccessor);
            }

            if (isIndexSignature()) {
                return parseIndexSignatureDeclaration(pos, hasJSDoc, /*decorators*/ {}, modifiers);
            }
            return parsePropertyOrMethodSignature(pos, hasJSDoc, modifiers);
        }

        shared<NodeArray> parseObjectTypeMembers() {
            shared<NodeArray> members;
            if (parseExpected(SyntaxKind::OpenBraceToken)) {
                members = parseList(ParsingContext::TypeMembers, CALLBACK(parseTypeMember));
                parseExpected(SyntaxKind::CloseBraceToken);
            } else {
                members = createMissingList();
            }

            return members;
        }
        shared<TypeLiteralNode> parseTypeLiteral() {
            auto pos = getNodePos();
            return finishNode(factory.createTypeLiteralNode(parseObjectTypeMembers()), pos);
        }

        shared<Expression> parseSpreadElement() {
            auto pos = getNodePos();
            parseExpected(SyntaxKind::DotDotDotToken);
            auto expression = parseAssignmentExpressionOrHigher();
            return finishNode(factory.createSpreadElement(expression), pos);
        }

        shared<Expression> parseArgumentOrArrayLiteralElement() {
            if (token() == SyntaxKind::DotDotDotToken) return parseSpreadElement();
            if (token() == SyntaxKind::CommaToken) return finishNode(factory.createOmittedExpression(), getNodePos());
            return parseAssignmentExpressionOrHigher();
        }

        shared<Expression> parseArgumentExpression() {
            return doOutsideOfContext<shared<Expression>>(disallowInAndDecoratorContext, CALLBACK(parseArgumentOrArrayLiteralElement));
        }

        shared<NodeArray> parseArgumentList() {
            parseExpected(SyntaxKind::OpenParenToken);
            auto result = parseDelimitedList(ParsingContext::ArgumentExpressions, CALLBACK(parseArgumentExpression));
            parseExpected(SyntaxKind::CloseParenToken);
            return result;
        }

        shared<LeftHandSideExpression> parseCallExpressionRest(int pos, shared<LeftHandSideExpression> expression) {
            while (true) {
                expression = parseMemberExpressionRest(pos, expression, /*allowOptionalChain*/ true);
                sharedOpt<NodeArray> typeArguments;
                auto questionDotToken = parseOptionalToken<QuestionDotToken>(SyntaxKind::QuestionDotToken);
                if (questionDotToken) {
                    typeArguments = tryParse<sharedOpt<NodeArray>>(CALLBACK(parseTypeArgumentsInExpression));
                    if (isTemplateStartOfTaggedTemplate()) {
                        expression = parseTaggedTemplateRest(pos, expression, questionDotToken, typeArguments);
                        continue;
                    }
                }
                if (typeArguments || token() == SyntaxKind::OpenParenToken) {
                    // Absorb type arguments into CallExpression when preceding expression is ExpressionWithTypeArguments
                    if (!questionDotToken && expression->kind == SyntaxKind::ExpressionWithTypeArguments) {
                        typeArguments = expression->to<ExpressionWithTypeArguments>().typeArguments;
                        expression = expression->to<ExpressionWithTypeArguments>().expression;
                    }
                    auto argumentList = parseArgumentList();
                    auto callExpr = questionDotToken || tryReparseOptionalChain(expression) ?
                                    factory.createCallChain(expression, questionDotToken, typeArguments, argumentList) :
                                    factory.createCallExpression(expression, typeArguments, argumentList);
                    expression = finishNode(callExpr, pos);
                    continue;
                }
                if (questionDotToken) {
                    // We parsed `?.` but then failed to parse anything, so report a missing identifier here.
                    auto name = createMissingNode<Identifier>(SyntaxKind::Identifier, /*reportAtCurrentPosition*/ false, Diagnostics::Identifier_expected);
                    expression = finishNode(factory.createPropertyAccessChain(expression, questionDotToken, name), pos);
                }
                break;
            }
            return expression;
        }

        shared<ParenthesizedExpression> parseParenthesizedExpression() {
            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();
            parseExpected(SyntaxKind::OpenParenToken);
            auto expression = allowInAnd<shared<Expression>>(CALLBACK(parseExpression));
            parseExpected(SyntaxKind::CloseParenToken);
            return withJSDoc(finishNode(factory.createParenthesizedExpression(expression), pos), hasJSDoc);
        }

        shared<ArrayLiteralExpression> parseArrayLiteralExpression() {
            auto pos = getNodePos();
            auto openBracketPosition = scanner.getTokenPos();
            auto openBracketParsed = parseExpected(SyntaxKind::OpenBracketToken);
            auto multiLine = scanner.hasPrecedingLineBreak();
            auto elements = parseDelimitedList(ParsingContext::ArrayLiteralMembers, CALLBACK(parseArgumentOrArrayLiteralElement));
            parseExpectedMatchingBrackets(SyntaxKind::OpenBracketToken, SyntaxKind::CloseBracketToken, openBracketParsed, openBracketPosition);
            return finishNode(factory.createArrayLiteralExpression(elements, multiLine), pos);
        }

        shared<MethodDeclaration> parseMethodDeclaration(
                int pos,
                bool hasJSDoc,
                sharedOpt<NodeArray> decorators,
                sharedOpt<NodeArray> modifiers,
                sharedOpt<AsteriskToken> asteriskToken,
                shared<Node> name,
                sharedOpt<QuestionToken> questionToken,
                sharedOpt<ExclamationToken> exclamationToken,
                const sharedOpt<DiagnosticMessage> &diagnosticMessage = nullptr
        ) {
            auto isGenerator = asteriskToken ? SignatureFlags::Yield : SignatureFlags::None;
            auto isAsync = some(modifiers, isAsyncModifier) ? SignatureFlags::Await : SignatureFlags::None;
            auto typeParameters = parseTypeParameters();
            auto parameters = parseParameters((int) isGenerator | (int) isAsync);
            auto type = parseReturnType(SyntaxKind::ColonToken, /*isType*/ false);
            auto body = parseFunctionBlockOrSemicolon((int) isGenerator | (int) isAsync, diagnosticMessage);
            auto node = factory.createMethodDeclaration(
                    decorators,
                    modifiers,
                    asteriskToken,
                    name,
                    questionToken,
                    typeParameters,
                    parameters,
                    type,
                    body
            );
            // An exclamation token on a method is invalid syntax and will be handled by the grammar checker
            node->exclamationToken = exclamationToken;
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        shared<ObjectLiteralElementLike> parseObjectLiteralElement() {
            ZoneScoped;
            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();

            if (parseOptionalToken<DotDotDotToken>(SyntaxKind::DotDotDotToken)) {
                auto expression = parseAssignmentExpressionOrHigher();
                return withJSDoc(finishNode(factory.createSpreadAssignment(expression), pos), hasJSDoc);
            }

            auto decorators = parseDecorators();
            auto modifiers = parseModifiers();

            if (parseContextualModifier(SyntaxKind::GetKeyword)) {
                return parseAccessorDeclaration(pos, hasJSDoc, decorators, modifiers, SyntaxKind::GetAccessor);
            }
            if (parseContextualModifier(SyntaxKind::SetKeyword)) {
                return parseAccessorDeclaration(pos, hasJSDoc, decorators, modifiers, SyntaxKind::SetAccessor);
            }

            auto asteriskToken = parseOptionalToken<AsteriskToken>(SyntaxKind::AsteriskToken);
            auto tokenIsIdentifier = isIdentifier();
            auto name = parsePropertyName();

            // Disallowing of optional property assignments and definite assignment assertion happens in the grammar checker.
            auto questionToken = parseOptionalToken<QuestionToken>(SyntaxKind::QuestionToken);
            auto exclamationToken = parseOptionalToken<ExclamationToken>(SyntaxKind::ExclamationToken);

            if (asteriskToken || token() == SyntaxKind::OpenParenToken || token() == SyntaxKind::LessThanToken) {
                return parseMethodDeclaration(pos, hasJSDoc, decorators, modifiers, asteriskToken, name, questionToken, exclamationToken);
            }

            // check if it is short-hand property assignment or normal property assignment
            // NOTE: if token is EqualsToken it is interpreted as CoverInitializedName production
            // CoverInitializedName[Yield] :
            //     IdentifierReference[?Yield] Initializer[In, ?Yield]
            // this is necessary because ObjectLiteral productions are also used to cover grammar for ObjectAssignmentPattern
            sharedOpt<Node> node; //: Mutable<ShorthandPropertyAssignment | PropertyAssignment>;
            auto isShorthandPropertyAssignment = tokenIsIdentifier && (token() != SyntaxKind::ColonToken);
            if (isShorthandPropertyAssignment) {
                auto equalsToken = parseOptionalToken<EqualsToken>(SyntaxKind::EqualsToken);
                sharedOpt<Expression> objectAssignmentInitializer = equalsToken ? allowInAnd<shared<Expression>>(CALLBACK(parseAssignmentExpressionOrHigher)) : nullptr;
                auto n = factory.createShorthandPropertyAssignment(name, objectAssignmentInitializer);
                // Save equals token for error reporting.
                // TODO(rbuckton): Consider manufacturing this when we need to report an error as it is otherwise not useful.
                n->equalsToken = equalsToken;
                n->questionToken = questionToken;
                n->exclamationToken = exclamationToken;
                node = n;
            } else {
                parseExpected(SyntaxKind::ColonToken);
                auto initializer = allowInAnd<shared<Expression>>(CALLBACK(parseAssignmentExpressionOrHigher));
                auto n = factory.createPropertyAssignment(name, initializer);
                n->questionToken = questionToken;
                n->exclamationToken = exclamationToken;
                node = n;
            }
            // Decorators, Modifiers, questionToken, and exclamationToken are not supported by property assignments and are reported in the grammar checker
            node->decorators = decorators;
            node->modifiers = modifiers;
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        shared<ObjectLiteralExpression> parseObjectLiteralExpression() {
            auto pos = getNodePos();
            auto openBracePosition = scanner.getTokenPos();
            auto openBraceParsed = parseExpected(SyntaxKind::OpenBraceToken);
            auto multiLine = scanner.hasPrecedingLineBreak();
            auto properties = parseDelimitedList(ParsingContext::ObjectLiteralMembers, CALLBACK(parseObjectLiteralElement), /*considerSemicolonAsDelimiter*/ true);
            parseExpectedMatchingBrackets(SyntaxKind::OpenBraceToken, SyntaxKind::CloseBraceToken, openBraceParsed, openBracePosition);
            return finishNode(factory.createObjectLiteralExpression(properties, multiLine), pos);
        }

        sharedOpt<Identifier> parseOptionalBindingIdentifier() {
            return isBindingIdentifier() ? parseBindingIdentifier() : nullptr;
        }

        shared<FunctionExpression> parseFunctionExpression() {
            // GeneratorExpression:
            //      function* BindingIdentifier [Yield][opt](FormalParameters[Yield]){ GeneratorBody }
            //
            // FunctionExpression:
            //      function BindingIdentifier[opt](FormalParameters){ FunctionBody }
            auto savedDecoratorContext = inDecoratorContext();
            setDecoratorContext(/*val*/ false);

            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();
            auto modifiers = parseModifiers();
            parseExpected(SyntaxKind::FunctionKeyword);
            auto asteriskToken = parseOptionalToken<AsteriskToken>(SyntaxKind::AsteriskToken);
            auto isGenerator = asteriskToken ? SignatureFlags::Yield : SignatureFlags::None;
            auto isAsync = some(modifiers, isAsyncModifier) ? SignatureFlags::Await : SignatureFlags::None;
            auto name = (int) isGenerator && (int) isAsync ? doInYieldAndAwaitContext<sharedOpt<Identifier>>(CALLBACK(parseOptionalBindingIdentifier)) :
                        (int) isGenerator ? doInYieldContext<sharedOpt<Identifier>>(CALLBACK(parseOptionalBindingIdentifier)) :
                        (int) isAsync ? doInAwaitContext<sharedOpt<Identifier>>(CALLBACK(parseOptionalBindingIdentifier)) :
                        parseOptionalBindingIdentifier();

            auto typeParameters = parseTypeParameters();
            auto parameters = parseParameters((int) isGenerator | (int) isAsync);
            auto type = parseReturnType(SyntaxKind::ColonToken, /*isType*/ false);
            auto body = parseFunctionBlock((int) isGenerator | (int) isAsync);

            setDecoratorContext(savedDecoratorContext);

            auto node = factory.createFunctionExpression(modifiers, asteriskToken, name, typeParameters, parameters, type, body);
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        bool isImplementsClause() {
            return token() == SyntaxKind::ImplementsKeyword && lookAhead<bool>(CALLBACK(nextTokenIsIdentifierOrKeyword));
        }

        sharedOpt<Identifier> parseNameOfClassDeclarationOrExpression() {
            // implements is a future reserved word so
            // 'class implements' might mean either
            // - class expression with omitted name, 'implements' starts heritage clause
            // - class with name 'implements'
            // 'isImplementsClause' helps to disambiguate between these two cases
            return isBindingIdentifier() && !isImplementsClause()
                   ? createIdentifier(isBindingIdentifier())
                   : nullptr;
        }

        shared<ExpressionWithTypeArguments> parseExpressionWithTypeArguments() {
            auto pos = getNodePos();
            auto expression = parseLeftHandSideExpressionOrHigher();
            if (expression->kind == SyntaxKind::ExpressionWithTypeArguments) {
                return reinterpret_pointer_cast<ExpressionWithTypeArguments>(expression);
            }
            auto typeArguments = tryParseTypeArguments();
            return finishNode(factory.createExpressionWithTypeArguments(expression, typeArguments), pos);
        }

        shared<HeritageClause> parseHeritageClause() {
            auto pos = getNodePos();
            auto tok = token();
            Debug::asserts(tok == SyntaxKind::ExtendsKeyword || tok == SyntaxKind::ImplementsKeyword); // isListElement() should ensure this.
            nextToken();
            auto types = parseDelimitedList(ParsingContext::HeritageClauseElement, CALLBACK(parseExpressionWithTypeArguments));
            return finishNode(factory.createHeritageClause(tok, types), pos);
        }

        sharedOpt<NodeArray> parseHeritageClauses() {
            // ClassTail[Yield,Await] : (Modified) See 14.5
            //      ClassHeritage[?Yield,?Await]opt { ClassBody[?Yield,?Await]opt }

            if (isHeritageClause()) {
                return parseList(ParsingContext::HeritageClauses, CALLBACK(parseHeritageClause));
            }

            return nullptr;
        }

        shared<Block> parseClassStaticBlockBody() {
            auto savedYieldContext = inYieldContext();
            auto savedAwaitContext = inAwaitContext();

            setYieldContext(false);
            setAwaitContext(true);

            auto body = parseBlock(/*ignoreMissingOpenBrace*/ false);

            setYieldContext(savedYieldContext);
            setAwaitContext(savedAwaitContext);

            return body;
        }

        shared<ClassStaticBlockDeclaration> parseClassStaticBlockDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray> decorators, sharedOpt<NodeArray> modifiers) {
            parseExpectedToken<StaticKeyword>(SyntaxKind::StaticKeyword);
            auto body = parseClassStaticBlockBody();
            return withJSDoc(finishNode(factory.createClassStaticBlockDeclaration(decorators, modifiers, body), pos), hasJSDoc);
        }

        bool parseConstructorName() {
            if (token() == SyntaxKind::ConstructorKeyword) {
                return parseExpected(SyntaxKind::ConstructorKeyword);
            }
            if (token() == SyntaxKind::StringLiteral && lookAhead<SyntaxKind>(CALLBACK(nextToken)) == SyntaxKind::OpenParenToken) {
                return tryParse<bool>([this]()->bool {
                    auto literalNode = parseLiteralNode();
                    return literalNode->text == "constructor";
                });
            }
            return false;
        }

        sharedOpt<ConstructorDeclaration> tryParseConstructorDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray> decorators, sharedOpt<NodeArray> modifiers) {
            return tryParse<sharedOpt<ConstructorDeclaration>>([this, &decorators, &modifiers, &pos, &hasJSDoc]()->sharedOpt<ConstructorDeclaration> {
                if (parseConstructorName()) {
                    auto typeParameters = parseTypeParameters();
                    auto parameters = parseParameters(SignatureFlags::None);
                    auto type = parseReturnType(SyntaxKind::ColonToken, /*isType*/ false);
                    auto body = parseFunctionBlockOrSemicolon((int) SignatureFlags::None, Diagnostics::or_expected);
                    auto node = factory.createConstructorDeclaration(decorators, modifiers, parameters, body);
                    // Attach `typeParameters` and `type` if they exist so that we can report them in the grammar checker.
                    node->typeParameters = typeParameters;
                    node->type = type;
                    return withJSDoc(finishNode(node, pos), hasJSDoc);
                }
                return nullptr;
            });
        }

        bool isDeclareModifier(shared<Node> modifier) {
            return modifier->kind == SyntaxKind::DeclareKeyword;
        }

        shared<PropertyDeclaration> parsePropertyDeclaration(
                int pos,
                bool hasJSDoc,
                sharedOpt<NodeArray> decorators,
                sharedOpt<NodeArray> modifiers,
                shared<Node> name,
                sharedOpt<QuestionToken> questionToken
        ) {
            sharedOpt<ExclamationToken> exclamationToken = !questionToken && !scanner.hasPrecedingLineBreak() ? parseOptionalToken<ExclamationToken>(SyntaxKind::ExclamationToken) : nullptr;
            auto type = parseTypeAnnotation();
            auto initializer = doOutsideOfContext<sharedOpt<Expression>>((int) NodeFlags::YieldContext | (int) NodeFlags::AwaitContext | (int) NodeFlags::DisallowInContext, CALLBACK(parseInitializer));
            parseSemicolonAfterPropertyName(name, type, initializer);
            auto node = factory.createPropertyDeclaration(decorators, modifiers, name, questionToken ? (shared<Node>) questionToken : (shared<Node>) exclamationToken, type, initializer);
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        shared<Node> parsePropertyOrMethodDeclaration(
                int pos,
                bool hasJSDoc,
                sharedOpt<NodeArray> decorators,
                sharedOpt<NodeArray> modifiers
        ) {
            auto asteriskToken = parseOptionalToken<AsteriskToken>(SyntaxKind::AsteriskToken);
            auto name = parsePropertyName();
            // Note: this is not legal as per the grammar.  But we allow it in the parser and
            // report an error in the grammar checker.
            auto questionToken = parseOptionalToken<QuestionToken>(SyntaxKind::QuestionToken);
            if (asteriskToken || token() == SyntaxKind::OpenParenToken || token() == SyntaxKind::LessThanToken) {
                return parseMethodDeclaration(pos, hasJSDoc, decorators, modifiers, asteriskToken, name, questionToken, /*exclamationToken*/ {}, Diagnostics::or_expected);
            }
            return parsePropertyDeclaration(pos, hasJSDoc, decorators, modifiers, name, questionToken);
        }

        shared<Node> parseClassElement() {
            auto pos = getNodePos();
            if (token() == SyntaxKind::SemicolonToken) {
                nextToken();
                return finishNode(factory.createSemicolonClassElement(), pos);
            }

            auto hasJSDoc = hasPrecedingJSDocComment();
            auto decorators = parseDecorators();
            auto modifiers = parseModifiers(/*permitInvalidConstAsModifier*/ true, /*stopOnStartOfClassStaticBlock*/ true);
            if (token() == SyntaxKind::StaticKeyword && lookAhead<bool>(CALLBACK(nextTokenIsOpenBrace))) {
                return parseClassStaticBlockDeclaration(pos, hasJSDoc, decorators, modifiers);
            }

            if (parseContextualModifier(SyntaxKind::GetKeyword)) {
                return parseAccessorDeclaration(pos, hasJSDoc, decorators, modifiers, SyntaxKind::GetAccessor);
            }

            if (parseContextualModifier(SyntaxKind::SetKeyword)) {
                return parseAccessorDeclaration(pos, hasJSDoc, decorators, modifiers, SyntaxKind::SetAccessor);
            }

            if (token() == SyntaxKind::ConstructorKeyword || token() == SyntaxKind::StringLiteral) {
                auto constructorDeclaration = tryParseConstructorDeclaration(pos, hasJSDoc, decorators, modifiers);
                if (constructorDeclaration) {
                    return constructorDeclaration;
                }
            }

            if (isIndexSignature()) {
                return parseIndexSignatureDeclaration(pos, hasJSDoc, decorators, modifiers);
            }

            // It is very important that we check this *after* checking indexers because
            // the [ token can start an index signature or a computed property name
            if (tokenIsIdentifierOrKeyword(token()) ||
                token() == SyntaxKind::StringLiteral ||
                token() == SyntaxKind::NumericLiteral ||
                token() == SyntaxKind::AsteriskToken ||
                token() == SyntaxKind::OpenBracketToken) {
                auto isAmbient = some(modifiers, CALLBACK(isDeclareModifier));
                if (isAmbient && modifiers) {
                    for (auto &&m: modifiers->list) {
                        m->flags |= (int) NodeFlags::Ambient;
                    }
                    return doInsideOfContext<shared<Node>>((int) NodeFlags::Ambient, [&]() {
                        return parsePropertyOrMethodDeclaration(pos, hasJSDoc, decorators, modifiers);
                    });
                } else {
                    return parsePropertyOrMethodDeclaration(pos, hasJSDoc, decorators, modifiers);
                }
            }

            if (decorators || modifiers) {
                // treat this as a property declaration with a missing name.
                auto name = createMissingNode<Identifier>(SyntaxKind::Identifier, /*reportAtCurrentPosition*/ true, Diagnostics::Declaration_expected);
                return parsePropertyDeclaration(pos, hasJSDoc, decorators, modifiers, name, /*questionToken*/ {});
            }

            // 'isClassMemberStart' should have hinted not to attempt parsing.
            throw runtime_error("Should not have attempted to parse class member declaration.");
        }

        shared<NodeArray> parseClassMembers() {
            return parseList(ParsingContext::ClassMembers, CALLBACK(parseClassElement));
        }

        shared<NodeUnion(ClassLikeDeclaration)> parseClassDeclarationOrExpression(int pos, bool hasJSDoc, sharedOpt<NodeArray> decorators, sharedOpt<NodeArray> modifiers, SyntaxKind kind) {
            auto savedAwaitContext = inAwaitContext();
            parseExpected(SyntaxKind::ClassKeyword);

            // We don't parse the name here in await context, instead we will report a grammar error in the checker.
            auto name = parseNameOfClassDeclarationOrExpression();
            auto typeParameters = parseTypeParameters();
            if (some(modifiers, isExportModifier)) setAwaitContext(/*value*/ true);
            auto heritageClauses = parseHeritageClauses();

            shared<NodeArray> members;
            if (parseExpected(SyntaxKind::OpenBraceToken)) {
                // ClassTail[Yield,Await] : (Modified) See 14.5
                //      ClassHeritage[?Yield,?Await]opt { ClassBody[?Yield,?Await]opt }
                members = parseClassMembers();
                parseExpected(SyntaxKind::CloseBraceToken);
            } else {
                members = createMissingList();
            }
            setAwaitContext(savedAwaitContext);
            auto node = kind == SyntaxKind::ClassDeclaration
                        ? (shared<Node>) factory.createClassDeclaration(decorators, modifiers, name, typeParameters, heritageClauses, members)
                        : (shared<Node>) factory.createClassExpression(decorators, modifiers, name, typeParameters, heritageClauses, members);
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        shared<ClassExpression> parseClassExpression() {
            return reinterpret_pointer_cast<ClassExpression>(parseClassDeclarationOrExpression(getNodePos(), hasPrecedingJSDocComment(), /*decorators*/ {}, /*modifiers*/ {}, SyntaxKind::ClassExpression));
        }

        shared<PrimaryExpression> parseNewExpressionOrNewDotTarget() {
            auto pos = getNodePos();
            parseExpected(SyntaxKind::NewKeyword);
            if (parseOptional(SyntaxKind::DotToken)) {
                auto name = parseIdentifierName();
                return finishNode(factory.createMetaProperty(SyntaxKind::NewKeyword, name), pos);
            }
            auto expressionPos = getNodePos();
            shared<Expression> expression = parseMemberExpressionRest(expressionPos, parsePrimaryExpression(), /*allowOptionalChain*/ false);
            sharedOpt<NodeArray> typeArguments;
            // Absorb type arguments into NewExpression when preceding expression is ExpressionWithTypeArguments
            if (expression->kind == SyntaxKind::ExpressionWithTypeArguments) {
                typeArguments = expression->to<ExpressionWithTypeArguments>().typeArguments;
                expression = expression->to<ExpressionWithTypeArguments>().expression;
            }
            sharedOpt<NodeArray> argumentList;
            if (token() == SyntaxKind::OpenParenToken) argumentList = parseArgumentList();

            return finishNode(factory.createNewExpression(expression, typeArguments, argumentList), pos);
        }

        shared<PrimaryExpression> parsePrimaryExpression() {
            switch (token()) {
                case SyntaxKind::NumericLiteral:
                case SyntaxKind::BigIntLiteral:
                case SyntaxKind::StringLiteral:
                case SyntaxKind::NoSubstitutionTemplateLiteral:
                    return parseLiteralNode();
                case SyntaxKind::ThisKeyword:
                case SyntaxKind::SuperKeyword:
                case SyntaxKind::NullKeyword:
                case SyntaxKind::TrueKeyword:
                case SyntaxKind::FalseKeyword:
                    return parseTokenNode<PrimaryExpression>();
                case SyntaxKind::OpenParenToken:
                    return parseParenthesizedExpression();
                case SyntaxKind::OpenBracketToken:
                    return parseArrayLiteralExpression();
                case SyntaxKind::OpenBraceToken:
                    return parseObjectLiteralExpression();
                case SyntaxKind::AsyncKeyword:
                    // Async arrow functions are parsed earlier in parseAssignmentExpressionOrHigher.
                    // If we encounter `async [no LineTerminator here] function` then this is an async
                    // function; otherwise, its an identifier.
                    if (!lookAhead<bool>(CALLBACK(nextTokenIsFunctionKeywordOnSameLine))) {
                        break;
                    }

                    return parseFunctionExpression();
                case SyntaxKind::ClassKeyword:
                    return parseClassExpression();
                case SyntaxKind::FunctionKeyword:
                    return parseFunctionExpression();
                case SyntaxKind::NewKeyword:
                    return parseNewExpressionOrNewDotTarget();
                case SyntaxKind::SlashToken:
                case SyntaxKind::SlashEqualsToken:
                    if (reScanSlashToken() == SyntaxKind::RegularExpressionLiteral) {
                        return parseLiteralNode();
                    }
                    break;
                case SyntaxKind::TemplateHead:
                    return parseTemplateExpression(/* isTaggedTemplate */ false);
                case SyntaxKind::PrivateIdentifier:
                    return parsePrivateIdentifier();
            }

            return parseIdentifier(Diagnostics::Expression_expected);
        }

        shared<MemberExpression> parseMemberExpressionOrHigher() {
            ZoneScoped;
            // Note: to make our lives simpler, we decompose the NewExpression productions and
            // place ObjectCreationExpression and FunctionExpression into PrimaryExpression.
            // like so:
            //
            //   PrimaryExpression : See 11.1
            //      this
            //      Identifier
            //      Literal
            //      ArrayLiteral
            //      ObjectLiteral
            //      (Expression)
            //      FunctionExpression
            //      new MemberExpression Arguments?
            //
            //   MemberExpression : See 11.2
            //      PrimaryExpression
            //      MemberExpression[Expression]
            //      MemberExpression.IdentifierName
            //
            //   CallExpression : See 11.2
            //      MemberExpression
            //      CallExpression Arguments
            //      CallExpression[Expression]
            //      CallExpression.IdentifierName
            //
            // Technically this is ambiguous.  i.e. CallExpression defines:
            //
            //   CallExpression:
            //      CallExpression Arguments
            //
            // If you see: "new Foo()"
            //
            // Then that could be treated as a single ObjectCreationExpression, or it could be
            // treated as the invocation of "new Foo".  We disambiguate that in code (to match
            // the original grammar) by making sure that if we see an ObjectCreationExpression
            // we always consume arguments if they are there. So we treat "new Foo()" as an
            // object creation only, and not at all as an invocation.  Another way to think
            // about this is that for every "new" that we see, we will consume an argument list if
            // it is there as part of the *associated* object creation node.  Any additional
            // argument lists we see, will become invocation expressions.
            //
            // Because there are no other places in the grammar now that refer to FunctionExpression
            // or ObjectCreationExpression, it is safe to push down into the PrimaryExpression
            // production.
            //
            // Because CallExpression and MemberExpression are left recursive, we need to bottom out
            // of the recursion immediately.  So we parse out a primary expression to start with.
            auto pos = getNodePos();
            auto expression = parsePrimaryExpression();
            return parseMemberExpressionRest(pos, expression, /*allowOptionalChain*/ true);
        }

        shared<MemberExpression> parseSuperExpression() {
            ZoneScoped;
            auto pos = getNodePos();
            auto expression = parseTokenNode<PrimaryExpression>();
            if (token() == SyntaxKind::LessThanToken) {
                auto startPos = getNodePos();
                auto typeArguments = tryParse<sharedOpt<NodeArray>>(CALLBACK(parseTypeArgumentsInExpression));
                if (typeArguments) {
                    parseErrorAt(startPos, getNodePos(), Diagnostics::super_may_not_use_type_arguments);
                }
            }

            if (token() == SyntaxKind::OpenParenToken || token() == SyntaxKind::DotToken || token() == SyntaxKind::OpenBracketToken) {
                return expression;
            }

            // If we have seen "super" it must be followed by '(' or '.'.
            // If it wasn't then just try to parse out a '.' and report an error.
            parseExpectedToken<DotToken>(SyntaxKind::DotToken, Diagnostics::super_must_be_followed_by_an_argument_list_or_member_access);
            // private names will never work with `super` (`super.#foo`), but that's a semantic error, not syntactic
            return finishNode(factory.createPropertyAccessExpression(expression, parseRightSideOfDot(/*allowIdentifierNames*/ true, /*allowPrivateIdentifiers*/ true)), pos);
        }

        shared<LeftHandSideExpression> parseLeftHandSideExpressionOrHigher() {
            ZoneScoped;
            // Original Ecma:
            // LeftHandSideExpression: See 11.2
            //      NewExpression
            //      CallExpression
            //
            // Our simplification:
            //
            // LeftHandSideExpression: See 11.2
            //      MemberExpression
            //      CallExpression
            //
            // See comment in parseMemberExpressionOrHigher on how we replaced NewExpression with
            // MemberExpression to make our lives easier.
            //
            // to best understand the below code, it's important to see how CallExpression expands
            // out into its own productions:
            //
            // CallExpression:
            //      MemberExpression Arguments
            //      CallExpression Arguments
            //      CallExpression[Expression]
            //      CallExpression.IdentifierName
            //      import (AssignmentExpression)
            //      super Arguments
            //      super.IdentifierName
            //
            // Because of the recursion in these calls, we need to bottom out first. There are three
            // bottom out states we can run into: 1) We see 'super' which must start either of
            // the last two CallExpression productions. 2) We see 'import' which must start import call.
            // 3)we have a MemberExpression which either completes the LeftHandSideExpression,
            // or starts the beginning of the first four CallExpression productions.
            auto pos = getNodePos();
            sharedOpt<MemberExpression> expression;
            if (token() == SyntaxKind::ImportKeyword) {
                if (lookAhead<bool>(CALLBACK(nextTokenIsOpenParenOrLessThan))) {
                    // We don't want to eagerly consume all import keyword as import call expression so we look ahead to find "("
                    // For example:
                    //      var foo3 = require("subfolder
                    //      import * as foo1 from "module-from-node
                    // We want this import to be a statement rather than import call expression
                    sourceFlags |= (int) NodeFlags::PossiblyContainsDynamicImport;
                    expression = parseTokenNode<PrimaryExpression>();
                } else if (lookAhead<bool>(CALLBACK(nextTokenIsDot))) {
                    // This is an 'import.*' metaproperty (i.e. 'import.meta')
                    nextToken(); // advance past the 'import'
                    nextToken(); // advance past the dot
                    expression = finishNode(factory.createMetaProperty(SyntaxKind::ImportKeyword, parseIdentifierName()), pos);
                    sourceFlags |= (int) NodeFlags::PossiblyContainsImportMeta;
                } else {
                    expression = parseMemberExpressionOrHigher();
                }
            } else {
                expression = token() == SyntaxKind::SuperKeyword ? parseSuperExpression() : parseMemberExpressionOrHigher();
            }

            // Now, we *may* be complete.  However, we might have consumed the start of a
            // CallExpression or OptionalExpression.  As such, we need to consume the rest
            // of it here to be complete.
            return parseCallExpressionRest(pos, expression);
        }

        shared<AsExpression> makeAsExpression(shared<Expression> left, shared<TypeNode> right) {
            return finishNode(factory.createAsExpression(left, right), left->pos);
        }

        shared<Expression> parseBinaryExpressionOrHigher(int precedence) {
            ZoneScoped;
            auto pos = getNodePos();
            auto leftOperand = parseUnaryExpressionOrHigher();
            return parseBinaryExpressionRest(precedence, leftOperand, pos);
        }

        shared<Expression> parseBinaryExpressionRest(int precedence, shared<Expression> leftOperand, int pos) {
            ZoneScoped;
            while (true) {
                // We either have a binary operator here, or we're finished.  We call
                // reScanGreaterToken so that we merge token sequences like > and = into >=

                reScanGreaterToken();
                auto newPrecedence = getBinaryOperatorPrecedence(token());

                // Check the precedence to see if we should "take" this operator
                // - For left associative operator (all operator but **), consume the operator,
                //   recursively call the function below, and parse binaryExpression as a rightOperand
                //   of the caller if the new precedence of the operator is greater then or equal to the current precedence.
                //   For example:
                //      a - b - c;
                //            ^token; leftOperand = b. Return b to the caller as a rightOperand
                //      a * b - c
                //            ^token; leftOperand = b. Return b to the caller as a rightOperand
                //      a - b * c;
                //            ^token; leftOperand = b. Return b * c to the caller as a rightOperand
                // - For right associative operator (**), consume the operator, recursively call the function
                //   and parse binaryExpression as a rightOperand of the caller if the new precedence of
                //   the operator is strictly grater than the current precedence
                //   For example:
                //      a ** b ** c;
                //             ^^token; leftOperand = b. Return b ** c to the caller as a rightOperand
                //      a - b ** c;
                //            ^^token; leftOperand = b. Return b ** c to the caller as a rightOperand
                //      a ** b - c
                //             ^token; leftOperand = b. Return b to the caller as a rightOperand
                auto consumeCurrentOperator = token() == SyntaxKind::AsteriskAsteriskToken ?
                                              newPrecedence >= (int) precedence :
                                              newPrecedence > (int) precedence;

                if (!consumeCurrentOperator) {
                    break;
                }

                if (token() == SyntaxKind::InKeyword && inDisallowInContext()) {
                    break;
                }

                if (token() == SyntaxKind::AsKeyword) {
                    // Make sure we *do* perform ASI for constructs like this:
                    //    var x = foo
                    //    as (Bar)
                    // This should be parsed as an initialized variable, followed
                    // by a function call to 'as' with the argument 'Bar'
                    if (scanner.hasPrecedingLineBreak()) {
                        break;
                    } else {
                        nextToken();
                        leftOperand = makeAsExpression(leftOperand, parseType());
                    }
                } else {

                    leftOperand = makeBinaryExpression(leftOperand, parseTokenNode<Node>(), parseBinaryExpressionOrHigher(newPrecedence), pos);
                }
            }

            return leftOperand;
        }

        /**
         * Parse ES7 UpdateExpression. UpdateExpression is used instead of ES6's PostFixExpression.
         *
         * ES7 UpdateExpression[yield]:
         *      1) LeftHandSideExpression[?yield]
         *      2) LeftHandSideExpression[?yield] [[no LineTerminator here]]++
         *      3) LeftHandSideExpression[?yield] [[no LineTerminator here]]--
         *      4) ++LeftHandSideExpression[?yield]
         *      5) --LeftHandSideExpression[?yield]
         * In TypeScript (2), (3) are parsed as PostfixUnaryExpression. (4), (5) are parsed as PrefixUnaryExpression
         */
        shared<Expression> parseUpdateExpression() {
            ZoneScoped;
            if (token() == SyntaxKind::PlusPlusToken || token() == SyntaxKind::MinusMinusToken) {
                auto pos = getNodePos();
                return finishNode(factory.createPrefixUnaryExpression(token(), nextTokenAnd<shared<LeftHandSideExpression>>(CALLBACK(parseLeftHandSideExpressionOrHigher))), pos);
            } else if (languageVariant == LanguageVariant::JSX && token() == SyntaxKind::LessThanToken && lookAhead<bool>(CALLBACK(nextTokenIsIdentifierOrKeywordOrGreaterThan))) {
                // JSXElement is part of primaryExpression
                return parseJsxElementOrSelfClosingElementOrFragment(/*inExpressionContext*/ true);
            }

            auto expression = parseLeftHandSideExpressionOrHigher();

            assert(isLeftHandSideExpression(expression));
            if ((token() == SyntaxKind::PlusPlusToken || token() == SyntaxKind::MinusMinusToken) && !scanner.hasPrecedingLineBreak()) {
                auto operatorToken = token();
                nextToken();
                return finishNode(factory.createPostfixUnaryExpression(expression, operatorToken), expression->pos);
            }

            return expression;
        }

        shared<PrefixUnaryExpression> parsePrefixUnaryExpression() {
            auto pos = getNodePos();
            return finishNode(factory.createPrefixUnaryExpression(token(), nextTokenAnd<shared<UnaryExpression>>(CALLBACK(parseSimpleUnaryExpression))), pos);
        }

        shared<DeleteExpression> parseDeleteExpression() {
            auto pos = getNodePos();
            return finishNode(factory.createDeleteExpression(nextTokenAnd<shared<UnaryExpression>>(CALLBACK(parseSimpleUnaryExpression))), pos);
        }

        shared<TypeOfExpression> parseTypeOfExpression() {
            auto pos = getNodePos();
            return finishNode(factory.createTypeOfExpression(nextTokenAnd<shared<UnaryExpression>>(CALLBACK(parseSimpleUnaryExpression))), pos);
        }

        shared<VoidExpression> parseVoidExpression() {
            auto pos = getNodePos();
            return finishNode(factory.createVoidExpression(nextTokenAnd<shared<UnaryExpression>>(CALLBACK(parseSimpleUnaryExpression))), pos);
        }

        shared<AwaitExpression> parseAwaitExpression() {
            auto pos = getNodePos();
            return finishNode(factory.createAwaitExpression(nextTokenAnd<shared<UnaryExpression>>(CALLBACK(parseSimpleUnaryExpression))), pos);
        }

        bool isAwaitExpression() {
            if (token() == SyntaxKind::AwaitKeyword) {
                if (inAwaitContext()) {
                    return true;
                }

                // here we are using similar heuristics as 'isYieldExpression'
                return lookAhead<bool>(CALLBACK(nextTokenIsIdentifierOrKeywordOrLiteralOnSameLine));
            }

            return false;
        }

        /**
         * Parse ES7 simple-unary expression or higher:
         *
         * ES7 UnaryExpression:
         *      1) UpdateExpression[?yield]
         *      2) delete UnaryExpression[?yield]
         *      3) void UnaryExpression[?yield]
         *      4) typeof UnaryExpression[?yield]
         *      5) + UnaryExpression[?yield]
         *      6) - UnaryExpression[?yield]
         *      7) ~ UnaryExpression[?yield]
         *      8) ! UnaryExpression[?yield]
         *      9) [+Await] await UnaryExpression[?yield]
         */
        shared<UnaryExpression> parseSimpleUnaryExpression() {
            ZoneScoped;
            switch (token()) {
                case SyntaxKind::PlusToken:
                case SyntaxKind::MinusToken:
                case SyntaxKind::TildeToken:
                case SyntaxKind::ExclamationToken:
                    return parsePrefixUnaryExpression();
                case SyntaxKind::DeleteKeyword:
                    return parseDeleteExpression();
                case SyntaxKind::TypeOfKeyword:
                    return parseTypeOfExpression();
                case SyntaxKind::VoidKeyword:
                    return parseVoidExpression();
                case SyntaxKind::LessThanToken:
                    // This is modified UnaryExpression grammar in TypeScript
                    //  UnaryExpression (modified):
                    //      < type > UnaryExpression
                    return parseTypeAssertion();
                case SyntaxKind::AwaitKeyword:
                    if (isAwaitExpression()) {
                        return parseAwaitExpression();
                    }
                    // falls through
                default:
                    return reinterpret_pointer_cast<UnaryExpression>(parseUpdateExpression());
            }
        }

        shared<TypeAssertion> parseTypeAssertion() {
            auto pos = getNodePos();
            parseExpected(SyntaxKind::LessThanToken);
            auto type = parseType();
            parseExpected(SyntaxKind::GreaterThanToken);
            auto expression = parseSimpleUnaryExpression();
            return finishNode(factory.createTypeAssertion(type, expression), pos);
        }

        /**
         * Parse ES7 exponential expression and await expression
         *
         * ES7 ExponentiationExpression:
         *      1) UnaryExpression[?Yield]
         *      2) UpdateExpression[?Yield] ** ExponentiationExpression[?Yield]
         *
         */
        shared<Expression> parseUnaryExpressionOrHigher() {
            ZoneScoped;
            /**
             * ES7 UpdateExpression:
             *      1) LeftHandSideExpression[?Yield]
             *      2) LeftHandSideExpression[?Yield][no LineTerminator here]++
             *      3) LeftHandSideExpression[?Yield][no LineTerminator here]--
             *      4) ++UnaryExpression[?Yield]
             *      5) --UnaryExpression[?Yield]
             */
            if (isUpdateExpression()) {
                auto pos = getNodePos();
                auto updateExpression = parseUpdateExpression();
                return token() == SyntaxKind::AsteriskAsteriskToken ?
                       parseBinaryExpressionRest(getBinaryOperatorPrecedence(token()), updateExpression, pos) : updateExpression;
            }

            /**
             * ES7 UnaryExpression:
             *      1) UpdateExpression[?yield]
             *      2) delete UpdateExpression[?yield]
             *      3) void UpdateExpression[?yield]
             *      4) typeof UpdateExpression[?yield]
             *      5) + UpdateExpression[?yield]
             *      6) - UpdateExpression[?yield]
             *      7) ~ UpdateExpression[?yield]
             *      8) ! UpdateExpression[?yield]
             */
            auto unaryOperator = token();
            auto simpleUnaryExpression = parseSimpleUnaryExpression();
            if (token() == SyntaxKind::AsteriskAsteriskToken) {
                auto pos = skipTrivia(sourceText, simpleUnaryExpression->pos);
                auto end = simpleUnaryExpression->end;
                if (simpleUnaryExpression->kind == SyntaxKind::TypeAssertionExpression) {
                    parseErrorAt(pos, end, Diagnostics::A_type_assertion_expression_is_not_allowed_in_the_left_hand_side_of_an_exponentiation_expression_Consider_enclosing_the_expression_in_parentheses);
                } else {
                    parseErrorAt(pos, end, Diagnostics::An_unary_expression_with_the_0_operator_is_not_allowed_in_the_left_hand_side_of_an_exponentiation_expression_Consider_enclosing_the_expression_in_parentheses, tokenToString(unaryOperator));
                }
            }
            return simpleUnaryExpression;
        }

        shared<TypeParameterDeclaration> parseTypeParameter() {
            auto pos = getNodePos();
            auto modifiers = parseModifiers();
            auto name = parseIdentifier();

            sharedOpt<TypeNode> constraint;
            sharedOpt<Expression> expression;

            if (parseOptional(SyntaxKind::ExtendsKeyword)) {
                // It's not uncommon for people to write improper constraints to a generic.  If the
                // user writes a constraint that is an expression and not an actual type, then parse
                // it out as an expression (so we can recover well), but report that a type is needed
                // instead.
                if (isStartOfType() || !isStartOfExpression()) {
                    constraint = parseType();
                } else {
                    // It was not a type, and it looked like an expression.  Parse out an expression
                    // here so we recover well.  Note: it is important that we call parseUnaryExpression
                    // and not parseExpression here.  If the user has:
                    //
                    //      <T extends "">
                    //
                    // We do *not* want to consume the `>` as we're consuming the expression for "".
                    expression = parseUnaryExpressionOrHigher();
                }
            }

            sharedOpt<TypeNode> defaultType;
            if (parseOptional(SyntaxKind::EqualsToken)) defaultType = parseType();

            auto node = factory.createTypeParameterDeclaration(modifiers, name, constraint, defaultType);
            node->expression = expression;
            return finishNode(node, pos);
        }

        shared<TypeNode> parseFunctionOrConstructorType() {
            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();
            auto modifiers = parseModifiersForConstructorType();
            auto isConstructorType = parseOptional(SyntaxKind::NewKeyword);
            auto typeParameters = parseTypeParameters();
            auto parameters = parseParameters(SignatureFlags::Type);
            auto type = parseReturnType(SyntaxKind::EqualsGreaterThanToken, /*isType*/ false);
            auto node = isConstructorType
                        ? (shared<TypeNode>) factory.createConstructorTypeNode1(modifiers, typeParameters, parameters, type)
                        : (shared<TypeNode>) factory.createFunctionTypeNode(typeParameters, parameters, type);
            if (!isConstructorType) node->modifiers = modifiers;
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        shared<TypeOperatorNode> parseTypeOperator(SyntaxKind operatorKind) {
            auto pos = getNodePos();
            parseExpected(operatorKind);
            return finishNode(factory.createTypeOperatorNode(operatorKind, parseTypeOperatorOrHigher()), pos);
        }

        sharedOpt<TypeNode> tryParseConstraintOfInferType() {
            if (parseOptional(SyntaxKind::ExtendsKeyword)) {
                auto constraint = disallowConditionalTypesAnd<shared<TypeNode>>(CALLBACK(parseType));
                if (inDisallowConditionalTypesContext() || token() != SyntaxKind::QuestionToken) {
                    return constraint;
                }
            }
            return nullptr;
        }

        shared<TypeParameterDeclaration> parseTypeParameterOfInferType() {
            auto pos = getNodePos();
            auto name = parseIdentifier();
            auto constraint = tryParse<sharedOpt<TypeNode>>(CALLBACK(tryParseConstraintOfInferType));
            auto node = factory.createTypeParameterDeclaration(/*modifiers*/ {}, name, constraint);
            return finishNode(node, pos);
        }

        shared<InferTypeNode> parseInferType() {
            auto pos = getNodePos();
            parseExpected(SyntaxKind::InferKeyword);
            return finishNode(factory.createInferTypeNode(parseTypeParameterOfInferType()), pos);
        }

        sharedOpt<TypeNode> parseKeywordAndNoDot() {
            auto node = parseTokenNode<TypeNode>();
            return token() == SyntaxKind::DotToken ? nullptr : node;
        }

        shared<TypeNode> parseJSDocAllType() {
            throw runtime_error("JSDoc not supported");
//            auto pos = getNodePos();
//            nextToken();
//            return finishNode(factory.createJSDocAllType(), pos);
        }

        shared<LiteralTypeNode> parseLiteralTypeNode(optional<bool> negative = {}) {
            auto pos = getNodePos();
            if (negative && *negative) {
                nextToken();
            }
            shared<Expression> expression =
                    token() == SyntaxKind::TrueKeyword || token() == SyntaxKind::FalseKeyword || token() == SyntaxKind::NullKeyword ?
                    parseTokenNode<Expression>() :
                    parseLiteralLikeNode(token());
            if (negative) {
                expression = finishNode(factory.createPrefixUnaryExpression(SyntaxKind::MinusToken, expression), pos);
            }
            return finishNode(factory.createLiteralTypeNode(expression), pos);
        }

        bool isStartOfTypeOfImportType() {
            nextToken();
            return token() == SyntaxKind::ImportKeyword;
        }

        shared<AssertEntry> parseAssertEntry() {
            ZoneScoped;
            auto pos = getNodePos();
            shared<Node> name = tokenIsIdentifierOrKeyword(token()) ? (shared<Node>) parseIdentifierName() : (shared<Node>) parseLiteralLikeNode(SyntaxKind::StringLiteral);
            parseExpected(SyntaxKind::ColonToken);
            auto value = parseAssignmentExpressionOrHigher();
            return finishNode(factory.createAssertEntry(name, value), pos);
        }

        shared<AssertClause> parseAssertClause(optional<bool> skipAssertKeyword = {}) {
            auto pos = getNodePos();
            if (!isTrue(skipAssertKeyword)) {
                parseExpected(SyntaxKind::AssertKeyword);
            }
            auto openBracePosition = scanner.getTokenPos();
            if (parseExpected(SyntaxKind::OpenBraceToken)) {
                auto multiLine = scanner.hasPrecedingLineBreak();
                auto elements = parseDelimitedList(ParsingContext::AssertEntries, CALLBACK(parseAssertEntry), /*considerSemicolonAsDelimiter*/ true);
                if (!parseExpected(SyntaxKind::CloseBraceToken)) {
                    auto lastError = lastOrUndefined(parseDiagnostics);
                    if (lastError && lastError->code == Diagnostics::_0_expected->code) {
                        addRelatedInfo(
                                *lastError,
                                {createDetachedDiagnostic(fileName, openBracePosition, 1, Diagnostics::The_parser_expected_to_find_a_1_to_match_the_0_token_here, {"{", "}"})}
                        );
                    }
                }
                return finishNode(factory.createAssertClause(elements, multiLine), pos);
            } else {
                auto elements = createNodeArray({}, getNodePos(), /*end*/ {}, /*hasTrailingComma*/ false);
                return finishNode(factory.createAssertClause(elements, /*multiLine*/ false), pos);
            }
        }

        shared<ImportTypeAssertionContainer> parseImportTypeAssertions() {
            auto pos = getNodePos();
            auto openBracePosition = scanner.getTokenPos();
            parseExpected(SyntaxKind::OpenBraceToken);
            auto multiLine = scanner.hasPrecedingLineBreak();
            parseExpected(SyntaxKind::AssertKeyword);
            parseExpected(SyntaxKind::ColonToken);
            auto clause = parseAssertClause(/*skipAssertKeyword*/ true);
            if (!parseExpected(SyntaxKind::CloseBraceToken)) {
                auto lastError = lastOrUndefined(parseDiagnostics);
                if (lastError && lastError->code == Diagnostics::_0_expected->code) {
                    addRelatedInfo(
                            *lastError,
                            {createDetachedDiagnostic(fileName, openBracePosition, 1, Diagnostics::The_parser_expected_to_find_a_1_to_match_the_0_token_here, {"{", "}"})}
                    );
                }
            }
            return finishNode(factory.createImportTypeAssertionContainer(clause, multiLine), pos);
        }

        shared<ImportTypeNode> parseImportType() {
            sourceFlags |= (int) NodeFlags::PossiblyContainsDynamicImport;
            auto pos = getNodePos();
            auto isTypeOf = parseOptional(SyntaxKind::TypeOfKeyword);
            parseExpected(SyntaxKind::ImportKeyword);
            parseExpected(SyntaxKind::OpenParenToken);
            auto type = parseType();
            sharedOpt<ImportTypeAssertionContainer> assertions;
            if (parseOptional(SyntaxKind::CommaToken)) {
                assertions = parseImportTypeAssertions();
            }
            parseExpected(SyntaxKind::CloseParenToken);
            sharedOpt<Node> qualifier = parseOptional(SyntaxKind::DotToken) ? parseEntityNameOfTypeReference() : nullptr;
            auto typeArguments = parseTypeArgumentsOfTypeReference();
            return finishNode(factory.createImportTypeNode(type, assertions, qualifier, typeArguments, isTypeOf), pos);
        }

        bool isStartOfMappedType() {
            nextToken();
            if (token() == SyntaxKind::PlusToken || token() == SyntaxKind::MinusToken) {
                return nextToken() == SyntaxKind::ReadonlyKeyword;
            }
            if (token() == SyntaxKind::ReadonlyKeyword) {
                nextToken();
            }
            return token() == SyntaxKind::OpenBracketToken && nextTokenIsIdentifier() && nextToken() == SyntaxKind::InKeyword;
        }

        shared<TypeParameterDeclaration> parseMappedTypeParameter() {
            auto pos = getNodePos();
            auto name = parseIdentifierName();
            parseExpected(SyntaxKind::InKeyword);
            auto type = parseType();
            return finishNode(factory.createTypeParameterDeclaration(/*modifiers*/ {}, name, type, /*defaultType*/ {}), pos);
        }

        shared<MappedTypeNode> parseMappedType() {
            auto pos = getNodePos();
            parseExpected(SyntaxKind::OpenBraceToken);
            sharedOpt<Node> readonlyToken; //: ReadonlyKeyword | PlusToken | MinusToken | undefined;
            if (token() == SyntaxKind::ReadonlyKeyword || token() == SyntaxKind::PlusToken || token() == SyntaxKind::MinusToken) {
                readonlyToken = parseTokenNode<Node>();
                if (readonlyToken->kind != SyntaxKind::ReadonlyKeyword) {
                    parseExpected(SyntaxKind::ReadonlyKeyword);
                }
            }
            parseExpected(SyntaxKind::OpenBracketToken);
            auto typeParameter = parseMappedTypeParameter();
            sharedOpt<TypeNode> nameType = parseOptional(SyntaxKind::AsKeyword) ? parseType() : nullptr;
            parseExpected(SyntaxKind::CloseBracketToken);
            sharedOpt<Node> questionToken; //: QuestionToken | PlusToken | MinusToken | undefined;
            if (token() == SyntaxKind::QuestionToken || token() == SyntaxKind::PlusToken || token() == SyntaxKind::MinusToken) {
                questionToken = parseTokenNode<Node>();
                if (questionToken->kind != SyntaxKind::QuestionToken) {
                    parseExpected(SyntaxKind::QuestionToken);
                }
            }
            auto type = parseTypeAnnotation();
            parseSemicolon();
            auto members = parseList(ParsingContext::TypeMembers, [this]() { return parseTypeMember(); });
            parseExpected(SyntaxKind::CloseBraceToken);
            return finishNode(factory.createMappedTypeNode(readonlyToken, typeParameter, nameType, questionToken, type, members), pos);
        }

        bool isNextTokenColonOrQuestionColon() {
            return nextToken() == SyntaxKind::ColonToken || (token() == SyntaxKind::QuestionToken && nextToken() == SyntaxKind::ColonToken);
        }

        bool isTupleElementName() {
            if (token() == SyntaxKind::DotDotDotToken) {
                return tokenIsIdentifierOrKeyword(nextToken()) && isNextTokenColonOrQuestionColon();
            }
            return tokenIsIdentifierOrKeyword(token()) && isNextTokenColonOrQuestionColon();
        }

        shared<TypeNode> parseTupleElementType() {
            auto pos = getNodePos();
            if (parseOptional(SyntaxKind::DotDotDotToken)) {
                return finishNode(factory.createRestTypeNode(parseType()), pos);
            }
            auto type = parseType();
            //no JSDoc support
//            if (isJSDocNullableType(type) && type->pos == type->type->pos) {
//                auto node = factory.createOptionalTypeNode(type->type);
//                setTextRange(node, type);
//                Node->flags = type->flags;
//                return node;
//            }
            return type;
        }

        shared<TypeNode> parseTupleElementNameOrTupleElementType() {
            if (lookAhead<bool>(CALLBACK(isTupleElementName))) {
                auto pos = getNodePos();
                auto hasJSDoc = hasPrecedingJSDocComment();
                auto dotDotDotToken = parseOptionalToken<DotDotDotToken>(SyntaxKind::DotDotDotToken);
                auto name = parseIdentifierName();
                auto questionToken = parseOptionalToken<QuestionToken>(SyntaxKind::QuestionToken);
                parseExpected(SyntaxKind::ColonToken);
                auto type = parseTupleElementType();
                auto node = factory.createNamedTupleMember(dotDotDotToken, name, questionToken, type);
                return withJSDoc(finishNode(node, pos), hasJSDoc);
            }
            return parseTupleElementType();
        }

        shared<TupleTypeNode> parseTupleType() {
            auto pos = getNodePos();
            return finishNode(
                    factory.createTupleTypeNode(
                            parseBracketedList(ParsingContext::TupleElementTypes, CALLBACK(parseTupleElementNameOrTupleElementType), SyntaxKind::OpenBracketToken, SyntaxKind::CloseBracketToken)
                    ),
                    pos
            );
        }

        shared<TypeNode> parseParenthesizedType() {
            auto pos = getNodePos();
            parseExpected(SyntaxKind::OpenParenToken);
            auto type = parseType();
            parseExpected(SyntaxKind::CloseParenToken);
            return finishNode(factory.createParenthesizedType(type), pos);
        }

        shared<TypeNode> parseAssertsTypePredicate() {
            auto pos = getNodePos();
            auto assertsModifier = parseExpectedToken<AssertsKeyword>(SyntaxKind::AssertsKeyword);
            shared<Node> parameterName = token() == SyntaxKind::ThisKeyword ? (shared<Node>) parseThisTypeNode() : (shared<Node>) parseIdentifier();
            sharedOpt<TypeNode> type = parseOptional(SyntaxKind::IsKeyword) ? parseType() : nullptr;
            return finishNode(factory.createTypePredicateNode(assertsModifier, parameterName, type), pos);
        }

        shared<TemplateLiteralTypeSpan> parseTemplateTypeSpan() {
            auto pos = getNodePos();
            return finishNode(
                    factory.createTemplateLiteralTypeSpan(
                            parseType(),
                            parseLiteralOfTemplateSpan(/*isTaggedTemplate*/ false)
                    ),
                    pos
            );
        }

        shared<NodeArray> parseTemplateTypeSpans() {
            auto pos = getNodePos();
            auto list = make_shared<NodeArray>();
            shared<TemplateLiteralTypeSpan> node;
            do {
                node = parseTemplateTypeSpan();
                list->push(node);
            } while (node->literal->kind == SyntaxKind::TemplateMiddle);
            return createNodeArray(list, pos);
        }

        shared<TemplateLiteralTypeNode> parseTemplateType() {
            auto pos = getNodePos();
            return finishNode(
                    factory.createTemplateLiteralType(
                            parseTemplateHead(/*isTaggedTemplate*/ false),
                            parseTemplateTypeSpans()
                    ),
                    pos
            );
        }

        shared<TypeNode> parseNonArrayType() {
            switch (token()) {
                case SyntaxKind::AnyKeyword:
                case SyntaxKind::UnknownKeyword:
                case SyntaxKind::StringKeyword:
                case SyntaxKind::NumberKeyword:
                case SyntaxKind::BigIntKeyword:
                case SyntaxKind::SymbolKeyword:
                case SyntaxKind::BooleanKeyword:
                case SyntaxKind::UndefinedKeyword:
                case SyntaxKind::NeverKeyword:
                case SyntaxKind::ObjectKeyword: {
                    // If these are followed by a dot, then parse these out as a dotted type reference instead.
                    if (auto a = tryParse<sharedOpt<TypeNode>>(CALLBACK(parseKeywordAndNoDot))) return a;
                    return parseTypeReference();
                }
                case SyntaxKind::AsteriskEqualsToken:
                    // If there is '*=', treat it as * followed by postfix =
                    scanner.reScanAsteriskEqualsToken();
                    // falls through
                case SyntaxKind::AsteriskToken:
                    return parseJSDocAllType();
                case SyntaxKind::QuestionQuestionToken:
                    // If there is '??', treat it as prefix-'?' in JSDoc type.
                    scanner.reScanQuestionToken();
                    // falls through
                case SyntaxKind::QuestionToken:
                    throw runtime_error("JSDoc not supported");
//                    return parseJSDocUnknownOrNullableType();
                case SyntaxKind::FunctionKeyword:
                    throw runtime_error("JSDoc not supported");
//                    return parseJSDocFunctionType();
                case SyntaxKind::ExclamationToken:
                    throw runtime_error("JSDoc not supported");
//                    return parseJSDocNonNullableType();
                case SyntaxKind::NoSubstitutionTemplateLiteral:
                case SyntaxKind::StringLiteral:
                case SyntaxKind::NumericLiteral:
                case SyntaxKind::BigIntLiteral:
                case SyntaxKind::TrueKeyword:
                case SyntaxKind::FalseKeyword:
                case SyntaxKind::NullKeyword:
                    return parseLiteralTypeNode();
                case SyntaxKind::MinusToken:
                    if (lookAhead<bool>(CALLBACK(nextTokenIsNumericOrBigIntLiteral))) return parseLiteralTypeNode(/*negative*/ true);
                    return parseTypeReference();
                case SyntaxKind::VoidKeyword:
                    return parseTokenNode<TypeNode>();
                case SyntaxKind::ThisKeyword: {
                    auto thisKeyword = parseThisTypeNode();
                    if (token() == SyntaxKind::IsKeyword && !scanner.hasPrecedingLineBreak()) {
                        return parseThisTypePredicate(thisKeyword);
                    } else {
                        return thisKeyword;
                    }
                }
                case SyntaxKind::TypeOfKeyword:
                    if (lookAhead<bool>(CALLBACK(isStartOfTypeOfImportType))) return parseImportType();
                    return parseTypeQuery();
                case SyntaxKind::OpenBraceToken:
                    if (lookAhead<bool>(CALLBACK(isStartOfMappedType))) return parseMappedType();
                    return parseTypeLiteral();
                case SyntaxKind::OpenBracketToken:
                    return parseTupleType();
                case SyntaxKind::OpenParenToken:
                    return parseParenthesizedType();
                case SyntaxKind::ImportKeyword:
                    return parseImportType();
                case SyntaxKind::AssertsKeyword:
                    if (lookAhead<bool>(CALLBACK(nextTokenIsIdentifierOrKeywordOnSameLine))) return parseAssertsTypePredicate();
                    return parseTypeReference();
                case SyntaxKind::TemplateHead:
                    return parseTemplateType();
                default:
                    return parseTypeReference();
            }
        }

        shared<TypeNode> parsePostfixTypeOrHigher() {
            auto pos = getNodePos();
            auto type = parseNonArrayType();
            while (!scanner.hasPrecedingLineBreak()) {
                switch (token()) {
                    case SyntaxKind::ExclamationToken:
                        throw runtime_error("No JSDoc support");
//                        nextToken();
//                        type = finishNode(factory.createJSDocNonNullableType(type, /*postfix*/ true), pos);
                        break;
                    case SyntaxKind::QuestionToken:
                        // If next token is start of a type we have a conditional type
                        if (lookAhead<bool>(CALLBACK(nextTokenIsStartOfType))) {
                            return type;
                        }
                        throw runtime_error("No JSDoc support");
//                        nextToken();
//                        type = finishNode(factory.createJSDocNullableType(type, /*postfix*/ true), pos);
                        break;
                    case SyntaxKind::OpenBracketToken:
                        parseExpected(SyntaxKind::OpenBracketToken);
                        if (isStartOfType()) {
                            auto indexType = parseType();
                            parseExpected(SyntaxKind::CloseBracketToken);
                            type = finishNode(factory.createIndexedAccessTypeNode(type, indexType), pos);
                        } else {
                            parseExpected(SyntaxKind::CloseBracketToken);
                            type = finishNode(factory.createArrayTypeNode(type), pos);
                        }
                        break;
                    default:
                        return type;
                }
            }
            return type;
        }

        shared<TypeNode> parseTypeOperatorOrHigher() {
            auto operatorToken = token();
            switch (operatorToken) {
                case SyntaxKind::KeyOfKeyword:
                case SyntaxKind::UniqueKeyword:
                case SyntaxKind::ReadonlyKeyword:
                    return parseTypeOperator(operatorToken);
                case SyntaxKind::InferKeyword:
                    return parseInferType();
            }
            return allowConditionalTypesAnd<shared<TypeNode>>(CALLBACK(parsePostfixTypeOrHigher));
        }

        sharedOpt<TypeNode> parseFunctionOrConstructorTypeToError(bool isInUnionType) {
            // the function type and constructor type shorthand notation
            // are not allowed directly in unions and intersections, but we'll
            // try to parse them gracefully and issue a helpful message.
            if (isStartOfFunctionTypeOrConstructorType()) {
                auto type = parseFunctionOrConstructorType();
                shared<DiagnosticMessage> diagnostic;
                if (isFunctionTypeNode(type)) {
                    diagnostic = isInUnionType
                                 ? Diagnostics::Function_type_notation_must_be_parenthesized_when_used_in_a_union_type
                                 : Diagnostics::Function_type_notation_must_be_parenthesized_when_used_in_an_intersection_type;
                } else {
                    diagnostic = isInUnionType
                                 ? Diagnostics::Constructor_type_notation_must_be_parenthesized_when_used_in_a_union_type
                                 : Diagnostics::Constructor_type_notation_must_be_parenthesized_when_used_in_an_intersection_type;
                }
                parseErrorAtRange(type, diagnostic);
                return type;
            }
            return nullptr;
        }

        shared<TypeNode> parseUnionOrIntersectionType(
                SyntaxKind operatorKind,
                function<shared<TypeNode>()> parseConstituentType,
                function<shared<TypeNode>(shared<NodeArray>)> createTypeNode //: (types: NodeArray<TypeNode>) => UnionOrIntersectionTypeNode
        ) {
            auto pos = getNodePos();
            auto isUnionType = operatorKind == SyntaxKind::BarToken;
            auto hasLeadingOperator = parseOptional(operatorKind);
            sharedOpt<TypeNode> type = hasLeadingOperator ? parseFunctionOrConstructorTypeToError(isUnionType) : parseConstituentType();
            if (token() == operatorKind || hasLeadingOperator) {
                auto types = make_shared<NodeArray>(type);
                while (parseOptional(operatorKind)) {
                    if (auto a = parseFunctionOrConstructorTypeToError(isUnionType)) {
                        types->push(a);
                    } else {
                        types->push(parseConstituentType());
                    }
                }
                type = finishNode<TypeNode>(createTypeNode(types), pos);
            }
            return type;
        }

        shared<TypeNode> parseIntersectionTypeOrHigher() {
            return parseUnionOrIntersectionType(SyntaxKind::AmpersandToken, CALLBACK(parseTypeOperatorOrHigher), CALLBACK(factory.createIntersectionTypeNode));
        }

        shared<TypeNode> parseUnionTypeOrHigher() {
            return parseUnionOrIntersectionType(SyntaxKind::BarToken, CALLBACK(parseIntersectionTypeOrHigher), CALLBACK(factory.createUnionTypeNode));
        }

        shared<TypeNode> parseType() {
            ZoneScoped;
            if (contextFlags & (int) NodeFlags::TypeExcludesFlags) {
                return doOutsideOfContext<shared<TypeNode>>(NodeFlags::TypeExcludesFlags, CALLBACK(parseType));
            }

            if (isStartOfFunctionTypeOrConstructorType()) {
                return parseFunctionOrConstructorType();
            }
            auto pos = getNodePos();
            auto type = parseUnionTypeOrHigher();
            if (!inDisallowConditionalTypesContext() && !scanner.hasPrecedingLineBreak() && parseOptional(SyntaxKind::ExtendsKeyword)) {
                // The type following 'extends' is not permitted to be another conditional type
                auto extendsType = disallowConditionalTypesAnd<shared<TypeNode>>(CALLBACK(parseType));
                parseExpected(SyntaxKind::QuestionToken);
                auto trueType = allowConditionalTypesAnd<shared<TypeNode>>(CALLBACK(parseType));
                parseExpected(SyntaxKind::ColonToken);
                auto falseType = allowConditionalTypesAnd<shared<TypeNode>>(CALLBACK(parseType));
                return finishNode(factory.createConditionalTypeNode(type, extendsType, trueType, falseType), pos);
            }
            return type;
        }

        sharedOpt<TypeNode> parseTypeAnnotation() {
            ZoneScoped;
            return parseOptional(SyntaxKind::ColonToken) ? parseType() : nullptr;
        }

//        function isStartOfType(inStartOfParameter?: boolean): boolean {
//            switch (token()) {
//                case SyntaxKind::AnyKeyword:
//                case SyntaxKind::UnknownKeyword:
//                case SyntaxKind::StringKeyword:
//                case SyntaxKind::NumberKeyword:
//                case SyntaxKind::BigIntKeyword:
//                case SyntaxKind::BooleanKeyword:
//                case SyntaxKind::ReadonlyKeyword:
//                case SyntaxKind::SymbolKeyword:
//                case SyntaxKind::UniqueKeyword:
//                case SyntaxKind::VoidKeyword:
//                case SyntaxKind::UndefinedKeyword:
//                case SyntaxKind::NullKeyword:
//                case SyntaxKind::ThisKeyword:
//                case SyntaxKind::TypeOfKeyword:
//                case SyntaxKind::NeverKeyword:
//                case SyntaxKind::OpenBraceToken:
//                case SyntaxKind::OpenBracketToken:
//                case SyntaxKind::LessThanToken:
//                case SyntaxKind::BarToken:
//                case SyntaxKind::AmpersandToken:
//                case SyntaxKind::NewKeyword:
//                case SyntaxKind::StringLiteral:
//                case SyntaxKind::NumericLiteral:
//                case SyntaxKind::BigIntLiteral:
//                case SyntaxKind::TrueKeyword:
//                case SyntaxKind::FalseKeyword:
//                case SyntaxKind::ObjectKeyword:
//                case SyntaxKind::AsteriskToken:
//                case SyntaxKind::QuestionToken:
//                case SyntaxKind::ExclamationToken:
//                case SyntaxKind::DotDotDotToken:
//                case SyntaxKind::InferKeyword:
//                case SyntaxKind::ImportKeyword:
//                case SyntaxKind::AssertsKeyword:
//                case SyntaxKind::NoSubstitutionTemplateLiteral:
//                case SyntaxKind::TemplateHead:
//                    return true;
//                case SyntaxKind::FunctionKeyword:
//                    return !inStartOfParameter;
//                case SyntaxKind::MinusToken:
//                    return !inStartOfParameter && lookAhead(nextTokenIsNumericOrBigIntLiteral);
//                case SyntaxKind::OpenParenToken:
//                    // Only consider '(' the start of a type if followed by ')', '...', an identifier, a modifier,
//                    // or something that starts a type. We don't want to consider things like '(1)' a type.
//                    return !inStartOfParameter && lookAhead(isStartOfParenthesizedOrFunctionType);
//                default:
//                    return isIdentifier();
//            }
//        }
//
        sharedOpt<Identifier> parseTypePredicatePrefix() {
            auto id = parseIdentifier();
            if (token() == SyntaxKind::IsKeyword && !scanner.hasPrecedingLineBreak()) {
                nextToken();
                return id;
            }
            return nullptr;
        }

        shared<TypeNode> parseTypeOrTypePredicate() {
            auto pos = getNodePos();
            sharedOpt<Identifier> typePredicateVariable = isIdentifier() ? tryParse<sharedOpt<Identifier>>(CALLBACK(parseTypePredicatePrefix)) : nullptr;
            auto type = parseType();
            if (typePredicateVariable) {
                return finishNode(factory.createTypePredicateNode(/*assertsModifier*/ {}, typePredicateVariable, type), pos);
            } else {
                return type;
            }
        }

        shared<BinaryExpression> makeBinaryExpression(shared<Expression> left, shared<Node> operatorNode, shared<Expression> right, int pos) {
            auto n = factory.createBinaryExpression(left, operatorNode, right);
            return finishNode(n, pos);
        }

        shared<Expression> parseExpression() {
            ZoneScoped;
            // Expression[in]:
            //      AssignmentExpression[in]
            //      Expression[in] , AssignmentExpression[in]

            // clear the decorator context when parsing Expression, as it should be unambiguous when parsing a decorator
            auto saveDecoratorContext = inDecoratorContext();
            if (saveDecoratorContext) {
                setDecoratorContext(/*val*/ false);
            }

            auto pos = getNodePos();
            auto expr = parseAssignmentExpressionOrHigher();
            sharedOpt<Node> operatorToken;
            while ((operatorToken = parseOptionalToken<CommaToken>(SyntaxKind::CommaToken))) {
                expr = makeBinaryExpression(expr, operatorToken, parseAssignmentExpressionOrHigher(), pos);
            }

            if (saveDecoratorContext) {
                setDecoratorContext(/*val*/ true);
            }
            return expr;
        }

        bool nextTokenIsIdentifierOrKeywordOrLiteralOnSameLine() {
            nextToken();
            return (tokenIsIdentifierOrKeyword(token()) || token() == SyntaxKind::NumericLiteral || token() == SyntaxKind::BigIntLiteral || token() == SyntaxKind::StringLiteral) && !scanner.hasPrecedingLineBreak();
        }

        bool isYieldExpression() {
            if (token() == SyntaxKind::YieldKeyword) {
                // If we have a 'yield' keyword, and this is a context where yield expressions are
                // allowed, then definitely parse out a yield expression.
                if (inYieldContext()) {
                    return true;
                }

                // We're in a context where 'yield expr' is not allowed.  However, if we can
                // definitely tell that the user was trying to parse a 'yield expr' and not
                // just a normal expr that start with a 'yield' identifier, then parse out
                // a 'yield expr'.  We can then report an error later that they are only
                // allowed in generator expressions.
                //
                // for example, if we see 'yield(foo)', then we'll have to treat that as an
                // invocation expression of something called 'yield'.  However, if we have
                // 'yield foo' then that is not legal as a normal expression, so we can
                // definitely recognize this as a yield expression.
                //
                // for now we just check if the next token is an identifier.  More heuristics
                // can be added here later as necessary.  We just need to make sure that we
                // don't accidentally consume something legal.
                return lookAhead<bool>(CALLBACK(nextTokenIsIdentifierOrKeywordOrLiteralOnSameLine));
            }

            return false;
        }

        shared<YieldExpression> parseYieldExpression() {
            ZoneScoped;
            auto pos = getNodePos();

            // YieldExpression[In] :
            //      yield
            //      yield [no LineTerminator here] [Lexical goal InputElementRegExp]AssignmentExpression[?In, Yield]
            //      yield [no LineTerminator here] * [Lexical goal InputElementRegExp]AssignmentExpression[?In, Yield]
            nextToken();

            if (!scanner.hasPrecedingLineBreak() &&
                (token() == SyntaxKind::AsteriskToken || isStartOfExpression())) {
                return finishNode(
                        factory.createYieldExpression(
                                parseOptionalToken<AsteriskToken>(SyntaxKind::AsteriskToken),
                                parseAssignmentExpressionOrHigher()
                        ),
                        pos
                );
            } else {
                // if the next token is not on the same line as yield.  or we don't have an '*' or
                // the start of an expression, then this is just a simple "yield" expression.
                return finishNode(factory.createYieldExpression(/*asteriskToken*/ {}, /*expression*/ {}), pos);
            }
        }

        Tristate isParenthesizedArrowFunctionExpressionWorker() {
            if (token() == SyntaxKind::AsyncKeyword) {
                nextToken();
                if (scanner.hasPrecedingLineBreak()) {
                    return Tristate::False;
                }
                if (token() != SyntaxKind::OpenParenToken && token() != SyntaxKind::LessThanToken) {
                    return Tristate::False;
                }
            }

            auto first = token();
            auto second = nextToken();

            if (first == SyntaxKind::OpenParenToken) {
                if (second == SyntaxKind::CloseParenToken) {
                    // Simple cases: "() =>", "(): ", and "() {".
                    // This is an arrow function with no parameters.
                    // The last one is not actually an arrow function,
                    // but this is probably what the user intended.
                    auto third = nextToken();
                    switch (third) {
                        case SyntaxKind::EqualsGreaterThanToken:
                        case SyntaxKind::ColonToken:
                        case SyntaxKind::OpenBraceToken:
                            return Tristate::True;
                        default:
                            return Tristate::False;
                    }
                }

                // If encounter "([" or "({", this could be the start of a binding pattern.
                // Examples:
                //      ([ x ]) => { }
                //      ({ x }) => { }
                //      ([ x ])
                //      ({ x })
                if (second == SyntaxKind::OpenBracketToken || second == SyntaxKind::OpenBraceToken) {
                    return Tristate::Unknown;
                }

                // Simple case: "(..."
                // This is an arrow function with a rest parameter.
                if (second == SyntaxKind::DotDotDotToken) {
                    return Tristate::True;
                }

                // Check for "(xxx yyy", where xxx is a modifier and yyy is an identifier. This
                // isn't actually allowed, but we want to treat it as a lambda so we can provide
                // a good error message.
                if (isModifierKind(second) && second != SyntaxKind::AsyncKeyword && lookAhead<bool>(CALLBACK(nextTokenIsIdentifier))) {
                    if (nextToken() == SyntaxKind::AsKeyword) {
                        // https://github.com/microsoft/TypeScript/issues/44466
                        return Tristate::False;
                    }
                    return Tristate::True;
                }

                // If we had "(" followed by something that's not an identifier,
                // then this definitely doesn't look like a lambda.  "this" is not
                // valid, but we want to parse it and then give a semantic error.
                if (!isIdentifier() && second != SyntaxKind::ThisKeyword) {
                    return Tristate::False;
                }

                switch (nextToken()) {
                    case SyntaxKind::ColonToken:
                        // If we have something like "(a:", then we must have a
                        // type-annotated parameter in an arrow function expression.
                        return Tristate::True;
                    case SyntaxKind::QuestionToken:
                        nextToken();
                        // If we have "(a?:" or "(a?," or "(a?=" or "(a?)" then it is definitely a lambda.
                        if (token() == SyntaxKind::ColonToken || token() == SyntaxKind::CommaToken || token() == SyntaxKind::EqualsToken || token() == SyntaxKind::CloseParenToken) {
                            return Tristate::True;
                        }
                        // Otherwise it is definitely not a lambda.
                        return Tristate::False;
                    case SyntaxKind::CommaToken:
                    case SyntaxKind::EqualsToken:
                    case SyntaxKind::CloseParenToken:
                        // If we have "(a," or "(a=" or "(a)" this *could* be an arrow function
                        return Tristate::Unknown;
                }
                // It is definitely not an arrow function
                return Tristate::False;
            } else {
                assert(first == SyntaxKind::LessThanToken);

                // If we have "<" not followed by an identifier,
                // then this definitely is not an arrow function.
                if (!isIdentifier()) {
                    return Tristate::False;
                }

                // JSX overrides
                if (languageVariant == LanguageVariant::JSX) {
                    auto isArrowFunctionInJsx = lookAhead<bool>([this]() {
                        auto third = nextToken();
                        if (third == SyntaxKind::ExtendsKeyword) {
                            auto fourth = nextToken();
                            switch (fourth) {
                                case SyntaxKind::EqualsToken:
                                case SyntaxKind::GreaterThanToken:
                                    return false;
                                default:
                                    return true;
                            }
                        } else if (third == SyntaxKind::CommaToken || third == SyntaxKind::EqualsToken) {
                            return true;
                        }
                        return false;
                    });

                    if (isArrowFunctionInJsx) {
                        return Tristate::True;
                    }

                    return Tristate::False;
                }

                // This *could* be a parenthesized arrow function.
                return Tristate::Unknown;
            }
        }

        //  True        -> We definitely expect a parenthesized arrow function here.
        //  False       -> There *cannot* be a parenthesized arrow function here.
        //  Unknown     -> There *might* be a parenthesized arrow function here.
        //                 Speculatively look ahead to be sure, and rollback if not.
        Tristate isParenthesizedArrowFunctionExpression() {
            if (token() == SyntaxKind::OpenParenToken || token() == SyntaxKind::LessThanToken || token() == SyntaxKind::AsyncKeyword) {
                return lookAhead<Tristate>(CALLBACK(isParenthesizedArrowFunctionExpressionWorker));
            }

            if (token() == SyntaxKind::EqualsGreaterThanToken) {
                // ERROR RECOVERY TWEAK:
                // If we see a standalone => try to parse it as an arrow function expression as that's
                // likely what the user intended to write.
                return Tristate::True;
            }
            // Definitely not a parenthesized arrow function.
            return Tristate::False;
        }

        sharedOpt<NodeArray> parseModifiersForArrowFunction() {
            if (token() == SyntaxKind::AsyncKeyword) {
                auto pos = getNodePos();
                nextToken();
                auto modifier = finishNode(factory.createToken<AsyncKeyword>(SyntaxKind::AsyncKeyword), pos);
                auto list = make_shared<NodeArray>();
                list->push(modifier);
                return factory.createNodeArray(list, pos);
            }
            return nullptr;
        }

        shared<Node> parseArrowFunctionExpressionBody(bool isAsync) {
            if (token() == SyntaxKind::OpenBraceToken) {
                return parseFunctionBlock(isAsync ? (int) SignatureFlags::Await : (int) SignatureFlags::None);
            }

            if (token() != SyntaxKind::SemicolonToken &&
                token() != SyntaxKind::FunctionKeyword &&
                token() != SyntaxKind::ClassKeyword &&
                isStartOfStatement() &&
                !isStartOfExpressionStatement()) {
                // Check if we got a plain statement (i.e. no expression-statements, no function/class expressions/declarations)
                //
                // Here we try to recover from a potential error situation in the case where the
                // user meant to supply a block. For example, if the user wrote:
                //
                //  a =>
                //      auto v = 0;
                //  }
                //
                // they may be missing an open brace.  Check to see if that's the case so we can
                // try to recover better.  If we don't do this, then the next close curly we see may end
                // up preemptively closing the containing construct.
                //
                // Note: even when 'IgnoreMissingOpenBrace' is passed, parseBody will still error.
                return parseFunctionBlock((int) SignatureFlags::IgnoreMissingOpenBrace | (isAsync ? (int) SignatureFlags::Await : (int) SignatureFlags::None));
            }

            auto savedTopLevel = topLevel;
            topLevel = false;
            auto node = isAsync
                        ? doInAwaitContext<shared<Expression>>(CALLBACK(parseAssignmentExpressionOrHigher))
                        : doOutsideOfAwaitContext<shared<Expression>>(CALLBACK(parseAssignmentExpressionOrHigher));
            topLevel = savedTopLevel;
            return node;
        }

        sharedOpt<ArrowFunction> parseParenthesizedArrowFunctionExpression(bool allowAmbiguity) {
            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();
            auto modifiers = parseModifiersForArrowFunction();
            auto isAsync = some(modifiers, isAsyncModifier) ? SignatureFlags::Await : SignatureFlags::None;
            // Arrow functions are never generators.
            //
            // If we're speculatively parsing a signature for a parenthesized arrow function, then
            // we have to have a complete parameter list.  Otherwise we might see something like
            // a => (b => c)
            // And think that "(b =>" was actually a parenthesized arrow function with a missing
            // close paren.
            auto typeParameters = parseTypeParameters();

            shared<NodeArray> parameters;
            if (!parseExpected(SyntaxKind::OpenParenToken)) {
                if (!allowAmbiguity) {
                    return nullptr;
                }
                parameters = createMissingList();
            } else {
                if (!allowAmbiguity) {
                    auto maybeParameters = parseParametersWorker((int) isAsync, allowAmbiguity);
                    if (!maybeParameters) {
                        return nullptr;
                    }
                    parameters = maybeParameters;
                } else {
                    parameters = parseParametersWorker((int) isAsync, allowAmbiguity);
                }
                if (!parseExpected(SyntaxKind::CloseParenToken) && !allowAmbiguity) {
                    return nullptr;
                }
            }

            auto type = parseReturnType(SyntaxKind::ColonToken, /*isType*/ false);
            if (type && !allowAmbiguity && typeHasArrowFunctionBlockingParseError(type)) {
                return nullptr;
            }

            // Parsing a signature isn't enough.
            // Parenthesized arrow signatures often look like other valid expressions.
            // For instance:
            //  - "(x = 10)" is an assignment expression parsed as a signature with a default parameter value.
            //  - "(x,y)" is a comma expression parsed as a signature with two parameters.
            //  - "a ? (b): c" will have "(b):" parsed as a signature with a return type annotation.
            //  - "a ? (b): function() {}" will too, since function() is a valid JSDoc function type.
            //  - "a ? (b): (function() {})" as well, but inside of a parenthesized type with an arbitrary amount of nesting.
            //
            // So we need just a bit of lookahead to ensure that it can only be a signature.

            auto unwrappedType = type;
            while (unwrappedType && unwrappedType->kind == SyntaxKind::ParenthesizedType) {
                unwrappedType = unwrappedType->to<ParenthesizedTypeNode>().type;  // Skip parens if need be
            }

            auto hasJSDocFunctionType = unwrappedType && isJSDocFunctionType(unwrappedType);
            if (!allowAmbiguity && token() != SyntaxKind::EqualsGreaterThanToken && (hasJSDocFunctionType || token() != SyntaxKind::OpenBraceToken)) {
                // Returning undefined here will cause our caller to rewind to where we started from.
                return nullptr;
            }

            // If we have an arrow, then try to parse the body. Even if not, try to parse if we
            // have an opening brace, just in case we're in an error state.
            auto lastToken = token();
            auto equalsGreaterThanToken = parseExpectedToken<EqualsGreaterThanToken>(SyntaxKind::EqualsGreaterThanToken);
            auto body = (lastToken == SyntaxKind::EqualsGreaterThanToken || lastToken == SyntaxKind::OpenBraceToken)
                        ? parseArrowFunctionExpressionBody(some(modifiers, isAsyncModifier))
                        : parseIdentifier();

            auto node = factory.createArrowFunction(modifiers, typeParameters, parameters, type, equalsGreaterThanToken, body);
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        sharedOpt<ArrowFunction> parsePossibleParenthesizedArrowFunctionExpression() {
            auto tokenPos = scanner.getTokenPos();
            if (has(notParenthesizedArrow, tokenPos)) {
                return nullptr;
            }

            auto result = parseParenthesizedArrowFunctionExpression(/*allowAmbiguity*/ false);
            if (!result) {
                notParenthesizedArrow.insert(tokenPos);
            }

            return result;
        }

        sharedOpt<Expression> tryParseParenthesizedArrowFunctionExpression() {
            ZoneScoped;
            auto triState = isParenthesizedArrowFunctionExpression();
            if (triState == Tristate::False) {
                // It's definitely not a parenthesized arrow function expression.
                return nullptr;
            }

            // If we definitely have an arrow function, then we can just parse one, not requiring a
            // following => or { token. Otherwise, we *might* have an arrow function.  Try to parse
            // it out, but don't allow any ambiguity, and return 'undefined' if this could be an
            // expression instead.
            return triState == Tristate::True ?
                   parseParenthesizedArrowFunctionExpression(/*allowAmbiguity*/ true) :
                   tryParse<sharedOpt<ArrowFunction>>(CALLBACK(parsePossibleParenthesizedArrowFunctionExpression));
        }

        Tristate isUnParenthesizedAsyncArrowFunctionWorker() {
            // AsyncArrowFunctionExpression:
            //      1) async[no LineTerminator here]AsyncArrowBindingIdentifier[?Yield][no LineTerminator here]=>AsyncConciseBody[?In]
            //      2) CoverCallExpressionAndAsyncArrowHead[?Yield, ?Await][no LineTerminator here]=>AsyncConciseBody[?In]
            if (token() == SyntaxKind::AsyncKeyword) {
                nextToken();
                // If the "async" is followed by "=>" token then it is not a beginning of an async arrow-function
                // but instead a simple arrow-function which will be parsed inside "parseAssignmentExpressionOrHigher"
                if (scanner.hasPrecedingLineBreak() || token() == SyntaxKind::EqualsGreaterThanToken) {
                    return Tristate::False;
                }
                // Check for un-parenthesized AsyncArrowFunction
                auto expr = parseBinaryExpressionOrHigher((int) OperatorPrecedence::Lowest);
                if (!scanner.hasPrecedingLineBreak() && expr->kind == SyntaxKind::Identifier && token() == SyntaxKind::EqualsGreaterThanToken) {
                    return Tristate::True;
                }
            }

            return Tristate::False;
        }

        shared<ArrowFunction> parseSimpleArrowFunctionExpression(int pos, shared<Identifier> identifier, sharedOpt<NodeArray> asyncModifier) {
            ZoneScoped;
            Debug::asserts(token() == SyntaxKind::EqualsGreaterThanToken, "parseSimpleArrowFunctionExpression should only have been called if we had a =>");
            auto parameter = factory.createParameterDeclaration(
                    /*decorators*/ {},
                    /*modifiers*/ {},
                    /*dotDotDotToken*/ {},
                                   identifier,
                    /*questionToken*/ {},
                    /*type*/ {},
                    /*initializer*/ {}
            );
            finishNode(parameter, identifier->pos);

            auto parameters = createNodeArray(make_shared<NodeArray>(parameter), parameter->pos, parameter->end);

            auto equalsGreaterThanToken = parseExpectedToken<EqualsGreaterThanToken>(SyntaxKind::EqualsGreaterThanToken);
            auto body = parseArrowFunctionExpressionBody(/*isAsync*/ !!asyncModifier);
            auto node = factory.createArrowFunction(asyncModifier, /*typeParameters*/ {}, parameters, /*type*/ {}, equalsGreaterThanToken, body);
            return addJSDocComment(finishNode(node, pos));
        }

        sharedOpt<ArrowFunction> tryParseAsyncSimpleArrowFunctionExpression() {
            ZoneScoped;
            // We do a check here so that we won't be doing unnecessarily call to "lookAhead"
            if (token() == SyntaxKind::AsyncKeyword) {
                if (lookAhead<Tristate>(CALLBACK(isUnParenthesizedAsyncArrowFunctionWorker)) == Tristate::True) {
                    auto pos = getNodePos();
                    auto asyncModifier = parseModifiersForArrowFunction();
                    auto expr = parseBinaryExpressionOrHigher((int) OperatorPrecedence::Lowest);
                    return parseSimpleArrowFunctionExpression(pos, to<Identifier>(expr), asyncModifier);
                }
            }
            return nullptr;
        }

        shared<Expression> parseConditionalExpressionRest(shared<Expression> leftOperand, int pos) {
            ZoneScoped;
            // Note: we are passed in an expression which was produced from parseBinaryExpressionOrHigher.
            auto questionToken = parseOptionalToken<QuestionToken>(SyntaxKind::QuestionToken);
            if (!questionToken) {
                return leftOperand;
            }

            // Note: we explicitly 'allowIn' in the whenTrue part of the condition expression, and
            // we do not that for the 'whenFalse' part.
            sharedOpt<ColonToken> colonToken;
            return finishNode(
                    factory.createConditionalExpression(
                            leftOperand,
                            questionToken,
                            doOutsideOfContext<shared<Expression>>(disallowInAndDecoratorContext, CALLBACK(parseAssignmentExpressionOrHigher)),
                            colonToken = parseExpectedToken<ColonToken>(SyntaxKind::ColonToken),
                            nodeIsPresent(colonToken)
                            ? parseAssignmentExpressionOrHigher()
                            : createMissingNode<Identifier>(SyntaxKind::Identifier, /*reportAtCurrentPosition*/ false, Diagnostics::_0_expected, tokenToString(SyntaxKind::ColonToken))
                    ),
                    pos
            );
        }

        shared<Expression> parseAssignmentExpressionOrHigher() {
            ZoneScoped;
            //  AssignmentExpression[in,yield]:
            //      1) ConditionalExpression[?in,?yield]
            //      2) LeftHandSideExpression = AssignmentExpression[?in,?yield]
            //      3) LeftHandSideExpression AssignmentOperator AssignmentExpression[?in,?yield]
            //      4) ArrowFunctionExpression[?in,?yield]
            //      5) AsyncArrowFunctionExpression[in,yield,await]
            //      6) [+Yield] YieldExpression[?In]
            //
            // Note: for ease of implementation we treat productions '2' and '3' as the same thing.
            // (i.e. they're both BinaryExpressions with an assignment operator in it).

            // First, do the simple check if we have a YieldExpression (production '6').
            if (isYieldExpression()) {
                return parseYieldExpression();
            }

            // Then, check if we have an arrow function (production '4' and '5') that starts with a parenthesized
            // parameter list or is an async arrow function.
            // AsyncArrowFunctionExpression:
            //      1) async[no LineTerminator here]AsyncArrowBindingIdentifier[?Yield][no LineTerminator here]=>AsyncConciseBody[?In]
            //      2) CoverCallExpressionAndAsyncArrowHead[?Yield, ?Await][no LineTerminator here]=>AsyncConciseBody[?In]
            // Production (1) of AsyncArrowFunctionExpression is parsed in "tryParseAsyncSimpleArrowFunctionExpression".
            // And production (2) is parsed in "tryParseParenthesizedArrowFunctionExpression".
            //
            // If we do successfully parse arrow-function, we must *not* recurse for productions 1, 2 or 3. An ArrowFunction is
            // not a LeftHandSideExpression, nor does it start a ConditionalExpression.  So we are done
            // with AssignmentExpression if we see one.
            if (auto a = tryParseParenthesizedArrowFunctionExpression()) return a;
            if (auto a = tryParseAsyncSimpleArrowFunctionExpression()) return a;

            // Now try to see if we're in production '1', '2' or '3'.  A conditional expression can
            // start with a LogicalOrExpression, while the assignment productions can only start with
            // LeftHandSideExpressions.
            //
            // So, first, we try to just parse out a BinaryExpression.  If we get something that is a
            // LeftHandSide or higher, then we can try to parse out the assignment expression part.
            // Otherwise, we try to parse out the conditional expression bit.  We want to allow any
            // binary expression here, so we pass in the 'lowest' precedence here so that it matches
            // and consumes anything.
            const auto pos = getNodePos();
            const auto expr = parseBinaryExpressionOrHigher((int) OperatorPrecedence::Lowest);

            // To avoid a look-ahead, we did not handle the case of an arrow function with a single un-parenthesized
            // parameter ('x => ...') above. We handle it here by checking if the parsed expression was a single
            // identifier and the current token is an arrow.
            if (expr->kind == SyntaxKind::Identifier && token() == SyntaxKind::EqualsGreaterThanToken) {
                return parseSimpleArrowFunctionExpression(pos, to<Identifier>(expr), /*asyncModifier*/ {});
            }

            // Now see if we might be in cases '2' or '3'.
            // If the expression was a LHS expression, and we have an assignment operator, then
            // we're in '2' or '3'. Consume the assignment and return.
            //
            // Note: we call reScanGreaterToken so that we get an appropriately merged token
            // for cases like `> > =` becoming `>>=`
            if (isLeftHandSideExpression(expr) && isAssignmentOperator(reScanGreaterToken())) {
                return makeBinaryExpression(expr, parseTokenNode<Expression>(), parseAssignmentExpressionOrHigher(), pos);
            }

            // It wasn't an assignment or a lambda.  This is a conditional expression:
            return parseConditionalExpressionRest(expr, pos);
        }

        shared<Statement> parseEmptyStatement() {
            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();
            parseExpected(SyntaxKind::SemicolonToken);
            return withJSDoc(finishNode(factory.createEmptyStatement(), pos), hasJSDoc);
        }

//        function parseIfStatement(): IfStatement {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            parseExpected(SyntaxKind::IfKeyword);
//            auto openParenPosition = scanner.getTokenPos();
//            auto openParenParsed = parseExpected(SyntaxKind::OpenParenToken);
//            auto expression = allowInAnd<shared<Expression>>(parseExpression);
//            parseExpectedMatchingBrackets(SyntaxKind::OpenParenToken, SyntaxKind::CloseParenToken, openParenParsed, openParenPosition);
//            auto thenStatement = parseStatement();
//            auto elseStatement = parseOptional(SyntaxKind::ElseKeyword) ? parseStatement() : undefined;
//            return withJSDoc(finishNode(factory.createIfStatement(expression, thenStatement, elseStatement), pos), hasJSDoc);
//        }
//
//        function parseDoStatement(): DoStatement {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            parseExpected(SyntaxKind::DoKeyword);
//            auto statement = parseStatement();
//            parseExpected(SyntaxKind::WhileKeyword);
//            auto openParenPosition = scanner.getTokenPos();
//            auto openParenParsed = parseExpected(SyntaxKind::OpenParenToken);
//            auto expression = allowInAnd<shared<Expression>>(parseExpression);
//            parseExpectedMatchingBrackets(SyntaxKind::OpenParenToken, SyntaxKind::CloseParenToken, openParenParsed, openParenPosition);
//
//            // From: https://mail.mozilla.org/pipermail/es-discuss/2011-August/016188.html
//            // 157 min --- All allen at wirfs-brock.com CONF --- "do{;}while(false)false" prohibited in
//            // spec but allowed in consensus reality. Approved -- this is the de-facto standard whereby
//            //  do;while(0)x will have a semicolon inserted before x.
//            parseOptional(SyntaxKind::SemicolonToken);
//            return withJSDoc(finishNode(factory.createDoStatement(statement, expression), pos), hasJSDoc);
//        }
//
//        function parseWhileStatement(): WhileStatement {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            parseExpected(SyntaxKind::WhileKeyword);
//            auto openParenPosition = scanner.getTokenPos();
//            auto openParenParsed = parseExpected(SyntaxKind::OpenParenToken);
//            auto expression = allowInAnd<shared<Expression>>(parseExpression);
//            parseExpectedMatchingBrackets(SyntaxKind::OpenParenToken, SyntaxKind::CloseParenToken, openParenParsed, openParenPosition);
//            auto statement = parseStatement();
//            return withJSDoc(finishNode(factory.createWhileStatement(expression, statement), pos), hasJSDoc);
//        }
//
//        function parseForOrForInOrForOfStatement(): Statement {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            parseExpected(SyntaxKind::ForKeyword);
//            auto awaitToken = parseOptionalToken<AwaitKeyword>(SyntaxKind::AwaitKeyword);
//            parseExpected(SyntaxKind::OpenParenToken);
//
//            auto initializer!: VariableDeclarationList | Expression;
//            if (token() != SyntaxKind::SemicolonToken) {
//                if (token() == SyntaxKind::VarKeyword || token() == SyntaxKind::LetKeyword || token() == SyntaxKind::ConstKeyword) {
//                    initializer = parseVariableDeclarationList(/*inForStatementInitializer*/ true);
//                }
//                else {
//                    initializer = disallowInAnd<shared<Expression>>(parseExpression);
//                }
//            }
//
//            auto node: IterationStatement;
//            if (awaitToken ? parseExpected(SyntaxKind::OfKeyword) : parseOptional(SyntaxKind::OfKeyword)) {
//                auto expression = allowInAnd(parseAssignmentExpressionOrHigher);
//                parseExpected(SyntaxKind::CloseParenToken);
//                node = factory.createForOfStatement(awaitToken, initializer, expression, parseStatement());
//            }
//            else if (parseOptional(SyntaxKind::InKeyword)) {
//                auto expression = allowInAnd<shared<Expression>>(parseExpression);
//                parseExpected(SyntaxKind::CloseParenToken);
//                node = factory.createForInStatement(initializer, expression, parseStatement());
//            }
//            else {
//                parseExpected(SyntaxKind::SemicolonToken);
//                auto condition = token() != SyntaxKind::SemicolonToken && token() != SyntaxKind::CloseParenToken
//                    ? allowInAnd<shared<Expression>>(parseExpression)
//                    : undefined;
//                parseExpected(SyntaxKind::SemicolonToken);
//                auto incrementor = token() != SyntaxKind::CloseParenToken
//                    ? allowInAnd<shared<Expression>>(parseExpression)
//                    : undefined;
//                parseExpected(SyntaxKind::CloseParenToken);
//                node = factory.createForStatement(initializer, condition, incrementor, parseStatement());
//            }
//
//            return withJSDoc(finishNode(node, pos) as ForStatement | ForInOrOfStatement, hasJSDoc);
//        }
//
//        function parseBreakOrContinueStatement(kind: SyntaxKind): BreakOrContinueStatement {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//
//            parseExpected(kind == SyntaxKind::BreakStatement ? SyntaxKind::BreakKeyword : SyntaxKind::ContinueKeyword);
//            auto label = canParseSemicolon() ? undefined : parseIdentifier();
//
//            parseSemicolon();
//            auto node = kind == SyntaxKind::BreakStatement
//                ? factory.createBreakStatement(label)
//                : factory.createContinueStatement(label);
//            return withJSDoc(finishNode(node, pos), hasJSDoc);
//        }
//
//        function parseReturnStatement(): ReturnStatement {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            parseExpected(SyntaxKind::ReturnKeyword);
//            auto expression = canParseSemicolon() ? undefined : allowInAnd<shared<Expression>>(parseExpression);
//            parseSemicolon();
//            return withJSDoc(finishNode(factory.createReturnStatement(expression), pos), hasJSDoc);
//        }
//
//        function parseWithStatement(): WithStatement {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            parseExpected(SyntaxKind::WithKeyword);
//            auto openParenPosition = scanner.getTokenPos();
//            auto openParenParsed = parseExpected(SyntaxKind::OpenParenToken);
//            auto expression = allowInAnd<shared<Expression>>(parseExpression);
//            parseExpectedMatchingBrackets(SyntaxKind::OpenParenToken, SyntaxKind::CloseParenToken, openParenParsed, openParenPosition);
//            auto statement = doInsideOfContext(NodeFlags::InWithStatement, parseStatement);
//            return withJSDoc(finishNode(factory.createWithStatement(expression, statement), pos), hasJSDoc);
//        }
//
//        function parseCaseClause(): CaseClause {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            parseExpected(SyntaxKind::CaseKeyword);
//            auto expression = allowInAnd<shared<Expression>>(parseExpression);
//            parseExpected(SyntaxKind::ColonToken);
//            auto statements = parseList(ParsingContext::SwitchClauseStatements, parseStatement);
//            return withJSDoc(finishNode(factory.createCaseClause(expression, statements), pos), hasJSDoc);
//        }
//
//        function parseDefaultClause(): DefaultClause {
//            auto pos = getNodePos();
//            parseExpected(SyntaxKind::DefaultKeyword);
//            parseExpected(SyntaxKind::ColonToken);
//            auto statements = parseList(ParsingContext::SwitchClauseStatements, parseStatement);
//            return finishNode(factory.createDefaultClause(statements), pos);
//        }
//
//        function parseCaseOrDefaultClause(): CaseOrDefaultClause {
//            return token() == SyntaxKind::CaseKeyword ? parseCaseClause() : parseDefaultClause();
//        }
//
//        function parseCaseBlock(): CaseBlock {
//            auto pos = getNodePos();
//            parseExpected(SyntaxKind::OpenBraceToken);
//            auto clauses = parseList(ParsingContext::SwitchClauses, parseCaseOrDefaultClause);
//            parseExpected(SyntaxKind::CloseBraceToken);
//            return finishNode(factory.createCaseBlock(clauses), pos);
//        }
//
//        function parseSwitchStatement(): SwitchStatement {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            parseExpected(SyntaxKind::SwitchKeyword);
//            parseExpected(SyntaxKind::OpenParenToken);
//            auto expression = allowInAnd<shared<Expression>>(parseExpression);
//            parseExpected(SyntaxKind::CloseParenToken);
//            auto caseBlock = parseCaseBlock();
//            return withJSDoc(finishNode(factory.createSwitchStatement(expression, caseBlock), pos), hasJSDoc);
//        }
//
//        function parseThrowStatement(): ThrowStatement {
//            // ThrowStatement[Yield] :
//            //      throw [no LineTerminator here]Expression[In, ?Yield];
//
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            parseExpected(SyntaxKind::ThrowKeyword);
//
//            // Because of automatic semicolon insertion, we need to report error if this
//            // throw could be terminated with a semicolon.  Note: we can't call 'parseExpression'
//            // directly as that might consume an expression on the following line.
//            // Instead, we create a "missing" identifier, but don't report an error. The actual error
//            // will be reported in the grammar walker.
//            auto expression = scanner.hasPrecedingLineBreak() ? undefined : allowInAnd<shared<Expression>>(parseExpression);
//            if (expression == undefined) {
//                identifierCount++;
//                expression = finishNode(factory.createIdentifier(""), getNodePos());
//            }
//            if (!tryParseSemicolon()) {
//                parseErrorForMissingSemicolonAfter(expression);
//            }
//            return withJSDoc(finishNode(factory.createThrowStatement(expression), pos), hasJSDoc);
//        }
//
//        // TODO: Review for error recovery
//        function parseTryStatement(): TryStatement {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//
//            parseExpected(SyntaxKind::TryKeyword);
//            auto tryBlock = parseBlock(/*ignoreMissingOpenBrace*/ false);
//            auto catchClause = token() == SyntaxKind::CatchKeyword ? parseCatchClause() : undefined;
//
//            // If we don't have a catch clause, then we must have a finally clause.  Try to parse
//            // one out no matter what.
//            auto finallyBlock: Block | undefined;
//            if (!catchClause || token() == SyntaxKind::FinallyKeyword) {
//                parseExpected(SyntaxKind::FinallyKeyword, Diagnostics::catch_or_finally_expected);
//                finallyBlock = parseBlock(/*ignoreMissingOpenBrace*/ false);
//            }
//
//            return withJSDoc(finishNode(factory.createTryStatement(tryBlock, catchClause, finallyBlock), pos), hasJSDoc);
//        }
//
//        function parseCatchClause(): CatchClause {
//            auto pos = getNodePos();
//            parseExpected(SyntaxKind::CatchKeyword);
//
//            auto variableDeclaration;
//            if (parseOptional(SyntaxKind::OpenParenToken)) {
//                variableDeclaration = parseVariableDeclaration();
//                parseExpected(SyntaxKind::CloseParenToken);
//            }
//            else {
//                // Keep shape of node to avoid degrading performance.
//                variableDeclaration = undefined;
//            }
//
//            auto block = parseBlock(/*ignoreMissingOpenBrace*/ false);
//            return finishNode(factory.createCatchClause(variableDeclaration, block), pos);
//        }
//
//        function parseDebuggerStatement(): Statement {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            parseExpected(SyntaxKind::DebuggerKeyword);
//            parseSemicolon();
//            return withJSDoc(finishNode(factory.createDebuggerStatement(), pos), hasJSDoc);
//        }

        shared<NodeUnion(ExpressionStatement, LabeledStatement)> parseExpressionOrLabeledStatement() {
            // Avoiding having to do the lookahead for a labeled statement by just trying to parse
            // out an expression, seeing if it is identifier and then seeing if it is followed by
            // a colon.
            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();
            shared<NodeUnion(ExpressionStatement, LabeledStatement)> node;
            auto hasParen = token() == SyntaxKind::OpenParenToken;
            auto expression = allowInAnd<shared<Expression>>(CALLBACK(parseExpression));
            if (ts::isIdentifier(expression) && parseOptional(SyntaxKind::ColonToken)) {
                node = factory.createLabeledStatement(expression, parseStatement());
            } else {
                if (!tryParseSemicolon()) {
                    parseErrorForMissingSemicolonAfter(expression);
                }
                node = factory.createExpressionStatement(expression);
                if (hasParen) {
                    // do not parse the same jsdoc twice
                    hasJSDoc = false;
                }
            }
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        bool canFollowContextualOfKeyword() {
            return nextTokenIsIdentifier() && nextToken() == SyntaxKind::CloseParenToken;
        }

        shared<VariableDeclaration> parseVariableDeclaration(optional<bool> allowExclamation = {}) {
            ZoneScoped;
            const auto pos = getNodePos();
            const auto hasJSDoc = hasPrecedingJSDocComment();
            const auto name = parseIdentifierOrPattern(Diagnostics::Private_identifiers_are_not_allowed_in_variable_declarations);
            sharedOpt<ExclamationToken> exclamationToken;
            if (isTrue(allowExclamation) && name->kind == SyntaxKind::Identifier &&
                token() == SyntaxKind::ExclamationToken && !scanner.hasPrecedingLineBreak()) {
                exclamationToken = parseTokenNode<ExclamationToken>();
            }
            const auto type = parseTypeAnnotation();
            sharedOpt<Expression> initializer = isInOrOfKeyword(token()) ? nullptr : parseInitializer();
            const auto node = factory.createVariableDeclaration(name, exclamationToken, type, initializer);
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        auto parseVariableDeclarationAllowExclamation() {
            return parseVariableDeclaration(/*allowExclamation*/ true);
        }

        shared<VariableDeclarationList> parseVariableDeclarationList(bool inForStatementInitializer) {
            ZoneScoped;
            auto pos = getNodePos();

            int flags = 0;
            switch (token()) {
                case SyntaxKind::VarKeyword:
                    break;
                case SyntaxKind::LetKeyword:
                    flags |= (int) NodeFlags::Let;
                    break;
                case SyntaxKind::ConstKeyword:
                    flags |= (int) NodeFlags::Const;
                    break;
                default:
                    Debug::fail();
            }

            nextToken();

            // The user may have written the following:
            //
            //    for (let of X) { }
            //
            // In this case, we want to parse an empty declaration list, and then parse 'of'
            // as a keyword. The reason this is not automatic is that 'of' is a valid identifier.
            // So we need to look ahead to determine if 'of' should be treated as a keyword in
            // this context.
            // The checker will then give an error that there is an empty declaration list.
            shared<NodeArray> declarations;
            if (token() == SyntaxKind::OfKeyword && lookAhead<bool>(CALLBACK(canFollowContextualOfKeyword))) {
                declarations = createMissingList();
            } else {
                auto savedDisallowIn = inDisallowInContext();
                setDisallowInContext(inForStatementInitializer);

                if (inForStatementInitializer) {
                    declarations = parseDelimitedList(ParsingContext::VariableDeclarations, [this]() { return parseVariableDeclaration(); });
                } else {
                    declarations = parseDelimitedList(ParsingContext::VariableDeclarations, CALLBACK(parseVariableDeclarationAllowExclamation));
                }

                setDisallowInContext(savedDisallowIn);
            }

            return finishNode(factory.createVariableDeclarationList(declarations, flags), pos);
        }

        shared<VariableStatement> parseVariableStatement(int pos, bool hasJSDoc, const sharedOpt<NodeArray> &decorators, const sharedOpt<NodeArray> &modifiers) {
            ZoneScoped;
            auto declarationList = parseVariableDeclarationList(/*inForStatementInitializer*/ false);
            parseSemicolon();
            auto node = factory.createVariableStatement(modifiers, declarationList);
            // Decorators are not allowed on a variable statement, so we keep track of them to report them in the grammar checker.
            node->decorators = decorators;
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        bool nextTokenIsBindingIdentifierOrStartOfDestructuring() {
            nextToken();
            return isBindingIdentifier() || token() == SyntaxKind::OpenBraceToken || token() == SyntaxKind::OpenBracketToken;
        }

        bool isLetDeclaration() {
            // In ES6 'let' always starts a lexical declaration if followed by an identifier or {
            // or [.
            return lookAhead<bool>(CALLBACK(nextTokenIsBindingIdentifierOrStartOfDestructuring));
        }

        shared<FunctionDeclaration> parseFunctionDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray> decorators, sharedOpt<NodeArray> modifiers) {
            auto savedAwaitContext = inAwaitContext();

            auto modifierFlags = modifiersToFlags(modifiers);
            parseExpected(SyntaxKind::FunctionKeyword);
            auto asteriskToken = parseOptionalToken<AsteriskToken>(SyntaxKind::AsteriskToken);
            // We don't parse the name here in await context, instead we will report a grammar error in the checker.
            auto name = modifierFlags & (int) ModifierFlags::Default ? parseOptionalBindingIdentifier() : parseBindingIdentifier();
            auto isGenerator = asteriskToken ? SignatureFlags::Yield : SignatureFlags::None;
            auto isAsync = modifierFlags & (int) ModifierFlags::Async ? SignatureFlags::Await : SignatureFlags::None;
            auto typeParameters = parseTypeParameters();
            if (modifierFlags & (int) ModifierFlags::Export) setAwaitContext(/*value*/ true);
            auto parameters = parseParameters((int) isGenerator | (int) isAsync);
            auto type = parseReturnType(SyntaxKind::ColonToken, /*isType*/ false);
            auto body = parseFunctionBlockOrSemicolon((int) isGenerator | (int) isAsync, Diagnostics::or_expected);
            setAwaitContext(savedAwaitContext);
            auto node = factory.createFunctionDeclaration(decorators, modifiers, asteriskToken, name, typeParameters, parameters, type, body);
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

        shared<Statement> parseDeclaration() {
            ZoneScoped;
            // TODO: Can we hold onto the parsed decorators/modifiers and advance the scanner
            //       if we can't reuse the declaration, so that we don't do this work twice?
            //
            // `parseListElement` attempted to get the reused node at this position,
            // but the ambient context flag was not yet set, so the node appeared
            // not reusable in that context.
            auto a = lookAhead<sharedOpt<NodeArray>>([this]() {
                parseDecorators();
                return parseModifiers();
            });
            auto isAmbient = some(a, CALLBACK(isDeclareModifier));
            if (isAmbient) {
                auto node = tryReuseAmbientDeclaration();
                if (node) {
                    return node;
                }
            }

            auto pos = getNodePos();
            auto hasJSDoc = hasPrecedingJSDocComment();
            auto decorators = parseDecorators();
            auto modifiers = parseModifiers();
            if (isAmbient) {
                for (auto &&m: modifiers->list) {
                    m->flags |= (int) NodeFlags::Ambient;
                }
                return doInsideOfContext<shared<Statement>>((int) NodeFlags::Ambient, [this, &pos, &hasJSDoc, &decorators, &modifiers]() { return parseDeclarationWorker(pos, hasJSDoc, decorators, modifiers); });
            } else {
                return parseDeclarationWorker(pos, hasJSDoc, decorators, modifiers);
            }
        }

        sharedOpt<Statement> tryReuseAmbientDeclaration() {
            ZoneScoped;
            return doInsideOfContext<sharedOpt<Statement>>((int) NodeFlags::Ambient, [this]()->sharedOpt<Statement> {
                auto node = currentNode((ParsingContext) parsingContext);
                if (node) {
                    return to<Statement>(consumeNode(node));
                }
                return nullptr;
            });
        }

        shared<Statement> parseDeclarationWorker(int pos, bool hasJSDoc, sharedOpt<NodeArray> decorators, sharedOpt<NodeArray> modifiers) {
            ZoneScoped;
            switch (token()) {
                case SyntaxKind::VarKeyword:
                case SyntaxKind::LetKeyword:
                case SyntaxKind::ConstKeyword:
                    return parseVariableStatement(pos, hasJSDoc, decorators, modifiers);
                case SyntaxKind::FunctionKeyword:
                    return parseFunctionDeclaration(pos, hasJSDoc, decorators, modifiers);
//                case SyntaxKind::ClassKeyword:
//                    return parseClassDeclaration(pos, hasJSDoc, decorators, modifiers);
//                case SyntaxKind::InterfaceKeyword:
//                    return parseInterfaceDeclaration(pos, hasJSDoc, decorators, modifiers);
                case SyntaxKind::TypeKeyword:
                    return parseTypeAliasDeclaration(pos, hasJSDoc, decorators, modifiers);
//                case SyntaxKind::EnumKeyword:
//                    return parseEnumDeclaration(pos, hasJSDoc, decorators, modifiers);
//                case SyntaxKind::GlobalKeyword:
//                case SyntaxKind::ModuleKeyword:
//                case SyntaxKind::NamespaceKeyword:
//                    return parseModuleDeclaration(pos, hasJSDoc, decorators, modifiers);
//                case SyntaxKind::ImportKeyword:
//                    return parseImportDeclarationOrImportEqualsDeclaration(pos, hasJSDoc, decorators, modifiers);
//                case SyntaxKind::ExportKeyword:
//                    nextToken();
//                    switch (token()) {
//                        case SyntaxKind::DefaultKeyword:
//                        case SyntaxKind::EqualsToken:
//                            return parseExportAssignment(pos, hasJSDoc, decorators, modifiers);
//                        case SyntaxKind::AsKeyword:
//                            return parseNamespaceExportDeclaration(pos, hasJSDoc, decorators, modifiers);
//                        default:
//                            return parseExportDeclaration(pos, hasJSDoc, decorators, modifiers);
//                    }
//                default:
//                    if (decorators || modifiers) {
//                        // We reached this point because we encountered decorators and/or modifiers and assumed a declaration
//                        // would follow. For recovery and error reporting purposes, return an incomplete declaration.
//                        auto missing = createMissingNode<MissingDeclaration>(SyntaxKind::MissingDeclaration, /*reportAtCurrentPosition*/ true, Diagnostics::Declaration_expected);
//                        setTextRangePos(missing, pos);
//                        missing.decorators = decorators;
//                        missing.modifiers = modifiers;
//                        return missing;
//                    }
//                    return undefined!; // TODO: GH#18217
            }
            throw runtime_error("not implemented");
        }

        shared<Statement> parseStatement() {
            ZoneScoped;
            switch (token()) {
                case SyntaxKind::SemicolonToken:
                    return parseEmptyStatement();
                case SyntaxKind::OpenBraceToken:
                    return parseBlock(/*ignoreMissingOpenBrace*/ false);
                case SyntaxKind::VarKeyword:
                    return parseVariableStatement(getNodePos(), hasPrecedingJSDocComment(), /*decorators*/ {}, /*modifiers*/ {});
                case SyntaxKind::LetKeyword:
                    if (isLetDeclaration()) {
                        return parseVariableStatement(getNodePos(), hasPrecedingJSDocComment(), /*decorators*/ {}, /*modifiers*/ {});
                    }
                    break;
                case SyntaxKind::FunctionKeyword:
                    return parseFunctionDeclaration(getNodePos(), hasPrecedingJSDocComment(), /*decorators*/ {}, /*modifiers*/ {});
//                case SyntaxKind::ClassKeyword:
//                    return parseClassDeclaration(getNodePos(), hasPrecedingJSDocComment(), /*decorators*/ {}, /*modifiers*/ {});
//                case SyntaxKind::IfKeyword:
//                    return parseIfStatement();
//                case SyntaxKind::DoKeyword:
//                    return parseDoStatement();
//                case SyntaxKind::WhileKeyword:
//                    return parseWhileStatement();
//                case SyntaxKind::ForKeyword:
//                    return parseForOrForInOrForOfStatement();
//                case SyntaxKind::ContinueKeyword:
//                    return parseBreakOrContinueStatement(SyntaxKind::ContinueStatement);
//                case SyntaxKind::BreakKeyword:
//                    return parseBreakOrContinueStatement(SyntaxKind::BreakStatement);
//                case SyntaxKind::ReturnKeyword:
//                    return parseReturnStatement();
//                case SyntaxKind::WithKeyword:
//                    return parseWithStatement();
//                case SyntaxKind::SwitchKeyword:
//                    return parseSwitchStatement();
//                case SyntaxKind::ThrowKeyword:
//                    return parseThrowStatement();
//                case SyntaxKind::TryKeyword:
//                // Include 'catch' and 'finally' for error recovery.
//                // falls through
//                case SyntaxKind::CatchKeyword:
//                case SyntaxKind::FinallyKeyword:
//                    return parseTryStatement();
//                case SyntaxKind::DebuggerKeyword:
//                    return parseDebuggerStatement();
                case SyntaxKind::AtToken:
                    return parseDeclaration();
                case SyntaxKind::AsyncKeyword:
                case SyntaxKind::InterfaceKeyword:
                case SyntaxKind::TypeKeyword:
                case SyntaxKind::ModuleKeyword:
                case SyntaxKind::NamespaceKeyword:
                case SyntaxKind::DeclareKeyword:
                case SyntaxKind::ConstKeyword:
                case SyntaxKind::EnumKeyword:
                case SyntaxKind::ExportKeyword:
                case SyntaxKind::ImportKeyword:
                case SyntaxKind::PrivateKeyword:
                case SyntaxKind::ProtectedKeyword:
                case SyntaxKind::PublicKeyword:
                case SyntaxKind::AbstractKeyword:
                case SyntaxKind::StaticKeyword:
                case SyntaxKind::ReadonlyKeyword:
                case SyntaxKind::GlobalKeyword:
                    if (isStartOfDeclaration()) {
                        return parseDeclaration();
                    }
                    break;
            }
            return to<Statement>(parseExpressionOrLabeledStatement());
        }

        shared<SourceFile> parseSourceFileWorker(ScriptTarget languageVersion, bool setParentNodes, ScriptKind scriptKind, function<void(shared<SourceFile>)> setExternalModuleIndicator) {
            ZoneScoped;
            auto isDeclarationFile = isDeclarationFileName(fileName);
            if (isDeclarationFile) {
                contextFlags |= (int) NodeFlags::Ambient;
            }

            sourceFlags = contextFlags;

            // Prime the scanner.
            nextToken();

            auto statements = parseList(ParsingContext::SourceElements, CALLBACK(parseStatement));
//            auto statements = parseList(ParsingContext::SourceElements, [this]() {
//                return this->parseStatement();
//            });
            Debug::asserts(token() == SyntaxKind::EndOfFileToken);
            auto endOfFileToken = addJSDocComment(parseTokenNode<EndOfFileToken>());

            auto sourceFile = createSourceFile(fileName, languageVersion, scriptKind, isDeclarationFile, statements, endOfFileToken, sourceFlags, setExternalModuleIndicator);

//            // A member of ReadonlyArray<T> isn't assignable to a member of T[] (and prevents a direct cast) - but this is where we set up those members so they can be readonly in the future
//            processCommentPragmas(sourceFile as {} as PragmaContext, sourceText);
//            processPragmasIntoFields(sourceFile as {} as PragmaContext, reportPragmaDiagnostic);
//
//            sourceFile.commentDirectives = scanner.getCommentDirectives();
//            sourceFile.nodeCount = nodeCount;
//            sourceFile.identifierCount = identifierCount;
//            sourceFile.identifiers = identifiers;
//            sourceFile.parseDiagnostics = attachFileToDiagnostics(parseDiagnostics, sourceFile);
//            if (jsDocDiagnostics) {
//                sourceFile.jsDocDiagnostics = attachFileToDiagnostics(jsDocDiagnostics, sourceFile);
//            }
//
//            if (setParentNodes) {
//                fixupParentReferences(sourceFile);
//            }
//
            return sourceFile;
//
//            function reportPragmaDiagnostic(int pos, end: number, diagnostic: DiagnosticMessage) {
//                parseDiagnostics.push(createDetachedDiagnostic(fileName, pos, end, diagnostic));
//            }
        }
//

//        // DECLARATIONS
//        function parseClassDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray>  decorators, sharedOpt<NodeArray>  modifiers): ClassDeclaration {
//            return parseClassDeclarationOrExpression(pos, hasJSDoc, decorators, modifiers, SyntaxKind::ClassDeclaration) as ClassDeclaration;
//        }
//
//        function parseInterfaceDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray>  decorators, sharedOpt<NodeArray>  modifiers): InterfaceDeclaration {
//            parseExpected(SyntaxKind::InterfaceKeyword);
//            auto name = parseIdentifier();
//            auto typeParameters = parseTypeParameters();
//            auto heritageClauses = parseHeritageClauses();
//            auto members = parseObjectTypeMembers();
//            auto node = factory.createInterfaceDeclaration(decorators, modifiers, name, typeParameters, heritageClauses, members);
//            return withJSDoc(finishNode(node, pos), hasJSDoc);
//        }

        shared<TypeAliasDeclaration> parseTypeAliasDeclaration(int pos, bool hasJSDoc, const sharedOpt<NodeArray> &decorators, const sharedOpt<NodeArray> &modifiers) {
            parseExpected(SyntaxKind::TypeKeyword);
            auto name = parseIdentifier();
            auto typeParameters = parseTypeParameters();
            parseExpected(SyntaxKind::EqualsToken);
            shared<TypeNode> type = token() == SyntaxKind::IntrinsicKeyword ? tryParse<sharedOpt<TypeNode>>(CALLBACK(parseKeywordAndNoDot)) : parseType();
            parseSemicolon();
            auto node = factory.createTypeAliasDeclaration(decorators, modifiers, name, typeParameters, type);
            return withJSDoc(finishNode(node, pos), hasJSDoc);
        }

//        // In an ambient declaration, the grammar only allows integer literals as initializers.
//        // In a non-ambient declaration, the grammar allows uninitialized members only in a
//        // ConstantEnumMemberSection, which starts at the beginning of an enum declaration
//        // or any time an integer literal initializer is encountered.
//        function parseEnumMember(): EnumMember {
//            auto pos = getNodePos();
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            auto name = parsePropertyName();
//            auto initializer = allowInAnd(parseInitializer);
//            return withJSDoc(finishNode(factory.createEnumMember(name, initializer), pos), hasJSDoc);
//        }
//
//        function parseEnumDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray>  decorators, sharedOpt<NodeArray>  modifiers): EnumDeclaration {
//            parseExpected(SyntaxKind::EnumKeyword);
//            auto name = parseIdentifier();
//            auto members;
//            if (parseExpected(SyntaxKind::OpenBraceToken)) {
//                members = doOutsideOfYieldAndAwaitContext(() => parseDelimitedList(ParsingContext::EnumMembers, parseEnumMember));
//                parseExpected(SyntaxKind::CloseBraceToken);
//            }
//            else {
//                members = createMissingList<EnumMember>();
//            }
//            auto node = factory.createEnumDeclaration(decorators, modifiers, name, members);
//            return withJSDoc(finishNode(node, pos), hasJSDoc);
//        }
//
//        function parseModuleBlock(): ModuleBlock {
//            auto pos = getNodePos();
//            auto statements;
//            if (parseExpected(SyntaxKind::OpenBraceToken)) {
//                statements = parseList(ParsingContext::BlockStatements, parseStatement);
//                parseExpected(SyntaxKind::CloseBraceToken);
//            }
//            else {
//                statements = createMissingList<Statement>();
//            }
//            return finishNode(factory.createModuleBlock(statements), pos);
//        }
//
//        function parseModuleOrNamespaceDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray>  decorators, sharedOpt<NodeArray>  modifiers, flags: NodeFlags): ModuleDeclaration {
//            // If we are parsing a dotted namespace name, we want to
//            // propagate the 'Namespace' flag across the names if set.
//            auto namespaceFlag = flags & NodeFlags::Namespace;
//            auto name = parseIdentifier();
//            auto body = parseOptional(SyntaxKind::DotToken)
//                ? parseModuleOrNamespaceDeclaration(getNodePos(), /*hasJSDoc*/ false, /*decorators*/ {}, /*modifiers*/ {}, NodeFlags::NestedNamespace | namespaceFlag) as NamespaceDeclaration
//                : parseModuleBlock();
//            auto node = factory.createModuleDeclaration(decorators, modifiers, name, body, flags);
//            return withJSDoc(finishNode(node, pos), hasJSDoc);
//        }
//
//        function parseAmbientExternalModuleDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray>  decorators, sharedOpt<NodeArray>  modifiers): ModuleDeclaration {
//            auto flags: NodeFlags = 0;
//            auto name;
//            if (token() == SyntaxKind::GlobalKeyword) {
//                // parse 'global' as name of global scope augmentation
//                name = parseIdentifier();
//                flags |= NodeFlags::GlobalAugmentation;
//            }
//            else {
//                name = parseLiteralNode() as StringLiteral;
//                name.text = internIdentifier(name.text);
//            }
//            auto body: ModuleBlock | undefined;
//            if (token() == SyntaxKind::OpenBraceToken) {
//                body = parseModuleBlock();
//            }
//            else {
//                parseSemicolon();
//            }
//            auto node = factory.createModuleDeclaration(decorators, modifiers, name, body, flags);
//            return withJSDoc(finishNode(node, pos), hasJSDoc);
//        }
//
//        function parseModuleDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray>  decorators, sharedOpt<NodeArray>  modifiers): ModuleDeclaration {
//            auto flags: NodeFlags = 0;
//            if (token() == SyntaxKind::GlobalKeyword) {
//                // global augmentation
//                return parseAmbientExternalModuleDeclaration(pos, hasJSDoc, decorators, modifiers);
//            }
//            else if (parseOptional(SyntaxKind::NamespaceKeyword)) {
//                flags |= NodeFlags::Namespace;
//            }
//            else {
//                parseExpected(SyntaxKind::ModuleKeyword);
//                if (token() == SyntaxKind::StringLiteral) {
//                    return parseAmbientExternalModuleDeclaration(pos, hasJSDoc, decorators, modifiers);
//                }
//            }
//            return parseModuleOrNamespaceDeclaration(pos, hasJSDoc, decorators, modifiers, flags);
//        }
//
//        function parseNamespaceExportDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray>  decorators, sharedOpt<NodeArray>  modifiers): NamespaceExportDeclaration {
//            parseExpected(SyntaxKind::AsKeyword);
//            parseExpected(SyntaxKind::NamespaceKeyword);
//            auto name = parseIdentifier();
//            parseSemicolon();
//            auto node = factory.createNamespaceExportDeclaration(name);
//            // NamespaceExportDeclaration nodes cannot have decorators or modifiers, so we attach them here so we can report them in the grammar checker
//            node.decorators = decorators;
//            node.modifiers = modifiers;
//            return withJSDoc(finishNode(node, pos), hasJSDoc);
//        }
//
//        function parseImportDeclarationOrImportEqualsDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray>  decorators, sharedOpt<NodeArray>  modifiers): ImportEqualsDeclaration | ImportDeclaration {
//            parseExpected(SyntaxKind::ImportKeyword);
//
//            auto afterImportPos = scanner.getStartPos();
//
//            // We don't parse the identifier here in await context, instead we will report a grammar error in the checker.
//            auto identifier: Identifier | undefined;
//            if (isIdentifier()) {
//                identifier = parseIdentifier();
//            }
//
//            auto isTypeOnly = false;
//            if (token() != SyntaxKind::FromKeyword &&
//                identifier?.escapedText == "type" &&
//                (isIdentifier() || tokenAfterImportDefinitelyProducesImportDeclaration())
//            ) {
//                isTypeOnly = true;
//                identifier = isIdentifier() ? parseIdentifier() : undefined;
//            }
//
//            if (identifier && !tokenAfterImportedIdentifierDefinitelyProducesImportDeclaration()) {
//                return parseImportEqualsDeclaration(pos, hasJSDoc, decorators, modifiers, identifier, isTypeOnly);
//            }
//
//            // ImportDeclaration:
//            //  import ImportClause from ModuleSpecifier ;
//            //  import ModuleSpecifier;
//            auto importClause: ImportClause | undefined;
//            if (identifier || // import id
//                token() == SyntaxKind::AsteriskToken || // import *
//                token() == SyntaxKind::OpenBraceToken    // import {
//            ) {
//                importClause = parseImportClause(identifier, afterImportPos, isTypeOnly);
//                parseExpected(SyntaxKind::FromKeyword);
//            }
//            auto moduleSpecifier = parseModuleSpecifier();
//
//            auto assertClause: AssertClause | undefined;
//            if (token() == SyntaxKind::AssertKeyword && !scanner.hasPrecedingLineBreak()) {
//                assertClause = parseAssertClause();
//            }
//
//            parseSemicolon();
//            auto node = factory.createImportDeclaration(decorators, modifiers, importClause, moduleSpecifier, assertClause);
//            return withJSDoc(finishNode(node, pos), hasJSDoc);
//        }
//
//        function tokenAfterImportDefinitelyProducesImportDeclaration() {
//            return token() == SyntaxKind::AsteriskToken || token() == SyntaxKind::OpenBraceToken;
//        }
//
//        function tokenAfterImportedIdentifierDefinitelyProducesImportDeclaration() {
//            // In `import id ___`, the current token decides whether to produce
//            // an ImportDeclaration or ImportEqualsDeclaration.
//            return token() == SyntaxKind::CommaToken || token() == SyntaxKind::FromKeyword;
//        }
//
//        function parseImportEqualsDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray>  decorators, sharedOpt<NodeArray>  modifiers, identifier: Identifier, isTypeOnly: boolean): ImportEqualsDeclaration {
//            parseExpected(SyntaxKind::EqualsToken);
//            auto moduleReference = parseModuleReference();
//            parseSemicolon();
//            auto node = factory.createImportEqualsDeclaration(decorators, modifiers, isTypeOnly, identifier, moduleReference);
//            auto finished = withJSDoc(finishNode(node, pos), hasJSDoc);
//            return finished;
//        }
//
//        function parseImportClause(identifier: Identifier | undefined, int pos, isTypeOnly: boolean) {
//            // ImportClause:
//            //  ImportedDefaultBinding
//            //  NameSpaceImport
//            //  NamedImports
//            //  ImportedDefaultBinding, NameSpaceImport
//            //  ImportedDefaultBinding, NamedImports
//
//            // If there was no default import or if there is comma token after default import
//            // parse namespace or named imports
//            auto namedBindings: NamespaceImport | NamedImports | undefined;
//            if (!identifier ||
//                parseOptional(SyntaxKind::CommaToken)) {
//                namedBindings = token() == SyntaxKind::AsteriskToken ? parseNamespaceImport() : parseNamedImportsOrExports(SyntaxKind::NamedImports);
//            }
//
//            return finishNode(factory.createImportClause(isTypeOnly, identifier, namedBindings), pos);
//        }
//
//        function parseModuleReference() {
//            return isExternalModuleReference()
//                ? parseExternalModuleReference()
//                : parseEntityName(/*allowReservedWords*/ false);
//        }
//
//        function parseExternalModuleReference() {
//            auto pos = getNodePos();
//            parseExpected(SyntaxKind::RequireKeyword);
//            parseExpected(SyntaxKind::OpenParenToken);
//            auto expression = parseModuleSpecifier();
//            parseExpected(SyntaxKind::CloseParenToken);
//            return finishNode(factory.createExternalModuleReference(expression), pos);
//        }
//
//        function parseModuleSpecifier(): Expression {
//            if (token() == SyntaxKind::StringLiteral) {
//                auto result = parseLiteralNode();
//                result.text = internIdentifier(result.text);
//                return result;
//            }
//            else {
//                // We allow arbitrary expressions here, even though the grammar only allows string
//                // literals.  We check to ensure that it is only a string literal later in the grammar
//                // check pass.
//                return parseExpression();
//            }
//        }
//
//        function parseNamespaceImport(): NamespaceImport {
//            // NameSpaceImport:
//            //  * as ImportedBinding
//            auto pos = getNodePos();
//            parseExpected(SyntaxKind::AsteriskToken);
//            parseExpected(SyntaxKind::AsKeyword);
//            auto name = parseIdentifier();
//            return finishNode(factory.createNamespaceImport(name), pos);
//        }
//
//        function parseNamedImportsOrExports(kind: SyntaxKind::NamedImports): NamedImports;
//        function parseNamedImportsOrExports(kind: SyntaxKind::NamedExports): NamedExports;
//        function parseNamedImportsOrExports(kind: SyntaxKind): NamedImportsOrExports {
//            auto pos = getNodePos();
//
//            // NamedImports:
//            //  { }
//            //  { ImportsList }
//            //  { ImportsList, }
//
//            // ImportsList:
//            //  ImportSpecifier
//            //  ImportsList, ImportSpecifier
//            auto node = kind == SyntaxKind::NamedImports
//                ? factory.createNamedImports(parseBracketedList(ParsingContext::ImportOrExportSpecifiers, parseImportSpecifier, SyntaxKind::OpenBraceToken, SyntaxKind::CloseBraceToken))
//                : factory.createNamedExports(parseBracketedList(ParsingContext::ImportOrExportSpecifiers, parseExportSpecifier, SyntaxKind::OpenBraceToken, SyntaxKind::CloseBraceToken));
//            return finishNode(node, pos);
//        }
//
//        function parseExportSpecifier() {
//            auto hasJSDoc = hasPrecedingJSDocComment();
//            return withJSDoc(parseImportOrExportSpecifier(SyntaxKind::ExportSpecifier) as ExportSpecifier, hasJSDoc);
//        }
//
//        function parseImportSpecifier() {
//            return parseImportOrExportSpecifier(SyntaxKind::ImportSpecifier) as ImportSpecifier;
//        }
//
//        function parseImportOrExportSpecifier(kind: SyntaxKind): ImportOrExportSpecifier {
//            auto pos = getNodePos();
//            // ImportSpecifier:
//            //   BindingIdentifier
//            //   IdentifierName as BindingIdentifier
//            // ExportSpecifier:
//            //   IdentifierName
//            //   IdentifierName as IdentifierName
//            auto checkIdentifierIsKeyword = isKeyword(token()) && !isIdentifier();
//            auto checkIdentifierStart = scanner.getTokenPos();
//            auto checkIdentifierEnd = scanner.getTextPos();
//            auto isTypeOnly = false;
//            auto propertyName: Identifier | undefined;
//            auto canParseAsKeyword = true;
//            auto name = parseIdentifierName();
//            if (name.escapedText == "type") {
//                // If the first token of an import specifier is 'type', there are a lot of possibilities,
//                // especially if we see 'as' afterwards:
//                //
//                // import { type } from "mod";          - isTypeOnly: false,   name: type
//                // import { type as } from "mod";       - isTypeOnly: true,    name: as
//                // import { type as as } from "mod";    - isTypeOnly: false,   name: as,    propertyName: type
//                // import { type as as as } from "mod"; - isTypeOnly: true,    name: as,    propertyName: as
//                if (token() == SyntaxKind::AsKeyword) {
//                    // { type as ...? }
//                    auto firstAs = parseIdentifierName();
//                    if (token() == SyntaxKind::AsKeyword) {
//                        // { type as as ...? }
//                        auto secondAs = parseIdentifierName();
//                        if (tokenIsIdentifierOrKeyword(token())) {
//                            // { type as as something }
//                            isTypeOnly = true;
//                            propertyName = firstAs;
//                            name = parseNameWithKeywordCheck();
//                            canParseAsKeyword = false;
//                        }
//                        else {
//                            // { type as as }
//                            propertyName = name;
//                            name = secondAs;
//                            canParseAsKeyword = false;
//                        }
//                    }
//                    else if (tokenIsIdentifierOrKeyword(token())) {
//                        // { type as something }
//                        propertyName = name;
//                        canParseAsKeyword = false;
//                        name = parseNameWithKeywordCheck();
//                    }
//                    else {
//                        // { type as }
//                        isTypeOnly = true;
//                        name = firstAs;
//                    }
//                }
//                else if (tokenIsIdentifierOrKeyword(token())) {
//                    // { type something ...? }
//                    isTypeOnly = true;
//                    name = parseNameWithKeywordCheck();
//                }
//            }
//
//            if (canParseAsKeyword && token() == SyntaxKind::AsKeyword) {
//                propertyName = name;
//                parseExpected(SyntaxKind::AsKeyword);
//                name = parseNameWithKeywordCheck();
//            }
//            if (kind == SyntaxKind::ImportSpecifier && checkIdentifierIsKeyword) {
//                parseErrorAt(checkIdentifierStart, checkIdentifierEnd, Diagnostics::Identifier_expected);
//            }
//            auto node = kind == SyntaxKind::ImportSpecifier
//                ? factory.createImportSpecifier(isTypeOnly, propertyName, name)
//                : factory.createExportSpecifier(isTypeOnly, propertyName, name);
//            return finishNode(node, pos);
//
//            function parseNameWithKeywordCheck() {
//                checkIdentifierIsKeyword = isKeyword(token()) && !isIdentifier();
//                checkIdentifierStart = scanner.getTokenPos();
//                checkIdentifierEnd = scanner.getTextPos();
//                return parseIdentifierName();
//            }
//        }
//
//        function parseNamespaceExport(int pos): NamespaceExport {
//            return finishNode(factory.createNamespaceExport(parseIdentifierName()), pos);
//        }
//
//        function parseExportDeclaration(int pos, bool hasJSDoc, sharedOpt<NodeArray>  decorators, sharedOpt<NodeArray>  modifiers): ExportDeclaration {
//            auto savedAwaitContext = inAwaitContext();
//            setAwaitContext(/*value*/ true);
//            auto exportClause: NamedExportBindings | undefined;
//            auto moduleSpecifier: Expression | undefined;
//            auto assertClause: AssertClause | undefined;
//            auto isTypeOnly = parseOptional(SyntaxKind::TypeKeyword);
//            auto namespaceExportPos = getNodePos();
//            if (parseOptional(SyntaxKind::AsteriskToken)) {
//                if (parseOptional(SyntaxKind::AsKeyword)) {
//                    exportClause = parseNamespaceExport(namespaceExportPos);
//                }
//                parseExpected(SyntaxKind::FromKeyword);
//                moduleSpecifier = parseModuleSpecifier();
//            }
//            else {
//                exportClause = parseNamedImportsOrExports(SyntaxKind::NamedExports);
//                // It is not uncommon to accidentally omit the 'from' keyword. Additionally, in editing scenarios,
//                // the 'from' keyword can be parsed as a named export when the export clause is unterminated (i.e. `export { from "moduleName";`)
//                // If we don't have a 'from' keyword, see if we have a string literal such that ASI won't take effect.
//                if (token() == SyntaxKind::FromKeyword || (token() == SyntaxKind::StringLiteral && !scanner.hasPrecedingLineBreak())) {
//                    parseExpected(SyntaxKind::FromKeyword);
//                    moduleSpecifier = parseModuleSpecifier();
//                }
//            }
//            if (moduleSpecifier && token() == SyntaxKind::AssertKeyword && !scanner.hasPrecedingLineBreak()) {
//                assertClause = parseAssertClause();
//            }
//            parseSemicolon();
//            setAwaitContext(savedAwaitContext);
//            auto node = factory.createExportDeclaration(decorators, modifiers, isTypeOnly, exportClause, moduleSpecifier, assertClause);
//            return withJSDoc(finishNode(node, pos), hasJSDoc);
//        }
//
//        function parseExportAssignment(int pos, bool hasJSDoc, sharedOpt<NodeArray>  decorators, sharedOpt<NodeArray>  modifiers): ExportAssignment {
//            auto savedAwaitContext = inAwaitContext();
//            setAwaitContext(/*value*/ true);
//            auto isExportEquals: boolean | undefined;
//            if (parseOptional(SyntaxKind::EqualsToken)) {
//                isExportEquals = true;
//            }
//            else {
//                parseExpected(SyntaxKind::DefaultKeyword);
//            }
//            auto expression = parseAssignmentExpressionOrHigher();
//            parseSemicolon();
//            setAwaitContext(savedAwaitContext);
//            auto node = factory.createExportAssignment(decorators, modifiers, isExportEquals, expression);
//            return withJSDoc(finishNode(node, pos), hasJSDoc);
//        }
//
//        export namespace JSDocParser {
//            export function parseJSDocTypeExpressionForTests(content: string, start: number | undefined, length: number | undefined): { jsDocTypeExpression: JSDocTypeExpression, diagnostics: Diagnostic[] } | undefined {
//                initializeState("file.js", content, ScriptTarget.Latest, /*_syntaxCursor:*/ undefined, ScriptKind.JS);
//                scanner.setText(content, start, length);
//                currentToken = scanner.scan();
//                auto jsDocTypeExpression = parseJSDocTypeExpression();
//
//                auto sourceFile = createSourceFile("file.js", ScriptTarget.Latest, ScriptKind.JS, /*isDeclarationFile*/ false, [], factory.createToken(SyntaxKind::EndOfFileToken), NodeFlags::None, noop);
//                auto diagnostics = attachFileToDiagnostics(parseDiagnostics, sourceFile);
//                if (jsDocDiagnostics) {
//                    sourceFile.jsDocDiagnostics = attachFileToDiagnostics(jsDocDiagnostics, sourceFile);
//                }
//
//                clearState();
//
//                return jsDocTypeExpression ? { jsDocTypeExpression, diagnostics } : undefined;
//            }
//
//            // Parses out a JSDoc type expression.
//            export function parseJSDocTypeExpression(mayOmitBraces?: boolean): JSDocTypeExpression {
//                auto pos = getNodePos();
//                auto hasBrace = (mayOmitBraces ? parseOptional : parseExpected)(SyntaxKind::OpenBraceToken);
//                auto type = doInsideOfContext(NodeFlags::JSDoc, parseJSDocType);
//                if (!mayOmitBraces || hasBrace) {
//                    parseExpectedJSDoc(SyntaxKind::CloseBraceToken);
//                }
//
//                auto result = factory.createJSDocTypeExpression(type);
//                fixupParentReferences(result);
//                return finishNode(result, pos);
//            }
//
//            export function parseJSDocNameReference(): JSDocNameReference {
//                auto pos = getNodePos();
//                auto hasBrace = parseOptional(SyntaxKind::OpenBraceToken);
//                auto p2 = getNodePos();
//                auto entityName: EntityName | JSDocMemberName = parseEntityName(/* allowReservedWords*/ false);
//                while (token() == SyntaxKind::PrivateIdentifier) {
//                    reScanHashToken(); // rescan #id as # id
//                    nextTokenJSDoc(); // then skip the #
//                    entityName = finishNode(factory.createJSDocMemberName(entityName, parseIdentifier()), p2);
//                }
//                if (hasBrace) {
//                    parseExpectedJSDoc(SyntaxKind::CloseBraceToken);
//                }
//
//                auto result = factory.createJSDocNameReference(entityName);
//                fixupParentReferences(result);
//                return finishNode(result, pos);
//            }
//
//            export function parseIsolatedJSDocComment(content: string, start: number | undefined, length: number | undefined): { jsDoc: JSDoc, diagnostics: Diagnostic[] } | undefined {
//                initializeState("", content, ScriptTarget.Latest, /*_syntaxCursor:*/ undefined, ScriptKind.JS);
//                auto jsDoc = doInsideOfContext(NodeFlags::JSDoc, () => parseJSDocCommentWorker(start, length));
//
//                auto sourceFile = { languageVariant: LanguageVariant::Standard, text: content } as SourceFile;
//                auto diagnostics = attachFileToDiagnostics(parseDiagnostics, sourceFile);
//                clearState();
//
//                return jsDoc ? { jsDoc, diagnostics } : undefined;
//            }
//
//            export function parseJSDocComment(parent: HasJSDoc, start: number, length: number): JSDoc | undefined {
//                auto saveToken = currentToken;
//                auto saveParseDiagnosticsLength = parseDiagnostics.length;
//                auto saveParseErrorBeforeNextFinishedNode = parseErrorBeforeNextFinishedNode;
//
//                auto comment = doInsideOfContext(NodeFlags::JSDoc, () => parseJSDocCommentWorker(start, length));
//                setParent(comment, parent);
//
//                if (contextFlags & NodeFlags::JavaScriptFile) {
//                    if (!jsDocDiagnostics) {
//                        jsDocDiagnostics = [];
//                    }
//                    jsDocDiagnostics::push(...parseDiagnostics);
//                }
//                currentToken = saveToken;
//                parseDiagnostics.length = saveParseDiagnosticsLength;
//                parseErrorBeforeNextFinishedNode = saveParseErrorBeforeNextFinishedNode;
//                return comment;
//            }
//
//            auto enum JSDocState {
//                BeginningOfLine,
//                SawAsterisk,
//                SavingComments,
//                SavingBackticks, // NOTE: Only used when parsing tag comments
//            }
//
//            auto enum PropertyLikeParse {
//                Property = 1 << 0,
//                Parameter = 1 << 1,
//                CallbackParameter = 1 << 2,
//            }
//
//            function parseJSDocCommentWorker(start = 0, length: number | undefined): JSDoc | undefined {
//                auto content = sourceText;
//                auto end = length == undefined ? content.length : start + length;
//                length = end - start;
//
//                Debug::asserts(start >= 0);
//                Debug::asserts(start <= end);
//                Debug::asserts(end <= content.length);
//
//                // Check for /** (JSDoc opening part)
//                if (!isJSDocLikeText(content, start)) {
//                    return undefined;
//                }
//
//                auto tags: JSDocTag[];
//                auto tagsPos: number;
//                auto tagsEnd: number;
//                auto linkEnd: number;
//                auto commentsPos: number | undefined;
//                auto comments: string[] = [];
//                auto parts: JSDocComment[] = [];
//
//                // + 3 for leading /**, - 5 in total for /** */
//                return scanner.scanRange(start + 3, length - 5, () => {
//                    // Initially we can parse out a tag.  We also have seen a starting asterisk.
//                    // This is so that /** * @type */ doesn't parse.
//                    auto state = JSDocState.SawAsterisk;
//                    auto margin: number | undefined;
//                    // + 4 for leading '/** '
//                    // + 1 because the last index of \n is always one index before the first character in the line and coincidentally, if there is no \n before start, it is -1, which is also one index before the first character
//                    auto indent = start - (content.lastIndexOf("\n", start) + 1) + 4;
//                    function pushComment(text: string) {
//                        if (!margin) {
//                            margin = indent;
//                        }
//                        comments.push(text);
//                        indent += text.length;
//                    }
//
//                    nextTokenJSDoc();
//                    while (parseOptionalJsdoc(SyntaxKind::WhitespaceTrivia));
//                    if (parseOptionalJsdoc(SyntaxKind::NewLineTrivia)) {
//                        state = JSDocState.BeginningOfLine;
//                        indent = 0;
//                    }
//                    loop: while (true) {
//                        switch (token()) {
//                            case SyntaxKind::AtToken:
//                                if (state == JSDocState.BeginningOfLine || state == JSDocState.SawAsterisk) {
//                                    removeTrailingWhitespace(comments);
//                                    if (!commentsPos) commentsPos = getNodePos();
//                                    addTag(parseTag(indent));
//                                    // NOTE: According to usejsdoc.org, a tag goes to end of line, except the last tag.
//                                    // Real-world comments may break this rule, so "BeginningOfLine" will not be a real line beginning
//                                    // for malformed examples like `/** @param {string} x @returns {number} the length */`
//                                    state = JSDocState.BeginningOfLine;
//                                    margin = undefined;
//                                }
//                                else {
//                                    pushComment(scanner.getTokenText());
//                                }
//                                break;
//                            case SyntaxKind::NewLineTrivia:
//                                comments.push(scanner.getTokenText());
//                                state = JSDocState.BeginningOfLine;
//                                indent = 0;
//                                break;
//                            case SyntaxKind::AsteriskToken:
//                                auto asterisk = scanner.getTokenText();
//                                if (state == JSDocState.SawAsterisk || state == JSDocState.SavingComments) {
//                                    // If we've already seen an asterisk, then we can no longer parse a tag on this line
//                                    state = JSDocState.SavingComments;
//                                    pushComment(asterisk);
//                                }
//                                else {
//                                    // Ignore the first asterisk on a line
//                                    state = JSDocState.SawAsterisk;
//                                    indent += asterisk.length;
//                                }
//                                break;
//                            case SyntaxKind::WhitespaceTrivia:
//                                // only collect whitespace if we're already saving comments or have just crossed the comment indent margin
//                                auto whitespace = scanner.getTokenText();
//                                if (state == JSDocState.SavingComments) {
//                                    comments.push(whitespace);
//                                }
//                                else if (margin != undefined && indent + whitespace.length > margin) {
//                                    comments.push(whitespace.slice(margin - indent));
//                                }
//                                indent += whitespace.length;
//                                break;
//                            case SyntaxKind::EndOfFileToken:
//                                break loop;
//                            case SyntaxKind::OpenBraceToken:
//                                state = JSDocState.SavingComments;
//                                auto commentEnd = scanner.getStartPos();
//                                auto linkStart = scanner.getTextPos() - 1;
//                                auto link = parseJSDocLink(linkStart);
//                                if (link) {
//                                    if (!linkEnd) {
//                                        removeLeadingNewlines(comments);
//                                    }
//                                    parts.push(finishNode(factory.createJSDocText(comments.join("")), linkEnd ?? start, commentEnd));
//                                    parts.push(link);
//                                    comments = [];
//                                    linkEnd = scanner.getTextPos();
//                                    break;
//                                }
//                                // fallthrough if it's not a {@link sequence
//                            default:
//                                // Anything else is doc comment text. We just save it. Because it
//                                // wasn't a tag, we can no longer parse a tag on this line until we hit the next
//                                // line break.
//                                state = JSDocState.SavingComments;
//                                pushComment(scanner.getTokenText());
//                                break;
//                        }
//                        nextTokenJSDoc();
//                    }
//                    removeTrailingWhitespace(comments);
//                    if (parts.length && comments.length) {
//                        parts.push(finishNode(factory.createJSDocText(comments.join("")), linkEnd ?? start, commentsPos));
//                    }
//                    if (parts.length && tags) Debug::assertsIsDefined(commentsPos, "having parsed tags implies that the end of the comment span should be set");
//                    auto tagsArray = tags && createNodeArray(tags, tagsPos, tagsEnd);
//                    return finishNode(factory.createJSDocComment(parts.length ? createNodeArray(parts, start, commentsPos) : comments.length ? comments.join("") : undefined, tagsArray), start, end);
//                });
//
//                function removeLeadingNewlines(comments: string[]) {
//                    while (comments.length && (comments[0] == "\n" || comments[0] == "\r")) {
//                        comments.shift();
//                    }
//                }
//
//                function removeTrailingWhitespace(comments: string[]) {
//                    while (comments.length && comments[comments.length - 1].trim() == "") {
//                        comments.pop();
//                    }
//                }
//
//                function isNextNonwhitespaceTokenEndOfFile(): boolean {
//                    // We must use infinite lookahead, as there could be any number of newlines :(
//                    while (true) {
//                        nextTokenJSDoc();
//                        if (token() == SyntaxKind::EndOfFileToken) {
//                            return true;
//                        }
//                        if (!(token() == SyntaxKind::WhitespaceTrivia || token() == SyntaxKind::NewLineTrivia)) {
//                            return false;
//                        }
//                    }
//                }
//
//                function skipWhitespace(): void {
//                    if (token() == SyntaxKind::WhitespaceTrivia || token() == SyntaxKind::NewLineTrivia) {
//                        if (lookAhead(isNextNonwhitespaceTokenEndOfFile)) {
//                            return; // Don't skip whitespace prior to EoF (or end of comment) - that shouldn't be included in any node's range
//                        }
//                    }
//                    while (token() == SyntaxKind::WhitespaceTrivia || token() == SyntaxKind::NewLineTrivia) {
//                        nextTokenJSDoc();
//                    }
//                }
//
//                function skipWhitespaceOrAsterisk(): string {
//                    if (token() == SyntaxKind::WhitespaceTrivia || token() == SyntaxKind::NewLineTrivia) {
//                        if (lookAhead(isNextNonwhitespaceTokenEndOfFile)) {
//                            return ""; // Don't skip whitespace prior to EoF (or end of comment) - that shouldn't be included in any node's range
//                        }
//                    }
//
//                    auto precedingLineBreak = scanner.hasPrecedingLineBreak();
//                    auto seenLineBreak = false;
//                    auto indentText = "";
//                    while ((precedingLineBreak && token() == SyntaxKind::AsteriskToken) || token() == SyntaxKind::WhitespaceTrivia || token() == SyntaxKind::NewLineTrivia) {
//                        indentText += scanner.getTokenText();
//                        if (token() == SyntaxKind::NewLineTrivia) {
//                            precedingLineBreak = true;
//                            seenLineBreak = true;
//                            indentText = "";
//                        }
//                        else if (token() == SyntaxKind::AsteriskToken) {
//                            precedingLineBreak = false;
//                        }
//                        nextTokenJSDoc();
//                    }
//                    return seenLineBreak ? indentText : "";
//                }
//
//                function parseTag(margin: number) {
//                    Debug::asserts(token() == SyntaxKind::AtToken);
//                    auto start = scanner.getTokenPos();
//                    nextTokenJSDoc();
//
//                    auto tagName = parseJSDocIdentifierName(/*message*/ undefined);
//                    auto indentText = skipWhitespaceOrAsterisk();
//
//                    auto tag: JSDocTag | undefined;
//                    switch (tagName.escapedText) {
//                        case "author":
//                            tag = parseAuthorTag(start, tagName, margin, indentText);
//                            break;
//                        case "implements":
//                            tag = parseImplementsTag(start, tagName, margin, indentText);
//                            break;
//                        case "augments":
//                        case "extends":
//                            tag = parseAugmentsTag(start, tagName, margin, indentText);
//                            break;
//                        case "class":
//                        case "constructor":
//                            tag = parseSimpleTag(start, factory.createJSDocClassTag, tagName, margin, indentText);
//                            break;
//                        case "public":
//                            tag = parseSimpleTag(start, factory.createJSDocPublicTag, tagName, margin, indentText);
//                            break;
//                        case "private":
//                            tag = parseSimpleTag(start, factory.createJSDocPrivateTag, tagName, margin, indentText);
//                            break;
//                        case "protected":
//                            tag = parseSimpleTag(start, factory.createJSDocProtectedTag, tagName, margin, indentText);
//                            break;
//                        case "readonly":
//                            tag = parseSimpleTag(start, factory.createJSDocReadonlyTag, tagName, margin, indentText);
//                            break;
//                        case "override":
//                            tag = parseSimpleTag(start, factory.createJSDocOverrideTag, tagName, margin, indentText);
//                            break;
//                        case "deprecated":
//                            hasDeprecatedTag = true;
//                            tag = parseSimpleTag(start, factory.createJSDocDeprecatedTag, tagName, margin, indentText);
//                            break;
//                        case "this":
//                            tag = parseThisTag(start, tagName, margin, indentText);
//                            break;
//                        case "enum":
//                            tag = parseEnumTag(start, tagName, margin, indentText);
//                            break;
//                        case "arg":
//                        case "argument":
//                        case "param":
//                            return parseParameterOrPropertyTag(start, tagName, PropertyLikeParse.Parameter, margin);
//                        case "return":
//                        case "returns":
//                            tag = parseReturnTag(start, tagName, margin, indentText);
//                            break;
//                        case "template":
//                            tag = parseTemplateTag(start, tagName, margin, indentText);
//                            break;
//                        case "type":
//                            tag = parseTypeTag(start, tagName, margin, indentText);
//                            break;
//                        case "typedef":
//                            tag = parseTypedefTag(start, tagName, margin, indentText);
//                            break;
//                        case "callback":
//                            tag = parseCallbackTag(start, tagName, margin, indentText);
//                            break;
//                        case "see":
//                            tag = parseSeeTag(start, tagName, margin, indentText);
//                            break;
//                        default:
//                            tag = parseUnknownTag(start, tagName, margin, indentText);
//                            break;
//                    }
//                    return tag;
//                }
//
//                function parseTrailingTagComments(int pos, end: number, margin: number, indentText: string) {
//                    // some tags, like typedef and callback, have already parsed their comments earlier
//                    if (!indentText) {
//                        margin += end - pos;
//                    }
//                    return parseTagComments(margin, indentText.slice(margin));
//                }
//
//                function parseTagComments(indent: number, initialMargin?: string): string | NodeArray<JSDocComment> | undefined {
//                    auto commentsPos = getNodePos();
//                    auto comments: string[] = [];
//                    auto parts: JSDocComment[] = [];
//                    auto linkEnd;
//                    auto state = JSDocState.BeginningOfLine;
//                    auto previousWhitespace = true;
//                    auto margin: number | undefined;
//                    function pushComment(text: string) {
//                        if (!margin) {
//                            margin = indent;
//                        }
//                        comments.push(text);
//                        indent += text.length;
//                    }
//                    if (initialMargin != undefined) {
//                        // jump straight to saving comments if there is some initial indentation
//                        if (initialMargin != "") {
//                            pushComment(initialMargin);
//                        }
//                        state = JSDocState.SawAsterisk;
//                    }
//                    auto tok = token() as JSDocSyntaxKind;
//                    loop: while (true) {
//                        switch (tok) {
//                            case SyntaxKind::NewLineTrivia:
//                                state = JSDocState.BeginningOfLine;
//                                // don't use pushComment here because we want to keep the margin unchanged
//                                comments.push(scanner.getTokenText());
//                                indent = 0;
//                                break;
//                            case SyntaxKind::AtToken:
//                                if (state == JSDocState.SavingBackticks
//                                    || state == JSDocState.SavingComments && (!previousWhitespace || lookAhead(isNextJSDocTokenWhitespace))) {
//                                    // @ doesn't start a new tag inside ``, and inside a comment, only after whitespace or not before whitespace
//                                    comments.push(scanner.getTokenText());
//                                    break;
//                                }
//                                scanner.setTextPos(scanner.getTextPos() - 1);
//                                // falls through
//                            case SyntaxKind::EndOfFileToken:
//                                // Done
//                                break loop;
//                            case SyntaxKind::WhitespaceTrivia:
//                                if (state == JSDocState.SavingComments || state == JSDocState.SavingBackticks) {
//                                    pushComment(scanner.getTokenText());
//                                }
//                                else {
//                                    auto whitespace = scanner.getTokenText();
//                                    // if the whitespace crosses the margin, take only the whitespace that passes the margin
//                                    if (margin != undefined && indent + whitespace.length > margin) {
//                                        comments.push(whitespace.slice(margin - indent));
//                                    }
//                                    indent += whitespace.length;
//                                }
//                                break;
//                            case SyntaxKind::OpenBraceToken:
//                                state = JSDocState.SavingComments;
//                                auto commentEnd = scanner.getStartPos();
//                                auto linkStart = scanner.getTextPos() - 1;
//                                auto link = parseJSDocLink(linkStart);
//                                if (link) {
//                                    parts.push(finishNode(factory.createJSDocText(comments.join("")), linkEnd ?? commentsPos, commentEnd));
//                                    parts.push(link);
//                                    comments = [];
//                                    linkEnd = scanner.getTextPos();
//                                }
//                                else {
//                                    pushComment(scanner.getTokenText());
//                                }
//                                break;
//                            case SyntaxKind::BacktickToken:
//                                if (state == JSDocState.SavingBackticks) {
//                                    state = JSDocState.SavingComments;
//                                }
//                                else {
//                                    state = JSDocState.SavingBackticks;
//                                }
//                                pushComment(scanner.getTokenText());
//                                break;
//                            case SyntaxKind::AsteriskToken:
//                                if (state == JSDocState.BeginningOfLine) {
//                                    // leading asterisks start recording on the *next* (non-whitespace) token
//                                    state = JSDocState.SawAsterisk;
//                                    indent += 1;
//                                    break;
//                                }
//                                // record the * as a comment
//                                // falls through
//                            default:
//                                if (state != JSDocState.SavingBackticks) {
//                                    state = JSDocState.SavingComments; // leading identifiers start recording as well
//                                }
//                                pushComment(scanner.getTokenText());
//                                break;
//                        }
//                        previousWhitespace = token() == SyntaxKind::WhitespaceTrivia;
//                        tok = nextTokenJSDoc();
//                    }
//
//                    removeLeadingNewlines(comments);
//                    removeTrailingWhitespace(comments);
//                    if (parts.length) {
//                        if (comments.length) {
//                            parts.push(finishNode(factory.createJSDocText(comments.join("")), linkEnd ?? commentsPos));
//                        }
//                        return createNodeArray(parts, commentsPos, scanner.getTextPos());
//                    }
//                    else if (comments.length) {
//                        return comments.join("");
//                    }
//                }
//
//                function isNextJSDocTokenWhitespace() {
//                    auto next = nextTokenJSDoc();
//                    return next == SyntaxKind::WhitespaceTrivia || next == SyntaxKind::NewLineTrivia;
//                }
//
//                function parseJSDocLink(start: number) {
//                    auto linkType = tryParse(parseJSDocLinkPrefix);
//                    if (!linkType) {
//                        return undefined;
//                    }
//                    nextTokenJSDoc(); // start at token after link, then skip any whitespace
//                    skipWhitespace();
//                    // parseEntityName logs an error for non-identifier, so create a MissingNode ourselves to avoid the error
//                    auto p2 = getNodePos();
//                    auto name: EntityName | JSDocMemberName | undefined = tokenIsIdentifierOrKeyword(token())
//                        ? parseEntityName(/*allowReservedWords*/ true)
//                        : undefined;
//                    if (name) {
//                        while (token() == SyntaxKind::PrivateIdentifier) {
//                            reScanHashToken(); // rescan #id as # id
//                            nextTokenJSDoc(); // then skip the #
//                            name = finishNode(factory.createJSDocMemberName(name, parseIdentifier()), p2);
//                        }
//                    }
//                    auto text = [];
//                    while (token() != SyntaxKind::CloseBraceToken && token() != SyntaxKind::NewLineTrivia && token() != SyntaxKind::EndOfFileToken) {
//                        text.push(scanner.getTokenText());
//                        nextTokenJSDoc();
//                    }
//                    auto create = linkType == "link" ? factory.createJSDocLink
//                        : linkType == "linkcode" ? factory.createJSDocLinkCode
//                        : factory.createJSDocLinkPlain;
//                    return finishNode(create(name, text.join("")), start, scanner.getTextPos());
//                }
//
//                function parseJSDocLinkPrefix() {
//                    skipWhitespaceOrAsterisk();
//                    if (token() == SyntaxKind::OpenBraceToken
//                        && nextTokenJSDoc() == SyntaxKind::AtToken
//                        && tokenIsIdentifierOrKeyword(nextTokenJSDoc())) {
//                        auto kind = scanner.getTokenValue();
//                        if (isJSDocLinkTag(kind)) return kind;
//                    }
//                }
//
//                function isJSDocLinkTag(kind: string) {
//                    return kind == "link" || kind == "linkcode" || kind == "linkplain";
//                }
//
//                function parseUnknownTag(start: number, tagName: Identifier, indent: number, indentText: string) {
//                    return finishNode(factory.createJSDocUnknownTag(tagName, parseTrailingTagComments(start, getNodePos(), indent, indentText)), start);
//                }
//
//                function addTag(tag: JSDocTag | undefined): void {
//                    if (!tag) {
//                        return;
//                    }
//                    if (!tags) {
//                        tags = [tag];
//                        tagsPos = tag.pos;
//                    }
//                    else {
//                        tags.push(tag);
//                    }
//                    tagsEnd = tag.end;
//                }
//
//                function tryParseTypeExpression(): JSDocTypeExpression | undefined {
//                    skipWhitespaceOrAsterisk();
//                    return token() == SyntaxKind::OpenBraceToken ? parseJSDocTypeExpression() : undefined;
//                }
//
//                function parseBracketNameInPropertyAndParamTag(): { name: EntityName, isBracketed: boolean } {
//                    // Looking for something like '[foo]', 'foo', '[foo.bar]' or 'foo.bar'
//                    auto isBracketed = parseOptionalJsdoc(SyntaxKind::OpenBracketToken);
//                    if (isBracketed) {
//                        skipWhitespace();
//                    }
//                    // a markdown-quoted name: `arg` is not legal jsdoc, but occurs in the wild
//                    auto isBackquoted = parseOptionalJsdoc(SyntaxKind::BacktickToken);
//                    auto name = parseJSDocEntityName();
//                    if (isBackquoted) {
//                        parseExpectedTokenJSDoc(SyntaxKind::BacktickToken);
//                    }
//                    if (isBracketed) {
//                        skipWhitespace();
//                        // May have an optional default, e.g. '[foo = 42]'
//                        if (parseOptionalToken<EqualsToken>(SyntaxKind::EqualsToken)) {
//                            parseExpression();
//                        }
//
//                        parseExpected(SyntaxKind::CloseBracketToken);
//                    }
//
//                    return { name, isBracketed };
//                }
//
//                function isObjectOrObjectArrayTypeReference(node: TypeNode): boolean {
//                    switch (node.kind) {
//                        case SyntaxKind::ObjectKeyword:
//                            return true;
//                        case SyntaxKind::ArrayType:
//                            return isObjectOrObjectArrayTypeReference(reinterpret_cast<ArrayTypeNode&>(node).elementType);
//                        default:
//                            return isTypeReferenceNode(node) && ts.isIdentifier(node.typeName) && node.typeName.escapedText == "Object" && !node.typeArguments;
//                    }
//                }
//
//                function parseParameterOrPropertyTag(start: number, tagName: Identifier, target: PropertyLikeParse, indent: number): JSDocParameterTag | JSDocPropertyTag {
//                    auto typeExpression = tryParseTypeExpression();
//                    auto isNameFirst = !typeExpression;
//                    skipWhitespaceOrAsterisk();
//
//                    auto { name, isBracketed } = parseBracketNameInPropertyAndParamTag();
//                    auto indentText = skipWhitespaceOrAsterisk();
//
//                    if (isNameFirst && !lookAhead(parseJSDocLinkPrefix)) {
//                        typeExpression = tryParseTypeExpression();
//                    }
//
//                    auto comment = parseTrailingTagComments(start, getNodePos(), indent, indentText);
//
//                    auto nestedTypeLiteral = target != PropertyLikeParse.CallbackParameter && parseNestedTypeLiteral(typeExpression, name, target, indent);
//                    if (nestedTypeLiteral) {
//                        typeExpression = nestedTypeLiteral;
//                        isNameFirst = true;
//                    }
//                    auto result = target == PropertyLikeParse.Property
//                        ? factory.createJSDocPropertyTag(tagName, name, isBracketed, typeExpression, isNameFirst, comment)
//                        : factory.createJSDocParameterTag(tagName, name, isBracketed, typeExpression, isNameFirst, comment);
//                    return finishNode(result, start);
//                }
//
//                function parseNestedTypeLiteral(typeExpression: JSDocTypeExpression | undefined, name: EntityName, target: PropertyLikeParse, indent: number) {
//                    if (typeExpression && isObjectOrObjectArrayTypeReference(typeExpression.type)) {
//                        auto pos = getNodePos();
//                        auto child: JSDocPropertyLikeTag | JSDocTypeTag | false;
//                        auto children: JSDocPropertyLikeTag[] | undefined;
//                        while (child = tryParse(() => parseChildParameterOrPropertyTag(target, indent, name))) {
//                            if (child.kind == SyntaxKind::JSDocParameterTag || child.kind == SyntaxKind::JSDocPropertyTag) {
//                                children = append(children, child);
//                            }
//                        }
//                        if (children) {
//                            auto literal = finishNode(factory.createJSDocTypeLiteral(children, typeExpression.type.kind == SyntaxKind::ArrayType), pos);
//                            return finishNode(factory.createJSDocTypeExpression(literal), pos);
//                        }
//                    }
//                }
//
//                function parseReturnTag(start: number, tagName: Identifier, indent: number, indentText: string): JSDocReturnTag {
//                    if (some(tags, isJSDocReturnTag)) {
//                        parseErrorAt(tagName.pos, scanner.getTokenPos(), Diagnostics::_0_tag_already_specified, tagName.escapedText);
//                    }
//
//                    auto typeExpression = tryParseTypeExpression();
//                    return finishNode(factory.createJSDocReturnTag(tagName, typeExpression, parseTrailingTagComments(start, getNodePos(), indent, indentText)), start);
//                }
//
//                function parseTypeTag(start: number, tagName: Identifier, indent?: number, indentText?: string): JSDocTypeTag {
//                    if (some(tags, isJSDocTypeTag)) {
//                        parseErrorAt(tagName.pos, scanner.getTokenPos(), Diagnostics::_0_tag_already_specified, tagName.escapedText);
//                    }
//
//                    auto typeExpression = parseJSDocTypeExpression(/*mayOmitBraces*/ true);
//                    auto comments = indent != undefined && indentText != undefined ? parseTrailingTagComments(start, getNodePos(), indent, indentText) : undefined;
//                    return finishNode(factory.createJSDocTypeTag(tagName, typeExpression, comments), start);
//                }
//
//                function parseSeeTag(start: number, tagName: Identifier, indent?: number, indentText?: string): JSDocSeeTag {
//                    auto isMarkdownOrJSDocLink = token() == SyntaxKind::OpenBracketToken
//                        || lookAhead(() => nextTokenJSDoc() == SyntaxKind::AtToken && tokenIsIdentifierOrKeyword(nextTokenJSDoc()) && isJSDocLinkTag(scanner.getTokenValue()));
//                    auto nameExpression = isMarkdownOrJSDocLink ? undefined : parseJSDocNameReference();
//                    auto comments = indent != undefined && indentText != undefined ? parseTrailingTagComments(start, getNodePos(), indent, indentText) : undefined;
//                    return finishNode(factory.createJSDocSeeTag(tagName, nameExpression, comments), start);
//                }
//
//                function parseAuthorTag(start: number, tagName: Identifier, indent: number, indentText: string): JSDocAuthorTag {
//                    auto commentStart = getNodePos();
//                    auto textOnly = parseAuthorNameAndEmail();
//                    auto commentEnd = scanner.getStartPos();
//                    auto comments = parseTrailingTagComments(start, commentEnd, indent, indentText);
//                    if (!comments) {
//                        commentEnd = scanner.getStartPos();
//                    }
//                    auto allParts = typeof comments != "string"
//                        ? createNodeArray(concatenate([finishNode(textOnly, commentStart, commentEnd)], comments) as JSDocComment[], commentStart) // cast away readonly
//                        : textOnly.text + comments;
//                    return finishNode(factory.createJSDocAuthorTag(tagName, allParts), start);
//                }
//
//                function parseAuthorNameAndEmail(): JSDocText {
//                    auto comments: string[] = [];
//                    auto inEmail = false;
//                    auto token = scanner.getToken();
//                    while (token != SyntaxKind::EndOfFileToken && token != SyntaxKind::NewLineTrivia) {
//                        if (token == SyntaxKind::LessThanToken) {
//                            inEmail = true;
//                        }
//                        else if (token == SyntaxKind::AtToken && !inEmail) {
//                            break;
//                        }
//                        else if (token == SyntaxKind::GreaterThanToken && inEmail) {
//                            comments.push(scanner.getTokenText());
//                            scanner.setTextPos(scanner.getTokenPos() + 1);
//                            break;
//                        }
//                        comments.push(scanner.getTokenText());
//                        token = nextTokenJSDoc();
//                    }
//
//                    return factory.createJSDocText(comments.join(""));
//                }
//
//                function parseImplementsTag(start: number, tagName: Identifier, margin: number, indentText: string): JSDocImplementsTag {
//                    auto className = parseExpressionWithTypeArgumentsForAugments();
//                    return finishNode(factory.createJSDocImplementsTag(tagName, className, parseTrailingTagComments(start, getNodePos(), margin, indentText)), start);
//                }
//
//                function parseAugmentsTag(start: number, tagName: Identifier, margin: number, indentText: string): JSDocAugmentsTag {
//                    auto className = parseExpressionWithTypeArgumentsForAugments();
//                    return finishNode(factory.createJSDocAugmentsTag(tagName, className, parseTrailingTagComments(start, getNodePos(), margin, indentText)), start);
//                }
//
//                function parseExpressionWithTypeArgumentsForAugments(): ExpressionWithTypeArguments & { expression: Identifier | PropertyAccessEntityNameExpression } {
//                    auto usedBrace = parseOptional(SyntaxKind::OpenBraceToken);
//                    auto pos = getNodePos();
//                    auto expression = parsePropertyAccessEntityNameExpression();
//                    auto typeArguments = tryParseTypeArguments();
//                    auto node = factory.createExpressionWithTypeArguments(expression, typeArguments) as ExpressionWithTypeArguments & { expression: Identifier | PropertyAccessEntityNameExpression };
//                    auto res = finishNode(node, pos);
//                    if (usedBrace) {
//                        parseExpected(SyntaxKind::CloseBraceToken);
//                    }
//                    return res;
//                }
//
//                function parsePropertyAccessEntityNameExpression() {
//                    auto pos = getNodePos();
//                    auto node: Identifier | PropertyAccessEntityNameExpression = parseJSDocIdentifierName();
//                    while (parseOptional(SyntaxKind::DotToken)) {
//                        auto name = parseJSDocIdentifierName();
//                        node = finishNode(factory.createPropertyAccessExpression(node, name), pos) as PropertyAccessEntityNameExpression;
//                    }
//                    return node;
//                }
//
//                function parseSimpleTag(start: number, createTag: (tagName: Identifier | undefined, comment?: string | NodeArray<JSDocComment>) => JSDocTag, tagName: Identifier, margin: number, indentText: string): JSDocTag {
//                    return finishNode(createTag(tagName, parseTrailingTagComments(start, getNodePos(), margin, indentText)), start);
//                }
//
//                function parseThisTag(start: number, tagName: Identifier, margin: number, indentText: string): JSDocThisTag {
//                    auto typeExpression = parseJSDocTypeExpression(/*mayOmitBraces*/ true);
//                    skipWhitespace();
//                    return finishNode(factory.createJSDocThisTag(tagName, typeExpression, parseTrailingTagComments(start, getNodePos(), margin, indentText)), start);
//                }
//
//                function parseEnumTag(start: number, tagName: Identifier, margin: number, indentText: string): JSDocEnumTag {
//                    auto typeExpression = parseJSDocTypeExpression(/*mayOmitBraces*/ true);
//                    skipWhitespace();
//                    return finishNode(factory.createJSDocEnumTag(tagName, typeExpression, parseTrailingTagComments(start, getNodePos(), margin, indentText)), start);
//                }
//
//                function parseTypedefTag(start: number, tagName: Identifier, indent: number, indentText: string): JSDocTypedefTag {
//                    auto typeExpression: JSDocTypeExpression | JSDocTypeLiteral | undefined = tryParseTypeExpression();
//                    skipWhitespaceOrAsterisk();
//
//                    auto fullName = parseJSDocTypeNameWithNamespace();
//                    skipWhitespace();
//                    auto comment = parseTagComments(indent);
//
//                    auto end: number | undefined;
//                    if (!typeExpression || isObjectOrObjectArrayTypeReference(typeExpression.type)) {
//                        auto child: JSDocTypeTag | JSDocPropertyTag | false;
//                        auto childTypeTag: JSDocTypeTag | undefined;
//                        auto jsDocPropertyTags: JSDocPropertyTag[] | undefined;
//                        auto hasChildren = false;
//                        while (child = tryParse(() => parseChildPropertyTag(indent))) {
//                            hasChildren = true;
//                            if (child.kind == SyntaxKind::JSDocTypeTag) {
//                                if (childTypeTag) {
//                                    auto lastError = parseErrorAtCurrentToken(Diagnostics::A_JSDoc_typedef_comment_may_not_contain_multiple_type_tags);
//                                    if (lastError) {
//                                        addRelatedInfo(lastError, createDetachedDiagnostic(fileName, 0, 0, Diagnostics::The_tag_was_first_specified_here));
//                                    }
//                                    break;
//                                }
//                                else {
//                                    childTypeTag = child;
//                                }
//                            }
//                            else {
//                                jsDocPropertyTags = append(jsDocPropertyTags, child);
//                            }
//                        }
//                        if (hasChildren) {
//                            auto isArrayType = typeExpression && typeExpression.type.kind == SyntaxKind::ArrayType;
//                            auto jsdocTypeLiteral = factory.createJSDocTypeLiteral(jsDocPropertyTags, isArrayType);
//                            typeExpression = childTypeTag && childTypeTag.typeExpression && !isObjectOrObjectArrayTypeReference(childTypeTag.typeExpression.type) ?
//                                childTypeTag.typeExpression :
//                                finishNode(jsdocTypeLiteral, start);
//                            end = typeExpression.end;
//                        }
//                    }
//
//                    // Only include the characters between the name end and the next token if a comment was actually parsed out - otherwise it's just whitespace
//                    end = end || comment != undefined ?
//                        getNodePos() :
//                        (fullName ?? typeExpression ?? tagName).end;
//
//                    if (!comment) {
//                        comment = parseTrailingTagComments(start, end, indent, indentText);
//                    }
//
//                    auto typedefTag = factory.createJSDocTypedefTag(tagName, typeExpression, fullName, comment);
//                    return finishNode(typedefTag, start, end);
//                }
//
//                function parseJSDocTypeNameWithNamespace(nested?: boolean) {
//                    auto pos = scanner.getTokenPos();
//                    if (!tokenIsIdentifierOrKeyword(token())) {
//                        return undefined;
//                    }
//                    auto typeNameOrNamespaceName = parseJSDocIdentifierName();
//                    if (parseOptional(SyntaxKind::DotToken)) {
//                        auto body = parseJSDocTypeNameWithNamespace(/*nested*/ true);
//                        auto jsDocNamespaceNode = factory.createModuleDeclaration(
//                            /*decorators*/ {},
//                            /*modifiers*/ {},
//                            typeNameOrNamespaceName,
//                            body,
//                            nested ? NodeFlags::NestedNamespace : undefined
//                        ) as JSDocNamespaceDeclaration;
//                        return finishNode(jsDocNamespaceNode, pos);
//                    }
//
//                    if (nested) {
//                        typeNameOrNamespaceName.isInJSDocNamespace = true;
//                    }
//                    return typeNameOrNamespaceName;
//                }
//
//
//                function parseCallbackTagParameters(indent: number) {
//                    auto pos = getNodePos();
//                    auto child: JSDocParameterTag | false;
//                    auto parameters;
//                    while (child = tryParse(() => parseChildParameterOrPropertyTag(PropertyLikeParse.CallbackParameter, indent) as JSDocParameterTag)) {
//                        parameters = append(parameters, child);
//                    }
//                    return createNodeArray(parameters || [], pos);
//                }
//
//                function parseCallbackTag(start: number, tagName: Identifier, indent: number, indentText: string): JSDocCallbackTag {
//                    auto fullName = parseJSDocTypeNameWithNamespace();
//                    skipWhitespace();
//                    auto comment = parseTagComments(indent);
//                    auto parameters = parseCallbackTagParameters(indent);
//                    auto returnTag = tryParse(() => {
//                        if (parseOptionalJsdoc(SyntaxKind::AtToken)) {
//                            auto tag = parseTag(indent);
//                            if (tag && tag.kind == SyntaxKind::JSDocReturnTag) {
//                                return tag as JSDocReturnTag;
//                            }
//                        }
//                    });
//                    auto typeExpression = finishNode(factory.createJSDocSignature(/*typeParameters*/ undefined, parameters, returnTag), start);
//                    if (!comment) {
//                        comment = parseTrailingTagComments(start, getNodePos(), indent, indentText);
//                    }
//                    auto end = comment != undefined ? getNodePos() : typeExpression.end;
//                    return finishNode(factory.createJSDocCallbackTag(tagName, typeExpression, fullName, comment), start, end);
//                }
//
//                function escapedTextsEqual(a: EntityName, b: EntityName): boolean {
//                    while (!ts.isIdentifier(a) || !ts.isIdentifier(b)) {
//                        if (!ts.isIdentifier(a) && !ts.isIdentifier(b) && a.right.escapedText == b.right.escapedText) {
//                            a = a.left;
//                            b = b.left;
//                        }
//                        else {
//                            return false;
//                        }
//                    }
//                    return a.escapedText == b.escapedText;
//                }
//
//                function parseChildPropertyTag(indent: number) {
//                    return parseChildParameterOrPropertyTag(PropertyLikeParse.Property, indent) as JSDocTypeTag | JSDocPropertyTag | false;
//                }
//
//                function parseChildParameterOrPropertyTag(target: PropertyLikeParse, indent: number, name?: EntityName): JSDocTypeTag | JSDocPropertyTag | JSDocParameterTag | false {
//                    auto canParseTag = true;
//                    auto seenAsterisk = false;
//                    while (true) {
//                        switch (nextTokenJSDoc()) {
//                            case SyntaxKind::AtToken:
//                                if (canParseTag) {
//                                    auto child = tryParseChildTag(target, indent);
//                                    if (child && (child.kind == SyntaxKind::JSDocParameterTag || child.kind == SyntaxKind::JSDocPropertyTag) &&
//                                        target != PropertyLikeParse.CallbackParameter &&
//                                        name && (ts.isIdentifier(child.name) || !escapedTextsEqual(name, child.name.left))) {
//                                        return false;
//                                    }
//                                    return child;
//                                }
//                                seenAsterisk = false;
//                                break;
//                            case SyntaxKind::NewLineTrivia:
//                                canParseTag = true;
//                                seenAsterisk = false;
//                                break;
//                            case SyntaxKind::AsteriskToken:
//                                if (seenAsterisk) {
//                                    canParseTag = false;
//                                }
//                                seenAsterisk = true;
//                                break;
//                            case SyntaxKind::Identifier:
//                                canParseTag = false;
//                                break;
//                            case SyntaxKind::EndOfFileToken:
//                                return false;
//                        }
//                    }
//                }
//
//                function tryParseChildTag(target: PropertyLikeParse, indent: number): JSDocTypeTag | JSDocPropertyTag | JSDocParameterTag | false {
//                    Debug::asserts(token() == SyntaxKind::AtToken);
//                    auto start = scanner.getStartPos();
//                    nextTokenJSDoc();
//
//                    auto tagName = parseJSDocIdentifierName();
//                    skipWhitespace();
//                    auto t: PropertyLikeParse;
//                    switch (tagName.escapedText) {
//                        case "type":
//                            return target == PropertyLikeParse.Property && parseTypeTag(start, tagName);
//                        case "prop":
//                        case "property":
//                            t = PropertyLikeParse.Property;
//                            break;
//                        case "arg":
//                        case "argument":
//                        case "param":
//                            t = PropertyLikeParse.Parameter | PropertyLikeParse.CallbackParameter;
//                            break;
//                        default:
//                            return false;
//                    }
//                    if (!(target & t)) {
//                        return false;
//                    }
//                    return parseParameterOrPropertyTag(start, tagName, target, indent);
//                }
//
//                function parseTemplateTagTypeParameter() {
//                    auto typeParameterPos = getNodePos();
//                    auto isBracketed = parseOptionalJsdoc(SyntaxKind::OpenBracketToken);
//                    if (isBracketed) {
//                        skipWhitespace();
//                    }
//                    auto name = parseJSDocIdentifierName(Diagnostics::Unexpected_token_A_type_parameter_name_was_expected_without_curly_braces);
//
//                    auto defaultType: TypeNode | undefined;
//                    if (isBracketed) {
//                        skipWhitespace();
//                        parseExpected(SyntaxKind::EqualsToken);
//                        defaultType = doInsideOfContext(NodeFlags::JSDoc, parseJSDocType);
//                        parseExpected(SyntaxKind::CloseBracketToken);
//                    }
//
//                    if (nodeIsMissing(name)) {
//                        return undefined;
//                    }
//                    return finishNode(factory.createTypeParameterDeclaration(/*modifiers*/ {}, name, /*constraint*/ undefined, defaultType), typeParameterPos);
//                }
//
//                function parseTemplateTagTypeParameters() {
//                    auto pos = getNodePos();
//                    auto typeParameters = [];
//                    do {
//                        skipWhitespace();
//                        auto node = parseTemplateTagTypeParameter();
//                        if (node != undefined) {
//                            typeParameters.push(node);
//                        }
//                        skipWhitespaceOrAsterisk();
//                    } while (parseOptionalJsdoc(SyntaxKind::CommaToken));
//                    return createNodeArray(typeParameters, pos);
//                }
//
//                function parseTemplateTag(start: number, tagName: Identifier, indent: number, indentText: string): JSDocTemplateTag {
//                    // The template tag looks like one of the following:
//                    //   @template T,U,V
//                    //   @template {Constraint} T
//                    //
//                    // According to the [closure docs](https://github.com/google/closure-compiler/wiki/Generic-Types#multiple-bounded-template-types):
//                    //   > Multiple bounded generics cannot be declared on the same line. For the sake of clarity, if multiple templates share the same
//                    //   > type bound they must be declared on separate lines.
//                    //
//                    // TODO: Determine whether we should enforce this in the checker.
//                    // TODO: Consider moving the `constraint` to the first type parameter as we could then remove `getEffectiveConstraintOfTypeParameter`.
//                    // TODO: Consider only parsing a single type parameter if there is a constraint.
//                    auto constraint = token() == SyntaxKind::OpenBraceToken ? parseJSDocTypeExpression() : undefined;
//                    auto typeParameters = parseTemplateTagTypeParameters();
//                    return finishNode(factory.createJSDocTemplateTag(tagName, constraint, typeParameters, parseTrailingTagComments(start, getNodePos(), indent, indentText)), start);
//                }
//
//                function parseOptionalJsdoc(t: JSDocSyntaxKind): boolean {
//                    if (token() == t) {
//                        nextTokenJSDoc();
//                        return true;
//                    }
//                    return false;
//                }
//
//                function parseJSDocEntityName(): EntityName {
//                    auto entity: EntityName = parseJSDocIdentifierName();
//                    if (parseOptional(SyntaxKind::OpenBracketToken)) {
//                        parseExpected(SyntaxKind::CloseBracketToken);
//                        // Note that y[] is accepted as an entity name, but the postfix brackets are not saved for checking.
//                        // Technically usejsdoc.org requires them for specifying a property of a type equivalent to Array<{ x: ...}>
//                        // but it's not worth it to enforce that restriction.
//                    }
//                    while (parseOptional(SyntaxKind::DotToken)) {
//                        auto name = parseJSDocIdentifierName();
//                        if (parseOptional(SyntaxKind::OpenBracketToken)) {
//                            parseExpected(SyntaxKind::CloseBracketToken);
//                        }
//                        entity = createQualifiedName(entity, name);
//                    }
//                    return entity;
//                }
//
//                function parseJSDocIdentifierName(message?: DiagnosticMessage): Identifier {
//                    if (!tokenIsIdentifierOrKeyword(token())) {
//                        return createMissingNode<Identifier>(SyntaxKind::Identifier, /*reportAtCurrentPosition*/ !message, message || Diagnostics::Identifier_expected);
//                    }
//
//                    identifierCount++;
//                    auto pos = scanner.getTokenPos();
//                    auto end = scanner.getTextPos();
//                    auto originalKeywordKind = token();
//                    auto text = internIdentifier(scanner.getTokenValue());
//                    auto result = finishNode(factory.createIdentifier(text, /*typeArguments*/ undefined, originalKeywordKind), pos, end);
//                    nextTokenJSDoc();
//                    return result;
//                }
//            }
//        }

        shared<SourceFile> parseSourceFile(const string &fileName, const string &sourceText, ScriptTarget languageVersion, bool setParentNodes, optional<ScriptKind> _scriptKind, optional<function<void(shared<SourceFile>)>> setExternalModuleIndicatorOverride) {
            ZoneScoped;
            auto scriptKind = ensureScriptKind(fileName, _scriptKind);

            //            if (scriptKind == ScriptKind.JSON) {
            //                auto result = parseJsonText(fileName, sourceText, languageVersion, syntaxCursor, setParentNodes);
            //                convertToObjectWorker(result, result.statements[0]?.expression, result.parseDiagnostics, /*returnValue*/ false, /*knownRootOptions*/ undefined, /*jsonConversionNotifier*/ undefined);
            //                result.referencedFiles = emptyArray;
            //                result.typeReferenceDirectives = emptyArray;
            //                result.libReferenceDirectives = emptyArray;
            //                result.amdDependencies = emptyArray;
            //                result.hasNoDefaultLib = false;
            //                result.pragmas = emptyMap as ReadonlyPragmaMap;
            //                return result;
            //            }

            initializeState(fileName, sourceText, languageVersion, scriptKind);

            auto result = parseSourceFileWorker(languageVersion, setParentNodes, scriptKind, setExternalModuleIndicatorOverride ? *setExternalModuleIndicatorOverride : setExternalModuleIndicator);

            clearState();

            return result;
        }
    };
//
//        export function parseIsolatedEntityName(content: string, languageVersion: ScriptTarget): EntityName | undefined {
//            // Choice of `isDeclarationFile` should be arbitrary
//            initializeState("", content, languageVersion, /*syntaxCursor*/ undefined, ScriptKind.JS);
//            // Prime the scanner.
//            nextToken();
//            auto entityName = parseEntityName(/*allowReservedWords*/ true);
//            auto isInvalid = token() == SyntaxKind::EndOfFileToken && !parseDiagnostics.length;
//            clearState();
//            return isInvalid ? entityName : undefined;
//        }
//
//        export function parseJsonText(fileName: string, sourceText: string, languageVersion: ScriptTarget = ScriptTarget.ES2015, syntaxCursor?: IncrementalParser.SyntaxCursor, setParentNodes = false): JsonSourceFile {
//            initializeState(fileName, sourceText, languageVersion, syntaxCursor, ScriptKind.JSON);
//            sourceFlags = contextFlags;
//
//            // Prime the scanner.
//            nextToken();
//            auto pos = getNodePos();
//            auto statements, endOfFileToken;
//            if (token() == SyntaxKind::EndOfFileToken) {
//                statements = createNodeArray([], pos, pos);
//                endOfFileToken = parseTokenNode<EndOfFileToken>();
//            }
//            else {
//                // Loop and synthesize an ArrayLiteralExpression if there are more than
//                // one top-level expressions to ensure all input text is consumed.
//                auto expressions: Expression[] | Expression | undefined;
//                while (token() != SyntaxKind::EndOfFileToken) {
//                    auto expression;
//                    switch (token()) {
//                        case SyntaxKind::OpenBracketToken:
//                            expression = parseArrayLiteralExpression();
//                            break;
//                        case SyntaxKind::TrueKeyword:
//                        case SyntaxKind::FalseKeyword:
//                        case SyntaxKind::NullKeyword:
//                            expression = parseTokenNode<BooleanLiteral | NullLiteral>();
//                            break;
//                        case SyntaxKind::MinusToken:
//                            if (lookAhead(() => nextToken() == SyntaxKind::NumericLiteral && nextToken() != SyntaxKind::ColonToken)) {
//                                expression = parsePrefixUnaryExpression() as JsonMinusNumericLiteral;
//                            }
//                            else {
//                                expression = parseObjectLiteralExpression();
//                            }
//                            break;
//                        case SyntaxKind::NumericLiteral:
//                        case SyntaxKind::StringLiteral:
//                            if (lookAhead(() => nextToken() != SyntaxKind::ColonToken)) {
//                                expression = parseLiteralNode() as StringLiteral | NumericLiteral;
//                                break;
//                            }
//                            // falls through
//                        default:
//                            expression = parseObjectLiteralExpression();
//                            break;
//                    }
//
//                    // Error recovery: collect multiple top-level expressions
//                    if (expressions && isArray(expressions)) {
//                        expressions.push(expression);
//                    }
//                    else if (expressions) {
//                        expressions = [expressions, expression];
//                    }
//                    else {
//                        expressions = expression;
//                        if (token() != SyntaxKind::EndOfFileToken) {
//                            parseErrorAtCurrentToken(Diagnostics::Unexpected_token);
//                        }
//                    }
//                }
//
//                auto expression = isArray(expressions) ? finishNode(factory.createArrayLiteralExpression(expressions), pos) : Debug.checkDefined(expressions);
//                auto statement = factory.createExpressionStatement(expression) as JsonObjectExpressionStatement;
//                finishNode(statement, pos);
//                statements = createNodeArray([statement], pos);
//                endOfFileToken = parseExpectedToken(SyntaxKind::EndOfFileToken, Diagnostics::Unexpected_token);
//            }
//
//            // Set source file so that errors will be reported with this file name
//            auto sourceFile = createSourceFile(fileName, ScriptTarget.ES2015, ScriptKind.JSON, /*isDeclaration*/ false, statements, endOfFileToken, sourceFlags, noop);
//
//            if (setParentNodes) {
//                fixupParentReferences(sourceFile);
//            }
//
//            sourceFile.nodeCount = nodeCount;
//            sourceFile.identifierCount = identifierCount;
//            sourceFile.identifiers = identifiers;
//            sourceFile.parseDiagnostics = attachFileToDiagnostics(parseDiagnostics, sourceFile);
//            if (jsDocDiagnostics) {
//                sourceFile.jsDocDiagnostics = attachFileToDiagnostics(jsDocDiagnostics, sourceFile);
//            }
//
//            auto result = sourceFile as JsonSourceFile;
//            clearState();
//            return result;
//        }
//
//
//    namespace IncrementalParser {
//        export function updateSourceFile(sourceFile: SourceFile, newText: string, textChangeRange: TextChangeRange, aggressiveChecks: boolean): SourceFile {
//            aggressiveChecks = aggressiveChecks || Debug.shouldAssert(AssertionLevel.Aggressive);
//
//            checkChangeRange(sourceFile, newText, textChangeRange, aggressiveChecks);
//            if (textChangeRangeIsUnchanged(textChangeRange)) {
//                // if the text didn't change, then we can just return our current source file as-is.
//                return sourceFile;
//            }
//
//            if (sourceFile.statements.length == 0) {
//                // If we don't have any statements in the current source file, then there's no real
//                // way to incrementally parse.  So just do a full parse instead.
//                return Parser.parseSourceFile(sourceFile.fileName, newText, sourceFile.languageVersion, /*syntaxCursor*/ undefined, /*setParentNodes*/ true, sourceFile.scriptKind, sourceFile.setExternalModuleIndicator);
//            }
//
//            // Make sure we're not trying to incrementally update a source file more than once.  Once
//            // we do an update the original source file is considered unusable from that point onwards.
//            //
//            // This is because we do incremental parsing in-place.  i.e. we take nodes from the old
//            // tree and give them new positions and parents.  From that point on, trusting the old
//            // tree at all is not possible as far too much of it may violate invariants.
//            auto incrementalSourceFile = sourceFile as Node as IncrementalNode;
//            Debug::asserts(!incrementalSourceFile.hasBeenIncrementallyParsed);
//            incrementalSourceFile.hasBeenIncrementallyParsed = true;
//            Parser.fixupParentReferences(incrementalSourceFile);
//            auto oldText = sourceFile.text;
//            auto syntaxCursor = createSyntaxCursor(sourceFile);
//
//            // Make the actual change larger so that we know to reparse anything whose lookahead
//            // might have intersected the change.
//            auto changeRange = extendToAffectedRange(sourceFile, textChangeRange);
//            checkChangeRange(sourceFile, newText, changeRange, aggressiveChecks);
//
//            // Ensure that extending the affected range only moved the start of the change range
//            // earlier in the file.
//            Debug::asserts(changeRange.span.start <= textChangeRange.span.start);
//            Debug::asserts(textSpanEnd(changeRange.span) == textSpanEnd(textChangeRange.span));
//            Debug::asserts(textSpanEnd(textChangeRangeNewSpan(changeRange)) == textSpanEnd(textChangeRangeNewSpan(textChangeRange)));
//
//            // The is the amount the nodes after the edit range need to be adjusted.  It can be
//            // positive (if the edit added characters), negative (if the edit deleted characters)
//            // or zero (if this was a pure overwrite with nothing added/removed).
//            auto delta = textChangeRangeNewSpan(changeRange).length - changeRange.span.length;
//
//            // If we added or removed characters during the edit, then we need to go and adjust all
//            // the nodes after the edit.  Those nodes may move forward (if we inserted chars) or they
//            // may move backward (if we deleted chars).
//            //
//            // Doing this helps us out in two ways.  First, it means that any nodes/tokens we want
//            // to reuse are already at the appropriate position in the new text.  That way when we
//            // reuse them, we don't have to figure out if they need to be adjusted.  Second, it makes
//            // it very easy to determine if we can reuse a node.  If the node's position is at where
//            // we are in the text, then we can reuse it.  Otherwise we can't.  If the node's position
//            // is ahead of us, then we'll need to rescan tokens.  If the node's position is behind
//            // us, then we'll need to skip it or crumble it as appropriate
//            //
//            // We will also adjust the positions of nodes that intersect the change range as well.
//            // By doing this, we ensure that all the positions in the old tree are consistent, not
//            // just the positions of nodes entirely before/after the change range.  By being
//            // consistent, we can then easily map from positions to nodes in the old tree easily.
//            //
//            // Also, mark any syntax elements that intersect the changed span.  We know, up front,
//            // that we cannot reuse these elements.
//            updateTokenPositionsAndMarkElements(incrementalSourceFile,
//                changeRange.span.start, textSpanEnd(changeRange.span), textSpanEnd(textChangeRangeNewSpan(changeRange)), delta, oldText, newText, aggressiveChecks);
//
//            // Now that we've set up our internal incremental state just proceed and parse the
//            // source file in the normal fashion.  When possible the parser will retrieve and
//            // reuse nodes from the old tree.
//            //
//            // Note: passing in 'true' for setNodeParents is very important.  When incrementally
//            // parsing, we will be reusing nodes from the old tree, and placing it into new
//            // parents.  If we don't set the parents now, we'll end up with an observably
//            // inconsistent tree.  Setting the parents on the new tree should be very fast.  We
//            // will immediately bail out of walking any subtrees when we can see that their parents
//            // are already correct.
//            auto result = Parser.parseSourceFile(sourceFile.fileName, newText, sourceFile.languageVersion, syntaxCursor, /*setParentNodes*/ true, sourceFile.scriptKind, sourceFile.setExternalModuleIndicator);
//            result.commentDirectives = getNewCommentDirectives(
//                sourceFile.commentDirectives,
//                result.commentDirectives,
//                changeRange.span.start,
//                textSpanEnd(changeRange.span),
//                delta,
//                oldText,
//                newText,
//                aggressiveChecks
//            );
//            result.impliedNodeFormat = sourceFile.impliedNodeFormat;
//            return result;
//        }
//
//        function getNewCommentDirectives(
//            oldDirectives: CommentDirective[] | undefined,
//            newDirectives: CommentDirective[] | undefined,
//            changeStart: number,
//            changeRangeOldEnd: number,
//            delta: number,
//            oldText: string,
//            newText: string,
//            aggressiveChecks: boolean
//        ): CommentDirective[] | undefined {
//            if (!oldDirectives) return newDirectives;
//            auto commentDirectives: CommentDirective[] | undefined;
//            auto addedNewlyScannedDirectives = false;
//            for (const directive of oldDirectives) {
//                auto { range, type } = directive;
//                // Range before the change
//                if (range.end < changeStart) {
//                    commentDirectives = append(commentDirectives, directive);
//                }
//                else if (range.pos > changeRangeOldEnd) {
//                    addNewlyScannedDirectives();
//                    // Node is entirely past the change range.  We need to move both its pos and
//                    // end, forward or backward appropriately.
//                    auto updatedDirective: CommentDirective = {
//                        range: { pos: range.pos + delta, end: range.end + delta },
//                        type
//                    };
//                    commentDirectives = append(commentDirectives, updatedDirective);
//                    if (aggressiveChecks) {
//                        Debug::asserts(oldText.substring(range.pos, range.end) == newText.substring(updatedDirective.range.pos, updatedDirective.range.end));
//                    }
//                }
//                // Ignore ranges that fall in change range
//            }
//            addNewlyScannedDirectives();
//            return commentDirectives;
//
//            function addNewlyScannedDirectives() {
//                if (addedNewlyScannedDirectives) return;
//                addedNewlyScannedDirectives = true;
//                if (!commentDirectives) {
//                    commentDirectives = newDirectives;
//                }
//                else if (newDirectives) {
//                    commentDirectives.push(...newDirectives);
//                }
//            }
//        }
//
//        function moveElementEntirelyPastChangeRange(element: IncrementalElement, isArray: boolean, delta: number, oldText: string, newText: string, aggressiveChecks: boolean) {
//            if (isArray) {
//                visitArray(element as IncrementalNodeArray);
//            }
//            else {
//                visitNode(element as IncrementalNode);
//            }
//            return;
//
//            function visitNode(node: IncrementalNode) {
//                auto text = "";
//                if (aggressiveChecks && shouldCheckNode(node)) {
//                    text = oldText.substring(node.pos, node.end);
//                }
//
//                // Ditch any existing LS children we may have created.  This way we can avoid
//                // moving them forward.
//                if (node._children) {
//                    node._children = undefined;
//                }
//
//                setTextRangePosEnd(node, node.pos + delta, node.end + delta);
//
//                if (aggressiveChecks && shouldCheckNode(node)) {
//                    Debug::asserts(text == newText.substring(node.pos, node.end));
//                }
//
//                forEachChild(node, visitNode, visitArray);
//                if (hasJSDocNodes(node)) {
//                    for (const jsDocComment of node.jsDoc!) {
//                        visitNode(jsDocComment as Node as IncrementalNode);
//                    }
//                }
//                checkNodePositions(node, aggressiveChecks);
//            }
//
//            function visitArray(array: IncrementalNodeArray) {
//                array._children = undefined;
//                setTextRangePosEnd(array, array.pos + delta, array.end + delta);
//
//                for (const node of array) {
//                    visitNode(node);
//                }
//            }
//        }
//
//        function shouldCheckNode(node: Node) {
//            switch (node.kind) {
//                case SyntaxKind::StringLiteral:
//                case SyntaxKind::NumericLiteral:
//                case SyntaxKind::Identifier:
//                    return true;
//            }
//
//            return false;
//        }
//
//        function adjustIntersectingElement(element: IncrementalElement, changeStart: number, changeRangeOldEnd: number, changeRangeNewEnd: number, delta: number) {
//            Debug::asserts(element.end >= changeStart, "Adjusting an element that was entirely before the change range");
//            Debug::asserts(element.pos <= changeRangeOldEnd, "Adjusting an element that was entirely after the change range");
//            Debug::asserts(element.pos <= element.end);
//
//            // We have an element that intersects the change range in some way.  It may have its
//            // start, or its end (or both) in the changed range.  We want to adjust any part
//            // that intersects such that the final tree is in a consistent state.  i.e. all
//            // children have spans within the span of their parent, and all siblings are ordered
//            // properly.
//
//            // We may need to update both the 'pos' and the 'end' of the element.
//
//            // If the 'pos' is before the start of the change, then we don't need to touch it.
//            // If it isn't, then the 'pos' must be inside the change.  How we update it will
//            // depend if delta is positive or negative. If delta is positive then we have
//            // something like:
//            //
//            //  -------------------AAA-----------------
//            //  -------------------BBBCCCCCCC-----------------
//            //
//            // In this case, we consider any node that started in the change range to still be
//            // starting at the same position.
//            //
//            // however, if the delta is negative, then we instead have something like this:
//            //
//            //  -------------------XXXYYYYYYY-----------------
//            //  -------------------ZZZ-----------------
//            //
//            // In this case, any element that started in the 'X' range will keep its position.
//            // However any element that started after that will have their pos adjusted to be
//            // at the end of the new range.  i.e. any node that started in the 'Y' range will
//            // be adjusted to have their start at the end of the 'Z' range.
//            //
//            // The element will keep its position if possible.  Or Move backward to the new-end
//            // if it's in the 'Y' range.
//            auto pos = Math.min(element.pos, changeRangeNewEnd);
//
//            // If the 'end' is after the change range, then we always adjust it by the delta
//            // amount.  However, if the end is in the change range, then how we adjust it
//            // will depend on if delta is positive or negative.  If delta is positive then we
//            // have something like:
//            //
//            //  -------------------AAA-----------------
//            //  -------------------BBBCCCCCCC-----------------
//            //
//            // In this case, we consider any node that ended inside the change range to keep its
//            // end position.
//            //
//            // however, if the delta is negative, then we instead have something like this:
//            //
//            //  -------------------XXXYYYYYYY-----------------
//            //  -------------------ZZZ-----------------
//            //
//            // In this case, any element that ended in the 'X' range will keep its position.
//            // However any element that ended after that will have their pos adjusted to be
//            // at the end of the new range.  i.e. any node that ended in the 'Y' range will
//            // be adjusted to have their end at the end of the 'Z' range.
//            auto end = element.end >= changeRangeOldEnd ?
//                // Element ends after the change range.  Always adjust the end pos.
//                element.end + delta :
//                // Element ends in the change range.  The element will keep its position if
//                // possible. Or Move backward to the new-end if it's in the 'Y' range.
//                Math.min(element.end, changeRangeNewEnd);
//
//            Debug::asserts(pos <= end);
//            if (element.parent) {
//                Debug::assertsGreaterThanOrEqual(pos, element.parent.pos);
//                Debug::assertsLessThanOrEqual(end, element.parent.end);
//            }
//
//            setTextRangePosEnd(element, pos, end);
//        }
//
//        function checkNodePositions(node: Node, aggressiveChecks: boolean) {
//            if (aggressiveChecks) {
//                auto pos = node.pos;
//                auto visitNode = (child: Node) => {
//                    Debug::asserts(child.pos >= pos);
//                    pos = child.end;
//                };
//                if (hasJSDocNodes(node)) {
//                    for (const jsDocComment of node.jsDoc!) {
//                        visitNode(jsDocComment);
//                    }
//                }
//                forEachChild(node, visitNode);
//                Debug::asserts(pos <= node.end);
//            }
//        }
//
//        function updateTokenPositionsAndMarkElements(
//            sourceFile: IncrementalNode,
//            changeStart: number,
//            changeRangeOldEnd: number,
//            changeRangeNewEnd: number,
//            delta: number,
//            oldText: string,
//            newText: string,
//            aggressiveChecks: boolean): void {
//
//            visitNode(sourceFile);
//            return;
//
//            function visitNode(child: IncrementalNode) {
//                Debug::asserts(child.pos <= child.end);
//                if (child.pos > changeRangeOldEnd) {
//                    // Node is entirely past the change range.  We need to move both its pos and
//                    // end, forward or backward appropriately.
//                    moveElementEntirelyPastChangeRange(child, /*isArray*/ false, delta, oldText, newText, aggressiveChecks);
//                    return;
//                }
//
//                // Check if the element intersects the change range.  If it does, then it is not
//                // reusable.  Also, we'll need to recurse to see what constituent portions we may
//                // be able to use.
//                auto fullEnd = child.end;
//                if (fullEnd >= changeStart) {
//                    child.intersectsChange = true;
//                    child._children = undefined;
//
//                    // Adjust the pos or end (or both) of the intersecting element accordingly.
//                    adjustIntersectingElement(child, changeStart, changeRangeOldEnd, changeRangeNewEnd, delta);
//                    forEachChild(child, visitNode, visitArray);
//                    if (hasJSDocNodes(child)) {
//                        for (const jsDocComment of child.jsDoc!) {
//                            visitNode(jsDocComment as Node as IncrementalNode);
//                        }
//                    }
//                    checkNodePositions(child, aggressiveChecks);
//                    return;
//                }
//
//                // Otherwise, the node is entirely before the change range.  No need to do anything with it.
//                Debug::asserts(fullEnd < changeStart);
//            }
//
//            function visitArray(array: IncrementalNodeArray) {
//                Debug::asserts(array.pos <= array.end);
//                if (array.pos > changeRangeOldEnd) {
//                    // Array is entirely after the change range.  We need to move it, and move any of
//                    // its children.
//                    moveElementEntirelyPastChangeRange(array, /*isArray*/ true, delta, oldText, newText, aggressiveChecks);
//                    return;
//                }
//
//                // Check if the element intersects the change range.  If it does, then it is not
//                // reusable.  Also, we'll need to recurse to see what constituent portions we may
//                // be able to use.
//                auto fullEnd = array.end;
//                if (fullEnd >= changeStart) {
//                    array.intersectsChange = true;
//                    array._children = undefined;
//
//                    // Adjust the pos or end (or both) of the intersecting array accordingly.
//                    adjustIntersectingElement(array, changeStart, changeRangeOldEnd, changeRangeNewEnd, delta);
//                    for (const node of array) {
//                        visitNode(node);
//                    }
//                    return;
//                }
//
//                // Otherwise, the array is entirely before the change range.  No need to do anything with it.
//                Debug::asserts(fullEnd < changeStart);
//            }
//        }
//
//        function extendToAffectedRange(sourceFile: SourceFile, changeRange: TextChangeRange): TextChangeRange {
//            // Consider the following code:
//            //      void foo() { /; }
//            //
//            // If the text changes with an insertion of / just before the semicolon then we end up with:
//            //      void foo() { //; }
//            //
//            // If we were to just use the changeRange a is, then we would not rescan the { token
//            // (as it does not intersect the actual original change range).  Because an edit may
//            // change the token touching it, we actually need to look back *at least* one token so
//            // that the prior token sees that change.
//            auto maxLookahead = 1;
//
//            auto start = changeRange.span.start;
//
//            // the first iteration aligns us with the change start. subsequent iteration move us to
//            // the left by maxLookahead tokens.  We only need to do this as long as we're not at the
//            // start of the tree.
//            for (let i = 0; start > 0 && i <= maxLookahead; i++) {
//                auto nearestNode = findNearestNodeStartingBeforeOrAtPosition(sourceFile, start);
//                Debug::asserts(nearestNode.pos <= start);
//                auto position = nearestNode.pos;
//
//                start = Math.max(0, position - 1);
//            }
//
//            auto finalSpan = createTextSpanFromBounds(start, textSpanEnd(changeRange.span));
//            auto finalLength = changeRange.newLength + (changeRange.span.start - start);
//
//            return createTextChangeRange(finalSpan, finalLength);
//        }
//
//        function findNearestNodeStartingBeforeOrAtPosition(sourceFile: SourceFile, position: number): Node {
//            auto bestResult: Node = sourceFile;
//            auto lastNodeEntirelyBeforePosition: Node | undefined;
//
//            forEachChild(sourceFile, visit);
//
//            if (lastNodeEntirelyBeforePosition) {
//                auto lastChildOfLastEntireNodeBeforePosition = getLastDescendant(lastNodeEntirelyBeforePosition);
//                if (lastChildOfLastEntireNodeBeforePosition.pos > bestResult.pos) {
//                    bestResult = lastChildOfLastEntireNodeBeforePosition;
//                }
//            }
//
//            return bestResult;
//
//            function getLastDescendant(node: Node): Node {
//                while (true) {
//                    auto lastChild = getLastChild(node);
//                    if (lastChild) {
//                        node = lastChild;
//                    }
//                    else {
//                        return node;
//                    }
//                }
//            }
//
//            function visit(child: Node) {
//                if (nodeIsMissing(child)) {
//                    // Missing nodes are effectively invisible to us.  We never even consider them
//                    // When trying to find the nearest node before us.
//                    return;
//                }
//
//                // If the child intersects this position, then this node is currently the nearest
//                // node that starts before the position.
//                if (child.pos <= position) {
//                    if (child.pos >= bestResult.pos) {
//                        // This node starts before the position, and is closer to the position than
//                        // the previous best node we found.  It is now the new best node.
//                        bestResult = child;
//                    }
//
//                    // Now, the node may overlap the position, or it may end entirely before the
//                    // position.  If it overlaps with the position, then either it, or one of its
//                    // children must be the nearest node before the position.  So we can just
//                    // recurse into this child to see if we can find something better.
//                    if (position < child.end) {
//                        // The nearest node is either this child, or one of the children inside
//                        // of it.  We've already marked this child as the best so far.  Recurse
//                        // in case one of the children is better.
//                        forEachChild(child, visit);
//
//                        // Once we look at the children of this node, then there's no need to
//                        // continue any further.
//                        return true;
//                    }
//                    else {
//                        Debug::asserts(child.end <= position);
//                        // The child ends entirely before this position.  Say you have the following
//                        // (where $ is the position)
//                        //
//                        //      <complex expr 1> ? <complex expr 2> $ : <...> <...>
//                        //
//                        // We would want to find the nearest preceding node in "complex expr 2".
//                        // To support that, we keep track of this node, and once we're done searching
//                        // for a best node, we recurse down this node to see if we can find a good
//                        // result in it.
//                        //
//                        // This approach allows us to quickly skip over nodes that are entirely
//                        // before the position, while still allowing us to find any nodes in the
//                        // last one that might be what we want.
//                        lastNodeEntirelyBeforePosition = child;
//                    }
//                }
//                else {
//                    Debug::asserts(child.pos > position);
//                    // We're now at a node that is entirely past the position we're searching for.
//                    // This node (and all following nodes) could never contribute to the result,
//                    // so just skip them by returning 'true' here.
//                    return true;
//                }
//            }
//        }
//
//        function checkChangeRange(sourceFile: SourceFile, newText: string, textChangeRange: TextChangeRange, aggressiveChecks: boolean) {
//            auto oldText = sourceFile.text;
//            if (textChangeRange) {
//                Debug::asserts((oldText.length - textChangeRange.span.length + textChangeRange.newLength) == newText.length);
//
//                if (aggressiveChecks || Debug.shouldAssert(AssertionLevel.VeryAggressive)) {
//                    auto oldTextPrefix = substr(oldText, 0, textChangeRange.span.start);
//                    auto newTextPrefix = substr(newText, 0, textChangeRange.span.start);
//                    Debug::asserts(oldTextPrefix == newTextPrefix);
//
//                    auto oldTextSuffix = oldText.substring(textSpanEnd(textChangeRange.span), oldText.length);
//                    auto newTextSuffix = newText.substring(textSpanEnd(textChangeRangeNewSpan(textChangeRange)), newText.length);
//                    Debug::asserts(oldTextSuffix == newTextSuffix);
//                }
//            }
//        }
//
//        interface IncrementalElement extends ReadonlyTextRange {
//            readonly parent: Node;
//            intersectsChange: boolean;
//            length?: number;
//            _children: Node[] | undefined;
//        }
//
//        export interface IncrementalNode extends Node, IncrementalElement {
//            hasBeenIncrementallyParsed: boolean;
//        }
//
//        interface IncrementalNodeArray extends NodeArray<IncrementalNode>, IncrementalElement {
//            length: number;
//        }
//
//        // Allows finding nodes in the source file at a certain position in an efficient manner.
//        // The implementation takes advantage of the calling pattern it knows the parser will
//        // make in order to optimize finding nodes as quickly as possible.
//        export interface SyntaxCursor {
//            currentNode(position: number): IncrementalNode;
//        }
//
//        export function createSyntaxCursor(sourceFile: SourceFile): SyntaxCursor {
//            auto currentArray: NodeArray<Node> = sourceFile.statements;
//            auto currentArrayIndex = 0;
//
//            Debug::asserts(currentArrayIndex < currentArray.length);
//            auto current = currentArray[currentArrayIndex];
//            auto lastQueriedPosition = InvalidPosition.Value;
//
//            return {
//                currentNode(position: number) {
//                    // Only compute the current node if the position is different than the last time
//                    // we were asked.  The parser commonly asks for the node at the same position
//                    // twice.  Once to know if can read an appropriate list element at a certain point,
//                    // and then to actually read and consume the node.
//                    if (position != lastQueriedPosition) {
//                        // Much of the time the parser will need the very next node in the array that
//                        // we just returned a node from.So just simply check for that case and move
//                        // forward in the array instead of searching for the node again.
//                        if (current && current.end == position && currentArrayIndex < (currentArray.length - 1)) {
//                            currentArrayIndex++;
//                            current = currentArray[currentArrayIndex];
//                        }
//
//                        // If we don't have a node, or the node we have isn't in the right position,
//                        // then try to find a viable node at the position requested.
//                        if (!current || current.pos != position) {
//                            findHighestListElementThatStartsAtPosition(position);
//                        }
//                    }
//
//                    // Cache this query so that we don't do any extra work if the parser calls back
//                    // into us.  Note: this is very common as the parser will make pairs of calls like
//                    // 'isListElement -> parseListElement'.  If we were unable to find a node when
//                    // called with 'isListElement', we don't want to redo the work when parseListElement
//                    // is called immediately after.
//                    lastQueriedPosition = position;
//
//                    // Either we don'd have a node, or we have a node at the position being asked for.
//                    Debug::asserts(!current || current.pos == position);
//                    return current as IncrementalNode;
//                }
//            };
//
//            // Finds the highest element in the tree we can find that starts at the provided position.
//            // The element must be a direct child of some node list in the tree.  This way after we
//            // return it, we can easily return its next sibling in the list.
//            function findHighestListElementThatStartsAtPosition(position: number) {
//                // Clear out any cached state about the last node we found.
//                currentArray = undefined!;
//                currentArrayIndex = InvalidPosition.Value;
//                current = undefined!;
//
//                // Recurse into the source file to find the highest node at this position.
//                forEachChild(sourceFile, visitNode, visitArray);
//                return;
//
//                function visitNode(node: Node) {
//                    if (position >= node.pos && position < node.end) {
//                        // Position was within this node.  Keep searching deeper to find the node.
//                        forEachChild(node, visitNode, visitArray);
//
//                        // don't proceed any further in the search.
//                        return true;
//                    }
//
//                    // position wasn't in this node, have to keep searching.
//                    return false;
//                }
//
//                function visitArray(array: NodeArray<Node>) {
//                    if (position >= array.pos && position < array.end) {
//                        // position was in this array.  Search through this array to see if we find a
//                        // viable element.
//                        for (let i = 0; i < array.length; i++) {
//                            auto child = array[i];
//                            if (child) {
//                                if (child.pos == position) {
//                                    // Found the right node.  We're done.
//                                    currentArray = array;
//                                    currentArrayIndex = i;
//                                    current = child;
//                                    return true;
//                                }
//                                else {
//                                    if (child.pos < position && position < child.end) {
//                                        // Position in somewhere within this child.  Search in it and
//                                        // stop searching in this array.
//                                        forEachChild(child, visitNode, visitArray);
//                                        return true;
//                                    }
//                                }
//                            }
//                        }
//                    }
//
//                    // position wasn't in this array, have to keep searching.
//                    return false;
//                }
//            }
//        }
//
//        auto enum InvalidPosition {
//            Value = -1
//        }
//    }
//
//    /*@internal*/
//    export interface PragmaContext {
//        languageVersion: ScriptTarget;
//        pragmas?: PragmaMap;
//        checkJsDirective?: CheckJsDirective;
//        referencedFiles: FileReference[];
//        typeReferenceDirectives: FileReference[];
//        libReferenceDirectives: FileReference[];
//        amdDependencies: AmdDependency[];
//        hasNoDefaultLib?: boolean;
//        moduleName?: string;
//    }
//
//    function parseResolutionMode(mode: string | undefined, int pos, end: number, reportDiagnostic: PragmaDiagnosticReporter): ModuleKind.ESNext | ModuleKind.CommonJS | undefined {
//        if (!mode) {
//            return undefined;
//        }
//        if (mode == "import") {
//            return ModuleKind.ESNext;
//        }
//        if (mode == "require") {
//            return ModuleKind.CommonJS;
//        }
//        reportDiagnostic(pos, end - pos, Diagnostics::resolution_mode_should_be_either_require_or_import);
//        return undefined;
//    }
//
//    /*@internal*/
//    export function processCommentPragmas(context: PragmaContext, sourceText: string): void {
//        auto pragmas: PragmaPseudoMapEntry[] = [];
//
//        for (const range of getLeadingCommentRanges(sourceText, 0) || emptyArray) {
//            auto comment = sourceText.substring(range.pos, range.end);
//            extractPragmas(pragmas, range, comment);
//        }
//
//        context.pragmas = new Map() as PragmaMap;
//        for (const pragma of pragmas) {
//            if (context.pragmas.has(pragma.name)) {
//                auto currentValue = context.pragmas.get(pragma.name);
//                if (currentValue instanceof Array) {
//                    currentValue.push(pragma.args);
//                }
//                else {
//                    context.pragmas.set(pragma.name, [currentValue, pragma.args]);
//                }
//                continue;
//            }
//            context.pragmas.set(pragma.name, pragma.args);
//        }
//    }
//
//    /*@internal*/
//    type PragmaDiagnosticReporter = (int pos, length: number, message: DiagnosticMessage) => void;
//
//    /*@internal*/
//    export function processPragmasIntoFields(context: PragmaContext, reportDiagnostic: PragmaDiagnosticReporter): void {
//        context.checkJsDirective = undefined;
//        context.referencedFiles = [];
//        context.typeReferenceDirectives = [];
//        context.libReferenceDirectives = [];
//        context.amdDependencies = [];
//        context.hasNoDefaultLib = false;
//        context.pragmas!.forEach((entryOrList, key) => { // TODO: GH#18217
//            // TODO: The below should be strongly type-guarded and not need casts/explicit annotations, since entryOrList is related to
//            // key and key is constrained to a union; but it's not (see GH#21483 for at least partial fix) :(
//            switch (key) {
//                case "reference": {
//                    auto referencedFiles = context.referencedFiles;
//                    auto typeReferenceDirectives = context.typeReferenceDirectives;
//                    auto libReferenceDirectives = context.libReferenceDirectives;
//                    forEach(toArray(entryOrList) as PragmaPseudoMap["reference"][], arg => {
//                        auto { types, lib, path, ["resolution-mode"]: res } = arg.arguments;
//                        if (arg.arguments["no-default-lib"]) {
//                            context.hasNoDefaultLib = true;
//                        }
//                        else if (types) {
//                            auto parsed = parseResolutionMode(res, types.pos, types.end, reportDiagnostic);
//                            typeReferenceDirectives.push({ pos: types.pos, end: types.end, fileName: types.value, ...(parsed ? { resolutionMode: parsed } : {}) });
//                        }
//                        else if (lib) {
//                            libReferenceDirectives.push({ pos: lib.pos, end: lib.end, fileName: lib.value });
//                        }
//                        else if (path) {
//                            referencedFiles.push({ pos: path.pos, end: path.end, fileName: path.value });
//                        }
//                        else {
//                            reportDiagnostic(arg.range.pos, arg.range.end - arg.range.pos, Diagnostics::Invalid_reference_directive_syntax);
//                        }
//                    });
//                    break;
//                }
//                case "amd-dependency": {
//                    context.amdDependencies = map(
//                        toArray(entryOrList) as PragmaPseudoMap["amd-dependency"][],
//                        x => ({ name: x.arguments.name, path: x.arguments.path }));
//                    break;
//                }
//                case "amd-module": {
//                    if (entryOrList instanceof Array) {
//                        for (const entry of entryOrList) {
//                            if (context.moduleName) {
//                                // TODO: It's probably fine to issue this diagnostic on all instances of the pragma
//                                reportDiagnostic(entry.range.pos, entry.range.end - entry.range.pos, Diagnostics::An_AMD_module_cannot_have_multiple_name_assignments);
//                            }
//                            context.moduleName = (entry as PragmaPseudoMap["amd-module"]).arguments.name;
//                        }
//                    }
//                    else {
//                        context.moduleName = (entryOrList as PragmaPseudoMap["amd-module"]).arguments.name;
//                    }
//                    break;
//                }
//                case "ts-nocheck":
//                case "ts-check": {
//                    // _last_ of either nocheck or check in a file is the "winner"
//                    forEach(toArray(entryOrList), entry => {
//                        if (!context.checkJsDirective || entry.range.pos > context.checkJsDirective.pos) {
//                            context.checkJsDirective = {
//                                enabled: key == "ts-check",
//                                end: entry.range.end,
//                                pos: entry.range.pos
//                            };
//                        }
//                    });
//                    break;
//                }
//                case "jsx":
//                case "jsxfrag":
//                case "jsximportsource":
//                case "jsxruntime":
//                    return; // Accessed directly
//                default: Debug.fail("Unhandled pragma kind"); // Can this be made into an assertNever in the future?
//            }
//        });
//    }
//
//    auto namedArgRegExCache = new Map<string, RegExp>();
//    function getNamedArgRegEx(name: string): RegExp {
//        if (namedArgRegExCache.has(name)) {
//            return namedArgRegExCache.get(name)!;
//        }
//        auto result = new RegExp(`(\\s${name}\\s*=\\s*)(?:(?:'([^']*)')|(?:"([^"]*)"))`, "im");
//        namedArgRegExCache.set(name, result);
//        return result;
//    }
//
//    auto tripleSlashXMLCommentStartRegEx = /^\/\/\/\s*<(\S+)\s.*?\/>/im;
//    auto singleLinePragmaRegEx = /^\/\/\/?\s*@(\S+)\s*(.*)\s*$/im;
//    function extractPragmas(pragmas: PragmaPseudoMapEntry[], range: CommentRange, text: string) {
//        auto tripleSlash = range.kind == SyntaxKind::SingleLineCommentTrivia && tripleSlashXMLCommentStartRegEx.exec(text);
//        if (tripleSlash) {
//            auto name = tripleSlash[1].toLowerCase() as keyof PragmaPseudoMap; // Technically unsafe cast, but we do it so the below check to make it safe typechecks
//            auto pragma = commentPragmas[name] as PragmaDefinition;
//            if (!pragma || !(pragma.kind! & PragmaKindFlags.TripleSlashXML)) {
//                return;
//            }
//            if (pragma.args) {
//                auto argument: {[index: string]: string | {value: string, int pos, end: number}} = {};
//                for (const arg of pragma.args) {
//                    auto matcher = getNamedArgRegEx(arg.name);
//                    auto matchResult = matcher.exec(text);
//                    if (!matchResult && !arg.optional) {
//                        return; // Missing required argument, don't parse
//                    }
//                    else if (matchResult) {
//                        auto value = matchResult[2] || matchResult[3];
//                        if (arg.captureSpan) {
//                            auto startPos = range.pos + matchResult.index + matchResult[1].length + 1;
//                            argument[arg.name] = {
//                                value,
//                                pos: startPos,
//                                end: startPos + value.length
//                            };
//                        }
//                        else {
//                            argument[arg.name] = value;
//                        }
//                    }
//                }
//                pragmas.push({ name, args: { arguments: argument, range } } as PragmaPseudoMapEntry);
//            }
//            else {
//                pragmas.push({ name, args: { arguments: {}, range } } as PragmaPseudoMapEntry);
//            }
//            return;
//        }
//
//        auto singleLine = range.kind == SyntaxKind::SingleLineCommentTrivia && singleLinePragmaRegEx.exec(text);
//        if (singleLine) {
//            return addPragmaForMatch(pragmas, range, PragmaKindFlags.SingleLine, singleLine);
//        }
//
//        if (range.kind == SyntaxKind::MultiLineCommentTrivia) {
//            auto multiLinePragmaRegEx = /@(\S+)(\s+.*)?$/gim; // Defined inline since it uses the "g" flag, which keeps a persistent index (for iterating)
//            auto multiLineMatch: RegExpExecArray | null;
//            while (multiLineMatch = multiLinePragmaRegEx.exec(text)) {
//                addPragmaForMatch(pragmas, range, PragmaKindFlags.MultiLine, multiLineMatch);
//            }
//        }
//    }
//
//    function addPragmaForMatch(pragmas: PragmaPseudoMapEntry[], range: CommentRange, kind: PragmaKindFlags, match: RegExpExecArray) {
//        if (!match) return;
//        auto name = match[1].toLowerCase() as keyof PragmaPseudoMap; // Technically unsafe cast, but we do it so they below check to make it safe typechecks
//        auto pragma = commentPragmas[name] as PragmaDefinition;
//        if (!pragma || !(pragma.kind! & kind)) {
//            return;
//        }
//        auto args = match[2]; // Split on spaces and match up positionally with definition
//        auto argument = getNamedPragmaArguments(pragma, args);
//        if (argument == "fail") return; // Missing required argument, fail to parse it
//        pragmas.push({ name, args: { arguments: argument, range } } as PragmaPseudoMapEntry);
//        return;
//    }
//
//    function getNamedPragmaArguments(pragma: PragmaDefinition, text: string | undefined): {[index: string]: string} | "fail" {
//        if (!text) return {};
//        if (!pragma.args) return {};
//        auto args = trimString(text).split(/\s+/);
//        auto argMap: {[index: string]: string} = {};
//        for (let i = 0; i < pragma.args.length; i++) {
//            auto argument = pragma.args[i];
//            if (!args[i] && !argument.optional) {
//                return "fail";
//            }
//            if (argument.captureSpan) {
//                return Debug.fail("Capture spans not yet implemented for non-xml pragmas");
//            }
//            argMap[argument.name] = args[i];
//        }
//        return argMap;
//    }

    inline shared<SourceFile> createSourceFile(
            string fileName,
            string sourceText,
            variant<ScriptTarget, CreateSourceFileOptions> languageVersionOrOptions,
            bool setParentNodes = false,
            optional<ScriptKind> scriptKind = {}
    ) {
//        tracing?.push(tracing.Phase.Parse, "createSourceFile", { path: fileName }, /*separateBeginAndEnd*/ true);
//        performance.mark("beforeParse");
        shared<SourceFile> result;
        optional<function<void(shared<SourceFile>)>> overrideSetExternalModuleIndicator;
        optional<ModuleKind> format;

        Parser parser;

//        perfLogger.logStartParseSourceFile(fileName);
        ScriptTarget languageVersion;
        if (std::holds_alternative<ScriptTarget>(languageVersionOrOptions)) {
            languageVersion = std::get<ScriptTarget>(languageVersionOrOptions);
        } else {
            auto options = std::get<CreateSourceFileOptions>(languageVersionOrOptions);
            languageVersion = options.languageVersion;
            format = options.impliedNodeFormat;
            overrideSetExternalModuleIndicator = options.setExternalModuleIndicator;
        }

        if (languageVersion == ScriptTarget::JSON) {
            result = parser.parseSourceFile(fileName, sourceText, languageVersion, setParentNodes, ScriptKind::JSON, {});
        } else {
            optional<function<void(shared<SourceFile>)>> setIndicator = !format.has_value() ? overrideSetExternalModuleIndicator : [format, overrideSetExternalModuleIndicator](shared<SourceFile> file) {
                file->impliedNodeFormat = format;
                if (overrideSetExternalModuleIndicator) {
                    (*overrideSetExternalModuleIndicator)(file);
                } else {
                    setExternalModuleIndicator(file);
                }
            };
            result = parser.parseSourceFile(fileName, sourceText, languageVersion, setParentNodes, scriptKind, setIndicator);
        }
//        perfLogger.logStopParseSourceFile();
//
//        performance.mark("afterParse");
//        performance.measure("Parse", "beforeParse", "afterParse");
//        tracing?.pop();
        return result;
    }
}
