/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2002  Matt Kraai
 * SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#define _GNU_SOURCE
#ifndef NCURSES_WIDECHAR
#define NCURSES_WIDECHAR 1
#endif

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

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

static size_t
mb_char_len (const char *s, size_t remaining)
{
  mbstate_t st;
  size_t n;

  if (remaining == 0)
    return 0;

  memset (&st, 0, sizeof (st));
  n = mbrlen (s, remaining, &st);
  if (n == (size_t)-1 || n == (size_t)-2)
    return 1;
  if (n == 0)
    return 1;
  return n;
}

static size_t
mb_prev_start (const char *buf, size_t pos)
{
  size_t back;

  if (pos == 0)
    return 0;

  for (back = 1; back <= pos && back <= 6; back++)
    {
      mbstate_t st;
      size_t n;

      memset (&st, 0, sizeof (st));
      n = mbrlen (buf + pos - back, back, &st);
      if (n == back)
        return pos - back;
    }
  return pos - 1;
}

static int
mb_char_width (const char *s, size_t remaining)
{
  mbstate_t st;
  wchar_t wc;
  size_t n;
  int w;

  if (remaining == 0)
    return 0;

  memset (&st, 0, sizeof (st));
  n = mbrtowc (&wc, s, remaining, &st);
  if (n == (size_t)-1 || n == (size_t)-2 || n == 0)
    return 1;

  w = wcwidth (wc);
  return w < 0 ? 1 : w;
}

static int
mb_display_width (const char *s, size_t nbytes)
{
  int width = 0;
  size_t i = 0;

  while (i < nbytes)
    {
      size_t clen = mb_char_len (s + i, nbytes - i);
      width += mb_char_width (s + i, nbytes - i);
      i += clen;
    }
  return width;
}

static wchar_t
mb_decode_at (const char *s, size_t remaining)
{
  mbstate_t st;
  wchar_t wc;
  size_t n;

  memset (&st, 0, sizeof (st));
  n = mbrtowc (&wc, s, remaining, &st);
  if (n == (size_t)-1 || n == (size_t)-2 || n == 0)
    return (wchar_t)(unsigned char)*s;
  return wc;
}

static size_t
find_display_offset (const char *buf, size_t len, size_t pos, int avail_cols)
{
  int pos_width = mb_display_width (buf, pos);

  if (pos_width < avail_cols)
    return 0;

  size_t beg = 0;
  int cols_before_pos = 0;
  size_t i = 0;
  while (i < pos)
    {
      size_t clen = mb_char_len (buf + i, len - i);
      int cw = mb_char_width (buf + i, len - i);
      cols_before_pos += cw;
      i += clen;
    }

  int target = cols_before_pos - (avail_cols - 1);
  if (target < 0)
    target = 0;

  beg = 0;
  int acc = 0;
  i = 0;
  while (i < len && acc < target)
    {
      size_t clen = mb_char_len (buf + i, len - i);
      acc += mb_char_width (buf + i, len - i);
      i += clen;
    }
  beg = i;
  return beg;
}

