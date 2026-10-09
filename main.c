/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001, 2002  Matt Kraai
 * SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#define _GNU_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1

#include <ctype.h>
#include <curses.h>
#include <errno.h>
#include <getopt.h>
#include <locale.h>
#include <pwd.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "curslib.h"
#include "database.h"
#include "error.h"
#include "item.h"
#include "memory.h"
#include "selection.h"
#include "view.h"

#ifndef __unused
#ifdef __GNUC__
#define __unused __attribute__((unused))
#else  /* __GNUC__ */
#define __unused
#endif /* __GNUC__ */
#endif /* __unused */

#define NUM_ELEMENTS(ARR) (sizeof (ARR) / sizeof ((ARR)[0]))

/* Add IT at POS to V.  */
static void
insert_item (struct view *v, size_t pos, struct item *it)
{
  size_t oldpos;

  oldpos = database_item_index (v->s->db, it);
  if (oldpos != v->s->db->nitems)
    database_item_remove (v->s->db, oldpos);
  database_item_add (v->s->db, it,
		     pos < v->s->nitems
		     ? database_item_index (v->s->db, v->s->items[pos])
		     : v->s->db->nitems);

  oldpos = selection_item_index (v->s, it);
  if (oldpos != v->s->nitems)
    {
      selection_item_remove (v->s, oldpos);

      if (oldpos < pos)
	pos--;
    }
  selection_item_add (v->s, it, pos);

  v->selected = pos;
}

/* Yank the selected item in V into CLIPBOARD.  */
static void
yank_item (struct view *v, struct item **clipboard)
{
  if (*clipboard != NULL && *clipboard != v->s->items[v->selected]
      && database_item_index (v->s->db, *clipboard) == v->s->db->nitems)
    item_delete (*clipboard);

  *clipboard = v->s->items[v->selected];
}

/* Search for an item matching PATTERN in V.  DIR should be 1 to search forward,
   and -1 to search backward.  */
static void
search_view (struct view *v, const char *pattern, int dir)
{
  int found = 0;
  regex_t reg;
  /*
   * NB: keep this signed because of the direction thing.
   *   -- Peter Pentchev  2010/06/04
   */
  int i;

  if (dir != -1 && dir != 1)
    abort ();

  if (regcomp (&reg, pattern, REG_EXTENDED | REG_NOSUB) != 0)
    {
      error ("Unable to compile regular expression.");
      return;
    }

  for (i = v->selected + dir; 0 <= i && i < (int)v->s->nitems; i += dir)
    if (regexec (&reg, v->s->items[i]->description, 0, NULL, 0) == 0)
      {
	found = 1;
	break;
      }

  if (! found)
    for (i = dir > 0 ? 0 : v->s->nitems - 1; i != v->selected + dir; i += dir)
      if (regexec (&reg, v->s->items[i]->description, 0, NULL, 0) == 0)
	{
	  if (dir > 0)
	    error ("Search wrapped to top.");
	  else
	    error ("Search wrapped to bottom.");

	  found = 1;
	  break;
	}

  if (found)
    v->selected = i;
  else
    error ("Not found.");

  regfree (&reg);
}

/* Create a new item with a unique (for this database) ID. */
static struct item *
unique_item_new_with_description (const struct view * const v, const char * const description)
{
  struct item * const it = item_new_with_description (description);
  while (database_item_index_by_id (v->s->db, it->identifier) < v->s->db->nitems)
    item_regenerate_id (it);
  return it;
}

/* Read an item and add it at POS to V.  */
static void
read_item (struct view *v, int pos)
{
  char *description;

  description = inquire ("Item: ", NULL);
  if (description != NULL)
    {
      insert_item (v, pos, unique_item_new_with_description (v, description));
      free (description);
    }
}

static void
cmd_categorize (struct view **v, struct item **clipboard __unused)
{
  char *category;

  category = inquire ("Category: ", NULL);
  if (category != NULL)
    {
      size_t i;

      for (i = 0; i < (*v)->s->db->nitems; i++)
	if (strcmp (category, (*v)->s->db->items[i]->description) == 0)
	  {
            item_category_add ((*v)->s->items[(*v)->selected],
			       (*v)->s->db->items[i]->identifier);
	    break;
	  }

      if (i == (*v)->s->db->nitems)
	{
	  struct item *it;

	  it = unique_item_new_with_description ((*v), category);
	  database_item_add ((*v)->s->db, it, (*v)->s->db->nitems);
	  item_category_add ((*v)->s->items[(*v)->selected], it->identifier);
	}

      free (category);
    }
}

