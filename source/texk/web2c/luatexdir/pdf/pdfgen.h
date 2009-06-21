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
extern str_number pdf_resname_prefix;

extern void initialize_pdfgen(void);

extern integer pdf_get_mem(integer s);

/*
We use the similiar subroutines to handle the output buffer for
PDF output. When compress is used, the state of writing to buffer
is held in |zip_write_state|. We must write the header of PDF
output file in initialization to ensure that it will be the first
written bytes.
*/

#  define pdf_op_buf_size 16384 /* size of the PDF output buffer */
#  define inf_pdf_os_buf_size 1 /* initial value of |pdf_os_buf_size| */
#  define sup_pdf_os_buf_size 5000000   /* arbitrary upper hard limit of |pdf_os_buf_size| */
#  define pdf_os_max_objs 100   /* maximum number of objects in object stream */

/* The following macros are similar as for \.{DVI} buffer handling */

#  define pdf_offset (pdf_gone + pdf_ptr)
                                        /* the file offset of last byte in PDF
                                           buffer that |pdf_ptr| points to */
typedef enum {
    no_zip = 0,                 /* no \.{ZIP} compression */
    zip_writing = 1,            /* \.{ZIP} compression being used */
    zip_finish = 2              /* finish \.{ZIP} compression */
} zip_write_states;

extern byte_file pdf_file;
extern real_eight_bits *pdf_buf;
extern integer pdf_buf_size;
extern longinteger pdf_ptr;
extern real_eight_bits *pdf_op_buf;
extern real_eight_bits *pdf_os_buf;
extern integer pdf_os_buf_size;
extern integer *pdf_os_objnum;
extern integer *pdf_os_objoff;
extern halfword pdf_os_objidx;
extern integer pdf_os_cntr;
extern integer pdf_op_ptr;
extern integer pdf_os_ptr;
extern boolean pdf_os_mode;
extern boolean pdf_os_enable;
extern integer pdf_os_cur_objnum;
extern longinteger pdf_gone;
extern longinteger pdf_save_offset;
extern integer zip_write_state;
extern integer fixed_pdf_minor_version;
extern boolean fixed_pdf_minor_version_set;
extern integer fixed_pdf_objcompresslevel;
extern integer fixed_pdfoutput;
extern boolean fixed_pdfoutput_set;
extern integer fixed_gamma;
extern integer fixed_image_gamma;
extern boolean fixed_image_hicolor;
extern integer fixed_image_apply_gamma;
extern integer fixed_pdf_draftmode;
extern integer pdf_page_group_val;
extern integer epochseconds;
extern integer microseconds;
extern integer page_divert_val;

extern void initialize_pdf_output(void);

extern void pdf_flush(void);
extern void pdf_os_get_os_buf(integer s);
extern void pdf_room(integer n);

extern void check_pdfminorversion(void);
extern void ensure_pdf_open(void);

 /* output a byte to PDF buffer without checking of overflow */
#  define pdf_quick_out(A) pdf_buf[pdf_ptr++]=A

/* do the same as |pdf_quick_out| and flush the PDF buffer if necessary */
#  define pdf_out(A) do { pdf_room(1); pdf_quick_out(A); } while (0)

/*
Basic printing procedures for PDF output are very similiar to \TeX\ basic
printing ones but the output is going to PDF buffer. Subroutines with
suffix |_ln| append a new-line character to the PDF output.
*/

#  define pdf_new_line_char 10  /* new-line character for UNIX platforms */

/* output a new-line character to PDF buffer */
#  define pdf_print_nl() pdf_out(pdf_new_line_char)

/* print out a string to PDF buffer followed by a new-line character */
#  define pdf_print_ln(A) do {                    \
        pdf_print(A);                           \
        pdf_print_nl();                         \
    } while (0)

/* print out an integer to PDF buffer followed by a new-line character */
#  define pdf_print_int_ln(A) do {                \
        pdf_print_int(A);                       \
        pdf_print_nl();                         \
    } while (0)

extern void pdf_puts(const char *);
extern __attribute__ ((format(printf, 1, 2)))
void pdf_printf(const char *, ...);

extern void pdf_print_char(internal_font_number f, integer cc);
extern void pdf_print(str_number s);
extern void pdf_print_int(longinteger n);
extern void pdf_print_real(integer m, integer d);
extern void pdf_print_str(char *s);

extern void pdf_begin_stream(void);
extern void pdf_end_stream(void);
extern void pdf_remove_last_space(void);

extern scaled one_hundred_inch;
extern scaled one_inch;
extern scaled one_true_inch;
extern scaled one_hundred_bp;
extern scaled one_bp;
extern integer ten_pow[10];

extern scaled round_xn_over_d(scaled x, integer n, integer d);
extern void pdf_print_bp(scaled s);
extern void pdf_print_mag_bp(scaled s);

extern integer fixed_pk_resolution;
extern integer fixed_decimal_digits;
extern integer fixed_gen_tounicode;
extern integer fixed_inclusion_copy_font;
extern integer fixed_replace_font;
extern integer pk_scale_factor;
extern integer pdf_output_option;
extern integer pdf_output_value;
extern integer pdf_draftmode_option;
extern integer pdf_draftmode_value;

#  define set_ff(A)  do {                         \
        if (pdf_font_num(A) < 0)                \
            ff = -pdf_font_num(A);              \
        else                                    \
            ff = A;                             \
    } while (0)

#  define pdf_print_resname_prefix() do {         \
        if (pdf_resname_prefix != 0)            \
            pdf_print(pdf_resname_prefix);      \
    } while (0)

extern void pdf_print_fw_int(longinteger n, integer w);
extern void pdf_out_bytes(longinteger n, integer w);
extern void pdf_int_entry(char  *s, integer v);
extern void pdf_int_entry_ln(char *s, integer v);
extern void pdf_indirect(char *s, integer o);
extern void pdf_indirect_ln(char *s, integer o);
extern void pdf_print_str_ln(char *s);
extern void pdf_str_entry(char *s, char *v);
extern void pdf_str_entry_ln(char *s, char *v);

extern void pdf_print_toks(halfword p);
extern void pdf_print_toks_ln(halfword p);

extern void pdf_print_rect_spec(halfword r);
extern void pdf_rectangle(halfword r);

extern void pdf_begin_obj(integer i, integer pdf_os_level);
extern void pdf_new_obj(integer t, integer i, integer pdf_os);
extern void pdf_end_obj(void);

extern void pdf_begin_dict(integer i, integer pdf_os_level);
extern void pdf_new_dict(integer t, integer i, integer pdf_os);
extern void pdf_end_dict(void);

extern void pdf_os_write_objstream(void);

extern void write_stream_length(integer, longinteger);
extern char *convertStringToPDFString(const char *in, int len);
extern void escapestring(pool_pointer in);
extern void escapename(pool_pointer in);

extern void print_ID(str_number);
extern void init_start_time();
extern void print_creation_date();
extern void print_mod_date();
extern void getcreationdate(void);

extern void pdf_use_font(internal_font_number f, integer fontnum);
extern void pdf_init_font(internal_font_number f);
extern internal_font_number pdf_set_font(internal_font_number f);


#endif
