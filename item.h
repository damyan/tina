/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TINA_ITEM_H
#define TINA_ITEM_H

#include <sys/types.h>

struct item
{
  char *identifier;
  char *description;
  char **categories;
  size_t ncategories;
};

/* Create a new item.  */
struct item *item_new (void);
/* Create a new item, and set its description to DESCRIPTION.  */
struct item *item_new_with_description (const char *description);
/* Create a copy of IT.  */
struct item *item_clone (struct item *it);
/* Delete IT.  */
void item_delete (struct item *it);

/* Generate a new ID for the item. */
void item_regenerate_id (struct item *it);

/* Set the identifier of IT to IDENTIFIER.  */
void item_identifier_set (struct item *it, const char *identifier);
/* Set the description of IT to DESCRIPTION.  */
void item_description_set (struct item *it, const char *description);
/* Add IT to CATEGORY.  */
void item_category_add (struct item *it, const char *category);
/* Remove IT from CATEGORY.  */
void item_category_remove (struct item *it, const char *category);
/* Return nonzero iff IT is a member of CATEGORY.  */
int item_category_member_p (struct item *it, const char *category);

#endif /* TINA_ITEM_H */
