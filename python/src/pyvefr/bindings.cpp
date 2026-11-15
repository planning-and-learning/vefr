#include "pyvefr/bindings.hpp"

#include "vefr/python/bindings.hpp"

#include <cstdint>
#include <urd/repr/graphs/adjacency_lists.hpp>
#include <urd/repr/graphs/csr_graph.hpp>
#include <utility>
#include <vector>

namespace nb = nanobind;
using namespace nb::literals;

namespace pyvefr
{
namespace
{

using vefr::python::Vertex;
/// A graph that grows and shrinks; edges numbered as added, in-edges kept.
using Graph = urd::Store<urd::graph<Vertex>, urd::positional, urd::bidirectional>;
/// A graph built once; edges keep their numbers, in-edges kept.
using StaticGraph = urd::Store<urd::graph<Vertex>, urd::frozen, urd::positional, urd::bidirectional>;
using Pairs = std::vector<std::pair<Vertex, Vertex>>;
// What each binds by its concepts (urd::python::bind_graph): a renamed member is a compile error, not a lost method.
static_assert(urd::bidirectional_graph<Graph> && urd::edge_list_graph<Graph> && urd::mutable_graph<Graph>);
static_assert(urd::bidirectional_graph<StaticGraph> && urd::edge_list_graph<StaticGraph>);

// The types the algorithms take and hand out, registered here once per process.

using vefr::python::Edge;
using vefr::python::Visitor;

/// Edge: an edge as the algorithms hand it out.
void bind_edge(nb::module_& m)
{
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
}

/// ControlFlow: what a visitor's event may answer.
void bind_control_flow(nb::module_& m)
{
    nb::enum_<vefr::ControlFlow>(m, "ControlFlow", "What a visitor's event may answer: go on, or stop the search.")
        .value("PROCEED", vefr::ControlFlow::proceed)
        .value("STOP", vefr::ControlFlow::stop);
}

/// Visitor: what Python subclasses, overriding the events it wants.
void bind_visitor(nb::module_& m)
{
    using namespace nb::literals;
    nb::class_<Visitor, vefr::python::detail::PyVisitor>(m, "Visitor", "Subclass it and override the events wanted; return ControlFlow.STOP to stop.")
        .def(nb::init<>())
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
}

/// CanonicalForm: compared and hashed, its labeling.
void bind_canonical_form_type(nb::module_& m)
{
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
}

}  // namespace

void bind(nb::module_& m)
{
    bind_edge(m);
    bind_control_flow(m);
    bind_visitor(m);
    vefr::python::bind_certificate<1>(m);
    vefr::python::bind_certificate<2>(m);
    vefr::python::bind_certificate<3>(m);
    bind_canonical_form_type(m);

    // pyvefr's own basic graphs: unlabelled (no vertex or edge data, so one color and no DOT labels), for using vefr from
    // Python without a library's graphs. A library binds its own graphs the same way.
    auto graph = nb::class_<Graph>(m, "Graph", "A graph that grows and shrinks; edges numbered as added, numbers never reused, in-edges kept.");
    graph.def(nb::init<>())
        .def(
            "__init__",
            [](Graph* self, const Pairs& edges) { new (self) Graph(edges.begin(), edges.end()); },
            "edges"_a)
        .def("add_vertex", [](Graph& g) { return g.add_vertex(); })
        .def(
            "add_edge",
            [](Graph& g, Vertex u, Vertex v) { return g.add_edge(u, v); },
            "source"_a,
            "target"_a)
        .def("remove_edge", &Graph::remove_edge, "edge"_a)
        .def("remove_vertex", &Graph::remove_vertex, "vertex"_a)
        .def("clear", &Graph::clear);
    urd::python::bind_graph(graph);
    vefr::python::bind_algorithms(graph, vefr::python::one_color, vefr::unlabeled, vefr::unlabeled);

    auto static_graph = nb::class_<StaticGraph>(m, "StaticGraph", "A graph built once; edges keep their numbers, in-edges kept.");
    static_graph.def(
                    "__init__",
                    [](StaticGraph* self, const Graph& g) { new (self) StaticGraph(urd::freeze<StaticGraph>(g)); },
                    "graph"_a)
        .def("__init__", [](StaticGraph* self, const Pairs& edges) { new (self) StaticGraph(edges.begin(), edges.end()); }, "edges"_a);
    urd::python::bind_graph(static_graph);
    vefr::python::bind_algorithms(static_graph, vefr::python::one_color, vefr::unlabeled, vefr::unlabeled);
}

}  // namespace pyvefr
