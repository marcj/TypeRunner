#pragma once

#include <string>
#include <functional>
#include <array>
#include <vector>
#include "../enum.h"
#include "../hash.h"

namespace ts::vm2 {
    using std::string;
    using std::string_view;

    enum class TypeKind: unsigned int {
        Unknown,
        Never,
        Any,
        Null,
        Undefined,
        String,
        Number,
        BigInt,
        Boolean,
        Symbol,
        Literal,
        IndexSignature,
        Method,
        MethodSignature,
        Property,
        PropertySignature,
        Class,
        ClassInstance,
        ObjectLiteral,
        Union,
        Array,
        Rest,
        Tuple,
        TupleMember,
        TemplateLiteral,
        Parameter,
        Function,
        FunctionRef,
    };

    //Used in the vm
    enum TypeFlag: unsigned int {
        Readonly = 1<<0,
        Optional = 1<<1,
        StringLiteral = 1<<2,
        NumberLiteral = 1<<3,
        BooleanLiteral = 1<<4,
        BigIntLiteral = 1<<5,
        True = 1<<6,
        False = 1<<7,
        Stored = 1<<8, //Used somewhere as cache or as value (subroutine->result for example), and thus can not be stolen/modified
        RestReuse = 1<<9, //allow to reuse/steal T in ...T
        Deleted = 1<<10, //for debugging purposes
        Static = 1<<11,
    };

    struct Type;

    inline constexpr auto defaultTypesSize = 1;
    struct Types {
        std::array<Type *, defaultTypesSize> list;
        unsigned int p;
//        std::vector<Type *> *extended = nullptr;
//
//        ~Types() {
//            delete extended;
//        }
//
//        void reserve(unsigned int size) {
//            if (p > 0) throw std::runtime_error("nope");
//
//            if (size>defaultTypesSize) {
//                if (extended) {
//                    extended->reserve(size);
//                } else {
//                    createExtended(size);
//                }
//            }
//        }
//
//        void createExtended(unsigned int size) {
//            if (!extended) {
//                extended = new std::vector<Type *>(size);
//                extended->insert(extended->begin(), list.begin(), list.end());
//            }
//        }

        void push(Type *type) {
//            if (extended) {
//                extended->push_back(type);
//            } else {
            if (p>defaultTypesSize) {
//                    createExtended(300);
//                    extended = new std::vector<Type *>(32);
//                    extended->insert(extended->begin(), list.begin(), list.end());
//                    extended->push_back(type);
            } else {
                list[p++] = type;
            }
//            }
        }

        Type **begin() {
//            if (extended) return &*extended->begin();
            return list.begin();
        }

        Type **end() {
//            if (extended) return &*extended->end();
            return list.begin() + p;
        }

        Type *back() {
            return list[p - 1];
        }

        unsigned int size() {
            return p;
        }
    };

    struct TypeRef {
        Type *type;
        TypeRef *next = nullptr;
        explicit TypeRef(): type(nullptr), next(nullptr) {}
        explicit TypeRef(Type *type, TypeRef *next = nullptr): type(type), next(next) {}
    };

    struct Type {
        TypeKind kind;

        //used to map type to sourcecode
        unsigned int ip;

        /** see TypeFlag */
        unsigned int flag = 0;

        //ref counter for garbage collection
        unsigned int refCount = 0;

        string_view text;
        uint64_t hash = 0;

        //used as address for FunctionRef
        unsigned int size = 0;

        //either Type* or TypeRef* or string* depending on kind
        void *type = nullptr;

        std::span<TypeRef> children;

        Type(TypeKind kind, uint64_t hash): kind(kind), hash(hash) {}

        ~Type() {
            if (kind == TypeKind::Literal && type) delete (string *) type;
        };

        bool isDeleted() {
            return flag & TypeFlag::Deleted;
        }

