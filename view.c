#define _GNU_SOURCE
/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "curslib.h"
#include "memory.h"
#include "selection.h"
#include "view.h"

struct view *
view_new_with_selection (struct selection *s)
{
  struct view *v;

  v = xcalloc (1, sizeof (struct view));
  v->s = s;

  return v;
}

void
view_delete (struct view *v)
{
  free (v->search_pattern);
  free (v);
}

static void
addstr_cols (const char *s, int max_cols)
{
  mbstate_t st;
  int cols = 0;
  size_t i = 0;
  size_t slen = strlen (s);

  memset (&st, 0, sizeof (st));
  while (i < slen && cols < max_cols)
    {
      wchar_t wc;
      size_t n = mbrtowc (&wc, s + i, slen - i, &st);
      int w;

      if (n == (size_t)-1 || n == (size_t)-2)
        {
          n = 1;
          w = 1;
        }
      else if (n == 0)
        break;
      else
        {
          w = wcwidth (wc);
          if (w < 0)
            w = 1;
        }

      if (cols + w > max_cols)
        break;
      cols += w;
      i += n;
    }
  addnstr (s, (int)i);
}

/* Display the help line.  */
static void
show_help_line (void)
{
  highlight ();
  mvaddstr (0, 0, "q:Quit  O:New  d:Delete  y:Yank  P:Paste  ?:Help");
  pad_to_eol ();
  lowlight ();
}

/* Display ITEM from V at LINE.  */
static void
show_item (struct view *v, int line, int item)
{
  if (item == v->selected)
    {
      if (has_colors ())
	color_set (color_selected, NULL);
      else
	standout ();
    }

  move (line, 0);
  addstr_cols (v->s->items[item]->description, COLS);

  if (item == v->selected)
    {
      pad_to_eol ();

      if (has_colors ())
	color_set (color_default, NULL);
      else
	standend ();
    }
  else
    clrtoeol ();
}

/* Display the mode line for V.  */
static void
show_mode_line (struct view *v)
{
  int x, y;

  highlight ();
  mvaddch (LINES - 2, 0, '-');
  addch (v->s->db->readonly ? '%' : '-');
  printw ("-Tina: %s [", v->s->db->path);
  if (v->s->category != NULL)
    {
      size_t i;

      for (i = 0; i < v->s->db->nitems; i++)
	if (strcmp (v->s->category, v->s->db->items[i]->identifier) == 0)
	  {
	    addstr ("Category:");
	    getyx (stdscr, y, x);
	    if (y != -1) {
		    addstr_cols (v->s->db->items[i]->description, COLS - x - 20);
		    addch (' ');
	    }
	    break;
	  }
    }
  printw ("Items:%zu]", v->s->nitems);
  for (getyx (stdscr, y, x); y != -1 && x < COLS - 8; x++)
    addch ('-');
  addch('(');
  if ((int)v->s->nitems <= LINES - 3)
    addstr ("all");
  else if (v->selected / (LINES - 3) == ((int)v->s->nitems - 1) / (LINES - 3))
    addstr ("end");
  else
    printw ("%zu%%",
	    (v->selected / (LINES - 3) + 1) * (LINES - 3) * 100 / v->s->nitems);
  addstr (")---");
  lowlight ();
}

void
view_show (struct view *v)
{
  size_t i, first;

  show_help_line ();

  first = v->selected - v->selected % (LINES - 3);
  for (i = 0; (int)i < LINES - 3 && first + i < v->s->nitems; i++)
    show_item (v, i + 1, first + i);
  for (; (int)i < LINES - 3; i++)
    CLEARLINE (i + 1);

  show_mode_line (v);
}
