# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Test some simple conversions between formats."""

from __future__ import annotations

import subprocess  # noqa: S404
import typing

import pytest

from unit import util as t_util

from . import util as tc_util


if typing.TYPE_CHECKING:
    from typing import Final


@pytest.mark.parametrize(
    "opts",
    [["-h"], ["--help"], ["-V"], ["--version"], ["-h", "--version"], ["-V", "--help"]],
)
def test_help_version(*, opts: list[str]) -> None:
    """Make sure the `--help` and `--version` options do not raise errors."""
    print()
    prog: Final = t_util.get_prog_path(t_util.ProgramType.CONVERT)

    assert opts
    res: Final = subprocess.run(  # noqa: S603
        [prog, *opts],
        capture_output=True,
        check=False,
        encoding="UTF-8",
        env=t_util.get_utf8_env(),
    )
    print(repr(res))
    assert res.returncode == 0
    assert res.stdout
    assert not res.stderr


def test_list_formats() -> None:
    """Make sure `tina-convert list formats` outputs something sensible."""
    print()
    prog: Final = t_util.get_prog_path(t_util.ProgramType.CONVERT)

    formats: Final = tc_util.list_formats(prog)
    for expected in ("tina", "json"):
        assert expected in formats
