#include <gtest/gtest.h>

#include <regex>
#include <memory>
#include <iostream>
#include <variant>
#include "../core.h"
#include "../types.h"
#include "../node_factory.h"

using namespace std;
using namespace ts;

TEST(core, regexp) {
    std::regex regex("^///?\\s*@(ts-expect-error|ts-ignore)");

    {
        std::cmatch m;
        if (std::regex_search("//@ts-ignore", m, regex)) {
            std::cout << m[1] << " " << m[1].length() << " = " << (m[1] == "ts-ignore") << "\n";
        }
    }

    {
        std::smatch m;
        const std::string bla = "//@ts-ignore";
        if (std::regex_search(bla, m, regex)) {
            std::cout << m[1] << " " << m[1].length() << " = " << (m[1] == "ts-ignore") << "\n";
        }
    }
}

TEST(core, sharedptr) {
    struct Node {
        shared_ptr<vector<Node>> decorators;
    };

    Node a;

    EXPECT_EQ(! ! a.decorators, false);
}

template<class T>
struct OptionalRef: public optional<T> {
    using optional<T>::emplace;

//    operator T&() {
//        return *(T*)(&this->value().get());
//    }
//
//    T &operator->() {
//        return *(T*)(&this->value().get());
//    }
};

//template<class T>
//struct ReceiveOptional {
//    ReceiveOptional(optional<const Node> &node){}
//};

TEST(core, optionalNode) {
    sharedOpt<Identifier> i;
    EXPECT_EQ(!!i, false);

    i = make_shared<Identifier>();
    EXPECT_EQ(!!i, true);

    auto fn = [](sharedOpt<Node> node) {
        node->to<Identifier>().escapedText = "changed";
    };

    fn(i);
//    fn(Identifier());

    EXPECT_EQ(i->escapedText, "changed");
}

struct TestEnum {
    constexpr static auto a = "a", b = "b", c = "c";
};

TEST(core, constHash) {
    int i = 0;

    auto b = "b";

    switch (const_hash(b)) {
        case const_hash(TestEnum::a):
            i = 1;
            break;
        case const_hash(TestEnum::b):
            i = 2;
            break;
        case const_hash(TestEnum::c):
            i = 3;
            break;
    }

    EXPECT_EQ(i, 2);
}

TEST(core, switchString) {
    int i = 0;

    auto b = "b";

    switch (const_hash(b)) {
        case "a"_hash:
            i = 1;
            break;
        case "b"_hash:
            i = 2;
            break;
        case "c"_hash:
            i = 3;
            break;
    }

    EXPECT_EQ(i, 2);
}

shared<Node> visitNode(const function<shared<Node>(shared<Node>)> &cbNode, sharedOpt<Node> node) {
    if (! node) return nullptr;
    return cbNode(node);
}

TEST(core, nodeVisitAlternative) {
    shared_ptr<Node> empty;
    shared_ptr<Node> a = make_shared<Identifier>();
    shared_ptr<Node> s = make_shared<SourceFile>();

    EXPECT_EQ(a->kind, SyntaxKind::Identifier);

    a = make_shared<SourceFile>();
    a = s;

    visitNode([&a](auto n) {
        EXPECT_EQ(&(*n), &(*a)) << "Same reference is passed";
        return nullptr;
    }, a);
}

//variant<reference_wrapper<optional<Node>>, optional<Node>> operator || (bool a, optional<Node> &b) {
//    if (!a) return b;
//    return nullopt;
//};

void acceptShared(shared<Node> node) {
    if (node->is<Identifier>()) {
        node->to<Identifier>().escapedText = "changed";
    }
}

TEST(core, passNodeToShared) {
    {
        Identifier node;
        acceptShared(make_shared<Identifier>(node));
        EXPECT_EQ(node.escapedText, "");
    }

    {
        auto node = make_shared<Identifier>();
        acceptShared(node);
        EXPECT_EQ(node->escapedText, "changed");
    }
}

TEST(core, logicalOrOverload) {
    sharedOpt<Node> empty;
    sharedOpt<Node> a = make_shared<Identifier>();
    sharedOpt<Node> b = make_shared<SourceFile>();
    sharedOpt<Node> c = make_shared<FunctionDeclaration>();

    EXPECT_EQ(a.get(), a.get());
    EXPECT_EQ(a.get(), (a || b).get());
    EXPECT_EQ(a.get(), (a || empty).get());
    EXPECT_EQ(a.get(), (empty || a).get());
    EXPECT_EQ(empty.get(), (empty || empty).get());

    EXPECT_EQ(b.get(), (empty || b || a).get());

    EXPECT_EQ(a.get(), (a || b || c).get());
    EXPECT_EQ(b.get(), (empty || b || c).get());
    EXPECT_EQ(c.get(), (empty || empty || c).get());
}

