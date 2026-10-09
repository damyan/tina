# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Test some simple conversions between formats."""

from __future__ import annotations

import json
import pathlib
import subprocess  # noqa: S404
import tempfile
import typing

import pytest
import yaml

from unit import util as t_util

from . import defs as tc_defs


if typing.TYPE_CHECKING:
    from typing import Any, Final


ALL_FORMATS: Final = ["tina", "json", "yaml"]
"""The formats we expect to be able to convert from and to."""


def tina_blocks(contents: str) -> list[str]:
    """Break the contents of a tina native database into blocks, sort them."""
    return sorted(block.strip() for block in contents.split("\n\n"))


def fmt_check_tina(contents: str) -> None:
    """Make sure the contents of a tina database are the same as the original one."""
    blocks: Final = tina_blocks(contents)
    exp_blocks: Final = tina_blocks(t_util.TEST_DB.read_text(encoding="UTF-8"))
    assert blocks == exp_blocks


def fmt_check(fmt: str, contents: str) -> None:
    """Decode the data read from the converted tina database."""
    match fmt:
        case "json":
            data = json.loads(contents)

        case "tina":
            fmt_check_tina(contents)
            return

        case "yaml":
            data = yaml.safe_load(contents)

        case other:
            raise RuntimeError(repr((other, contents)))

    assert isinstance(data, dict)
    assert data["format"]["version"]["major"] == 1
    assert isinstance(data["format"]["version"]["minor"], int)
    tina_db: Final = data["tina"]
    assert isinstance(tina_db, list)
    assert tina_db == tc_defs.TEST_DATA


@pytest.mark.parametrize("fmt", [fmt for fmt in ALL_FORMATS if fmt != "tina"])
def test_convert(fmt: str) -> None:
    """Test conversion from and to the specified format."""
    print()
    prog: Final = t_util.get_prog_path(t_util.ProgramType.CONVERT)

    with tempfile.TemporaryDirectory(prefix="test-tina-convert.") as tempd_obj:
        tempd: Final = pathlib.Path(tempd_obj)

        print(f"Converting the test database to {fmt}")
        fmt_db: Final = tempd / f"test-db.{fmt}"
        assert not fmt_db.is_symlink()
        assert not fmt_db.exists()
        subprocess.check_call(  # noqa: S603
            [prog, "convert", "-O", fmt, "-o", fmt_db, "--", t_util.TEST_DB],
            env=t_util.get_utf8_env(),
        )
        assert not fmt_db.is_symlink()
        assert fmt_db.is_file()

        fmt_check(fmt, fmt_db.read_text(encoding="UTF-8"))

        for dest_fmt in (dest_fmt for dest_fmt in ALL_FORMATS if dest_fmt != fmt):
            out_db = tempd / f"out-db.{dest_fmt}"
            assert not out_db.is_symlink()
            assert not out_db.exists()
            subprocess.check_call(  # noqa: S603
                [prog, "convert", "-I", fmt, "-O", dest_fmt, "-o", out_db, "--", fmt_db],
                env=t_util.get_utf8_env(),
            )
            assert not out_db.is_symlink()
            assert out_db.is_file()

            fmt_check(dest_fmt, out_db.read_text(encoding="UTF-8"))


@pytest.mark.parametrize("fmt", ALL_FORMATS)
def test_invalid_contents(*, fmt: str) -> None:
    """Make sure `tina-convert` fails on a file that is not a Tina database."""
    print()
    prog: Final = t_util.get_prog_path(t_util.ProgramType.CONVERT)

    with tempfile.TemporaryDirectory(prefix="test-tina-convert.") as tempd_obj:
        tempd: Final = pathlib.Path(tempd_obj)
        print(f"Using {tempd} as a temporary directory")

        source: Final = tempd / "source.txt"
        source.write_text("This is not a Tina database.\n", encoding="UTF-8")

        res: Final = subprocess.run(  # noqa: S603
            [prog, "convert", "-I", fmt, "--", source],
            check=False,
            env=t_util.get_utf8_env(),
        )
        assert res.returncode, repr(res)


@pytest.mark.parametrize("fmt", ALL_FORMATS)
def test_duplicates(*, fmt: str) -> None:
    """Make sure `tina-convert` breaks on duplicate items."""
    print()
    prog: Final = t_util.get_prog_path(t_util.ProgramType.CONVERT)

    with tempfile.TemporaryDirectory(prefix="test-tina-convert.") as tempd_obj:
        tempd: Final = pathlib.Path(tempd_obj)
        print(f"Using {tempd} as a temporary directory")

        def prepare_hier_items() -> dict[str, Any]:
            """Prepare a dictionary to be stored into a JSON or YAML file."""
            return {
                "format": {"version": {"major": 1, "minor": 0}},
                "tina": [
                    {"id": "<616@beleriand>", "desc": "An item", "children": []},
                    {"id": "<616@beleriand>", "desc": "The same item", "children": []},
                ],
            }

        def prepare_source() -> pathlib.Path:
            """Prepare the source file in the specified format."""
            match fmt:
                case "json":
                    source = tempd / "source.json"
                    source.write_text(json.dumps(prepare_hier_items()), encoding="UTF-8")
                    return source

                case "tina":
                    source = tempd / "source.tina"
                    source.write_text(
                        """Item-ID: <616@beleriand>
Description: An item

Item-ID: <616@beleriand>
Description: The same item
""",
                        encoding="UTF-8",
                    )
                    return source

                case "yaml":
                    source = tempd / "source.json"
                    source.write_text(yaml.safe_dump(prepare_hier_items()), encoding="UTF-8")
                    return source

                case other:
                    raise RuntimeError(repr(other))

        source: Final = prepare_source()
        res: Final = subprocess.run(  # noqa: S603
            [prog, "convert", "-I", fmt, "--", source],
            check=False,
            encoding="UTF-8",
            env=t_util.get_utf8_env(),
            stdout=subprocess.PIPE,
        )
        assert res.returncode
        assert not res.stdout
