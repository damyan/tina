# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Parse and generate the native tina format."""

from __future__ import annotations

import itertools
import typing

from tina_mgr import parse

from . import f_base


if typing.TYPE_CHECKING:
    from typing import Final

    from tina_mgr import db


class TinaFormatHandler(f_base.FormatHandler):
    """Encode and decode Tina database entries."""

    @classmethod
    def _do_dumps(cls, entries: list[db.TinaEntry]) -> str:
        """Format Tina database entries."""

        def dump_single(entry: db.TinaEntry, category: str | None) -> str:
            """Dump a single entry into a multiline string."""
            cat: Final = f"Category: {category}\n" if category is not None else ""
            return f"Item-ID: {entry.id}\nDescription: {entry.desc}\n{cat}"

        def dump_single_rec(entry: db.TinaEntry, category: str | None) -> list[str]:
            """Dump a single entry and all its descendants."""
            return [
                dump_single(entry, category),
                *itertools.chain(*(dump_single_rec(child, entry.id) for child in entry.children)),
            ]

        return "\n".join(itertools.chain(*(dump_single_rec(entry, None) for entry in entries)))

    @classmethod
    def _do_loads(cls, contents: str) -> list[db.TinaEntry]:
        """Parse Tina database entries."""
        return parse.loads(contents)
