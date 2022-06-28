#pragma once

#include <vector>
#include <cmath>
#include <limits>
#include <utility>
#include <optional>
#include <cassert>

#include "subgraph.hpp"
#include "report.hpp"

namespace detail {

// ----------------------------------------------------------------------------------------------
// --------------------------------------  HIERARCHY  -------------------------------------------
// ----------------------------------------------------------------------------------------------

/**
 * Represents the partitioning of a graph into a set of layers.
 * Each vertex is in exatly one layer, which is its rank.
 */
struct hierarchy {
    vertex_map<int> ranking;
    vertex_map<int> pos;
    std::vector< std::vector<vertex_t> > layers;
    subgraph& g;

    hierarchy(subgraph& g) : hierarchy(g, -1) {}
    hierarchy(subgraph& g, int val) : ranking(g, val), g(g) {}

    /**
     * Calculates the span of an edge - the number of layers the edge crosses.
     * The span can be negative if the edge goes from higher layer to a lower one.
     */
    int span(vertex_t u, vertex_t v) const { return ranking[v] - ranking[u]; }

    // number of layers
    int size() const { return layers.size(); }

    // get the layer of the given vertex
    std::vector<vertex_t>& layer(vertex_t u) { return layers[ranking[u]]; }
    const std::vector<vertex_t>& layer(vertex_t u) const { return layers[ranking[u]]; }

    // recalculate positions according to the current layers
    void update_pos() {
        for (const auto& l : layers) {
            int i = 0;
            for (auto u : l) {
                pos[u] = i++;
            }
        }
    }

    // get the successor/predecessor of u on its layer
    vertex_t next(vertex_t u) const { return layers[ ranking[u] ][ pos[u] + 1 ]; }
    vertex_t prev(vertex_t u) const { return layers[ ranking[u] ][ pos[u] - 1 ]; }

    // true iff u has a successor/predecessor on its layer
    bool has_next(vertex_t u) const { return pos[u] < layer(u).size() - 1; }
    bool has_prev(vertex_t u) const { return pos[u] > 0; }

    // Swap two vertices. Updates all necessary attributes.
    void swap(vertex_t u, vertex_t v) {
        std::swap(pos[u], pos[v]);
        std::swap(ranking[u], ranking[v]);

        layers[ ranking[u] ][ pos[u] ] = u;
        layers[ ranking[v] ][ pos[v] ] = v;
    }

    // is i a valid index in the given layer?
    bool valid_pos(int layer_idx, int i) const {
        return i >= 0 && i < layers[layer_idx].size();
    }
};

std::ostream& operator<<(std::ostream& out, const hierarchy& h) {
    /*out << "ranking: [";
    for (auto u : h.g.vertices()) {
        out << u << "(" << h.ranking[u] << "), ";    
    }
    out << "]\n";
    for (const auto& l : h.layers) {
        for (auto u : l) {
            out << u << " ";
        }
        out << "\n";
    }
    return out;*/

    for (auto& l : h.layers) {
        for (auto u : l) {
            out << u << " ";
        }
        out << "\n";
    }
    return out;
}

// ------------------------------------------------------------------------------------------
// --------------------------------  SPLITING LONG EDGES  -----------------------------------
// ------------------------------------------------------------------------------------------


/**
 * Edge which crosses several layers and had to by split into multiple "subedges".
 */
struct long_edge {
    edge orig;
    std::vector<vertex_t> path;

