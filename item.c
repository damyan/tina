/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "item.h"
#include "memory.h"

static inline unsigned int
get_srand_seed (void)
{
  {
    unsigned int seed;
    const int random_fd = open ("/dev/urandom", O_RDONLY);
    if (random_fd != 0)
    {
      const ssize_t nread = read (random_fd, &seed, sizeof(seed));
      close (random_fd);
      if (nread > 0 && (size_t)nread == sizeof(seed))
        return seed;
    }
  }

  {
    unsigned int seed;
    const int random_fd = open ("/dev/random", O_RDONLY);
    if (random_fd != 0)
    {
      const ssize_t nread = read (random_fd, &seed, sizeof(seed));
      close (random_fd);
      if (nread > 0 && (size_t)nread == sizeof(seed))
        return seed;
    }
  }

  /* OK, do it the old naive way. */
  return (unsigned int)getpid () ^ (unsigned int)time (NULL);
}

static inline int
rand_i (void)
{
  static bool initialized = false;

  if (!initialized)
  {
    srand (get_srand_seed ());
    initialized = true;
  }

  return rand();
}

/* Return a unique identifier.  */
static char *
unique_identifier (void)
{
  char buf[256];
  size_t len;

  snprintf (buf, sizeof (buf), "<%jx.%x@", (intmax_t)time (NULL), rand_i ());
  len = strlen (buf);
  gethostname (buf + len, sizeof (buf) - len - 1);
  strcat (buf, ">");

  return xstrdup (buf);
}

struct item *
item_new (void)
{
  struct item *it;

  it = xcalloc (1, sizeof (struct item));
  it->identifier = unique_identifier ();

  return it;
}

struct item *
item_new_with_description (const char *description)
{
  struct item *it;

  if (description == NULL)
    abort ();

  it = item_new ();
  it->description = xstrdup (description);

  return it;
}

void
item_regenerate_id (struct item * const item)
{
  char * const p_dot = strchr (item->identifier, '.');
  if (p_dot == NULL)
    abort();
  const char * const p_at = strchr (p_dot + 1, '@');
  if (p_at == NULL)
    abort();

  errno = 0;
  char *p_end;
  const unsigned long current = strtoul (p_dot + 1, &p_end, 16);
  if (p_end == NULL || *p_end != '@' || errno != 0)
    abort();

  char buf[256];
  snprintf (buf, sizeof (buf), "%lx", current + 1);

  const size_t len = strlen (buf);
  if (p_dot + 1 + len != p_at )
    abort();
  memcpy (p_dot + 1, buf, len);
}

/* Is this function unused? If not, we need to check for duplicate item ID here. */
struct item *
item_clone (struct item *it)
{
  struct item *newit;
  size_t i;

  newit = item_new_with_description (it->description);
  newit->categories = xmalloc (sizeof (char *) * it->ncategories);
  for (i = 0; i < it->ncategories; i++)
    newit->categories[i] = xstrdup (it->categories[i]);
  newit->ncategories = it->ncategories;

  return newit;
}

void
item_delete (struct item *it)
{
  size_t i;

  free (it->description);
  for (i = 0; i < it->ncategories; i++)
    free (it->categories[i]);
  free (it->categories);
  free (it);
}

void
item_identifier_set (struct item *it, const char *identifier)
{
  free (it->identifier);
  it->identifier = xstrdup (identifier);
}

void
item_description_set (struct item *it, const char *description)
{
  free (it->description);
  it->description = xstrdup (description);
}

void
item_category_add (struct item *it, const char *category)
{
  if (! item_category_member_p (it, category))
    {
      it->categories = xrealloc (it->categories,
				 sizeof (char *) * (it->ncategories + 1));
      it->categories[it->ncategories++] = xstrdup (category);
    }
}

void
item_category_remove (struct item *it, const char *category)
{
  /*
   * NB: keep this signed because of the decrement at the end.
   *   -- Peter Pentchev  2010/06/04
   */
  int i;

  for (i = 0; i < (int)it->ncategories; i++)
    if (strcmp (it->categories[i], category) == 0)
      {
	free (it->categories[i]);
	memmove (it->categories + i, it->categories + i + 1,
		 sizeof (char *) * (it->ncategories - i - 1));
	it->ncategories--;
	i--;
      }
}

int
item_category_member_p (struct item *it, const char *category)
{
  size_t i;

  for (i = 0; i < it->ncategories; i++)
    if (strcmp (it->categories[i], category) == 0)
      return 1;

  return 0;
}
