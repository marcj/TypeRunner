#include <gtest/gtest.h>

#include <regex>
#include <memory>
#include <iostream>
#include <variant>
#include "../core.h"
#include "../types.h"
#include "../factory.h"

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
    optional<Identifier> i;
    EXPECT_EQ(i.has_value(), false);

    i.emplace(Identifier());
    EXPECT_EQ(i.has_value(), true);

    auto fn = [](OptionalNode node) {
        resolve(node).to<Identifier>().escapedText = "changed";
    };

    fn(i);
//    fn(Identifier());

    EXPECT_EQ(i->escapedText, "changed");
}

SyntaxKind takeUnion(BaseUnion &node) {
    return node.kind();
}

SyntaxKind takeNode(const Node &node) {
    return node.kind;
}

SyntaxKind takeOptionalNode(OptionalNode node) {
    return resolve(node).kind;
}

TEST(core, nodeUnion) {
    NodeUnion<Identifier, SourceFile> i1;
    EXPECT_EQ(takeUnion(i1), SyntaxKind::Identifier);
    EXPECT_EQ(takeNode(i1), SyntaxKind::Identifier);

    NodeUnion<Identifier, SourceFile> i2{SourceFile()};
    EXPECT_EQ(takeUnion(i2), SyntaxKind::SourceFile);
    EXPECT_EQ(takeNode(i2), SyntaxKind::SourceFile);

    EXPECT_EQ(takeOptionalNode(i1), SyntaxKind::Identifier);
    optional<NodeUnion<Identifier, SourceFile>> o1;
    o1.emplace(Identifier());
    EXPECT_EQ(takeOptionalNode(o1), SyntaxKind::Identifier);
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
    auto node = createBaseNode<Identifier>();
    node.to<Identifier>().escapedText = "id";

    EXPECT_EQ(node.is<MetaProperty>(), false);
    EXPECT_EQ(node.is<Identifier>(), true);
    EXPECT_EQ(node.to<Identifier>().escapedText, "id");
}

//TEST(core, nodeUnion) {
//    NodeType<Identifier, QualifiedName> node;
//
//    EXPECT_EQ(node.kinds()[0], SyntaxKind::Identifier);
//    EXPECT_EQ(node.kinds()[1], SyntaxKind::QualifiedName);
//
//    EXPECT_EQ(node.contains(SyntaxKind::Identifier), true);
//    EXPECT_EQ(node.contains(SyntaxKind::QualifiedName), true);
//    EXPECT_EQ(node.contains(SyntaxKind::TypeParameter), false);
//
////    using MyIds = variant<Identifier, QualifiedName>;
//////    struct MyIds: NodeType<Identifier, QualifiedName>{};
////
////    auto id = createBaseNode<Identifier>();
////    id.to<Identifier>().escapedText = "id";
////
////    auto id2 = id.toUnion<MyIds>();
//}

struct A {
    constexpr static auto kind = types::SyntaxKind::TypeParameter;
};

#include <type_traits>

struct B {
//    using kind = types::SyntaxKind::Identifier;
    constexpr static auto kind = types::SyntaxKind::Identifier;
};

//template<template <int> typename T, int N>
//constexpr auto extract(const T<N>&...) -> val<N>;

//template<typename ... T>
//constexpr auto extract_N = declval(T...)::kind;

//template<typename ... T>
//struct NodeUnion2: public Node {
//    using ETypes = std::tuple<decltype(T{})...>;
//    ETypes types;
//
//    bool contains(types::SyntaxKind kind) {
//        getKinds(types);
//        return false;
//    }
//};

////    using Ts::g...;
//
//    template<Ts... Values> // expands to a non-type template parameter
//    struct apply {};      // list, such as <int, char, int(&)[5]>
//
//    int types() {
//        vector<int> kinds{...Ts::kind};
////        auto lm = [&, Ts...] { return g(Ts...); };
//    }
//};

//void myFunction(BaseNodeStructureWithoutDecorators &node) {
//    auto sourceFile = to<SourceFile>(node);
//}
//
//TEST(core, variants) {
//    SourceFile sourceFile;
//    myFunction(sourceFile);
//}

TEST(core, variantUnion) {
//    NodeUnion2<A, B> n;
//    n.contains(types::SyntaxKind::Identifier);
////    std::cout << n.contains(types::SyntaxKind::Identifier) << '\n';
////    std::cout << n.contains(types::SyntaxKind::TypeParameter) << '\n';
////    std::cout << n.contains(types::SyntaxKind::ArrayType) << '\n';
////    std::cout << std::get<0>(n.types).kind;
////    std::cout << std::get<1>(n.types).kind;
}

template<class T>
int extractKind() {
//    auto a = declval<T>();
//    std::cout << a;
    return T::kind;
}

template<typename ... T>
struct NodeUnion: public Node {
    using T2 = std::tuple<decltype(T{})...>;
};

TEST(core, extractKind) {
    std::cout << extractKind<A>() << '\n';
    std::cout << extractKind<B>() << '\n';
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

//#include <regex>
//#include <iostream>
//
//TEST_CASE( "test regex" ) {
//    std::regex regex("^///?\\s*@(ts-expect-error|ts-ignore)");
//    std::cmatch m;
//
//    if(std::regex_search("//@ts-ignore", m, regex)) {
//        std::cout << m[1] << "\n";
//    }
//
//    REQUIRE( 1 == 1 );
//}
