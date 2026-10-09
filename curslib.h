/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2002  Matt Kraai
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TINA_CURSLIB_H
#define TINA_CURSLIB_H

enum
{
  color_default = 1,
  color_selected,
  color_status
};

/* Turn on highlighting for the help or mode line.  */
void highlight (void);
/* Turn off highlighting for the help or mode line.  */
void lowlight (void);
/* Pad with spaces until the end of line.  */
void pad_to_eol (void);

/* Display PROMPT and return the user's response.  Use VALUE as a default if it
   is not NULL.  */
char *inquire (const char *prompt, const char *value);

#define CLEARLINE(LINE) do { move ((LINE), 0); clrtoeol (); } while (0)
#define CONTROL(X) ((X) & ~0x40)

#endif /* TINA_CURSLIB_H */