    /* so emplace_back can be used */
    long_edge(edge orig, std::vector<vertex_t> path) : orig(orig), path(std::move(path)) {}
};

/**
 * Splits all edges that span across more than one layer in the provided hierarchy
 * into segments that each span across just one layer.
 * ( aka converts the hierachy into proper hierarchy )
 * 
 * @return list of edges which had to be split
 */
std::vector< long_edge > add_dummy_nodes(hierarchy& h) {
    std::vector< long_edge > split_edges;

    // find edges to be split
    for (auto u : h.g.vertices()) {
        for (auto v : h.g.out_neighbours(u)) {
            int span = h.span(u, v);
            if (span > 1) {
                split_edges.emplace_back( edge{u, v}, std::vector<vertex_t>{} );
            }
        }
    }

    // split the found edges
    for (auto& [ orig, path ] : split_edges) {
        int span = h.span(orig.from, orig.to);
        path.push_back(orig.from);

        vertex_t s = orig.from;
        for (int i = 0; i < span - 1; ++i) {
            vertex_t t = h.g.add_dummy();

            h.ranking.add_vertex(t);
            h.ranking[t] = h.ranking[s] + 1;
            h.layers[ h.ranking[t] ].push_back(t);
            h.pos.insert( t, h.layers[ h.ranking[t] ].size() - 1 );

            h.g.add_edge(s, t);
            path.push_back(t);

            s = t;
        }
        path.push_back(orig.to);
        h.g.add_edge(s, orig.to);
        h.g.remove_edge(orig);
    }

    return split_edges;
}

// --------------------------------------------------------------------------------------
// ------------------------------  LAYERING  --------------------------------------------
// --------------------------------------------------------------------------------------


/**
 * Interface for an algorithm which constructs a hierarchy for a given graph.
 */
struct layering {
    virtual hierarchy run(detail::subgraph&) = 0;
    virtual ~layering() = default;
};


struct tree_edge {
    vertex_t u, v;
    int dir;
};

std::ostream& operator<<(std::ostream& out, tree_edge e) {
    out << edge{e.u, e.v} << "| " << e.dir;
    return out;
}


struct tree_node {
    std::optional<vertex_t> parent = std::nullopt;
    vertex_t u;

    int cut_value = 0;
    int out_cut_value = 0;

    std::vector<vertex_t> children;

    // for marking the order of nodes using pre-order traversal
    int min; 
    int order;
};


struct tight_tree {
    vertex_map<tree_node> nodes;
    vertex_t root;
    hierarchy* h;

    tight_tree() = default;
    
    tight_tree(hierarchy* h, vertex_t root) : nodes(h->g), root(root), h(h) {
        for ( auto u : h->g.vertices() ) {
            nodes[u].u = u;
        }
    }

    std::vector<vertex_t>& children(vertex_t u) { return nodes[u].children; }
    const std::vector<vertex_t>& children(vertex_t u) const { return nodes[u].children; }

    vertex_t parent(vertex_t u) const { return *nodes[u].parent; }

    tree_node& node(vertex_t u) { return nodes[u]; }
    const tree_node& node(vertex_t u) const { return nodes[u]; }

    int cut_val(vertex_t u, vertex_t v) const {
        assert(u == *nodes[v].parent);
        return nodes[v].cut_value;
    }

    void cut_val(vertex_t u, vertex_t v, int val) {
        assert(u == *nodes[v].parent);
        nodes[v].cut_value = val;
    }
    
    int out_cut_val(vertex_t u, vertex_t v) const {
        assert(u == *nodes[v].parent);
        return nodes[v].out_cut_value;
    }

    void out_cut_val(vertex_t u, vertex_t v, int val) {
        assert(u == *nodes[v].parent);
        nodes[v].out_cut_value = val;
    }

    void add_child(vertex_t parent, vertex_t child) {
        nodes[parent].children.push_back(child);
        nodes[child].parent = parent;
    }

    void remove_child(vertex_t parent, vertex_t child) {
        unlink_child(parent, child);
        nodes[child].parent = -1;
    }

    void unlink_child(vertex_t parent, vertex_t child) {
        nodes[parent].children.erase(std::find(nodes[parent].children.begin(), nodes[parent].children.end(), child));
    }

    /**
     * Find which component of the tree <u> falls in, if the tree is split by removing <e>.
     * Postorder **must** by calculated first.
     */
    vertex_t component(edge e, vertex_t u) {
        if (nodes[e.to].min > nodes[u].min || nodes[e.to].order < nodes[u].order) {
            return e.from;
        }
        return e.to;
    }

