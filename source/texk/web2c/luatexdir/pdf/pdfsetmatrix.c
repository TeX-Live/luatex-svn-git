/* pdfsetmatrix.c
   
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

void pdf_out_setmatrix(halfword p)
{
    integer old_setting;        /* holds print |selector| */
    str_number s;
    old_setting = selector;
    selector = new_string;
    show_token_list(fixmem[(pdf_setmatrix_data(p))].hhrh, null,
                    pool_size - pool_ptr);
    selector = old_setting;
    str_room(7);
    str_pool[pool_ptr] = 0;     /* make C string for pdfsetmatrix  */
    pos = synch_p_with_c(cur);
    pdfsetmatrix(str_start_macro(str_ptr), pos);
    str_room(7);
    append_char(' ');
    append_char('0');
    append_char(' ');
    append_char('0');
    append_char(' ');
    append_char('c');
    append_char('m');
    s = make_string();
    pdf_literal(s, set_origin, false);
    flush_str(s);
}