static void
cmd_change (struct view **v, struct item **clipboard __unused)
{
  char *description;

  description = inquire ("Item: ", (*v)->s->items[(*v)->selected]->description);
  if (description != NULL)
    {
      item_description_set ((*v)->s->items[(*v)->selected], description);
      free (description);
    }
}

static void
cmd_delete (struct view **v, struct item **clipboard)
{
  size_t pos;

  yank_item (*v, clipboard);

  selection_item_remove ((*v)->s, (*v)->selected);

  if (((*v)->s->category == NULL || (*clipboard)->ncategories == 0)
      && (pos = database_item_index ((*v)->s->db, *clipboard))
	 != (*v)->s->db->nitems)
    database_item_remove ((*v)->s->db, pos);
}

static void
cmd_last_item (struct view **v, struct item **clipboard __unused)
{
  (*v)->selected = (*v)->s->nitems - 1;
}

static void
cmd_limit (struct view **v, struct item **clipboard __unused)
{
  char *limit;

  limit = inquire ("Limit: ", NULL);
  if (limit != NULL)
    {
      struct selection *s;
      struct view *newv;

      s = selection_new_with_database ((*v)->s->db);

      if (limit[0] != '\0')
	{
	  size_t i;

	  for (i = 0; i < (*v)->s->db->nitems; i++)
	    if (strcmp (limit, (*v)->s->db->items[i]->description) == 0)
	      {
		selection_category_set (s, (*v)->s->db->items[i]->identifier);
		break;
	      }

	  if (i == (*v)->s->db->nitems)
	    {
	      struct item *it;

	      it = unique_item_new_with_description ((*v), limit);
	      database_item_add ((*v)->s->db, it, (*v)->s->db->nitems);
	      selection_category_set (s, it->identifier);
	    }
	}
      else
	selection_category_set (s, NULL);

      newv = view_new_with_selection (s);
      newv->prev = *v;
      *v = newv;
      free (limit);
    }
}

static void
cmd_next_add (struct view **v, struct item **clipboard __unused)
{
  read_item (*v, (*v)->s->nitems != 0 ? (*v)->selected + 1 : 0);
}

static void
cmd_next_item (struct view **v, struct item **clipboard __unused)
{
  (*v)->selected++;
}

static void
cmd_next_page (struct view **v, struct item **clipboard __unused)
{
  if ((*v)->selected / (LINES - 3) == ((int)(*v)->s->nitems - 1) / (LINES - 3))
    (*v)->selected = (*v)->s->nitems - 1;
  else
    (*v)->selected = ((*v)->selected / (LINES - 3) + 1) * (LINES - 3);
}

static void
cmd_next_paste (struct view **v, struct item **clipboard)
{
  insert_item (*v, (*v)->s->nitems != 0 ? (*v)->selected + 1 : 0,
	       *clipboard);
}

static void
cmd_pop_view (struct view **v, struct item **clipboard __unused)
{
  if ((*v)->prev == NULL)
    error ("You are on the first view.");
  else
    {
      struct view *prev;

      prev = (*v)->prev;
      selection_delete ((*v)->s);
      view_delete (*v);
      *v = prev;
      (*v)->selected = 0;
    }
}

static void
cmd_previous_add (struct view **v, struct item **clipboard __unused)
{
  read_item (*v, (*v)->selected);
}

static void
cmd_previous_item (struct view **v, struct item **clipboard __unused)
{
  (*v)->selected--;
}

static void
cmd_previous_page (struct view **v, struct item **clipboard __unused)
{
  if ((*v)->selected < LINES - 3)
    (*v)->selected = 0;
  else
    (*v)->selected = ((*v)->selected / (LINES - 3)) * (LINES - 3) - 1;
}

static void
cmd_previous_paste (struct view **v, struct item **clipboard)
{
  insert_item (*v, (*v)->selected, *clipboard);
}

