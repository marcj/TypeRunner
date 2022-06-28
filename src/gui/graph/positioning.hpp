#pragma once

#include <algorithm>
#include <tuple>
#include <array>
#include <optional>

#include "utils.hpp"
#include "subgraph.hpp"
#include "vec2.hpp"
#include "layering.hpp"

#ifdef DEBUG_COORDINATE
int produce_layout = 0;
#endif

namespace detail {

/**
 * Bounding box of a vertex.
 * The center is relative to upper left corner.
 */
struct bounding_box {
    vec2 size = { 0, 0 };
    vec2 center = { 0, 0 }; 
};

/**
 * Interface for a positioning algorithm that determines the final positions of nodes.
 * The y coordinates of nodes on the same layer have to be the same.
 */
struct positioning {
    virtual vec2 run(detail::hierarchy& h, vec2 origin) = 0;
    virtual ~positioning() = default;
};


/**
 * Naive positioning for testing purpouses.
 */
struct test_positioning : public positioning {
    std::vector<node>& nodes;
    attributes attr;

    test_positioning(attributes attr, std::vector<node>& nodes)
        : nodes(nodes)
        , attr(attr) {}  

    vec2 run(detail::hierarchy& h, vec2 origin) override {
        float y = origin.y + attr.layer_dist;
        float width = 0;
        for (auto layer : h.layers) {
            float x = origin.x;
            for (auto u : layer) {
                x += attr.node_dist + nodes[u].size;
                nodes[u].pos = { x, y };
//                nodes[u].size = attr.node_size;
                x += nodes[u].size;
            }
            if ((x - origin.x) > width) {
                width = x;
            }
            y += attr.layer_dist;
        }
        return { width, y - origin.y };
    }
};


class fast_and_simple_positioning : public positioning {
    std::vector<node>& nodes;
    attributes attr;
    const detail::vertex_map<bounding_box>& boxes;

    enum orient { upper_left, lower_left, upper_right, lower_right };

    std::array< detail::vertex_map<vertex_t>, 4 > medians;
    std::array< detail::vertex_map<vertex_t>, 4 > root;
    std::array< detail::vertex_map<vertex_t>, 4 > align;
    std::array< detail::vertex_map<vertex_t>, 4 > sink; // the class of a block
    std::array< detail::vertex_map<float>, 4 > shift;
    std::array< detail::vertex_map< std::optional<float> >, 4 > x;

    std::array< float, 4 > max;
    std::array< float, 4 > min;

    edge_set conflicting;

public:
    fast_and_simple_positioning(attributes attr, 
                                std::vector<node>& nodes,
                                const detail::vertex_map<bounding_box>& boxes)
        : nodes(nodes)
        , attr(attr)
        , boxes(boxes)
    { }

    void init(const detail::hierarchy& h) {
        for (int i = 0; i < 4; ++i) {
            medians[i].resize(h.g);
            root[i].resize(h.g);
            align[i].resize(h.g);
            sink[i].resize(h.g);
            shift[i].resize(h.g);
            x[i].resize(h.g);

            min[i] = std::numeric_limits<float>::max();
            max[i] = std::numeric_limits<float>::lowest();
        }

        for (auto u : h.g.vertices()) {
            for (int j = 0; j < 4; ++j) {
                root[j][u] = u;
                align[j][u] = u;
                sink[j][u] = u;
                shift[j][u] = 0;
            }
        }
    }

    vec2 run(detail::hierarchy& h, vec2 origin) override {
        init(h);
        init_medians(h);

        mark_conflicts(h);

        for (int i = 0; i < 4; ++i) {
            vertical_align(h, static_cast<orient>(i));
            horizontal_compaction(h, static_cast<orient>(i));
        }

#ifdef DEBUG_COORDINATE
        if(produce_layout < 4) {
            std::cout << "TYPE: " << produce_layout << "\n";
            for (auto u : h.g.vertices()) {
                if (root[produce_layout][u] == u) {
                    vertex_t v = u;
                    const char* sep = "";
                    do {
                        std::cout << sep << v;
                        sep = " -> ";
                        v = align[produce_layout][v];
                    } while (v != u);
                    std::cout << "\n";
                }
            }
            std::cout << "\n";
        }
#endif

        return assign_final_coordinates(h, origin);
    }

    vec2 assign_final_coordinates(const hierarchy& h, vec2 origin) {
        align_layouts(h);

        float y = origin.y;
        std::vector<float> vals(4);
        for (const auto& layer : h.layers) {
            y += biggestSize(nodes, layer);

            for (auto u : layer) {
#ifdef DEBUG_COORDINATE
                if(produce_layout < 4) {
                    nodes[u].pos = vec2{ *x[produce_layout][u] + shift[produce_layout], y };
                } else {
#endif
                vals = { *x[0][u], *x[1][u], *x[2][u], *x[3][u] };
                std::sort(vals.begin(), vals.end());
                nodes[u].pos = { (vals[1] + vals[2])/2, y };
#ifdef DEBUG_COORDINATE
                }
#endif
            }
            y += biggestSize(nodes, layer) + attr.layer_dist;
        }
        float height = y - attr.layer_dist;
        float width = normalize(h, origin.x);

        return { width, height };
    }

