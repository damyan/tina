#![deny(missing_docs)]
#![deny(clippy::missing_docs_in_private_items)]
// SPDX-FileCopyrightText: Peter Pentchev
// SPDX-License-Identifier: GPL-2.0-or-later
//! Convert a Tina database to/from other formats,
//!
//! Read a Tina database file and output the data in JSON or YAML format,
//! or read that representation and output a Tina database back.

use std::fs;
use std::io::{self, Read as _, Write as _};

use anyhow::{Context as _, Result};
use itertools::Itertools as _;

mod cli;
mod defs;

use cli::Mode;
use defs::Config;

use tina_mgr::convert;

/// Write the converted contents to a file or the standard output.
///
/// # Errors
///
/// Propagates I/O errors.
fn create_file(target: Option<&str>, contents: &str) -> Result<()> {
    let fname = target.unwrap_or("-");
    if fname == "-" {
        io::stdout().write_all(contents.as_bytes())
    } else {
        fs::write(fname, contents.as_bytes())
    }
    .context("Could not write to the output file")
}

/// Perform the requested conversion, write the output file.
///
/// # Errors
///
/// Propagates input parse errors.
/// Propagates output generation errors.
/// Propagates I/O errors.
fn convert(cfg: &Config<'_>, contents: &str) -> Result<()> {
    let db = cfg
        .infmt
        .decode(contents)
        .context("Could not parse the input file")?;
    create_file(cfg.target.as_deref(), &cfg.outfmt.encode(&db)?)
}

/// Read the specified input file or the standard input stream to a string.
///
/// # Errors
///
/// Propagates I/O errors.
fn read_file(cfg: &Config<'_>) -> Result<String> {
    if cfg.source == "-" {
        let mut contents = String::new();
        io::stdin()
            .read_to_string(&mut contents)
            .context("Could not read from the standard input stream")?;
        Ok(contents)
    } else {
        fs::read_to_string(&cfg.source).context("Could not read from the input file")
    }
}

/// List the supported input/output file formats.
#[allow(clippy::print_stdout)]
fn list_formats() {
    println!(
        "Formats: {formats}",
        formats = convert::MAP
            .keys()
            .map(AsRef::as_ref)
            .sorted_unstable()
            .join(" ")
    );
}

fn main() -> Result<()> {
    match cli::try_parse()? {
        Mode::Handled => Ok(()),
        Mode::Convert(cfg) => convert(&cfg, &read_file(&cfg)?),
        Mode::ListFormats => {
            list_formats();
            Ok(())
        }
    }
}
