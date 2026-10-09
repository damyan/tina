# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Test a few simple commands for tina."""

from __future__ import annotations

import pathlib
import tempfile
import typing

import pytest

from tina_mgr import parse

from . import util as tt_util


if typing.TYPE_CHECKING:
    from typing import Final


MANY: Final = 100
"""What we consider 'many' for the purposes of these tests."""


def test_one_run() -> None:
    """Create lots of items, all in one run."""
    print()

    with tt_util.tina_with_test_db_in_tempd(empty_db=True) as (tina, db_contents):
        assert not db_contents

        cfgpath: Final = tina.home / ".tina"
        assert not cfgpath.is_symlink()
        assert not cfgpath.exists()

        for idx in range(MANY):
            tina.pane.send_keys(f"oItem {idx + 1}", enter=True, literal=True)

        tina.quit()

        entries: Final = parse.loads(cfgpath.read_text(encoding="UTF-8"))
        assert len(entries) == MANY
        for idx, entry in enumerate(entries):
            assert entry.desc == f"Item {idx + 1}"

        assert len({entry.id for entry in entries}) == MANY


@pytest.mark.parametrize("recycle", [False, True])
def test_many_runs(*, recycle: bool) -> None:
    """Create lots of items, all in one run."""
    print()

    with tempfile.TemporaryDirectory(prefix="tina-test.") as tempd_obj:
        tempd: Final = pathlib.Path(tempd_obj)
        print(f"Using {tempd} as a temporary directory")

        cfgpath: Final = tempd / ".tina"
        assert not cfgpath.is_symlink()
        assert not cfgpath.exists()

        for idx in range(1, MANY + 1):
            print(f"Running tina iteration {idx}")

            with tt_util.tina_with_test_db_in(tempd, empty_db=True) as (tina, db_contents):
                assert len(db_contents) == idx - 1

                print(f"Creating item {idx}")
                tina.pane.send_keys(f"GoItem {idx}", enter=True, literal=True)

                print("Quitting tina so that it will save the database")
                tina.quit()

            contents = cfgpath.read_text(encoding="UTF-8")
            print(contents)
            entries = parse.loads(contents)
            assert len(entries) == idx
            for item_idx, entry in enumerate(entries):
                assert entry.desc == f"Item {item_idx + 1}"

            assert len({entry.id for entry in entries}) == idx

        if not recycle:
            return

        print("Recycling")
        for idx in range(1, MANY + 1):
            large_idx = idx + MANY

            print(f"Running recycle iteration {idx}")

            with tt_util.tina_with_test_db_in(tempd, empty_db=True) as (tina, db_contents):
                assert len(db_contents) == MANY

                print(f"Creating item {large_idx}")
                tina.pane.send_keys(f"dGoItem {large_idx}", enter=True, literal=True)

                print("Quitting tina so that it will save the database")
                tina.quit()

            contents = cfgpath.read_text(encoding="UTF-8")
            print(contents)
            entries = parse.loads(contents)
            assert len(entries) == MANY

            assert len({entry.id for entry in entries}) == MANY
