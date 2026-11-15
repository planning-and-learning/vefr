# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""A graph built once (pyvefr.StaticGraph): structure, traversal, shortest paths, refinement and nauty.

Example usage:

    python3 python/examples/static_graph.py
"""

from typing_extensions import override

import pyvefr

# Each graph as its edges, (source, target), numbered in this order: 0, 1, ...
# 0 -> 1 -> 2 -> 0 and 3 -> 4 -> 3, joined by 2 -> 3.
TWO_CYCLES = [(0, 1), (1, 2), (2, 0), (2, 3), (3, 4), (4, 3)]
# A weight per edge number, for the shortest paths.
WEIGHTS = [1.0, 2.0, 1.0, 0.5, 1.0, 1.0]
# Undirected (each edge in both directions, by both_ways): a hexagon, two triangles, and the hexagon numbered otherwise.
HEXAGON = [(0, 1), (1, 2), (2, 3), (3, 4), (4, 5), (5, 0)]
TWO_TRIANGLES = [(0, 1), (1, 2), (2, 0), (3, 4), (4, 5), (5, 3)]
HEXAGON_RENUMBERED = [(0, 2), (2, 4), (4, 1), (1, 3), (3, 5), (5, 0)]


def both_ways(edges: list[tuple[int, int]]) -> list[tuple[int, int]]:
    """Each edge in both directions: the undirected graph."""
    return [*edges, *((target, source) for source, target in edges)]


class Order(pyvefr.Visitor):
    """Records the order in which a search discovers vertices; stops at a target, if given."""

    def __init__(self, target: int | None = None) -> None:
        """Start with no vertex discovered."""
        super().__init__()
        self.target = target
        self.order: list[int] = []

    @override
    def discover_vertex(self, vertex: int) -> pyvefr.ControlFlow | None:
        """Record the vertex; stop the search at the target."""
        self.order.append(vertex)
        return pyvefr.ControlFlow.STOP if vertex == self.target else None


def structure() -> None:
    """Its construction, from edges or from a complete growing graph; its properties, components and cycles."""
    s = pyvefr.StaticGraph(TWO_CYCLES)
    print("two cycles:", s, sep="\n")  # DOT
    print("in-neighbours of 3:", s.in_neighbours(3), "in-edges:", s.in_edges(3))
    assert s.in_neighbours(3) == [2, 4]
    assert s.in_edges(3) == [3, 5]
    frozen = pyvefr.StaticGraph(pyvefr.Graph(TWO_CYCLES))
    assert frozen.edges() == s.edges()
    assert [(frozen.source(e), frozen.target(e)) for e in frozen.edges()] == TWO_CYCLES

    assert s.is_loopless()
    assert not s.is_multigraph()
    assert not s.is_symmetric()
    assert not s.is_simple_undirected()
    count, component = s.strong_components()
    print("strong components:", count, component)
    assert count == 2
    assert component[0] == component[1] == component[2] != component[3] == component[4]
    cycle = s.find_cycle()
    print("a cycle:", cycle)
    assert [(edge.source, edge.target) for edge in cycle] == [(0, 1), (1, 2), (2, 0)]


def traversal() -> None:
    """Breadth- and depth-first search: their results, and a visitor that sees their events."""
    g = pyvefr.StaticGraph(TWO_CYCLES)
    predecessors, distances = g.breadth_first_search([0])
    print("breadth-first from 0: predecessors", predecessors, "distances", distances)
    assert predecessors == [None, 0, 1, 2, 3]
    assert distances == [0, 1, 2, 3, 4]
    assert g.depth_first_search([0]) == [None, 0, 1, 2, 3]

    # A visitor sees the search's events; ControlFlow.STOP ends it.
    order = Order(target=3)
    g.breadth_first_visit([0], order)
    print("breadth-first order until 3:", order.order)
    assert order.order == [0, 1, 2, 3]  # 4 is never discovered
    order = Order()
    g.depth_first_visit([0], order)
    print("depth-first order:", order.order)
    assert order.order == [0, 1, 2, 3, 4]


def shortest_paths() -> None:
    """Shortest paths: from a source, until a target's distance is final, and between all pairs."""
    s = pyvefr.StaticGraph(TWO_CYCLES)
    predecessors, distances = s.dijkstra_shortest_paths([0], WEIGHTS)
    print("from 0: predecessors", predecessors, "distances", distances)
    assert predecessors == [None, 0, 1, 2, 3]
    assert distances == [0.0, 1.0, 3.0, 3.5, 4.5]

    _, distances = s.dijkstra_shortest_paths([0], WEIGHTS, targets=[1])
    print("from 0 until 1 is final:", distances)
    assert distances[1] == 1.0

    all_pairs = s.floyd_warshall_all_pairs_shortest_paths(WEIGHTS)
    print("all pairs:", all_pairs)
    assert all_pairs[0] == [0.0, 1.0, 3.0, 3.5, 4.5]
    assert all_pairs[1][0] == 3.0  # round the cycle: 1 -> 2 -> 0
    assert all_pairs[3][0] == float("inf")  # no way back from the second cycle


def refinement_and_nauty() -> None:
    """Color refinement, Weisfeiler-Leman and nauty on the classic pair: a hexagon and two triangles.

    Both are 2-regular on six vertices, so color refinement colors every vertex alike in both and cannot tell them
    apart; Weisfeiler-Leman over pairs and triples can, and nauty's canonical forms are equal exactly for isomorphic
    graphs.
    """
    hexagon = pyvefr.StaticGraph(both_ways(HEXAGON))
    triangles = pyvefr.StaticGraph(both_ways(TWO_TRIANGLES))
    print("hexagon:", hexagon, sep="\n")  # DOT; an undirected edge as an edge each way
    print("two triangles:", triangles, sep="\n")

    # Color refinement: every vertex has two neighbours of the same color, so one color stays in both.
    hexagon_certificate, hexagon_colors = hexagon.color_refinement()
    triangles_certificate, triangles_colors = triangles.color_refinement()
    print("color refinement colors:", hexagon_colors, triangles_colors)
    assert hexagon_colors == triangles_colors == [0] * 6
    assert hexagon_certificate == triangles_certificate

    # Weisfeiler-Leman over pairs and triples sees the triangles.
    for k in (2, 3):
        assert hexagon.weisfeiler_leman(k)[0] != triangles.weisfeiler_leman(k)[0]
    print("2-WL and 3-WL tell them apart")

    # nauty: canonical forms are equal exactly for isomorphic graphs.
    assert hexagon.canonical_form() != triangles.canonical_form()
    print("canonical forms tell them apart")

    renumbered = pyvefr.StaticGraph(both_ways(HEXAGON_RENUMBERED))
    assert renumbered.canonical_form() == hexagon.canonical_form()
    print("the renumbered hexagon is isomorphic to the hexagon")
    # Colors, a color per vertex index, take part: one vertex colored apart, the forms differ.
    assert hexagon.canonical_form([1, 0, 0, 0, 0, 0]) != hexagon.canonical_form()
    print("colors take part in the canonical form")


def main() -> None:
    """Run each part."""
    structure()
    traversal()
    shortest_paths()
    refinement_and_nauty()


if __name__ == "__main__":
    main()
