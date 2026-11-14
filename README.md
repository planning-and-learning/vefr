# vefr

Graph algorithms over [urd](https://github.com/planning-and-learning/urd)'s graphs: color refinement, the k-dimensional
Weisfeiler-Leman algorithm, canonical forms by nauty, and searches, components and shortest paths. vefr has no graph type of its own:
its algorithms take any urd graph (`urd::Store<urd::graph<V>, ...>`), or a graph of one's own with the same primitives
(`urd::graph_like`), and a coloring of its vertices that lives outside the graph, as urd keeps vertex data.

```cpp
#include <urd/repr/graphs/csr_graph.hpp>
#include <vefr/nauty.hpp>
#include <vefr/refinement.hpp>

using Graph = urd::Store<urd::graph<std::uint32_t>, urd::frozen>;

const auto g = Graph(edges.begin(), edges.end());
const auto colors = [&](std::uint32_t v) { return labels[v]; };
const auto [certificate, stable] = vefr::color_refinement(g, colors);
const auto form = vefr::canonical_form(g, colors);  // equal exactly for isomorphic graphs
```

| Header | What |
|---|---|
| `vefr/graphs.hpp` | graphs by urd's concepts, colorings, vertex positions |
| `vefr/properties.hpp` | `is_loopless`, `is_multigraph`, `is_symmetric`, `is_simple_undirected` |
| `vefr/refinement.hpp` | `color_refinement`, `weisfeiler_leman<K>`: certificates that do not depend on the numbering |
| `vefr/nauty.hpp` | `canonical_form` (directed graphs and loops too), `isomorphism` |
| `vefr/algorithms.hpp` | searches with visitors, topological order, strong components, cycles, shortest paths: written on a graph's ranges, results as Stores keyed by vertex |

pyvefr binds them for a growing `Graph` and a frozen `StaticGraph`.

## Build

```console
python -m pip install 'pyurd>=0.2.0,<0.3' pybrokkr
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DVEFR_BUILD_TESTS=ON \
      -DCMAKE_PREFIX_PATH="$(python -c 'import pyurd, pyurd_native; print(f"{pyurd.native_prefix()};{pyurd_native.native_prefix()}")')"
cmake --build build -j4
ctest --test-dir build --output-on-failure

python -m pip install .[test]
pytest python/tests
```

The documentation is at <https://planning-and-learning.github.io/vefr/> (`docs/`, built with Sphinx). The shared rules
of the planning-and-learning libraries are in [AGENTS.md](AGENTS.md).
