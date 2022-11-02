#pragma once

#include "../core.h"
#include "./utils.h"
#include "../utf.h"
#include "../hash.h"
#include "../enum.h"
#include <string>
#include <array>
#include <utility>
#include <vector>
#include <memory>
#include <unordered_set>

namespace tr::checker {
}

namespace tr::vm {
    using std::string_view;
    using std::array;
    using std::to_string;
    using std::make_shared;
    using std::reference_wrapper;
    using tr::utf::eatWhitespace;
    using namespace tr::checker;

    struct HashString {
        uint64_t hash;
//        uint64_t i;
        string_view text;

        HashString() { }

        HashString(const string_view &text): text(text) {
            hash = hash::runtime_hash(text);
        }

        string getString() {
            return string(text);
        }

        bool operator ==(HashString &other) {
            return getHash() == other.getHash();
        }

        uint64_t getHash() {
            return hash;
        }
    };

    enum class TypeKind: int {
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
        FunctionRef, //a function with generics which can be instantiated
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

        //this is the OP position (instruction pointer) of the bytecode. sourcemap is necessary to map it to source positions.
        unsigned int ip{};

        virtual ~Type() {
        }
    };

//todo make configurable
//#define TS_PROFILE

    struct ProfilerData {
        vector<vector<long long>> comparisons;
        vector<long long> instantiations;
        unsigned int typeCount = magic_enum::enum_count<TypeKind>();

        ProfilerData() {
            instantiations.resize(typeCount);
            comparisons.resize(typeCount);
            for (auto &&c: comparisons) c.resize(typeCount);
        }
    };
    struct Profiler {
#ifdef TS_PROFILE
        ProfilerData data;
#endif

        void clear() {
#ifdef TS_PROFILE
            data = ProfilerData();
#endif
        }

        void instantiate(TypeKind kind) {
#ifdef TS_PROFILE
            data.instantiations[(int)kind]++;
#endif
        }

        void compare(const node<Type> &left, const node<Type> &right) {
#ifdef TS_PROFILE
            data.comparisons[(int)left->kind][(int)right->kind]++;
#endif
        }
    };

    static Profiler profiler;

    template<TypeKind T, class ... Base>
    struct BrandKind: Base ... {
        constexpr static const TypeKind KIND = T;
        BrandKind() {
            this->kind = T;
            profiler.instantiate(T);
        }
    };

    struct Module;

    template<class T>
    optionalNode<T> to(const optionalNode<Type> &p) {
        if (!p) return nullptr;
        if (T::KIND != TypeKind::Unknown && p->kind != T::KIND) return nullptr;
        return reinterpret_pointer_cast<T>(p);
    }

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
        vector<node<Type>> types;

