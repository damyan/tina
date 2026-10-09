# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Parse the native Tina database format."""

from __future__ import annotations

import dataclasses
import itertools
import typing

import pyparsing as pyp

from . import db
from . import defs


if typing.TYPE_CHECKING:
    from typing import Any, Final


@dataclasses.dataclass(frozen=True)
class RawEntry:
    """A single entry as stored in the native Tina database."""

    id: str
    """The unique ID of this entry within the Tina database."""

    desc: str
    """The text of the entry itself."""

    category: str | None
    """The item ID of the parent entry, if any."""


@dataclasses.dataclass
class ParseError(defs.Error):
    """An error that occurred during the parsing of the Tina database."""

    element: str
    """The element that we tried to parse."""

    obj: Any
    """The object that `pyparsing` returned instead."""

    def __str__(self) -> str:
        """Provide a human-readable description of the error."""
        return f"Could not parse a {self.element} Tina element, got unexpected {self.obj!r}"


@dataclasses.dataclass
class DuplicatesError(defs.Error):
    """Duplicate entries detected."""

    item_id: str
    """The duplicate item ID detected."""

    def __str__(self) -> str:
        """Provide a human-readable description of the error."""
        return f"Items with duplicate ID {self.item_id!r} detected"


_p_id: Final = pyp.Literal("<") + pyp.CharsNotIn("> \t\n") + pyp.Literal(">")


@_p_id.set_parse_action
def _parse_id(tokens: pyp.ParseResults) -> str:
    """Parse an item ID string."""
    match tokens.as_list():
        case [str(first), str(second), str(third)] if first == "<" and third == ">":
            return f"{first}{second}{third}"

        case other:
            raise ParseError("id", other)


_p_item_id: Final = pyp.Literal("Item-ID: ").suppress() + _p_id + pyp.Char("\n").suppress()

_p_item_description: Final = (
    pyp.Literal("Description: ").suppress() + pyp.CharsNotIn("\n") + pyp.Char("\n").suppress()
)

_p_item_category: Final = pyp.Literal("Category: ").suppress() + _p_id + pyp.Char("\n").suppress()

_p_entry: Final = _p_item_id + _p_item_description + pyp.Opt(_p_item_category)


@_p_entry.set_parse_action
def _parse_entry(tokens: pyp.ParseResults) -> RawEntry:
    """Build up a [`RawEntry`] object."""
    match tokens.as_list():
        case [str(item_id), str(desc)]:
            return RawEntry(id=item_id, desc=desc, category=None)

        case [str(item_id), str(desc), str(category)]:
            return RawEntry(id=item_id, desc=desc, category=category)

        case other:
            raise ParseError("entry", other)


_p_entries: Final = pyp.Opt(pyp.DelimitedList(_p_entry, pyp.Char("\n")))

_p_entries_complete: Final = _p_entries.leave_whitespace()


def _get_reachable_entries(entries: list[db.TinaEntry]) -> set[str]:
    """Count the number of all reachable entries in the parsed database."""

    def single(entry: db.TinaEntry) -> set[str]:
        """Count the number of entries in this entry's subtree."""
        return {entry.id} | _get_reachable_entries(entry.children)

    return set(itertools.chain(*(single(entry) for entry in entries)))


def loads(contents: str) -> list[db.TinaEntry]:
    """Parse the contents of a Tina database file."""
    raw_entries: Final = _p_entries_complete.parse_string(contents, parse_all=True).as_list()
    entries: Final = {
        entry.id: db.TinaEntry(id=entry.id, desc=entry.desc, children=[]) for entry in raw_entries
    }

    for entry in raw_entries:
        if entry.category is not None:
            entries[entry.category].children.append(entries[entry.id])

    res: Final = [entries[entry.id] for entry in raw_entries if entry.category is None]
    unreachable: Final = sorted(set(entries) - _get_reachable_entries(res))
    if unreachable:
        raise RuntimeError(repr(unreachable))

    check_for_duplicates(res)
    return res


def check_for_duplicates(entries: list[db.TinaEntry]) -> None:
    """Make sure there are no items with the same IDs."""
    ids: set[str] = set()

    def check_tree(entry: db.TinaEntry) -> None:
        """Check a single entry and its children."""
        item_id: Final = entry.id
        if item_id in ids:
            raise DuplicatesError(item_id)
        ids.add(item_id)

        for child in entry.children:
            check_tree(child)

    for entry in entries:
        check_tree(entry)
