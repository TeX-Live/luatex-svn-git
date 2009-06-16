/* pdfgen.h

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

#ifndef PDFGEN_H
#  define PDFGEN_H

#  define inf_pdf_mem_size 10000        /* min size of the |pdf_mem| array */
#  define sup_pdf_mem_size 10000000     /* max size of the |pdf_mem| array */

extern integer pdf_mem_size;
extern integer *pdf_mem;
extern integer pdf_mem_ptr;

extern void initialize_pdfgen (void);

extern integer pdf_get_mem(integer s);

/*
We use the similiar subroutines to handle the output buffer for
PDF output. When compress is used, the state of writing to buffer
is held in |zip_write_state|. We must write the header of PDF
output file in initialization to ensure that it will be the first
written bytes.
*/

#define pdf_op_buf_size 16384  /* size of the PDF output buffer */
#define inf_pdf_os_buf_size 1  /* initial value of |pdf_os_buf_size| */
#define sup_pdf_os_buf_size 5000000 /* arbitrary upper hard limit of |pdf_os_buf_size| */
#define pdf_os_max_objs 100 /* maximum number of objects in object stream */

/* The following macros are similar as for \.{DVI} buffer handling */

#define pdf_offset (pdf_gone + pdf_ptr) /* the file offset of last byte in PDF
                                           buffer that |pdf_ptr| points to */
typedef enum {
    no_zip=0, /* no \.{ZIP} compression */
    zip_writing=1, /* \.{ZIP} compression being used */
    zip_finish=2 /* finish \.{ZIP} compression */
} zip_write_states;

extern byte_file        pdf_file; 
extern real_eight_bits  *pdf_buf; 
extern integer          pdf_buf_size; 
extern longinteger      pdf_ptr; 
extern real_eight_bits  *pdf_op_buf; 
extern real_eight_bits  *pdf_os_buf; 
extern integer          pdf_os_buf_size; 
extern integer          *pdf_os_objnum; 
extern integer          *pdf_os_objoff; 
extern halfword         pdf_os_objidx; 
extern integer          pdf_os_cntr; 
extern integer          pdf_op_ptr; 
extern integer          pdf_os_ptr; 
extern boolean          pdf_os_mode; 
extern boolean          pdf_os_enable; 
extern integer          pdf_os_cur_objnum; 
extern longinteger      pdf_gone; 
extern longinteger      pdf_save_offset; 
extern integer          zip_write_state; 
extern integer          fixed_pdf_minor_version; 
extern boolean          fixed_pdf_minor_version_set; 
extern integer          fixed_pdf_objcompresslevel; 
extern integer          fixed_pdfoutput; 
extern boolean          fixed_pdfoutput_set; 
extern integer          fixed_gamma;
extern integer          fixed_image_gamma;
extern boolean          fixed_image_hicolor;
extern integer          fixed_image_apply_gamma;
extern integer          fixed_pdf_draftmode; 
extern integer          pdf_page_group_val;
extern integer          epochseconds;
extern integer          microseconds;
extern integer          page_divert_val;

extern void initialize_pdf_output (void);

extern void pdf_flush(void);

extern void check_pdfminorversion (void) ;
extern void ensure_pdf_open (void) ;

 /* output a byte to PDF buffer without checking of overflow */
#define pdf_quick_out(A) pdf_buf[pdf_ptr++]=A

/* do the same as |pdf_quick_out| and flush the PDF buffer if necessary */
#define pdf_out(A) do { pdf_room(1); pdf_quick_out(A); } while (0)


/*
Basic printing procedures for PDF output are very similiar to \TeX\ basic
printing ones but the output is going to PDF buffer. Subroutines with
suffix |_ln| append a new-line character to the PDF output.
*/

#define pdf_new_line_char 10 /* new-line character for UNIX platforms */

/* output a new-line character to PDF buffer */
#define pdf_print_nl() pdf_out(pdf_new_line_char) 

/* print out a string to PDF buffer followed by a new-line character */
#define pdf_print_ln(A) do {                    \
        pdf_print(A);                           \
        pdf_print_nl();                         \
    } while (0)

/* print out an integer to PDF buffer followed by a new-line character */
#define pdf_print_int_ln(A) do {                \
        pdf_print_int(A);                       \
        pdf_print_nl();                         \
    } while (0)


extern void pdf_print_char(internal_font_number f, integer cc);
extern void pdf_print(str_number s);
extern void pdf_print_int(longinteger n);
extern void pdf_print_real(integer m, integer d);
extern void pdf_print_str(str_number s);

#endif
