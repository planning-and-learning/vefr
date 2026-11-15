# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Graph algorithms over urd's graphs: color refinement, Weisfeiler-Leman, nauty and the Boost Graph Library."""

from importlib import import_module as _import_module
from pathlib import Path
from typing import Protocol

# The native libraries vefr links (nauty, Boost.Graph) come with pyurd_native: load them before the extension.
_import_module("pyurd_native")

from pyvefr._pyvefr import (
    CanonicalForm as CanonicalForm,
    Certificate as Certificate,
    Certificate2 as Certificate2,
    Certificate3 as Certificate3,
    Edge as Edge,
    Flow as Flow,
    Graph as Graph,
    StaticGraph as StaticGraph,
    Visitor as Visitor,
    breadth_first_search as breadth_first_search,
    breadth_first_visit as breadth_first_visit,
    canonical_form as canonical_form,
    color_refinement as color_refinement,
    depth_first_search as depth_first_search,
    depth_first_visit as depth_first_visit,
    dijkstra_shortest_paths as dijkstra_shortest_paths,
    find_cycle as find_cycle,
    floyd_warshall_all_pairs_shortest_paths as floyd_warshall_all_pairs_shortest_paths,
    is_loopless as is_loopless,
    is_multigraph as is_multigraph,
    is_simple_undirected as is_simple_undirected,
    is_symmetric as is_symmetric,
    isomorphism as isomorphism,
    strong_components as strong_components,
    topological_sort as topological_sort,
    weisfeiler_leman as weisfeiler_leman,
)


class GraphLike(Protocol):
    """What the algorithms take: a graph urd binds, pyvefr's own or a downstream package's.

    vefr::python::bind_algorithms adds a downstream graph's overloads; a graph without them raises TypeError.
    """

    @property
    def num_vertices(self) -> int:
        """The number of vertices."""
        ...


def native_prefix() -> Path:
    """Return the prefix of vefr's bundled native build (headers, libraries, CMake files)."""
    package_dir = Path(__file__).resolve().parent
    native_dir = package_dir / "native"
    if (native_dir / "include" / "vefr").is_dir():
        return native_dir
    for parent in package_dir.parents:
        if (parent / "include" / "vefr").is_dir():
            return parent
    return native_dir


def native_include_dir() -> Path:
    """Return the directory of vefr's C++ headers (and vefr/python/bindings.hpp for downstream bindings)."""
    return native_prefix() / "include"


def native_lib_dir() -> Path:
    """Return the directory of vefr's native libraries."""
    native_dir = native_prefix()
    for lib_dir_name in ("lib", "lib64"):
        lib_dir = native_dir / lib_dir_name
        if lib_dir.is_dir():
            return lib_dir
    return native_dir / "lib"


def cmake_prefix() -> Path:
    """Return the prefix to put on CMAKE_PREFIX_PATH to find vefr via find_package."""
    return native_prefix()
