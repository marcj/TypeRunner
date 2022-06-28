#pragma once

#include <vector>
#include <algorithm>
#include <map>

#include "utils.hpp"
#include "graph.hpp"


namespace detail {

struct edge {
    vertex_t from, to;
};

inline bool operator==(edge lhs, edge rhs) { return lhs.to == rhs.to && lhs.from == rhs.from; }
inline bool operator!=(edge lhs, edge rhs) { return !(lhs == rhs); }

inline edge reversed(edge e) { return {e.to, e.from}; }

inline std::ostream& operator<<(std::ostream& out, edge e) {
    out << "(" << e.from << ", " << e.to << ")";
    return out;
}

// ----------------------------------------------------------------------------------------------
// --------------------------------------  SUBGRAPH  --------------------------------------------
// ----------------------------------------------------------------------------------------------


/**
 * Subgraph of a given graph.
 * 
 * Vertices which are added after the construction are called dummy vertices. 
 * They can be used to distinguis between the vertices of the original graph and vertices which were added for "algorithmic" purpouses.
 */
class subgraph {
    graph& m_source;
    std::vector< vertex_t > m_vertices;
    vertex_t m_dummy_border;
    
public:
    subgraph(graph& g, std::vector< vertex_t > vertices) 
        : m_source(g)
        , m_vertices(std::move(vertices)) {
            if (!m_vertices.empty()) {
                m_dummy_border = 1 + *std::max_element(m_vertices.begin(), m_vertices.end());
            } else {
                m_dummy_border = 0;
            }
        }

    unsigned size() const { return m_vertices.size(); }

    void add_edge(edge e) { m_source.add_edge(e.from, e.to); }
    void add_edge(vertex_t u, vertex_t v) { add_edge( { u, v } ); }

    vertex_t add_dummy() { 
        auto u = m_source.add_node();
        m_vertices.push_back(u);
        return u;
    }

    bool is_dummy(vertex_t u) const { return u >= m_dummy_border; }

    void remove_edge(edge e) { m_source.remove_edge(e.from, e.to); }
    void remove_edge(vertex_t u, vertex_t v) { remove_edge( { u, v } ); }

    bool has_edge(edge e) const { 
        auto out = out_neighbours(e.from);
        return std::find(out.begin(), out.end(), e.to) != out.end();
    }
    bool has_edge(vertex_t u, vertex_t v) const { return has_edge( { u, v } ); }

    const std::vector<vertex_t>& out_neighbours(vertex_t u) const { return m_source.out_neighbours(u); }
    const std::vector<vertex_t>& in_neighbours(vertex_t u) const { return m_source.in_neighbours(u); }
    chain_range< std::vector<vertex_t> > neighbours(vertex_t u) const { return { out_neighbours(u), in_neighbours(u) }; }

    vertex_t out_neighbour(vertex_t u, int i) const { return m_source.out_neighbours(u)[i]; }
    vertex_t in_neighbour(vertex_t u, int i) const { return m_source.in_neighbours(u)[i]; }

    unsigned out_degree(vertex_t u) const { return m_source.out_neighbours(u).size(); }
    unsigned in_deree(vertex_t u) const { return m_source.in_neighbours(u).size(); }

