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

static const char __svn_version[] =
    "$Id$"
    "$URL$";

#include "ptexlib.h"
#include "lua/luatex-api.h"

integer pdf_last_obj;

void pdf_check_obj(PDF pdf, integer t, integer n)
{
    integer k;
    k = pdf->head_tab[t];
    while ((k != 0) && (k != n))
        k = obj_link(pdf, k);
    if (k == 0)
        pdf_error("ext1", "cannot find referenced object");
}

/* write a raw PDF object */

void pdf_write_obj(PDF pdf, integer n)
{
    char *s;
    int saved_compress_level = pdf->compress_level;
    int os_level = 1;           /* gives compressed objects for \pdfobjcompresslevel > 0 */
    int l = 0; /* possibly a lua registry reference */
    if ((obj_obj_is_stream(pdf, n) == 2) || (obj_obj_is_file(pdf, n) == 2)) {
        /* this value indicates a lua reference */
        l = obj_obj_data(pdf, n);
        lua_rawgeti(Luas, LUA_REGISTRYINDEX, l);
        s = (char *)lua_tostring(Luas,-1);
    } else {
        s = tokenlist_to_cstring(obj_obj_data(pdf, n), true, NULL);
        delete_token_ref(obj_obj_data(pdf, n));
    }
    set_obj_obj_data(pdf, n, null);
    if (obj_obj_pdfcompresslevel(pdf, n) > -1) {        /* -1 = "unset" */
        pdf->compress_level = obj_obj_pdfcompresslevel(pdf, n);
        if (obj_obj_pdfcompresslevel(pdf, n) == 0)
            os_level = 0;
    }
    if (obj_obj_is_stream(pdf, n) > 0) {
        pdf_begin_dict(pdf, n, 0);
        if (obj_obj_stream_attr(pdf, n) != null) {
            pdf_print_toks_ln(pdf, obj_obj_stream_attr(pdf, n));
            delete_token_ref(obj_obj_stream_attr(pdf, n));
            set_obj_obj_stream_attr(pdf, n, null);
        }
        pdf_begin_stream(pdf);
    } else
        pdf_begin_obj(pdf, n, os_level);
    if (obj_obj_is_file(pdf, n) > 0) {
        integer data_size = 0;  /* total size of the data file */
        integer data_cur = 0;   /* index into |data_buffer| */
        eight_bits *data_buffer = NULL; /* byte buffer for data files */
        boolean res = false;    /* callback status value */
        char *fnam = NULL;      /* callback found filename */
        integer callback_id;
        fnam = luatex_find_file(s, find_data_file_callback);
        callback_id = callback_defined(read_data_file_callback);
        if (fnam && callback_id > 0) {
            boolean file_opened = false;
            res = run_callback(callback_id, "S->bSd", fnam,
                               &file_opened, &data_buffer, &data_size);
            if (!file_opened)
                pdf_error("ext5", "cannot open file for embedding");
        } else {
            byte_file f;        /* the data file's FILE* */
            if (!fnam)
                fnam = s;
            if (!luatex_open_input
                (&f, fnam, kpse_tex_format, FOPEN_RBIN_MODE, true))
                pdf_error("ext5", "cannot open file for embedding");
            res = read_data_file(f, &data_buffer, &data_size);
            close_file(f);
        }
        if (!data_size)
            pdf_error("ext5", "empty file for embedding");
        if (!res)
            pdf_error("ext5", "error reading file for embedding");
        tprint("<<");
        tprint(s);
        for (data_cur = 0; data_cur < data_size; data_cur++) {
            pdf_out(pdf, data_buffer[data_cur]);
        }
        if (data_buffer != NULL)
            xfree(data_buffer);
        tprint(">>");
    } else if (obj_obj_is_stream(pdf, n) > 0) {
        pdf_puts(pdf, s);
    } else {
        pdf_puts(pdf, s);
        pdf_print_nl(pdf);
    }
    if (obj_obj_is_stream(pdf, n) > 0)
        pdf_end_stream(pdf);
    else
        pdf_end_obj(pdf);
    if (l>0) {
        /* doing this here removes the need for |strdup()| earlier */
        lua_pop(Luas,1);
        luaL_unref(Luas, LUA_REGISTRYINDEX, l);
    } else {
        xfree(s);
    }
    pdf->compress_level = saved_compress_level;
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
        incr(pdf->obj_count);
        pdf_create_obj(pdf, obj_type_obj, pdf->obj_count);
        pdf_last_obj = pdf->obj_ptr;
    } else {
        k = -1;
        if (scan_keyword("useobjnum")) {
            scan_int();
            k = cur_val;
            if ((k <= 0) || (k > pdf->obj_ptr) || (obj_data_ptr(pdf, k) != 0)) {
                pdf_warning("\\pdfobj",
                            "invalid object number being ignored", true, true);
                pdf_retval = -1;        /* signal the problem */
                k = -1;         /* will be generated again */
            }
        }
        if (k < 0) {
            incr(pdf->obj_count);
            pdf_create_obj(pdf, obj_type_obj, pdf->obj_count);
            k = pdf->obj_ptr;
        }
        set_obj_data_ptr(pdf, k, pdf_get_mem(pdf, pdfmem_obj_size));
        if (scan_keyword("uncompressed"))
            set_obj_obj_pdfcompresslevel(pdf, k, 0);
        else
            set_obj_obj_pdfcompresslevel(pdf, k, -1);
        if (scan_keyword("stream")) {
            set_obj_obj_is_stream(pdf, k, 1);
            if (scan_keyword("attr")) {
                scan_pdf_ext_toks();
                set_obj_obj_stream_attr(pdf, k, def_ref);
            } else {
                set_obj_obj_stream_attr(pdf, k, null);
            }
        } else {
            set_obj_obj_is_stream(pdf, k, 0);
        }
        if (scan_keyword("file"))
            set_obj_obj_is_file(pdf, k, 1);
        else
            set_obj_obj_is_file(pdf, k, 0);
        scan_pdf_ext_toks();
        set_obj_obj_data(pdf, k, def_ref);
        pdf_last_obj = k;
    }
}

void pdf_ref_obj(PDF pdf __attribute__ ((unused)), halfword p)
{
    if (!is_obj_scheduled(pdf, pdf_obj_objnum(p))) {
        append_object_list(pdf, obj_type_obj, pdf_obj_objnum(p));
        set_obj_scheduled(pdf, pdf_obj_objnum(p));
    }
}
