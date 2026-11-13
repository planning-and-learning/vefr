# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""A downstream module's graph takes pyvefr's functions.

It binds the graph with urd::python::bind_graph and vefr::python::bind_algorithms.
"""

import importlib
import os
import sys
from types import ModuleType

import pytest

import pyvefr


def downstream() -> ModuleType:
    directory = os.environ.get("VEFR_DOWNSTREAM_TEST_DIR")
    if directory is None:
        pytest.skip("VEFR_DOWNSTREAM_TEST_DIR names no built downstream test module")
    sys.path.insert(0, directory)
    return importlib.import_module("_pyvefr_downstream_test")


def test_a_bound_graph_takes_pyvefrs_algorithms() -> None:
    module = downstream()
    g = module.NamedGraph([(0, 1), (1, 2), (2, 0)], ["a", "b", "c"])
    assert g.vertex_value(1) == "b"
    assert g.num_vertices == 3
    assert [g.target(edge) for edge in g.out_edges(0)] == [1]
    assert isinstance(pyvefr.find_cycle(g)[0], pyvefr.Edge)
    assert pyvefr.breadth_first_search(g, [0])[1] == [0, 1, 2]
    assert len(pyvefr.find_cycle(g)) == 3
    assert pyvefr.strong_components(g)[0] == 1
    assert pyvefr.canonical_form(g) == pyvefr.canonical_form(pyvefr.StaticGraph([(1, 2), (2, 0), (0, 1)]))
    # pyvefr's own graphs still take them as before
    assert pyvefr.breadth_first_search(pyvefr.Graph([(0, 1)]), [0])[1] == [0, 1]
