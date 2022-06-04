#pragma once

#include <utility>
#include <vector>

namespace ts::checker {
    using std::vector;

    uint32_t readUint32(const vector<unsigned char> &bin, unsigned int offset) {
        return *(uint32_t *) (bin.data() + offset);
    }

    void writeUint32(vector<unsigned char> &bin, unsigned int offset, uint32_t value) {
        if (offset + 4 > bin.size()) bin.resize(bin.size() + 4);
        *(uint32_t *) (bin.data() + offset) = value;
    }

    uint16_t readUint16(const vector<unsigned char> &bin, unsigned int offset) {
        return *(uint16_t *) (bin.data() + offset);
    }

    void writeUint16(vector<unsigned char> &bin, unsigned int offset, uint16_t value) {
        if (offset + 2 > bin.size()) bin.resize(bin.size() + 2);
        *(uint16_t *) (bin.data() + offset) = value;
    }
}