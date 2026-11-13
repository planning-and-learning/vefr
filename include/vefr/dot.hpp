#pragma once

// A graph in the DOT language (Graphviz): its vertices and edges, each labelled by a function the caller names.

#include "vefr/algorithms.hpp"

#include <fmt/format.h>
#include <string>
#include <string_view>

namespace vefr
{

/// Labels, as to_dot takes them: a function of the graph and a vertex, or of the graph and an edge (vefr::Edge), whose
/// result fmt formats.
/// No label.
inline constexpr auto unlabeled = [](const auto& /*graph*/, const auto& /*vertex_or_edge*/) { return std::string_view {}; };
/// A vertex's data (vertex_value).
inline constexpr auto vertex_data = [](const auto& graph, const auto& vertex) -> decltype(auto) { return graph.vertex_value(vertex); };
/// An edge's data (edge_value, by its number).
inline constexpr auto edge_data = [](const auto& graph, const auto& edge) -> decltype(auto) { return graph.edge_value(edge.id); };

/// `text` as a DOT string literal's contents: backslashes, quotes and newlines escaped, carriage returns dropped.
inline std::string dot_escape(std::string_view text)
{
    auto result = std::string {};
    result.reserve(text.size());
    for (const auto ch : text)
    {
        switch (ch)
        {
            case '\\':
                result += "\\\\";
                break;
            case '"':
                result += "\\\"";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                break;
            default:
                result += ch;
                break;
        }
    }
    return result;
}

/// `g` as a DOT digraph named `name`: vertex v as `v<index>` labelled by vertex_label(g, v), each edge (vefr::Edge)
/// by edge_label(g, edge), each as fmt formats it. An empty label is left out.
template<urd::adjacency_graph G, typename VL, typename EL>
[[nodiscard]] std::string to_dot(const G& g, const VL& vertex_label, const EL& edge_label, std::string_view name = "G")
{
    const auto attribute = [](const auto& label)
    {
        const auto text = fmt::format("{}", label);
        return text.empty() ? std::string {} : fmt::format(" [label=\"{}\"]", dot_escape(text));
    };
    auto text = fmt::format("digraph {} {{\n  rankdir=LR;\n", name);
    for (auto v : g.vertices())
        text += fmt::format("  v{}{};\n", index(v), attribute(vertex_label(g, v)));
    for (auto u : g.vertices())
        for (const auto& e : out_edges_of(g, u))
            text += fmt::format("  v{} -> v{}{};\n", index(e.source), index(e.target), attribute(edge_label(g, e)));
    text += "}\n";
    return text;
}

}  // namespace vefr
