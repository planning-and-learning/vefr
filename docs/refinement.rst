Color refinement and Weisfeiler-Leman
=====================================

``vefr/refinement.hpp``.

.. cpp:namespace:: vefr

Both algorithms refine colors until no color splits any more. Each round gives every item (a vertex, or a k-tuple of
vertices) a signature: its color and the sorted colors around it. The signatures are then sorted and numbered, and each
item's new color is its signature's number. A certificate records each round: every signature and how many items had
it. Two graphs whose certificates differ are not isomorphic. Equal certificates prove nothing: color refinement, for
one, cannot tell regular graphs of one degree apart.

Colors are numbered by sorted signature in every round, the first included. A certificate therefore does not depend on
how the vertices are numbered.

.. cpp:struct:: template<typename T> Signature

   .. cpp:member:: Color color
   .. cpp:member:: urd::Store<urd::sequence<T>> neighbours

      Sorted: a color per neighbour (color refinement) or a tuple of colors per vertex (Weisfeiler-Leman).

.. cpp:struct:: template<typename T> Certificate

   .. cpp:member:: urd::Store<urd::sequence<Round>> rounds

      Per round, each signature seen and how many items had it, in sorted order.

   Certificates compare and hash by their rounds (``urd::Hash``), so they go in urd's sets and maps.

.. cpp:struct:: template<typename T> Refinement

   .. cpp:member:: Certificate<T> certificate
   .. cpp:member:: Colors colors

      The stable color of each item: a vertex by its position (:cpp:class:`Positions`), a k-tuple by its number in
      base n, its first place lowest.

.. cpp:function:: template<urd::graph_like G, coloring<vertex_t<G>> F> Refinement<Color> color_refinement(const G& g, const F& coloring = uncolored)

   A vertex's signature is its color and the colors of its out-neighbours. An undirected urd graph lists every
   neighbour. Costs O((n + m) log n) per round, at most n rounds.

.. cpp:function:: template<std::size_t K, urd::graph_like G, coloring<vertex_t<G>> F> Refinement<std::array<Color, K>> weisfeiler_leman(const G& g, const F& coloring = uncolored)

   k-dimensional Weisfeiler-Leman, ``K >= 2``, over the n\ :sup:`K` tuples of vertices. A tuple's first color is the
   colors of its vertices, with which of them are equal and which adjacent. Its neighbours are, for each vertex w, the
   colors of the K tuples with w in one place. Each round costs O(n\ :sup:`K+1` K); too many tuples to count throw
   ``std::length_error``.

   ``weisfeiler_leman<3>`` tells a hexagon from two triangles; color refinement does not.
