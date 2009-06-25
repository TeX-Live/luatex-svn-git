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

#include "commands.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";

halfword pdf_obj_list;          /* list of objects in the current page */
integer pdf_obj_count;          /* counter of objects */
integer pdf_last_obj;



/* write a raw PDF object */
void pdf_write_obj(PDF pdf, integer n)
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
    s = tokens_to_string(obj_obj_data(pdf,n));
    delete_token_ref(obj_obj_data(pdf,n));
    set_obj_obj_data(pdf,n,null);
    if (obj_obj_is_stream(pdf,n) > 0) {
        pdf_begin_dict(pdf, n, 0);
        if (obj_obj_stream_attr(pdf,n) != null) {
            pdf_print_toks_ln(pdf, obj_obj_stream_attr(pdf,n));
            delete_token_ref(obj_obj_stream_attr(pdf,n));
            set_obj_obj_stream_attr(pdf,n, null);
        }
        pdf_begin_stream(pdf);
    } else {
        pdf_begin_obj(pdf, n, 1);
    }
    if (obj_obj_is_file(pdf,n) > 0) {
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
            pdf_out(pdf, data_buffer[data_cur]);
            incr(data_cur);
        }
        if (data_buffer != 0)
            xfree(data_buffer);
        tprint(">>");
    } else if (obj_obj_is_stream(pdf,n) > 0) {
        pdf_print(pdf, s);
    } else {
        pdf_print_ln(pdf, s);
    }
    if (obj_obj_is_stream(pdf,n) > 0)
        pdf_end_stream(pdf);
    else
        pdf_end_obj(pdf);
    flush_str(s);
}

/* The \.{\\pdfobj} primitive is used to create a ``raw'' object in the PDF
   output file. The object contents will be hold in memory and will be written
   out only when the object is referenced by \.{\\pdfrefobj}. When \.{\\pdfobj}
   is used with \.{\\immediate}, the object contents will be written out
   immediately. Objects referenced in the current page are appended into
   |pdf_obj_list|. */

void scan_obj(PDF pdf)
{
    integer k;
    if (scan_keyword("reserveobjnum")) {
        /* Scan an optional space */
        get_x_token();
        if (cur_cmd != spacer_cmd)
            back_input();
        incr(pdf_obj_count);
        pdf_create_obj(obj_type_obj, pdf_obj_count);
        pdf_last_obj = obj_ptr;
    } else {
        k = -1;
        if (scan_keyword("useobjnum")) {
            scan_int();
            k = cur_val;
            if ((k <= 0) || (k > obj_ptr) || (obj_data_ptr(k) != 0)) {
                pdf_warning(maketexstring("\\pdfobj"),
                            maketexstring
                            ("invalid object number being ignored"), true,
                            true);
                pdf_retval = -1;        /* signal the problem */
                k = -1;         /* will be generated again */
            }
        }
        if (k < 0) {
            incr(pdf_obj_count);
            pdf_create_obj(obj_type_obj, pdf_obj_count);
            k = obj_ptr;
        }
        set_obj_data_ptr(k, pdf_get_mem(pdf,pdfmem_obj_size));
        if (scan_keyword("stream")) {
            set_obj_obj_is_stream(pdf,k, 1);
            if (scan_keyword("attr")) {
                scan_pdf_ext_toks();
                set_obj_obj_stream_attr(pdf,k, def_ref);
            } else {
                set_obj_obj_stream_attr(pdf,k, null);
            }
        } else {
            set_obj_obj_is_stream(pdf,k, 0);
        }
        if (scan_keyword("file"))
            set_obj_obj_is_file(pdf,k, 1);
        else
            set_obj_obj_is_file(pdf,k, 0);
        scan_pdf_ext_toks();
        set_obj_obj_data(pdf,k, def_ref);
        pdf_last_obj = k;
    }
}
