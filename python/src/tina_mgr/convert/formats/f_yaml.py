# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Provide a simplistic hierarchical structure, encode/decode as YAML."""

from __future__ import annotations

import dataclasses
import typing

import yaml

from . import f_base


if typing.TYPE_CHECKING:
    from typing import Final

    from tina_mgr import db


@dataclasses.dataclass
class YAMLDeserializationError(f_base.DeserializationError):
    """An error that occurred while deserializing a YAML object."""

    def __str__(self) -> str:
        """Provide a human-readable description of the error."""
        return f"Could not deserialize a YAML object: {self.err}"


class YAMLFormatHandler(f_base.FormatHandler):
    """Encode and decode Tina database entries in a hierarchical structure."""

    @classmethod
    def _do_dumps(cls, entries: list[db.TinaEntry]) -> str:
        """Format Tina database entries into the YAML structure."""
        return yaml.safe_dump(f_base.serialize(entries))

    @classmethod
    def _do_loads(cls, contents: str) -> list[db.TinaEntry]:
        """Parse the YAML structure into database entries."""
        try:
            top: Final = yaml.safe_load(contents)
        except yaml.YAMLError as err:
            raise YAMLDeserializationError(err) from err

        return f_base.deserialize(top)