    // 1 if (parent, u) is an edge, -1 if (u, parent) is an edge
    int dir(vertex_t u, vertex_t v) const { return sgn( h->span(u, v) ); }

    // find a common ancestor of two vertices, postorder **must** be calculated first
    vertex_t common_acestor(vertex_t u, vertex_t v) {
        vertex_t ancestor = u;

        int left_min;
        int right_order;
        if (nodes[u].order < nodes[v].order) {
            left_min = nodes[u].min;
            right_order = nodes[v].order;
        } else {
            left_min = nodes[v].min;
            right_order = nodes[u].order;
        }

        // the common acestor is the first node such that min <= left_min && order >= right_order
        while (nodes[ancestor].min > left_min || nodes[ancestor].order < right_order) {
            // no need to check if parent exists, root has the smalles min and the largest order
            ancestor = *nodes[ancestor].parent;
        }

        return ancestor;
    }

    template<typename F>
    void for_each(F f, vertex_t u) const {
        f(u);
        for ( const auto& child : children(u) ) {
            for_each(f, child);
        }
    }

    /**
     * Performs a postorder search of the subtree rooted at <u>.
     * The first leaf node is given the order <order>.
     */
    int postorder_search(vertex_t u, int order) {
        node(u).min = order;
        for (auto child : children(u)) {
            order = postorder_search(child, order);
        }
        node(u).order = order;
        return order + 1;
    }

    void print(vertex_t u, int depth, std::ostream& out = std::cout) const {
        int indent = 4;
        for (int i = 0; i < depth; ++i) {
            for (int j = 0; j < indent; ++j) {
                out << " ";
            }
        }
        out << u << "(";
        out << "dir=" << (u == root ? 0 : dir(parent(u), u)) << " ";
        out << "cut=" << node(u).cut_value << " "; 
        out << "out=" << node(u).out_cut_value << " ";
        out << "ord=" << node(u).order << " ";
        out << "min=" << node(u).min << " ";
        out << ")\n";

        for ( const auto& child : children(u) ) {
            print(child, depth + 1, out);
        }
    }

    friend std::ostream& operator<<(std::ostream& out, const tight_tree& tree) {
        tree.print(tree.root, 0, out);
        return out;
    }

    /**
     * Swaps a non-tree edge <entering> for a tree edge <leaving> and restructures the tree so
     * that <leaving.to> is the predecessor of all edges in the subtree rooted at <leaving.to>
     */
    void swap_edges(edge entering, edge leaving) {
        vertex_t parent = entering.from;
        vertex_t u = entering.to;
        while(u != leaving.from) {
            //std::cout << u << "\n";
            vertex_t tmp = *nodes[u].parent;
            nodes[u].parent = parent;
            nodes[parent].children.push_back(u);
            parent = u;
            u = tmp;
            unlink_child(u, parent);
        }
    }
};


/**
 * Network simple algorithm for layering a graph.
 */
class network_simplex_layering : public layering {
    tight_tree tree;

public:

    hierarchy run(subgraph& g) override {
        if (g.size() == 0) {
            return hierarchy(g);
        }
        auto h = init_hierarchy(g);

        init_tree(h);
        init_cut_values();

        optimize_edge_length(g, h);

#ifdef DEBUG_LABELS
        for (int i = 0; i < g.size(); ++i) {
            debug_labels[i] = std::to_string(i) + "(" +
                        std::to_string(tree.node(i).parent ? *tree.node(i).parent : -1) + ", " +
                        std::to_string(tree.node(i).cut_value) + ")";
        }
#endif

        calculate_layers(h);

        return h;
    }

private:

