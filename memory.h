/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TINA_MEMORY_H
#define TINA_MEMORY_H

#include <sys/types.h>

/* Reallocate PTR to be SIZE bytes.  */
void *xrealloc (void *ptr, size_t size);
/* Allocate a pointer to SIZE bytes.  */
void *xmalloc (size_t size);
/* Allocate a zeroed array of NMEMB SIZE-byte elements.  */
void *xcalloc (size_t nmemb, size_t size);
/* Allocate a copy of S.  */
char *xstrdup (const char *s);
/* Allocate a copy of the first N characters of S.  */
char *xstrndup (const char *s, size_t n);

#endif /* TINA_MEMORY_H */
