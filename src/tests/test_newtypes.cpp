#include <gtest/gtest.h>
#include <variant>
#include <any>

#include "../core.h"

using namespace std;
using namespace ts;

enum class SyntaxKind {
    Unknown,
    SourceFile,
    BinaryExpression,
    BinaryOperator,
    Block,
    TryStatement,
    Identifier = 6,
    CatchClause,
};

struct ReadonlyTextRange {
    int pos;
    int end;
};

struct Node: ReadonlyTextRange {
    SyntaxKind kind = SyntaxKind::Unknown;
    Node &parent = *this;

//    Node() {
//        debug("Node() %d", kind);
//    }
//
//    ~Node() {
//        debug("~Node() %d", kind);
//    }

    template<typename T>
    T &to() {
        if (kind != T::KIND) throw std::runtime_error(format("Can not convert Node, from kind %d to %d", kind, T::KIND));
        return *reinterpret_cast<T *>(this);
    }
};

template<SyntaxKind Kind, class ... B>
struct BrandKind: B ... {
    constexpr static auto KIND = Kind;
    BrandKind() {
        this->kind = Kind;
    }
};

//todo: add? https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
struct BaseUnion {
//    template<typename T>
//    bool is() {
//        return std::holds_alternative<shared_ptr<T>>(value);
//    }
//
//    template<typename T>
//    T &to() {
//        if (! is<T>()) {
//            value = make_shared<T>();
//        }
//        return *std::get<shared_ptr<T>>(value);
//    }
};

template<typename Default, typename ... Ts>
struct Union: BaseUnion {
    std::variant<shared_ptr<Default>, shared_ptr<Ts>...> value = make_shared<Default>();
////    auto types() {
////        using ETypes = std::tuple<decltype(Ts{})...>;
////        ETypes types;
////        return types;
////    }

    template<typename T>
    bool is() {
        return std::holds_alternative<shared_ptr<T>>(value);
    }

    template<typename T>
    T &to() {
        if (! is<T>()) {
            value = make_shared<T>();
        }
        return *std::get<shared_ptr<T>>(value);
    }
};

template<typename ... Ts>
struct NodeUnion: Union<Node, Ts...> {
    /**
     * Casts whatever is current hold as Node.
     */
    Node &getNode() {
        Node *b = nullptr;

        std::visit([&b](auto &arg) {
            b = reinterpret_cast<Node *>(&(*arg));
        }, this->value);

        if (! b) throw std::runtime_error("Union does not hold a Node");

        return *b;
    }
};

struct NodeArray {
    vector<reference_wrapper<Node>> list;
};

template<class T>
struct NodeArrayType: NodeArray {
//    vector<T> list;
};

struct Expression: Node {
    void initKind(SyntaxKind *kind) {
    }
};
struct UnaryExpression: Expression {};

struct UpdateExpression: UnaryExpression {};

struct LeftHandSideExpression: UpdateExpression {};

struct MemberExpression: LeftHandSideExpression {};

struct Declaration {};

struct PrimaryExpression: MemberExpression {};

struct Statement: Node {
    void initKind(SyntaxKind *kind) {
    }
};

template<SyntaxKind T>
struct Token: BrandKind<T, Node> {
};

using BinaryOperatorToken = Token<SyntaxKind::BinaryOperator>;

struct Block: BrandKind<SyntaxKind::Block, Statement> {
    NodeArrayType<Statement> statements;
    /*@internal*/ bool multiLine;
};

struct BinaryExpression: BrandKind<SyntaxKind::BinaryExpression, Expression, Declaration> {
    Expression left;
    BinaryOperatorToken operatorToken;
    Expression right;
};

struct CatchClause;

struct TryStatement: BrandKind<SyntaxKind::TryStatement, Statement> {
    Block tryBlock;
    optional<reference_wrapper<CatchClause>> catchClause;
    optional<Block> finallyBlock;
};

struct CatchClause: BrandKind<SyntaxKind::CatchClause, Node> {
    TryStatement parent;
    Block block;
};

//#define Model(x) struct x
//
//Model(Bla) {}

struct SourceFile: BrandKind<SyntaxKind::SourceFile, Node, Declaration> {
    NodeArrayType<Statement> statements;
};

struct Identifier: BrandKind<SyntaxKind::Identifier, PrimaryExpression> {
    /**
     * Prefer to use `id.unescapedText`. (Note: This is available only in services, not internally to the TypeScript compiler.)
     * Text of identifier, but if the identifier begins with two underscores, this will begin with three.
     */
    string escapedText;
    SyntaxKind originalKeywordKind = SyntaxKind::Unknown;
};

struct PropertyAccessExpression: Node {};

struct PropertyAccessEntityNameExpression;

using EntityNameExpression = NodeUnion<Identifier, PropertyAccessEntityNameExpression>;

struct PropertyAccessEntityNameExpression: PropertyAccessExpression {
    EntityNameExpression expression;
    Identifier name;
};

void visitNode(Node &node, const function<void(Node &)> &cbNode) {
    cbNode(node);
}

void visitNodes(NodeArray &nodes, const function<void(Node &)> &cbNode) {
    debug("visit nodes %d", (int) nodes.list.size());
    for (auto node: nodes.list) {
        cbNode(node);
    }
}

