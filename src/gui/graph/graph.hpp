#pragma once

#include <vector>
#include <iostream>
#include <ostream>
#include <algorithm>

#include "utils.hpp"
#include "types.hpp"

/**
 * Basic clas for representing a directed graph.
 */
class graph {
public:
    std::vector<float> nodeSizes;

    /**
     * Add a new vertex to the graph.
     * 
     * @return the identifier of the vertex 
     */
    vertex_t add_node(float size = 0) {
        nodeSizes.push_back(size);
        m_out_neighbours.emplace_back();
        m_in_neighbours.emplace_back();
        return m_out_neighbours.size() - 1;
    }

    /**
     * Add a new edge to the graph.
     * 
     * The behavious is undefined if the same edge is added twice 
     * or if an identifier other then one returned by add_node() is used.
     * 
     * @param from the identifier of the starting vertex
     * @param to   the identifier of the ending vertex
     * 
     * @return a reference to the graph for chaining multiple calls
     */
    graph& add_edge(vertex_t from, vertex_t to) { 
        m_out_neighbours[from].push_back(to);
        m_in_neighbours[to].push_back(from);
        return *this;
    }

    /**
     * Get the number of vertices in the graph.
     * 
     * @return the number of vertices
     */
    unsigned size() const { return m_out_neighbours.size(); }

    /**
     * Get an immutable list of all successors.
     */
    const std::vector<vertex_t>& out_neighbours(vertex_t u) const { return m_out_neighbours[u]; }
    /**
     * Get an immutable list of all predecessors.
     */
    const std::vector<vertex_t>& in_neighbours(vertex_t u) const { return m_in_neighbours[u]; }
   
    /**
     * Get an implementation defined object which can be used for iterating through the vertices.
     * 
     * The returned object can be used in a range for loop.
     * It provides begin() and end() methods which yield forward iterators for iterating through the vertices.
     */
    range<vertex_t> vertices() const { return range<vertex_t>(0, size(), 1); }

    /**
     * Remove the given edge.
     * Slow operation - should be avoided if possible.
     */
    void remove_edge(vertex_t from, vertex_t to) {
        remove_neighour(m_out_neighbours[from], to);
        remove_neighour(m_in_neighbours[to], from);
    }

    friend std::ostream& operator<<(std::ostream& out, const graph& g) {
        for (auto u : g.vertices()) {
            out << u << ": [";

            const char* sep = "";
            for (auto v : g.out_neighbours(u)) {
                out << sep << v;
                sep = ", ";
            }

            out << "]\n";
        }
        return out;
    }

private:
    std::vector< std::vector<vertex_t> > m_out_neighbours;
    std::vector< std::vector<vertex_t> > m_in_neighbours;

    void remove_neighour(std::vector<vertex_t>& neighbours, vertex_t u) {
        auto it = std::find(neighbours.begin(), neighbours.end(), u);
        if (it != neighbours.end()) {
            neighbours.erase(it);
        }
    }

};

/**
 * Builder class for creating graphs by hand.
 * 
 * Should be used with care since the node identifiers are chosen by the caller.
 * The chosen identifiers should be a consecutive sequence of numbers [0, n-1] where n is the number of vertices in the graph. 
 * Otherwise, the unused identifiers in the range [0, max_id] will be added into the graph as vertices
 * without any edges which is probably not what you want.
 * 
 * For example if edges (0,2), (2,3), (0,3) are added, 
 * the resulting graph will contain an aditional vertex with identifier 1.
 */
struct graph_builder {
    graph g;

    /**
     * Add a new edges with the given identifiers.
     */
    graph_builder& add_edge(vertex_t u, vertex_t v) {
        add_vertex(u);
        add_vertex(v);
        g.add_edge(u, v);
        return *this;
    }

    /**
     * Get the resulting graph.
     */
    graph build() { return g; }
    
private:
    void add_vertex(vertex_t u) {
        while (u >= g.size()) {
            g.add_node();
        }
    }
};
