#include <iostream>
#include "src/utf.h"

int main() {
    std::string text = "今天12";

    for (int i = 0; i < text.length();) {
        auto p = ts::charCodeAt(text, i);
        std::cout << p.code << std::endl;
        i += p.length;
    }

    return 0;
}
