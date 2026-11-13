#pragma once

// Canonical forms by nauty: a graph relabelled canonically, so that two graphs are isomorphic, colors kept, exactly
// when their canonical forms are equal.

#include "vefr/graphs.hpp"
#include "vefr/properties.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <urd/elements/concepts.hpp>
#include <urd/elements/less.hpp>

namespace vefr
{

using Numbers = urd::Store<urd::sequence<std::uint32_t>>;

/// A graph relabelled canonically: its edges as rows of sorted targets by canonical position, and the color at each.
/// `labeling` maps a vertex's position (vefr::Positions) to its canonical position; it is not part of the form.
struct CanonicalForm : urd::compared_by_identity<CanonicalForm>
{
    bool digraph {};
    Numbers offsets;
    Numbers targets;
    Colors colors;
    Numbers labeling;

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&CanonicalForm::digraph, &CanonicalForm::offsets, &CanonicalForm::targets, &CanonicalForm::colors> {}; }
};

namespace detail
{
/// nauty on rows of targets by position: `digraph` where the graph is directed or has loops.
CanonicalForm canonical_form(bool digraph, const Numbers& offsets, const Numbers& targets, const Colors& colors);
}

/// The canonical form of `g` with its vertices colored. Directed graphs and loops are fine; parallel edges are not.
/// nauty keeps global state: one call at a time.
template<urd::adjacency_graph G, coloring<vertex_t<G>> F = decltype(uncolored)>
CanonicalForm canonical_form(const G& g, const F& coloring = uncolored)
{
    if (is_multigraph(g))
        throw std::invalid_argument("vefr::canonical_form: parallel edges");
    const auto positions = Positions<G>(g);
    auto offsets = Numbers(1, 0);
    auto targets = Numbers();
    auto colors = Colors();
    for (std::size_t at = 0; at < positions.size(); ++at)
    {
        const auto u = positions.at(at);
        for (auto v : g.neighbours(u))
            targets.push_back(static_cast<std::uint32_t>(positions.of(v)));
        offsets.push_back(static_cast<std::uint32_t>(targets.size()));
        colors.push_back(color_of(coloring, u));
    }
    return detail::canonical_form(!is_symmetric(g) || !is_loopless(g), offsets, targets, colors);
}

/// An isomorphism from `a`'s graph to `b`'s, by vertex position, if they are isomorphic.
std::optional<Numbers> isomorphism(const CanonicalForm& a, const CanonicalForm& b);

}  // namespace vefr
