/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "selection.h"

struct selection *
selection_new_with_database (struct database *db)
{
  struct selection *s;

  s = xcalloc (1, sizeof (struct selection));
  s->db = db;
  selection_refresh (s);

  return s;
}

void
selection_delete (struct selection *s)
{
  free (s->category);
  free (s->items);
  free (s);
}

void
selection_refresh (struct selection *s)
{
  size_t i;

  free (s->items);
  s->items = NULL;
  s->nitems = 0;

  for (i = 0; i < s->db->nitems; i++)
    if ((s->category == NULL
	 && s->db->items[i]->ncategories == 0)
	|| (s->category != NULL
	    && (item_category_member_p (s->db->items[i], s->category)
		|| (s->category[0] == '!'
		    && ! item_category_member_p (s->db->items[i],
						 s->category + 1)))))
      {
	s->items = xrealloc (s->items,
			     sizeof (struct item *) * (s->nitems + 1));
	s->items[s->nitems++] = s->db->items[i];
      }
}

void
selection_category_set (struct selection *s, const char *category)
{
  free (s->category);
  s->category = category != NULL ? xstrdup (category) : NULL;

  selection_refresh (s);
}

int
selection_item_index (struct selection *s, struct item *it)
{
  size_t pos;

  for (pos = 0; pos < s->nitems; pos++)
    if (s->items[pos] == it)
      break;

  return pos;
}

void
selection_item_add (struct selection *s, struct item *it, size_t pos)
{
  if (s->nitems < pos)
    abort ();

  s->items = xrealloc (s->items, sizeof (struct item *) * (s->nitems + 1));
  memmove (s->items + pos + 1, s->items + pos,
	   sizeof (struct item *) * (s->nitems - pos));
  s->items[pos] = it;
  s->nitems++;

  if (s->category != NULL)
    {
      if (s->category[0] == '!' && item_category_member_p (it, s->category + 1))
	item_category_remove (it, s->category + 1);
      else if (! item_category_member_p (it, s->category))
	item_category_add (it, s->category);
    }
}

void
selection_item_remove (struct selection *s, size_t pos)
{
  if (s->nitems - 1 < pos)
    abort ();

  if (s->category != NULL)
    item_category_remove (s->items[pos], s->category);

  memmove (s->items + pos, s->items + pos + 1,
	   sizeof (struct item *) * (s->nitems - pos - 1));
  s->items = xrealloc (s->items, sizeof (struct item *) * (s->nitems - 1));
  s->nitems--;
}