static void
cmd_push_view (struct view **v, struct item **clipboard __unused)
{
  struct selection *s;
  struct view *newv;

  s = selection_new_with_database ((*v)->s->db);
  selection_category_set (s, (*v)->s->items[(*v)->selected]->identifier);
  newv = view_new_with_selection (s);
  newv->prev = *v;
  *v = newv;
}

static void
cmd_refresh (struct view **v __unused, struct item **clipboard __unused)
{
  clearok (stdscr, TRUE);
}

static void
cmd_search (struct view **v, struct item **clipboard __unused)
{
  free ((*v)->search_pattern);
  (*v)->search_pattern = inquire ("Search for: ", NULL);
  if ((*v)->search_pattern != NULL)
    search_view (*v, (*v)->search_pattern, 1);
}

static void
cmd_search_again (struct view **v, struct item **clipboard __unused)
{
  if ((*v)->search_pattern == NULL)
    error ("No search pattern.");
  else
    search_view (*v, (*v)->search_pattern, 1);
}

static void
cmd_search_opposite (struct view **v, struct item **clipboard __unused)
{
  if ((*v)->search_pattern == NULL)
    error ("No search pattern.");
  else
    search_view (*v, (*v)->search_pattern, -1);
}

static void
cmd_shell_escape (struct view **v __unused, struct item **clipboard __unused)
{
  char *command;

  command = inquire ("Shell command: ", NULL);
  if (command != NULL)
    {
      CLEARLINE (LINES - 1);
      refresh ();
      endwin ();

      if (system (command) == -1)
	printf ("Could not execute the command: %s\n", strerror (errno));
      fputs ("Press <Return> to continue...", stdout);
      fflush (stdout);
      getchar ();

      curs_set (0);
      free (command);
    }
}

static void
cmd_sync_database (struct view **v, struct item **clipboard __unused)
{
  database_sync ((*v)->s->db);
}

static void
cmd_yank (struct view **v, struct item **clipboard)
{
  yank_item (*v, clipboard);
}

enum command_flags
{
  FLAGS_WRITE = 1,
  FLAGS_ITEMS = 2,
  FLAGS_PREV_ITEM = 4,
  FLAGS_NEXT_ITEM = 8,
  FLAGS_CLIPBOARD = 16
};

struct command
{
  const char *name;
  int flags;
  void (*callback) (struct view **v, struct item **clipboard);
  const char *help;
};

struct binding
{
  int key;
  const char *name;
};

static void cmd_help (struct view **v, struct item **clipboard);

static struct command commands[] =
  {
    {
      "categorize", FLAGS_WRITE | FLAGS_ITEMS, cmd_categorize,
      "categorize the current item"
    },
    {
      "change", FLAGS_WRITE | FLAGS_ITEMS, cmd_change,
      "change the current item"
    },
    {
      "delete", FLAGS_WRITE | FLAGS_ITEMS, cmd_delete,
      "delete the current item"
    },
    {
      "help", 0, cmd_help,
      "this screen"
    },
    {
      "last-item", FLAGS_ITEMS | FLAGS_NEXT_ITEM, cmd_last_item,
      "select the last item"
    },
    {
      "limit", 0, cmd_limit,
      "show only items in a category"
    },
    {
      "next-add", FLAGS_WRITE, cmd_next_add,
      "add an item after the current item"
    },
    {
      "next-item", FLAGS_ITEMS | FLAGS_NEXT_ITEM, cmd_next_item,
      "select the next item"
    },
    {
      "next-page", FLAGS_ITEMS | FLAGS_NEXT_ITEM, cmd_next_page,
      "move to the next page"
    },
    {
      "next-paste", FLAGS_WRITE | FLAGS_CLIPBOARD, cmd_next_paste,
      "paste an item after the current item"
    },
    {
      "pop-view", 0, cmd_pop_view,
      "show the previous view"
    },
    {
      "previous-add", FLAGS_WRITE, cmd_previous_add,
      "add an item before the current item"
    },
    {
      "previous-item", FLAGS_ITEMS | FLAGS_PREV_ITEM, cmd_previous_item,
      "select the previous item"
    },
    {
      "previous-page", FLAGS_ITEMS | FLAGS_PREV_ITEM, cmd_previous_page,
      "move to the previous page"
    },
    {
      "previous-paste", FLAGS_WRITE | FLAGS_CLIPBOARD, cmd_previous_paste,
      "paste an item before the current item"
    },
    {
      "push-view", FLAGS_ITEMS, cmd_push_view,
      "show the items in the current item"
    },
    {
      "quit", 0, NULL,
      "quit"
    },
    {
      "refresh", 0, cmd_refresh,
      "refresh the screen"
    },
    {
      "search", FLAGS_ITEMS, cmd_search,
      "search for an item"
    },
    {
      "search-again", FLAGS_ITEMS, cmd_search_again,
      "search for the next match"
    },
    {
      "search-opposite", FLAGS_ITEMS, cmd_search_opposite,
      "search for the next match in the opposite direction"
    },
    {
      "shell-escape", 0, cmd_shell_escape,
      "invoke a command in a subshell"
    },
    {
      "sync-database", 0, cmd_sync_database,
      "save changes to database"
    },
    {
      "yank", FLAGS_WRITE | FLAGS_ITEMS, cmd_yank,
      "yank the current item"
    },
  };

