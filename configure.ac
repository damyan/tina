dnl Process this file with autoconf to produce configure
dnl
dnl SPDX-FileCopyrightText: 2002 - 2007  Matt Kraai
dnl SPDX-FileCopyrightText: Peter Pentchev <roam@ringlet.net>
dnl SPDX-License-Identifier: GPL-2.0-or-later

AC_INIT(tina, 0.1.15)
AM_INIT_AUTOMAKE

AC_PROG_CC

AC_MSG_CHECKING([whether the C compiler accepts -Wall])
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$ac_save_CFLAGS -Wall"
AC_TRY_COMPILE([], [],
	       [ac_accepts_Wall="yes"],
	       [ac_accepts_Wall="no"; CFLAGS="$ac_save_CFLAGS"])
AC_MSG_RESULT([$ac_accepts_Wall])

AC_SYS_LARGEFILE

AC_SEARCH_LIBS([initscr], [ncursesw ncurses curses], [],
  [AC_MSG_ERROR([No curses library found])])

AC_MSG_CHECKING([whether the C compiler accepts -Wstrict-prototypes])
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$ac_save_CFLAGS -Wstrict-prototypes"
AC_COMPILE_IFELSE(
	[AC_LANG_SOURCE([
#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc > 0)
		puts(*argv);
	return (0);
}
	])],
	[ac_accepts_Wstrict_proto="yes"],
	[ac_accepts_Wstrict_proto="no"; CFLAGS="$ac_save_CFLAGS"]
)
AC_MSG_RESULT([$ac_accepts_Wstrict_proto])

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
