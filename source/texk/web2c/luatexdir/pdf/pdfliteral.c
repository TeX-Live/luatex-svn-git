/* pdfliteral.c
   
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

void pdf_special(PDF pdf, halfword p)
{
    integer old_setting;        /* holds print |selector| */
    str_number s;
    old_setting = selector;
    selector = new_string;
    show_token_list(token_link(write_tokens(p)), null, pool_size - pool_ptr);
    selector = old_setting;
    s = make_string();
    pdf_literal(pdf, s, scan_special, true);
    flush_str(s);
}


/*
To ship out a \TeX\ box to PDF page description we need to implement
|pdf_hlist_out|, |pdf_vlist_out| and |pdf_ship_out|, which are equivalent to
the \TeX' original |hlist_out|, |vlist_out| and |ship_out| resp. But first we
need to declare some procedures needed in |pdf_hlist_out| and |pdf_vlist_out|.
*/

void pdf_out_literal(PDF pdf, halfword p)
{
    integer old_setting;        /* holds print |selector| */
    str_number s;
    if (pdf_literal_type(p) == normal) {
        old_setting = selector;
        selector = new_string;
        show_token_list(token_link(pdf_literal_data(p)), null,
                        pool_size - pool_ptr);
        selector = old_setting;
        s = make_string();
        pdf_literal(pdf, s, pdf_literal_mode(p), false);
        flush_str(s);
    } else {
        assert(pdf_literal_mode(p) != scan_special);
        switch (pdf_literal_mode(p)) {
        case set_origin:
            pdf_goto_pagemode(pdf);
            pdf_set_pos(pdf, pdf->posstruct->pos);
            break;
        case direct_page:
            pdf_goto_pagemode(pdf);
            break;
        case direct_always:
            pdf_end_string_nl(pdf);
            break;
        default:
            confusion("literal1");
            break;
        }
        lua_pdf_literal(pdf, pdf_literal_data(p));
    }
}

/* test equality of start of strings */
static boolean str_in_cstr(str_number s, char *r, unsigned i)
{
    pool_pointer j;             /* running indices */
    unsigned char *k;
    if ((unsigned) str_length(s) < i + strlen(r))
        return false;
    j = i + str_start_macro(s);
    k = (unsigned char *) r;
    while ((j < str_start_macro(s + 1)) && (*k)) {
        if (str_pool[j] != *k)
            return false;
        j++;
        k++;
    }
    return true;
}

void pdf_literal(PDF pdf, str_number s, integer literal_mode, boolean warn)
{
    pool_pointer j = 0;         /* current character code position, initialized to make the compiler happy */
    if (s > STRING_OFFSET) {    /* needed for |out_save| */
        j = str_start_macro(s);
        if (literal_mode == scan_special) {
            if (!(str_in_cstr(s, "PDF:", 0) || str_in_cstr(s, "pdf:", 0))) {
                if (warn
                    &&
                    ((!(str_in_cstr(s, "SRC:", 0) || str_in_cstr(s, "src:", 0)))
                     || (str_length(s) == 0)))
                    tprint_nl("Non-PDF special ignored!");
                return;
            }
            j = j + strlen("PDF:");
            if (str_in_cstr(s, "direct:", strlen("PDF:"))) {
                j = j + strlen("direct:");
                literal_mode = direct_always;
            } else if (str_in_cstr(s, "page:", strlen("PDF:"))) {
                j = j + strlen("page:");
                literal_mode = direct_page;
            } else {
                literal_mode = set_origin;
            }
        }
    }
    switch (literal_mode) {
    case set_origin:
        pdf_goto_pagemode(pdf);
        pdf_set_pos(pdf, pdf->posstruct->pos);
        break;
    case direct_page:
        pdf_goto_pagemode(pdf);
        break;
    case direct_always:
        pdf_end_string_nl(pdf);
        break;
    default:
        confusion("literal1");
        break;
    }
    if (s > STRING_OFFSET) {
        while (j < str_start_macro(s + 1))
            pdf_out(pdf, str_pool[j++]);
    } else {
        pdf_out(pdf, s);
    }
    pdf_print_nl(pdf);
}
