/* pdfcolorstack.c
   
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


void pdf_out_colorstack(halfword p)
{
    integer old_setting;
    str_number s;
    integer cmd;
    integer stack_no;
    integer literal_mode;
    cmd = pdf_colorstack_cmd(p);
    stack_no = pdf_colorstack_stack(p);
    literal_mode = 0;
    if (stack_no >= colorstackused()) {
        tprint_nl("");
        tprint("Color stack ");
        print_int(stack_no);
        tprint(" is not initialized for use!");
        tprint_nl("");
        return;
    }
    switch (cmd) {
    case colorstack_set:
    case colorstack_push:
        old_setting = selector;
        selector = new_string;
        show_token_list(fixmem[(pdf_colorstack_data(p))].hhrh, null,
                        pool_size - pool_ptr);
        selector = old_setting;
        s = make_string();
        if (cmd == colorstack_set)
            literal_mode = colorstackset(stack_no, s);
        else
            literal_mode = colorstackpush(stack_no, s);
        if (str_length(s) > 0)
            pdf_literal(s, literal_mode, false);
        flush_str(s);
        return;
        break;
    case colorstack_pop:
        literal_mode = colorstackpop(stack_no);
        break;
    case colorstack_current:
        literal_mode = colorstackcurrent(stack_no);
        break;
    default:
        break;
    }
    if (cur_length > 0) {
        s = make_string();
        pdf_literal(s, literal_mode, false);
        flush_str(s);
    }
}

void pdf_out_colorstack_startpage(void)
{
    integer i;
    integer max;
    integer start_status;
    integer literal_mode;
    str_number s;
    i = 0;
    max = colorstackused();
    while (i < max) {
        start_status = colorstackskippagestart(i);
        if (start_status == 0) {
            literal_mode = colorstackcurrent(i);
            if (cur_length > 0) {
                s = make_string();
                pdf_literal(s, literal_mode, false);
                flush_str(s);
            }
        }
        incr(i);
    }
}
