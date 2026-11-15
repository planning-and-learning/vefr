#pragma once

// Python bindings (nanobind) of vefr's algorithms for any graph class. pyvefr registers the types every graph shares
// once (bind_shared); a graph class answers the graph's members through urd (urd::python::bind_graph,
// urd/python/graphs.hpp) and takes vefr's algorithm functions (pyvefr.breadth_first_search, pyvefr.canonical_form,
// ...) through bind_algorithms, as pyvefr's own graphs do.
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
#include <cstdint>
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
#include <urd/python/graphs.hpp>
#include <urd/python/sequences/sequence.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace vefr::python
{

/// An algorithm's graph argument: G's object, which the stubs name pyvefr.GraphLike, the Protocol every graph urd binds
/// satisfies. pyvefr's stubs list overloads only for its own graphs; so a downstream graph type-checks too.
template<typename G>
struct GraphLike
{
    const G* graph = nullptr;

    const G& operator*() const { return *graph; }
};

}  // namespace vefr::python

namespace nanobind::detail
{

template<typename G>
struct type_caster<vefr::python::GraphLike<G>>
{
    NB_TYPE_CASTER(vefr::python::GraphLike<G>, const_name("pyvefr.GraphLike"))

    bool from_python(handle src, uint32_t flags, cleanup_list* cleanup) noexcept
    {
        make_caster<G> caster;
        if (!caster.from_python(src, flags, cleanup))
            return false;
        value.graph = static_cast<G*>(caster);  // None casts to nullptr: no graph
        return value.graph != nullptr;
    }
};

}  // namespace nanobind::detail

namespace vefr::python
{

namespace nb = nanobind;

/// A vertex as Python sees it: its index (Python's visitor and Edge are written for it).
using Vertex = std::uint32_t;
using Edge = vefr::Edge<Vertex>;
using Weights = std::vector<double>;
using Colors = std::optional<std::vector<vefr::Color>>;

/// A visitor Python subclasses: each event a method, called by vefr's searches; a method returns None (or
/// Flow.PROCEED) to go on, or Flow.STOP to stop the search.
class Visitor
{
public:
    Visitor() = default;
    Visitor(const Visitor&) = default;
    Visitor& operator=(const Visitor&) = default;
    Visitor(Visitor&&) noexcept = default;
    Visitor& operator=(Visitor&&) noexcept = default;
    virtual ~Visitor() = default;

