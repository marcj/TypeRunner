#pragma once

#include <string>
#include "../enum.h"

namespace ts::vm2 {
    using std::string;
    using std::string_view;

    enum class TypeKind: unsigned char {
        Unknown,
        Never,
        Any,
        String,
        Number,
        Boolean,
        Literal,
        PropertySignature,
        ObjectLiteral,
        Union,
        Tuple,
        TupleMember,
    };

    enum TypeFlag: unsigned int {
        Readonly = 1 << 0,
        Optional = 1 << 1,
        StringLiteral = 1 << 2,
        NumberLiteral = 1 << 3,
        BooleanLiteral = 1 << 4,
        True = 1 << 5,
        False = 1 << 6,
        UnprovidedArgument = 1 << 7,
    };

    struct Type {
        TypeKind kind;
        unsigned int ip;
        string_view text;
        /** see TypeFlag */
        unsigned int flag;
        uint64_t hash;
//        string_view text2;
        Type *type;
//        Type * type2;
        vector<Type *> types;
    };

//    struct TypeMeta {
//        string_view typeName;
//    };
//
//#define BaseProps(T) \
//    TypeKind kind = T; \
//    unsigned int ip; \
//    TypeMeta meta;
//
//    struct Type {
//        BaseProps(TypeKind::Unknown);
//    };
//
//    struct TypeNever {
//        BaseProps(TypeKind::Never);
//    };
//
//    struct TypeAny {
//        BaseProps(TypeKind::Any);
//    };
//
//    struct TypeLiteral {
//        BaseProps(TypeKind::Literal);
//        unsigned char type;
//        string_view value;
//    };
//
//    struct TypePropertySignature {
//        BaseProps(TypeKind::PropertySignature);
//        string_view name;
//        Type *type;
//    };
//
//    struct TypeObjectLiteral {
//        BaseProps(TypeKind::ObjectLiteral);
//        vector<Type *> types;
//    };
//
//    struct TypeUnion {
//        BaseProps(TypeKind::Union);
//        vector<Type *> types;
//    };
//
//    struct TypeTuple {
//        BaseProps(TypeKind::Tuple);
//        vector<Type *> types;
//    };
//
//    struct TypeTupleMember {
//        BaseProps(TypeKind::TupleMember);
//        string_view name;
//        Type *type;
//    };
}

template<>
struct fmt::formatter<ts::vm2::TypeKind>: formatter<std::string_view> {
    template<typename FormatContext>
    auto format(ts::vm2::TypeKind p, FormatContext &ctx) {
        return formatter<string_view>::format(magic_enum::enum_name(p), ctx);
    }
};
