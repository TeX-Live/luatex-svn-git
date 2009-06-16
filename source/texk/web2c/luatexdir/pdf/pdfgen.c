/* pdfgen.c
   
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
#include <ctype.h>
#include "commands.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";


/*
Sometimes it is neccesary to allocate memory for PDF output that cannot
be deallocated then, so we use |pdf_mem| for this purpose.
*/

integer pdf_mem_size = inf_pdf_mem_size;        /* allocated size of |pdf_mem| array */
integer *pdf_mem;
/* the first word is not used so we can use zero as a value for testing
   whether a pointer to |pdf_mem| is valid  */
integer pdf_mem_ptr = 1;

byte_file        pdf_file; /* the PDF output file */
real_eight_bits *pdf_buf; /* pointer to the PDF output buffer or PDF object stream buffer */
integer          pdf_buf_size=pdf_op_buf_size; /* end of PDF output buffer or PDF object stream buffer */
longinteger      pdf_ptr=0; /* pointer to the first unused byte in the PDF buffer or object stream buffer */
real_eight_bits *pdf_op_buf; /* the PDF output buffer */
real_eight_bits *pdf_os_buf; /* the PDF object stream buffer */
integer          pdf_os_buf_size=inf_pdf_os_buf_size; /* current size of the PDF object stream buffer, grows dynamically */
integer         *pdf_os_objnum; /* array of object numbers within object stream */
integer         *pdf_os_objoff; /* array of object offsets within object stream */
halfword         pdf_os_objidx; /* pointer into |pdf_os_objnum| and |pdf_os_objoff| */
integer          pdf_os_cntr=0; /* counter for object stream objects */
integer          pdf_op_ptr=0; /* store for PDF buffer |pdf_ptr| while inside object streams */
integer          pdf_os_ptr=0; /* store for object stream |pdf_ptr| while outside object streams */
boolean          pdf_os_mode=false; /* true if producing object stream */
boolean          pdf_os_enable; /* true if object streams are globally enabled */
integer          pdf_os_cur_objnum=0; /* number of current object stream object */
longinteger      pdf_gone=0; /* number of bytes that were flushed to output */
longinteger      pdf_save_offset; /* to save |pdf_offset| */
integer zip_write_state=no_zip; /* which state of compression we are in */
integer fixed_pdf_minor_version; /* fixed minor part of the PDF version */
boolean fixed_pdf_minor_version_set=false; /* flag if the PDF version has been set */
integer fixed_pdf_objcompresslevel; /* fixed level for activating PDF object streams */
integer fixed_pdfoutput; /* fixed output format */
boolean fixed_pdfoutput_set=false; /* |fixed_pdfoutput| has been set? */
integer fixed_gamma;
integer fixed_image_gamma;
boolean fixed_image_hicolor;
integer fixed_image_apply_gamma;
integer fixed_pdf_draftmode; /* fixed \\pdfdraftmode */
integer pdf_page_group_val=-1;
integer epochseconds;
integer microseconds;
integer page_divert_val=0;

void initialize_pdfgen (void)
{
    pdf_buf = pdf_op_buf;
}

#define pdf_minor_version        int_par(param_pdf_minor_version_code)
#define pdf_decimal_digits       int_par(param_pdf_decimal_digits_code)
#define pdf_gamma                int_par(param_pdf_gamma_code)
#define pdf_image_gamma          int_par(param_pdf_image_gamma_code)
#define pdf_image_hicolor        int_par(param_pdf_image_hicolor_code)
#define pdf_image_apply_gamma    int_par(param_pdf_image_apply_gamma_code)
#define pdf_objcompresslevel     int_par(param_pdf_objcompresslevel_code)
#define pdf_draftmode            int_par(param_pdf_draftmode_code)
#define pdf_inclusion_copy_font  int_par(param_pdf_inclusion_copy_font_code)
#define pdf_replace_font         int_par(param_pdf_replace_font_code)
#define pdf_pk_resolution        int_par(param_pdf_pk_resolution_code)
#define pdf_pk_mode              int_par(param_pdf_pk_mode_code)
#define pdf_unique_resname       int_par(param_pdf_unique_resname_code)
#define pdf_compress_level       int_par(param_pdf_compress_level_code)

