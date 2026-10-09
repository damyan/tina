// SPDX-FileCopyrightText: Peter Pentchev
// SPDX-License-Identifier: GPL-2.0-or-later
//! Handlers for the various database representation formats.
//!
//! The [`FormatHandler`] trait describes a handler for a single database
//! format, e.g. the classic tina database, JSON, YAML, etc.
//!
//! The [`MAP`] hashmap contains all the currently supported handlers.
use std::collections::HashMap;
use std::fmt::Debug;

use once_cell::sync::Lazy;

use crate::db::TinaEntry;
use crate::defs::{DbFormat, Error};

mod json;
mod tina;
mod yaml;

use json::JSON_HANDLER;
use tina::TINA_HANDLER;
use yaml::YAML_HANDLER;

/// Parse and generate a representation of the tina database.
pub trait FormatHandler: Debug + Sync {
    /// Parse a file contents to an in-memory list of top-level entries.
    ///
    /// # Errors
    ///
    /// See the format-specific implementations for details.
    fn decode(&self, contents: &str) -> Result<Vec<TinaEntry>, Error>;

    /// Serialize the in-memory database.
    ///
    /// # Errors
    ///
    /// See the format-specific implementations for details.
    fn encode(&self, value: &[TinaEntry]) -> Result<String, Error>;
}

/// The currently supported format conversion handlers.
pub static MAP: Lazy<HashMap<DbFormat, &'static dyn FormatHandler>> = Lazy::new(|| {
    let values: [(DbFormat, &dyn FormatHandler); 3] = [
        (DbFormat::Tina, &TINA_HANDLER),
        (DbFormat::Json, &JSON_HANDLER),
        (DbFormat::Yaml, &YAML_HANDLER),
    ];
    values.into()
});
