#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>

namespace ts {
    using namespace std;

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
}
