Graphs and colorings
====================

``vefr/graphs.hpp`` and ``vefr/properties.hpp``.

.. cpp:namespace:: vefr

What an algorithm takes
-----------------------

vefr's algorithms take a graph by urd's graph concepts (``urd/graphs.hpp``, after the Boost Graph Library's), each
stating what it needs: a search ``urd::adjacency_graph``, edge numbers ``urd::incidence_graph``, ``vefr::reversed``
``urd::in_adjacency_graph``, color refinement and the properties ``urd::graph_like`` (adjacency too). Every urd graph
is graph-like; one that keeps edge lists is an incidence and an edge list graph. A graph of one's own with the same
primitives qualifies too.

.. cpp:type:: template<urd::vertex_list_graph G> vertex_t = typename G::vertex_type

Vertices are ``0..n-1`` by their index (``urd::index_of``). A graph that lost vertices has holes:

.. cpp:function:: template<typename V> std::size_t index(V v)
.. cpp:function:: template<urd::vertex_list_graph G> std::size_t vertex_bound(const G& g)

   One past the largest vertex index. O(1) where the vertices can be read from the back; O(n) for a graph that lost
   vertices.

.. cpp:class:: template<urd::vertex_list_graph G> Positions

   The vertices numbered densely, in increasing order: ``of(v)`` is a vertex's position, ``at(position)`` the vertex
   there, ``size()`` their number. Certificates and canonical forms number vertices by position.

Colorings
---------

A coloring is a function of a vertex; the graph does not hold it.

.. cpp:type:: Color = std::uint32_t
.. cpp:type:: Colors = urd::Store<urd::sequence<Color>>

.. cpp:concept:: template<typename F, typename V> coloring

   ``std::regular_invocable<const F&, V>``, returning an integer, an enum or an urd index (read as its index).

.. cpp:var:: inline constexpr auto uncolored

   Every vertex the same color: the default of every algorithm that takes a coloring.

Properties
----------

Read from a graph's neighbours, each in O(n + m) (``is_multigraph`` sorts each row).

.. cpp:function:: template<urd::graph_like G> bool is_loopless(const G& g)
.. cpp:function:: template<urd::graph_like G> bool is_multigraph(const G& g)

   Whether two edges lead from one vertex to the same vertex.

.. cpp:function:: template<urd::graph_like G> bool is_symmetric(const G& g)

   Whether every edge has one back.

.. cpp:function:: template<urd::graph_like G> bool is_simple_undirected(const G& g)
