/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TINA_DATABASE_H
#define TINA_DATABASE_H

#include <sys/types.h>

#include "item.h"

struct database
{
  char *path;

  struct item **items;
  size_t nitems;

  unsigned readonly:1;
};

/* Create a database whose backing store is PATH.  */
struct database *database_new_with_path (const char *path);
/* Write DB to its backing store.  */
void database_sync (struct database *db);
/* Write DB to its backing store and delete.  */
void database_delete (struct database *db);

/* Add IT to DB at POS.  */
void database_item_add (struct database *db, struct item *it, int pos);
/* Remove the item at POS from DB.  */
void database_item_remove (struct database *db, int pos);
/* Return the position of IT in DB->items, or DB->nitems if it is not
   present.  */
size_t database_item_index (struct database *db, struct item *it);
/* Return the position of an item with the identifier ID in DB->items, or
   DB->nitems if it is not present.  */
size_t database_item_index_by_id (struct database *db, const char *id);

#endif /* TINA_DATABASE_H */
