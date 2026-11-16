pyvefr
======

.. py:module:: pyvefr

``pyvefr`` binds vefr for two graphs with vertices ``0..n-1`` (Python ints) and numbered edges. Colors and weights
are lists: a color per vertex index, a weight per edge number; a list too short raises ``ValueError``. Results are
lists by vertex index, ``None`` where there is no vertex or no distance, ``inf`` where no path leads.

.. code-block:: python

   import pyvefr

   g = pyvefr.Graph([(0, 1), (1, 2), (2, 0)])
   predecessors, distances = g.breadth_first_search([0])
   form = g.canonical_form(colors=[0, 0, 1])

``python/examples`` holds runnable examples of both graphs.

Graphs
------

.. py:class:: Graph(edges=None)

   A graph that grows and shrinks: urd's adjacency lists. Edges are numbered as added; a number is never reused. In-edges
   are kept.

   .. py:method:: add_vertex() -> int
   .. py:method:: add_edge(source: int, target: int) -> int
   .. py:method:: remove_edge(edge: int) -> None
   .. py:method:: remove_vertex(vertex: int) -> None
   .. py:method:: clear() -> None

.. py:class:: StaticGraph(graph: Graph | list[tuple[int, int]])

   A graph built once from a :py:class:`Graph`: urd's compressed sparse rows, edges keeping their numbers, in-edges
   kept.

Both have ``num_vertices``, ``num_edges``, ``vertices()``, ``neighbours(vertex)``, ``in_neighbours(vertex)``,
``out_degree(vertex)``, ``in_degree(vertex)``, ``adjacent(source, target)``, ``edges()``, ``out_edges(vertex)``,
``in_edges(vertex)``, ``edge(source, target)``, ``source(edge)`` and ``target(edge)``; edges are their numbers.
``vertex_value(vertex)`` and ``edge_value(edge)`` give a graph's data, ``None`` where it carries none.

.. py:class:: Edge

   An edge as the algorithms hand it out (cycles, visitor events): ``source``, ``target`` and ``id``.

Certificates and canonical forms
--------------------------------

The algorithms are methods of each graph, :py:class:`Graph`, :py:class:`StaticGraph` and any a library binds (below).

``colors`` is a color per vertex index; without it, a graph colors its vertices by what they hold, as its library
binds it (pyvefr's own graphs hold nothing: one color).

.. py:method:: Graph.color_refinement(colors=None) -> tuple[Certificate1, list[int]]
.. py:method:: Graph.weisfeiler_leman(k, colors=None) -> tuple[Certificate2 | Certificate3, list[int]]

   ``k`` is 2 or 3 for pyvefr's graphs, the k its library binds for another graph. The colors are the stable color of
   each vertex by position, or of each k-tuple.

.. py:class:: Certificate1

   Compares and hashes; ``num_rounds``. ``Certificate<k>`` is the certificate over k-tuples: :py:class:`Certificate1`
   color refinement's, :py:class:`Certificate2` and :py:class:`Certificate3` Weisfeiler-Leman's.

.. py:class:: Certificate2
.. py:class:: Certificate3

.. py:method:: Graph.canonical_form(colors=None) -> CanonicalForm

.. py:class:: CanonicalForm

   Compares and hashes: equal exactly for isomorphic graphs. ``labeling`` is each vertex's canonical position; of two
   equal forms, a vertex maps to the other graph's vertex of the same canonical position.

Algorithms
----------

.. py:method:: Graph.is_loopless() -> bool
.. py:method:: Graph.is_multigraph() -> bool
.. py:method:: Graph.is_symmetric() -> bool
.. py:method:: Graph.is_simple_undirected() -> bool
.. py:method:: Graph.breadth_first_search(sources, visitor=None) -> tuple[list[int | None], list[int | None]]
.. py:method:: Graph.depth_first_search(sources, visitor=None) -> list[int | None]
.. py:method:: Graph.breadth_first_visit(sources, visitor) -> None
.. py:method:: Graph.depth_first_visit(sources, visitor) -> None
.. py:method:: Graph.strong_components() -> tuple[int, list[int]]
.. py:method:: Graph.topological_sort() -> list[int]
.. py:method:: Graph.find_cycle() -> list[Edge]
.. py:method:: Graph.dijkstra_shortest_paths(sources, weights, visitor=None, targets=None) -> tuple[list[int | None], list[float]]

   With ``targets``, it stops once their distances are final. Only a graph that numbers its edges has it (weights are
   a list by edge number).
.. py:method:: Graph.floyd_warshall_all_pairs_shortest_paths(weights) -> list[list[float]]

   Only a graph that numbers its edges has it.

.. py:class:: Visitor

   Subclass it and override the events wanted: ``start_vertex``, ``discover_vertex``, ``examine_vertex``,
   ``finish_vertex`` take a vertex; ``examine_edge``, ``tree_edge``, ``non_tree_edge``, ``gray_target``,
   ``black_target``, ``back_edge``, ``forward_or_cross_edge``, ``finish_edge``, ``edge_relaxed`` and
   ``edge_not_relaxed`` an :py:class:`Edge`. An event returns ``None`` to go on, or ``ControlFlow.STOP`` to stop the search.

.. py:class:: ControlFlow

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
(``vefr::python::bind_algorithms``, ``vefr/python/bindings.hpp``) as methods of the class, so its generated stub lists
them. ``bind_algorithms`` binds every algorithm the graph takes; one binder each (``bind_find_cycle``,
``bind_breadth_first_search``, ...) binds a choice of them. Its vertex coloring, a function of the graph and a vertex
whose result is an integer, an enum or an urd index (``urd::vertex_data`` where the vertex data is one), colors the
refinements and canonical forms where Python gives no colors: by what a vertex holds, never by its number.
``bind_weisfeiler_leman<2, 3, 4>(cls, coloring)`` binds the k a graph needs; a k beyond 3 needs its certificate,
``bind_certificate<4>(m)``, in one module. The module imports pyvefr first, which registers the types
every graph shares (``Edge``, ``Visitor``, the certificates).

.. code-block:: cpp

   nb::module_::import_("pyvefr");
   auto cls = nb::class_<MyGraph>(m, "MyGraph");  // MyGraph: an urd graph, its labels as its data
   urd::python::bind_graph(cls);
   vefr::python::bind_algorithms(cls, vefr::python::one_color, urd::vertex_data, urd::edge_data);
