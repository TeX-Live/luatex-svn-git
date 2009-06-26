/* directions.c
   
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
#include "commands.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";

/* |scan_direction| has to be defined here because luatangle will output 
   a character constant when it sees a string literal of length 1 */

#define scan_single_dir(A) do {                     \
        if (scan_keyword("T")) A=dir_T;             \
        else if (scan_keyword("L")) A=dir_L;        \
        else if (scan_keyword("B")) A=dir_B;        \
        else if (scan_keyword("R")) A=dir_R;        \
        else {                                      \
            tex_error("Bad direction", NULL);       \
            cur_val=0;                              \
            return;                                 \
        }                                           \
    } while (0)

void scan_direction(void)
{
    integer d1, d2, d3;
    get_x_token();
    if (cur_cmd == assign_dir_cmd) {
        cur_val = zeqtb[cur_chr].cint;
        return;
    } else {
        back_input();
    }
    scan_single_dir(d1);
    scan_single_dir(d2);
    if (dir_parallel(d1, d2)) {
        tex_error("Bad direction", NULL);
        cur_val = 0;
        return;
    }
    scan_single_dir(d3);
    get_x_token();
    if (cur_cmd != spacer_cmd)
        back_input();
    cur_val = d1 * 8 + dir_rearrange[d2] * 4 + d3;
}

/* the next two are used by postlinebreak.c */

halfword do_push_dir_node(halfword p, halfword a)
{
    halfword n;
    n = copy_node(a);
    vlink(n) = p;
    return n;
}

halfword do_pop_dir_node(halfword p)
{
    halfword n = vlink(p);
    flush_node(p);
    return n;
}
