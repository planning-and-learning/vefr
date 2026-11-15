#pragma once

// Python bindings (nanobind) of vefr's algorithms for any graph class. A graph class answers the graph's members through
// urd (urd::python::bind_graph, urd/python/graphs.hpp) and vefr's algorithms as its methods, one binder each
// (bind_find_cycle, ...), or all it takes (bind_algorithms); so its generated stub lists them on the class. The types
// the algorithms share (Edge, Visitor, the certificates, ...) have binders of their own, which pyvefr calls once.
//
// A vertex is its index in Python (an int), whatever the graph's vertex type; an edge an algorithm hands out is a
// pyvefr.Edge.

#include "vefr/algorithms.hpp"
#include "vefr/dot.hpp"
#include "vefr/nauty.hpp"
#include "vefr/properties.hpp"
#include "vefr/refinement.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <functional>
#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <nanobind/trampoline.h>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <urd/elements/hash.hpp>
#include <urd/elements/requirements.hpp>
#include <urd/python/graphs.hpp>
#include <urd/python/sequences/sequence.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace vefr::python
{

namespace nb = nanobind;

/// A vertex as Python sees it: its index (Python's visitor and Edge are written for it).
using Vertex = std::uint32_t;
using Edge = vefr::Edge<Vertex>;
using Weights = std::vector<double>;
using Colors = std::optional<std::vector<vefr::Color>>;

/// A visitor Python subclasses: each event a method, called by vefr's searches; a method returns None (or
/// ControlFlow.PROCEED) to go on, or ControlFlow.STOP to stop the search.
class Visitor
{
public:
    Visitor() = default;
    Visitor(const Visitor&) = default;
    Visitor& operator=(const Visitor&) = default;
    Visitor(Visitor&&) noexcept = default;
    Visitor& operator=(Visitor&&) noexcept = default;
    virtual ~Visitor() = default;

    virtual std::optional<vefr::ControlFlow> start_vertex(Vertex /*vertex*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> discover_vertex(Vertex /*vertex*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> examine_vertex(Vertex /*vertex*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> finish_vertex(Vertex /*vertex*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> examine_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> tree_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> non_tree_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> gray_target(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> black_target(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> back_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> forward_or_cross_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> finish_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> edge_relaxed(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::ControlFlow> edge_not_relaxed(const Edge& /*edge*/) { return std::nullopt; }
};

namespace detail
{

/// The coloring a list gives, checked against the graph (a color per vertex index); without one, vertex_color's.
template<typename G, typename VC>
auto coloring(const G& g, const Colors& colors, const VC& vertex_color)
{
    if (colors && colors->size() < vefr::vertex_bound(g))
        throw std::invalid_argument("colors: one per vertex index needed");
    return [&g, &colors, &vertex_color](auto v)
    {
        if (colors)
            return (*colors)[vefr::index(v)];
        return vefr::color_of([&](auto u) -> decltype(auto) { return std::invoke(vertex_color, g, u); }, v);
    };
}

/// The weight of an edge by its number, checked against the graph: a weight per edge number.
template<typename G>
auto weighing(const G& g, const Weights& weights)
{
    for (const auto e : g.edges())
        if (e >= weights.size())
            throw std::invalid_argument("weights: one per edge number needed");
    return [&weights](std::size_t e) { return weights[e]; };
}

/// `search(visitor)` with the Python visitor given, or with none.
template<typename F>
decltype(auto) with_visitor(Visitor* visitor, const F& search)
{
    if (visitor)
        return search(*visitor);  // its events are the searches' events, by name
    return search(vefr::null_visitor {});
}

class PyVisitor : public Visitor
{
public:
    NB_TRAMPOLINE(Visitor);

    std::optional<vefr::ControlFlow> start_vertex(Vertex u) override { NB_OVERRIDE(start_vertex, u); }
    std::optional<vefr::ControlFlow> discover_vertex(Vertex u) override { NB_OVERRIDE(discover_vertex, u); }
    std::optional<vefr::ControlFlow> examine_vertex(Vertex u) override { NB_OVERRIDE(examine_vertex, u); }
    std::optional<vefr::ControlFlow> finish_vertex(Vertex u) override { NB_OVERRIDE(finish_vertex, u); }
    std::optional<vefr::ControlFlow> examine_edge(const Edge& e) override { NB_OVERRIDE(examine_edge, e); }
    std::optional<vefr::ControlFlow> tree_edge(const Edge& e) override { NB_OVERRIDE(tree_edge, e); }
    std::optional<vefr::ControlFlow> non_tree_edge(const Edge& e) override { NB_OVERRIDE(non_tree_edge, e); }
    std::optional<vefr::ControlFlow> gray_target(const Edge& e) override { NB_OVERRIDE(gray_target, e); }
    std::optional<vefr::ControlFlow> black_target(const Edge& e) override { NB_OVERRIDE(black_target, e); }
    std::optional<vefr::ControlFlow> back_edge(const Edge& e) override { NB_OVERRIDE(back_edge, e); }
    std::optional<vefr::ControlFlow> forward_or_cross_edge(const Edge& e) override { NB_OVERRIDE(forward_or_cross_edge, e); }
    std::optional<vefr::ControlFlow> finish_edge(const Edge& e) override { NB_OVERRIDE(finish_edge, e); }
    std::optional<vefr::ControlFlow> edge_relaxed(const Edge& e) override { NB_OVERRIDE(edge_relaxed, e); }
    std::optional<vefr::ControlFlow> edge_not_relaxed(const Edge& e) override { NB_OVERRIDE(edge_not_relaxed, e); }
};

}  // namespace detail

// The types the algorithms take and hand out are registered once per process, by pyvefr's module; a module binding a
// graph imports pyvefr first. A certificate of a k pyvefr does not register, its library registers with
// bind_certificate.

/// The certificate of the refinement over k-tuples: k = 1 is color refinement's, k >= 2 Weisfeiler-Leman's.
template<std::size_t K>
using certificate_t = vefr::Certificate<std::conditional_t<K == 1, vefr::Color, std::array<vefr::Color, K>>>;

/// Certificate<K> ("Certificate1", "Certificate2", ...): compared and hashed, num_rounds.
template<std::size_t K>
void bind_certificate(nb::module_& m)
{
    static_assert(K >= 1, "vefr::python::bind_certificate: k >= 1");
    using Certificate = certificate_t<K>;
    static const auto name = "Certificate" + std::to_string(K);  // the type keeps its name: static, as a literal is
    nb::class_<Certificate>(m, name.c_str())
        .def(
            "__eq__",
            [](const Certificate& lhs, const Certificate& rhs) { return lhs == rhs; },
            nb::is_operator())
        .def("__hash__", [](const Certificate& self) { return urd::Hash<Certificate> {}(self); })
        .def_prop_ro("num_rounds", [](const Certificate& self) { return self.rounds.size(); });
}

/// A graph the bindings take: an urd graph whose vertices are the Vertex Python's visitor and Edge are written for.
template<typename G>
concept python_graph = urd::graph_like<G> && std::same_as<vertex_t<G>, Vertex>;

/// What colors a vertex where Python gives no colors: a function of the graph and a vertex, as to_dot's labels are,
/// whose result is a color (an integer, an enum or an urd index; vefr::color_of). vefr::vertex_data colors by the
/// vertex's data where that is one. It answers by what a vertex is, not by its number, so that isomorphic graphs color
/// alike.
template<typename VC, typename G>
concept vertex_coloring =
    std::regular_invocable<const VC&, const G&, vertex_t<G>> && urd::indexable<std::remove_cvref_t<std::invoke_result_t<const VC&, const G&, vertex_t<G>>>>;

/// Every vertex the same color: a graph without colors of its own.
inline constexpr auto one_color = [](const auto& /*graph*/, const auto& /*vertex*/) { return Color { 0 }; };

// Each binder adds one algorithm as a method of the graph's class; urd binds its graph members
// (urd::python::bind_graph). Values pass through nanobind's casters (urd's for sequences and views).

/// __str__: the graph's DOT text, labelled by vertex_label and edge_label (vefr::to_dot).
template<python_graph G, typename VL, typename EL>
void bind_dot(nb::class_<G>& cls, VL vertex_label, EL edge_label)
{
    cls.def("__str__", [vertex_label, edge_label](const G& g) { return vefr::to_dot(g, vertex_label, edge_label); });
}

template<python_graph G>
void bind_is_loopless(nb::class_<G>& cls)
{
    cls.def("is_loopless", [](const G& g) { return vefr::is_loopless(g); });
}

template<python_graph G>
void bind_is_multigraph(nb::class_<G>& cls)
{
    cls.def("is_multigraph", [](const G& g) { return vefr::is_multigraph(g); });
}

template<python_graph G>
void bind_is_symmetric(nb::class_<G>& cls)
{
    cls.def("is_symmetric", [](const G& g) { return vefr::is_symmetric(g); });
}

template<python_graph G>
void bind_is_simple_undirected(nb::class_<G>& cls)
{
    cls.def("is_simple_undirected", [](const G& g) { return vefr::is_simple_undirected(g); });
}

template<python_graph G, vertex_coloring<G> VC>
void bind_color_refinement(nb::class_<G>& cls, VC vertex_color)
{
    using namespace nb::literals;
    cls.def(
        "color_refinement",
        [vertex_color](const G& g, const Colors& colors)
        {
            auto result = vefr::color_refinement(g, detail::coloring(g, colors, vertex_color));
            return std::pair(std::move(result.certificate), result.colors);
        },
        "colors"_a = nb::none(),
        "The certificate and each vertex's stable color, by position; colors default to the graph's own.");
}

/// weisfeiler_leman(k) for each k of K... (each >= 2), its certificate the Certificate<k> that bind_certificate binds.
template<std::size_t... K, python_graph G, vertex_coloring<G> VC>
void bind_weisfeiler_leman(nb::class_<G>& cls, VC vertex_color)
{
    static_assert(sizeof...(K) > 0 && ((K >= 2) && ...), "vefr::python::bind_weisfeiler_leman: each k >= 2");
    using namespace nb::literals;
    using Result = std::pair<std::variant<certificate_t<K>...>, vefr::Colors>;
    cls.def(
        "weisfeiler_leman",
        [vertex_color](const G& g, std::size_t k, const Colors& colors)
        {
            const auto coloring = detail::coloring(g, colors, vertex_color);
            const auto refine = [&]<std::size_t L>()
            {
                auto result = vefr::weisfeiler_leman<L>(g, coloring);
                return Result(std::move(result.certificate), std::move(result.colors));
            };
            auto result = std::optional<Result>();
            static_cast<void>(((k == K && (result.emplace(refine.template operator()<K>()), true)) || ...));
            if (!result)
                throw std::invalid_argument("k: not bound for this graph");
            return std::move(*result);
        },
        "k"_a,
        "colors"_a = nb::none(),
        "The certificate and each k-tuple's stable color; colors default to the graph's own.");
}

template<python_graph G, vertex_coloring<G> VC>
void bind_canonical_form(nb::class_<G>& cls, VC vertex_color)
{
    using namespace nb::literals;
    cls.def(
        "canonical_form",
        [vertex_color](const G& g, const Colors& colors) { return vefr::canonical_form(g, detail::coloring(g, colors, vertex_color)); },
        "colors"_a = nb::none(),
        "Colors default to the graph's own.");
}

template<python_graph G>
void bind_strong_components(nb::class_<G>& cls)
{
    cls.def("strong_components",
            [](const G& g)
            {
                auto result = vefr::strong_components(g);
                return std::pair(result.count, result.component);
            });
}

template<python_graph G>
void bind_breadth_first_search(nb::class_<G>& cls)
{
    using namespace nb::literals;
    cls.def(
        "breadth_first_search",
        [](const G& g, const std::vector<Vertex>& sources, Visitor* visitor)
        {
            auto result = detail::with_visitor(visitor, [&](auto&& each) { return vefr::breadth_first_search(g, sources, each); });
            return std::pair(result.predecessors, result.distances);
        },
        "sources"_a,
        "visitor"_a = nb::none());
}

template<python_graph G>
void bind_depth_first_search(nb::class_<G>& cls)
{
    using namespace nb::literals;
    cls.def(
        "depth_first_search",
        [](const G& g, const std::vector<Vertex>& sources, Visitor* visitor)
        { return detail::with_visitor(visitor, [&](auto&& each) { return vefr::depth_first_search(g, sources, each); }); },
        "sources"_a,
        "visitor"_a = nb::none());
}

template<python_graph G>
void bind_breadth_first_visit(nb::class_<G>& cls)
{
    using namespace nb::literals;
    cls.def(
        "breadth_first_visit",
        [](const G& g, const std::vector<Vertex>& sources, Visitor& visitor) { vefr::breadth_first_visit(g, sources, visitor); },
        "sources"_a,
        "visitor"_a);
}

template<python_graph G>
void bind_depth_first_visit(nb::class_<G>& cls)
{
    using namespace nb::literals;
    cls.def(
        "depth_first_visit",
        [](const G& g, const std::vector<Vertex>& sources, Visitor& visitor) { vefr::depth_first_visit(g, sources, visitor); },
        "sources"_a,
        "visitor"_a);
}

template<python_graph G>
void bind_topological_sort(nb::class_<G>& cls)
{
    cls.def("topological_sort", [](const G& g) { return vefr::topological_sort(g); });
}

template<python_graph G>
void bind_find_cycle(nb::class_<G>& cls)
{
    cls.def("find_cycle", [](const G& g) { return vefr::find_cycle(g); });
}

/// Python's weights are a list by edge number, so the shortest paths need a graph that numbers its edges.
template<typename G>
concept python_weighted_graph = python_graph<G> && urd::incidence_graph<G> && urd::edge_list_graph<G>;

template<python_weighted_graph G>
void bind_dijkstra_shortest_paths(nb::class_<G>& cls)
{
    using namespace nb::literals;
    cls.def(
        "dijkstra_shortest_paths",
        [](const G& g, const std::vector<Vertex>& sources, const Weights& weights, Visitor* visitor, const std::optional<std::vector<Vertex>>& targets)
        {
            const auto weight = detail::weighing(g, weights);
            const auto search = [&](auto&& each)
            {
                if (targets)
                    return vefr::dijkstra_shortest_paths(g, sources, weight, vefr::visitors(each, vefr::stop_at(*targets)));
                return vefr::dijkstra_shortest_paths(g, sources, weight, each);
            };
            auto result = detail::with_visitor(visitor, search);
            return std::pair(result.predecessors, result.distances);
        },
        "sources"_a,
        "weights"_a,
        "visitor"_a = nb::none(),
        "targets"_a = nb::none(),
        "With targets, it stops once their distances are final.");
}

template<python_weighted_graph G>
void bind_floyd_warshall_all_pairs_shortest_paths(nb::class_<G>& cls)
{
    using namespace nb::literals;
    cls.def(
        "floyd_warshall_all_pairs_shortest_paths",
        [](const G& g, const Weights& weights) { return vefr::floyd_warshall_all_pairs_shortest_paths(g, detail::weighing(g, weights)); },
        "weights"_a);
}

/// Every binder above that G takes (Weisfeiler-Leman for k 2 and 3), its vertices colored by vertex_color, and its DOT
/// text labelled by vertex_label and edge_label.
template<python_graph G, vertex_coloring<G> VC, typename VL, typename EL>
void bind_algorithms(nb::class_<G>& cls, const VC& vertex_color, VL vertex_label, EL edge_label)
{
    bind_dot(cls, std::move(vertex_label), std::move(edge_label));
    bind_is_loopless(cls);
    bind_is_multigraph(cls);
    bind_is_symmetric(cls);
    bind_is_simple_undirected(cls);
    bind_color_refinement(cls, vertex_color);
    bind_weisfeiler_leman<2, 3>(cls, vertex_color);
    bind_canonical_form(cls, vertex_color);
    bind_strong_components(cls);
    bind_breadth_first_search(cls);
    bind_depth_first_search(cls);
    bind_breadth_first_visit(cls);
    bind_depth_first_visit(cls);
    bind_topological_sort(cls);
    bind_find_cycle(cls);
    if constexpr (python_weighted_graph<G>)
    {
        bind_dijkstra_shortest_paths(cls);
        bind_floyd_warshall_all_pairs_shortest_paths(cls);
    }
}

}  // namespace vefr::python
