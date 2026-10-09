# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Test a few simple commands for tina."""

from __future__ import annotations

import dataclasses
import time
import typing

from tina_mgr import db
from tina_mgr import parse
from unit import util as t_util

from . import util as tt_util


if typing.TYPE_CHECKING:
    from typing import Final


def test_start_quit_empty() -> None:
    """Start tina with the test database, send 'q', wait for it to end."""
    print()

    with tt_util.tina_with_test_db_in_tempd(empty_db=True) as (tina, db_contents):
        assert not db_contents

        cfgpath: Final = tina.home / ".tina"
        assert not cfgpath.is_symlink()
        assert not cfgpath.exists()

        tina.quit()

        assert not cfgpath.is_symlink()
        assert cfgpath.is_file()
        assert not cfgpath.read_text(encoding="UTF-8")

    assert not cfgpath.is_symlink()
    assert not cfgpath.exists()


def test_display_test_db() -> None:
    """Load the test database, check for its entries."""
    print()

    with tt_util.tina_with_test_db_in_tempd() as (tina, db_contents):
        pane_height_str: Final = tina.pane.height
        assert isinstance(pane_height_str, str)
        last_y: Final = int(pane_height_str) - 1

        def walk_recursive(items: list[db.TinaEntry]) -> None:
            """Recurse into the items one by one."""
            for item_idx, item in enumerate(items, start=1):
                # Just in case...
                tina.assert_text_at(0, item_idx, item.desc)

                print(f"Recursing into {item.desc!r}")
                tina.pane.send_keys("l", enter=None, literal=True)
                time.sleep(0.1)

                if not item.children:
                    print("No child entries to check")
                    tina.assert_no_text_at(0, 1)
                else:
                    print(f"About to check {len(item.children)} child entries")
                    walk_recursive(item.children)

                print(f"Coming back from {item.desc!r}")
                tina.pane.send_keys("h", enter=None)
                time.sleep(0.1)
                tina.assert_text_at(0, item_idx, item.desc)

                print(f"Going back down to {item.desc!r}")
                for _ in range(1, item_idx):
                    tina.pane.send_keys("j", enter=None)
                    time.sleep(0.1)
                    tina.assert_no_text_at(0, last_y)

                print("Proceeding to the next item")
                tina.pane.send_keys("j", enter=None)
                time.sleep(0.1)
                if item_idx + 1 <= len(items):
                    tina.assert_no_text_at(0, last_y)
                else:
                    tina.assert_text_at(0, last_y, "You are on the last item")

        walk_recursive(db_contents)
        tina.quit()


def test_search_change() -> None:
    """Descend into a category, search for an item, change its value, save the database."""
    print()

    with tt_util.tina_with_test_db_in_tempd() as (tina, orig_entries):
        db_path: Final = tina.home / ".tina"
        assert db_path.read_text(encoding="UTF-8") == t_util.TEST_DB.read_text(encoding="UTF-8")

        tina.pane.send_keys("Down", enter=False)
        tina.pane.send_keys("Right", enter=False)

        tina.pane.send_keys("/even look", enter=True, literal=True)
        tina.pane.send_keys("c", enter=False, literal=True)
        tina.pane.send_keys("End", enter=False)
        tina.pane.send_keys(" and stuff", enter=True, literal=True)

        tina.pane.send_keys("$", enter=None, literal=True)
        assert db_path.read_text(encoding="UTF-8") != t_util.TEST_DB.read_text(encoding="UTF-8")

        tina.quit()

        entries: Final = parse.loads(db_path.read_text(encoding="UTF-8"))
        assert entries != orig_entries

        orig_top: Final = orig_entries[1]
        orig_child: Final = orig_top.children[-1]
        assert entries == [
            orig_entries[0],
            dataclasses.replace(
                orig_top,
                children=[
                    *orig_top.children[:-1],
                    dataclasses.replace(orig_child, desc=orig_child.desc + " and stuff"),
                ],
            ),
            *orig_entries[2:],
        ]


def test_insert_categorize() -> None:
    """Add an item, check for it, categorize it, make sure the new category goes last."""
    print()

    with tt_util.tina_with_test_db_in_tempd() as (tina, orig_entries):
        db_path: Final = tina.home / ".tina"
        assert db_path.read_text(encoding="UTF-8") == t_util.TEST_DB.read_text(encoding="UTF-8")

        print("Adding the 'nothing' entry")
        tina.pane.send_keys("GOnothing", enter=True, literal=True)
        assert db_path.read_text(encoding="UTF-8") == t_util.TEST_DB.read_text(encoding="UTF-8")

        print("Saving the 'nothing' entry")
        tina.pane.send_keys("$", enter=False, literal=True)
        assert db_path.read_text(encoding="UTF-8") != t_util.TEST_DB.read_text(encoding="UTF-8")
        print("Parsing the database after the 'nothing' entry was added")
        contents_nothing: Final = db_path.read_text(encoding="UTF-8")
        print(contents_nothing)
        mid: Final = parse.loads(contents_nothing)
        id_nothing: Final = mid[-2].id
        entry_nothing: Final = db.TinaEntry(id=id_nothing, desc="nothing", children=[])
        assert mid == [*orig_entries[:-1], entry_nothing, orig_entries[-1]]

        print("Setting the category of the 'nothing' entry to 'something'")
        tina.pane.send_keys("Csomething", enter=True, literal=True)
        print("Telling 'tina' to quit, hopefully it will save the database")
        tina.quit()

        print("Parsing the database after the 'something' entry was added")
        contents_something: Final = db_path.read_text(encoding="UTF-8")
        print(contents_something)
        done: Final = parse.loads(contents_something)
        id_something: Final = done[-1].id
        entry_something: Final = db.TinaEntry(
            id=id_something,
            desc="something",
            children=[entry_nothing],
        )
        assert done == [*orig_entries, entry_something]
