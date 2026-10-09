# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Unit tests for `tina` and `tina-convert`."""

from __future__ import annotations

import json
import subprocess  # noqa: S404
import typing

import pytest

from unit import util as t_util
from unit.mgr import util as tt_util


if typing.TYPE_CHECKING:
    from typing import Final, TypedDict

    class JSONEntry(TypedDict):
        """A raw JSON entry."""

        id: str
        desc: str
        children: list[JSONEntry]


def test_categorize_invert() -> None:  # noqa: PLR0915
    """Set a category on an entry, then reverse that."""
    print()
    prog_cvt: Final = t_util.get_prog_path(t_util.ProgramType.CONVERT)

    with tt_util.tina_with_test_db_in_tempd(empty_db=True) as (tina, _empty_entries):
        db_path: Final = tina.home / ".tina"

        def no_db() -> None:
            """Make sure there is no database file yet."""
            assert not db_path.is_symlink()
            assert not db_path.exists()

        def print_db() -> None:
            """Dump the contents of the database file."""
            assert not db_path.is_symlink()
            assert db_path.is_file()
            print(db_path.read_text(encoding="UTF-8"))

        def get_json() -> list[JSONEntry]:
            """Get the raw JSON contents of the database file."""
            print_db()

            print("Converting that to JSON")
            top: Final = json.loads(
                subprocess.check_output(  # noqa: S603
                    [prog_cvt, "convert", "-O", "json", "--", db_path],
                    encoding="UTF-8",
                    env=t_util.get_utf8_env(),
                ),
            )
            print(repr(top))

            assert isinstance(top, dict)
            tina: Final = top.get("tina")
            assert isinstance(tina, list)
            assert all(isinstance(item, dict) for item in tina), repr(tina)
            return tina

        no_db()

        print("Creating a single 'Flip' entry")
        tina.pane.send_keys("oFlip", enter=True, literal=True)
        tina.assert_text_at(0, 1, "Flip")
        tina.assert_no_text_at(4, 1)
        tina.assert_no_text_at(0, 2)
        no_db()

        print("Saving the single 'Flip' entry")
        tina.pane.send_keys("$", enter=False, literal=True)
        tina.assert_text_at(0, 1, "Flip")
        tina.assert_no_text_at(4, 1)
        tina.assert_no_text_at(0, 2)
        match get_json():
            case [single]:
                assert isinstance(single, dict)
                assert single["desc"] == "Flip"
                assert single["children"] == []

            case other:
                pytest.fail(repr(other))

        print("Putting the 'Flip' entry into the 'Flop' category")
        tina.pane.send_keys("CFlop", enter=True, literal=True)
        print("Telling 'tina' to quit, hopefully it will save the database")
        tina.quit()

        match get_json():
            case [single_flop]:
                assert isinstance(single_flop, dict)
                assert single_flop["desc"] == "Flop"
                match single_flop["children"]:
                    case [single_flip]:
                        assert isinstance(single_flip, dict)
                        assert single_flip["desc"] == "Flip"
                        assert single_flip["children"] == []

            case other_top:
                pytest.fail(repr(other_top))
