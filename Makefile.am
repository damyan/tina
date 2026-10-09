## SPDX-FileCopyrightText: 2001, 2002  Matt Kraai
## SPDX-License-Identifier: GPL-2.0-or-later
##
## Process this file with automake to produce Makefile.in

bin_PROGRAMS=tina
tina_SOURCES=curslib.c curslib.h database.c database.h error.c error.h item.c \
	     item.h main.c memory.c memory.h selection.c selection.h view.c \
	     view.h
man_MANS=tina.1
EXTRA_DIST=$(man_MANS) debian/changelog debian/control debian/copyright \
	   debian/postinst debian/prerm debian/rules
