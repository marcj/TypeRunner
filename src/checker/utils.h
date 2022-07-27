#pragma once

#include <utility>
#include <vector>
#include <string>
#include "./instructions.h"

namespace ts::vm {
    using std::vector;
    using std::string_view;

    inline uint64_t readUint64(const vector<unsigned char> &bin, unsigned int offset) {
        return *(uint64_t *) (bin.data() + offset);
    }

    inline uint64_t readUint64(const string_view &bin, unsigned int offset) {
        return *(uint64_t *) (bin.data() + offset);
    }

    inline uint32_t readUint32(const vector<unsigned char> &bin, unsigned int offset) {
        return *(uint32_t *) (bin.data() + offset);
    }

    inline uint32_t readUint32(const string_view &bin, unsigned int offset) {
        return *(uint32_t *) (bin.begin() + offset);
    }

    inline int32_t readInt32(const string_view &bin, unsigned int offset) {
        return *(int32_t *) (bin.begin() + offset);
    }

    inline void writeUint32(vector<unsigned char> &bin, unsigned int offset, uint32_t value) {
        if (offset + 4>bin.size()) bin.resize(bin.size() + 4);
        *(uint32_t *) (bin.data() + offset) = value;
    }

    inline void writeInt32(vector<unsigned char> &bin, unsigned int offset, int32_t value) {
        if (offset + 4>bin.size()) bin.resize(bin.size() + 4);
        *(int32_t *) (bin.data() + offset) = value;
    }

    inline void writeUint64(vector<unsigned char> &bin, unsigned int offset, uint64_t value) {
        if (offset + 8>bin.size()) bin.resize(bin.size() + 8);
        *(uint64_t *) (bin.data() + offset) = value;
    }

    inline uint16_t readUint16(const vector<unsigned char> &bin, unsigned int offset) {
        return *(uint16_t *) (bin.data() + offset);
    }

    inline uint16_t readUint16(const string_view &bin, unsigned int offset) {
        return *(uint16_t *) (bin.begin() + offset);
    }

    inline void writeUint16(vector<unsigned char> &bin, unsigned int offset, uint16_t value) {
        if (offset + 2>bin.size()) bin.resize(bin.size() + 2);
        *(uint16_t *) (bin.data() + offset) = value;
    }

    inline string_view readStorage(const string_view &bin, const uint32_t offset) {
        const auto size = readUint16(bin, offset);
        return string_view(reinterpret_cast<const char *>(bin.data() + offset + 2), size);
    }

    using ts::instructions::OP;
    inline void eatParams(OP op, unsigned int *i) {
        switch (op) {
            case OP::TailCall:
            case OP::Call: {
                *i += 6;
                break;
            }
            case OP::Subroutine: {
                *i += 4 + 4 + 1;
                break;
            }
            case OP::Main: {
                break;
            }
            case OP::Jump: {
                *i += 4;
                break;
            }
            case OP::JumpCondition: {
                *i += 4;
                break;
            }
            case OP::Distribute: {
                *i += 2 + 4;
                break;
            }
            case OP::Set:
            case OP::TypeArgumentDefault: {
                *i += 4;
                break;
            }
            case OP::FunctionRef: {
                *i += 4;
                break;
            }
            case OP::Instantiate: {
                *i += 2;
                break;
            }
            case OP::Error: {
                *i += 2;
                break;
            }
            case OP::Union:
            case OP::Tuple:
            case OP::TemplateLiteral:
            case OP::ObjectLiteral:
            case OP::Slots:
            case OP::CallExpression: {
                *i += 2;
                break;
            }
            case OP::Loads: {
                *i += 2;
                break;
            }
            case OP::Parameter:
            case OP::NumberLiteral:
            case OP::BigIntLiteral:
            case OP::StringLiteral: {
                *i += 4;
                break;
            }
        }
    }
}