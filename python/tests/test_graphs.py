# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import math

import pytest
from typing_extensions import override

import pyvefr

HEXAGON = [(i, (i + 1) % 6) for i in range(6)]
TWO_TRIANGLES = [(0, 1), (1, 2), (2, 0), (3, 4), (4, 5), (5, 3)]
SHUFFLE = [3, 5, 0, 4, 1, 2]


def undirected(edges: list[tuple[int, int]]) -> list[tuple[int, int]]:
    return [*edges, *((v, u) for u, v in edges)]


def relabel(edges: list[tuple[int, int]], to: list[int]) -> list[tuple[int, int]]:
    return [(to[u], to[v]) for u, v in edges]


def test_graphs_grow_shrink_and_freeze() -> None:
    g = pyvefr.Graph()
    a, b, c = g.add_vertex(), g.add_vertex(), g.add_vertex()
    e = g.add_edge(a, b)
    g.add_edge(b, c)
    assert g.num_edges == 2
    assert [g.target(edge) for edge in g.out_edges(a)] == [b]
    assert g.edges() == [0, 1]
    # Without data, a graph's data is the one value of a unit: None.
    assert g.vertex_value(a) is None
    assert g.edge_value(e) is None
    g.remove_edge(e)
    assert not g.adjacent(a, b)
    frozen = pyvefr.StaticGraph(g)
    assert frozen.in_neighbours(c) == [b]
    assert pyvefr.is_loopless(frozen)


def test_certificates_and_canonical_forms() -> None:
    hexagon = pyvefr.StaticGraph(pyvefr.Graph(undirected(HEXAGON)))
    shuffled = pyvefr.Graph(undirected(relabel(HEXAGON, SHUFFLE)))
    triangles = pyvefr.Graph(undirected(TWO_TRIANGLES))

    assert pyvefr.color_refinement(hexagon)[0] == pyvefr.color_refinement(shuffled)[0]
    assert pyvefr.color_refinement(hexagon)[0] == pyvefr.color_refinement(triangles)[0]
    assert pyvefr.weisfeiler_leman(hexagon, 3)[0] != pyvefr.weisfeiler_leman(triangles, 3)[0]
    assert len({pyvefr.weisfeiler_leman(hexagon, 3)[0], pyvefr.weisfeiler_leman(shuffled, 3)[0]}) == 1

    lhs, rhs = pyvefr.canonical_form(hexagon), pyvefr.canonical_form(shuffled)
    assert lhs == rhs
    assert hash(lhs) == hash(rhs)
    mapping = pyvefr.isomorphism(lhs, rhs)
    assert mapping is not None
    assert all(shuffled.adjacent(mapping[u], mapping[v]) for u, v in HEXAGON)
    assert pyvefr.isomorphism(lhs, pyvefr.canonical_form(triangles)) is None
    assert pyvefr.canonical_form(hexagon, [1, 0, 0, 0, 0, 0]) != pyvefr.canonical_form(hexagon)

    with pytest.raises(ValueError, match="colors"):
        pyvefr.canonical_form(hexagon, [0])
    with pytest.raises(ValueError, match="k"):
        pyvefr.weisfeiler_leman(hexagon, 4)


def test_searches_and_paths() -> None:
    g = pyvefr.StaticGraph(pyvefr.Graph([(0, 1), (1, 2), (0, 3), (3, 2)]))
    predecessors, distances = pyvefr.breadth_first_search(g, [0])
    assert predecessors[0] is None
    assert distances[2] == 2
    assert pyvefr.topological_sort(g)[0] == 0
    assert pyvefr.strong_components(g)[0] == 4
    assert pyvefr.find_cycle(g) == []

    # Edges keep the numbers they were given: 0->1, 1->2, 0->3, 3->2.
    weights = [1.0, 1.0, 5.0, 1.0]
    predecessors, distances = pyvefr.dijkstra_shortest_paths(g, [0], weights)
    assert distances[2] == 2.0
    assert predecessors[2] == 1
    matrix = pyvefr.floyd_warshall_all_pairs_shortest_paths(g, weights)
    assert matrix[0][2] == 2.0
    assert math.isinf(matrix[2][0])
    with pytest.raises(ValueError, match="weights"):
        pyvefr.dijkstra_shortest_paths(g, [0], [1.0])


def test_visitors_and_cycles() -> None:
    g = pyvefr.Graph([(0, 1), (1, 2), (2, 0)])

    class Record(pyvefr.Visitor):
        def __init__(self) -> None:
            super().__init__()
            self.discovered: list[int] = []

        @override
        def discover_vertex(self, vertex: int) -> pyvefr.Flow:
            self.discovered.append(vertex)
            return pyvefr.Flow.PROCEED

    record = Record()
    pyvefr.depth_first_visit(g, [0], record)
    assert record.discovered == [0, 1, 2]

    cycle = pyvefr.find_cycle(g)
    assert len(cycle) == 3
    assert all(cycle[i].target == cycle[(i + 1) % 3].source for i in range(3))

    g.remove_vertex(1)
    assert pyvefr.find_cycle(g) == []
    assert pyvefr.breadth_first_search(g, [2])[1][1] is None


def test_visitors_stop_searches_and_dijkstra_stops_at_targets() -> None:
    g = pyvefr.StaticGraph(pyvefr.Graph([(0, 1), (1, 2), (2, 3), (0, 3)]))

    class UntilTwo(pyvefr.Visitor):
        # None reads as Flow.PROCEED, which the stub's Flow does not say.
        @override
        def discover_vertex(self, vertex: int) -> pyvefr.Flow | None:  # pyright: ignore[reportIncompatibleMethodOverride]
            return pyvefr.Flow.STOP if vertex == 2 else None

    _, distances = pyvefr.breadth_first_search(g, [0], visitor=UntilTwo())
    assert distances[2] == 2
    assert distances[3] == 1  # found from 0 before 2 was

    class Settled(pyvefr.Visitor):
        def __init__(self) -> None:
            super().__init__()
            self.order: list[int] = []

        @override
        def examine_vertex(self, vertex: int) -> pyvefr.Flow:
            self.order.append(vertex)
            return pyvefr.Flow.PROCEED

    # Edges keep the numbers they were given: 0->1, 1->2, 2->3, 0->3.
    settled = Settled()
    _, distances = pyvefr.dijkstra_shortest_paths(g, [0], [1.0, 1.0, 1.0, 10.0], visitor=settled, targets=[2])
    assert distances[2] == 2.0
    assert settled.order == [0, 1, 2]
