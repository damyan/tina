// SPDX-FileCopyrightText: Peter Pentchev
// SPDX-License-Identifier: GPL-2.0-or-later
//! Common definitions for the `tina-convert` command-line tool.

use tina_mgr::convert::FormatHandler;

/// Configuration settings for converting a tina database from one format to another.
#[derive(Debug)]
pub struct Config<'fmt> {
    /// The source file or "-" for the standard input stream.
    pub source: String,

    /// The output file or "-" for the standard output stream.
    pub target: Option<String>,

    /// The format handler used to parse the source file.
    pub infmt: &'fmt dyn FormatHandler,

    /// The format handler used to parse the output file.
    pub outfmt: &'fmt dyn FormatHandler,
}
