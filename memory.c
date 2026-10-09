/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "memory.h"

void *
xrealloc (void *ptr, size_t size)
{
  ptr = realloc (ptr, size);
  if (ptr == NULL && size != 0)
    fatal_error ("You are out of memory.");

  return ptr;
}

void *
xmalloc (size_t size)
{
  void *ptr;

  ptr = malloc (size);
  if (ptr == NULL && size != 0)
    fatal_error ("You are out of memory.");

  return ptr;
}

void *
xcalloc (size_t nmemb, size_t size)
{
  void *ptr;

  ptr = calloc (nmemb, size);
  if (ptr == NULL && nmemb != 0 && size != 0)
    fatal_error ("You are out of memory.");

  return ptr;
}

char *
xstrdup (const char *s)
{
  char *news;

  news = strdup (s);
  if (news == NULL)
    fatal_error ("You are out of memory.");

  return news;
}

char *
xstrndup (const char *s, size_t n)
{
  char *news;
  size_t len;

  len = strlen (s);
  if (n < len)
    len = n;

  news = xmalloc (len + 1);
  memcpy (news, s, len);
  news[len] = '\0';

  return news;
}
