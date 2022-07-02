#pragma once

#include <utility>
#include <vector>
#include <string>

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

    inline void writeUint32(vector<unsigned char> &bin, unsigned int offset, uint32_t value) {
        if (offset + 4 > bin.size()) bin.resize(bin.size() + 4);
        *(uint32_t *) (bin.data() + offset) = value;
    }

    inline void writeUint64(vector<unsigned char> &bin, unsigned int offset, uint64_t value) {
        if (offset + 8 > bin.size()) bin.resize(bin.size() + 8);
        *(uint64_t *) (bin.data() + offset) = value;
    }

    inline uint16_t readUint16(const vector<unsigned char> &bin, unsigned int offset) {
        return *(uint16_t *) (bin.data() + offset);
    }

    inline uint16_t readUint16(const string_view &bin, unsigned int offset) {
        return *(uint16_t *) (bin.begin() + offset);
    }

    inline void writeUint16(vector<unsigned char> &bin, unsigned int offset, uint16_t value) {
        if (offset + 2 > bin.size()) bin.resize(bin.size() + 2);
        *(uint16_t *) (bin.data() + offset) = value;
    }

    inline string_view readStorage(const string_view &bin, const uint32_t offset) {
        const auto size = readUint16(bin, offset);
        return string_view(reinterpret_cast<const char *>(bin.data() + offset + 2), size);
    }
}