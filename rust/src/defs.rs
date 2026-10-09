// SPDX-FileCopyrightText: Peter Pentchev
// SPDX-License-Identifier: GPL-2.0-or-later
//! Common definitions for the tina file handling routines.

use std::fmt::{Display, Formatter, Result as FmtResult};
use std::str::FromStr;

use anyhow::Error as AnyError;
use thiserror::Error;

/// Errors that may occur during the file processing.
#[derive(Debug, Error)]
#[allow(clippy::error_impl_error)]
#[non_exhaustive]
pub enum Error {
    /// Could not deserialize an entry.
    #[error("Could not deserialize a tina database entry")]
    Decode(#[source] AnyError),

    /// Items with the same ID detected.
    #[error("Items with duplicate ID '{0}' detected")]
    Duplicate(String),

    /// Could not serialize an entry.
    #[error("Could not serialize a tina database entry")]
    Encode(#[source] AnyError),

    /// Something went really, really wrong...
    #[error("tina internal error")]
    Internal(#[source] AnyError),

    /// There is a loop in the list of Tina database items.
    #[error("One or more loops with a total of {0} items detected")]
    Loop(usize),

    /// An empty path was specified for diving into a structure.
    #[error("Empty path")]
    PathEmpty,

    /// An unknown child was specified within the path.
    #[error("No child entry '{0}' for '{1}'")]
    PathNoChild(String, String),

    /// An unknown first element was specified for a path.
    #[error("No first item '{0}'")]
    PathNoFirst(String),

    /// An invalid name was specified for the input or output format.
    #[error("Unknown file format '{0}'")]
    UnknownFormat(String),

    /// An attempt to add an item to an unknown parent.
    #[error("Unknown parent '{0}' for the '{1}' item")]
    UnknownParent(String, String),

    /// An attempt to deserialize an unsupported version of the Tina database.
    #[error("Unsupported Tina format version {0}.{1}")]
    UnsupportedVersion(u32, u32),
}

/// The supported input/output formats.
#[derive(Debug, Clone, Copy, Hash, PartialEq, Eq)]
#[non_exhaustive]
pub enum DbFormat {
    /// A hierarchical JSON representation.
    Json,

    /// The tina native format.
    Tina,

    /// A hierarchical YAML representation.
    Yaml,
}

impl DbFormat {
    /// A hierarchical JSON representation.
    const JSON: &str = "json";

    /// The tina native format.
    const TINA: &str = "tina";

    /// A hierarchical YAML representation.
    const YAML: &str = "yaml";
}

impl AsRef<str> for DbFormat {
    #[inline]
    fn as_ref(&self) -> &str {
        match *self {
            Self::Json => Self::JSON,
            Self::Tina => Self::TINA,
            Self::Yaml => Self::YAML,
        }
    }
}

impl Display for DbFormat {
    #[inline]
    #[allow(clippy::min_ident_chars)]
    fn fmt(&self, f: &mut Formatter<'_>) -> FmtResult {
        write!(f, "{value}", value = self.as_ref())
    }
}

impl FromStr for DbFormat {
    type Err = Error;

    #[inline]
    fn from_str(value: &str) -> Result<Self, Self::Err> {
        match value {
            Self::JSON => Ok(Self::Json),
            Self::TINA => Ok(Self::Tina),
            Self::YAML => Ok(Self::Yaml),
            other => Err(Error::UnknownFormat(other.to_owned())),
        }
    }
}
