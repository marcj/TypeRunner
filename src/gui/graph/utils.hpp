#pragma once

#include <string>
#include <vector>

#include "vec2.hpp"
#include "types.hpp"


template <typename T> 
T sgn(T val) { return ( T(0) < val ) - ( val < T(0) ); }


template<typename T>
struct range {
    T r_start;
    T r_end; 
    int step;

    range(T start, T end, int step = 1) : r_start(start), r_end(end), step(step) {}

    struct iterator {
        T val;
        int step;
        iterator(T val, int step) : val(val), step(step) {}

        T operator*() { return val; }
        friend bool operator==(const iterator& lhs, const iterator& rhs) { return lhs.val == rhs.val; }
        friend bool operator!=(const iterator& lhs, const iterator& rhs) { return !(lhs == rhs); }
        iterator& operator++() { val += step; return *this; }
    };
    
    iterator begin() const { return iterator(r_start, step); }
    iterator end() const { return iterator(r_end, step); }
};


template<typename T>
struct chain_range {
    const T& first;
    const T& second;

    chain_range(const T& first, const T& second) : first(first), second(second) {}
        
    struct iterator {
        using It = typename T::const_iterator;
        It curr;
        It st_end;
        It nd_beg;

        iterator(It curr, It st_end, It nd_beg)
            : curr(curr)
            , st_end(st_end)
            , nd_beg(nd_beg) 
        { 
            if (curr == st_end) {
                this->curr = nd_beg;
            }    
        }

        typename T::value_type operator*() { return *curr; }
        friend bool operator==(const iterator& lhs, const iterator& rhs) { return lhs.curr == rhs.curr; }
        friend bool operator!=(const iterator& lhs, const iterator& rhs) { return !(lhs == rhs); }
        iterator& operator++() {
            if (++curr == st_end) {
                curr = nd_beg;
            }
            return *this;
        }
    };

    iterator begin() {
        return iterator( std::begin(first), std::end(first), std::begin(second) );
    }
    iterator begin() const {
        return iterator( std::begin(first), std::end(first), std::begin(second) );
    }
    iterator end() {
        return iterator( std::end(second),  std::end(first), std::begin(second) );
    }
    iterator end() const {
        return iterator( std::end(second),  std::end(first), std::begin(second) );
    }
};


template<typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& vec) {
    const char* sep = "";
    for (const auto& x : vec) {
        out << sep << x;
        sep = ", ";
    }
    return out;
}
