#include "vefr/nauty.hpp"

#include <algorithm>
#include <cstddef>
#include <nausparse.h>
#include <numeric>
#include <span>

namespace vefr
{

CanonicalForm detail::canonical_form(bool digraph, const Numbers& offsets, const Numbers& targets, const Colors& colors)
{
    const auto n = colors.size();
    auto result = CanonicalForm {};
    result.digraph = digraph;
    result.offsets.assign(1, 0);
    result.labeling.resize(n);
    if (n == 0)
        return result;

    // The cells: vertices by color, a cell ending where ptn is 0.
    auto lab = urd::Store<urd::sequence<int>>(n);
    auto ptn = urd::Store<urd::sequence<int>>(n);
    auto orbits = urd::Store<urd::sequence<int>>(n);
    std::iota(lab.begin(), lab.end(), 0);
    std::ranges::stable_sort(lab, {}, [&](int v) { return colors[static_cast<std::size_t>(v)]; });
    for (std::size_t i = 0; i < n; ++i)
        ptn[i] = (i + 1 < n && colors[static_cast<std::size_t>(lab[i])] == colors[static_cast<std::size_t>(lab[i + 1])]) ? 1 : 0;

    auto v = urd::Store<urd::sequence<std::size_t>>(offsets.begin(), offsets.end() - 1);
    auto d = urd::Store<urd::sequence<int>>(n);
    for (std::size_t u = 0; u < n; ++u)
        d[u] = static_cast<int>(offsets[u + 1] - offsets[u]);
    auto e = urd::Store<urd::sequence<int>>(targets.begin(), targets.end());

    auto graph = sparsegraph {};
    SG_INIT(graph);
    graph.nv = static_cast<int>(n);
    graph.nde = targets.size();
    graph.v = std::ranges::data(v);
    graph.d = std::ranges::data(d);
    graph.e = std::ranges::data(e);
    graph.vlen = v.size();
    graph.dlen = d.size();
    graph.elen = e.size();

    auto canonical = sparsegraph {};
    SG_INIT(canonical);
    DEFAULTOPTIONS_SPARSEGRAPH(options);
    options.defaultptn = FALSE;
    options.getcanon = TRUE;
    options.digraph = digraph ? TRUE : FALSE;
    auto stats = statsblk {};
    sparsenauty(&graph, std::ranges::data(lab), std::ranges::data(ptn), std::ranges::data(orbits), &options, &stats, &canonical);
    sortlists_sg(&canonical);

    const auto canonical_offsets = std::span(canonical.v, n);
    const auto canonical_degrees = std::span(canonical.d, n);
    const auto canonical_edges = std::span(canonical.e, canonical.elen);
    for (std::size_t at = 0; at < n; ++at)
    {
        const auto row = canonical_edges.subspan(canonical_offsets[at], static_cast<std::size_t>(canonical_degrees[at]));
        for (auto target : row)
            result.targets.push_back(static_cast<std::uint32_t>(target));
        result.offsets.push_back(static_cast<std::uint32_t>(result.targets.size()));
        result.colors.push_back(colors[static_cast<std::size_t>(lab[at])]);
        result.labeling[static_cast<std::size_t>(lab[at])] = static_cast<std::uint32_t>(at);
    }
    SG_FREE(canonical);
    return result;
}

std::optional<Numbers> isomorphism(const CanonicalForm& a, const CanonicalForm& b)
{
    if (a != b)
        return std::nullopt;
    auto from_canonical = Numbers(b.labeling.size());
    for (std::size_t at = 0; at < b.labeling.size(); ++at)
        from_canonical[b.labeling[at]] = static_cast<std::uint32_t>(at);
    auto result = Numbers();
    result.reserve(a.labeling.size());
    for (auto canonical : a.labeling)
        result.push_back(from_canonical[canonical]);
    return result;
}

}  // namespace vefr
