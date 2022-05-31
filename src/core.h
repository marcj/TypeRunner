#pragma once

#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>
#include <sstream>
#include <iostream>

#define CALLBACK(name) [this](auto ...a) { return name(a...); }

namespace ts::Debug {
    void asserts(bool v, std::string text = "");
    void fail(std::string text = "");
}

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

    inline constexpr unsigned const_hash(const char *input) {
        return *input ? static_cast<unsigned int>(*input) + 33 * const_hash(input + 1) : 5381;
    }

    inline constexpr unsigned const_hash(const string &input) {
        return const_hash(input.c_str());
    }

    inline constexpr unsigned operator ""_hash(const char *s, size_t) {
        return const_hash(s);
    }

    //compatible with JavaScript's String.substr
    string substr(const string &str, int start, optional<int> len = {});

    //compatible with JavaScript's String.substring
    string substring(const string &str, int start, optional<int> end = {});

    /**
     * shared_ptr has optional semantic already built-in, so we use it instead of std::optional<shared_ptr<>>,
     * but instead of using shared_ptr directly, we use sharedOpt<T> to make it clear that it can be empty.
     */
    template<typename T>
    using sharedOpt = shared_ptr<T>;

    /**
     * Because shared_ptr<T> is ugly.
     */
    template<typename T>
    using shared = shared_ptr<T>;

    string replaceLeading(const string &text, const string &from, const string &to);

    /**
     * Returns the last element of an array if non-empty, `undefined` otherwise.
     */
    template<typename T>
    inline optional<T> lastOrUndefined(vector<T> &array) {
        if (array.empty()) return std::nullopt;
        return array.back();
    }

    template<typename T>
    inline T defaultTo(optional<T> v, T def) {
        if (v) return *v;
        return def;
    }

    bool isTrue(optional<bool> b = {});

    bool isFalse(optional<bool> b = {});

    vector<string> charToStringVector(vector<const char *> chars);

    bool startsWith(const string &str, const string &suffix);

    bool endsWith(const string &str, const string &suffix);

    template<typename T>
    inline vector<T> append(vector<T> &v, T item) {
        v.push_back(item);
        return v;
    }

    template<typename T>
    inline vector<T> slice(const vector<T> &v, int start = 0, int end = 0) {
        if (!end) end = v.size();
        return std::vector<T>(v.begin() + start, v.begin() + end);
    }

    template<typename T>
    inline optional<vector<T>> slice(const optional<vector<T>> &v, int start = 0, int end = 0) {
        if (!v) return std::nullopt;
        return slice(*v, start, end);
    }

    string join(const vector<string> &vec, const char *delim);

    vector<string> split(const string &s, const string &delimiter);

    string replaceAll(const string &text, const string &from, const string &to);

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


    string trimStringStart(string s);

    string trimStringEnd(string s);

    template<typename T>
    inline bool some(optional<vector<T>> array, optional<std::function<bool(decltype(decltype(array)::value_type::value_type))>> predicate) {
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
    }

    template<typename T>
    inline bool some(optional<vector<T>> array) {
        return array && !array->empty();
    }

    template<typename T>
    inline bool some(vector<T> array, std::function<bool(decltype(decltype(array)::value_type))> predicate) {
        return some(optional(array), optional(predicate));
    }

    template<typename T, typename Func>
    inline void remove(vector<T> &vector, const Func &filter) {
        auto new_end = remove_if(vector.begin(), vector.end(), filter);
        vector.erase(new_end, vector.end());
    }

    template<typename T>
    inline void remove(vector<T> &vector, T item) {
        vector.erase(remove(vector.begin(), vector.end(), item), vector.end());
    }

    template<typename T>
    inline bool has(std::set<T> &s, T item) {
        return s.find(item) != s.end();
    }

    template<typename T>
    inline bool has(vector<T> &vector, T item) {
        return find(vector.begin(), vector.end(), item) != vector.end();
    };

    template<typename T>
    inline bool has(const vector <T> &vector, T item) {
        return find(vector.begin(), vector.end(), item) != vector.end();
    };

    template<typename T>
    inline unordered_map<string, T> combine(
            const unordered_map<string, T> &map1,
            const unordered_map<string, T> &map2
    ) {
        unordered_map<string, T> res(map1);
        res.insert(map2.begin(), map2.end());
        return res;
    }

    template<typename K, typename V>
    inline unordered_map<V, K> reverse(
            const unordered_map<K, V> &map1
    ) {
        unordered_map<V, K> res;
        for (auto &i: map1) {
            res[i.second] = i.first;
        }
        return res;
    }

    template<typename K, typename T>
    inline optional<T> get(unordered_map<K, T> &m, K key) {
        auto v = m.find(key);
        if (v == m.end()) return std::nullopt;
        return v->second;
    }

    template<typename K, typename T>
    inline optional<T> set(unordered_map<K, T> &m, K key, T v) {
//        m.insert_or_assign(key, v);
        std::cout << "set map: " << key << ", " << v << "\n";
//        m.insert({key, v});
//        m[key] = v;
    }

    template<typename T, typename U>
    inline bool has(unordered_map<T, U> &map, T key) {
        return map.find(key) != map.end();
    };

    template<typename T, typename U>
    inline bool has(map<T, U> &map, T key) {
        return map.find(key) != map.end();
    };

    std::string format(const std::string &fmt, ...);

    template<typename...Args>
    inline void debug(const std::string &fmt, Args &&...args) {
        std::cout << format(fmt, args...) << "\n";
    }
}
