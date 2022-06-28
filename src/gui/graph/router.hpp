#pragma once

#include "types.hpp"
#include "subgraph.hpp"

#include <vector>
#include <cmath>
#include <optional>
#include <algorithm>


namespace detail {

struct edge_router {
    virtual void run(const hierarchy& h, const feedback_set& rev) = 0;
    virtual ~edge_router() = default;
};

class router : public edge_router {
    const float min_sep = 5;
    const float loop_angle_sep = 5;
    
    const attributes& attr;
    std::vector<node>& nodes;
    std::vector<path>& links;

    vertex_map< std::array< float, 4> > shifts;

    vertex_map< bool > has_loop;

public:
    router(std::vector<node>& nodes, std::vector<path>& paths, const attributes& attr) 
        : attr(attr)
        , nodes(nodes)
        , links(paths) {}

    void run(const hierarchy& h, const feedback_set& rev) override {
        init(h, rev);
        calculate_shifts(h);
        make_paths(h, rev);
    }

private:
    void init(const hierarchy& h, const feedback_set& rev) {
        shifts.init( h.g, { 0 } );
        
        has_loop.init(h.g, false);
        for (auto u : rev.loops) {
            has_loop.set(u, true);
        }
    }

    void calculate_shifts(const hierarchy& h) {
        for (const auto l : h.layers) {
            for (auto u : l) {
                for (auto v : h.g.out_neighbours(u)) {
                    if (!h.g.is_dummy(u) || !h.g.is_dummy(v)) 
                        set_regular_shifts(h, {u, v});
                }
            }
        }
        unify_dummy_shifts(h);
    }

    void make_paths(const hierarchy& h, const feedback_set& rev) {
        for (auto u : h.g.vertices()) {
            for (auto v : h.g.out_neighbours(u)) {
                if (!h.g.is_dummy(u)) make_path(h, rev, u, v);
            }
        }

        for (auto u : rev.loops) {
            make_loop_square(u);
        }
    }

    void set_regular_shifts(const hierarchy& h, edge e) {
        auto dirs = get_dirs(e);
        if (dirs.x == 0)
            return;

        check_loop(e);
        check_loop(reversed(e));
        
        // find the possible candidates for an intersection
        // if the adjacent node is a dummy vertex we want to check for an intersection with the closest regular vertex as well
        auto up = next_non_dummy(h, e.from, dirs.x);
        auto down = next_non_dummy(h, e.to, -dirs.x);
        auto up_d = next_vertex(h, e.from, dirs.x);
        auto down_d = next_vertex(h, e.to, -dirs.x);

        // try all combinations
        set_shift(h, e, up, down, dirs);
        set_shift(h, e, up, down_d, dirs);
        set_shift(h, e, up_d, down, dirs);
        set_shift(h, e, up_d, down_d, dirs);
    }

    void set_shift(const hierarchy& h, edge e, std::optional<vertex_t> up, std::optional<vertex_t> down, vec2 dirs) {
        auto from = get_center(e.from, dirs);
        auto to = get_center(e.to, -dirs);
        
        // set it to something which the edge will never intersect
        vec2 c_up = pos(e.from) - vec2{ dirs.x*10, 0};
        float r_up = 0;
        // set it to the actual value if one exists
        if (up) {
            c_up = pos(*up);
            r_up = h.g.is_dummy(*up) ? shifts[*up][dir_idx(dirs)] : nodes[*up].size;
        }

        // set it to something which the edge will never intersect
        vec2 c_down = pos(e.from) - vec2{ dirs.x*10, 0};
        float r_down = 0;
        // set it to the actual value if one exists
        if (down) {
            c_down = pos(*down);
            r_down = h.g.is_dummy(*down) ? shifts[*down][dir_idx(-dirs)] : nodes[*down].size;
        }

        bool can_inter_up = sgn(c_up.x - from.x) != sgn(c_up.x - to.x);
        bool can_inter_down = sgn(c_down.x - from.x) != sgn(c_down.x - to.x);

        float node_size = attr.default_node_size; //todo: adjust

        // if it is possible for the edge to intersect the vertex
        if ( can_inter_up || can_inter_down ) {
            float s = get_shift(e.from, dirs);
            float t = get_shift(e.to, -dirs);

            bool up_done = false, down_done = false;
            while (!up_done || !down_done) {
                if (can_inter_up && s <= node_size && line_point_dist(from, to, c_up) <= r_up + min_sep) {
                    s += 5;
                    from = pos(e.from) + vec2{ 0, dirs.y*s };
                    up_done = false;
                } else {
                    up_done = true;
                }

                if (can_inter_down && t <= node_size && line_point_dist(to, from, c_down) <= r_down + min_sep) {
                    t += 5;
                    to = pos(e.to) + vec2{ 0, -dirs.y*t };
                    down_done = false;
                } else {
                    down_done = true;
                }
            }

            // clip it
            if (s > node_size) s = node_size;
            if (t > node_size) t = node_size;

            shifts[e.from][dir_idx(dirs)] = std::max(shifts[e.from][dir_idx(dirs)], s);
            shifts[e.to][dir_idx(-dirs)] = std::max(shifts[e.to][dir_idx(-dirs)], t);
        }
    }

