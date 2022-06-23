#pragma once

#include "../core.h"
#include "./utils.h"
#include "../utf.h"
#include <string>
#include <array>
#include <utility>
#include <vector>
#include <memory>

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 512

#include "magic_enum.hpp"

namespace ts::checker {
}

namespace ts::vm {
    using std::string_view;
    using std::array;
    using std::to_string;
    using std::make_shared;
    using std::reference_wrapper;
    using namespace ts::checker;

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

    struct ModuleSubroutine {
        string_view name;
        unsigned int address;
        bool exported = false;
        sharedOpt<Type> result = nullptr;
        sharedOpt<Type> narrowed = nullptr; //when control flow analysis sets a new value
        ModuleSubroutine(string_view name, unsigned int address): name(name), address(address) {}
    };

    struct Module;

    struct DiagnosticMessage {
        string message;
        unsigned int ip; //ip of the node/OP
        shared<Module> module = nullptr;
        DiagnosticMessage() {}
        explicit DiagnosticMessage(const string &message, unsigned int &ip): message(message), ip(ip) {}
    };

    struct FoundSourceMap {
        unsigned int pos;
        unsigned int end;

        bool found() {
            return pos != 0 && end != 0;
        }
    };

    struct FoundSourceLineCharacter {
        unsigned int line;
        unsigned int pos;
        unsigned int end;
    };

    /**
     * const     v2: a<number> = "as"
     *      ^-----^ this is the identifier source map.
     *
     * This function makes sure map.pos starts at `v`, basically eating whitespace.
     */
    inline void omitWhitespace(const string &code, FoundSourceMap &map) {
        map.pos = eatWhitespace(code, map.pos);
    }

    struct Module {
        const string bin;
        string fileName = "index.ts";
        const string code = ""; //for diagnostic messages only

        vector<ModuleSubroutine> subroutines;
        unsigned int mainAddress;
        unsigned int sourceMapAddress;
        unsigned int sourceMapAddressEnd;

        vector<DiagnosticMessage> errors;
        Module() {}

        Module(const string_view &bin, const string &fileName, const string &code): bin(bin), fileName(fileName), code(code) {
        }

        void clear() {
            errors.clear();
            subroutines.clear();
        }

        ModuleSubroutine *getSubroutine(unsigned int index) {
            return &subroutines[index];
        }

        ModuleSubroutine *getMain() {
            return &subroutines.back();
        }

        string findIdentifier(unsigned int ip) {
            auto map = findNormalizedMap(ip);
            if (!map.found()) return "";
            return code.substr(map.pos, map.end - map.pos);
        }

        FoundSourceMap findMap(unsigned int ip) {
            unsigned int found = 0;
            for (unsigned int i = sourceMapAddress; i < sourceMapAddressEnd; i += 3 * 4) {
                auto mapIp = readUint32(bin, i);
                if (mapIp == ip) {
                    found = i;
                    break;
                }
//                if (mapIp > ip) break;
//                found = i;
            }

            if (found) {
                return {readUint32(bin, found + 4), readUint32(bin, found + 8)};
            }
            return {0, 0};
        }

        FoundSourceMap findNormalizedMap(unsigned int ip) {
            auto map = findMap(ip);
            if (map.found()) omitWhitespace(code, map);
            return map;
        }

        /**
         * Converts FindSourceMap{x,y} to
         */
        FoundSourceLineCharacter mapToLineCharacter(FoundSourceMap map) {
            unsigned int pos = 0;
            unsigned int line = 0;
            while (pos < map.pos) {
                std::size_t lineStart = code.find('\n', pos);
                if (lineStart == std::string::npos) {
                    return {.line = line, .pos = map.pos - pos, .end = map.end - pos};
                } else if (lineStart > map.pos) {
                    //dont overshot
                    break;
                }
                pos = lineStart + 1;
                line++;
            }
            return {.line = line, .pos = map.pos - pos, .end = map.end - pos};
        }

