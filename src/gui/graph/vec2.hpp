#pragma once

#include <cmath>
#include <ostream>
#include <optional>

struct vec2 {
    float x, y;   
};

inline std::ostream& operator<<(std::ostream& out, vec2 v) {
    out << "[" << v.x << ", " << v.y << "]";
    return out;
}

template<typename T>
inline vec2 operator*(T a, vec2 vec) { return { a*vec.x, a*vec.y}; }
template<typename T>
inline vec2 operator*(vec2 vec, T a) { return { a*vec.x, a*vec.y}; }

inline bool operator==(vec2 lhs, vec2 rhs) { return rhs.x == lhs.x && rhs.y == lhs.y; }
inline bool operator!=(vec2 lhs, vec2 rhs) { return !(lhs == rhs); }

inline vec2 operator+(vec2 lhs, vec2 rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y }; }
inline vec2& operator+=(vec2& lhs, vec2 rhs) {
    lhs = lhs + rhs;
    return lhs;
}

inline vec2 operator-(vec2 v) { return -1*v; }
inline vec2 operator-(vec2 lhs, vec2 rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y }; }
inline vec2& operator-=(vec2& lhs, vec2 rhs) {
    lhs = lhs - rhs;
    return lhs;
}

inline float dot(vec2 u, vec2 v) { return u.x*v.x + u.y*v.y; }
inline float cross(vec2 u, vec2 v) {
    return u.x*v.y - u.y*v.x;
}

inline float magnitude(vec2 v) { return std::sqrt( dot(v, v) ); }
inline float distance(vec2 u, vec2 v) { return magnitude( v - u ); }

inline vec2 normalized(vec2 v) { return { v.x/magnitude(v), v.y/magnitude(v) }; }

inline float to_radians(float deg) { return (M_PI*deg)/180; }
inline float to_degrees(float rad) { return (180*rad)/M_PI; }

inline vec2 rotate(vec2 vec, float deg) {
    float rad = to_radians(deg);
    float sin = std::sin(rad);
    float cos = std::cos(rad);
    return { vec.x*cos + vec.y*sin, -vec.x*sin + vec.y*cos };
}

float line_point_dist(vec2 from, vec2 to, vec2 p) {  
    auto v = to - from;
    auto w = p - from;

    float c1 = dot(w,v);
    float c2 = dot(v,v);
    float t = c1 / c2;

    auto x = from + t*v;
    return distance(p, x);
}

// get the first interesection
std::optional<vec2> line_circle_intersection(vec2 from, vec2 to, vec2 center, float r) {
    auto d = to - from;
    auto f = from - center;

    auto a = dot(d, d);
    auto b = 2*dot(f, d);
    auto c = dot(f, f) - r*r;

    float discriminant = b*b - 4*a*c;
    if (discriminant < 0) {
        return std::nullopt;
    }

    discriminant = sqrt(discriminant);
    float t1 = (-b - discriminant)/(2*a);
    float t2 = (-b + discriminant)/(2*a);

    if (t1 >= 0 && t1 <= 1) {
        return { from + t1*d };
    }

    if (t2 >= 0 && t2 <= 1) {
        return { from + t2*d };
    }

    return std::nullopt;
}
