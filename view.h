/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TINA_VIEW_H
#define TINA_VIEW_H

#include "item.h"
#include "selection.h"

struct view
{
  struct view *prev;
  struct selection *s;

  int selected;
  char *search_pattern;
};

/* Create a new view, and set its selection to S.  */
struct view *view_new_with_selection (struct selection *s);
/* Delete V.  */
void view_delete (struct view *v);

/* Show V.  */
void view_show (struct view *v);

#endif /* TINA_VIEW_H */