        void printErrors() {
            for (auto &&e: errors) {
                if (e.ip) {
                    auto map = findNormalizedMap(e.ip);

                    if (map.found()) {
                        std::size_t lineStart = code.rfind('\n', map.pos);
                        lineStart = lineStart == std::string::npos ? 0 : lineStart + 1;

                        std::size_t lineEnd = code.find('\n', map.end);
                        if (lineEnd == std::string::npos) lineEnd = code.size();
                        std::cout << cyan << fileName << ":" << yellow << map.pos << ":" << map.end << reset << " - " << red << "error" << reset << " TS0000: " << e.message << "\n\n";
                        std::cout << code.substr(lineStart, lineEnd - lineStart - 1) << "\n";
                        auto space = map.pos - lineStart;
                        std::cout << std::string(space, ' ') << red << "^" << reset << "\n\n";
                        continue;
                    }
                }
                std::cout << "  " << e.message << "\n";
            }
            std::cout << "Found " << errors.size() << " errors in " << fileName << "\n";
        }
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
        shared<Type> type;
        sharedOpt<Type> initializer = nullptr;
        bool optional = false;
        TypeParameter(const string_view &name, const shared<ts::vm::Type> &type): name(name), type(type) {}
    };

    struct TypeFunction: BrandKind<TypeKind::Function, Type> {
        string_view name; //number, string, symbol
        vector<shared<TypeParameter>> parameters;
        shared<Type> returnType;
    };

    struct TypeFunctionRef: BrandKind<TypeKind::FunctionRef, Type> {
        unsigned int address;
        explicit TypeFunctionRef(unsigned int address): address(address) {}
    };

    struct TypeProperty: BrandKind<TypeKind::Property, Type> {
        string_view name; //todo change to something that is number | string | symbol, same for MethodSignature
        bool optional = false;
        bool readonly = false;
        shared<Type> type;
        TypeProperty(const shared<Type> &type): type(type) {}
    };

    struct TypePropertySignature: BrandKind<TypeKind::PropertySignature, Type> {
        string_view name; //todo change to something that is number | string | symbol, same for MethodSignature
        bool optional = false;
        bool readonly = false;
        shared<Type> type;
        TypePropertySignature(const shared<Type> &type): type(type) {}
    };

    struct TypeMethodSignature: BrandKind<TypeKind::MethodSignature, Type> {
        string_view name;
        bool optional = false;
        vector<shared<Type>> parameters;
        shared<Type> returnType;
    };

    struct TypeMethod: BrandKind<TypeKind::MethodSignature, Type> {
        string_view name;
        bool optional = false;
        vector<shared<Type>> parameters;
        shared<Type> returnType;
    };

    struct TypeIndexSignature: BrandKind<TypeKind::IndexSignature, Type> {
        shared<Type> index;
        shared<Type> type;
    };

    struct TypeObjectLiteral: BrandKind<TypeKind::ObjectLiteral, Type> {
        vector<shared<Type>> types; //TypeIndexSignature | TypePropertySignature | TypeMethodSignature
        explicit TypeObjectLiteral() {}
        explicit TypeObjectLiteral(vector<shared<Type>> types): types(types) {}
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
            case TypeKind::Parameter: {
                auto n = to<TypeParameter>(type);
                string r = string(n->name);
                if (n->optional) r += "?";
                r += ": " + stringify(n->type);
                return r;
            }
            case TypeKind::PropertySignature: {
                auto n = to<TypePropertySignature>(type);
                string r = (n->readonly ? "readonly " : "") + string(n->name);
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

    shared<Type> unboxUnion(shared<Type> type) {
        if (type->kind == TypeKind::Union) {
            auto types = to<TypeUnion>(type)->types;
            if (types.size() == 0) return make_shared<TypeNever>(); //{ kind: ReflectionKind.never };
            if (types.size() == 1) return types[0];
        }
        return type;
    }

    bool isOptional(shared<Type> type) {
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

    shared<Type> widen(shared<Type> type) {
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
struct fmt::formatter<ts::vm::TypeKind>: formatter<std::string_view> {
    template<typename FormatContext>
    auto format(ts::vm::TypeKind p, FormatContext &ctx) {
        return formatter<string_view>::format(magic_enum::enum_name(p), ctx);
    }
};
