#pragma once

// Graph algorithms written on a graph's ranges: its vertices() and each vertex's out-edges (out_edges(u) where it
// numbers its edges, else neighbours(u)). As in the Boost Graph Library, a search reports what it does to a visitor,
// and the visitor may stop it; unlike there, a visitor is any object with the events it wants, visitors compose
// (vefr::visitors), and the parameters are plain arguments: sources are any range of vertices, a weight is a function of
// an edge or of its number, and results are urd Stores keyed by vertex (VertexMap).

#include "vefr/graphs.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <urd/repr/queues/queue.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <utility>

namespace vefr
{

/// An edge as the algorithms hand it out: its endpoints and its number (`none` where the graph numbers no edges).
template<typename V>
struct Edge
{
    static constexpr auto none = std::numeric_limits<std::size_t>::max();

    V source {};
    V target {};
    std::size_t id = none;

    friend bool operator==(const Edge&, const Edge&) = default;
};

/// The edges leaving `u`, as Edge: from their numbers in an incidence graph, else from its neighbours.
template<urd::adjacency_graph G>
[[nodiscard]] auto out_edges_of(const G& g, vertex_t<G> u)
{
    using V = vertex_t<G>;
    if constexpr (urd::incidence_graph<G>)
        return g.out_edges(u) | std::views::transform([u, g = &g](std::size_t e) { return Edge<V> { u, g->target(e), e }; });
    else
        return g.neighbours(u) | std::views::transform([u](V v) { return Edge<V> { u, v }; });
}

/// Data per vertex, keyed by vertex: sized one past the largest vertex index, so removed vertices leave holes.
template<typename T, typename V>
using VertexMap = urd::Store<urd::sequence<T, V>>;

template<typename T, urd::adjacency_graph G>
[[nodiscard]] VertexMap<T, vertex_t<G>> vertex_map(const G& g, const T& value = T {})
{ return VertexMap<T, vertex_t<G>>(vefr::vertex_bound(g), value); }

/// A range of vertices: where a search starts, or what it looks for.
template<typename R, typename G>
concept vertex_range = std::ranges::input_range<R> && std::convertible_to<std::ranges::range_reference_t<R>, vertex_t<G>>;

/// A weight per edge: a function of the edge (vefr::Edge), or of its number, which the graph must then give.
template<typename F, typename G>
concept edge_weight =
    (std::regular_invocable<const F&, const Edge<vertex_t<G>>&>) || (urd::incidence_graph<G> && std::regular_invocable<const F&, std::size_t>);

namespace detail
{
template<typename G, typename F>
[[nodiscard]] decltype(auto) weigh(const F& weight, const Edge<vertex_t<G>>& e)
{
    if constexpr (std::regular_invocable<const F&, const Edge<vertex_t<G>>&>)
        return std::invoke(weight, e);
    else
        return std::invoke(weight, e.id);
}
}  // namespace detail

template<typename G, edge_weight<G> F>
using weight_t = std::remove_cvref_t<decltype(detail::weigh<G>(std::declval<const F&>(), std::declval<const Edge<vertex_t<G>>&>()))>;

// Visitors -------------------------------------------------------------------------------------------------------------

/// What an event may answer: go on, or stop the search where it is.
enum class Flow : std::uint8_t
{
    proceed,
    stop,
};

/// A visitor with no events: the default.
struct null_visitor
{
};

namespace detail
{

// NOLINTBEGIN(cppcoreguidelines-macro-usage): an event is called by its name, which a template cannot spell
/// Calls visitor.NAME(x) where the visitor has that event: whether it asked to stop (it returned Flow::stop).
#define VEFR_EVENT(NAME)                                                 \
    template<typename Visitor, typename X>                               \
    [[nodiscard]] bool NAME(Visitor& visitor, const X& x)                \
    {                                                                    \
        if constexpr (requires { visitor.NAME(x); })                     \
        {                                                                \
            if constexpr (std::same_as<decltype(visitor.NAME(x)), Flow>) \
                return visitor.NAME(x) == Flow::stop;                    \
            else                                                         \
            {                                                            \
                visitor.NAME(x);                                         \
                return false;                                            \
            }                                                            \
        }                                                                \
        else                                                             \
            return false;                                                \
    }
/// Every event, each by its name: what a search calls and a composite visitor forwards.
#define VEFR_EVENTS(EVENT)       \
    EVENT(start_vertex)          \
    EVENT(discover_vertex)       \
    EVENT(examine_vertex)        \
    EVENT(finish_vertex)         \
    EVENT(examine_edge)          \
    EVENT(tree_edge)             \
    EVENT(non_tree_edge)         \
    EVENT(gray_target)           \
    EVENT(black_target)          \
    EVENT(back_edge)             \
    EVENT(forward_or_cross_edge) \
    EVENT(finish_edge)           \
    EVENT(edge_relaxed)          \
    EVENT(edge_not_relaxed)
VEFR_EVENTS(VEFR_EVENT)
#undef VEFR_EVENT

}  // namespace detail

/// Visitors called one after the other on each event; the search stops where any of them asks. Each is held as given:
/// a named visitor by reference, a temporary by value.
template<typename... Visitors>
class visitors
{
public:
    explicit visitors(Visitors&&... each) : m_each(std::forward<Visitors>(each)...) {}

#define VEFR_FORWARD(NAME)                                                                         \
    template<typename X>                                                                           \
    Flow NAME(const X& x)                                                                          \
    {                                                                                              \
        auto stop = false;                                                                         \
        std::apply([&](auto&... each) { ((stop = detail::NAME(each, x) || stop), ...); }, m_each); \
        return stop ? Flow::stop : Flow::proceed;                                                  \
    }
    VEFR_EVENTS(VEFR_FORWARD)
#undef VEFR_FORWARD

private:
    std::tuple<Visitors...> m_each;
};
template<typename... Visitors>
visitors(Visitors&&...) -> visitors<Visitors...>;
// NOLINTEND(cppcoreguidelines-macro-usage)

/// Stops a search once it has examined every vertex of `targets` (taken off its queue: in breadth-first search and
/// Dijkstra's, where their distances are final).
template<typename V>
class stop_at
{
public:
    template<std::ranges::input_range R>
    explicit stop_at(R&& targets)
    {
        for (V t : std::forward<R>(targets))
            m_remaining.insert(t);
    }

