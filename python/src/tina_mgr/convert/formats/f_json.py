# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Provide a simplistic hierarchical structure, encode/decode as JSON."""

from __future__ import annotations

import dataclasses
import json
import typing

from . import f_base


if typing.TYPE_CHECKING:
    from typing import Final

    from tina_mgr import db


@dataclasses.dataclass
class JSONDeserializationError(f_base.DeserializationError):
    """An error that occurred while deserializing a JSON object."""

    def __str__(self) -> str:
        """Provide a human-readable description of the error."""
        return f"Could not deserialize a JSON object: {self.err}"


class JSONFormatHandler(f_base.FormatHandler):
    """Encode and decode Tina database entries in a hierarchical structure."""

    @classmethod
    def _do_dumps(cls, entries: list[db.TinaEntry]) -> str:
        """Format Tina database entries into the JSON structure."""
        return json.dumps(f_base.serialize(entries))

    @classmethod
    def _do_loads(cls, contents: str) -> list[db.TinaEntry]:
        """Parse the JSON structure into database entries."""
        try:
            top: Final = json.loads(contents)
        except ValueError as err:
            raise JSONDeserializationError(err) from err

        return f_base.deserialize(top)
