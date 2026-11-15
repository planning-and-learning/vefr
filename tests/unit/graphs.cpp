#include "vefr/algorithms.hpp"
#include "vefr/dot.hpp"
#include "vefr/nauty.hpp"
#include "vefr/properties.hpp"
#include "vefr/refinement.hpp"
#include "vefr/reversed.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <urd/repr/graphs/adjacency_lists.hpp>
#include <urd/repr/graphs/bit_rows.hpp>
#include <urd/repr/graphs/csr_graph.hpp>
#include <utility>
#include <vector>

namespace
{

using V = std::uint32_t;
using Dynamic = urd::Store<urd::graph<V>, urd::positional>;
using Static = urd::Store<urd::graph<V>, urd::frozen, urd::positional>;
using Pairs = std::vector<std::pair<V, V>>;

template<typename G = Static>
G make(const Pairs& edges)
{ return G(edges.begin(), edges.end()); }

/// Both ways.
Pairs undirected(const Pairs& edges)
{
    auto result = Pairs();
    for (auto [u, v] : edges)
    {
        result.emplace_back(u, v);
        result.emplace_back(v, u);
    }
    return result;
}

Pairs relabel(const Pairs& edges, const std::vector<V>& to)
{
    auto result = Pairs();
    for (auto [u, v] : edges)
        result.emplace_back(to[u], to[v]);
    return result;
}

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers): the graphs are their numbers
Pairs hexagon_edges() { return undirected({ { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 }, { 4, 5 }, { 5, 0 } }); }
Pairs two_triangles_edges() { return undirected({ { 0, 1 }, { 1, 2 }, { 2, 0 }, { 3, 4 }, { 4, 5 }, { 5, 3 } }); }
std::vector<V> shuffle() { return { 3, 5, 0, 4, 1, 2 }; }
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

constexpr auto last_of_hexagon = V { 5 };  // NOLINT(cppcoreguidelines-avoid-magic-numbers)

}  // namespace

TEST(Properties, ReadFromNeighbours)
{
    EXPECT_TRUE(vefr::is_simple_undirected(make(hexagon_edges())));
    EXPECT_FALSE(vefr::is_symmetric(make({ { 0, 1 } })));
    EXPECT_FALSE(vefr::is_loopless(make({ { 0, 0 } })));
    EXPECT_TRUE(vefr::is_multigraph(make<Dynamic>({ { 0, 1 }, { 0, 1 } })));
}

TEST(ColorRefinement, DoesNotDependOnNumbering)
{
    const auto lhs = vefr::color_refinement(make(hexagon_edges()));
    const auto rhs = vefr::color_refinement(make(relabel(hexagon_edges(), shuffle())));
    EXPECT_EQ(lhs.certificate, rhs.certificate);
    // Regular graphs of one degree look alike to color refinement.
    EXPECT_EQ(lhs.certificate, vefr::color_refinement(make(two_triangles_edges())).certificate);
}

TEST(ColorRefinement, SplitsByColorsAndDegrees)
{
    const auto path = make(undirected({ { 0, 1 }, { 1, 2 } }));
    const auto result = vefr::color_refinement(path);
    EXPECT_EQ(result.colors[0], result.colors[2]);
    EXPECT_NE(result.colors[0], result.colors[1]);
    const auto colored = vefr::color_refinement(path, [](V v) { return v == 0 ? 1U : 0U; });
    EXPECT_NE(colored.colors[0], colored.colors[2]);
    EXPECT_NE(result.certificate, colored.certificate);
}

TEST(WeisfeilerLeman, ThreeTellsHexagonFromTwoTriangles)
{
    const auto hexagon_certificate = vefr::weisfeiler_leman<3>(make(hexagon_edges())).certificate;
    EXPECT_EQ(hexagon_certificate, vefr::weisfeiler_leman<3>(make(relabel(hexagon_edges(), shuffle()))).certificate);
    EXPECT_NE(hexagon_certificate, vefr::weisfeiler_leman<3>(make(two_triangles_edges())).certificate);
    EXPECT_EQ(vefr::weisfeiler_leman<2>(make(hexagon_edges())).certificate, vefr::weisfeiler_leman<2>(make(relabel(hexagon_edges(), shuffle()))).certificate);
}