    float get_shift(edge e) { return get_shift(e.from, get_dirs(e)); }
    float get_shift(vertex_t u, vec2 dirs) { return get_shift(u, dir_idx(dirs)); }
    float get_shift(vertex_t u, int quadrant) { return shifts[u][quadrant]; }

    void unify_dummy_shifts(const hierarchy& h) {
        for (int layer_idx = 0; layer_idx < h.size(); ++layer_idx) {
            const auto& l = h.layers[layer_idx];
            int j = -1;
            for (int i = 0; i < l.size(); ++i) {
                if ( !h.g.is_dummy(l[i]) ) {
                    set_sequence_shifts(h, layer_idx, j + 1, i - 1);
                    j = i; 
                }
            }
            set_sequence_shifts(h, layer_idx, j + 1, l.size() - 1);
        }
    }

    void set_sequence_shifts(const hierarchy& h, int layer, int start, int end) {
        if (end < start)
            return;
        
        const auto& l = h.layers[layer];

        std::array<float, 4> s = { 0 };
        for (int i = start; i <= end; ++i) {
            vertex_t u = l[i];
            assert( h.g.is_dummy(u) );

            s[0] = std::max(s[0], shifts[u][0]);
            s[1] = std::max(s[1], shifts[u][1]);
            s[2] = std::max(s[2], shifts[u][2]);
            s[3] = std::max(s[3], shifts[u][3]);
        }

        for (int i = start; i <= end; ++i) {
            shifts[l[i]][0] = s[0];
            shifts[l[i]][1] = s[1];
            shifts[l[i]][2] = s[2];
            shifts[l[i]][3] = s[3];
        }

        fix_regular_ends(h, layer, start, end);
    }

    void fix_regular_ends(const hierarchy& h, int layer, int start, int end) {
        const auto& l = h.layers[layer];

        if (h.has_prev(l[start])) {
            auto u = h.prev(l[start]);
            for (auto v : h.g.neighbours(u)) {
                    set_regular_shifts(h, { u, v });
            }
        }

        if (h.has_next(l[end])) {
            auto u = h.next(l[end]);
            for (auto v : h.g.neighbours(u)) {
                set_regular_shifts(h, { u, v });
            }
        }
    }

    void check_loop(edge e) {
        auto dirs = get_dirs(e);
        if (!has_loop.at(e.from) || dir_idx(dirs) == 2 || dir_idx(dirs) == 3)
            return;

        float s = get_shift(e.from, dirs);
        auto from = get_center(e.from, dirs);
        auto to = get_center(e.to, -dirs);
        auto intersection = *line_circle_intersection(from, to, nodes[e.from].pos, nodes[e.from].size);
        float a = angle(pos(e.from), intersection);

        if (a > attr.loop_angle - loop_angle_sep) {
            a = attr.loop_angle - loop_angle_sep;
            auto p = angle_point(a, e.from, dirs);

            float t = (pos(e.from).x - p.x)/(p.x - to.x);
            s = fabs( pos(e.from).y - (p.y + t*(p.y - to.y)) );

            assert(s >= 0 && s <= nodes[e.from].size);

            shifts[e.from][dir_idx(dirs)] = s;
        }
    }

    vec2 get_center(vertex_t u, vec2 dirs) {
        return nodes[u].pos + vec2{ 0, dirs.y*get_shift(u, dirs) };
    }


    float angle(vec2 from, vec2 to) {
        auto dir = to - from;
        return to_degrees( std::atan(fabs(dir.x)/fabs(dir.y)) );
    }

    vec2 angle_point(float angle, vertex_t u, vec2 dirs) {
        return pos(u) + vec2{ dirs.x * nodes[u].size * std::sin(to_radians(angle)),
                              dirs.y * nodes[u].size * std::cos(to_radians(angle)) };
    }


