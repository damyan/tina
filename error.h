/*
 * tina - a personal information manager
 * SPDX-FileCopyrightText: 2001  Matt Kraai
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TINA_ERROR_H
#define TINA_ERROR_H

/* Display MESSAGE on the status line.  */
void error (const char *message);
/* Display MESSAGE on the status line and exit unsuccessfully.  */
void fatal_error (const char *message);

#endif /* TINA_ERROR_H */
