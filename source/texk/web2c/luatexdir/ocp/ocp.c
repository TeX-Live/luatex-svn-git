/* ocp.c
   
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
#include "luatex-api.h"
#include "commands.h"
#include "primitive.h"
#include "tokens.h"

static const char _svn_version[] = "$Id $ $URL $";

#define define(A,B,C) do { if (a>=4) geq_define(A,B,C); else eq_define(A,B,C); } while (0)

#define flush_string() do { decr(str_ptr); pool_ptr=str_start_macro(str_ptr); } while (0)

#define text(a) hash[(a)].rh

int **ocp_tables;

static int ocp_entries = 0;
integer ocp_maxint = 10000000;

/*
When the user defines \.{\\ocp\\f}, say, \TeX\ assigns an internal number
to the user's ocp~\.{\\f}. Adding this number to |ocp_id_base| gives the
|eqtb| location of a ``frozen'' control sequence that will always select
the ocp.
*/

internal_ocp_number ocp_ptr;    /* largest internal ocp number in use */

void new_ocp(small_number a)
{
    pointer u;                  /* user's ocp identifier */
    internal_ocp_number f;      /* runs through existing ocps */
    str_number t;               /* name for the frozen ocp identifier */
    str_number flushable_string;        /* string not yet referenced */
    boolean external_ocp = false;       /* external binary file */
    integer ocp_id_base = get_ocp_id_base();
    if (job_name == 0)
        open_log_file();
    /* avoid confusing \.{texput} with the ocp name */
    if (cur_chr == 1)
        external_ocp = true;
    get_r_token();
    u = cur_cs;
    if (u >= hash_base)
        t = text(u);
    else
        t = maketexstring("OCP");
    define(u, set_ocp_cmd, null_ocp);
    scan_optional_equals();
    scan_file_name();
    /* If this ocp has already been loaded, set |f| to the internal
       ocp number and |goto common_ending| */
    /* When the user gives a new identifier to a ocp that was previously loaded,
       the new name becomes the ocp identifier of record. OCP names `\.{xyz}' and
       `\.{XYZ}' are considered to be different.
     */
    flushable_string = str_ptr - 1;
    for (f = ocp_base + 1; f <= ocp_ptr; f++) {
        if (str_eq_str(ocp_name(f), cur_name)
            && str_eq_str(ocp_area(f), cur_area)) {
            if (cur_name == flushable_string) {
                flush_string();
                cur_name = ocp_name(f);
            }
            goto COMMON_ENDING;
        }
    }
    f = read_ocp_info(u, cur_name, cur_area, cur_ext, external_ocp);
  COMMON_ENDING:
    equiv(u) = f;
    zeqtb[ocp_id_base + f] = zeqtb[u];
    text(ocp_id_base + f) = t;
    if (ocp_trace_level == 1) {
        tprint_nl("");
        tprint_esc("ocp");
        print_esc(t);
        tprint("=");
        print(cur_name);
    }
}

/* Before we forget about the format of these tables, let's deal with
$\Omega$'s basic scanning routine related to ocp information. */

void scan_ocp_ident(void)
{
    internal_ocp_number f;
    do {
        get_x_token();
    } while (cur_cmd == spacer_cmd);

    if (cur_cmd == set_ocp_cmd) {
        f = cur_chr;
    } else {
        char *hlp[] = { "I was looking for a control sequence whose",
            "current meaning has been defined by \\ocp.",
            NULL
        };
        back_input();
        tex_error("Missing ocp identifier", hlp);
        f = null_ocp;
    }
    cur_val = f;
}

void allocate_ocp_table(int ocp_number, int ocp_size)
{
    int i;
    if (ocp_entries == 0) {
        ocp_tables = (int **) xmalloc(256 * sizeof(int **));
        ocp_entries = 256;
    } else if ((ocp_number == 256) && (ocp_entries == 256)) {
        ocp_tables = xrealloc(ocp_tables, 65536);
        ocp_entries = 65536;
    }
    ocp_tables[ocp_number] = (int *) xmalloc((1 + ocp_size) * sizeof(int));
    ocp_tables[ocp_number][0] = ocp_size;
    for (i = 1; i <= ocp_size; i++) {
        ocp_tables[ocp_number][i] = 0;
    }
}

static void dump_ocp_table(int ocp_number)
{
    dump_things(ocp_tables[ocp_number][0], ocp_tables[ocp_number][0] + 1);
}

void dump_ocp_info(void)
{
    integer k;
    integer ocp_id_base = get_ocp_id_base();
    dump_int(123456);
    dump_int(ocp_ptr);
    for (k = null_ocp; k <= ocp_ptr; k++) {
        dump_ocp_table(k);
        if (ocp_ptr - ocp_base > 0) {
            tprint_nl("\\ocp");
            print_esc(text(ocp_id_base + k));
            print_char('=');
            print_file_name(ocp_name(k), ocp_area(k), get_nullstr());
        }
    }
    dump_int(123456);
    if (ocp_ptr - ocp_base > 0) {
        print_ln();
        print_int(ocp_ptr - ocp_base);
        tprint(" preloaded ocp");
        if (ocp_ptr != ocp_base + 1)
            print_char('s');
    }
}

static void undump_ocp_table(int ocp_number)
{
    int sizeword;
    if (ocp_entries == 0) {
        ocp_tables = (int **) xmalloc(256 * sizeof(int **));
        ocp_entries = 256;
    } else if ((ocp_number == 256) && (ocp_entries == 256)) {
        ocp_tables = xrealloc(ocp_tables, 65536);
        ocp_entries = 65536;
    }
    undump_things(sizeword, 1);
    ocp_tables[ocp_number] = (int *) xmalloc((1 + sizeword) * sizeof(int));
    ocp_tables[ocp_number][0] = sizeword;
    undump_things(ocp_tables[ocp_number][1], sizeword);
}


void undump_ocp_info(void)
{
    integer k;
    integer x;
    undump_int(x);
    assert(x == 123456);
    undump_int(ocp_ptr);
    for (k = null_ocp; k <= ocp_ptr; k++)
        undump_ocp_table(k);
    undump_int(x);
    assert(x == 123456);
}
