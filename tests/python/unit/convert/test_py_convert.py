# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Test the interoperability between `tina-convert` and the Python `tina_mgr` library."""

from __future__ import annotations

import typing

from tina_mgr import db
from tina_mgr import parse
from unit import util as t_util

from . import defs as tc_defs


if typing.TYPE_CHECKING:
    from typing import Any, Final


def raw_to_db(entries: list[dict[str, Any]]) -> list[db.TinaEntry]:
    """Convert the raw dict representation into a list of [`TinaEntry`] objects."""

    def raw_to_entry(entry: dict[str, Any]) -> db.TinaEntry:
        """Convert a single dict to a [`TinaEntry`] object."""
        return db.TinaEntry(
            id=entry["id"],
            desc=entry["desc"],
            children=raw_to_db(entry["children"]) if entry["children"] is not None else [],
        )

    return [raw_to_entry(entry) for entry in entries]


def test_read_db() -> None:
    """Read the test database in the native format, compare it to what we expect."""
    contents: Final = t_util.TEST_DB.read_text(encoding="UTF-8")
    test_entries: Final = parse.loads(contents)
    assert test_entries == raw_to_db(tc_defs.TEST_DATA)
