#pragma once

#include <string>
#include <fstream>
#include <filesystem>

using std::string;
using std::string_view;

inline auto fileRead(const string &file) {
    std::ifstream t;
    t.open(file);
    t.seekg(0, std::ios::end);
    size_t size = t.tellg();
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(&buffer[0], size);
    return std::move(buffer);
}

inline void fileWrite(const string &file, const string_view &content) {
    std::ofstream t;
    t.open(file);
    t << content;
}

inline bool fileExists(const string &file)
{
    std::ifstream infile(file);
    return infile.good();
}