static struct binding bindings[] =
  {
    { CONTROL ('L'),	"refresh" },
    { '!',		"shell-escape" },
    { '$',		"sync-database" },
    { '/',		"search" },
    { '?',		"help" },
    { 'C',		"categorize" },
    { 'G',		"last-item" },
    { 'L',		"limit" },
    { 'N',		"search-opposite" },
    { 'O',		"previous-add" },
    { 'P',		"previous-paste" },
    { 'Z',		"previous-page" },
    { 'c',		"change" },
    { 'd',		"delete" },
    { 'h',		"pop-view" },
    { 'j',		"next-item" },
    { 'k',		"previous-item" },
    { 'l',		"push-view" },
    { 'n',		"search-again" },
    { 'o',		"next-add" },
    { 'p',		"next-paste" },
    { 'q',		"quit" },
    { 'y',		"yank" },
    { 'z',		"next-page" },
    { KEY_DOWN,		"next-item" },
    { KEY_LEFT,		"pop-view" },
    { KEY_NPAGE,	"next-page" },
    { KEY_PPAGE,	"previous-page" },
    { KEY_RIGHT,	"push-view" },
    { KEY_UP,		"previous-item" }
  };

static void
show_help (int first)
{
  size_t i;

  highlight ();
  mvaddstr (0, 0, "q:Quit  -:PrevPg  <Space>:NextPg");
  pad_to_eol ();
  lowlight ();

  for (i = 0; (int)i < LINES - 3 && first + i < NUM_ELEMENTS (bindings); i++)
    {
      size_t j;

      CLEARLINE (i + 1);
      move (i + 1, 0);

      switch (bindings[first + i].key)
	{
	case KEY_DOWN:
	  addstr ("<Down>");
	  break;

	case KEY_LEFT:
	  addstr ("<Left>");
	  break;

	case KEY_NPAGE:
	  addstr ("<PageDown>");
	  break;

	case KEY_PPAGE:
	  addstr ("<PageUp>");
	  break;

	case KEY_RIGHT:
	  addstr ("<Right>");
	  break;

	case KEY_UP:
	  addstr ("<Up>");
	  break;

	case CONTROL ('L'):
	  addstr ("^L");
	  break;

	default:
	  addch (bindings[first + i].key);
	  break;
	}

      for (j = 0; j < NUM_ELEMENTS (commands); j++)
	if (strcmp (bindings[first + i].name, commands[j].name) == 0)
	  break;

      mvaddstr (i + 1, 12, commands[j].help);
    }

  for (; (int)i < LINES - 3; i++)
    CLEARLINE (i + 1);
}

static void
cmd_help (struct view **v __unused, struct item **clipboard __unused)
{
  size_t first;

  for (first = 0; first < NUM_ELEMENTS (bindings);)
    {
      wint_t input;

      show_help (first);

      get_wch (&input);

      CLEARLINE (LINES - 1);

      switch (input)
	{
	  case ' ':
	    first += LINES - 2;
	    break;

	  case '-':
	    if (first == 0)
	      error ("You are on the first page.");
	    else if ((int)first < LINES - 2)
	      first = 0;
	    else
	      first -= LINES - 2;
	    break;

	  case CONTROL ('L'):
	    clearok (stdscr, TRUE);
	    break;

	  default:
	    first = NUM_ELEMENTS (bindings);
	    break;
	}
    }
}