void initialize_pdf_output (void)
{
    if ((pdf_minor_version < 0) || (pdf_minor_version > 9)) {
        char *hlp[] = 
            { "The pdfminorversion must be between 0 and 9.",
              "I changed this to 4.", NULL };
        char msg [256];
        snprintf(msg,255, "LuaTeX error (illegal pdfminorversion %d)", (int)pdf_minor_version);
        tex_error(msg,hlp);
        pdf_minor_version = 4;
    }
    fixed_pdf_minor_version = pdf_minor_version;
    fixed_decimal_digits    = fix_int(pdf_decimal_digits, 0, 4);
    fixed_gamma             = fix_int(pdf_gamma, 0, 1000000);
    fixed_image_gamma       = fix_int(pdf_image_gamma, 0, 1000000);
    fixed_image_hicolor     = fix_int(pdf_image_hicolor, 0, 1);
    fixed_image_apply_gamma = fix_int(pdf_image_apply_gamma, 0, 1);
    fixed_pdf_objcompresslevel = fix_int(pdf_objcompresslevel, 0, 3);
    fixed_pdf_draftmode     = fix_int(pdf_draftmode, 0, 1);
    fixed_inclusion_copy_font = fix_int(pdf_inclusion_copy_font, 0, 1);
    fixed_replace_font      = fix_int(pdf_replace_font, 0, 1);
    fixed_pk_resolution     = fix_int(pdf_pk_resolution, 72, 8000);
    if ((fixed_pdf_minor_version >= 5) && (fixed_pdf_objcompresslevel > 0)) {
        pdf_os_enable = true;
    } else {
        if (fixed_pdf_objcompresslevel > 0 ) {
            pdf_warning(maketexstring("Object streams"),
                        maketexstring("\\pdfobjcompresslevel > 0 requires \\pdfminorversion > 4. Object streams disabled now."), true, true);
            fixed_pdf_objcompresslevel = 0;
        }
        pdf_os_enable = false;
    }
    if (pdf_pk_resolution == 0)  /* if not set from format file or by user */
        pdf_pk_resolution = pk_dpi; /* take it from \.{texmf.cnf} */
    pk_scale_factor = divide_scaled(72, fixed_pk_resolution, 5 + fixed_decimal_digits);
    if (!callback_defined(read_pk_file_callback)) {
        if (pdf_pk_mode !=  null) {
            kpseinitprog("PDFTEX", fixed_pk_resolution,
                         makecstring(tokens_to_string(pdf_pk_mode)), nil);
            flush_string();
        } else {
            kpseinitprog("PDFTEX", fixed_pk_resolution, nil, nil);
        }
        if (!kpsevarvalue("MKTEXPK")) 
            kpsesetprogramenabled (kpsepkformat, 1, kpsesrccmdline);
    }
    set_job_id(int_par(param_year_code), 
               int_par(param_month_code), 
               int_par(param_day_code), 
               int_par(param_time_code));

    if ((pdf_unique_resname > 0) && (pdf_resname_prefix == 0) )
        pdf_resname_prefix = get_resname_prefix();
    pdf_page_init();
}

/*
  We use |pdf_get_mem| to allocate memory in |pdf_mem|
*/