char *
inquire (const char *prompt, const char *value)
{
  char *buf = NULL, *killed = NULL;
  size_t beg = 0, len = 0, pos = 0, tmppos;
  wint_t wch;
  int input_type, x, y;
  int avail_cols;

  mvaddstr (LINES - 1, 0, prompt);
  getyx (stdscr, y, x);
  avail_cols = COLS - x;
  if (value != NULL && y != -1)
    {
      addstr (value);
      buf = xstrdup (value);
      len = strlen (value);
      move (y, x);
    }
  curs_set (1);

  while ((input_type = get_wch (&wch)) != ERR)
    {
      if (input_type == OK && wch == '\n')
        break;
      if (input_type == OK && (wch == CONTROL ('G') || wch == CONTROL ('C')))
        break;

      /* With keypad(TRUE) set in main(), ncurses disambiguates ESC for us:
         a lone ESC is delivered as 0x1B once ESCDELAY expires, while a
         function/arrow-key escape sequence is delivered as a KEY_* code.
         So a plain 0x1B here always means the Escape key alone: cancel. */
      if (input_type == OK && wch == 0x1B)
        break;

      else if ((input_type == KEY_CODE_YES && wch == KEY_BACKSPACE)
               || (input_type == OK && wch == 0x7F)
               || (input_type == OK && wch == '\b'))
        {
          if (pos > 0)
            {
              size_t prev = mb_prev_start (buf, pos);
              size_t clen = pos - prev;
              memmove (buf + prev, buf + pos, len - pos);
              len -= clen;
              pos = prev;
            }
        }
      else if (input_type == OK && wch == CONTROL ('D'))
        {
          if (pos < len)
            {
              size_t clen = mb_char_len (buf + pos, len - pos);
              memmove (buf + pos, buf + pos + clen, len - pos - clen);
              len -= clen;
            }
        }
      else if ((input_type == KEY_CODE_YES && wch == KEY_END)
               || (input_type == OK && wch == CONTROL ('E')))
        {
          pos = len;
        }
      else if ((input_type == KEY_CODE_YES && wch == KEY_HOME)
               || (input_type == OK && wch == CONTROL ('A')))
        {
          pos = 0;
        }
      else if ((input_type == KEY_CODE_YES && wch == KEY_LEFT)
               || (input_type == OK && wch == CONTROL ('B')))
        {
          if (pos > 0)
            pos = mb_prev_start (buf, pos);
        }
      else if ((input_type == KEY_CODE_YES && wch == KEY_RIGHT)
               || (input_type == OK && wch == CONTROL ('F')))
        {
          if (pos < len)
            pos += mb_char_len (buf + pos, len - pos);
        }
      else if (input_type == OK && wch == CONTROL ('K'))
        {
          free (killed);
          killed = xstrndup (buf + pos, len - pos);
          len = pos;
        }
      else if (input_type == OK && wch == CONTROL ('L'))
        {
          clearok (stdscr, TRUE);
        }
      else if (input_type == OK && wch == CONTROL ('T'))
        {
          if (len > 0 && pos > 0)
            {
              size_t cur_start, cur_len, prev_start, prev_len;
              char tmp[12];

              if (pos >= len)
                pos = mb_prev_start (buf, pos);

              cur_start = pos;
              cur_len = mb_char_len (buf + cur_start, len - cur_start);
              prev_start = mb_prev_start (buf, cur_start);
              prev_len = cur_start - prev_start;

              memcpy (tmp, buf + prev_start, prev_len);
              memmove (buf + prev_start, buf + cur_start, cur_len);
              memcpy (buf + prev_start + cur_len, tmp, prev_len);
              pos = prev_start + cur_len + prev_len;
            }
        }
      else if (input_type == OK && wch == CONTROL ('U'))
        {
          free (killed);
          killed = xstrndup (buf, pos);
          memmove (buf, buf + pos, len - pos);
          len = len - pos;
          pos = 0;
        }
      else if (input_type == OK && wch == CONTROL ('W'))
        {
          tmppos = pos;
          while (tmppos > 0 && iswspace (mb_decode_at (buf + mb_prev_start (buf, tmppos),
                   tmppos - mb_prev_start (buf, tmppos))))
            tmppos = mb_prev_start (buf, tmppos);
          while (tmppos > 0 && !iswspace (mb_decode_at (buf + mb_prev_start (buf, tmppos),
                   tmppos - mb_prev_start (buf, tmppos))))
            tmppos = mb_prev_start (buf, tmppos);

          free (killed);
          killed = xstrndup (buf + tmppos, pos - tmppos);

          memmove (buf + tmppos, buf + pos, len - pos);
          len -= pos - tmppos;
          pos = tmppos;
        }
      else if (input_type == OK && wch == CONTROL ('Y'))
        {
          if (killed != NULL)
            {
              size_t killed_len = strlen (killed);
              buf = xrealloc (buf, len + killed_len);
              memmove (buf + pos + killed_len, buf + pos, len - pos);
              memcpy (buf + pos, killed, killed_len);
              pos += killed_len;
              len += killed_len;
            }
        }
      else if (input_type == OK)
        {
          char mb[MB_CUR_MAX];
          mbstate_t st;
          size_t nbytes;

          memset (&st, 0, sizeof (st));
          nbytes = wcrtomb (mb, (wchar_t)wch, &st);
          if (nbytes != (size_t)-1 && nbytes > 0)
            {
              buf = xrealloc (buf, len + nbytes);
              memmove (buf + pos + nbytes, buf + pos, len - pos);
              memcpy (buf + pos, mb, nbytes);
              pos += nbytes;
              len += nbytes;
            }
        }

      beg = find_display_offset (buf, len, pos, avail_cols);

      {
        size_t disp_end = beg;
        int cols_used = 0;
        while (disp_end < len && cols_used < avail_cols)
          {
            size_t clen = mb_char_len (buf + disp_end, len - disp_end);
            int cw = mb_char_width (buf + disp_end, len - disp_end);
            if (cols_used + cw > avail_cols)
              break;
            cols_used += cw;
            disp_end += clen;
          }

        move (y, x);
        addnstr (buf + beg, (int)(disp_end - beg));
        clrtoeol ();

        int cursor_col = mb_display_width (buf + beg, pos - beg);
        move (y, x + cursor_col);
      }
    }

  free (killed);

  if (input_type == OK && wch == '\n')
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
