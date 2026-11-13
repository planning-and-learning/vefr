Searches and paths
==================

``vefr/algorithms.hpp``.

.. cpp:namespace:: vefr

The algorithms are written on a graph's ranges: its ``vertices()``, and each vertex's ``out_edges(u)`` where it
numbers its edges, else its ``neighbours(u)``. As in the Boost Graph Library, a search reports what it does to a
visitor, and the visitor may stop it. Unlike there, a visitor is any object with the events it wants, visitors compose,
and the parameters are plain arguments: sources are any range of vertices, a weight is a function of an edge or of its
number, and results are urd Stores keyed by vertex. None recurses: deep graphs do not overflow the stack.

.. code-block:: cpp

   // Shortest paths from 0, stopping once 7's distance is final; `record` sees each vertex settled.
   struct
   {
       std::vector<V> settled;
       void examine_vertex(V v) { settled.push_back(v); }
   } record;
   const auto length = [&](const vefr::Edge<V>& e) { return distance(e.source, e.target); };
   const auto paths = vefr::dijkstra_shortest_paths(g, std::array { V { 0 } }, length,
                                                    vefr::visitors(record, vefr::stop_at(std::array { V { 7 } })));
   const auto route = vefr::path_to(paths, V { 7 });

Edges, data and weights
-----------------------

.. cpp:struct:: template<typename V> Edge

   An edge as the algorithms hand it out: ``source``, ``target`` and ``id``, its number, or ``Edge::none`` where the
   graph numbers no edges.

.. cpp:function:: template<urd::adjacency_graph G> auto out_edges_of(const G& g, vertex_t<G> u)

   The edges leaving ``u``, as :cpp:struct:`Edge`.

.. cpp:type:: template<typename T, typename V> VertexMap = urd::Store<urd::sequence<T, V>>

   Data per vertex, keyed by vertex.

.. cpp:function:: template<typename T, urd::adjacency_graph G> VertexMap<T, vertex_t<G>> vertex_map(const G& g, const T& value = T {})

   Sized :cpp:func:`vertex_bound`, every entry ``value``: a graph that lost vertices leaves holes.

.. cpp:concept:: template<typename R, typename G> vertex_range

   An input range of vertices of ``G``: a container, a span, a view.

.. cpp:concept:: template<typename F, typename G> edge_weight

   A function of an :cpp:struct:`Edge` (a length from its endpoints, say), or of its number, which the graph must then
   give (``urd::incidence_graph``).

.. cpp:type:: template<typename G, typename F> weight_t

   What a weight ``F`` gives on ``G``'s edges: the type of a distance.

Visitors
--------

A visitor is any object with the events it wants, each taking a vertex or an :cpp:struct:`Edge`. Events it does not have
are not called. An event may return :cpp:enum:`Flow`: ``Flow::stop`` ends the search where it is.

.. cpp:enum-class:: Flow

   .. cpp:enumerator:: proceed
   .. cpp:enumerator:: stop

.. cpp:struct:: null_visitor

   No events: the default.

.. cpp:class:: template<typename... Visitors> visitors

   Visitors called one after the other on each event; the search stops where any of them asks. A named visitor is held
   by reference, a temporary by value: ``vefr::visitors(record, vefr::stop_at(targets))``.

.. cpp:class:: template<typename V> stop_at

   Stops a search once it has examined every vertex of the targets given: in breadth-first search and Dijkstra's, where
   their distances are then final.

Searches
--------

.. cpp:function:: template<urd::adjacency_graph G, vertex_range<G> R, typename Visitor> void breadth_first_visit(const G& g, R&& sources, Visitor&& visitor)

   Events: ``start_vertex``, ``discover_vertex``, ``examine_vertex``, ``examine_edge``, ``tree_edge``,
   ``non_tree_edge``, ``gray_target``, ``black_target``, ``finish_vertex``.

.. cpp:function:: template<urd::adjacency_graph G, vertex_range<G> R, typename Visitor> void depth_first_visit(const G& g, R&& sources, Visitor&& visitor)

   A source at a time, skipping those already reached. Events: ``start_vertex``, ``discover_vertex``,
   ``examine_edge``, ``tree_edge``, ``back_edge``, ``forward_or_cross_edge``, ``finish_edge``, ``finish_vertex``.

.. cpp:struct:: template<typename V, typename D> Paths

   .. cpp:member:: VertexMap<std::optional<V>, V> predecessors

      Each vertex's predecessor on a path from a source; ``nullopt`` for a source and for a vertex not reached.

   .. cpp:member:: VertexMap<D, V> distances

.. cpp:function:: template<urd::adjacency_graph G, vertex_range<G> R, typename Visitor> auto breadth_first_search(const G& g, R&& sources, Visitor&& visitor = {})

   Shortest paths in edges, ``Paths<V, std::optional<std::size_t>>``; ``visitor`` sees the search too, and may stop it.

.. cpp:function:: template<urd::adjacency_graph G, vertex_range<G> R, typename Visitor> auto depth_first_search(const G& g, R&& sources, Visitor&& visitor = {})

   Each vertex's parent in the depth-first forest, ``VertexMap<std::optional<V>, V>``.

.. cpp:function:: template<typename V, typename D> auto path_to(const Paths<V, D>& paths, V target)

   The vertices of the path a search found to ``target``, from a source on; empty where it did not reach ``target``.

Structure
---------

.. cpp:function:: template<urd::adjacency_graph G> auto topological_sort(const G& g)

   The vertices in topological order, each before the vertices its edges lead to; a cycle throws
   :cpp:struct:`not_a_dag`, a ``std::invalid_argument``.

.. cpp:struct:: not_a_dag

.. cpp:function:: template<urd::adjacency_graph G> auto find_cycle(const G& g)

   A closed path of edges, each leaving where the one before enters; empty where the graph has none.

.. cpp:function:: template<urd::adjacency_graph G> auto strong_components(const G& g)

   Tarjan's algorithm, O(n + m): ``Components<V>``, their ``count`` and each vertex's ``component``, numbered in the
   order they close, a component before the components that reach it.

Shortest paths
--------------

Distances not reached are infinite where the type has an infinity, else its largest value.

.. cpp:function:: template<urd::adjacency_graph G, vertex_range<G> R, edge_weight<G> F, typename Visitor> void dijkstra_visit(const G& g, R&& sources, const F& weight, Paths<vertex_t<G>, weight_t<G, F>>& paths, Visitor&& visitor = {})

   Dijkstra's algorithm into ``paths``, which the caller keeps across searches (made with :cpp:func:`vertex_map`) and
   which it resets. No weight is negative; O((n + m) log n). Events: ``discover_vertex`` (a distance first found),
   ``examine_vertex`` (settled: its distance is final), ``examine_edge``, ``edge_relaxed``, ``edge_not_relaxed``,
   ``finish_vertex``.

.. cpp:function:: template<urd::adjacency_graph G, vertex_range<G> R, edge_weight<G> F, typename Visitor> auto dijkstra_shortest_paths(const G& g, R&& sources, const F& weight, Visitor&& visitor = {})

   The same into new ``Paths<V, D>``, ``D`` what ``weight`` returns. With :cpp:class:`stop_at`, the targets' distances
   and paths are final; a vertex not settled may have only a bound.

.. cpp:function:: template<urd::adjacency_graph G, edge_weight<G> F> auto floyd_warshall_all_pairs_shortest_paths(const G& g, const F& weight)

   The distance between every two vertices, ``result[u][v]``; O(n\ :sup:`3`). A negative cycle throws
   ``std::domain_error``.
