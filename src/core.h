#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>

namespace ts {
    using namespace std;

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
        LogicalOrReturnLast(T value) : value(value) {}
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
    static bool has(const vector<T> &vector, T item) {
        return find(vector.begin(), vector.end(), item) != vector.end();
    };

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

    template<typename...Args>
    inline void debug(const std::string &fmt, Args&&...args) {
        cout << format(fmt, args...) << "\n";
    }
}
