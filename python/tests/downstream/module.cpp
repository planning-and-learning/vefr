// A downstream module as a library built on vefr writes one: its own graph class, vefr's algorithms as its methods.

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
    nb::module_::import_("pyvefr");  // the types every graph shares: Edge, Visitor, the certificates
    auto cls = nb::class_<NamedGraph>(std::move(m), "NamedGraph");
    cls.def(
        "__init__",
        [](NamedGraph* self, const std::vector<Edge>& edges, const std::vector<std::string>& names)
        { new (self) NamedGraph(urd::graph_data<Edge, std::string> { names.size(), edges, {}, names }); },
        "edges"_a,
        "names"_a);
    urd::python::bind_graph(cls);
    // Colored by what a vertex holds, not by its number: here its name's length.
    const auto name_length = [](const NamedGraph& g, std::uint32_t v) { return static_cast<vefr::Color>(g.vertex_value(v).size()); };
    vefr::python::bind_algorithms(cls, name_length, vefr::vertex_data, vefr::unlabeled);
}
