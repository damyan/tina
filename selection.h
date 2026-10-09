/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TINA_SELECTION_H
#define TINA_SELECTION_H

#include <sys/types.h>

#include "database.h"
#include "item.h"

struct selection
{
  struct database *db;
  char *category;

  struct item **items;
  size_t nitems;
};

/* Create a new selection.  */
struct selection *selection_new_with_database (struct database *db);
/* Delete S.  */
void selection_delete (struct selection *s);

/* Set the category of S to CATEGORY.  */
void selection_category_set (struct selection *s, const char *category);
/* Refresh S to reflect changes to the database.  */
void selection_refresh (struct selection *s);

/* Add IT to S at POS.  */
void selection_item_add (struct selection *s, struct item *it, size_t pos);
/* Remove the item at POS from S.  */
void selection_item_remove (struct selection *s, size_t pos);
/* Return the position of IT in S->items, or S->nitems if it is not present.  */
int selection_item_index (struct selection *s, struct item *it);

#endif /* TINA_SELECTION_H */