integer pdf_get_mem(integer s)
{                               /* allocate |s| words in |pdf_mem| */
    integer a;
    integer ret;
    if (s > sup_pdf_mem_size - pdf_mem_ptr)
        overflow(maketexstring("PDF memory size (pdf_mem_size)"), pdf_mem_size);
    if (pdf_mem_ptr + s > pdf_mem_size) {
        a = 0.2 * pdf_mem_size;
        if (pdf_mem_ptr + s > pdf_mem_size + a) {
            pdf_mem_size = pdf_mem_ptr + s;
        } else if (pdf_mem_size < sup_pdf_mem_size - a) {
            pdf_mem_size = pdf_mem_size + a;
        } else {
            pdf_mem_size = sup_pdf_mem_size;
        }
        pdf_mem = xreallocarray(pdf_mem, integer, pdf_mem_size);
    }
    ret = pdf_mem_ptr;
    pdf_mem_ptr = pdf_mem_ptr + s;
    return ret;
}


/*
This ensures that |pdfminorversion| is set only before any bytes have
been written to the generated \.{PDF} file. Here also all variables for
\.{PDF} output are initialized, the \.{PDF} file is opened by |ensure_pdf_open|,
and the \.{PDF} header is written.
*/

void check_pdfminorversion (void) 
{
    fix_pdfoutput();
    assert(fixed_pdfoutput > 0);
    if (!fixed_pdf_minor_version_set) {
        fixed_pdf_minor_version_set = true;
        /* Initialize variables for \.{PDF} output */
        prepare_mag();
        initialize_pdf_output();
        /* Write \.{PDF} header */
        ensure_pdf_open();
        pdf_print(maketexstring("%PDF-1."));
        pdf_print_int_ln(fixed_pdf_minor_version);
        pdf_out('%');
        pdf_out('P' + 128);
        pdf_out('T' + 128);
        pdf_out('E' + 128);
        pdf_out('X' + 128);
        pdf_print_nl();

    } else {
        /* Check that variables for \.{PDF} output are unchanged */
        if (fixed_pdf_minor_version != pdf_minor_version)
            pdf_error(maketexstring("setup"),
                      maketexstring("\\pdfminorversion cannot be changed after data is written to the PDF file"));
        if (fixed_pdf_draftmode != pdf_draftmode) 
            pdf_error(maketexstring("setup"),
                      maketexstring("\\pdfdraftmode cannot be changed after data is written to the PDF file"));

    }
    if (fixed_pdf_draftmode != 0) {
        pdf_compress_level = 0; /* re-fix it, might have been changed inbetween */
        fixed_pdf_objcompresslevel = 0;
    }
}

/* Checks that we have a name for the generated PDF file and that it's open. */

void ensure_pdf_open (void) 
{
    if (output_file_name != 0)
        return;
    if (job_name == 0) 
        open_log_file();
    pack_job_name(".pdf");
    if (fixed_pdf_draftmode == 0) {
        while (!lua_b_open_out(pdf_file))
            prompt_file_name("file name for output",".pdf");
    }
    pdf_file=name_file_pointer;
    output_file_name = make_name_string();
}

/*
The PDF buffer is flushed by calling |pdf_flush|, which checks the
variable |zip_write_state| and will compress the buffer before flushing if
neccesary. We call |pdf_begin_stream| to begin a stream  and |pdf_end_stream|
to finish it. The stream contents will be compressed if compression is turn on.
*/

void pdf_flush (void) { /* flush out the |pdf_buf| */
    longinteger saved_pdf_gone;
    if (!pdf_os_mode) {
        saved_pdf_gone = pdf_gone;
        switch (zip_write_state) {
        case no_zip: 
            if (pdf_ptr > 0) {
                if (fixed_pdf_draftmode == 0) write_pdf(0, pdf_ptr - 1);
                pdf_gone = pdf_gone + pdf_ptr;
                pdf_last_byte = pdf_buf[pdf_ptr - 1];
            }
            break;
        case zip_writing:
            if (fixed_pdf_draftmode == 0) write_zip(false);
            break;
        case zip_finish: 
            if (fixed_pdf_draftmode == 0) write_zip(true);
            zip_write_state = no_zip;
            break;
        }
        pdf_ptr = 0;
        if (saved_pdf_gone > pdf_gone)
            pdf_error("file size", "File size exceeds architectural limits (pdf_gone wraps around)");
    }
}



