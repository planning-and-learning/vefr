# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""A downstream module's graph and vefr's algorithms as its methods.

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


def test_a_bound_graph_has_the_algorithms_as_methods() -> None:
    module = downstream()
    g = module.NamedGraph([(0, 1), (1, 2), (2, 0)], ["a", "b", "c"])
    assert g.vertex_value(1) == "b"
    assert g.num_vertices == 3
    assert [g.target(edge) for edge in g.out_edges(0)] == [1]
    assert isinstance(g.find_cycle()[0], pyvefr.Edge)
    assert g.breadth_first_search([0])[1] == [0, 1, 2]
    assert len(g.find_cycle()) == 3
    assert g.strong_components()[0] == 1
    # its vertices colored by their names' lengths: equal for the same names around the cycle, else not
    rotated = module.NamedGraph([(1, 2), (2, 0), (0, 1)], ["c", "a", "b"])
    assert g.canonical_form() == rotated.canonical_form()
    longer = module.NamedGraph([(0, 1), (1, 2), (2, 0)], ["a", "bb", "c"])
    assert g.canonical_form() != longer.canonical_form()
    assert g.canonical_form([0, 0, 0]) == longer.canonical_form([0, 0, 0])
    assert g.color_refinement()[0] != longer.color_refinement()[0]
    assert pyvefr.Graph([(0, 1)]).breadth_first_search([0])[1] == [0, 1]
