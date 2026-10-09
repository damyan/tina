// SPDX-FileCopyrightText: Peter Pentchev
// SPDX-License-Identifier: GPL-2.0-or-later
//! Data structures used in the various database representation formats.

use std::collections::HashSet;

use serde_derive::{Deserialize, Serialize};

use crate::defs::Error;

/// A classic representation of a Tina database entry.
///
/// This is the structure used when parsing or generating the tina
/// database itself.
#[derive(Debug, Clone)]
pub struct TinaEntry {
    /// The internal tina entry ID.
    item_id: String,

    /// The text of the entry as displayed.
    description: String,

    /// The internal tina ID of the parent entry, if any.
    category: Option<String>,

    /// Entries one level deep that have this one as a parent.
    children: Vec<Self>,
}

impl TinaEntry {
    /// The internal tina entry ID.
    #[inline]
    #[must_use]
    pub fn item_id(&self) -> &str {
        &self.item_id
    }

    /// The text of the entry as displayed.
    #[inline]
    #[must_use]
    pub fn description(&self) -> &str {
        &self.description
    }

    /// The internal tina ID of the parent entry, if any.
    #[inline]
    #[must_use]
    pub fn category(&self) -> Option<&str> {
        self.category.as_deref()
    }

    /// Entries one level deep that have this one as a parent.
    #[inline]
    #[must_use]
    pub fn children(&self) -> &[Self] {
        &self.children
    }

    /// Construct a [`TinaEntry`] object with the specified parameters.
    #[inline]
    #[must_use]
    pub const fn new(
        item_id: String,
        description: String,
        category: Option<String>,
        children: Vec<Self>,
    ) -> Self {
        Self {
            item_id,
            description,
            category,
            children,
        }
    }

    /// Add a [`TinaEntry`] to the list of child entries.
    #[inline]
    pub fn add_child(&mut self, item: Self) {
        self.children.push(item);
    }

    /// Get a reference to the child entry with the specified item ID.
    #[inline]
    #[must_use]
    pub fn get_child(&self, child_id: &str) -> Option<&Self> {
        self.children.iter().find(|child| child.item_id == child_id)
    }

    /// Follow a path, child entry by child entry.
    ///
    /// # Errors
    ///
    /// [`Error::PathNoChild`] if a path element could not be found.
    #[inline]
    pub fn follow_path<S: AsRef<str>>(&self, path: &[S]) -> Result<&Self, Error> {
        path.iter().try_fold(self, |current, child_id_ref| {
            let child_id = child_id_ref.as_ref();
            current
                .get_child(child_id)
                .ok_or_else(|| Error::PathNoChild(child_id.to_owned(), current.item_id.clone()))
        })
    }

    /// Get a mutable reference to the child entry with the specified item ID.
    #[inline]
    #[must_use]
    pub fn get_child_mut(&mut self, child_id: &str) -> Option<&mut Self> {
        self.children
            .iter_mut()
            .find(|child| child.item_id == child_id)
    }

    /// Follow a path, child entry by child entry, and return a mutable reference.
    ///
    /// # Errors
    ///
    /// [`Error::PathNoChild`] if a path element could not be found.
    #[inline]
    pub fn follow_path_mut<S: AsRef<str>>(&mut self, path: &[S]) -> Result<&mut Self, Error> {
        path.iter().try_fold(self, |current, child_id_ref| {
            let child_id = child_id_ref.as_ref();
            let item_id = current.item_id().to_owned();
            current
                .get_child_mut(child_id)
                .ok_or_else(|| Error::PathNoChild(child_id.to_owned(), item_id))
        })
    }
}

/// A recursive representation of the Tina database entries.
///
/// This is the structure used in the JSON and YAML formats.
#[derive(Deserialize, Serialize)]
pub struct SEntry {
    /// The internal tina entry ID.
    id: String,

    /// The text of the entry as displayed.
    desc: String,

    /// Entries one level deep that have this one as a parent.
    children: Vec<SEntry>,
}

/// The version of the tina database representation format.
#[derive(Deserialize, Serialize)]
pub struct FormatVersion {
    /// The major version number.
    major: u32,

    /// The minor version number.
    minor: u32,
}