    /**
     * Get the index of the quadrant <dirs> points to.
     * The indexes are as follows:
     * 
     *    2 | 1
     *   ---+--->
     *    3 | 0
     *      v
     */ 
    int dir_idx(vec2 dirs) const {
        if (dirs.x == 1)
            return dirs.y != 1;
        return 2 + (dirs.y == 1);
    }

    vec2 pos(vertex_t u) const { return nodes[u].pos; }
    vec2 get_dirs(vec2 v) const { return { sgn(v.x), sgn(v.y) }; }
    vec2 get_dirs(edge e) const { return get_dirs(pos(e.to) - pos(e.from)); }
    int get_xdir(edge e) const { return sgn(pos(e.to).x - pos(e.from).x); }

    // finds next vertex in the desired direction which is not a dummy vertex
    std::optional<vertex_t> next_non_dummy(const hierarchy& h, vertex_t u, int d) {
        int i = h.pos[u] + d;
        while (h.valid_pos(h.ranking[u], i) && h.g.is_dummy( h.layer(u)[i] )) {
            i += d;
        }

        if (h.valid_pos(h.ranking[u], i)) {
            return h.layer(u)[i];
        }
        return std::nullopt;
    }

    std::optional<vertex_t> next_vertex(const hierarchy& h, vertex_t u, int d) {
        if (d == -1 && h.has_prev(u)) {
            return h.prev(u);
        }
        if (d == 1 && h.has_next(u)) {
            return h.next(u);
        }
        return std::nullopt;
    }

    vertex_t next(const subgraph& g, vertex_t u) { return *g.out_neighbours(u).begin(); }
    vertex_t prev(const subgraph& g, vertex_t u) { return *g.in_neighbours(u).begin(); }


    void make_path(const hierarchy& h, const feedback_set& rev, vertex_t u, vertex_t v) {
        auto& g = h.g;
        path l{ u, v, {} };
        auto orig = edge{ u, v };

        l.points.push_back( calculate_port_shifted(u, nodes[v].pos - nodes[u].pos) );

        while (g.is_dummy(v)) {
            auto s = shifts[v][dir_idx(get_dirs(edge{v, u}))];
            if (s > 0)
                l.points.push_back( nodes[v].pos + vec2{ 0, -s } );
            
            l.points.push_back( nodes[v].pos );

            auto n = next(g, v);
            s = shifts[v][dir_idx(get_dirs(edge{v, n}))];
            if (s > 0)
                l.points.push_back( nodes[v].pos + vec2{ 0, s } );

            u = v;
            v = n;
        }

        l.points.push_back( calculate_port_shifted(v, nodes[u].pos - nodes[v].pos) );
        l.to = v;

        if (rev.reversed.contains(orig)) {
            reverse(l);
        } else if (rev.removed.contains(orig)) {
            l.bidirectional = true;
        }

        links.push_back(std::move(l));
    }

    void reverse(path& l) {
        std::swap(l.from, l.to);
        for (int i = 0; i < l.points.size()/2; ++i) {
            std::swap(l.points[i], l.points[l.points.size() - i - 1]);
        }
    }
    

    void make_loop_square(vertex_t u) {
        path l{ u, u, {} };
        l.points.resize(4);

        l.points[0] = angle_point(attr.loop_angle, u, { 1, -1 });
        l.points[3] = angle_point(attr.loop_angle, u, { 1, 1 });

        l.points[1] = vec2{ pos(u).x + nodes[u].size + attr.loop_size/2, l.points[0].y };
        l.points[2] = vec2{ pos(u).x + nodes[u].size + attr.loop_size/2, l.points[3].y };

        links.push_back(std::move(l));
    }

    // calculate the port using shifts
    vec2 calculate_port_shifted(vertex_t u, vec2 dir) {
        vec2 dirs { sgn(dir.x), sgn(dir.y) };
        auto s = get_shift(u, dirs);
        auto center = get_center(u, dirs);
        if (s == nodes[u].size) {
            return center;
        }
        return *line_circle_intersection(center, center + dir, pos(u), nodes[u].size);
    }

    // calculate the port as if the edge was at the center
    vec2 calculate_port_centered(vertex_t u, vec2 dir) {
        return nodes[u].pos + nodes[u].size * normalized(dir);
    }

    // calculate the port as the lowest/highest points on the node
    vec2 calculate_port_single(vertex_t u, vec2 dir) {
        return nodes[u].pos + vec2{0, sgn(dir.y) * nodes[u].size};
    }

};

} // namespace detail
