#pragma once

#include <string>
#include <array>
#include <vector>
#include <memory>

namespace ts::checker {
    using std::string_view;
    using std::array;
    using std::reference_wrapper;

    enum class TypeKind: unsigned char {
        Never,
        Any,
        Unknown,
        Void,
        Object,
        String,
        Number,
        Boolean,
        Symbol,
        Bigint,
        Null,
        Undefined,
        Regexp,

        Literal,
        TemplateLiteral,
        Property,
        Method,
        Function,
        Parameter,

        Promise,

        /**
         * Uint8Array, Date, custom classes, Set, Map, etc
         */
        Class,

        TypeParameter,
        Enum,
        Union,
        Intersection,

        Array,
        Tuple,
        TupleMember,
        EnumMember,

        Rest,

        ObjectLiteral,
        IndexSignature,
        PropertySignature,
        MethodSignature,

        Infer,
    };

    struct Type {
        string_view typeName;
        array<reference_wrapper<Type>, 0> typeArguments;
    };

    template<TypeKind T, class ... Base>
    struct BrandKind: Base ... {
//        constexpr static const TypeKind KIND = T;
//        const TypeKind kind = T;
    };

    struct TypeNever: BrandKind<TypeKind::Never, Type> {};

    struct TypeAny: BrandKind<TypeKind::Any, Type> {};
    struct TypeUnknown: BrandKind<TypeKind::Unknown, Type> {};
    struct TypeVoid: BrandKind<TypeKind::Void, Type> {};
    struct TypeObject: BrandKind<TypeKind::Object, Type> {};
    struct TypeString: BrandKind<TypeKind::String, Type> {};
    struct TypeNumber: BrandKind<TypeKind::Number, Type> {};
    struct TypeBigint: BrandKind<TypeKind::Bigint, Type> {};
    struct TypeBoolean: BrandKind<TypeKind::Boolean, Type> {};
    struct TypeSymbol: BrandKind<TypeKind::Symbol, Type> {};
    struct TypeNull: BrandKind<TypeKind::Null, Type> {};
    struct TypeUndefined: BrandKind<TypeKind::Undefined, Type> {};
    struct TypeLiteral: BrandKind<TypeKind::Literal, Type> {
        //symbol, string, number, boolean, bigint (regexp)
    };

    struct TypeParameter: BrandKind<TypeKind::Parameter, Type> {
        string_view name; //number, string, symbol
        std::unique_ptr<Type> type;
    };

    struct TypeFunction: BrandKind<TypeKind::Function, Type> {
        string_view name; //number, string, symbol
//        array<TypeParameter*, 0> parameters;
        std::unique_ptr<TypeParameter> t;
        std::array<std::unique_ptr<TypeParameter>, 42> m_dates;
    };
}