#pragma once

// A graph read backwards: the edges of a graph that keeps its in-edges (urd's `bidirectional` or `undirected`), each
// from its target to its source. A view: it holds the graph, which outlives it. Searches on it run against the edges,
// as goal distances do.

#include "vefr/graphs.hpp"

#include <cstddef>
#include <optional>
#include <ranges>

namespace vefr
{

template<urd::in_adjacency_graph G>
class Reversed
{
public:
    using vertex_type = vertex_t<G>;

    explicit Reversed(const G& g) noexcept : m_graph(&g) {}

    [[nodiscard]] std::size_t num_vertices() const { return m_graph->num_vertices(); }

    [[nodiscard]] auto vertices() const { return m_graph->vertices(); }

    [[nodiscard]] auto neighbours(vertex_type u) const { return m_graph->in_neighbours(u); }

    [[nodiscard]] std::size_t out_degree(vertex_type u) const { return m_graph->in_degree(u); }

    [[nodiscard]] bool adjacent(vertex_type u, vertex_type v) const { return m_graph->adjacent(v, u); }

    [[nodiscard]] auto in_neighbours(vertex_type v) const { return m_graph->neighbours(v); }

    [[nodiscard]] std::size_t in_degree(vertex_type v) const { return m_graph->out_degree(v); }

    // Edges by number, where the graph numbers its edges and keeps its in-edges: the same numbers, turned around, with
    // the same data.
    [[nodiscard]] std::size_t num_edges() const
        requires urd::bidirectional_graph<G>
    { return m_graph->num_edges(); }

    [[nodiscard]] auto out_edges(vertex_type u) const
        requires urd::bidirectional_graph<G>
    { return m_graph->in_edges(u); }

    [[nodiscard]] auto in_edges(vertex_type v) const
        requires urd::bidirectional_graph<G>
    { return m_graph->out_edges(v); }

    [[nodiscard]] auto edges() const
        requires urd::bidirectional_graph<G>
    { return m_graph->edges(); }

    [[nodiscard]] vertex_type source(std::size_t e) const { return m_graph->target(e); }

    [[nodiscard]] vertex_type target(std::size_t e) const { return m_graph->source(e); }

    [[nodiscard]] decltype(auto) operator[](std::size_t e) const { return (*m_graph)[e]; }

    [[nodiscard]] decltype(auto) edge_value(std::size_t e) const { return m_graph->edge_value(e); }

    [[nodiscard]] decltype(auto) vertex_value(vertex_type v) const { return m_graph->vertex_value(v); }

    /// The graph read backwards.
    [[nodiscard]] const G& base() const noexcept { return *m_graph; }

private:
    const G* m_graph;
};

/// `g` read backwards.
template<urd::in_adjacency_graph G>
[[nodiscard]] Reversed<G> reversed(const G& g) noexcept
{ return Reversed<G>(g); }

}  // namespace vefr
