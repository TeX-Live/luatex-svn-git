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

#include "luatex-api.h"         /* for tokenlist_to_cstring */

static const char __svn_version[] =
    "$Id$"
    "$URL$";

/* write an action specification */
void write_action(PDF pdf, halfword p)
{
    char *s;
    integer d = 0;
    if (pdf_action_type(p) == pdf_action_user) {
        pdf_print_toks_ln(pdf, pdf_action_tokens(p));
        return;
    }
    pdf_printf(pdf, "<< ");
    if (pdf_action_file(p) != null) {
        pdf_printf(pdf, "/F ");
        s = tokenlist_to_cstring(pdf_action_file(p), true, NULL);
        pdf_print_str(pdf, s);
        xfree(s);
        pdf_printf(pdf, " ");
        if (pdf_action_new_window(p) > 0) {
            pdf_printf(pdf, "/NewWindow ");
            if (pdf_action_new_window(p) == 1)
                pdf_printf(pdf, "true ");
            else
                pdf_printf(pdf, "false ");
        }
    }
    switch (pdf_action_type(p)) {
    case pdf_action_page:
        if (pdf_action_file(p) == null) {
            pdf_printf(pdf, "/S /GoTo /D [");
            pdf_print_int(pdf, get_obj(pdf, obj_type_page, pdf_action_id(p), false));
            pdf_printf(pdf, " 0 R");
        } else {
            pdf_printf(pdf, "/S /GoToR /D [");
            pdf_print_int(pdf, pdf_action_id(p) - 1);
        }
        {
            char *tokstr =
                tokenlist_to_cstring(pdf_action_tokens(p), true, NULL);
            pdf_printf(pdf, " %s]", tokstr);
            xfree(tokstr);
        }
        break;
    case pdf_action_goto:
        if (pdf_action_file(p) == null) {
            pdf_printf(pdf, "/S /GoTo ");
            d = get_obj(pdf, obj_type_dest, pdf_action_id(p),
                        pdf_action_named_id(p));
        } else {
            pdf_printf(pdf, "/S /GoToR ");
        }
        if (pdf_action_named_id(p) > 0) {
            char *tokstr = tokenlist_to_cstring(pdf_action_id(p), true, NULL);
            pdf_str_entry(pdf, "D", tokstr);
            xfree(tokstr);
        } else if (pdf_action_file(p) == null) {
            pdf_indirect(pdf, "D", d);
        } else {
            pdf_error(maketexstring("ext4"),
                      maketexstring
                      ("`goto' option cannot be used with both `file' and `num'"));
        }
        break;
    case pdf_action_thread:
        pdf_printf(pdf, "/S /Thread ");
        if (pdf_action_file(p) == null) {
            d = get_obj(pdf, obj_type_thread, pdf_action_id(p),
                        pdf_action_named_id(p));
            if (pdf_action_named_id(p) > 0) {
                char *tokstr =
                    tokenlist_to_cstring(pdf_action_id(p), true, NULL);
                pdf_str_entry(pdf, "D", tokstr);
                xfree(tokstr);
            } else if (pdf_action_file(p) == null) {
                pdf_indirect(pdf, "D", d);
            } else {
                pdf_int_entry(pdf, "D", pdf_action_id(p));
            }
        }
        break;
    }
    pdf_printf(pdf, " >>\n");
}
