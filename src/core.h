#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>
#include <sstream>

namespace ts {
    using std::string;
    using std::vector;
    using std::function;
    using std::optional;
    using std::shared_ptr;
    using std::replace;
    using std::unordered_map;
    using std::map;
    using std::cout;

    unsigned constexpr const_hash(char const *input) {
        return *input ?
               static_cast<unsigned int>(*input) + 33 * const_hash(input + 1) :
               5381;
    }

    unsigned constexpr const_hash(const string &input) {
        return const_hash(input.c_str());
    }

    constexpr unsigned operator "" _hash(const char *s, size_t) {
        return const_hash(s);
    }

    /**
     * shared_ptr has optional semantic already built-in, so we use it instead of std::optional,
     * but instead of using shared_ptr directly, we use opt<T> to make it clear that it can be empty.
     */
    template<typename T>
    using sharedOpt = shared_ptr<T>;

    /**
     * Because shared_ptr<T> is ugly.
     */
    template<typename T>
    using shared = shared_ptr<T>;

    string replaceLeading(const string &text, const string &from, const string &to) {
        if (0 == text.find(from)) {
            string str = text;
            str.replace(0, from.length(), to);
            return str;
        }
        return text;
    }

    /**
     * Returns the last element of an array if non-empty, `undefined` otherwise.
     */
    template<typename T>
    optional<T> lastOrUndefined(const vector<T> &array) {
        if (array.empty()) return std::nullopt;
        return array.back();
    }

    vector<string> charToStringVector(vector<const char *> chars) {
        vector<string> s;
        for (auto c: chars) s.push_back(c);
        return s;
    }

    bool endsWith(const string &str, const string &suffix) {
        auto expectedPos = str.size() - suffix.size();
        return expectedPos >= 0 && str.find(suffix, expectedPos) == expectedPos;
    }

    template<typename T>
    vector<T> append(vector<T> &v, T item) {
        v.push_back(item);
        return v;
    }

    template<typename T>
    vector<T> slice(const vector<T> &v, int start) {
        return vector<T>(v.begin() + start, v.end());
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

//
//    template<class T>
//    using Optional = optional<reference_wrapper<const T>>;
////    using Optional = variant<BaseUnion, optional<reference_wrapper<const T>>>;
//    //struct Optional: public optional<reference_wrapper<const T>> {};
//
//    template<class T>
//    bool empty(Optional<T> v) {
//    //    if (holds_alternative<T>(v)) return false;
//    //    auto opt = get<optional<reference_wrapper<const T>>>(v);
//    //    return opt.has_value();
//        return v.has_value();
//    }
//
//    template<class T>
//    T &resolve(Optional<T> v) {
//    //    if (holds_alternative<T>(v)) {
//    //        return get<T>(v);
//    //    }
//    //    auto opt = get<optional<reference_wrapper<const T>>>(v);
//    //
//    //    if (!opt.has_value()) throw runtime_error("Optional is empty");
//    //
//    //    return *(Node*)(&(opt.value().get()));
//        return *(T*)(&(v.value().get()));
//    }


    inline string trimStringStart(string s) {
        while (s.compare(0, 1, " ") == 0) s.erase(s.begin());
        return s;
    }

    inline string trimStringEnd(string s) {
        while (s.size() > 0 && s.compare(s.size() - 1, 1, " ") == 0) s.erase(s.end() - 1);
        return s;
    }

    template<typename T>
    inline bool some(optional<vector<T>> array, optional<std::function<bool(typename decltype(array)::value_type::value_type)>> predicate) {
        //inline bool some(vector<any> array, std::function<void(T)> predicate) {
        //inline bool some(optional<vector<T>> array) {
        if (array) {
            if (predicate) {
                for (auto &v: (*array)) {
                    if ((*predicate)(v)) {
                        return true;
                    }
                }
            } else {
                return (*array).size() > 0;
            }
        }
        return false;
    };

    //inline bool some(optional<T> array, std::function<void(typename decltype(data)::value_type::value_type)> predicate) {
    template<typename T>
    inline bool some(vector<T> array, std::function<bool(typename decltype(array)::value_type)> predicate) {
        return some(optional(array), optional(predicate));
    }

    template<typename T>
    class LogicalOrReturnLast {
    protected:
        T value;
    public:
        LogicalOrReturnLast(T value): value(value) {}
        operator T() { return value; }
        LogicalOrReturnLast operator||(LogicalOrReturnLast other) {
            if (value) return *this;

            return other;
        }
        LogicalOrReturnLast operator||(T other) {
            if (value) return *this;

            return LogicalOrReturnLast(other);
        }
    };

    //template<typename T>
    //inline bool some(ts::NodeArray<T> array, std::function<bool(typename decltype(array)::value_type::value_type)> predicate) {
    //    return some(optional(array.list), optional(predicate));
    //}

    /**
     * Filters vector by filter in-place.
     */
    template<typename T, typename Func>
    static void remove(vector<T> &vector, const Func &filter) {
        auto new_end = remove_if(vector.begin(), vector.end(), filter);
        vector.erase(new_end, vector.end());
    };

    template<typename T>
    static void remove(vector<T> &vector, T item) {
        vector.erase(remove(vector.begin(), vector.end(), item), vector.end());
    };

    template<typename T>
    static bool has(vector<T> &vector, T item) {
        return find(vector.begin(), vector.end(), item) != vector.end();
    };

    template<typename T>
    static bool has(const vector <T> &vector, T item) {
        return find(vector.begin(), vector.end(), item) != vector.end();
    };

    template<typename T>
    unordered_map<string, T> combine(
            const unordered_map<string, T> &map1,
            const unordered_map<string, T> &map2
    ) {
        unordered_map<string, T> res(map1);
        res.insert(map2.begin(), map2.end());
        return res;
    }

    template<typename K, typename T>
    static optional<T> get(unordered_map<K, T> &m, K key) {
        auto v = m.find(key);
        if (v == m.end()) return std::nullopt;
        return v->second;
    }

    template<typename K, typename T>
    static optional<T> set(unordered_map<K, T> &m, K key, T v) {
        m[key] = v;
    }

    template<typename T, typename U>
    static bool has(unordered_map<T, U> &map, T key) {
        return map.find(key) != map.end();
    };

    template<typename T, typename U>
    static bool has(map<T, U> &map, T key) {
        return map.find(key) != map.end();
    };

    static std::string format(const std::string &fmt, ...) {
        int size = ((int) fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
        std::string str;
        va_list ap;
        while (1) {     // Maximum two passes on a POSIX system...
            str.resize(size);
            va_start(ap, fmt);
            int n = vsnprintf((char *) str.data(), size, fmt.c_str(), ap);
            va_end(ap);
            if (n > - 1 && n < size) {  // Everything worked
                str.resize(n);
                return str;
            }
            if (n > - 1)  // Needed size returned
                size = n + 1;   // For null char
            else
                size *= 2;      // Guess at a larger size (OS specific)
        }
        return str;
    }

    template<typename...Args>
    inline void debug(const std::string &fmt, Args &&...args) {
        cout << format(fmt, args...) << "\n";
    }
}
