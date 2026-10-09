/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2002  Matt Kraai
 * SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <ctype.h>
#include <curses.h>
#include <stdlib.h>
#include <string.h>

#include "curslib.h"
#include "memory.h"

void
highlight (void)
{
  if (has_colors ())
    {
      color_set (color_status, NULL);
      attron (A_BOLD);
    }
  else
    standout ();
}

void
lowlight (void)
{
  if (has_colors ())
    {
      color_set (color_default, NULL);
      attroff (A_BOLD);
    }
  else
    standout ();
}

void
pad_to_eol (void)
{
  int x, y;

  for (getyx (stdscr, y, x); y != -1 && x < COLS; x++)
    addch (' ');
}

char *
inquire (const char *prompt, const char *value)
{
  char *buf = NULL, *killed = NULL, swap;
  size_t beg = 0, len = 0, pos = 0, tmppos;
  int input, x, y;

  mvaddstr (LINES - 1, 0, prompt);
  getyx (stdscr, y, x);
  if (value != NULL && y != -1)
    {
      addstr (value);
      buf = xstrdup (value);
      len = strlen (value);
      move (y, x + pos);
    }
  curs_set (1);

  while ((input = getch ()) != '\n' && input != CONTROL ('G'))
    {
      switch (input)
	{
	case 0x1B:
	  switch (getch ())
	    {
	    case 'b':
	      while (pos > 0 && ! isalnum (buf[pos - 1]))
		pos--;
	      while (pos > 0 && isalnum (buf[pos - 1]))
		pos--;
	      break;

	    case 'f':
	      while (pos < len && ! isalnum (buf[pos]))
		pos++;
	      while (pos < len && isalnum (buf[pos]))
		pos++;
	      break;

	    case 'd':
	      tmppos = pos;
	      while (tmppos < len && ! isalnum (buf[tmppos]))
		tmppos++;
	      while (tmppos < len && isalnum (buf[tmppos]))
		tmppos++;

	      free (killed);
	      killed = xstrndup (buf + pos, tmppos - pos);

	      memmove (buf + pos, buf + tmppos, len - tmppos);
	      len -= tmppos - pos;
	      break;

	    case KEY_BACKSPACE:
	      tmppos = pos;
	      while (tmppos > 0 && ! isalnum (buf[tmppos - 1]))
		tmppos--;
	      while (tmppos > 0 && isalnum (buf[tmppos - 1]))
		tmppos--;

	      free (killed);
	      killed = xstrndup (buf + tmppos, pos - tmppos);

	      memmove (buf + tmppos, buf + pos, len - pos);
	      len -= pos - tmppos;
	      pos = tmppos;
	      break;
	    }
	  break;

	case KEY_BACKSPACE:
	  if (pos > 0)
	    {
	      memmove (buf + pos - 1, buf + pos, len - pos);
	      len--;
	      pos--;
	    }
	  break;

	case CONTROL ('D'):
	  if (pos < len)
	    {
	      memmove (buf + pos, buf + pos + 1, len - pos - 1);
	      len--;
	    }
	  break;

	case KEY_END:
	case CONTROL ('E'):
	  pos = len;
	  break;

	case KEY_HOME:
	case CONTROL ('A'):
	  pos = 0;
	  break;

	case KEY_LEFT:
	case CONTROL ('B'):
	  if (pos > 0)
	    pos--;
	  break;

	case KEY_RIGHT:
	case CONTROL ('F'):
	  if (pos < len)
	    pos++;
	  break;

	case CONTROL ('K'):
	  free (killed);
	  killed = xstrndup (buf + pos, len - pos);

	  len = pos;
	  break;

	case CONTROL ('L'):
	  clearok (stdscr, TRUE);
	  break;

	case CONTROL ('T'):
	  if (pos == len)
	    pos--;

	  swap = buf[pos - 1];
	  buf[pos - 1] = buf[pos];
	  buf[pos] = swap;
	  pos++;
	  break;

	case CONTROL ('U'):
	  free (killed);
	  killed = xstrndup (buf, pos);

	  memmove (buf, buf + pos, len - pos);
	  len = len - pos;
	  pos = 0;
	  break;

	case CONTROL ('W'):
	  tmppos = pos;
	  while (tmppos > 0 && isspace (buf[tmppos - 1]))
	    tmppos--;
	  while (tmppos > 0 && ! isspace (buf[tmppos - 1]))
	    tmppos--;

	  free (killed);
	  killed = xstrndup (buf + tmppos, pos - tmppos);

	  memmove (buf + tmppos, buf + pos, len - pos);
	  len -= pos - tmppos;
	  pos = tmppos;
	  break;

	case CONTROL ('Y'):
	  if (killed != NULL)
	    {
	      size_t killed_len;

	      killed_len = strlen (killed);
	      buf = xrealloc (buf, len + killed_len);
	      memmove (buf + pos + killed_len, buf + pos, len - pos);
	      memcpy (buf + pos, killed, killed_len);
	      pos += killed_len;
	      len += killed_len;
	    }
	  break;

	default:
	  buf = xrealloc (buf, len + 1);
	  memmove (buf + pos + 1, buf + pos, len - pos);
	  buf[pos++] = input;
	  len++;
	  break;
	}

      if (beg + COLS - x <= pos)
	beg = pos - (COLS - x) + 1;
      else if (pos < beg)
	beg = pos;

      mvaddnstr (y, x, buf + beg,
		 (int)(len - beg) < COLS - x ? (int)(len - beg) : COLS - x);
      clrtoeol ();
      move (y, x + pos - beg);
    }

  free (killed);

  if (input == '\n')
    {
      buf = xrealloc (buf, len + 1);
      buf[len] = '\0';
    }
  else
    {
      free (buf);
      buf = NULL;
    }

  curs_set (0);
  CLEARLINE (y);

  return buf;
}