template<typename T>
T &to(Node &node) {
    if (node.kind != T::KIND) {
        throw std::runtime_error(format("Can not convert Node, invalid kind: node has %d, passed type has %d", node.kind, (int) T::KIND));
    }
    return *reinterpret_cast<T *>(&node);
}

void forEachChild(Node &node, const function<void(Node &)> &cbNode) {
    switch (node.kind) {
        case SyntaxKind::SourceFile:
            visitNodes(to<SourceFile>(node).statements, cbNode);
            break;
        case SyntaxKind::BinaryExpression:
            visitNode(to<BinaryExpression>(node).left, cbNode);
            visitNode(to<BinaryExpression>(node).right, cbNode);
            visitNode(to<BinaryExpression>(node).operatorToken, cbNode);
            break;
        case SyntaxKind::Block:
            visitNodes(to<Block>(node).statements, cbNode);
            break;
    }
}

#define AcceptUnion(...) BaseUnion

//void acceptUnion2(class BaseUnion2 u) {
//
//}

void acceptUnion(AcceptUnion(Identifier, PropertyAccessEntityNameExpression) u) {

}

TEST(new_types, variants) {
    variant<int, bool, string> v;

    std::visit([](auto &arg) {
        using T = std::decay_t<decltype(arg)>;
        debug("visited");

        if constexpr (std::is_same_v<T, string>) {
            printf("string: %s\n", arg.c_str());
            // ...
        }
        else if constexpr (std::is_same_v<T, int>) {
            printf("integer: %d\n", arg);
            // ...
        }
        else if constexpr (std::is_same_v<T, bool>) {
            printf("bool: %d\n", arg);
            // ...
        }
    }, v);
}

TEST(new_types, passunion) {
    NodeUnion<Identifier, PropertyAccessEntityNameExpression> entityNameExpression;
    acceptUnion(entityNameExpression);

    NodeUnion<Identifier, PropertyAccessEntityNameExpression, SourceFile> entityNameExpression2;
    acceptUnion(entityNameExpression2);
}

TEST(new_types, unionbase) {
    NodeUnion<Identifier, PropertyAccessEntityNameExpression> entityNameExpression;
    EXPECT_EQ(entityNameExpression.getNode().kind, SyntaxKind::Unknown);

    entityNameExpression.to<Identifier>();
    EXPECT_EQ(entityNameExpression.getNode().kind, SyntaxKind::Identifier);
    EXPECT_EQ(entityNameExpression.getNode().kind, SyntaxKind::Identifier);

    entityNameExpression.to<Identifier>();
    EXPECT_EQ(entityNameExpression.getNode().kind, SyntaxKind::Identifier);

    //PropertyAccessEntityNameExpression has no kind
    entityNameExpression.to<PropertyAccessEntityNameExpression>();
    EXPECT_EQ(entityNameExpression.getNode().kind, SyntaxKind::Unknown);
}

TEST(new_types, union) {
    PropertyAccessEntityNameExpression exp;

    debug("------- is:");

    EXPECT_EQ(exp.name.kind, SyntaxKind::Identifier);
    EXPECT_EQ(exp.expression.is<Node>(), true);
    EXPECT_EQ(exp.expression.is<Identifier>(), false);
    EXPECT_EQ(exp.expression.is<PropertyAccessEntityNameExpression>(), false);

    debug("------- to:");
    exp.expression.to<Identifier>().escapedText = "hi";
    exp.expression.to<Identifier>().escapedText = "hi";

    EXPECT_EQ(exp.expression.to<Identifier>().escapedText, "hi");
    EXPECT_EQ(exp.expression.is<PropertyAccessEntityNameExpression>(), false);

    debug("------- to property");

    exp.expression.to<PropertyAccessEntityNameExpression>().name.escapedText = "myName";
    EXPECT_EQ(exp.expression.is<PropertyAccessEntityNameExpression>(), true);
    debug("------- done");
}

TEST(new_types, base) {
//    SourceFile sourceFile;

//    Statement s1;
//    sourceFile.statements.list.push_back(s1);
//
//    Block b1;
//    sourceFile.statements.list.push_back(b1);
//
//    cout << "sourceFile " << (int) sourceFile.kind << "\n";
//    forEachChild(sourceFile, [](Node &node) {
//        cout << "Visit " << (int) node.kind << "\n";
//    });
//
//    EXPECT_EQ(SourceFile().kind, SyntaxKind::SourceFile);
//    EXPECT_EQ(Block().kind, SyntaxKind::Block);
//    EXPECT_EQ(TryStatement().kind, SyntaxKind::TryStatement);
//
//    SourceFile source;
//
//    EXPECT_EQ(source.kind, SyntaxKind::SourceFile);
//    auto cb = [](Node &node) {
//        EXPECT_EQ(node.kind, SyntaxKind::SourceFile);
//        EXPECT_EQ(node.to<SourceFile>().kind, SyntaxKind::SourceFile);
//    };
//
//    //todo: how do we implement Union?
//    //todo: test reference_wrapper
//
//    cb(source);
}