int
main (int argc, char **argv)
{
  static struct option options[] =
    {
      { "help", no_argument, NULL, 'h' },
      { "version", no_argument, NULL, 'v'}
    };

  struct database *db;
  struct selection *s;
  struct view *v;
  struct item *clipboard;

  size_t i;
  wint_t input;
  int opt;

  while ((opt = getopt_long (argc, argv, "", options, NULL)) != -1)
    switch (opt)
      {
      case 'h':
	puts ("Usage: tina [OPTION]... [FILE]");
	puts ("Manage personal information in FILE (~/.tina by default).");
	putchar ('\n');
	puts ("      --help    display this help and exit");
	puts ("      --version output version information and exit");
	putchar ('\n');
	puts ("Report bugs to <kraai@debian.org>.");
	return 0;

      case 'v':
	puts ("Tina " VERSION);
	puts ("Copyright (C) 2002 - 2007  Matt Kraai");
	puts ("Copyright (C) 2016  Peter Pentchev <roam@ringlet.net>");
	puts ("Tina comes with ABSOLUTELY NO WARRANTY.");
	puts ("You may redistribute copies of Tina under the terms of the GNU General Public");
	puts ("License.  For more information about these matters, see the file named COPYING.");
	return 0;

      default:
	fputs ("tina: Try `tina --help' for more information.\n", stderr);
	return 1;
      }

  if (argc - optind >= 2)
    {
      fputs ("tina: extra operand\n", stderr);
      fputs ("tina: Try `tina --help' for more information.\n", stderr);
      return 1;
    }

  setlocale (LC_ALL, "");

  initscr ();
  cbreak ();
  noecho ();
  keypad (stdscr, TRUE);
  curs_set (0);

  if (has_colors ())
    {
      start_color ();
      init_pair (color_default, COLOR_WHITE, COLOR_BLACK);
      init_pair (color_selected, COLOR_BLACK, COLOR_CYAN);
      init_pair (color_status, COLOR_GREEN, COLOR_BLUE);
    }

  db = database_new_with_path (optind != argc ? argv[optind] : "~/.tina");
  s = selection_new_with_database (db);
  v = view_new_with_selection (s);
  view_show (v);

  clipboard = NULL;

  while (get_wch (&input) != ERR && input != 'q')
    {
      CLEARLINE (LINES - 1);

      for (i = 0; i < NUM_ELEMENTS (bindings); i++)
	if (bindings[i].key == (int)input)
	  {
	    size_t j;

	    for (j = 0; j < NUM_ELEMENTS (commands); j++)
	      if (strcmp (bindings[i].name, commands[j].name) == 0)
		break;

	    if ((commands[j].flags & FLAGS_WRITE) && db->readonly)
	      error ("The database is read-only.");
	    else if ((commands[j].flags & FLAGS_ITEMS) && v->s->nitems == 0)
	      error ("There are no items.");
	    else if ((commands[j].flags & FLAGS_PREV_ITEM) && v->selected == 0)
	      error ("You are on the first item.");
	    else if ((commands[j].flags & FLAGS_NEXT_ITEM)
		     && v->selected == (int)v->s->nitems - 1)
	      error ("You are on the last item.");
	    else if ((commands[j].flags & FLAGS_CLIPBOARD) && clipboard == NULL)
	      error ("There is no item in the clipboard.");
	    else if (commands[j].callback != NULL)
	      commands[j].callback (&v, &clipboard);

	    break;
	  }

      if (i == NUM_ELEMENTS (bindings))
	error ("Key is not bound.  Press '?' for help.");

      selection_refresh (v->s);

      if (v->selected >= (int)v->s->nitems)
	{
	  if (v->s->nitems > 0)
	    v->selected = v->s->nitems - 1;
	  else
	    v->selected = 0;
	}

      view_show (v);
    }

  database_delete (db);

  endwin ();

  putchar ('\n');

  return 0;
}
