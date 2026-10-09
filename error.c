/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <curses.h>
#include <stdlib.h>

#include "error.h"

void
error (const char *message)
{
  mvaddstr (LINES - 1, 0, message);
  clrtoeol ();
}

void
fatal_error (const char *message)
{
  endwin ();
  puts (message);
  exit (1);
}