    void  calculate_layers(hierarchy& h) {
        int min = std::numeric_limits<int>::max();
        int max = std::numeric_limits<int>::min();
        for (auto u : h.g.vertices()) {
            if (h.ranking[u] > max)
                max = h.ranking[u];
            if (h.ranking[u] < min)
                min = h.ranking[u];
        }

        h.layers.resize(max - min + 1);
        h.pos.resize(h.g);

        for (auto u : h.g.vertices()) {
            h.ranking[u] -= min;
            h.layers[ h.ranking[u] ].push_back(u);
            h.pos[u] = h.layers[ h.ranking[u] ].size() - 1;
        }
    }


    /**
     * Assignes each vertex a layer, such that each edge goes from a lower layer to higher one
     * and the source vertices are at the lowest layer.
     * The resulting ranking is not normalized, meaning that the lowest layer doesnt have to be 0.
     * It is also not neccesarly the optimal ranking.
     * 
     * @param g the graph whose vertices are to be assigned to layers
     * @return resulting hierarchy, only the ranking of nodes is defined, layers and pos are undefined
     */
    hierarchy init_hierarchy(subgraph& g) {
        hierarchy h(g, -1);
        int curr = 0;
        unsigned processed = 0;

        std::vector<vertex_t> to_rank;
        while (processed < g.size()) {
            to_rank.clear();
            for (auto u : g.vertices()) {
                if (h.ranking[u] == -1) {
                    // check if there are any edges going from unranked vertices
                    bool source = true;
                    for (auto v : g.in_neighbours(u)) {
                        if (h.ranking[v] == -1) {
                            source = false;
                            break;
                        }
                    }
                    if (source) {
                        to_rank.push_back(u);
                        ++processed;
                    }
                }
            }
            for (auto u : to_rank) {
                h.ranking[u] = curr;
            }
            ++curr;
        }
        return h;
    }

    /**
     * Constructs a tree of all vertices
     * reachable from the root through tight edges. 
     */
    int basic_tree(vertex_map<bool>& done, const hierarchy& h, vertex_t root) {
        done.set(root, true);
        int added = 1;

        for ( auto u : h.g.neighbours(root) ) {
            int span = h.span(root, u);
            if ( !done.at(u) && std::abs(span) == 1 ) {
                tree.add_child(root, u);
                added += basic_tree(done, h, u);
            }
        }

        return added;
    }

    /**
     * Finds a spanning tree of tight edges. 
     * Tight edge is any edge (u, v) for which ranking[v] - ranking[u] == 1.
     * 
     * @param g the graph
     * @param ranking ranking of the vertices of the graph
     * @return the spanning tree
     */
    void init_tree(hierarchy& h) {
        const subgraph& g = h.g;
        tree = tight_tree( &h, g.vertex(0) );
        vertex_map<bool> done(g, false);

        int finished = basic_tree(done, h, tree.root);

        while(finished < g.size()) {
            // in the underlying undirected graph find edge (u, v) with the smallest span
            // such that u is already in the tree and v is not in the tree
            edge e = { 0, 0 };
            int min_span = std::numeric_limits<int>::max();
            for ( auto u : g.vertices() ) {
                if ( done.at(u) ) {
                    for ( auto v : g.neighbours(u) ) {
                        int span = h.span(u, v);
                        if ( !done.at(v) && std::abs(span) < std::abs(min_span) ) {
                            e = { u, v };
                            min_span = span;
                        }
                    }
                }
            }

            // make the edge tight by moving all the vertices in the tree
            int dir = sgn(min_span);
            min_span -= dir;
            for (auto u : g.vertices()) {
                if (done.at(u)) {
                    h.ranking[u] += min_span;
                }
            }
            tree.add_child(e.from, e.to);
            done.set(e.to, true);
            ++finished;
        }

        tree.postorder_search(tree.root, 0);
    }


    // Calculates the initial cut values of all edges in the tight tree.
    void init_cut_values() {
        for ( auto child : tree.children(tree.root) ) {
            init_cut_values(tree.root, child);
        }
    }

