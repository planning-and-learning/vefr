# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""The examples in python/examples run."""

import subprocess
import sys
from pathlib import Path

import pytest

EXAMPLES = Path(__file__).parent.parent / "examples"


@pytest.mark.parametrize("example", ["graph.py", "static_graph.py"])
def test_example_runs(example: str) -> None:
    result = subprocess.run(
        [sys.executable, EXAMPLES / example], capture_output=True, text=True, timeout=30, check=False
    )
    assert result.returncode == 0, result.stderr
