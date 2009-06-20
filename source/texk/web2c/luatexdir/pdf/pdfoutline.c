/* pdfoutline.c
   
   Copyright 2009 Taco Hoekwater <taco@luatex.org>

   This file is part of LuaTeX.

   LuaTeX is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your
   option) any later version.

   LuaTeX is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with LuaTeX; if not, see <http://www.gnu.org/licenses/>. */

#include "ptexlib.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";


integer open_subentries(halfword p)
{
    integer k, c;
    integer l, r;
    k = 0;
    if (obj_outline_first(p) != 0) {
        l = obj_outline_first(p);
        do {
            incr(k);
            c = open_subentries(l);
            if (obj_outline_count(l) > 0)
                k = k + c;
            obj_outline_parent(l) = p;
            r = obj_outline_next(l);
            if (r == 0)
                obj_outline_last(p) = l;
            l = r;
        } while (l != 0);
    }
    if (obj_outline_count(p) > 0)
        obj_outline_count(p) = k;
    else
        obj_outline_count(p) = -k;
    return k;
}