    virtual std::optional<vefr::Flow> start_vertex(Vertex /*vertex*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> discover_vertex(Vertex /*vertex*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> examine_vertex(Vertex /*vertex*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> finish_vertex(Vertex /*vertex*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> examine_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> tree_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> non_tree_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> gray_target(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> black_target(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> back_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> forward_or_cross_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> finish_edge(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> edge_relaxed(const Edge& /*edge*/) { return std::nullopt; }
    virtual std::optional<vefr::Flow> edge_not_relaxed(const Edge& /*edge*/) { return std::nullopt; }
};

namespace detail
{

/// The coloring a list gives, checked against the graph: a color per vertex index.
template<typename G>
auto coloring(const G& g, const Colors& colors)
{
    if (colors && colors->size() < vefr::vertex_bound(g))
        throw std::invalid_argument("colors: one per vertex index needed");
    return [&colors](auto v) { return colors ? (*colors)[vefr::index(v)] : vefr::Color { 0 }; };
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

    std::optional<vefr::Flow> start_vertex(Vertex u) override { NB_OVERRIDE(start_vertex, u); }
    std::optional<vefr::Flow> discover_vertex(Vertex u) override { NB_OVERRIDE(discover_vertex, u); }
    std::optional<vefr::Flow> examine_vertex(Vertex u) override { NB_OVERRIDE(examine_vertex, u); }
    std::optional<vefr::Flow> finish_vertex(Vertex u) override { NB_OVERRIDE(finish_vertex, u); }
    std::optional<vefr::Flow> examine_edge(const Edge& e) override { NB_OVERRIDE(examine_edge, e); }
    std::optional<vefr::Flow> tree_edge(const Edge& e) override { NB_OVERRIDE(tree_edge, e); }
    std::optional<vefr::Flow> non_tree_edge(const Edge& e) override { NB_OVERRIDE(non_tree_edge, e); }
    std::optional<vefr::Flow> gray_target(const Edge& e) override { NB_OVERRIDE(gray_target, e); }
    std::optional<vefr::Flow> black_target(const Edge& e) override { NB_OVERRIDE(black_target, e); }
    std::optional<vefr::Flow> back_edge(const Edge& e) override { NB_OVERRIDE(back_edge, e); }
    std::optional<vefr::Flow> forward_or_cross_edge(const Edge& e) override { NB_OVERRIDE(forward_or_cross_edge, e); }
    std::optional<vefr::Flow> finish_edge(const Edge& e) override { NB_OVERRIDE(finish_edge, e); }
    std::optional<vefr::Flow> edge_relaxed(const Edge& e) override { NB_OVERRIDE(edge_relaxed, e); }
    std::optional<vefr::Flow> edge_not_relaxed(const Edge& e) override { NB_OVERRIDE(edge_not_relaxed, e); }
};

template<typename T>
void bind_certificate(nb::module_& m, const char* name)
{
    using Certificate = vefr::Certificate<T>;
    nb::class_<Certificate>(m, name)
        .def(
            "__eq__",
            [](const Certificate& lhs, const Certificate& rhs) { return lhs == rhs; },
            nb::is_operator())
        .def("__hash__", [](const Certificate& self) { return urd::Hash<Certificate> {}(self); })
        .def_prop_ro("num_rounds", [](const Certificate& self) { return self.rounds.size(); });
}

}  // namespace detail

/// The types every bound graph shares, registered once: Edge, Flow, Visitor, the certificates, CanonicalForm and
/// isomorphism. pyvefr calls it; a downstream module imports pyvefr instead.
inline void bind_shared(nb::module_& m)
{
    using namespace nb::literals;
    nb::class_<Edge>(m, "Edge")
        .def_ro("source", &Edge::source)
        .def_ro("target", &Edge::target)
        .def_ro("id", &Edge::id)
        .def(
            "__eq__",
            [](const Edge& lhs, const Edge& rhs) { return lhs == rhs; },
            nb::is_operator())
        .def("__repr__",
             [](const Edge& e) { return "Edge(" + std::to_string(e.source) + ", " + std::to_string(e.target) + ", id=" + std::to_string(e.id) + ")"; });

    nb::enum_<vefr::Flow>(m, "Flow", "What a visitor's event may answer: go on, or stop the search.")
        .value("PROCEED", vefr::Flow::proceed)
        .value("STOP", vefr::Flow::stop);

    auto visitor = nb::class_<Visitor, detail::PyVisitor>(m, "Visitor", "Subclass it and override the events wanted; return Flow.STOP to stop.");
    visitor.def(nb::init<>())
        .def("start_vertex", &Visitor::start_vertex, "vertex"_a)
        .def("discover_vertex", &Visitor::discover_vertex, "vertex"_a)
        .def("examine_vertex", &Visitor::examine_vertex, "vertex"_a)
        .def("finish_vertex", &Visitor::finish_vertex, "vertex"_a)
        .def("examine_edge", &Visitor::examine_edge, "edge"_a)
        .def("tree_edge", &Visitor::tree_edge, "edge"_a)
        .def("non_tree_edge", &Visitor::non_tree_edge, "edge"_a)
        .def("gray_target", &Visitor::gray_target, "edge"_a)
        .def("black_target", &Visitor::black_target, "edge"_a)
        .def("back_edge", &Visitor::back_edge, "edge"_a)
        .def("forward_or_cross_edge", &Visitor::forward_or_cross_edge, "edge"_a)
        .def("finish_edge", &Visitor::finish_edge, "edge"_a)
        .def("edge_relaxed", &Visitor::edge_relaxed, "edge"_a)
        .def("edge_not_relaxed", &Visitor::edge_not_relaxed, "edge"_a);

    detail::bind_certificate<vefr::Color>(m, "Certificate");
    detail::bind_certificate<std::array<vefr::Color, 2>>(m, "Certificate2");
    detail::bind_certificate<std::array<vefr::Color, 3>>(m, "Certificate3");

    nb::class_<vefr::CanonicalForm>(m, "CanonicalForm", "A graph relabelled canonically: equal exactly for isomorphic graphs.")
        .def(
            "__eq__",
            [](const vefr::CanonicalForm& lhs, const vefr::CanonicalForm& rhs) { return lhs == rhs; },
            nb::is_operator())
        .def("__hash__", [](const vefr::CanonicalForm& self) { return urd::Hash<vefr::CanonicalForm> {}(self); })
        .def_prop_ro(
            "labeling",
            [](const vefr::CanonicalForm& self) { return std::vector<std::uint32_t>(self.labeling.begin(), self.labeling.end()); },
            "Each vertex's canonical position, by position.");
    m.def(
        "isomorphism",
        [](const vefr::CanonicalForm& lhs, const vefr::CanonicalForm& rhs) -> std::optional<std::vector<std::uint32_t>>
        {
            const auto found = vefr::isomorphism(lhs, rhs);
            if (!found)
                return std::nullopt;
            return std::vector<std::uint32_t>(found->begin(), found->end());
        },
        "lhs"_a,
        "rhs"_a,
        "A map from lhs's vertex positions to rhs's, if isomorphic.");
}

/// Binds vefr's algorithms for graph G: an overload for G of each algorithm function of `algorithms` (pyvefr's module),
/// and its class's DOT text (__str__) labelled by vertex_label and edge_label (vefr::to_dot). Its graph members come
/// from urd (urd::python::bind_graph). Values pass through nanobind's casters (urd's for sequences and views).
template<urd::graph_like G, typename VL, typename EL>
void bind_algorithms(nb::module_& algorithms, nb::class_<G>& cls, VL vertex_label, EL edge_label)
{
    using namespace nb::literals;
    using V = vertex_t<G>;
    using Vertices = std::vector<V>;
    static_assert(std::same_as<V, Vertex>, "vefr::python: Python's visitor and Edge take a Vertex (std::uint32_t)");

    cls.def("__str__", [vertex_label, edge_label](const G& g) { return vefr::to_dot(g, vertex_label, edge_label); });
    auto& m = algorithms;
    m.def("is_loopless", [](GraphLike<G> g) { return vefr::is_loopless(*g); }, "graph"_a);
    m.def("is_multigraph", [](GraphLike<G> g) { return vefr::is_multigraph(*g); }, "graph"_a);
    m.def("is_symmetric", [](GraphLike<G> g) { return vefr::is_symmetric(*g); }, "graph"_a);
    m.def("is_simple_undirected", [](GraphLike<G> g) { return vefr::is_simple_undirected(*g); }, "graph"_a);

    m.def(
        "color_refinement",
        [](GraphLike<G> g, const Colors& colors)
        {
            auto result = vefr::color_refinement(*g, detail::coloring(*g, colors));
            return std::pair(std::move(result.certificate), result.colors);
        },
        "graph"_a,
        "colors"_a = nb::none(),
        "The certificate and each vertex's stable color, by position.");
    m.def(
        "weisfeiler_leman",
        [](GraphLike<G> g, std::size_t k, const Colors& colors)
        {
            // One result type for both k, so that the stub names it: the certificate is Certificate2 or Certificate3.
            using Result = std::pair<std::variant<vefr::Certificate<std::array<vefr::Color, 2>>, vefr::Certificate<std::array<vefr::Color, 3>>>,
                                     decltype(vefr::weisfeiler_leman<2>(*g, detail::coloring(*g, colors)).colors)>;
            if (k == 2)
            {
                auto result = vefr::weisfeiler_leman<2>(*g, detail::coloring(*g, colors));
                return Result(std::move(result.certificate), std::move(result.colors));
            }
            if (k == 3)
            {
                auto result = vefr::weisfeiler_leman<3>(*g, detail::coloring(*g, colors));
                return Result(std::move(result.certificate), std::move(result.colors));
            }
            throw std::invalid_argument("k: 2 or 3");
        },
        "graph"_a,
        "k"_a,
        "colors"_a = nb::none(),
        "The certificate and each k-tuple's stable color.");
    m.def(
        "canonical_form",
        [](GraphLike<G> g, const Colors& colors) { return vefr::canonical_form(*g, detail::coloring(*g, colors)); },
        "graph"_a,
        "colors"_a = nb::none());

    m.def(
        "strong_components",
        [](GraphLike<G> g)
        {
            auto result = vefr::strong_components(*g);
            return std::pair(result.count, result.component);
        },
        "graph"_a);
    m.def(
        "breadth_first_search",
        [](GraphLike<G> g, const Vertices& sources, Visitor* visitor)
        {
            auto result = detail::with_visitor(visitor, [&](auto&& each) { return vefr::breadth_first_search(*g, sources, each); });
            return std::pair(result.predecessors, result.distances);
        },
        "graph"_a,
        "sources"_a,
        "visitor"_a = nb::none());
    m.def(
        "depth_first_search",
        [](GraphLike<G> g, const Vertices& sources, Visitor* visitor)
        { return detail::with_visitor(visitor, [&](auto&& each) { return vefr::depth_first_search(*g, sources, each); }); },
        "graph"_a,
        "sources"_a,
        "visitor"_a = nb::none());
    m.def(
        "breadth_first_visit",
        [](GraphLike<G> g, const Vertices& sources, Visitor& visitor) { vefr::breadth_first_visit(*g, sources, visitor); },
        "graph"_a,
        "sources"_a,
        "visitor"_a);
    m.def(
        "depth_first_visit",
        [](GraphLike<G> g, const Vertices& sources, Visitor& visitor) { vefr::depth_first_visit(*g, sources, visitor); },
        "graph"_a,
        "sources"_a,
        "visitor"_a);
    m.def("topological_sort", [](GraphLike<G> g) { return vefr::topological_sort(*g); }, "graph"_a);
    m.def("find_cycle", [](GraphLike<G> g) { return vefr::find_cycle(*g); }, "graph"_a);

    if constexpr (urd::incidence_graph<G> && urd::edge_list_graph<G>)
    {
        m.def(
            "dijkstra_shortest_paths",
            [](GraphLike<G> g, const Vertices& sources, const Weights& weights, Visitor* visitor, const std::optional<Vertices>& targets)
            {
                const auto weight = detail::weighing(*g, weights);
                const auto search = [&](auto&& each)
                {
                    if (targets)
                        return vefr::dijkstra_shortest_paths(*g, sources, weight, vefr::visitors(each, vefr::stop_at(*targets)));
                    return vefr::dijkstra_shortest_paths(*g, sources, weight, each);
                };
                auto result = detail::with_visitor(visitor, search);
                return std::pair(result.predecessors, result.distances);
            },
            "graph"_a,
            "sources"_a,
            "weights"_a,
            "visitor"_a = nb::none(),
            "targets"_a = nb::none(),
            "With targets, it stops once their distances are final.");
        m.def(
            "floyd_warshall_all_pairs_shortest_paths",
            [](GraphLike<G> g, const Weights& weights) { return vefr::floyd_warshall_all_pairs_shortest_paths(*g, detail::weighing(*g, weights)); },
            "graph"_a,
            "weights"_a);
    }
}

/// The algorithm functions bind_algorithms adds overloads to.
inline constexpr auto algorithm_names = std::array {
    "is_loopless",
    "is_multigraph",
    "is_symmetric",
    "is_simple_undirected",
    "color_refinement",
    "weisfeiler_leman",
    "canonical_form",
    "strong_components",
    "breadth_first_search",
    "depth_first_search",
    "breadth_first_visit",
    "depth_first_visit",
    "topological_sort",
    "find_cycle",
    "dijkstra_shortest_paths",
    "floyd_warshall_all_pairs_shortest_paths",
};

/// Binds vefr's algorithms for a downstream graph class: as above, added to pyvefr's. nanobind adds an overload only in the
/// module that defined the function (pyvefr._pyvefr), so the package's names, which re-export them, are renewed after.
template<urd::graph_like G, typename VL, typename EL>
void bind_algorithms(nb::class_<G>& cls, VL vertex_label, EL edge_label)
{
    auto native = nb::module_::import_("pyvefr._pyvefr");
    bind_algorithms(native, cls, std::move(vertex_label), std::move(edge_label));
    auto package = nb::module_::import_("pyvefr");
    for (const auto* name : algorithm_names)
        if (nb::hasattr(native, name))
            nb::setattr(package, name, native.attr(name));
}

}  // namespace vefr::python
