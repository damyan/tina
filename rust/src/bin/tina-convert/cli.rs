// SPDX-FileCopyrightText: Peter Pentchev
// SPDX-License-Identifier: GPL-2.0-or-later
//! Parse the tina-convert command-line options and subcommands.

use std::io;

use anyhow::{bail, Context as _, Result};
use clap::error::ErrorKind as ClapErrorKind;
use clap::Parser as _;
use clap_derive::{Parser, Subcommand};
use tracing::Level;

use tina_mgr::defs::DbFormat;

use crate::convert;
use crate::defs::Config;

/// What do display?
#[derive(Debug, Subcommand)]
enum CliListCommand {
    /// Display information about supported formats.
    Formats,
}

/// What to do, what to do?
#[derive(Debug, Subcommand)]
enum CliCommand {
    /// Convert a tina database from one format to another.
    Convert {
        /// The input file format.
        #[clap(short('I'), default_value("tina"))]
        infmt: DbFormat,

        /// The output file format.
        #[clap(short('O'), default_value("tina"))]
        outfmt: DbFormat,

        /// The output file or "-" for the standard output stream.
        #[clap(short('o'), allow_hyphen_values(true))]
        target: Option<String>,

        /// The source file or "-" for the standard input stream.
        source: String,
    },

    /// Display information about supported features.
    List {
        /// What to do?
        #[clap(subcommand)]
        cmd: CliListCommand,
    },
}

/// Convert a tina database from one format to another.
#[derive(Debug, Parser)]
#[clap(version)]
struct Cli {
    /// Debug mode; display even more diagnostic output.
    #[clap(short, long)]
    debug: bool,

    /// Verbose operation; display diagnostic output.
    #[clap(short, long)]
    verbose: bool,

    /// What to do?
    #[clap(subcommand)]
    cmd: CliCommand,
}

/// What to do, what to do?
#[derive(Debug)]
pub enum Mode<'data> {
    /// Display a help or version message; handled by the command-line parser.
    Handled,

    /// Convert a tina database from one format to another.
    Convert(Config<'data>),

    /// List the supported input/output formats.
    ListFormats,
}

/// Initialize the logging subsystem provided by the `tracing` library.
fn setup_tracing(verbose: bool, debug: bool) -> Result<()> {
    let sub = tracing_subscriber::fmt()
        .without_time()
        .with_max_level(if debug {
            Level::TRACE
        } else if verbose {
            Level::DEBUG
        } else {
            Level::INFO
        })
        .with_writer(io::stderr)
        .finish();
    #[allow(clippy::absolute_paths)]
    tracing::subscriber::set_global_default(sub).context("Could not initialize the tracing logger")
}

/// Parse the command-line options and subcommands.
///
/// # Errors
///
/// Propagate command-line parsing errors.
pub fn try_parse() -> Result<Mode<'static>> {
    let args = match Cli::try_parse() {
        Ok(args) => args,
        Err(err)
            if matches!(
                err.kind(),
                ClapErrorKind::DisplayHelp | ClapErrorKind::DisplayVersion
            ) =>
        {
            err.print()
                .context("Could not display the usage or version message")?;
            return Ok(Mode::Handled);
        }
        Err(err) if err.kind() == ClapErrorKind::DisplayHelpOnMissingArgumentOrSubcommand => {
            err.print()
                .context("Could not display the usage or version message")?;
            bail!("Invalid or missing command-line options");
        }
        Err(err) => return Err(err).context("Could not parse the command-line options"),
    };
    setup_tracing(args.verbose, args.debug)
        .context("Could not initialize the logging infrastructure")?;
    match args.cmd {
        CliCommand::Convert {
            infmt,
            outfmt,
            target,
            source,
        } => Ok(Mode::Convert(Config {
            source,
            target,
            infmt: *convert::MAP
                .get(&infmt)
                .with_context(|| format!("Internal error: no '{infmt}' handler"))?,
            outfmt: *convert::MAP
                .get(&outfmt)
                .with_context(|| format!("Internal error: no '{outfmt}' handler"))?,
        })),

        CliCommand::List { cmd } => match cmd {
            CliListCommand::Formats => Ok(Mode::ListFormats),
        },
    }
}
