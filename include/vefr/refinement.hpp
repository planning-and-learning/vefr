#pragma once

// Color refinement (1-WL) and the k-dimensional Weisfeiler-Leman algorithm: colors refined until they stop splitting.
// A certificate is what each round saw, the signatures and how often; two graphs whose certificates differ are not
// isomorphic. Colors are numbered by sorted signature in each round, so certificates do not depend on how the vertices
// are numbered.

#include "vefr/graphs.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <urd/elements/concepts.hpp>
#include <urd/elements/hash.hpp>
#include <urd/elements/less.hpp>
#include <urd/repr/values/indexed.hpp>
#include <utility>

namespace vefr
{

/// An item's color and the colors (or color tuples) around it, sorted.
template<typename T>
struct Signature : urd::compared_by_identity<Signature<T>>
{
    Color color {};
    urd::Store<urd::sequence<T>> neighbours;

    static constexpr auto identifying_members() noexcept { return urd::Members<&Signature::color, &Signature::neighbours> {}; }
};

/// Per round, each signature seen and how many items had it, in sorted order.
template<typename T>
struct Certificate : urd::compared_by_identity<Certificate<T>>
{
    using Round = urd::Store<urd::sequence<std::pair<Signature<T>, std::size_t>>>;
    urd::Store<urd::sequence<Round>> rounds;

    static constexpr auto identifying_members() noexcept { return urd::Members<&Certificate::rounds> {}; }
};

/// The certificate and the stable color of each item: a vertex by position (vefr::Positions), or a k-tuple of positions
/// read as a number in base n, its first position lowest.
template<typename T>
struct Refinement
{
    Certificate<T> certificate;
    Colors colors;
};

namespace detail
{

/// Refines `colors` until no color splits; `neighbours_of(item, colors, into)` appends the colors around an item.
template<typename T, typename F>
Refinement<T> refine(Colors colors, const F& neighbours_of)
{
    const auto n = colors.size();
    auto result = Refinement<T>();
    auto signatures = urd::Store<urd::sequence<std::pair<Signature<T>, std::size_t>>>();
    signatures.reserve(n);
    auto interner = urd::Store<urd::set<Signature<T>>, urd::positional>(urd::layer { 0 });
    // Colors come in as ranks (detail::ranks): their number is one past the largest.
    auto num_colors = colors.empty() ? std::size_t { 0 } : std::size_t { *std::ranges::max_element(colors) } + 1;
    while (true)
    {
        signatures.clear();
        for (std::size_t item = 0; item < n; ++item)
        {
            auto signature = Signature<T> {};
            signature.color = colors[item];
            neighbours_of(item, colors, signature.neighbours);
            std::ranges::sort(signature.neighbours);
            signatures.emplace_back(std::move(signature), item);
        }
        std::ranges::sort(signatures);

        auto& round = result.certificate.rounds.emplace_back();
        for (const auto& [signature, item] : signatures)
        {
            if (round.empty() || round.back().first != signature)
                round.emplace_back(signature, 0);
            ++round.back().second;
        }

        interner.clear();
        for (auto& [signature, item] : signatures)
            colors[item] = static_cast<Color>(interner.index(interner.insert(std::move(signature)).first).value);
        if (interner.size() == num_colors)
            break;
        num_colors = interner.size();
    }
    result.colors = std::move(colors);
    return result;
}

/// Each value replaced by its rank among the distinct values: colors that do not depend on the order they were met.
template<typename Key>
Colors ranks(const urd::Store<urd::sequence<Key>>& values)
{
    auto sorted = values;
    std::ranges::sort(sorted);
    const auto [first, last] = std::ranges::unique(sorted);
    sorted.erase(first, last);
    auto result = Colors();
    result.reserve(values.size());
    for (const auto& value : values)
        result.push_back(static_cast<Color>(std::ranges::lower_bound(sorted, value) - sorted.begin()));
    return result;
}

}  // namespace detail

/// Color refinement: a vertex's signature is its color and the multiset of its out-neighbours' colors.
template<urd::graph_like G, coloring<vertex_t<G>> F = decltype(uncolored)>
Refinement<Color> color_refinement(const G& g, const F& coloring = uncolored)
{
    const auto positions = Positions<G>(g);
    auto initial = Colors();
    initial.reserve(positions.size());
    for (std::size_t at = 0; at < positions.size(); ++at)
        initial.push_back(color_of(coloring, positions.at(at)));
    return detail::refine<Color>(detail::ranks(initial),
                                 [&](std::size_t at, const Colors& colors, auto& into)
                                 {
                                     for (auto v : g.neighbours(positions.at(at)))
                                         into.push_back(colors[positions.of(v)]);
                                 });
}

/// k-dimensional Weisfeiler-Leman over the k-tuples of vertices. A tuple's first color is the colors of its vertices
/// with which of them are equal and adjacent; its neighbours are, for each vertex w, the colors of the k tuples with w
/// in one place. Costs n^k tuples, each with n neighbours.
template<std::size_t K, urd::graph_like G, coloring<vertex_t<G>> F = decltype(uncolored)>
    requires(K >= 2)
Refinement<std::array<Color, K>> weisfeiler_leman(const G& g, const F& coloring = uncolored)
{
    const auto positions = Positions<G>(g);
    const auto n = positions.size();
    auto num_tuples = std::size_t { 1 };
    for (std::size_t i = 0; i < K; ++i)
    {
        if (n != 0 && num_tuples > std::numeric_limits<std::size_t>::max() / n)
            throw std::length_error("vefr::weisfeiler_leman: too many tuples");
        num_tuples *= n;
    }
    const auto tuple_of = [n](std::size_t number)
    {
        auto tuple = std::array<std::size_t, K>();
        for (auto& place : tuple)
        {
            place = number % n;
            number /= n;
        }
        return tuple;
    };
    const auto number_of = [n](const std::array<std::size_t, K>& tuple)
    {
        auto number = std::size_t { 0 };
        for (std::size_t i = K; i-- > 0;)
            number = number * n + tuple[i];
        return number;
    };

    // The vertices' colors, then for each pair of places 2 * equal + adjacent.
    auto initial = urd::Store<urd::sequence<urd::Store<urd::sequence<Color>>>>();
    initial.reserve(num_tuples);
    for (std::size_t number = 0; number < num_tuples; ++number)
    {
        const auto tuple = tuple_of(number);
        auto& key = initial.emplace_back();
        for (auto at : tuple)
            key.push_back(color_of(coloring, positions.at(at)));
        for (auto i : tuple)
            for (auto j : tuple)
                key.push_back(static_cast<Color>(2 * (i == j) + g.adjacent(positions.at(i), positions.at(j))));
    }

    return detail::refine<std::array<Color, K>>(detail::ranks(initial),
                                                [&](std::size_t number, const Colors& colors, auto& into)
                                                {
                                                    const auto tuple = tuple_of(number);
                                                    for (std::size_t w = 0; w < n; ++w)
                                                    {
                                                        auto around = std::array<Color, K>();
                                                        for (std::size_t place = 0; place < K; ++place)
                                                        {
                                                            auto replaced = tuple;
                                                            replaced[place] = w;
                                                            around[place] = colors[number_of(replaced)];
                                                        }
                                                        into.push_back(around);
                                                    }
                                                });
}

}  // namespace vefr
