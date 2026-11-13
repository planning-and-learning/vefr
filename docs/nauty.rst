Canonical forms
===============

``vefr/nauty.hpp``, linked as ``vefr::vefr`` (nauty comes with ``pyurd_native``).

.. cpp:namespace:: vefr

A canonical form is a graph relabelled by `nauty <https://pallini.di.uniroma1.it/>`__ so that two colored graphs are
isomorphic exactly when their canonical forms are equal. Equal colors are what an isomorphism must keep: nauty's cells
are the vertices of each color, in increasing order of color.

.. cpp:type:: Numbers = urd::Store<urd::sequence<std::uint32_t>>

.. cpp:struct:: CanonicalForm

   .. cpp:member:: bool digraph

      Whether nauty read it as directed: where the graph is not symmetric or has loops.

   .. cpp:member:: Numbers offsets
   .. cpp:member:: Numbers targets

      The edges by canonical position, row after row, each row sorted.

   .. cpp:member:: Colors colors

      The color at each canonical position.

   .. cpp:member:: Numbers labeling

      Each vertex's canonical position, by its position (:cpp:class:`Positions`). Not part of the form: forms compare
      and hash by the members above.

.. cpp:function:: template<urd::adjacency_graph G, coloring<vertex_t<G>> F> CanonicalForm canonical_form(const G& g, const F& coloring = uncolored)

   Directed graphs and loops are fine. Parallel edges throw ``std::invalid_argument``. nauty keeps global state: call
   it from one thread at a time.

.. cpp:function:: std::optional<Numbers> isomorphism(const CanonicalForm& a, const CanonicalForm& b)

   An isomorphism from ``a``'s graph to ``b``'s, by vertex position, if their forms are equal: a vertex at position
   ``p`` maps to the vertex at position ``(*isomorphism(a, b))[p]``.