TEST(Nauty, CanonicalFormsAreEqualExactlyForIsomorphicGraphs)
{
    const auto lhs = vefr::canonical_form(make(hexagon_edges()));
    const auto rhs = vefr::canonical_form(make(relabel(hexagon_edges(), shuffle())));
    EXPECT_EQ(lhs, rhs);
    EXPECT_NE(lhs, vefr::canonical_form(make(two_triangles_edges())));

    // The isomorphism maps every edge to an edge.
    const auto found = vefr::isomorphism(lhs, rhs);
    ASSERT_TRUE(found.has_value());
    const auto& map = found.value();  // NOLINT(bugprone-unchecked-optional-access): asserted above
    const auto image = make(relabel(hexagon_edges(), shuffle()));
    for (auto [u, v] : hexagon_edges())
        EXPECT_TRUE(image.adjacent(map[u], map[v]));
    EXPECT_FALSE(vefr::isomorphism(lhs, vefr::canonical_form(make(two_triangles_edges()))).has_value());
}

TEST(Nauty, KeepsColorsAndDirections)
{
    const auto one = [](V v) { return v == 0 ? 1U : 0U; };
    const auto last = [](V v) { return v == last_of_hexagon ? 1U : 0U; };
    const auto two = [](V v) { return v == 0 || v == 1 ? 1U : 0U; };
    EXPECT_EQ(vefr::canonical_form(make(hexagon_edges()), one), vefr::canonical_form(make(hexagon_edges()), last));
    EXPECT_NE(vefr::canonical_form(make(hexagon_edges()), one), vefr::canonical_form(make(hexagon_edges()), two));

    const auto forward = vefr::canonical_form(make({ { 0, 1 }, { 1, 2 } }));
    EXPECT_TRUE(forward.digraph);
    EXPECT_EQ(forward, vefr::canonical_form(make({ { 2, 1 }, { 1, 0 } })));
    EXPECT_NE(forward, vefr::canonical_form(make({ { 0, 1 }, { 2, 1 } })));
    EXPECT_THROW(static_cast<void>(vefr::canonical_form(make<Dynamic>({ { 0, 1 }, { 0, 1 } }))), std::invalid_argument);
}

TEST(Algorithms, SearchesAndPaths)
{
    const auto g = make({ { 0, 1 }, { 1, 2 }, { 0, 3 }, { 3, 2 } });
    const auto sources = std::vector<V> { 0 };

    const auto bfs = vefr::breadth_first_search(g, sources);
    EXPECT_EQ(bfs.distances[2], 2U);
    EXPECT_EQ(bfs.predecessors[0], std::nullopt);

    const auto order = vefr::topological_sort(g);
    EXPECT_EQ(order.front(), 0U);
    EXPECT_EQ(order.back(), 2U);
    EXPECT_EQ(vefr::strong_components(g).count, 4U);
    EXPECT_TRUE(vefr::find_cycle(g).empty());

    // Edges keep the numbers they were given: 0->1, 1->2, 0->3, 3->2.
    const auto weights = std::array<double, 4> { 1.0, 1.0, 5.0, 1.0 };
    const auto weight = [&](std::size_t e) { return weights.at(e); };
    const auto paths = vefr::dijkstra_shortest_paths(g, sources, weight);
    EXPECT_EQ(paths.distances[3], weights[2]);
    EXPECT_EQ(paths.distances[2], 2.0);
    EXPECT_EQ(paths.predecessors[2], 1U);
    EXPECT_EQ(paths.predecessors[0], std::nullopt);
    const auto all = vefr::floyd_warshall_all_pairs_shortest_paths(g, weight);
    EXPECT_EQ(all[0][2], 2.0);
    EXPECT_TRUE(std::isinf(all[2][0]));
}