        explicit TypeTemplateLiteral() {}
        explicit TypeTemplateLiteral(const vector<node<Type>> &types): types(types) {}
    };

    struct TypeRest: BrandKind<TypeKind::Rest, Type> {
        node<Type> type;
        explicit TypeRest(node<Type> type): type(std::move(type)) {
        }
    };

    struct TypeArray: BrandKind<TypeKind::Array, Type> {
        node<Type> type;
        explicit TypeArray(node<Type> type): type(std::move(type)) {
        }
    };

    struct TypeTupleMember: BrandKind<TypeKind::TupleMember, Type> {
        node<Type> type;
        bool optional = false;
        string_view name = "";

        explicit TypeTupleMember() {
        }

        explicit TypeTupleMember(node<Type> type): type(std::move(type)) {
        }
    };

    struct TypeTuple: BrandKind<TypeKind::Tuple, Type> {
        vector<node<TypeTupleMember>> types;
    };

    enum class TypeLiteralType {
        Number,
        String,
        Boolean,
        Bigint
    };

    struct TypeLiteral: BrandKind<TypeKind::Literal, Type> {
    public:
        string *dynamicString = nullptr;
        HashString literal;
        TypeLiteralType type;

        TypeLiteral() {}

        explicit TypeLiteral(const HashString literal, TypeLiteralType type): literal(literal), type(type) {

        }

        explicit TypeLiteral(const string_view &literal, TypeLiteralType type): literal(HashString(literal)), type(type) {

        }

        void append(const string_view &text) {
            if (!dynamicString) {
                dynamicString = new string(literal.text);
            }
            dynamicString->append(text);
            literal = HashString(*dynamicString);
        }

        void append(int n) {
            append(to_string(n));
        }

        virtual ~TypeLiteral() {
            delete dynamicString;
        }

        bool equal(const node<TypeLiteral> &other) {
            return type == other->type && literal.getHash() == other->literal.getHash();
//            if (type != other->type) return false;
//            if (!literalText && !other->literalText) return literal == other->literal;
//            if (!literalText && other->literalText) return literal == *other->literalText;
//            if (literalText && !other->literalText) return *literalText == other->literal;
//            return *literalText == *other->literalText;
        }

        string_view text() {
            return dynamicString ? *dynamicString : literal.text;
        }
    };

    struct TypeUnion: BrandKind<TypeKind::Union, Type> {
        vector<node<Type>> types;
        bool indexed = false;
        bool onlyLiterals = false;
        std::unordered_set<uint64_t> literalIndex;

        void index() {
            onlyLiterals = true;
            for (auto &&t: types) {
                if (t->kind != TypeKind::Literal) {
                    onlyLiterals = false;
                    break;
                }
            }
            if (onlyLiterals) {
                for (auto &&t: types) {
                    literalIndex.insert(to<TypeLiteral>(t)->literal.hash);
                }
            }
        }

        bool fastLiteralCheck() {
            if (!indexed) index();
            return onlyLiterals;
        }

        bool includes(const node<Type> &type) {
            if (type->kind == TypeKind::Literal) {
                return literalIndex.contains(to<TypeLiteral>(type)->literal.hash);
            }

            return false;
        }
    };

    struct TypeParameter: BrandKind<TypeKind::Parameter, Type> {
        string_view name; //number, string, symbol
        node<Type> type;
        optionalNode<Type> initializer = nullptr;
        bool optional = false;
        TypeParameter(const string_view &name, const node<tr::vm::Type> &type): name(name), type(type) {}
    };

    struct TypeFunction: BrandKind<TypeKind::Function, Type> {
        string_view name; //number, string, symbol
        vector<node<TypeParameter>> parameters;
        node<Type> returnType;
    };

    struct TypeFunctionRef: BrandKind<TypeKind::FunctionRef, Type> {
        unsigned int address;
        explicit TypeFunctionRef(unsigned int address): address(address) {}
    };

    struct TypeProperty: BrandKind<TypeKind::Property, Type> {
        HashString name; //todo change to something that is number | string | symbol, same for MethodSignature
        bool optional = false;
        bool readonly = false;
        node<Type> type;
        TypeProperty(const node<Type> &type): type(type) {}
    };

    struct TypePropertySignature: BrandKind<TypeKind::PropertySignature, Type> {
        HashString name; //todo change to something that is number | string | symbol, same for MethodSignature
        bool optional = false;
        bool readonly = false;
        node<Type> type;
        TypePropertySignature() {}
        TypePropertySignature(const node<Type> &type): type(type) {}
    };

    struct TypeMethodSignature: BrandKind<TypeKind::MethodSignature, Type> {
        HashString name;
        bool optional = false;
        vector<node<Type>> parameters;
        node<Type> returnType;
    };

    struct TypeMethod: BrandKind<TypeKind::MethodSignature, Type> {
        HashString name;
        bool optional = false;
        vector<node<Type>> parameters;
        node<Type> returnType;
    };

    struct TypeIndexSignature: BrandKind<TypeKind::IndexSignature, Type> {
        node<Type> index;
        node<Type> type;
    };

    struct TypeObjectLiteral: BrandKind<TypeKind::ObjectLiteral, Type> {
        vector<node<Type>> types; //TypeIndexSignature | TypePropertySignature | TypeMethodSignature
        explicit TypeObjectLiteral() {}
        explicit TypeObjectLiteral(vector<node<Type>> types): types(types) {}
    };

    string stringify(node<Type> type) {
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
            case TypeKind::Parameter: {
                auto n = to<TypeParameter>(type);
                string r = string(n->name);
                if (n->optional) r += "?";
                r += ": " + stringify(n->type);
                return r;
            }
            case TypeKind::PropertySignature: {
                auto n = to<TypePropertySignature>(type);
                string r = (n->readonly ? "readonly " : "") + n->name.getString();
                if (n->optional) r += "?";
                return r + ": " + stringify(n->type);
            }
            case TypeKind::ObjectLiteral: {
                auto n = to<TypeObjectLiteral>(type);
                string r = string(n->typeName) + "{";
                for (auto &&member: n->types) {
                    r += stringify(member);
                    if (member != n->types.back()) r += ", ";
                }
                return r + "}";
            }
            case TypeKind::FunctionRef: {
                return "%FunctionRef";
            }
            case TypeKind::Function: {
                auto fn = to<TypeFunction>(type);
                string r = string(fn->name) + "(";
                for (auto &p: fn->parameters) {
                    r += stringify(p);
                    if (p != fn->parameters.back()) r += ", ";
                }
                r += ") => ";
                r += stringify(fn->returnType);
                return r;
            }
            case TypeKind::Array: {
                return "Array<" + stringify(to<TypeArray>(type)->type) + ">";
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

        return fmt::format("error-{}", type->kind);
    }

    node<Type> unboxUnion(node<Type> type) {
        if (type->kind == TypeKind::Union) {
            auto types = to<TypeUnion>(type)->types;
            if (types.size() == 0) return make_shared<TypeNever>(); //{ kind: ReflectionKind.never };
            if (types.size() == 1) return types[0];
        }
        return type;
    }

    bool isOptional(node<Type> type) {
        switch (type->kind) {
            case TypeKind::Union: {
                auto a = to<TypeUnion>(type);
                for (auto &&item: a->types) {
                    if (item->kind == TypeKind::Undefined) return true;
                }
                return false;
            }
            case TypeKind::Parameter: {
                auto a = to<TypeParameter>(type);
                if (a->optional) return true;
                return isOptional(a->type);
            }
            case TypeKind::Property: {
                auto a = to<TypeProperty>(type);
                if (a->optional) return true;
                return isOptional(a->type);
            }
            case TypeKind::PropertySignature: {
                auto a = to<TypePropertySignature>(type);
                if (a->optional) return true;
                return isOptional(a->type);
            }
        }
        return false;
    }

    node<Type> widen(node<Type> type) {
        switch (type->kind) {
            case TypeKind::Literal: {
                auto a = to<TypeLiteral>(type);
                switch (a->type) {
                    case TypeLiteralType::String: return make_shared<TypeString>();
                    case TypeLiteralType::Number: return make_shared<TypeNumber>();
                    case TypeLiteralType::Bigint: return make_shared<TypeBigint>();
                    case TypeLiteralType::Boolean: return make_shared<TypeBoolean>();
                }
            }
        }

        return type;
    }
}

template<>
struct fmt::formatter<tr::vm::TypeKind>: formatter<std::string_view> {
    template<typename FormatContext>
    auto format(tr::vm::TypeKind p, FormatContext &ctx) {
        return formatter<string_view>::format(magic_enum::enum_name(p), ctx);
    }
};