//TEST(core, assignUnionKeepsRef) {
//    //this is important so that moving around NodeUnion (even accidentally via by-value) doesn't mean we lose the reference
//    //since NodeUnion should act as regular {} in JS.
//    NodeUnion<Identifier, SourceFile> i1;
//    EXPECT_EQ(i1.kind(), SyntaxKind::Identifier);
//    i1.to<SourceFile>();
//    EXPECT_EQ(i1.kind(), SyntaxKind::SourceFile);
//
//    NodeUnion<Identifier, SourceFile> i2 = i1;
//    EXPECT_EQ(i2.kind(), SyntaxKind::SourceFile);
//    EXPECT_EQ(&(*i1.node), &(*i2.node));
//}


//NodeUnion has no type related semantics, just meta-data
SyntaxKind takeUnion(shared<NodeUnion(Identifier, SourceFile)> node) {
    return node->kind;
}

SyntaxKind takeNode(shared<Node> node) {
    return node->kind;
}

SyntaxKind takeOptionalNode(sharedOpt<Node> node) {
    if (node) return node->kind;
    return SyntaxKind::Unknown;
}

SyntaxKind takeOptionalNodeUnion(sharedOpt<NodeUnion(Identifier, SourceFile)> node) {
    if (node) return node->kind;
    return SyntaxKind::Unknown;
}

TEST(core, passBaseToSharedNode) {
    auto e = factory::createBaseExpression<BinaryExpression>(SyntaxKind::BinaryExpression);
    EXPECT_EQ(takeNode(e), SyntaxKind::BinaryExpression);
}


TEST(core, passBaseToSharedNode2) {
    auto left = factory::createBaseNode<Expression>(SyntaxKind::Unknown);
    auto right = factory::createBaseNode<Expression>(SyntaxKind::Unknown);
    auto operatorNode = factory::createBaseNode<Node>(SyntaxKind::Unknown);

    auto e = factory::createBinaryExpression(left, operatorNode, right);
    EXPECT_EQ(takeNode(e), SyntaxKind::BinaryExpression);
}

TEST(core, ParameterDeclaration) {
    auto p = make_shared<ParameterDeclaration>();

    EXPECT_EQ(takeUnion(p->name), SyntaxKind::Identifier);
    EXPECT_EQ(takeNode(p->name), SyntaxKind::Identifier);
    EXPECT_EQ(takeOptionalNode(p->name), SyntaxKind::Identifier);
    EXPECT_EQ(takeOptionalNodeUnion(p->name), SyntaxKind::Identifier);
}

TEST(core, nodeUnion) {
    shared<NodeUnion(Identifier, SourceFile)> i1 = make_shared<Identifier>();
    EXPECT_EQ(takeUnion(i1), SyntaxKind::Identifier);
    EXPECT_EQ(takeNode(i1), SyntaxKind::Identifier);
    EXPECT_EQ(takeOptionalNode(i1), SyntaxKind::Identifier);
    EXPECT_EQ(takeOptionalNodeUnion(i1), SyntaxKind::Identifier);

    shared<NodeUnion(Identifier, SourceFile)> i2(new SourceFile);
    EXPECT_EQ(takeUnion(i2), SyntaxKind::SourceFile);
    EXPECT_EQ(takeNode(i2), SyntaxKind::SourceFile);
    EXPECT_EQ(takeOptionalNode(i2), SyntaxKind::SourceFile);
    EXPECT_EQ(takeOptionalNodeUnion(i2), SyntaxKind::SourceFile);

    sharedOpt<NodeUnion(Identifier, SourceFile)> o1(new Identifier);
    EXPECT_EQ(takeUnion(o1), SyntaxKind::Identifier);
    EXPECT_EQ(takeNode(o1), SyntaxKind::Identifier);
    EXPECT_EQ(takeOptionalNode(o1), SyntaxKind::Identifier);
    EXPECT_EQ(takeOptionalNodeUnion(o1), SyntaxKind::Identifier);

    sharedOpt<NodeUnion(Identifier, SourceFile)> o2;
    EXPECT_EQ(takeOptionalNode(o2), SyntaxKind::Unknown);
    EXPECT_EQ(takeOptionalNodeUnion(o2), SyntaxKind::Unknown);
}

