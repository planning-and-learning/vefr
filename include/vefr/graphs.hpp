#pragma once

// What vefr's algorithms take: a graph by urd's graph concepts (urd/graphs.hpp, after the Boost Graph Library's: a
// vertex list, adjacency, incidence, edge lists, ...), an urd graph or one of one's own with the same primitives, and a
// coloring of its vertices that lives outside the graph.

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <ranges>
#include <type_traits>
#include <urd/elements/index.hpp>
#include <urd/graphs.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace vefr
{

template<urd::vertex_list_graph G>
using vertex_t = typename G::vertex_type;

/// A vertex's index, 0..n-1 where no vertex was removed.
template<typename V>
[[nodiscard]] std::size_t index(V v)
{ return urd::index_of<V>::to_index(v); }

/// One past the largest vertex index: a graph that lost vertices has holes below it.
template<urd::vertex_list_graph G>
[[nodiscard]] std::size_t vertex_bound(const G& g)
{
    auto vertices = g.vertices();  // not const: std::views::filter caches its begin
    using R = decltype(vertices);
    if constexpr (std::ranges::bidirectional_range<R> && std::ranges::common_range<R>)  // O(1): the last vertex
        return std::ranges::empty(vertices) ? 0 : index(*std::ranges::prev(std::ranges::end(vertices))) + 1;
    else
    {
        // ponytail: O(n) where vertices only go forward (a graph that lost vertices); urd could hand out its bound.
        auto bound = std::size_t { 0 };
        for (auto v : vertices)
            bound = index(v) + 1;
        return bound;
    }
}

/// A color: an integer, an enum or an urd index, read as its index.
using Color = std::uint32_t;
using Colors = urd::Store<urd::sequence<Color>>;

/// What colors a vertex: a function of a vertex to a color.
template<typename F, typename V>
concept coloring = std::regular_invocable<const F&, V>;

template<typename F, typename V>
[[nodiscard]] Color color_of(const F& coloring, V v)
{ return static_cast<Color>(urd::index_of<std::remove_cvref_t<std::invoke_result_t<const F&, V>>>::to_index(std::invoke(coloring, v))); }

/// Every vertex the same color.
inline constexpr auto uncolored = [](auto) { return Color { 0 }; };

/// The vertices numbered by their position in increasing order (dense, also where vertices were removed).
template<urd::vertex_list_graph G>
class Positions
{
public:
    static constexpr auto none = std::numeric_limits<std::size_t>::max();

    explicit Positions(const G& g) : m_position(vefr::vertex_bound(g), none)
    {
        for (auto v : g.vertices())
        {
            m_position[index(v)] = m_vertices.size();
            m_vertices.push_back(v);
        }
    }

    [[nodiscard]] std::size_t size() const noexcept { return m_vertices.size(); }

    [[nodiscard]] std::size_t of(vertex_t<G> v) const { return m_position[index(v)]; }

    [[nodiscard]] vertex_t<G> at(std::size_t position) const { return m_vertices[position]; }

private:
    urd::Store<urd::sequence<std::size_t>> m_position;
    urd::Store<urd::sequence<vertex_t<G>>> m_vertices;
};

}  // namespace vefr