TEST(Algorithms, DijkstraStopsAtItsTargets)
{
    // A path 0 -> 1 -> 2 -> 3 of unit weights, and a long way round 0 -> 3.
    const auto g = make({ { 0, 1 }, { 1, 2 }, { 2, 3 }, { 0, 3 } });
    constexpr auto detour = 10.0;
    const auto unit = [](const vefr::Edge<V>& e) { return e.source + 1 == e.target ? 1.0 : detour; };  // a weight by edge
    struct
    {
        std::vector<V> settled;
        void examine_vertex(V v) { settled.push_back(v); }
    } record;
    const auto paths = vefr::dijkstra_shortest_paths(g, std::vector<V> { 0 }, unit, vefr::visitors(record, vefr::stop_at(std::vector<V> { 2 })));
    EXPECT_EQ(paths.distances[2], 2.0);
    EXPECT_EQ(vefr::path_to(paths, V { 2 }), (urd::Store<urd::sequence<V>> { 0, 1, 2 }));
    EXPECT_EQ(record.settled, (std::vector<V> { 0, 1, 2 }));  // 3 was never settled
    EXPECT_EQ(paths.distances[3], detour);                    // only a bound
}

TEST(Algorithms, DijkstraReusesItsPaths)
{
    const auto g = make({ { 0, 1 }, { 1, 2 } });
    const auto weight = [](std::size_t) { return 1; };
    auto paths = vefr::Paths<V, int> { vefr::vertex_map<std::optional<V>>(g), vefr::vertex_map<int>(g) };
    vefr::dijkstra_visit(g, std::vector<V> { 0 }, weight, paths);
    EXPECT_EQ(paths.distances[2], 2);
    vefr::dijkstra_visit(g, std::vector<V> { 1 }, weight, paths);
    EXPECT_EQ(paths.distances[2], 1);
    EXPECT_EQ(paths.predecessors[1], std::nullopt);
    EXPECT_EQ(paths.distances[0], std::numeric_limits<int>::max());  // reset: not reached from 1
}

TEST(Algorithms, VisitorsStopSearches)
{
    const auto g = make({ { 0, 1 }, { 1, 2 }, { 2, 3 } });
    struct
    {
        [[nodiscard]] vefr::ControlFlow discover_vertex(V v) const { return v == 2 ? vefr::ControlFlow::stop : vefr::ControlFlow::proceed; }
    } until_two;
    const auto bfs = vefr::breadth_first_search(g, std::vector<V> { 0 }, until_two);
    EXPECT_EQ(bfs.distances[2], 2U);
    EXPECT_EQ(bfs.distances[3], std::nullopt);
    const auto dfs = vefr::depth_first_search(g, std::vector<V> { 0 }, until_two);
    EXPECT_EQ(dfs[3], std::nullopt);
}

TEST(Algorithms, SourcesAreAnyRange)
{
    const auto g = make({ { 0, 1 }, { 2, 3 } });
    const auto bfs = vefr::breadth_first_search(g, std::views::iota(V { 0 }, V { 4 }) | std::views::filter([](V v) { return v % 2 == 0; }));
    EXPECT_EQ(bfs.distances[1], 1U);
    EXPECT_EQ(bfs.distances[3], 1U);
}

TEST(Algorithms, VisitorsHaveTheEventsTheyWant)
{
    const auto g = make({ { 0, 1 }, { 1, 2 }, { 0, 2 } });
    struct
    {
        std::vector<V> discovered;
        void discover_vertex(V v) { discovered.push_back(v); }
    } breadth, depth;
    vefr::breadth_first_visit(g, std::vector<V> { 0 }, breadth);
    EXPECT_EQ(breadth.discovered, (std::vector<V> { 0, 1, 2 }));
    vefr::depth_first_visit(g, std::vector<V> { 0 }, depth);
    EXPECT_EQ(depth.discovered, (std::vector<V> { 0, 1, 2 }));
}

TEST(Algorithms, TopologicalSortRefusesACycle) { EXPECT_THROW(static_cast<void>(vefr::topological_sort(make({ { 0, 1 }, { 1, 0 } }))), vefr::not_a_dag); }

TEST(Algorithms, StrongComponentsAgreeWithReachability)
{
    // Two cycles, {0, 1, 2} and {3, 4}, the first leading to the second, and 5 alone.
    const auto g = make({ { 0, 1 }, { 1, 2 }, { 2, 0 }, { 2, 3 }, { 3, 4 }, { 4, 3 }, { 4, 5 } });
    const auto result = vefr::strong_components(g);
    EXPECT_EQ(result.count, 3U);
    EXPECT_EQ(result.component[0], result.component[1]);
    EXPECT_EQ(result.component[0], result.component[2]);
    EXPECT_EQ(result.component[3], result.component[4]);
    EXPECT_NE(result.component[0], result.component[3]);
    EXPECT_NE(result.component[3], result.component[5]);
    // Closed in reverse topological order: a component before the components that reach it.
    EXPECT_LT(result.component[5], result.component[3]);
    EXPECT_LT(result.component[3], result.component[0]);
}

