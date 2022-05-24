#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "utf.h"
#include "core.h"
#include "types.h"

using namespace ts;
using namespace std;

inline string parsePseudoBigInt(string &stringValue) {
    int log2Base;
    switch (charCodeAt(stringValue, 1).code) { // "x" in "0x123"
        case CharacterCodes::b:
        case CharacterCodes::B: // 0b or 0B
            log2Base = 1;
            break;
        case CharacterCodes::o:
        case CharacterCodes::O: // 0o or 0O
            log2Base = 3;
            break;
        case CharacterCodes::x:
        case CharacterCodes::X: // 0x or 0X
            log2Base = 4;
            break;
        default: // already in decimal; omit trailing "n"
            auto nIndex = stringValue.size() - 1;
            // Skip leading 0s
            auto nonZeroStart = 0;
            while (charCodeAt(stringValue, nonZeroStart).code == CharacterCodes::_0) {
                nonZeroStart++;
            }
            return stringValue.substr(nonZeroStart, nIndex);
    }

    // Omit leading "0b", "0o", or "0x", and trailing "n"
    auto startIndex = 2;
    auto endIndex = stringValue.size() - 1;
    auto bitsNeeded = (endIndex - startIndex) * log2Base;
    // Stores the value specified by the string as a LE array of 16-bit integers
    // using Uint16 instead of Uint32 so combining steps can use bitwise operators
//    auto segments = new Uint16Array((bitsNeeded >>> 4) + (bitsNeeded & 15 ? 1 : 0));
    vector<int> segments;
    segments.reserve((bitsNeeded >> 4) + (bitsNeeded & 15 ? 1 : 0));

    // Add the digits, one at a time
    for (int i = endIndex - 1, bitOffset = 0; i >= startIndex; i--, bitOffset += log2Base) {
        auto segment = bitOffset >> 4;
        auto digitChar = charCodeAt(stringValue, i).code;
        // Find character range: 0-9 < A-F < a-f
        auto digit = digitChar <= CharacterCodes::_9
                     ? digitChar - CharacterCodes::_0
                     : 10 + digitChar -
                       (digitChar <= CharacterCodes::F ? CharacterCodes::A : CharacterCodes::a);
        auto shiftedDigit = digit << (bitOffset & 15);
        segments[segment] |= shiftedDigit;
        auto residual = shiftedDigit >> 16;
        if (residual) segments[segment + 1] |= residual; // overflows segment
    }
    // Repeatedly divide segments by 10 and add remainder to base10Value
    string base10Value;
    auto firstNonzeroSegment = segments.size() - 1;
    auto segmentsRemaining = true;
    while (segmentsRemaining) {
        auto mod10 = 0;
        segmentsRemaining = false;
        for (unsigned long segment = firstNonzeroSegment; segment >= 0; segment--) {
            auto newSegment = mod10 << 16 | segments[segment];
            auto segmentValue = (newSegment / 10) | 0;
            segments[segment] = segmentValue;
            mod10 = newSegment - segmentValue * 10;
            if (segmentValue && !segmentsRemaining) {
                firstNonzeroSegment = segment;
                segmentsRemaining = true;
            }
        }
        base10Value = to_string(mod10) + base10Value;
    }
    return base10Value;
}

inline bool isKeyword(types::SyntaxKind token) {
    return SyntaxKind::FirstKeyword <= token && token <= SyntaxKind::LastKeyword;
}

inline bool isContextualKeyword(types::SyntaxKind token) {
    return SyntaxKind::FirstContextualKeyword <= token && token <= SyntaxKind::LastContextualKeyword;
}

//unordered_map<string, string> localizedDiagnosticMessages{};

inline string getLocaleSpecificMessage(DiagnosticMessage message) {
//    if (has(localizedDiagnosticMessages, message.key)) {
//        return localizedDiagnosticMessages[message.key];
//    }
    return message.message;
}

inline DiagnosticWithDetachedLocation createDetachedDiagnostic(string fileName, int start, int length, DiagnosticMessage message) {
//    assertDiagnosticLocation(/*file*/ undefined, start, length);
    auto text = getLocaleSpecificMessage(message);

//    if (arguments.length > 4) {
//        text = formatStringFromArgs(text, arguments, 4);
//    }

    return DiagnosticWithDetachedLocation{
            {
                    {
                            .messageText = text,
                            .category = message.category,
                            .code = message.code,
                    },
                    .reportsUnnecessary = message.reportsUnnecessary,
            },
            .fileName = fileName,
            .start = start,
            .length = length,
    };
}
