// SPDX-FileCopyrightText: Peter Pentchev
// SPDX-License-Identifier: GPL-2.0-or-later
//! Parse and generate a classic text line-based tina database structure.

// It's okay for the data structures to be named descriptively so that
// they can be e.g. directly reexported.
#![allow(clippy::module_name_repetitions)]

use std::borrow::ToOwned;
use std::collections::HashMap;

use anyhow::{anyhow, Context as _};
use itertools::{FoldWhile, Itertools as _};
use nom::{
    bytes::complete::tag,
    character::complete::none_of,
    combinator::{all_consuming, opt},
    multi::{many0, many1, separated_list0},
    sequence::tuple,
    IResult,
};

use crate::convert::FormatHandler;
use crate::db::{self, TinaEntry};
use crate::defs::Error;

/// Parse and generate a classic text line-based tina database structure.
#[derive(Debug)]
pub struct TinaHandler;

/// Parse the internal tina ID of a single item.
///
/// # Errors
///
/// None by itself, propagates parsing errors.
fn p_item_id(input: &str) -> IResult<&str, String> {
    let (r_input, (prefix, id, suffix)) =
        tuple((tag("<"), many1(none_of(">\n")), tag(">")))(input)?;
    Ok((
        r_input,
        format!(
            "{prefix}{collected}{suffix}",
            collected = id.into_iter().collect::<String>()
        ),
    ))
}

/// Parse a single item in a classic tina database file.
///
/// # Errors
///
/// None by itself, propagates parsing errors, e.g. unrecognized line.
fn p_item(input: &str) -> IResult<&str, TinaEntry> {
    let (r_input, (_, item_id, _, _, description, _, category)) = tuple((
        tag("Item-ID: "),
        p_item_id,
        tag("\n"),
        tag("Description: "),
        many0(none_of("\n")),
        tag("\n"),
        opt(tuple((tag("Category: "), p_item_id, tag("\n")))),
    ))(input)?;
    Ok((
        r_input,
        TinaEntry::new(
            item_id,
            description.into_iter().collect(),
            category.map(|(_, cat, _)| cat),
            vec![],
        ),
    ))
}

/// Parse a classic tina database file into a list of [`TinaEntry`] objects.
///
/// Note that this parser does not attempt to create the tree structure of
/// the objects; they are all returned as a flat list.
///
/// # Errors
///
/// None by itself, propagates parsing errors.
fn p_tina(input: &str) -> IResult<&str, Vec<TinaEntry>> {
    let (r_input, items) = all_consuming(separated_list0(tag("\n"), p_item))(input)?;
    Ok((r_input, items))
}

/// The recursive items parsed successfully so far.
type AccItems = Vec<TinaEntry>;

/// The path (item IDs) to each parsed item starting from the top.
type AccPathsById = HashMap<String, Vec<String>>;

/// The items that cannot be added yet, since we don't have their parent yet.
type AccOrphanItems = Vec<TinaEntry>;

/// The whole thing passed across iterations.
type Acc = (AccItems, AccPathsById, AccOrphanItems);

/// Dive down into a slice of child items, get a mutable reference to the specified one.
fn get_mut_by_path_in_vec<'items>(
    children: &'items mut [TinaEntry],
    child_id: &'_ String,
) -> Option<&'items mut TinaEntry> {
    children
        .iter_mut()
        .find(|child| child.item_id() == *child_id)
}

/// Get a mutable reference to the item specified by the path.
fn get_mut_by_path<'items>(
    items: &'items mut AccItems,
    path: &'_ [String],
) -> Result<&'items mut TinaEntry, Error> {
    let (first_id, path_rest) = path.split_first().ok_or(Error::PathEmpty)?;
    let first = get_mut_by_path_in_vec(items, first_id)
        .ok_or_else(|| Error::PathNoFirst((*first_id).clone()))?;
    first.follow_path_mut(path_rest)
}

