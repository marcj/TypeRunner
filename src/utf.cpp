#include "utf.h"

/**
 * Note that an arbitrary `charCodeAt(text, position+1)` does not work since the current code point might be longer than one byte.
 * We probably should introduction `int position, int offset` so that `charCodeAt(text, position, 1)` returns the correct unicode code point.
 */
ts::utf::CharCode ts::utf::charCodeAt(const std::string &text, int position, int *size) {
    //from - https://stackoverflow.com/a/40054802/979328
    int length = 1;
    int first = text[position];
    if ((first & 0xf8) == 0xf0) length = 4;
    else if ((first & 0xf0) == 0xe0) length = 3;
    else if ((first & 0xe0) == 0xc0) length = 2;
    if ((unsigned long)(position + length) > text.length()) length = 1;

    if (size != nullptr) *size = length;

    //from http://www.zedwood.com/article/cpp-utf8-char-to-codepoint
    if (length < 1) return {-1, position};
    unsigned char u0 = first;
    if (u0 >= 0 && u0 <= 127) return {u0, length};
    if (length < 2) return {-1, position};
    unsigned char u1 = text[position + 1];
    if (u0 >= 192 && u0 <= 223) return {(u0 - 192) * 64 + (u1 - 128), length};
    if (first == (const char) 0xed && (text[position + 1] & 0xa0) == 0xa0)
        return {-1, position}; //code points, 0xd800 to 0xdfff
    if (length < 3) return {-1, position};
    unsigned char u2 = text[position + 2];
    if (u0 >= 224 && u0 <= 239) return {(u0 - 224) * 4096 + (u1 - 128) * 64 + (u2 - 128), length};
    if (length < 4) return {-1, position};
    unsigned char u3 = text[position + 3];
    if (u0 >= 240 && u0 <= 247)
        return {(u0 - 240) * 262144 + (u1 - 128) * 4096 + (u2 - 128) * 64 + (u3 - 128), length};
    return {-1, position};
}

std::string ts::utf::fromCharCode(int cp) {
    char c[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    if (cp <= 0x7F) { c[0] = cp; }
    else if (cp <= 0x7FF) {
        c[0] = (cp >> 6) + 192;
        c[1] = (cp & 63) + 128;
    } else if (0xd800 <= cp && cp <= 0xdfff) {} //invalid block of utf8
    else if (cp <= 0xFFFF) {
        c[0] = (cp >> 12) + 224;
        c[1] = ((cp >> 6) & 63) + 128;
        c[2] = (cp & 63) + 128;
    } else if (cp <= 0x10FFFF) {
        c[0] = (cp >> 18) + 240;
        c[1] = ((cp >> 12) & 63) + 128;
        c[2] = ((cp >> 6) & 63) + 128;
        c[3] = (cp & 63) + 128;
    }
    return {c};
}