impl FormatVersion {
    /// The major version number.
    #[inline]
    #[must_use]
    pub const fn major(&self) -> u32 {
        self.major
    }

    /// The minor version number.
    #[inline]
    #[must_use]
    pub const fn minor(&self) -> u32 {
        self.minor
    }
}

/// The format metadata of the tina database representation.
#[derive(Deserialize, Serialize)]
pub struct Format {
    /// The version of the tina database representation format.
    version: FormatVersion,
}

impl Format {
    /// The version of the tina database representation format.
    #[inline]
    #[must_use]
    pub const fn version(&self) -> &FormatVersion {
        &self.version
    }
}

/// Just the format metadata, useful for checking the format version.
#[derive(Deserialize, Serialize)]
pub struct TopFormatOnly {
    /// The format metadata of the tina database representation.
    format: Format,
}

impl TopFormatOnly {
    /// The format metadata of the tina database representation.
    #[inline]
    #[must_use]
    pub const fn format(&self) -> &Format {
        &self.format
    }
}

/// The top-level structure of the serialized Tina database entries.
#[derive(Deserialize, Serialize)]
pub struct Top {
    /// The format metadata of the tina database representation.
    format: Format,

    /// The tina database entries.
    tina: Vec<SEntry>,
}

impl Top {
    /// The format metadata of the tina database representation.
    #[inline]
    #[must_use]
    pub const fn format(&self) -> &Format {
        &self.format
    }

    /// The tina database entries.
    #[inline]
    #[must_use]
    pub fn tina(&self) -> &[SEntry] {
        &self.tina
    }
}

/// Parse a list of recursive [`SEntry`] nodes into a recursive list of [`TinaEntry`] objects.
#[must_use]
#[allow(clippy::missing_inline_in_public_items)]
pub fn from_entries(value: &[SEntry], cat: Option<&str>) -> Vec<TinaEntry> {
    value
        .iter()
        .map(|entry| TinaEntry {
            item_id: entry.id.clone(),
            description: entry.desc.clone(),
            category: cat.map(ToOwned::to_owned),
            children: from_entries(&entry.children, Some(&entry.id)),
        })
        .collect()
}

/// Prepare a list of [`TinaEntry`] objects for serializing.
#[must_use]
#[allow(clippy::missing_inline_in_public_items)]
pub fn to_entries(value: &[TinaEntry]) -> Vec<SEntry> {
    value
        .iter()
        .map(|tree| SEntry {
            id: tree.item_id.clone(),
            desc: tree.description.clone(),
            children: to_entries(&tree.children),
        })
        .collect()
}

/// Build the full serialization structure.
#[inline]
#[must_use]
pub fn serialize(value: &[TinaEntry]) -> Top {
    Top {
        format: Format {
            version: FormatVersion { major: 1, minor: 0 },
        },

        tina: to_entries(value),
    }
}

/// Parse the full serialized structure.
#[inline]
#[must_use]
pub fn deserialize(value: &Top) -> Vec<TinaEntry> {
    from_entries(&value.tina, None)
}

/// Check a list of entries and their subtrees for duplicate IDs, update the ID set.
///
/// # Errors
///
/// [`Error::Duplicate`] on duplicate IDs.
fn check_for_duplicates_rec<'data>(
    entry: &'data TinaEntry,
    mut ids: HashSet<&'data str>,
) -> Result<HashSet<&'data str>, Error> {
    let id = entry.item_id();
    if ids.contains(&id) {
        return Err(Error::Duplicate(id.to_owned()));
    }
    ids.insert(id);

    entry
        .children()
        .iter()
        .try_fold(ids, |acc, child| check_for_duplicates_rec(child, acc))
}

/// Check a list of entries and their subtrees for duplicate IDs.
///
/// # Errors
///
/// [`Error::Duplicate`] on duplicate IDs.
#[allow(clippy::missing_inline_in_public_items)]
pub fn check_for_duplicates(entries: &[TinaEntry]) -> Result<(), Error> {
    entries
        .iter()
        .try_fold(HashSet::new(), |acc, entry| {
            check_for_duplicates_rec(entry, acc)
        })
        .map(|_ids| ())
}
