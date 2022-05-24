#pragma once

#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

namespace ts {
//    std::wstring utf16From(std::string text);

    struct CharCode {
        int code; //unicode code point
        int length;
    };

    // Updates size if non-nullptr is given
    CharCode charCodeAt(const std::string &text, int position, int *size = nullptr);

    std::string fromCharCode(int cp);
}