    void align_layouts(const hierarchy& h) {
        orient min_width_layout = static_cast<orient>(0);
        for (int i = 1; i < 4; ++i) {
            if ( max[min_width_layout] - min[min_width_layout] > max[i] - min[i] ) {
                min_width_layout = static_cast<orient>(i);
            }
        }

        for (int i = 0; i < 4; ++i) {
            float d = left(static_cast<orient>(i)) ?
                           min[min_width_layout] - min[i] :
                           max[min_width_layout] - max[i];

            for (auto u : h.g.vertices()) {
                *x[i][u] += d;
            } 
        }
    }
    

    float normalize(const detail::hierarchy& h, float start) {
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::lowest();

        for(auto u : h.g.vertices()) {
            if (nodes[u].pos.x + boxes[u].size.x - boxes[u].center.x > max) {
                max = nodes[u].pos.x + boxes[u].size.x - boxes[u].center.x;
            }
            if (nodes[u].pos.x - boxes[u].center.x < min) {
                min = nodes[u].pos.x - boxes[u].center.x;
            }
        }

        for(auto u : h.g.vertices()) {
            nodes[u].pos.x = start + nodes[u].pos.x - min;
        }
        return max - min;
    }


    void init_medians(const detail::hierarchy& h) {
        std::vector<vertex_t> neighbours;
        for (auto u : h.g.vertices()) {
            {
                neighbours.insert(neighbours.begin(), h.g.out_neighbours(u).begin(), h.g.out_neighbours(u).end());
                auto [ left, right ] = median(h, u, neighbours);
                medians[orient::lower_left][u] = left;
                medians[orient::lower_right][u] = right;
                neighbours.clear();
            }

            neighbours.insert(neighbours.begin(), h.g.in_neighbours(u).begin(), h.g.in_neighbours(u).end());
            auto [ left, right ] = median(h, u, neighbours);
            medians[orient::upper_left][u] = left;
            medians[orient::upper_right][u] = right;
            neighbours.clear();
        }
    }

    template< typename Neighbours >
    std::pair<vertex_t, vertex_t> median(const hierarchy& h, vertex_t u, Neighbours neigh) {
        int count = neigh.size();
        int m = count / 2;
        if (count == 0) {
            return { u, u };
        }
        std::nth_element(
                neigh.begin(), 
                neigh.begin() + m, 
                neigh.end(),
                [&h] (auto u, auto v) {
                    return h.pos[u] < h.pos[v];
                }
        );
        vertex_t right = *(neigh.begin() + m);
        if (count % 2 == 1) {
            return { right, right };
        }
        std::nth_element(
                neigh.begin(), 
                neigh.begin() + m - 1, 
                neigh.end(),
                [&h] (auto u, auto v) {
                    return h.pos[u] < h.pos[v];
                }
        );
        return { *(neigh.begin() + m - 1), right };
    }


    // Is 'u' tail of an inner segment?
    bool is_inner(const detail::hierarchy& h, vertex_t u) {
        if (h.g.out_degree(u) != 1 || !h.g.is_dummy(u)) {
            return false;
        }
        return h.g.is_dummy( *h.g.out_neighbours(u).begin() );
    }

    /**
     * Vertex 'u' must be a tail of inner segment.
     * Returns the position of the head of this inner segment on its layer.
     */
    int inner_pos(const hierarchy& h, vertex_t u) {
        return h.pos[ *h.g.out_neighbours(u).begin() ];
    }

    /**
     * Saves edges causing type 1 conflicts.
     * Type 1 conflict occur when non-inner edge crosses inner segment.
     * Inner segment is an edge between two dummy vertices.
     */
    void mark_conflicts(const hierarchy& h) {
        if (h.size() < 4) {
            return;
        }
        for (int i = 1; i < h.size() - 2; ++i) {
            int last_pos = 0;
            int p = 0;

            for ( int j = 0; j < h.layers[i].size(); ++j ) {
                auto& lay = h.layers[i];
                vertex_t u = lay[j];

                if ( j == h.layers[i].size() - 1 || is_inner(h, u) ) {
                    int curr_pos = h.layers[i + 1].size();

                    if (is_inner(h, u)) {
                        curr_pos = inner_pos(h, u);
                    }

                    while(p <= j) {
                        vertex_t pth = h.layers[i][p];
                        for (auto v : h.g.out_neighbours(pth)) {
                            if (h.pos[v] < last_pos || h.pos[v] > curr_pos) {
                                conflicting.insert(pth, v);
                            }
                        }
                        ++p;
                    }
                    last_pos = curr_pos; 
                }
            }
        }
    }

