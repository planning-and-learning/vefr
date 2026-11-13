#pragma once

// Properties of a graph's edges, read from its neighbours.

#include "vefr/graphs.hpp"

#include <algorithm>

namespace vefr
{

/// Whether no edge leads from a vertex to itself.
template<urd::graph_like G>
[[nodiscard]] bool is_loopless(const G& g)
{
    for (auto u : g.vertices())
        for (auto v : g.neighbours(u))
            if (v == u)
                return false;
    return true;
}

/// Whether two edges lead from one vertex to the same vertex.
template<urd::graph_like G>
[[nodiscard]] bool is_multigraph(const G& g)
{
    auto targets = urd::Store<urd::sequence<std::size_t>>();
    for (auto u : g.vertices())
    {
        targets.clear();
        for (auto v : g.neighbours(u))
            targets.push_back(index(v));
        std::ranges::sort(targets);
        if (std::ranges::adjacent_find(targets) != targets.end())
            return true;
    }
    return false;
}

/// Whether every edge has one back.
template<urd::graph_like G>
[[nodiscard]] bool is_symmetric(const G& g)
{
    for (auto u : g.vertices())
        for (auto v : g.neighbours(u))
            if (!g.adjacent(v, u))
                return false;
    return true;
}

/// Whether the graph is a simple undirected one: symmetric, without loops and parallel edges.
template<urd::graph_like G>
[[nodiscard]] bool is_simple_undirected(const G& g)
{ return is_loopless(g) && !is_multigraph(g) && is_symmetric(g); }

}  // namespace vefr
