#!/bin/sh
# build fspec.c from fspec.def
# Copyright (C) 1995 Free Software Foundation, Inc.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

: ${1?}

echo '/* Generated automatically from fspec.defs.  DO NOT EDIT. */

#include <tabs.h>
'

sed -n	-e '/^#/d' \
-e '/^[ 	]*$/d' \
-e 's+^\([^	]*\).\([^	]*\).*+static int t_\1[] = {\2,0};+p' \
"$1"

echo '
struct fspec_table fspec_table[] = {'

sed -n	-e '/^#/d' \
-e '/^[ 	]*$/d' \
-e 's+^\([^	]*\).\([^	]*\).\(.*\)$+  {"\1", t_\1, "\3"},+p' \
"$1"

echo '  {0, 0}
};'
exit 0
