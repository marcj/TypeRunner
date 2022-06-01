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
    string substr(const string &str, int start, optional<int> len) {
        if (start <= 0 && len && len < 0) return "";
        if (!len || *len < 0 || len > str.size()) *len = str.size();
        if (start < 0) start += str.length();
        return str.substr(start, *len);
    }

    //compatible with JavaScript's String.substring
    string substring(const string &str, int start, optional<int> end) {
        if (start < 0) start = 0;
        int len = str.size();
        if (end) {
            if (*end < start) {
                len = start - *end;
                start = *end;
            } else {
                len = *end - start;
            }
        }
        if (len < 0) len = 0;
        if (start > str.size()) start = str.size();
        return str.substr(start, len);
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
        for (auto &&c: chars) s.push_back(c);
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
}