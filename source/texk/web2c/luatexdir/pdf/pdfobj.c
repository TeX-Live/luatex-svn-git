/* pdfobj.c
   
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

/* write a raw PDF object */
void pdf_write_obj(integer n)
{
    str_number s;
    byte_file f;
    integer data_size;          /* total size of the data file */
    integer data_cur;           /* index into |data_buffer| */
    real_eight_bits *data_buffer;       /* byte buffer for data files */
    boolean file_opened;
    integer k;
    boolean res;
    str_number fnam;
    integer callback_id;
    s = tokens_to_string(obj_obj_data(n));
    delete_token_ref(obj_obj_data(n));
    obj_obj_data(n) = null;
    if (obj_obj_is_stream(n) > 0) {
        pdf_begin_dict(n, 0);
        if (obj_obj_stream_attr(n) != null) {
            pdf_print_toks_ln(obj_obj_stream_attr(n));
            delete_token_ref(obj_obj_stream_attr(n));
            obj_obj_stream_attr(n) = null;
        }
        pdf_begin_stream();
    } else {
        pdf_begin_obj(n, 1);
    }
    if (obj_obj_is_file(n) > 0) {
        data_size = 0;
        data_cur = 0;
        data_buffer = 0;
        pack_file_name(s, get_nullstr(), get_nullstr());
        callback_id = callback_defined(find_data_file_callback);
        if (callback_id > 0) {
            res =
                run_callback(callback_id, "S->s", stringcast(nameoffile + 1),
                             addressof(fnam));
            if ((res) && (fnam != 0) && (str_length(fnam) > 0)) {
                /* Fixup |nameoffile| after callback */
                xfree(nameoffile);
                nameoffile =
                    xmallocarray(packed_ASCII_code, str_length(fnam) + 2);
                for (k = str_start_macro(fnam);
                     k >= str_start_macro(fnam + 1) - 1; k++)
                    nameoffile[k - str_start_macro(fnam) + 1] = str_pool[k];
                nameoffile[str_length(fnam) + 1] = 0;
                namelength = str_length(fnam);
                flush_string();
            }
        }
        callback_id = callback_defined(read_data_file_callback);
        if (callback_id > 0) {
            file_opened = false;
            res =
                run_callback(callback_id, "S->bSd", stringcast(nameoffile + 1),
                             addressof(file_opened), addressof(data_buffer),
                             addressof(data_size));
            if (!file_opened)
                pdf_error(maketexstring("ext5"),
                          maketexstring("cannot open file for embedding"));
        } else {
            if (!tex_b_open_in(f))
                pdf_error(maketexstring("ext5"),
                          maketexstring("cannot open file for embedding"));
            res =
                read_data_file(f, addressof(data_buffer), addressof(data_size));
            b_close(f);
        }
        if (!data_size)
            pdf_error(maketexstring("ext5"),
                      maketexstring("empty file for embedding"));
        if (!res)
            pdf_error(maketexstring("ext5"),
                      maketexstring("error reading file for embedding"));
        tprint("<<");
        print(s);
        while (data_cur < data_size) {
            pdf_out(data_buffer[data_cur]);
            incr(data_cur);
        }
        if (data_buffer != 0)
            xfree(data_buffer);
        tprint(">>");
    } else if (obj_obj_is_stream(n) > 0) {
        pdf_print(s);
    } else {
        pdf_print_ln(s);
    }
    if (obj_obj_is_stream(n) > 0)
        pdf_end_stream();
    else
        pdf_end_obj();
    flush_str(s);
}
