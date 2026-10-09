# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Helper functions for the tina suite unit tests."""

from __future__ import annotations

import enum
import functools
import json
import os
import pathlib
import shutil
import subprocess  # noqa: S404
import typing

import pytest


if typing.TYPE_CHECKING:
    from typing import Any, Final


TEST_DB: Final = pathlib.Path(__file__).resolve().parent.parent.parent / "data" / "tina-test.db"
"""The test database in the tina native format."""

TAG_SKIP: Final = "SKIP"
"""The value of the environment variable for the program path to skip the tests."""


class ProgramType(enum.StrEnum):
    """The programs we know how to test."""

    CONVERT = "tina-convert"
    """The tina-convert command-line utility."""

    MGR = "tina"
    """The tina personal information manager itself."""

    def env_var_prog(self) -> str:
        """Get the environment variable name to override the program path with."""
        match self:
            case ProgramType.CONVERT:
                return "TINA_CONVERT"

            case ProgramType.MGR:
                return "TINA"

            case other:
                raise RuntimeError(repr(other))


@functools.lru_cache
def get_prog_path(prog: ProgramType) -> pathlib.Path:
    """Get the path to the specified program to test."""
    var: Final = prog.env_var_prog()
    env_value: Final = os.environ.get(var, None)
    if env_value is not None:
        if env_value == TAG_SKIP:
            pytest.skip(f"No testing for {prog} requested ({var} = {env_value})")

        return pathlib.Path(env_value)

    path: Final = shutil.which(str(prog))
    if path is None:
        pytest.fail(f"No {prog} in the search path and no {var} in the environment")

    return pathlib.Path(path)


@functools.lru_cache
def get_utf8_env() -> dict[str, str]:
    """Prepare a UTF-8-capable environment to run child processes in."""
    env: Final = dict(os.environ)
    env["LC_ALL"] = "C.UTF-8"
    if "LANGUAGE" in env:
        del env["LANGUAGE"]
    return env


def load_db(db: pathlib.Path, *, infmt: str = "tina") -> list[dict[str, Any]]:
    """Read the database in as an a list of untyped dicts."""
    prog_cvt: Final = get_prog_path(ProgramType.CONVERT)

    res: Final = json.loads(
        subprocess.check_output(  # noqa: S603
            [prog_cvt, "convert", "-I", infmt, "-O", "json", "--", db],
            encoding="UTF-8",
            env=get_utf8_env(),
        ),
    )
    assert isinstance(res, list)
    for item in res:
        assert isinstance(item, dict)
    return res
