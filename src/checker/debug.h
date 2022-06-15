#pragma once

#include <string>
#include "./instructions.h"
#include "../core.h"
#include "./utils.h"

namespace ts::checker {
    using std::string_view;
    using ts::instructions::OP;

    inline string_view readStorage(string_view &ops, unsigned int address) {
        auto size = readUint16(ops, address);
        return ops.substr(address + 2, size);
    }

    struct PrintSubroutine {
        string_view name;
        unsigned int address;
    };

    inline void printBin(string_view bin) {
        const auto end = bin.size();
        unsigned int storageEnd = 0;
        bool newSubRoutine = false;
        vector<PrintSubroutine> subroutines;

        for (unsigned int i = 0; i < end; i++) {
            if (storageEnd) {
                while (i < storageEnd) {
                    auto size = readUint16(bin, i);
                    auto data = bin.substr(i + 2, size);
                    fmt::print("(Storage ({})\"{}\") ", size, data);
                    i += 2 + size;
                }
                debug("");
                storageEnd = 0;
            }

            if (newSubRoutine) {
                auto found = false;
                unsigned int j = 0;
                for (auto &&r: subroutines) {
                    if (r.address == i) {
                        fmt::print("\n&{} {}(): ", j, r.name);
                        found = true;
                        break;
                    }
                    j++;
                }
                if (!found) {
                    fmt::print("\nunknown!(): ");
                }
                newSubRoutine = false;
            }
            std::string params = "";
            auto op = (OP) bin[i];

            switch (op) {
                case OP::Call: {
                    params += fmt::format(" &{}[{}]", readUint32(bin, i + 1), readUint16(bin, i + 5));
                    i += 6;
                    break;
                }
                case OP::Subroutine: {
                    auto nameAddress = readUint32(bin, i + 1);
                    auto address = readUint32(bin, i + 5);
                    string_view name = nameAddress ? readStorage(bin, nameAddress) : "";
                    params += fmt::format(" {}[{}]", name, address);
                    i += 8;
                    subroutines.push_back({.name = name, .address = address});
                    break;
                }
                case OP::Main:
                case OP::Jump: {
                    auto address = readUint32(bin, i + 1);
                    params += fmt::format(" &{}", address);
                    i += 4;
                    if (op == OP::Jump) {
                        storageEnd = address;
                    } else {
                        subroutines.push_back({.name = "main", .address = address});
                        newSubRoutine = true;
                    }
                    break;
                }
                case OP::Return: {
                    newSubRoutine = true;
                    break;
                }
                case OP::JumpCondition: {
                    params += fmt::format(" &{}:&{}", readUint16(bin, i + 1), readUint16(bin, i + 3));
                    i += 4;
                    break;
                }
                case OP::TypeArgumentDefault:
                case OP::Distribute: {
                    params += fmt::format(" &{}", readUint32(bin, i + 1));
                    i += 4;
                    break;
                }
                case OP::Loads: {
                    params += fmt::format(" &{}:{}", readUint16(bin, i + 1), readUint16(bin, i + 3));
                    i += 4;
                    break;
                }
                case OP::Parameter:
                case OP::NumberLiteral:
                case OP::BigIntLiteral:
                case OP::StringLiteral: {
                    params += fmt::format(" \"{}\"", readStorage(bin, readUint32(bin, i + 1)));
                    i += 4;
                    break;
                }
            }

            if (params.empty()) {
                fmt::print("{} ", op);
            } else {
                fmt::print("({}{}) ", op, params);
            }
        }
        fmt::print("\n");
    }
}