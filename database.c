/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "database.h"
#include "error.h"
#include "item.h"
#include "memory.h"

/* Perform tilde expansion on PATH and return the result.  */
static char *
expand_path (const char *path)
{
  if (path[0] == '~')
    {
      const char *home = NULL;
      struct passwd *pw = NULL;

      if (path[1] == '\0' || path[1] == '/')
	{
	  home = getenv ("HOME");

	  if (home == NULL)
	    pw = getpwuid (getuid ());
	}
      else
	{
	  const char *end;

	  end = strchr (path + 1, '/');
	  if (end != NULL)
	    {
	      char *userid;

	      userid = xstrndup (path + 1, end - (path + 1));
	      pw = getpwnam (userid);
	      free (userid);
	    }
	  else
	    pw = getpwnam (path + 1);
	}

      if (home == NULL && pw != NULL)
	home = pw->pw_dir;

      if (home != NULL)
	{
	  char *newpath;

	  newpath = xmalloc (strlen (home) + strlen (path + 1) + 1);
	  strcpy (newpath, home);
	  strcat (newpath, path + 1);

	  return newpath;
	}
    }

  return xstrdup (path);
}

/* Perform tilde expansion on PATH and call fopen on the result.  */
static FILE *
efopen (const char *path, const char *mode)
{
  char *newpath;
  FILE *fp;

  newpath = expand_path (path);
  fp = fopen (newpath, mode);
  free (newpath);

  return fp;
}

/* Generate the lockfile path for PATH.  */
static char *
lockfile_path (const char *path)
{
  char *lockfile;

  lockfile = expand_path (path);
  lockfile = xrealloc (lockfile, strlen (lockfile) + 6);
  strcat (lockfile, ".lock");

  return lockfile;
}

/* Return nonzero if able to lock PATH.  */
static int
lock (const char *path)
{
  char *lockfile;
  int fd;

  lockfile = lockfile_path (path);
  fd = open (lockfile, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd != -1)
    close (fd);
  free (lockfile);

  return fd != -1;
}

/* Unlock PATH.  */
static void
unlock (const char *path)
{
  char *lockfile;

  lockfile = lockfile_path (path);
  remove (lockfile);
  free (lockfile);
}

struct database *
database_new_with_path (const char *path)
{
  struct database *db;
  FILE *fp;

  db = xcalloc (1, sizeof (struct database));

  if (! lock (path))
    {
      if (errno == EEXIST)
	error ("The database is locked.");
      else
	error ("The database cannot be locked.");

      db->readonly = 1;
    }

  db->path = xstrdup (path);

  fp = efopen (db->path, "r");
  if (fp != NULL)
    {
      struct item *it = NULL;
      char *line = NULL;
      size_t n = 0;

      while (getline (&line, &n, fp) != -1)
	{
	  char *value;
	  size_t len = strlen (line);

	  if (len > 0 && line[len - 1] == '\n')
	    line[len - 1] = '\0';

	  value = strchr (line, ':');
	  if (value != NULL)
	    {
	      if (it == NULL)
		{
		  db->items = xrealloc (db->items,
					sizeof (struct item *)
					* (db->nitems + 1));
		  it = db->items[db->nitems++] = item_new ();
		}

	      *value++ = '\0';
	      while (isspace (*value))
		value++;

	      if (strcmp (line, "Item-ID") == 0)
		item_identifier_set (it, value);
	      else if (strcmp (line, "Description") == 0)
		it->description = xstrdup (value);
	      else if (strcmp (line, "Category") == 0)
		item_category_add (it, value);
	      else
		fatal_error ("There is an unrecognized field.");
	    }
	  else if (line[0] == '\0')
	    {
	      if (it != NULL && it->description == NULL)
		{
		  fatal_error ("There is an item with no description.");
		  free (it);
		  db->nitems--;
		}

	      it = NULL;
	    }
	  else
	    fatal_error ("There is an invalid line.");
	}

      if (it != NULL && it->description == NULL)
	{
	  fatal_error ("There is an item with no description.");
	  free (it);
	  db->nitems--;
	}

      fclose (fp);
    }

  /* We need to check for duplicate item IDs here, don't we... */
  return db;
}

void
database_sync (struct database *db)
{
  FILE *fp;
  char *backup;

  backup = xmalloc (strlen (db->path) + 2);
  strcpy (backup, db->path);
  strcat (backup, "~");
  rename (db->path, backup);
  free (backup);

  fp = efopen (db->path, "w");
  if (fp != NULL)
    {
      size_t i;

      for (i = 0; i < db->nitems; i++)
	{
	  size_t j;

	  if (i != 0)
	    putc ('\n', fp);

	  fputs ("Item-ID: ", fp);
	  fputs (db->items[i]->identifier, fp);
	  putc ('\n', fp);

	  fputs ("Description: ", fp);
	  fputs (db->items[i]->description, fp);
	  putc ('\n', fp);

	  for (j = 0; j < db->items[i]->ncategories; j++)
	    {
	      fputs ("Category: ", fp);
	      fputs (db->items[i]->categories[j], fp);
	      putc ('\n', fp);
	    }
	}

      fclose (fp);
    }
}

void
database_delete (struct database *db)
{
  size_t i;

  if (! db->readonly)
    {
      database_sync (db);
      unlock (db->path);
    }

  free (db->path);
  for (i = 0; i < db->nitems; i++)
    item_delete (db->items[i]);
  free (db->items);
  free (db);
}

void
database_item_add (struct database *db, struct item *it, int pos)
{
  if (pos < 0 || db->nitems < (size_t)pos)
    abort ();

  db->items = xrealloc (db->items, sizeof (struct item *) * (db->nitems + 1));
  memmove (db->items + pos + 1, db->items + pos,
	   sizeof (struct item *) * (db->nitems - pos));
  db->items[pos] = it;
  db->nitems++;
}

void
database_item_remove (struct database *db, int pos)
{
  if (pos < 0 || db->nitems < (size_t)pos + 1)
    abort ();

  memmove (db->items + pos, db->items + pos + 1,
	   sizeof (struct item *) * (db->nitems - pos - 1));
  db->items = xrealloc (db->items, sizeof (struct item *) * (db->nitems - 1));
  db->nitems--;
}

size_t
database_item_index (struct database *db, struct item *it)
{
  size_t i;

  for (i = 0; i < db->nitems; i++)
    if (db->items[i] == it)
      break;

  return i;
}

size_t
database_item_index_by_id (struct database *db, const char * const id)
{
  size_t i;

  for (i = 0; i < db->nitems; i++)
    if (strcmp (db->items[i]->identifier, id) == 0)
      break;

  return i;
}
