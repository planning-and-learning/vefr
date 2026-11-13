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
/// A graph that grows and shrinks; edges numbered as added.
using Graph = urd::Store<urd::graph<Vertex>, urd::positional>;
/// A graph built once; edges keep their numbers, in-edges kept.
using StaticGraph = urd::Store<urd::graph<Vertex>, urd::frozen, urd::positional, urd::bidirectional>;
using Pairs = std::vector<std::pair<Vertex, Vertex>>;
// What each binds by its concepts (urd::python::bind_graph): a renamed member is a compile error, not a lost method.
static_assert(urd::incidence_graph<Graph> && urd::edge_list_graph<Graph> && urd::mutable_graph<Graph>);
static_assert(urd::bidirectional_graph<StaticGraph> && urd::edge_list_graph<StaticGraph>);

}  // namespace

void bind(nb::module_& m)
{
    vefr::python::bind_shared(m);

    auto graph = nb::class_<Graph>(m, "Graph", "A graph that grows and shrinks; edges numbered as added, numbers never reused.");
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
    vefr::python::bind_algorithms(m, graph, vefr::unlabeled, vefr::unlabeled);

    auto static_graph = nb::class_<StaticGraph>(m, "StaticGraph", "A graph built once; edges keep their numbers, in-edges kept.");
    static_graph.def(
                    "__init__",
                    [](StaticGraph* self, const Graph& g) { new (self) StaticGraph(urd::freeze<StaticGraph>(g)); },
                    "graph"_a)
        .def("__init__", [](StaticGraph* self, const Pairs& edges) { new (self) StaticGraph(edges.begin(), edges.end()); }, "edges"_a);
    urd::python::bind_graph(static_graph);
    vefr::python::bind_algorithms(m, static_graph, vefr::unlabeled, vefr::unlabeled);
}

}  // namespace pyvefr