    const std::vector<vertex_t>& vertices() const { return m_vertices; }
    vertex_t vertex(int i) const { return m_vertices[i]; }
};


inline std::ostream& operator<<(std::ostream& out, const subgraph& g) {
    for (auto u : g.vertices()) {
        out << u << ": {";
        const char* sep = "";
        for (auto v : g.out_neighbours(u)) {
            out << sep << v;
            sep = ", ";
        }
        out << "}\n";
    }
    return out;
}


// ----------------------------------------------------------------------------------------------
// --------------------------------------  SPLITING  --------------------------------------------
// ----------------------------------------------------------------------------------------------

/**
 * Recursively assign u and all vertices reachable from u in the underlying undirected graph to the same component.
 */
inline void split(const graph& g, std::vector<bool>& done, std::vector<vertex_t>& component, vertex_t u) {
    done[u] = true;
    component.push_back(u);
    for (auto v : g.out_neighbours(u)) {
        if (!done[v]) {
            split(g, done, component, v);
        }
    }
    for (auto v : g.in_neighbours(u)) {
        if (!done[v]) {
            split(g, done, component, v);
        }
    }
}


/**
 * Split the given graph into connected components represented by subgrapgs.
 */
inline std::vector<subgraph> split(graph& g) {
    std::vector< std::vector<vertex_t> > components;
    std::vector< bool > done(g.size(), false);

    for (auto u : g.vertices()) {
        if (!done[u]) {
            components.emplace_back();
            split(g, done, components.back(), u);
        }
    }

    std::vector<subgraph> subgraphs;
    for (auto component : components) {
        subgraphs.emplace_back(g, component);
    }

    return subgraphs;
}


// ----------------------------------------------------------------------------------------------
// -----------------------------------  VERTEX MAP  ---------------------------------------------
// ----------------------------------------------------------------------------------------------

/**
 * Maps vertices to objects of type T
 */
template< typename T >
struct vertex_map {
    std::vector< T > data;

    vertex_map() = default;

    vertex_map(const graph& g) : data(g.size(), T{}) {}
    vertex_map(const graph& g, T val) : data(g.size(), val) {}

    vertex_map(const subgraph& g) : vertex_map(g, T{}) {}
    vertex_map(const subgraph& g, T val) {
        for (auto u : g.vertices()) {
            if (u >= data.size()) {
                data.resize(u + 1);
            }
            data[u] = val;
        }
    }

    void resize(const graph& g) { data.resize(g.size()); }
    void resize(const graph& g, T val) { data.resize(g.size(), val); }
    void resize(const subgraph& g) { resize(g, T{}); }
    void resize(const subgraph& g, T val) {
        for (auto u : g.vertices()) {
            if (u >= data.size()) {
                data.resize(u + 1);
                data[u] = val;
            }
        }
    }

    void init(const subgraph& g, T val) {
        for (auto u : g.vertices()) {
            if (u >= data.size()) {
                data.resize(u + 1);
            }
            data[u] = val;
        }
    }

    // only to be used with T = bool, because the operator[] doesnt work :(
    T at(vertex_t u) const { return data[u]; }
    void set(vertex_t u, T val) { data[u] = val; }

    T& operator[](vertex_t u) { return data[u]; }
    const T& operator[](vertex_t u) const { return data[u]; }

    void insert(vertex_t u, const T& val) {
        add_vertex(u);
        data[u] = val;
    }
    void insert(vertex_t u) { insert(u, T{}); }

    void add_vertex(vertex_t u) {
        if (u >= data.size()) {
            data.resize(u + 1);
        }
    }

    bool contains(vertex_t u) const {
        return u < data.size();
    }

    void clear() { data.clear(); }
};


struct edge_set {
    vertex_map< std::vector<vertex_t> > data;

    bool contains(edge e) const { return contains(e.from, e.to); }
    bool contains(vertex_t u, vertex_t v) const { 
        return data.contains(u) && std::find(data[u].begin(), data[u].end(), v) != data[u].end();
    }

    void insert(edge e) { insert(e.from, e.to); }
    void insert(vertex_t u, vertex_t v) {
        data.add_vertex(u);
        data[u].push_back(v);
    }

    bool remove(edge e) { return remove(e.from, e.to); }
    bool remove(vertex_t u, vertex_t v) {
        if (!data.contains(u))
            return false;
        auto it = std::find(data[u].begin(), data[u].end(), v);
        if (it == data[u].end())
            return false;
        data[u].erase(it);
        return true;
    }
};

#ifdef DEBUG_LABELS
std::map<vertex_t, std::string> debug_labels;
#endif


} //namespace detail