TEST(core, node2) {
    EXPECT_EQ(SourceFile().kind, SyntaxKind::SourceFile);
    EXPECT_EQ(IfStatement().kind, SyntaxKind::IfStatement);
    EXPECT_EQ(Block().kind, SyntaxKind::Block);

    SourceFile source;

    EXPECT_EQ(source.hasParent(), false);
    EXPECT_EQ(source.kind, SyntaxKind::SourceFile);
    auto cb = [](Node &node) {
        EXPECT_EQ(node.kind, SyntaxKind::SourceFile);
        EXPECT_EQ(node.to<SourceFile>().kind, SyntaxKind::SourceFile);
    };

    cb(source);
}

TEST(core, node) {
    auto node = factory::createBaseNode<Identifier>();
    node->to<Identifier>().escapedText = "id";

    EXPECT_EQ(node->is<MetaProperty>(), false);
    EXPECT_EQ(node->is<Identifier>(), true);
    EXPECT_EQ(node->to<Identifier>().escapedText, "id");
}

TEST(core, logicalOrOverride) {
    auto a = LogicalOrReturnLast(0);
    auto b = LogicalOrReturnLast(1);
    auto c = LogicalOrReturnLast(2);

    EXPECT_EQ((int) a, 0);

    EXPECT_EQ((int) (a || 0), 0);
    EXPECT_EQ((int) (0 || a), 0);
    EXPECT_EQ((int) (a || 1), 1);
    EXPECT_EQ((int) (1 || a), 1);
    EXPECT_EQ((int) (a || b), 1);
    EXPECT_EQ((int) (a || b || 2), 1);
    EXPECT_EQ((int) (a || 0 || c), 2);
    EXPECT_EQ((int) ((LogicalOrReturnLast<int>) 3 || a || b), 3);
}

TEST(core, charToString) {
    vector<const char *> chars{"a", "b", "c"};
    auto s = charToStringVector(chars);
    EXPECT_EQ(s, vector<string>({"a", "b", "c"}));
}

TEST(core, split) {
    EXPECT_EQ(split("a", "/"), vector<string>({"a"}));
    EXPECT_EQ(split("a/b", "/"), vector<string>({"a", "b"}));
    EXPECT_EQ(split("a/b/c", "/"), vector<string>({"a", "b", "c"}));
}

TEST(core, replaceLeading) {
    EXPECT_EQ(replaceLeading("./is/a/path", "./", "/"), "/is/a/path");
    EXPECT_EQ(replaceLeading("./is/a/./path", "./", "/"), "/is/a/./path");
    EXPECT_EQ(replaceLeading("/is/a/path", "./", "."), "/is/a/path");
}

TEST(core, stringReplace) {
    EXPECT_EQ(replaceAll("this/is/a/path", "/", "_"), "this_is_a_path");
    EXPECT_EQ(replaceAll("this/is/a/path", "/", "/"), "this/is/a/path");
}

TEST(core, endsWith) {
    EXPECT_EQ(endsWith("this/is/a/path", "asd"), false);
    EXPECT_EQ(endsWith("this/is/a/path", "/path"), true);
}

TEST(core, startsWith) {
    EXPECT_EQ(startsWith("this/is/a/path", "asd"), false);
    EXPECT_EQ(startsWith("this/is/a/path", "this"), true);
    EXPECT_EQ(startsWith("this/is/a/path", "this/"), true);
}

TEST(core, substrNegative) {
    string a = "abc";
    EXPECT_EQ(substr(a, -1), "c");
    EXPECT_EQ(substr(a, 0, -1), "ab");
}

TEST(core, regex) {
    std::regex regex("^///?\\s*@(ts-expect-error|ts-ignore)");
    std::cmatch m;

    if (std::regex_search("//@ts-ignore", m, regex)) {
        std::cout << m[1] << "\n";
    }

    EXPECT_EQ(m[1], "ts-ignore");
}

bool test() {
    return true;
}

template<typename T>
T executeFn(function<T()> callback) {
    return callback();
}

TEST(core, passFn) {
    auto a = executeFn<bool>(test);
}