    Flow examine_vertex(V v)
    {
        m_remaining.erase(v);
        return m_remaining.empty() ? Flow::stop : Flow::proceed;
    }

private:
    urd::Store<urd::set<V>> m_remaining;
};
template<std::ranges::input_range R>
stop_at(R&&) -> stop_at<std::remove_cvref_t<std::ranges::range_value_t<R>>>;

namespace detail
{

/// Not reached, reached and not finished, finished.
enum class Mark : std::uint8_t
{
    white,
    gray,
    black,
};

/// A distance no path has: infinity where the type has one, else its largest value.
template<typename D>
inline constexpr D unreached = std::numeric_limits<D>::has_infinity ? std::numeric_limits<D>::infinity() : std::numeric_limits<D>::max();

}  // namespace detail

// Searches -------------------------------------------------------------------------------------------------------------

/// The vertices reached from `sources`, breadth first. Events: start_vertex, discover_vertex, examine_vertex,
/// examine_edge, tree_edge, non_tree_edge, gray_target, black_target, finish_vertex.
template<urd::adjacency_graph G, vertex_range<G> R, typename Visitor>
void breadth_first_visit(const G& g, R&& sources, Visitor&& visitor)  // NOLINT(cppcoreguidelines-missing-std-forward): called, not passed on
{
    using V = vertex_t<G>;
    auto marks = vertex_map(g, detail::Mark::white);
    auto queue = urd::Store<urd::queue<V>, urd::fifo>();
    for (V s : std::forward<R>(sources))
    {
        if (detail::start_vertex(visitor, s))
            return;
        if (marks[s] != detail::Mark::white)
            continue;
        marks[s] = detail::Mark::gray;
        if (detail::discover_vertex(visitor, s))
            return;
        queue.push(s);
    }
    while (!queue.empty())
    {
        const auto u = queue.front();
        queue.pop();
        if (detail::examine_vertex(visitor, u))
            return;
        for (const auto& e : out_edges_of(g, u))
        {
            if (detail::examine_edge(visitor, e))
                return;
            switch (marks[e.target])
            {
                case detail::Mark::white:
                    marks[e.target] = detail::Mark::gray;
                    if (detail::tree_edge(visitor, e) || detail::discover_vertex(visitor, e.target))
                        return;
                    queue.push(e.target);
                    break;
                case detail::Mark::gray:
                    if (detail::non_tree_edge(visitor, e) || detail::gray_target(visitor, e))
                        return;
                    break;
                case detail::Mark::black:
                    if (detail::non_tree_edge(visitor, e) || detail::black_target(visitor, e))
                        return;
                    break;
            }
        }
        marks[u] = detail::Mark::black;
        if (detail::finish_vertex(visitor, u))
            return;
    }
}

/// The vertices reached from `sources`, depth first, a source at a time (those already reached are skipped); without
/// recursion. Events: start_vertex, discover_vertex, examine_edge, tree_edge, back_edge, forward_or_cross_edge,
/// finish_edge, finish_vertex.
template<urd::adjacency_graph G, vertex_range<G> R, typename Visitor>
void depth_first_visit(const G& g, R&& sources, Visitor&& visitor)  // NOLINT(cppcoreguidelines-missing-std-forward): called, not passed on
{
    using V = vertex_t<G>;
    struct Frame
    {
        V vertex;
        std::size_t next;  ///< its next edge in `edges`
        std::size_t end;
        std::optional<Edge<V>> entered_by;
    };
    auto marks = vertex_map(g, detail::Mark::white);
    auto edges = urd::Store<urd::sequence<Edge<V>>>();  // the out-edges of the vertices on the path, frame after frame
    auto path = urd::Store<urd::sequence<Frame>>();
    const auto enter = [&](V u, std::optional<Edge<V>> by)
    {
        marks[u] = detail::Mark::gray;
        const auto first = edges.size();
        for (const auto& e : out_edges_of(g, u))
            edges.push_back(e);
        path.push_back(Frame { u, first, edges.size(), by });
        return detail::discover_vertex(visitor, u);
    };
    for (V s : std::forward<R>(sources))
    {
        if (marks[s] != detail::Mark::white)
            continue;
        if (detail::start_vertex(visitor, s) || enter(s, std::nullopt))
            return;
        while (!path.empty())
        {
            auto& frame = path.back();
            if (frame.next == frame.end)
            {
                const auto done = frame;
                path.pop_back();
                edges.resize(path.empty() ? 0 : path.back().end);  // its edges were the last ones
                marks[done.vertex] = detail::Mark::black;
                if (detail::finish_vertex(visitor, done.vertex) || (done.entered_by && detail::finish_edge(visitor, *done.entered_by)))
                    return;
                continue;
            }
            const auto e = edges[frame.next++];
            if (detail::examine_edge(visitor, e))
                return;
            switch (marks[e.target])
            {
                case detail::Mark::white:
                    if (detail::tree_edge(visitor, e) || enter(e.target, e))
                        return;
                    break;
                case detail::Mark::gray:
                    if (detail::back_edge(visitor, e) || detail::finish_edge(visitor, e))
                        return;
                    break;
                case detail::Mark::black:
                    if (detail::forward_or_cross_edge(visitor, e) || detail::finish_edge(visitor, e))
                        return;
                    break;
            }
        }
    }
}

/// Each vertex's predecessor on a path from a source, and the path's length; nullopt where there is none.
template<typename V, typename D>
struct Paths
{
    VertexMap<std::optional<V>, V> predecessors;
    VertexMap<D, V> distances;
};

/// Shortest paths in edges from `sources`; `visitor` sees the search too, and may stop it.
template<urd::adjacency_graph G, vertex_range<G> R, typename Visitor = null_visitor>
auto breadth_first_search(const G& g, R&& sources, Visitor&& visitor = {})
{
    using V = vertex_t<G>;
    using Result = Paths<V, std::optional<std::size_t>>;
    auto result = Result { vertex_map<std::optional<V>>(g), vertex_map<std::optional<std::size_t>>(g) };
    struct Record
    {
        Result* paths;
        void discover_vertex(V v) const
        {
            if (!paths->distances[v])
                paths->distances[v] = 0;  // a source
        }
        void tree_edge(const Edge<V>& e) const
        {
            paths->predecessors[e.target] = e.source;
            paths->distances[e.target] = *paths->distances[e.source] + 1;  // NOLINT(bugprone-unchecked-optional-access): its source was reached first
        }
    };
    breadth_first_visit(g, std::forward<R>(sources), visitors(Record { &result }, std::forward<Visitor>(visitor)));
    return result;
}

/// Each vertex's parent in the depth-first forest from `sources`; `visitor` sees the search too, and may stop it.
template<urd::adjacency_graph G, vertex_range<G> R, typename Visitor = null_visitor>
auto depth_first_search(const G& g, R&& sources, Visitor&& visitor = {})
{
    using V = vertex_t<G>;
    auto result = vertex_map<std::optional<V>>(g);
    struct Record
    {
        VertexMap<std::optional<V>, V>* parents;
        void tree_edge(const Edge<V>& e) const { (*parents)[e.target] = e.source; }
    };
    depth_first_visit(g, std::forward<R>(sources), visitors(Record { &result }, std::forward<Visitor>(visitor)));
    return result;
}

/// The vertices of the path to `target` that a search found (its predecessors), from a source to `target`; empty where
/// it did not reach `target`.
template<typename V, typename D>
auto path_to(const Paths<V, D>& paths, V target)
{
    auto result = urd::Store<urd::sequence<V>>();
    const auto& distance = paths.distances[target];
    if constexpr (requires { distance.has_value(); })
    {
        if (!distance)
            return result;
    }
    else if (distance == detail::unreached<D>)
        return result;
    for (auto v = std::optional(target); v; v = paths.predecessors[*v])
        result.push_back(*v);
    std::ranges::reverse(result);
    return result;
}

// Structure ------------------------------------------------------------------------------------------------------------

/// Thrown where a topological order is asked of a graph with a cycle.
struct not_a_dag : std::invalid_argument
{
    not_a_dag() : std::invalid_argument("vefr::topological_sort: the graph has a cycle") {}
};

/// The vertices in topological order: each before the vertices its edges lead to. Throws not_a_dag on a cycle.
template<urd::adjacency_graph G>
auto topological_sort(const G& g)
{
    using V = vertex_t<G>;
    struct Record
    {
        urd::Store<urd::sequence<V>> finished;
        void finish_vertex(V v) { finished.push_back(v); }
        void back_edge(const Edge<V>& /*edge*/) const { throw not_a_dag(); }
    } record;
    depth_first_visit(g, g.vertices(), record);
    std::ranges::reverse(record.finished);
    return std::move(record.finished);
}

/// A closed path of edges, each leaving where the one before enters; empty where the graph has no cycle.
template<urd::adjacency_graph G>
auto find_cycle(const G& g)
{
    using V = vertex_t<G>;
    struct Record
    {
        VertexMap<Edge<V>, V> entered_by;
        urd::Store<urd::sequence<Edge<V>>> cycle;
        void tree_edge(const Edge<V>& e) { entered_by[e.target] = e; }
        Flow back_edge(const Edge<V>& e)
        {
            cycle.push_back(e);
            for (auto v = e.source; v != e.target; v = entered_by[v].source)
                cycle.push_back(entered_by[v]);
            std::ranges::reverse(cycle);
            return Flow::stop;  // one cycle is enough
        }
    } record { vertex_map<Edge<V>>(g), {} };
    depth_first_visit(g, g.vertices(), record);
    return std::move(record.cycle);
}

/// A cycle as a closed path of vertices, the first again at the end; empty where the graph has no cycle.
template<urd::adjacency_graph G>
auto find_vertex_cycle(const G& g)
{
    auto result = urd::Store<urd::sequence<vertex_t<G>>>();
    const auto cycle = find_cycle(g);
    for (const auto& edge : cycle)
        result.push_back(edge.source);
    if (!cycle.empty())
        result.push_back(cycle.back().target);
    return result;
}

/// The strongly connected components: their number and each vertex's, numbered in the order Tarjan's algorithm closes
/// them (a component before those that reach it).
template<typename V>
struct Components
{
    std::size_t count {};
    VertexMap<std::size_t, V> component;
};

/// Tarjan's algorithm on depth_first_visit: O(n + m).
template<urd::adjacency_graph G>
auto strong_components(const G& g)
{
    using V = vertex_t<G>;
    constexpr auto none = std::numeric_limits<std::size_t>::max();
    struct Record
    {
        VertexMap<std::size_t, V> order;  ///< when each vertex was discovered
        VertexMap<std::size_t, V> low;    ///< the earliest vertex on the stack it reaches
        VertexMap<std::optional<V>, V> parent;
        urd::Store<urd::sequence<V>> stack;
        VertexMap<bool, V> on_stack;
        Components<V> result;
        std::size_t time = 0;

        void discover_vertex(V v)
        {
            order[v] = low[v] = time++;
            stack.push_back(v);
            on_stack[v] = true;
        }
        void tree_edge(const Edge<V>& e) { parent[e.target] = e.source; }
        void back_edge(const Edge<V>& e) { low[e.source] = std::min(low[e.source], order[e.target]); }
        void forward_or_cross_edge(const Edge<V>& e)
        {
            if (on_stack[e.target])
                low[e.source] = std::min(low[e.source], order[e.target]);
        }
        void finish_vertex(V v)
        {
            if (low[v] == order[v])  // the root of a component: what is above it on the stack
            {
                while (true)
                {
                    const auto w = stack.back();
                    stack.pop_back();
                    on_stack[w] = false;
                    result.component[w] = result.count;
                    if (w == v)
                        break;
                }
                ++result.count;
            }
            if (const auto above = parent[v])
                low[*above] = std::min(low[*above], low[v]);
        }
    } record { vertex_map(g, none), vertex_map(g, none), vertex_map<std::optional<V>>(g), {}, vertex_map(g, false), Components<V> { 0, vertex_map(g, none) } };
    depth_first_visit(g, g.vertices(), record);
    return std::move(record.result);
}

// Shortest paths -------------------------------------------------------------------------------------------------------

/// Dijkstra's algorithm from `sources` into `paths`, which it resets (sized by vertex_map, and kept by the caller across
/// searches); no weight is negative. O((n + m) log n). Events: discover_vertex (a distance first found), examine_vertex
/// (settled: its distance is final), examine_edge, edge_relaxed, edge_not_relaxed, finish_vertex.
template<urd::adjacency_graph G, vertex_range<G> R, edge_weight<G> F, typename Visitor = null_visitor>
void dijkstra_visit(const G& g,
                    R&& sources,
                    const F& weight,
                    Paths<vertex_t<G>, weight_t<G, F>>& paths,
                    Visitor&& visitor = {})  // NOLINT(cppcoreguidelines-missing-std-forward): the visitor is called, not passed on
{
    using V = vertex_t<G>;
    using D = weight_t<G, F>;
    std::ranges::fill(paths.predecessors, std::nullopt);
    std::ranges::fill(paths.distances, detail::unreached<D>);
    auto settled = vertex_map(g, false);
    auto queue = urd::Store<urd::queue<std::pair<D, V>>, urd::by_priority>();  // the least distance first
    for (V s : std::forward<R>(sources))
    {
        paths.distances[s] = D {};
        queue.push({ D {}, s });
        if (detail::discover_vertex(visitor, s))
            return;
    }
    while (!queue.empty())
    {
        const auto [distance, u] = queue.top();
        queue.pop();
        if (settled[u])  // an entry left behind by a shorter path, or a source given twice
            continue;
        settled[u] = true;
        if (detail::examine_vertex(visitor, u))
            return;
        for (const auto& e : out_edges_of(g, u))
        {
            if (detail::examine_edge(visitor, e))
                return;
            const auto through = distance + detail::weigh<G>(weight, e);
            if (through < paths.distances[e.target])
            {
                const auto first = paths.distances[e.target] == detail::unreached<D>;
                paths.distances[e.target] = through;
                paths.predecessors[e.target] = u;
                queue.push({ through, e.target });
                if (detail::edge_relaxed(visitor, e) || (first && detail::discover_vertex(visitor, e.target)))
                    return;
            }
            else if (detail::edge_not_relaxed(visitor, e))
                return;
        }
        if (detail::finish_vertex(visitor, u))
            return;
    }
}

/// Shortest paths from `sources`, an edge weighing weight(edge) or weight(its number); `visitor` sees the search too,
/// and may stop it: vefr::stop_at(targets) stops once the targets' distances are final.
template<urd::adjacency_graph G, vertex_range<G> R, edge_weight<G> F, typename Visitor = null_visitor>
auto dijkstra_shortest_paths(const G& g, R&& sources, const F& weight, Visitor&& visitor = {})
{
    using V = vertex_t<G>;
    using D = weight_t<G, F>;
    auto result = Paths<V, D> { vertex_map<std::optional<V>>(g), vertex_map(g, detail::unreached<D>) };
    dijkstra_visit(g, std::forward<R>(sources), weight, result, std::forward<Visitor>(visitor));
    return result;
}

/// The distance between every two vertices, result[u][v], an edge weighing weight(edge) or weight(its number).
/// O(n^3). Throws std::domain_error on a negative cycle.
template<urd::adjacency_graph G, edge_weight<G> F>
auto floyd_warshall_all_pairs_shortest_paths(const G& g, const F& weight)
{
    using D = weight_t<G, F>;
    constexpr auto infinity = detail::unreached<D>;
    auto result = vertex_map(g, vertex_map(g, infinity));
    for (auto u : g.vertices())
    {
        result[u][u] = D {};
        for (const auto& e : out_edges_of(g, u))
            result[u][e.target] = std::min(result[u][e.target], detail::weigh<G>(weight, e));
    }
    for (auto k : g.vertices())
        for (auto u : g.vertices())
        {
            if (result[u][k] == infinity)
                continue;
            for (auto v : g.vertices())
                if (result[k][v] != infinity && result[u][k] + result[k][v] < result[u][v])
                    result[u][v] = result[u][k] + result[k][v];
        }
    for (auto v : g.vertices())
        if (result[v][v] < D {})
            throw std::domain_error("vefr::floyd_warshall_all_pairs_shortest_paths: a negative cycle");
    return result;
}

#undef VEFR_EVENTS

}  // namespace vefr