    // for each vertex choose the vertex it will be verticaly aligned to
    void vertical_align(const detail::hierarchy& h, orient dir) {
        detail::vertex_map<vertex_t>& align = this->align[dir];
        detail::vertex_map<vertex_t>& root = this->root[dir];

        for ( auto l : idx_range(h.size(), !up(dir)) ) {
            auto layer = h.layers[l];

            int m_pos = left(dir) ? 0 : h.g.size();
            int d = left(dir) ? 1 : -1;

            for ( auto k : idx_range(layer.size(), !left(dir)) ) {
                vertex_t u = layer[k];
                
                for ( auto m : { medians[dir][u], medians[invert_horizontal(dir)][u] } ) {
                    if (m != u && !is_conflicting(u, m, dir) && d*h.pos[m] >= d*m_pos) {
                        align[m] = u;
                        root[u] = root[m];
                        align[u] = root[m];

                        m_pos = h.pos[m] + d;
                        break;
                    }
                }
            }
        }
    }

    void horizontal_compaction(const hierarchy& h, orient dir) {
        auto& sink = this->sink[dir];
        auto& root = this->root[dir];
        auto& shift = this->shift[dir];
        auto& x = this->x[dir];

        for (auto u : h.g.vertices()) {
            if (root[u] == u) {
                place_block(h, u, dir);
            }
        }

        for (auto i : idx_range(h.size(), up(dir))) {
            auto layer = h.layers[i];
            for (auto u : layer) {
                x[u] = *x[ root[u] ] + shift[ sink[ root[u] ] ];   

                if (*x[u] > max[dir]) {
                    max[dir] = *x[u];
                }

                if (*x[u] < min[dir]) {
                    min[dir] = *x[u];
                }
            }
        }
    }

    void place_block(const detail::hierarchy& h, vertex_t u, orient type) {
        if (x[type][u]) {
            return;
        }

        auto& sink = this->sink[type];
        auto& root = this->root[type];
        auto& shift = this->shift[type];
        auto& align = this->align[type];
        auto& x = this->x[type];

        x[u] = 0;
        int d = left(type) ? -1 : 1;
        vertex_t w = u;
        do {
            if ( !is_last_idx(h.pos[w], h.layer(w).size(), !left(type)) ) {
                vertex_t v = h.layer(w)[ h.pos[w] + d ];
                vertex_t rv = root[v];

                place_block(h, rv, type);

                if (sink[u] == u)
                    sink[u] = sink[rv];

                if (sink[u] != sink[rv]) {
                    float new_shift = shift[ sink[rv] ] + *x[rv] - *x[u] - d*(left(type) ? node_dist(v, w) : node_dist(w, v));
                    shift[ sink[u] ] = left(type) ? std::max(shift[ sink[u] ], new_shift)
                                                    : std::min(shift[ sink[u] ], new_shift);
                } else {
                    float new_x = *x[rv] - d*(left(type) ? node_dist(v, w) : node_dist(w, v));
                    x[u] = !left(type) ? std::min(*x[u], new_x)
                                       : std::max(*x[u], new_x);
                }
            }
            w = align[w];
        } while (w != u);
    }
    

    bool left(orient dir) const { return dir == orient::lower_left || dir == orient::upper_left; }
    bool up(orient dir) const { return dir == orient::upper_left || dir == orient::upper_right; }
    
    // do the vertex 'u' and its median 'med' participate in type 1 conflict?
    bool is_conflicting(vertex_t u, vertex_t med, orient dir) {
        return (  up(dir) && conflicting.contains(med, u) ) || 
               ( !up(dir) && conflicting.contains(u, med) );
    }

    float node_dist(vertex_t u, vertex_t v) {
        return boxes[u].size.x - boxes[u].center.x +
               boxes[v].center.x + 
               attr.node_dist;
    }

    // Get the range of indexes.
    range<int> idx_range(std::size_t size, bool desc) const {
        if ( desc ) {
            return { static_cast<int>(size) - 1, -1, -1 };
        }
        return { 0, static_cast<int>(size), 1 };
    }

    // Is i one of the endpoints of the interval <0, size - 1>
    bool is_last_idx(int i, std::size_t size, bool desc) const {
        return (desc && i == size - 1) || (!desc && i == 0);
    }

    orient invert_horizontal(orient dir) {
        switch(dir) {
            case orient::upper_left:
                return orient::upper_right;
            case orient::upper_right:
                return orient::upper_left;
            case orient::lower_left:
                return orient::lower_right;
            case orient::lower_right:
                return orient::lower_left;      
        }
        assert(false);
    }

};

} // namespace detail