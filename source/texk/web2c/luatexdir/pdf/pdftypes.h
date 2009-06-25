/* pdftypes.h

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

/* $Id$ */

#ifndef PDFTYPES_H
#  define PDFTYPES_H

/* This stucture holds everything that is needed for the actual pdf generation.

Because this structure interfaces with C++, it is not wise to use |boolean|
here (C++ has a boolean type built-in that is not compatible). Also, I have
plans to convert the backend code into a C library for use with e.g. standalone
lua. Together, this means that it is best only to use the standard C types and
the types explicitly defined in this header, and stay away from types like 
|integer| and |eight_bits| that are used elsewhere in the \LUATEX\ sources.

 */

typedef struct os_obj_data_ {
    int num;
    int off;
} os_obj_data;

typedef struct pdf_output_file_ {
    FILE *file;                 /* the PDF output file */
    /* generation parameters */
    int gamma;
    int image_gamma;
    int image_hicolor;          /* boolean */
    int image_apply_gamma;
    int draftmode;
    int pk_resolution;
    int decimal_digits;
    int gen_tounicode;
    int inclusion_copy_font;
    int replace_font;
    int minor_version;          /* fixed minor part of the PDF version */
    int minor_version_set;      /* flag if the PDF version has been set */
    int objcompresslevel;       /* fixed level for activating PDF object streams */

    /* output file buffering  */
    unsigned char *op_buf;      /* the PDF output buffer */
    int op_buf_size;            /* output buffer size (static) */
    int op_ptr;                 /* store for PDF buffer |pdf_ptr| while inside object streams */
    unsigned char *os_buf;      /* the PDF object stream buffer */
    int os_buf_size;            /* current size of the PDF object stream buffer, grows dynamically */
    int os_ptr;                 /* store for object stream |pdf_ptr| while outside object streams */

    os_obj_data *os_obj;        /* array of object stream objects */
    int os_idx;                 /* pointer into |pdf_os_objnum| and |pdf_os_objoff| */
    int os_cntr;                /* counter for object stream objects */
    int os_mode;                /* true if producing object stream */
    int os_enable;              /* true if object streams are globally enabled */
    int os_cur_objnum;          /* number of current object stream object */

    unsigned char *buf;         /* pointer to the PDF output buffer or PDF object stream buffer */
    int buf_size;               /* end of PDF output buffer or PDF object stream buffer */
    int ptr;                    /* pointer to the first unused byte in the PDF buffer or object stream buffer */
    off_t save_offset;          /* to save |pdf_offset| */
    off_t gone;                 /* number of bytes that were flushed to output */

    int zip_write_state;        /* which state of compression we are in */
    int pk_scale_factor;        /* this is just a preprocessed value that depends on 
                                   |pk_resolution| and |decimal_digits| */

} pdf_output_file;

typedef pdf_output_file *PDF;

#endif
