# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Common definitions for the tina-mgr library."""

from __future__ import annotations

import dataclasses
import typing


if typing.TYPE_CHECKING:
    from typing import Final


VERSION: Final = "0.1.14"
"""The version of tina-mgr, semver-like."""


@dataclasses.dataclass
class Error(Exception):
    """An error that occurred during the handling of the Tina files."""

    def __str__(self) -> str:
        """Provide a human-readable error message."""
        return f"tina files handling error: {self!r}"
