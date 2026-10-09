# SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
# SPDX-License-Identifier: GPL-2.0-or-later
"""Convert the Tina database from one format to another."""

from __future__ import annotations

import pathlib
import sys
import typing

import click

from tina_mgr import defs as t_defs

from . import formats


if typing.TYPE_CHECKING:
    from typing import Final


def arg_version(_ctx: click.Context, _self: click.Parameter, value: bool) -> bool:  # noqa: FBT001
    """Display program version information."""
    if not value:
        return value

    print(f"tina-convert {t_defs.VERSION}")  # noqa: T201
    sys.exit(0)


@click.command(name="convert")
@click.option(
    "-I",
    "infmt",
    type=str,
    default="tina",
    help="The input file format",
)
@click.option(
    "-O",
    "outfmt",
    type=str,
    default="tina",
    help="The output file format",
)
@click.option(
    "-o",
    "target",
    type=str,
    default="-",
    help='The output file or "-" for the standard output stream',
)
@click.argument("source", type=str)
def cmd_convert(*, infmt: str, outfmt: str, source: str, target: str) -> None:  # noqa: C901
    """Convert a tina database from one format to another."""
    source_name: Final = "standard input" if source == "-" else source
    try:
        indata: Final = (
            sys.stdin.read() if source == "-" else pathlib.Path(source).read_text(encoding="UTF-8")
        )
    except OSError as err:
        sys.exit(f"tina-convert: Could not read from {source_name}: {err}")
    except ValueError as err:
        sys.exit(f"tina-convert: Could not parse {source_name} as UTF-8 text: {err}")
    except Exception as err:  # noqa: BLE001
        sys.exit(
            f"tina-convert: Unexpected error while reading from {source_name}: "
            f"{type(err).__name__}: {err}",
        )
    try:
        entries: Final = formats.FORMATS[infmt].loads(indata)
    except t_defs.Error as err:
        sys.exit(f"tina-convert: Could not parse {source_name} as a Tina database: {err}")
    except Exception as err:  # noqa: BLE001
        sys.exit(
            f"tina-convert: Unexpected error while parsing {source_name}: "
            f"{type(err).__name__}: {err}",
        )
    try:
        outdata: Final = formats.FORMATS[outfmt].dumps(entries)
    except t_defs.Error as err:
        sys.exit(f"tina-convert: Could not format the output: {err}")
    except Exception as err:  # noqa: BLE001
        sys.exit(
            f"tina-convert: Unexpected error while formatting the output: "
            f"{type(err).__name__}: {err}",
        )
    if target == "-":
        print(outdata)  # noqa: T201
    else:
        try:
            pathlib.Path(target).write_text(outdata, encoding="UTF-8")
        except OSError as err:
            sys.exit(f"tina-convert: Could not write to {target}: {err}")
        except Exception as err:  # noqa: BLE001
            sys.exit(
                f"tina-convert: Unexpected error while writing to {target}: "
                f"{type(err).__name__}: {err}",
            )


@click.command(name="formats")
def cmd_list_formats() -> None:
    """Display information about supported formats."""
    print(f"Formats: {' '.join(formats.FORMATS)}")  # noqa: T201


@click.group(name="list")
def cmd_list() -> None:
    """Display information about supported features."""


cmd_list.add_command(cmd_list_formats)


@click.group(name="tina-convert")
@click.help_option("--help", "-h")
@click.option(
    "--version",
    "-V",
    is_flag=True,
    is_eager=True,
    callback=arg_version,
    help="display program version information and exit",
)
def main(*, version: bool) -> None:
    """Convert the Tina database from one format to another."""
    if version:
        raise RuntimeError(repr(version))


main.add_command(cmd_convert)
main.add_command(cmd_list)


if __name__ == "__main__":
    main()
