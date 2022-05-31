#include "core.h"

namespace ts::Debug {
    void asserts(bool v, std::string text) {
        if (!v) throw std::runtime_error("assert: " + text);
    }

    void fail(std::string text) {
        throw std::runtime_error("Fail: " + text);
    }
}

namespace ts {
    string substr(const string &str, int start, int length) {
        if (start < 0) start += str.length();
        if (length < 0) length = str.length() + length - start;
        if (length < 0) return "";
        return str.substr(start, length);
    }
    string substr(const string &str, int start) {
        return substr(str, start, str.length());
    }
    string replaceLeading(const string &text, const string &from, const string &to) {
        if (0 == text.find(from)) {
            string str = text;
            str.replace(0, from.length(), to);
            return str;
        }
        return text;
    }

    bool isTrue(optional<bool> b) {
        return b && *b;
    }
    bool isFalse(optional<bool> b) {
        return !b || !*b;
    }
    vector<string> charToStringVector(vector<const char *> chars) {
        vector<string> s;
        for (auto c: chars) s.push_back(c);
        return s;
    }
    bool startsWith(const string &str, const string &suffix) {
        return str.find(suffix) == 0;
    }
    bool endsWith(const string &str, const string &suffix) {
        auto expectedPos = str.size() - suffix.size();
        return expectedPos >= 0 && str.find(suffix, expectedPos) == expectedPos;
    }
    string join(const vector<string> &vec, const char *delim) {
        std::stringstream res;
        copy(vec.begin(), vec.end(), std::ostream_iterator<string>(res, delim));
        return res.str();
    }
    vector<string> split(const string &s, const string &delimiter) {
        vector<string> result;
        size_t last = 0;
        size_t next = 0;
        while ((next = s.find(delimiter, last)) != string::npos) {
            result.push_back(s.substr(last, next - last));
            last = next + 1;
        }
        result.push_back(s.substr(last));
        return result;
    }
    string replaceAll(const string &text, const string &from, const string &to) {
        if (from.empty()) return text;
        string str = text;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
        return str;
    }
    string trimStringStart(string s) {
        while (s.compare(0, 1, " ") == 0) s.erase(s.begin());
        return s;
    }
    string trimStringEnd(string s) {
        while (s.size() > 0 && s.compare(s.size() - 1, 1, " ") == 0) s.erase(s.end() - 1);
        return s;
    }
    std::string format(const std::string &fmt, ...) {
        int size = ((int) fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
        std::string str;
        va_list ap;
        while (1) {     // Maximum two passes on a POSIX system...
            str.resize(size);
            va_start(ap, fmt);
            int n = vsnprintf((char *) str.data(), size, fmt.c_str(), ap);
            va_end(ap);
            if (n > -1 && n < size) {  // Everything worked
                str.resize(n);
                return str;
            }
            if (n > -1)  // Needed size returned
                size = n + 1;   // For null char
            else
                size *= 2;      // Guess at a larger size (OS specific)
        }
        return str;
    }

}