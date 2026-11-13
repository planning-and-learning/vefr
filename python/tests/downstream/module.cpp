// A downstream module as a library built on vefr writes one: its own graph class, bound with one call, after which
// pyvefr's functions take it.

#include <cstdint>
#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <string>
#include <urd/python/graphs.hpp>
#include <urd/repr/graphs/csr_graph.hpp>
#include <utility>
#include <variant>
#include <vector>
#include <vefr/python/bindings.hpp>

namespace
{

/// A graph with a name per vertex: its vertex data, as downstream libraries keep labels.
using NamedGraph = urd::Store<urd::graph<std::uint32_t, std::monostate, std::string>, urd::frozen>;
using Edge = std::pair<std::uint32_t, std::uint32_t>;

}  // namespace

NB_MODULE(_pyvefr_downstream_test, m)
{
    namespace nb = nanobind;
    using namespace nb::literals;
    auto cls = nb::class_<NamedGraph>(std::move(m), "NamedGraph");
    cls.def(
        "__init__",
        [](NamedGraph* self, const std::vector<Edge>& edges, const std::vector<std::string>& names)
        { new (self) NamedGraph(urd::graph_data<Edge, std::string> { names.size(), edges, {}, names }); },
        "edges"_a,
        "names"_a);
    urd::python::bind_graph(cls);
    vefr::python::bind_algorithms(cls, vefr::vertex_data, vefr::unlabeled);
}