#define is_hex_char isxdigit

/* print out a character to PDF buffer; the character will be printed in octal
 * form in the following cases: chars <= 32, backslash (92), left parenthesis
 * (40) and  right parenthesis (41) 
 */

#define pdf_print_escaped(c)                                            \
  if ((c)<=32||(c)=='\\'||(c)=='('||(c)==')'||(c)>127) {                \
    pdf_room(4);                                                        \
    pdf_quick_out('\\');                                                \
    pdf_quick_out('0' + (((c)>>6) & 0x3));                              \
    pdf_quick_out('0' + (((c)>>3) & 0x7));                              \
    pdf_quick_out('0' + ( (c)     & 0x7));                              \
  } else {                                                              \
    pdf_out((c));                                                       \
  }

void pdf_print_char(internal_font_number f, integer cc)
{
    register int c;
    pdf_mark_char(f, cc);
    if (font_encodingbytes(f) == 2) {
        register int chari;
        chari = char_index(f, cc);
        c = chari >> 8;
        pdf_print_escaped(c);
        c = chari & 0xFF;
    } else {
        if (cc > 255)
            return;
        c = cc;
    }
    pdf_print_escaped(c);
}

/* print out a string to PDF buffer */

void pdf_print(str_number s)
{
    if (s < number_chars) {
        assert(s < 256);
        pdf_out(s);
    } else {
        register pool_pointer j = str_start_macro(s);
        while (j < str_start_macro(s + 1)) {
            pdf_out(str_pool[j++]);
        }
    }
}

/* print out a integer to PDF buffer */

void pdf_print_int(longinteger n)
{
    register integer k = 0;     /*  current digit; we assume that $|n|<10^{23}$ */
    if (n < 0) {
        pdf_out('-');
        if (n < -0x7FFFFFFF) {  /* need to negate |n| more carefully */
            register longinteger m;
            k++;
            m = -1 - n;
            n = m / 10;
            m = (m % 10) + 1;
            if (m < 10) {
                dig[0] = m;
            } else {
                dig[0] = 0;
                incr(n);
            }
        } else {
            n = -n;
        }
    }
    do {
        dig[k++] = n % 10;
        n = n / 10;
    } while (n != 0);
    pdf_room(k);
    while (k-- > 0) {
        pdf_quick_out('0' + dig[k]);
    }
}


/* print $m/10^d$ as real */
void pdf_print_real(integer m, integer d)
{
    if (m < 0) {
        pdf_out('-');
        m = -m;
    };
    pdf_print_int(m / ten_pow[d]);
    m = m % ten_pow[d];
    if (m > 0) {
        pdf_out('.');
        d--;
        while (m < ten_pow[d]) {
            pdf_out('0');
            d--;
        }
        while (m % 10 == 0)
            m = m / 10;
        pdf_print_int(m);
    }
}

/* print out |s| as string in PDF output */

void pdf_print_str(str_number s)
{
    pool_pointer i, j;
    i = str_start_macro(s);
    j = str_start_macro(s + 1) - 1;
    if (i > j) {
        pdf_room(2);
        pdf_quick_out('(');
        pdf_quick_out(')');
        return;
    }
    /* the next is not really safe, the string could be "(a)xx(b)" */
    if ((str_pool[i] == '(') && (str_pool[j] == ')')) {
        pdf_print(s);
        return;
    }
    if ((str_pool[i] != '<') || (str_pool[j] != '>') || odd(str_length(s))) {
        pdf_out('(');
        pdf_print(s);
        pdf_out(')');
        return;
    }
    i++;
    j--;
    while (i < j) {
        if (!is_hex_char(str_pool[i++])) {
            pdf_out('(');
            pdf_print(s);
            pdf_out(')');
            return;
        }
    }
    pdf_print(s);               /* it was a hex string after all  */
}