TEST(Algorithms, GraphsWithoutEdgeNumbers)
{
    using Dense = urd::Store<urd::graph<V>, urd::frozen, urd::constant_adjacency>;
    const auto edges = Pairs { { 0, 1 }, { 1, 2 }, { 2, 0 } };
    const auto g = urd::freeze<Dense>(make<Dynamic>(edges));
    EXPECT_EQ(vefr::find_cycle(g).size(), 3U);
    EXPECT_EQ(vefr::strong_components(g).count, 1U);
}

TEST(Algorithms, CyclesAndRemovedVertices)
{
    auto g = make<Dynamic>({ { 0, 1 }, { 1, 2 }, { 2, 0 }, { 2, 3 } });
    const auto cycle = vefr::find_cycle(g);
    ASSERT_EQ(cycle.size(), 3U);
    const auto vertices = vefr::find_vertex_cycle(g);
    ASSERT_EQ(vertices.size(), 4U);
    EXPECT_EQ(vertices.front(), vertices.back());
    for (std::size_t at = 0; at < cycle.size(); ++at)
        EXPECT_EQ(cycle[at].target, cycle[(at + 1) % cycle.size()].source);
    EXPECT_EQ(vefr::strong_components(g).count, 2U);

    g.remove_vertex(1);
    const auto bfs = vefr::breadth_first_search(g, std::vector<V> { 2 });
    EXPECT_EQ(bfs.distances[0], 1U);
    EXPECT_EQ(bfs.distances[1], std::nullopt);
    EXPECT_TRUE(vefr::find_cycle(g).empty());
}

TEST(Reversed, SearchesAgainstTheEdges)
{
    using Both = urd::Store<urd::graph<V>, urd::frozen, urd::positional, urd::bidirectional>;
    const auto edges = Pairs { { 0, 1 }, { 1, 2 }, { 3, 2 } };
    const auto g = Both(edges.begin(), edges.end());
    const auto back = vefr::reversed(g);
    static_assert(urd::bidirectional_graph<decltype(back)> && urd::edge_list_graph<decltype(back)>);
    const auto bfs = vefr::breadth_first_search(back, std::vector<V> { 2 });  // distances to 2
    EXPECT_EQ(bfs.distances[0], 2U);
    EXPECT_EQ(bfs.distances[3], 1U);
    for (const auto e : back.out_edges(2))
    {
        EXPECT_EQ(back.source(e), 2U);
        EXPECT_EQ(g.target(e), 2U);  // the same number as the edge it turns around
    }
}

TEST(Dot, LabelsByData)
{
    using Labeled = urd::Store<urd::graph<V, int, char>, urd::frozen>;
    const auto edges = std::vector<std::tuple<V, V, int>> { { 0, 1, 7 } };
    const auto names = std::vector<char> { 'a', 'b' };
    const auto g = Labeled(urd::graph_data<std::tuple<V, V, int>, char> { 2, edges, {}, names });
    EXPECT_EQ(vefr::to_dot(g, vefr::vertex_data, vefr::edge_data),
              "digraph G {\n  rankdir=LR;\n  v0 [label=\"a\"];\n  v1 [label=\"b\"];\n  v0 -> v1 [label=\"7\"];\n}\n");
}

TEST(Dot, LabelsAndEscapes)
{
    const auto g = make({ { 0, 1 } });
    const auto text = vefr::to_dot(
        g,
        [](const auto&, V v) { return v == 0 ? std::string("say \"hi\"") : std::string(); },
        [](const auto&, const vefr::Edge<V>& e) { return e.id; });
    EXPECT_EQ(text, "digraph G {\n  rankdir=LR;\n  v0 [label=\"say \\\"hi\\\"\"];\n  v1;\n  v0 -> v1 [label=\"0\"];\n}\n");
}
