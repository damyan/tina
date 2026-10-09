# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""The format handlers."""

from __future__ import annotations

import typing


if typing.TYPE_CHECKING:
    from typing import Final


from . import f_base
from . import f_json
from . import f_tina
from . import f_yaml


FORMATS: Final[dict[str, type[f_base.FormatHandler]]] = {
    "json": f_json.JSONFormatHandler,
    "tina": f_tina.TinaFormatHandler,
    "yaml": f_yaml.YAMLFormatHandler,
}
"""The supported format handlers."""
