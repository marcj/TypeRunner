#pragma once
#include "../core.h"
#include <string>
#include <array>
#include <utility>
#include <vector>
#include <memory>

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 512
#include "magic_enum.hpp"

namespace ts::vm {
    using std::string_view;
    using std::array;
    using std::to_string;
    using std::make_shared;
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
        TypeKind kind = TypeKind::Never;

        virtual ~Type() {
        }
//        array<reference_wrapper<Type>, 0> typeArguments;
    };

    template<TypeKind T, class ... Base>
    struct BrandKind: Base ... {
        constexpr static const TypeKind KIND = T;
        BrandKind() {
            this->kind = T;
        }
    };

    struct TypeNever: BrandKind<TypeKind::Never, Type> {};
    struct TypeAny: BrandKind<TypeKind::Any, Type> {};
    struct TypeUnknown: BrandKind<TypeKind::Unknown, Type> {
        bool unprovidedArgument = false; //when used as TypeArgument and nothing was provided
    };
    struct TypeVoid: BrandKind<TypeKind::Void, Type> {};
    struct TypeObject: BrandKind<TypeKind::Object, Type> {};
    struct TypeString: BrandKind<TypeKind::String, Type> {};
    struct TypeNumber: BrandKind<TypeKind::Number, Type> {};
    struct TypeBigint: BrandKind<TypeKind::Bigint, Type> {};
    struct TypeBoolean: BrandKind<TypeKind::Boolean, Type> {};
    struct TypeSymbol: BrandKind<TypeKind::Symbol, Type> {};
    struct TypeNull: BrandKind<TypeKind::Null, Type> {};
    struct TypeUndefined: BrandKind<TypeKind::Undefined, Type> {};

    struct TypeTemplateLiteral: BrandKind<TypeKind::TemplateLiteral, Type> {
//        types: (TypeString | TypeAny | TypeNumber | TypeStringLiteral | TypeInfer)[]
        vector<shared<Type>> types;

        explicit TypeTemplateLiteral() {}
        explicit TypeTemplateLiteral(const vector<shared<Type>> &types): types(types) {}
    };

    struct TypeUnion: BrandKind<TypeKind::Union, Type> {
        vector<shared<Type>> types;
    };

    struct TypeRest: BrandKind<TypeKind::Rest, Type> {
        shared<Type> type;
        explicit TypeRest(shared<Type> type): type(std::move(type)) {
        }
    };

    struct TypeTupleMember: BrandKind<TypeKind::TupleMember, Type> {
        shared<Type> type;
        bool optional = false;
        string_view name = "";

        explicit TypeTupleMember(shared<Type> type): type(std::move(type)) {
        }
    };

    struct TypeTuple: BrandKind<TypeKind::Tuple, Type> {
        vector<shared<TypeTupleMember>> types;
    };

    enum class TypeLiteralType {
        Number,
        String,
        Boolean,
        Bigint
    };

    struct TypeLiteral: BrandKind<TypeKind::Literal, Type> {
    public:
        string *literalText = nullptr;
        string_view literal;
        TypeLiteralType type;
        void append(const string_view &text) {
            if (!literalText) literalText = new string(literal);
            literalText->append(text);
        }

        void append(int n) {
            append(to_string(n));
        }

        virtual ~TypeLiteral() {
            delete literalText;
        }

        string_view text() {
            return literalText ? *literalText : literal;
        }
        TypeLiteral() {}

        TypeLiteral(const string_view literal, TypeLiteralType type): literal(literal), type(type) {}
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

    template<class T>
    sharedOpt<T> to(const sharedOpt<Type> &p) {
        if (!p) return nullptr;
        if (T::KIND != TypeKind::Unknown && p->kind != T::KIND) return nullptr;
        return reinterpret_pointer_cast<T>(p);
    }

    string stringify(shared<Type> type) {
        //todo: recursive types

        switch (type->kind) {
            case TypeKind::Never: return "never";
            case TypeKind::Void: return "void";
            case TypeKind::Unknown: return "unknown";
            case TypeKind::Undefined: return "undefined";
            case TypeKind::Null: return "null";
            case TypeKind::String: return "string";
            case TypeKind::Number: return "number";
            case TypeKind::Boolean: return "boolean";
            case TypeKind::Bigint: return "bigint";
            case TypeKind::Rest: return "..." + stringify(to<TypeRest>(type)->type);
            case TypeKind::TemplateLiteral: {
                return "nop";
            }
            case TypeKind::Literal: {
                auto literal = to<TypeLiteral>(type);
                switch (literal->type) {
                    case TypeLiteralType::String: return string("\"").append(literal->text()).append("\"");
                    case TypeLiteralType::Number: return string(literal->text());
                    case TypeLiteralType::Bigint: return string(literal->text());
                    case TypeLiteralType::Boolean: return string(literal->text());
                }
                return "unknown-literal";
            }
            case TypeKind::TupleMember: {
                auto member = to<TypeTupleMember>(type);
                string r = "";
                if (member->name != "") {
                    r += member->name;
                    if (member->optional) r += "?";
                    r += ": ";
                    r += stringify(member->type);
                } else {
                    if (member->optional) r += "?";
                    r += stringify(member->type);
                }
                return r;
            }
            case TypeKind::Tuple: {
                auto tuple = to<TypeTuple>(type);
                string r = "[";
                for (auto &e: tuple->types) {
                    r += stringify(e);
                    if (e != tuple->types.back()) r += ", ";
                }
                r += "]";
                return r;
            }
            case TypeKind::Union: {
                auto t = to<TypeUnion>(type);
                auto i = 0;
                string r = "";
                for (auto &&v: t->types) {
                    if (i > 0) r += " | ";
                    r += stringify(v);
                    i++;
                }
                return r;
            }
        }

        return "error";
    }

    shared<Type> unboxUnion(shared<Type> type) {
        if (type->kind == TypeKind::Union) {
            auto types = to<TypeUnion>(type)->types;
            if (types.size() == 0) return make_shared<TypeNever>(); //{ kind: ReflectionKind.never };
            if (types.size() == 1) return types[0];
        }
        return type;
    }
}

template<>
struct fmt::formatter<ts::vm::TypeKind>: formatter<std::string_view> {
    template<typename FormatContext>
    auto format(ts::vm::TypeKind p, FormatContext &ctx) {
        return formatter<string_view>::format(magic_enum::enum_name(p), ctx);
    }
};
