# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Test some simple conversions between formats."""

from __future__ import annotations

import itertools
import pathlib
import subprocess  # noqa: S404
import tempfile
import typing

import pytest

from unit import util as t_util


if typing.TYPE_CHECKING:
    from typing import Final


@pytest.mark.parametrize("length", list(range(1, 11)))
def test_loop(*, length: int) -> None:
    """Make sure `tina-convert` refuses to output a loop."""
    print()
    prog: Final = t_util.get_prog_path(t_util.ProgramType.CONVERT)

    with tempfile.TemporaryDirectory(prefix="test-tina-convert.") as tempd_obj:
        tempd: Final = pathlib.Path(tempd_obj)
        print(f"Using {tempd} as a temporary directory")

        lines: Final = []

        for item_idx in range(1, length):
            parent_idx = length if item_idx == 1 else item_idx - 1
            lines.extend(
                [
                    f"Item-ID: <{item_idx}@beleriand>",
                    f"Description: item {item_idx}",
                    f"Category: <{parent_idx}@beleriand>",
                ],
            )

        lines.extend(
            [
                f"Item-ID: <{length}@beleriand>",
                f"Description: item {length}",
                "Category: <1@beleriand>",
            ],
        )
        contents: Final = "".join(f"{line}\n" for line in lines)

        source: Final = tempd / "source.tina"
        target: Final = tempd / "target.json"

        assert not target.is_symlink()
        assert not target.exists()

        source.write_text(contents, encoding="UTF-8")
        print(source.read_text(encoding="UTF-8"))
        res: Final = subprocess.run(  # noqa: S603
            [prog, "convert", "-I", "tina", "-O", "json", "-o", target, "--", source],
            check=False,
            env=t_util.get_utf8_env(),
        )
        assert res.returncode, repr(res)

        assert not target.is_symlink()
        assert not target.exists()
        assert source.read_text(encoding="UTF-8") == contents


@pytest.mark.parametrize(
    ("count_reachable", "count_unreachable"),
    itertools.product(range(5), range(1, 5)),
)
def test_unreachable(*, count_reachable: int, count_unreachable: int) -> None:
    """Make sure `tina-convert` fails on unreachable entries."""
    print()
    prog: Final = t_util.get_prog_path(t_util.ProgramType.CONVERT)

    with tempfile.TemporaryDirectory(prefix="test-tina-convert.") as tempd_obj:
        tempd: Final = pathlib.Path(tempd_obj)
        print(f"Using {tempd} as a temporary directory")

        lines: Final = []

        for item_idx in range(1, count_reachable + 1):
            lines.extend(
                [
                    f"Item-ID: <ok{item_idx}@beleriand>",
                    f"Description: reachable item {item_idx}",
                ],
            )

        for item_idx in range(1, count_unreachable + 1):
            lines.extend(
                [
                    f"Item-ID: <unreach{item_idx}@beleriand>",
                    f"Description: unreachable item {item_idx}",
                    f"Category: <bad{item_idx}@beleriand>",
                ],
            )

        contents: Final = "".join(f"{line}\n" for line in lines)

        source: Final = tempd / "source.tina"
        target: Final = tempd / "target.json"

        assert not target.is_symlink()
        assert not target.exists()

        source.write_text(contents, encoding="UTF-8")
        print(source.read_text(encoding="UTF-8"))
        res: Final = subprocess.run(  # noqa: S603
            [prog, "convert", "-I", "tina", "-O", "json", "-o", target, "--", source],
            check=False,
            env=t_util.get_utf8_env(),
        )
        assert res.returncode, repr(res)

        assert not target.is_symlink()
        assert not target.exists()
        assert source.read_text(encoding="UTF-8") == contents
