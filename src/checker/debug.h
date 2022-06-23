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

    struct PrintSubroutineOp {
        string text;
        unsigned int address;
    };

    struct PrintSubroutine {
        string name;
        unsigned int address;
        vector<PrintSubroutineOp> operations;
    };

    struct DebugSourceMapEntry {
        OP op;
        unsigned int bytecodePos;
        unsigned int sourcePos;
        unsigned int sourceEnd;
    };

    struct DebugBinResult {
        vector<string> operations;
        vector<string> storages;
        vector<PrintSubroutine> subroutines;
        vector<DebugSourceMapEntry> sourceMap;
        PrintSubroutine *activeSubroutine = nullptr;
    };

    inline DebugBinResult parseBin(string_view bin, bool print = false) {
        const auto end = bin.size();
        unsigned int storageEnd = 0;
        bool newSubRoutine = false;
        DebugBinResult result;
        if (print) fmt::print("Bin {} bytes: ", bin.size());

        for (unsigned int i = 0; i < end; i++) {
            if (storageEnd) {
                while (i < storageEnd) {
                    auto size = readUint16(bin, i);
                    auto data = bin.substr(i + 2, size);
                    if (print) fmt::print("(Storage ({})\"{}\") ", size, data);
                    result.storages.push_back(string(data));
                    i += 2 + size;
                }
                debug("");
                storageEnd = 0;
            }

            if (newSubRoutine) {
                auto found = false;
                unsigned int j = 0;
                for (auto &&r: result.subroutines) {
                    if (r.address == i) {
                        if (print) fmt::print("\n&{} {}(): ", j, r.name);
                        result.activeSubroutine = &r;
                        found = true;
                        break;
                    }
                    j++;
                }
                if (!found) {
                    if (print) fmt::print("\nunknown!(): ");
                }
                newSubRoutine = false;
            }
            std::string params = "";
            auto startI = i;
            auto op = (OP) bin[i];

            switch (op) {
                case OP::Call: {
                    params += fmt::format(" &{}[{}]", readUint32(bin, i + 1), readUint16(bin, i + 5));
                    i += 6;
                    break;
                }
                case OP::SourceMap: {
                    auto size = readUint32(bin, i + 1);
                    auto start = i + 1;
                    i += 4 + size;
                    params += fmt::format(" {}->{} ({})", start, i, size / (4 * 3)); //each entry has 3x 4bytes (uint32)

                    for (unsigned int j = start + 4; j < i; j += 4 * 3) {
                        DebugSourceMapEntry sourceMapEntry{
                                                                   .op = (OP)(bin[readUint32(bin, j)]),
                                                                   .bytecodePos = readUint32(bin, j),
                                                                   .sourcePos = readUint32(bin, j + 4),
                                                                   .sourceEnd =  readUint32(bin, j + 8),
                                                           };
                        result.sourceMap.push_back(sourceMapEntry);
                        if (print) debug("Map [{}]{} to {}:{}", sourceMapEntry.bytecodePos, sourceMapEntry.op, sourceMapEntry.sourcePos, sourceMapEntry.sourceEnd);
                    }
                    break;
                }
                case OP::Subroutine: {
                    auto nameAddress = readUint32(bin, i + 1);
                    auto address = readUint32(bin, i + 5);
                    string name = nameAddress ? string(readStorage(bin, nameAddress)) : "";
                    params += fmt::format(" {}[{}]", name, address);
                    i += 8;
                    result.subroutines.push_back({.name = name, .address = address});
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
                        result.subroutines.push_back({.name = "main", .address = address});
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
                case OP::Set:
                case OP::TypeArgumentDefault:
                case OP::Distribute: {
                    params += fmt::format(" &{}", readUint32(bin, i + 1));
                    i += 4;
                    break;
                }
                case OP::FunctionRef: {
                    params += fmt::format(" &{}", readUint32(bin, i + 1));
                    i += 4;
                    break;
                }
                case OP::Instantiate: {
                    params += fmt::format(" {}", readUint16(bin, i + 1));
                    i += 2;
                    break;
                }
                case OP::Error: {
                    params += fmt::format(" {}", (ErrorCode)readUint16(bin, i + 1));
                    i += 2;
                    break;
                }
                case OP::CallExpression: {
                    params += fmt::format(" &{}", readUint16(bin, i + 1));
                    i += 2;
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
                    auto address = readUint32(bin, i + 1);
                    params += fmt::format(" \"{}\"", readStorage(bin, address));
                    i += 4;
                    break;
                }
            }

            string text;
            if (params.empty()) {
                text = fmt::format("{}", op);
            } else {
                text = fmt::format("{}{}", op, params);
            }
            if (result.activeSubroutine) {
                result.activeSubroutine->operations.push_back({.text = text, .address = startI});
            } else {
                result.operations.push_back(text);
            }
            if (print) {
                std::cout << "[" << startI << "] (" << text << ") ";
            }
        }
        if (print) fmt::print("\n");
        return result;
    }

    inline void printBin(string_view bin) {
        parseBin(bin, true);
    }
}