    // Calculates cut values of all edges in the subtree rooted at <v> and then the cut value of (<u>, <v>).
    void init_cut_values(vertex_t u, vertex_t v) {
        for (auto child : tree.children(v)) {
            init_cut_values(v, child);
        }
        set_cut_value(u, v);
    }

    /**
     * Calculates cut value of the edge (u, v). 
     * Requires the cut values of the edges between <v> and its children to be already calculated.
     */
    void set_cut_value(vertex_t u, vertex_t v) {
        subgraph& g = tree.h->g;
        int val = 0;

        for (auto child : tree.children(v)) {
            val += tree.dir(u, v) * tree.dir(v, child) * tree.out_cut_val(v, child);
        }

        for (auto x : g.neighbours(v)) {
            if (tree.component( {u, v}, x ) == u) {
                val += tree.dir(x, v) * tree.dir(u, v);
            }
        }
        tree.cut_val(u, v, val);

        for (auto x : g.neighbours(u)) {
            if (tree.component( {u, v}, x ) == v) {
                val -= tree.dir(u, x) * tree.dir(u, v);
            }
        }
        tree.out_cut_val(u, v, val);  
    }

    void switch_tree_edges(tree_edge orig, tree_edge entering) {
        auto ancestor = tree.common_acestor(entering.u, entering.v);

        tree.swap_edges({entering.u, entering.v},{orig.u, orig.v});

        tree.postorder_search(ancestor, tree.node(ancestor).min);
        fix_cut_values(ancestor, orig.u);
        fix_cut_values(ancestor, orig.v);
    }

    void fix_cut_values(vertex_t root, vertex_t u) {
        while (u != root) {
            vertex_t parent = *tree.node(u).parent;
            set_cut_value(parent, u);
            u = parent;
        }
    }

    void move_subtree(hierarchy& h, vertex_t root, int d) {
        h.ranking[root] += d;
        for (auto child : tree.children(root)) {
            move_subtree(h, child, d);
        }
    }

    tree_edge find_entering_edge(const subgraph& g, hierarchy& h, tree_edge leaving) {
        tree_edge entering { 0, 0, -leaving.dir };
        int span = std::numeric_limits<int>::max();

        vertex_t start_component = leaving.dir == 1 ? leaving.v : leaving.u;
        vertex_t end_component = start_component == leaving.u ? leaving.v : leaving.u;

        for (auto u : g.vertices()) {
            if (tree.component({leaving.u, leaving.v}, u) != start_component)
                continue;

            for (auto v : g.out_neighbours(u)) {
                if (tree.component({leaving.u, leaving.v}, v) == end_component && h.span(u, v) < span) {
                    entering.u = entering.dir == 1 ? u : v;
                    entering.v = entering.dir == 1 ? v : u;
                    span = h.span(u, v);
                }
            }
        }

        assert(span < std::numeric_limits<int>::max());

        return entering;
    }

    std::optional<tree_edge> find_leaving_edge(hierarchy& h) {
        for (auto u : h.g.vertices()) {
            for (auto v : h.g.out_neighbours(u)) {
                if (v != tree.root && tree.parent(v) == u && tree.node(v).cut_value < 0) {
                    return tree_edge{ u, v, 1 };
                }
                if (u != tree.root && tree.parent(u) == v && tree.node(u).cut_value < 0) {
                    return tree_edge{ v, u, -1 };
                }
            }
        }

        return std::nullopt;
    }

    void optimize_edge_length(const subgraph& g, hierarchy& h) {
        int iters = 0;

        while(true) {
            auto leaving = find_leaving_edge(h);
            if (!leaving)
                break;

            tree_edge entering = find_entering_edge(g, h, *leaving);

            switch_tree_edges(*leaving, entering);

            int d = h.span( entering.u, entering.v );
            d = -d + sgn(d);
            move_subtree(h, entering.v, d);

            iters++;
        }

#ifdef REPORTING
        report::simplex::iters = iters;
#endif
    }

};


} // namespace detail
