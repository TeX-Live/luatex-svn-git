/* pdfaction.c
   
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

static const char __svn_version[] = "$Id$" "$URL$";

/* write an action specification */
void write_action(halfword p)
{
    str_number s;
    integer d = 0;
    if (pdf_action_type(p) == pdf_action_user) {
        pdf_print_toks_ln(pdf_action_tokens(p));
        return;
    }
    pdf_printf("<< ");
    if (pdf_action_file(p) != null) {
        pdf_printf("/F ");
        s = tokens_to_string(pdf_action_file(p));
        if ((str_pool[str_start_macro(s)] == 40) &&
            (str_pool[str_start_macro(s) + str_length(s) - 1] == 41)) {
            pdf_print(s);
        } else {
            pdf_print_str(s);
        }
        flush_str(s);
        pdf_printf(" ");
        if (pdf_action_new_window(p) > 0) {
            pdf_printf("/NewWindow ");
            if (pdf_action_new_window(p) == 1)
                pdf_printf("true ");
            else
                pdf_printf("false ");
        }
    }
    switch (pdf_action_type(p)) {
    case pdf_action_page:
        if (pdf_action_file(p) == null) {
            pdf_printf("/S /GoTo /D [");
            pdf_print_int(get_obj(obj_type_page, pdf_action_id(p), false));
            pdf_printf(" 0 R");
        } else {
            pdf_printf("/S /GoToR /D [");
            pdf_print_int(pdf_action_id(p) - 1);
        }
        pdf_out(' ');
        pdf_print(tokens_to_string(pdf_action_tokens(p)));
        flush_str(last_tokens_string);
        pdf_out(']');
        break;
    case pdf_action_goto:
        if (pdf_action_file(p) == null) {
            pdf_printf("/S /GoTo ");
            d = get_obj(obj_type_dest, pdf_action_id(p),
                        pdf_action_named_id(p));
        } else {
            pdf_printf("/S /GoToR ");
        }
        if (pdf_action_named_id(p) > 0) {
            pdf_str_entry('D', tokens_to_string(pdf_action_id(p)));
            flush_str(last_tokens_string);
        } else if (pdf_action_file(p) == null) {
            pdf_indirect('D', d);
        } else {
            pdf_error(maketexstring("ext4"),
                      maketexstring
                      ("`goto' option cannot be used with both `file' and `num'"));
        }
        break;
    case pdf_action_thread:
        pdf_printf("/S /Thread ");
        if (pdf_action_file(p) == null) {
            d = get_obj(obj_type_thread, pdf_action_id(p),
                        pdf_action_named_id(p));
            if (pdf_action_named_id(p) > 0) {
                pdf_str_entry('D', tokens_to_string(pdf_action_id(p)));
                flush_str(last_tokens_string);
            } else if (pdf_action_file(p) == null) {
                pdf_indirect('D', d);
            } else {
                pdf_int_entry('D', pdf_action_id(p));
            }
        }
        break;
    }
    pdf_printf(" >>\n");
}
