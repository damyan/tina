# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Helper functions for the `tina-convert` unit tests."""

from __future__ import annotations

import subprocess  # noqa: S404
import typing

import pytest

from unit import util as t_util


if typing.TYPE_CHECKING:
    import pathlib


def list_formats(prog: pathlib.Path) -> list[str]:
    """Run `tina-convert list formats`, validate some of the output."""
    match subprocess.check_output(  # noqa: S603
        [prog, "list", "formats"],
        encoding="UTF-8",
        env=t_util.get_utf8_env(),
    ).splitlines():
        case [single]:
            match single.split(" "):
                case ["Formats:", *formats]:
                    assert "tina" in formats
                    assert all(word for word in formats)
                    return sorted(formats)

                case other:
                    pytest.fail(repr(other))

        case other:
            pytest.fail(repr(other))