        void fromLiteral(Type *literal) {
            flag = literal->flag;
            if (literal->type) {
                //dynamic value, so copy it
                setDynamicText(literal->text, literal->hash);
            } else {
                //static value, safe reuse of string_view's reference
                text = literal->text;
                hash = literal->hash;
            }
        }

        /**
         * Returns true of there is only one child
         */
        bool singleChild() {
            return type && ((TypeRef *) type)->next == nullptr;
        }
        /**
         * Returns true of there is only one child
         */
        Type *child() {
            return type ? ((TypeRef *) type)->type : nullptr;
        }

        void appendLiteral(Type *literal) {
            appendText(literal->text);
        }

        void appendText(string_view value) {
            if (!type) {
                setDynamicText(value);
            } else {
                ((string *) type)->append(value);
                text = *(string *) type;
                hash = hash::runtime_hash(text);
            }
        }

        void setDynamicText(string_view value, uint64_t hash = 0) {
            if (!type) {
                type = new string(value);
            } else {
                ((string *) type)->clear();
                ((string *) type)->append(value);
            }
            text = *(string *) type;
            this->hash = hash ? hash : hash::runtime_hash(text);
        }

        void setDynamicLiteral(TypeFlag flag, string_view value) {
            this->flag |= flag;
            setDynamicText(value);
        }

        Type *setFlag(TypeFlag flag) {
            this->flag |= flag;
            return this;
        }

        Type *setLiteral(TypeFlag flag, string_view value) {
            this->flag |= flag;
            text = value;
            hash = hash::runtime_hash(text);
            return this;
        }

        void appendChild(TypeRef *ref) {
            if (!type) {
                type = ref;
            } else {
                ((TypeRef *) type)->next = ref;
            }
            size++;
        }

        void readStorage(const string_view &bin, uint32_t offset) {
            //offset points to the start of the storage entry: its structure is: hash+size+data;
            hash = vm::readUint64(bin, offset);
            offset += 8;
            text = vm::readStorage(bin, offset);
        }
    };

    inline Type *findChild(Type *type, uint64_t hash) {
        if (type->children.empty()) {
            auto current = (TypeRef *)type->type;
            while (current) {
                if (current->type->hash == hash) return current->type;
                current = current->next;
            }
            return nullptr;
        } else {
            TypeRef *entry = &type->children[hash % type->children.size()];
            if (!entry->type) return nullptr;
            while (entry && entry->type->hash != hash) {
                //step through linked collisions
                entry = entry->next;
            }
            return entry ? entry->type : nullptr;
        }
    }

    inline void forEachChild(Type *type, const std::function<void(Type *child, bool &stop)> &callback) {
        if (type->type) {
            auto stop = false;
            auto current = (TypeRef *)type->type;
            while (!stop && current) {
                callback(current->type, stop);
                current = current->next;
            }
        } else {
            auto stop = false;
            unsigned int i = 0;
            unsigned int end = type->children.size();
            while (!stop && i<end) {
                auto child = type->children[i];
                //bucket could be empty
                if (child.type) callback(child.type, stop);
                if (child.next) {
                    //has hash collision, execute them as well
                    auto current = child.next;
                    while (!stop && current) {
                        callback(current->type, stop);
                        current = current->next;
                    }
                }
                i++;
            }
        }
    }

    inline void forEachHashTableChild(Type *type, const std::function<void(Type *child, bool &stop)> &callback) {
        auto stop = false;
        unsigned int i = 0;
        unsigned int end = type->children.size();
        while (!stop && i<end) {
            auto child = type->children[i];
            //bucket could be empty
            if (child.type) callback(child.type, stop);
            if (child.next) {
                //has hash collision, execute them as well
                auto current = child.next;
                while (!stop && current) {
                    callback(current->type, stop);
                    current = current->next;
                }
            }
            i++;
        }
    }

    inline Type *getPropertyOrMethodName(Type *type) {
        return type->children[0].type;
    }

    inline Type *getPropertyOrMethodType(Type *type) {
        return type->children[1].type;
    }

