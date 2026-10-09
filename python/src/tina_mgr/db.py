# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""The native format of the Tina database."""

from __future__ import annotations

import dataclasses


@dataclasses.dataclass(frozen=True)
class TinaEntry:
    """A database entry with the tree of child entries."""

    id: str
    """The unique ID of this entry within the Tina database."""

    desc: str
    """The text of the entry itself."""

    children: list[TinaEntry]
    """The child entries as an ordered list."""
