vefr
====

vefr is a library of graph algorithms over `urd <https://planning-and-learning.github.io/urd/>`__'s graphs: color
refinement, the k-dimensional Weisfeiler-Leman algorithm, canonical forms by nauty, and searches, components and
shortest paths. It is written in C++20 with Python bindings (``pyvefr``).

vefr has no graph type of its own. Its algorithms take any graph through concepts: an urd graph
(``urd::Store<urd::graph<V, E>, ...>``), whatever representation its request resolves to, or a type of one's own with
the same primitives. Data per vertex lives outside the graph, as urd keeps it: a coloring is a function of a vertex,
results are urd Stores keyed by vertex.

.. code-block:: cpp

   #include <urd/repr/graphs/csr_graph.hpp>
   #include <vefr/algorithms.hpp>
   #include <vefr/nauty.hpp>
   #include <vefr/refinement.hpp>

   using Graph = urd::Store<urd::graph<std::uint32_t>, urd::frozen, urd::positional>;

   const auto g = Graph(edges.begin(), edges.end());
   const auto label = [&](std::uint32_t v) { return labels[v]; };

   const auto [certificate, colors] = vefr::color_refinement(g, label);
   const auto form = vefr::canonical_form(g, label);        // equal exactly for isomorphic graphs
   const auto paths = vefr::breadth_first_search(g, sources);

The rules the planning-and-learning libraries share are documented in
`brokkr <https://planning-and-learning.github.io/brokkr/>`__.

.. toctree::
   :maxdepth: 2
   :caption: C++

   graphs
   refinement
   nauty
   algorithms

.. toctree::
   :maxdepth: 2
   :caption: Python

   python