/// Add a single item into the tree, either at the end, or as a child of another one.
///
/// # Errors
///
/// [`Error::PathNoFirst`], [`Error::PathNoChild`], [`Error::PathEmpty`] if
/// the paths item of the [`acc`] parameter contains an invalid value.
fn add_item(acc: Acc, item: TinaEntry) -> Result<Acc, Error> {
    let (mut items, mut paths, mut orphans) = acc;
    let item_id = item.item_id().to_owned();

    if let Some(category) = item.category() {
        match paths.get(category) {
            Some(parent_path) => {
                let parent = get_mut_by_path(&mut items, parent_path)?;
                let new_path: Vec<String> = {
                    let mut new_path = parent_path.clone();
                    new_path.push(item_id.clone());
                    new_path
                };

                paths.insert(item_id, new_path);
                parent.add_child(item);
            }

            None => {
                orphans.push(item);
            }
        }
    } else {
        paths.insert(item_id.clone(), vec![item_id]);
        items.push(item);
    }
    Ok((items, paths, orphans))
}

/// Add items in iterations, keeping track of orphaned items along the way.
///
/// # Errors
///
/// [`Error::Duplicate`] if there are items with the same ID.
/// [`Error::Loop`] if there is a loop in the list of raw items.
/// [`Error::PathNoFirst`], [`Error::PathNoChild`], [`Error::PathEmpty`],
/// [`Error::Internal`] on internal errors.
fn add_items_loop(flat_items: Vec<TinaEntry>) -> Result<Vec<TinaEntry>, Error> {
    let acc_init = (Vec::new(), HashMap::new(), flat_items);
    match (0..).fold_while(
        Ok(acc_init),
        |acc_res: Result<Acc, Error>, _idx| -> FoldWhile<Result<Acc, Error>> {
            match acc_res {
                Ok((int_res, int_paths, remaining_items)) => {
                    let start_len = remaining_items.len();
                    match remaining_items
                        .into_iter()
                        .try_fold((int_res, int_paths, Vec::new()), add_item)
                    {
                        Ok((res, paths, orphans)) if orphans.is_empty() => {
                            FoldWhile::Done(Ok((res, paths, orphans)))
                        }
                        Ok((_res, _paths, orphans)) if orphans.len() == start_len => {
                            FoldWhile::Done(Err(Error::Loop(start_len)))
                        }
                        Ok((res, paths, orphans)) => FoldWhile::Continue(Ok((res, paths, orphans))),
                        Err(err) => FoldWhile::Done(Err(err)),
                    }
                }
                Err(err) => FoldWhile::Done(Err(err)),
            }
        },
    ) {
        FoldWhile::Continue(res) => Err(Error::Internal(anyhow!(
            "add_items_loop: FoldWhile::Continue: {res:?}"
        ))),
        FoldWhile::Done(Err(err)) => Err(err),
        FoldWhile::Done(Ok((items, _, _))) => Ok(items),
    }
}

impl FormatHandler for TinaHandler {
    fn decode(&self, contents: &str) -> Result<Vec<TinaEntry>, Error> {
        let (_, flat_items) = p_tina(contents)
            .map_err(|err| err.map_input(ToOwned::to_owned))
            .context("Could not parse the tina database file")
            .map_err(Error::Decode)?;
        let res = add_items_loop(flat_items)?;
        db::check_for_duplicates(&res)?;
        Ok(res)
    }

    fn encode(&self, value: &[TinaEntry]) -> Result<String, Error> {
        db::check_for_duplicates(value)?;
        let res: Vec<String> = value
            .iter()
            .map(|tree| -> Result<String, Error> {
                let header = format!(
                    "Item-ID: {item_id}\nDescription: {description}\n{category}",
                    item_id = tree.item_id(),
                    description = tree.description(),
                    category = tree
                        .category()
                        .as_ref()
                        .map_or_else(String::new, |cat| format!("Category: {cat}\n")),
                );
                Ok(if tree.children().is_empty() {
                    header
                } else {
                    format!(
                        "{header}\n{children}",
                        children = self.encode(tree.children())?
                    )
                })
            })
            .collect::<Result<_, _>>()?;
        Ok(res.join("\n"))
    }
}

/// A singleton object used for encoding and decoding the classic tina format.
pub static TINA_HANDLER: TinaHandler = TinaHandler {};
