# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Common definitions for the format conversion tests."""

from __future__ import annotations

import typing


if typing.TYPE_CHECKING:
    from typing import Any, Final


TEST_DATA: Final[list[dict[str, Any]]] = [
    {"id": "<57866329.6b8b4567@straylight>", "desc": "First item, a simple one.", "children": []},
    {
        "id": "<57866333.327b23c6@straylight>",
        "desc": "Second item - this is a ``category!``",
        "children": [
            {
                "id": "<57866340.643c9869@straylight>",
                "desc": "...and it has a sub-item of sorts, doesn't it now?",
                "children": [
                    {
                        "id": "<57877504.2ae8944a@straylight>",
                        "desc": "Whee, a third-level item!",
                        "children": [],
                    },
                ],
            },
            {
                "id": "<57866349.66334873@straylight>",
                "desc": '# And some of them `even look" funny',
                "children": [],
            },
        ],
    },
    {
        "id": "<5786635d.74b0dc51@straylight>",
        "desc": "Let's have [one more](while we're) at it.",
        "children": [
            {
                "id": "<57866365.19495cff@straylight>",
                "desc": "And another sub-item, I guess.",
                "children": [],
            },
        ],
    },
]
"""The contents of the test database."""
