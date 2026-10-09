// SPDX-FileCopyrightText: Peter Pentchev
// SPDX-License-Identifier: GPL-2.0-or-later
//! Parse and generate a YAML representation of the tina database.

// It's okay for the data structures to be named descriptively so that
// they can be e.g. directly reexported.
#![allow(clippy::module_name_repetitions)]

use anyhow::Context as _;

use crate::convert::FormatHandler;
use crate::db::{self, TinaEntry, Top, TopFormatOnly};
use crate::defs::Error;

/// Parse and generate a YAML representation of the tina database.
#[derive(Debug)]
pub struct YamlHandler;

impl FormatHandler for YamlHandler {
    fn decode(&self, contents: &str) -> Result<Vec<TinaEntry>, Error> {
        {
            let top_fmt_only = serde_yaml::from_str::<TopFormatOnly>(contents)
                .context("Could not parse the YAML format metadata")
                .map_err(Error::Decode)?;
            let vers = top_fmt_only.format().version();
            if vers.major() != 1 {
                return Err(Error::UnsupportedVersion(vers.major(), vers.minor()));
            }
        }

        let top = serde_yaml::from_str::<Top>(contents)
            .context("Could not parse the YAML document")
            .map_err(Error::Decode)?;
        let res = db::deserialize(&top);
        db::check_for_duplicates(&res)?;
        Ok(res)
    }

    fn encode(&self, value: &[TinaEntry]) -> Result<String, Error> {
        db::check_for_duplicates(value)?;
        serde_yaml::to_string(&db::serialize(value))
            .context("Could not format TinaEntry objects as a YAML string")
            .map_err(Error::Encode)
    }
}

/// A singleton object used for encoding and decoding YAML databases.
pub static YAML_HANDLER: YamlHandler = YamlHandler {};