    inline void stringifyType(Type *type, std::string &r) {
        switch (type->kind) {
            case TypeKind::Boolean: {
                r += "boolean";
                break;
            }
            case TypeKind::Number: {
                r += "number";
                break;
            }
            case TypeKind::String: {
                r += "string";
                break;
            }
            case TypeKind::Never: {
                r += "never";
                break;
            }
            case TypeKind::Any: {
                r += "any";
                break;
            }
            case TypeKind::Unknown: {
                r += "unknown";
                break;
            }
            case TypeKind::PropertySignature: {
                stringifyType(((TypeRef*)type->type)->type, r);
                r += ": ";
                stringifyType(((TypeRef*)type->type)->next->type, r);
                break;
            }
            case TypeKind::ObjectLiteral: {
                r += "{";
                unsigned int i = 0;
                forEachChild(type, [&i, &r](Type *child, auto stop) {
                    if (i++>20) {
                        r += "...";
                        stop = true;
                        return;
                    }
                    stringifyType(child, r);
                    r + "; ";
                });
                r += "}";
                break;
            }
            case TypeKind::TupleMember: {
                if (r.empty()) r += "TupleMember:";
                if (!type->text.empty()) {
                    r += string(type->text);
                    if (type->flag & TypeFlag::Optional) r += "?";
                    r += ": ";
                }
                if (!type->type) {
                    r += "UnknownTupleMember";
                } else {
                    stringifyType((Type *) type->type, r);
                }
                break;
            }
            case TypeKind::Array: {
                r += "Array<";
                stringifyType((Type *) type->type, r);
                r += ">";
                break;
            }
            case TypeKind::Rest: {
                r += "...";
                stringifyType((Type *) type->type, r);
                break;
            }
            case TypeKind::Parameter: {
                stringifyType((Type *) type->type, r);
                break;
            }
            case TypeKind::Tuple: {
                r += "[";
                auto current = (TypeRef *) type->type;
                unsigned int i = 0;
                while (current) {
                    if (i++>20) {
                        r += "...";
                        break;
                    }
                    stringifyType(current->type, r);
                    current = current->next;
                    if (current) r += ", ";
                }
                r += "]";
                break;
            }
            case TypeKind::Union: {
                auto current = (TypeRef *) type->type;
                unsigned int i = 0;
                while (current) {
                    if (i++>20) {
                        r += "...";
                        break;
                    }
                    stringifyType(current->type, r);
                    current = current->next;
                    if (current) r += " | ";
                }
                break;
            }
            case TypeKind::TemplateLiteral: {
                r += "`";
                auto current = (TypeRef *) type->type;
                while (current) {
                    if (current->type->kind != TypeKind::Literal) r += "${";
                    if (current->type->flag & TypeFlag::StringLiteral) {
                        r += current->type->text;
                    } else {
                        stringifyType(current->type, r);
                    }
                    if (current->type->kind != TypeKind::Literal) r += "}";
                    current = current->next;
                }
                r += "`";
                break;
            }
            case TypeKind::Literal: {
                if (type->flag & TypeFlag::StringLiteral) {
                    r += "\"";
                    r += type->text;
                    r += "\"";
                } else if (type->flag & TypeFlag::NumberLiteral) {
                    r += type->text;
                } else if (type->flag & TypeFlag::True) {
                    r += "true";
                } else if (type->flag & TypeFlag::False) {
                    r += "false";
                } else {
                    r += "UnknownLiteral";
                }
                break;
            }
            default: {
                r += "*notStringified*";
            }
        }
    }

    inline string stringify(Type *type) {
        std::string r;
        stringifyType(type, r);
        return r;
    }

    inline bool isOptional(Type *type) {
        return type->flag & TypeFlag::Optional;
    }

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
struct fmt::formatter<ts::vm2::TypeKind>: formatter<string_view> {
    template<typename FormatContext>
    auto format(ts::vm2::TypeKind p, FormatContext &ctx) {
        return formatter<string_view>::format(magic_enum::enum_name(p), ctx);
    }
};
