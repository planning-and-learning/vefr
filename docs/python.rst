pyvefr
======

.. py:module:: pyvefr

``pyvefr`` binds vefr for two graphs with vertices ``0..n-1`` (Python ints) and numbered edges. Colors and weights
are lists: a color per vertex index, a weight per edge number; a list too short raises ``ValueError``. Results are
lists by vertex index, ``None`` where there is no vertex or no distance, ``inf`` where no path leads.

.. code-block:: python

   import pyvefr

   g = pyvefr.Graph([(0, 1), (1, 2), (2, 0)])
   predecessors, distances = pyvefr.breadth_first_search(g, [0])
   form = pyvefr.canonical_form(g, colors=[0, 0, 1])

Graphs
------

.. py:class:: Graph(edges=None)

   A graph that grows and shrinks: urd's adjacency lists. Edges are numbered as added; a number is never reused.

   .. py:method:: add_vertex() -> int
   .. py:method:: add_edge(source: int, target: int) -> int
   .. py:method:: remove_edge(edge: int) -> None
   .. py:method:: remove_vertex(vertex: int) -> None
   .. py:method:: clear() -> None

.. py:class:: StaticGraph(graph: Graph)

   A graph built once from a :py:class:`Graph`: urd's compressed sparse rows, edges keeping their numbers, in-edges
   kept.

   .. py:method:: in_neighbours(vertex: int) -> list[int]
   .. py:method:: in_degree(vertex: int) -> int
   .. py:method:: in_edges(vertex: int) -> list[int]

Both have ``num_vertices``, ``num_edges``, ``vertices()``, ``neighbours(vertex)``, ``out_degree(vertex)``,
``adjacent(source, target)``, ``edges()``, ``out_edges(vertex)``, ``edge(source, target)``, ``source(edge)`` and
``target(edge)``; edges are their numbers. ``vertex_value(vertex)`` and ``edge_value(edge)`` give a graph's data,
``None`` where it carries none.

.. py:class:: Edge

   An edge as the algorithms hand it out (cycles, visitor events): ``source``, ``target`` and ``id``.

Certificates and canonical forms
--------------------------------

.. py:function:: color_refinement(graph, colors=None) -> tuple[Certificate, list[int]]
.. py:function:: weisfeiler_leman(graph, k, colors=None) -> tuple[Certificate2 | Certificate3, list[int]]

   ``k`` is 2 or 3. The colors are the stable color of each vertex by position, or of each k-tuple.

.. py:class:: Certificate

   Compares and hashes; ``num_rounds``. :py:class:`Certificate2` and :py:class:`Certificate3` are Weisfeiler-Leman's.

.. py:class:: Certificate2
.. py:class:: Certificate3

.. py:function:: canonical_form(graph, colors=None) -> CanonicalForm
.. py:function:: isomorphism(lhs: CanonicalForm, rhs: CanonicalForm) -> list[int] | None

.. py:class:: CanonicalForm

   Compares and hashes: equal exactly for isomorphic graphs. ``labeling`` is each vertex's canonical position.

Algorithms
----------

.. py:function:: is_loopless(graph) -> bool
.. py:function:: is_multigraph(graph) -> bool
.. py:function:: is_symmetric(graph) -> bool
.. py:function:: is_simple_undirected(graph) -> bool
.. py:function:: breadth_first_search(graph, sources, visitor=None) -> tuple[list[int | None], list[int | None]]
.. py:function:: depth_first_search(graph, sources, visitor=None) -> list[int | None]
.. py:function:: breadth_first_visit(graph, sources, visitor) -> None
.. py:function:: depth_first_visit(graph, sources, visitor) -> None
.. py:function:: strong_components(graph) -> tuple[int, list[int]]
.. py:function:: topological_sort(graph) -> list[int]
.. py:function:: find_cycle(graph) -> list[Edge]
.. py:function:: dijkstra_shortest_paths(graph, sources, weights, visitor=None, targets=None) -> tuple[list[int | None], list[float]]

   With ``targets``, it stops once their distances are final.
.. py:function:: floyd_warshall_all_pairs_shortest_paths(graph, weights) -> list[list[float]]

.. py:class:: Visitor

   Subclass it and override the events wanted: ``start_vertex``, ``discover_vertex``, ``examine_vertex``,
   ``finish_vertex`` take a vertex; ``examine_edge``, ``tree_edge``, ``non_tree_edge``, ``gray_target``,
   ``black_target``, ``back_edge``, ``forward_or_cross_edge``, ``finish_edge``, ``edge_relaxed`` and
   ``edge_not_relaxed`` an :py:class:`Edge`. An event returns ``None`` to go on, or ``Flow.STOP`` to stop the search.

.. py:class:: Flow

   ``PROCEED`` or ``STOP``.

   .. code-block:: python

      class Order(pyvefr.Visitor):
          def __init__(self) -> None:
              super().__init__()
              self.order: list[int] = []

          def discover_vertex(self, vertex: int) -> None:
              self.order.append(vertex)

Binding a graph of one's own
----------------------------

A library built on vefr binds its graph class (an urd graph) with two calls: urd binds the graph's members
(``urd::python::bind_graph``, ``urd/python/graphs.hpp``), vefr its algorithms and the class's DOT text
(``vefr::python::bind_algorithms``, ``vefr/python/bindings.hpp``). pyvefr's functions then take the class as they take
pyvefr's own graphs.

.. code-block:: cpp

   auto cls = nb::class_<MyGraph>(m, "MyGraph");  // MyGraph: an urd graph, its labels as its data
   urd::python::bind_graph(cls);
   vefr::python::bind_algorithms(cls, vefr::vertex_data, vefr::edge_data);
