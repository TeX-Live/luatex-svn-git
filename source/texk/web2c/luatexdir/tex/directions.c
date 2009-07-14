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
    int save_cur_cmd = cur_cmd;
    int save_cur_chr = cur_chr;
    get_x_token();
    if (cur_cmd == assign_dir_cmd) {
        cur_val = eqtb[cur_chr].cint;
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
    cur_cmd = save_cur_cmd;
    cur_chr = save_cur_chr;
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

integer dvi_direction;
int dir_primary[32];
int dir_secondary[32];
int dir_tertiary[32];
int dir_rearrange[4];
str_number dir_names[4];
halfword text_dir_ptr;
halfword text_dir_tmp;

void initialize_directions(void)
{
    int k;
    pack_direction = -1;
    for (k = 0; k <= 7; k++) {
        dir_primary[k] = dir_T;
        dir_primary[k + 8] = dir_L;
        dir_primary[k + 16] = dir_B;
        dir_primary[k + 24] = dir_R;
    }
    for (k = 0; k <= 3; k++) {
        dir_secondary[k] = dir_L;
        dir_secondary[k + 4] = dir_R;
        dir_secondary[k + 8] = dir_T;
        dir_secondary[k + 12] = dir_B;

        dir_secondary[k + 16] = dir_L;
        dir_secondary[k + 20] = dir_R;
        dir_secondary[k + 24] = dir_T;
        dir_secondary[k + 28] = dir_B;
    }
    for (k = 0; k <= 7; k++) {
        dir_tertiary[k * 4] = dir_T;
        dir_tertiary[k * 4 + 1] = dir_L;
        dir_tertiary[k * 4 + 2] = dir_B;
        dir_tertiary[k * 4 + 3] = dir_R;
    }
    dir_rearrange[0] = 0;
    dir_rearrange[1] = 0;
    dir_rearrange[2] = 1;
    dir_rearrange[3] = 1;
    dir_names[0] = 'T';
    dir_names[1] = 'L';
    dir_names[2] = 'B';
    dir_names[3] = 'R';
}

halfword new_dir(int s)
{
    halfword p;                 /* the new node */
    p = new_node(whatsit_node, dir_node);
    dir_dir(p) = s;
    dir_dvi_ptr(p) = -1;
    dir_level(p) = cur_level;
    return p;
}

void print_dir(int d)
{
    print_char(dir_names[dir_primary[d]]);
    print_char(dir_names[dir_secondary[d]]);
    print_char(dir_names[dir_tertiary[d]]);
}
