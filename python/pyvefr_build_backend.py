# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""PEP 517 backend for the pyvefr wheel; machinery lives in pybrokkr.build_backend."""

from pybrokkr.build_backend import ProviderBackend

ProviderBackend(
    package="pyvefr",
    providers=("pyurd", "pyurd_native"),
    prefix_defines={
        "pyurd": "URD_PREFIX",
        "pyurd_native": "URD_NATIVE_PREFIX",
    },
    jobs_env="VEFR_JOBS",
    strip_env="VEFR_STRIP_WHEEL",
).install_hooks(globals())
