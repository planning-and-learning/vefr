# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Graph algorithms over urd's graphs: color refinement, Weisfeiler-Leman, nauty and the Boost Graph Library."""

from importlib import import_module as _import_module
from pathlib import Path

# The native libraries vefr links (nauty, Boost.Graph) come with pyurd_native: load them before the extension.
_import_module("pyurd_native")

from pyvefr._pyvefr import (
    CanonicalForm as CanonicalForm,
    Certificate1 as Certificate1,
    Certificate2 as Certificate2,
    Certificate3 as Certificate3,
    ControlFlow as ControlFlow,
    Edge as Edge,
    Graph as Graph,
    StaticGraph as StaticGraph,
    Visitor as Visitor,
)


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
