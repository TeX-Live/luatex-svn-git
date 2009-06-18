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

byte_file pdf_file;             /* the PDF output file */
real_eight_bits *pdf_buf;       /* pointer to the PDF output buffer or PDF object stream buffer */
integer pdf_buf_size = pdf_op_buf_size; /* end of PDF output buffer or PDF object stream buffer */
longinteger pdf_ptr = 0;        /* pointer to the first unused byte in the PDF buffer or object stream buffer */
real_eight_bits *pdf_op_buf;    /* the PDF output buffer */
real_eight_bits *pdf_os_buf;    /* the PDF object stream buffer */
integer pdf_os_buf_size = inf_pdf_os_buf_size;  /* current size of the PDF object stream buffer, grows dynamically */
integer *pdf_os_objnum;         /* array of object numbers within object stream */
integer *pdf_os_objoff;         /* array of object offsets within object stream */
halfword pdf_os_objidx;         /* pointer into |pdf_os_objnum| and |pdf_os_objoff| */
integer pdf_os_cntr = 0;        /* counter for object stream objects */
integer pdf_op_ptr = 0;         /* store for PDF buffer |pdf_ptr| while inside object streams */
integer pdf_os_ptr = 0;         /* store for object stream |pdf_ptr| while outside object streams */
boolean pdf_os_mode = false;    /* true if producing object stream */
boolean pdf_os_enable;          /* true if object streams are globally enabled */
integer pdf_os_cur_objnum = 0;  /* number of current object stream object */
longinteger pdf_gone = 0;       /* number of bytes that were flushed to output */
longinteger pdf_save_offset;    /* to save |pdf_offset| */
integer zip_write_state = no_zip;       /* which state of compression we are in */
integer fixed_pdf_minor_version;        /* fixed minor part of the PDF version */
boolean fixed_pdf_minor_version_set = false;    /* flag if the PDF version has been set */
integer fixed_pdf_objcompresslevel;     /* fixed level for activating PDF object streams */
integer fixed_pdfoutput;        /* fixed output format */
boolean fixed_pdfoutput_set = false;    /* |fixed_pdfoutput| has been set? */
integer fixed_gamma;
integer fixed_image_gamma;
boolean fixed_image_hicolor;
integer fixed_image_apply_gamma;
integer fixed_pdf_draftmode;    /* fixed \\pdfdraftmode */
integer pdf_page_group_val = -1;
integer epochseconds;
integer microseconds;
integer page_divert_val = 0;

void initialize_pdfgen(void)
{
    pdf_buf = pdf_op_buf;
}

void initialize_pdf_output(void)
{
    if ((pdf_minor_version < 0) || (pdf_minor_version > 9)) {
        char *hlp[] = { "The pdfminorversion must be between 0 and 9.",
            "I changed this to 4.", NULL
        };
        char msg[256];
        snprintf(msg, 255, "LuaTeX error (illegal pdfminorversion %d)",
                 (int) pdf_minor_version);
        tex_error(msg, hlp);
        pdf_minor_version = 4;
    }
    fixed_pdf_minor_version = pdf_minor_version;
    fixed_decimal_digits = fix_int(pdf_decimal_digits, 0, 4);
    fixed_gamma = fix_int(pdf_gamma, 0, 1000000);
    fixed_image_gamma = fix_int(pdf_image_gamma, 0, 1000000);
    fixed_image_hicolor = fix_int(pdf_image_hicolor, 0, 1);
    fixed_image_apply_gamma = fix_int(pdf_image_apply_gamma, 0, 1);
    fixed_pdf_objcompresslevel = fix_int(pdf_objcompresslevel, 0, 3);
    fixed_pdf_draftmode = fix_int(pdf_draftmode, 0, 1);
    fixed_inclusion_copy_font = fix_int(pdf_inclusion_copy_font, 0, 1);
    fixed_replace_font = fix_int(pdf_replace_font, 0, 1);
    fixed_pk_resolution = fix_int(pdf_pk_resolution, 72, 8000);
    if ((fixed_pdf_minor_version >= 5) && (fixed_pdf_objcompresslevel > 0)) {
        pdf_os_enable = true;
    } else {
        if (fixed_pdf_objcompresslevel > 0) {
            pdf_warning(maketexstring("Object streams"),
                        maketexstring
                        ("\\pdfobjcompresslevel > 0 requires \\pdfminorversion > 4. Object streams disabled now."),
                        true, true);
            fixed_pdf_objcompresslevel = 0;
        }
        pdf_os_enable = false;
    }
    if (pdf_pk_resolution == 0) /* if not set from format file or by user */
        pdf_pk_resolution = pk_dpi;     /* take it from \.{texmf.cnf} */
    pk_scale_factor =
        divide_scaled(72, fixed_pk_resolution, 5 + fixed_decimal_digits);
    if (!callback_defined(read_pk_file_callback)) {
        if (pdf_pk_mode != null) {
            kpseinitprog("PDFTEX", fixed_pk_resolution,
                         makecstring(tokens_to_string(pdf_pk_mode)), nil);
            flush_string();
        } else {
            kpseinitprog("PDFTEX", fixed_pk_resolution, nil, nil);
        }
        if (!kpsevarvalue("MKTEXPK"))
            kpsesetprogramenabled(kpsepkformat, 1, kpsesrccmdline);
    }
    set_job_id(int_par(param_year_code),
               int_par(param_month_code),
               int_par(param_day_code), int_par(param_time_code));

    if ((pdf_unique_resname > 0) && (pdf_resname_prefix == 0))
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

void check_pdfminorversion(void)
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
        pdf_printf("%%PDF-1.%d\n", (int) fixed_pdf_minor_version);
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
                      maketexstring
                      ("\\pdfminorversion cannot be changed after data is written to the PDF file"));
        if (fixed_pdf_draftmode != pdf_draftmode)
            pdf_error(maketexstring("setup"),
                      maketexstring
                      ("\\pdfdraftmode cannot be changed after data is written to the PDF file"));

    }
    if (fixed_pdf_draftmode != 0) {
        pdf_compress_level = 0; /* re-fix it, might have been changed inbetween */
        fixed_pdf_objcompresslevel = 0;
    }
}

/* Checks that we have a name for the generated PDF file and that it's open. */

void ensure_pdf_open(void)
{
    if (output_file_name != 0)
        return;
    if (job_name == 0)
        open_log_file();
    pack_job_name(".pdf");
    if (fixed_pdf_draftmode == 0) {
        while (!lua_b_open_out(pdf_file))
            prompt_file_name("file name for output", ".pdf");
    }
    pdf_file = name_file_pointer;
    output_file_name = make_name_string();
}

/*
The PDF buffer is flushed by calling |pdf_flush|, which checks the
variable |zip_write_state| and will compress the buffer before flushing if
neccesary. We call |pdf_begin_stream| to begin a stream  and |pdf_end_stream|
to finish it. The stream contents will be compressed if compression is turn on.
*/

void pdf_flush(void)
{                               /* flush out the |pdf_buf| */
    longinteger saved_pdf_gone;
    if (!pdf_os_mode) {
        saved_pdf_gone = pdf_gone;
        switch (zip_write_state) {
        case no_zip:
            if (pdf_ptr > 0) {
                if (fixed_pdf_draftmode == 0)
                    write_pdf(0, pdf_ptr - 1);
                pdf_gone = pdf_gone + pdf_ptr;
                pdf_last_byte = pdf_buf[pdf_ptr - 1];
            }
            break;
        case zip_writing:
            if (fixed_pdf_draftmode == 0)
                write_zip(false);
            break;
        case zip_finish:
            if (fixed_pdf_draftmode == 0)
                write_zip(true);
            zip_write_state = no_zip;
            break;
        }
        pdf_ptr = 0;
        if (saved_pdf_gone > pdf_gone)
            pdf_error(maketexstring("file size"),
                      maketexstring
                      ("File size exceeds architectural limits (pdf_gone wraps around)"));
    }
}

/* low-level buffer checkers */

/* check that |s| bytes more fit into |pdf_os_buf|; increase it if required */
void pdf_os_get_os_buf(integer s)
{
    integer a;
    if (s > sup_pdf_os_buf_size - pdf_ptr)
        overflow(maketexstring("PDF object stream buffer"), pdf_os_buf_size);
    if (pdf_ptr + s > pdf_os_buf_size) {
        a = 0.2 * pdf_os_buf_size;
        if (pdf_ptr + s > pdf_os_buf_size + a)
            pdf_os_buf_size = pdf_ptr + s;
        else if (pdf_os_buf_size < sup_pdf_os_buf_size - a)
            pdf_os_buf_size = pdf_os_buf_size + a;
        else
            pdf_os_buf_size = sup_pdf_os_buf_size;
        pdf_os_buf =
            xreallocarray(pdf_os_buf, real_eight_bits, pdf_os_buf_size);
        pdf_buf = pdf_os_buf;
        pdf_buf_size = pdf_os_buf_size;
    }
}

/* make sure that there are at least |n| bytes free in PDF buffer */
void pdf_room(integer n)
{
    if (pdf_os_mode && (n + pdf_ptr > pdf_buf_size))
        pdf_os_get_os_buf(n);
    else if ((!pdf_os_mode) && (n > pdf_buf_size))
        overflow(maketexstring("PDF output buffer"), pdf_op_buf_size);
    else if ((!pdf_os_mode) && (n + pdf_ptr > pdf_buf_size))
        pdf_flush();
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

void pdf_puts(const char *s)
{
    pdf_room(strlen(s) + 1);
    while (*s)
        pdf_buf[pdf_ptr++] = *s++;
}

static char pdf_printf_buf[PRINTF_BUF_SIZE];

__attribute__ ((format(printf, 1, 2)))
void pdf_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(pdf_printf_buf, PRINTF_BUF_SIZE, fmt, args);
    pdf_puts(pdf_printf_buf);
    va_end(args);
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


/* begin a stream */
void pdf_begin_stream(void)
{
    pdf_printf("/Length           \n");
    pdf_seek_write_length = true;       /* fill in length at |pdf_end_stream| call */
    pdf_stream_length_offset = pdf_offset - 11;
    pdf_stream_length = 0;
    pdf_last_byte = 0;
    if (pdf_compress_level > 0) {
        pdf_printf("/Filter /FlateDecode\n");
        pdf_printf(">>\n");
        pdf_printf("stream\n");
        pdf_flush();
        zip_write_state = zip_writing;
    } else {
        pdf_printf(">>\n");
        pdf_printf("stream\n");
        pdf_save_offset = pdf_offset;
    }
}

/* end a stream */
void pdf_end_stream(void)
{
    if (zip_write_state == zip_writing)
        zip_write_state = zip_finish;
    else
        pdf_stream_length = pdf_offset - pdf_save_offset;
    pdf_flush();
    if (pdf_seek_write_length)
        write_stream_length(pdf_stream_length, pdf_stream_length_offset);
    pdf_seek_write_length = false;
    if (pdf_last_byte != pdf_new_line_char)
        pdf_out(pdf_new_line_char);
    pdf_printf("endstream\n");
    pdf_end_obj();
}

void pdf_remove_last_space(void)
{
    if ((pdf_ptr > 0) && (pdf_buf[pdf_ptr - 1] == ' '))
        decr(pdf_ptr);
}


/*
To print |scaled| value to PDF output we need some subroutines to ensure
accurary.
*/

#define max_integer 0x7FFFFFFF  /* $2^{31}-1$ */

/* scaled value corresponds to 100in, exact, 473628672 */
scaled one_hundred_inch = 7227 * 65536;

/* scaled value corresponds to 1in (rounded to 4736287) */
scaled one_inch = (7227 * 65536 + 50) / 100;

    /* scaled value corresponds to 1truein (rounded!) */
scaled one_true_inch = (7227 * 65536 + 50) / 100;

/* scaled value corresponds to 100bp */
scaled one_hundred_bp = (7227 * 65536) / 72;

/* scaled value corresponds to 1bp (rounded to 65782) */
scaled one_bp = ((7227 * 65536) / 72 + 50) / 100;

/* $10^0..10^9$ */
integer ten_pow[10] =
    { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000,
    1000000000
};

/*
The function |divide_scaled| divides |s| by |m| using |dd| decimal
digits of precision. It is defined in C because it is a good candidate
for optimizations that are not possible in pascal.
*/

scaled round_xn_over_d(scaled x, integer n, integer d)
{
    boolean positive;           /* was |x>=0|? */
    nonnegative_integer t, u, v;        /* intermediate quantities */
    if (x >= 0) {
        positive = true;
    } else {
        x = -(x);
        positive = false;
    }
    t = (x % 0100000) * n;
    u = (x / 0100000) * n + (t / 0100000);
    v = (u % d) * 0100000 + (t % 0100000);
    if (u / d >= 0100000)
        arith_error = true;
    else
        u = 0100000 * (u / d) + (v / d);
    v = v % d;
    if (2 * v >= d)
        u++;
    if (positive)
        return u;
    else
        return (-u);
}

void pdf_print_bp(scaled s)
{                               /* print scaled as |bp| */
    pdf_print_real(divide_scaled(s, one_hundred_bp, fixed_decimal_digits + 2),
                   fixed_decimal_digits);
}

void pdf_print_mag_bp(scaled s)
{                               /* take |mag| into account */
    prepare_mag();
    if (int_par(param_mag_code) != 1000)
        s = round_xn_over_d(s, int_par(param_mag_code), 1000);
    pdf_print_bp(s);
}

integer fixed_pk_resolution;
integer fixed_decimal_digits;
integer fixed_gen_tounicode;
integer fixed_inclusion_copy_font;
integer fixed_replace_font;
integer pk_scale_factor;
integer pdf_output_option;
integer pdf_output_value;
integer pdf_draftmode_option;
integer pdf_draftmode_value;

/* mark |f| as a used font; set |font_used(f)|, |pdf_font_size(f)| and |pdf_font_num(f)| */
void pdf_use_font(internal_font_number f, integer fontnum)
{
    set_pdf_font_size(f, font_size(f));
    set_font_used(f, true);
    assert((fontnum > 0) || ((fontnum < 0) && (pdf_font_num(-fontnum) > 0)));
    set_pdf_font_num(f, fontnum);
    if (pdf_move_chars > 0) {
        pdf_warning(0, maketexstring("Primitive \\pdfmovechars is obsolete."),
                    true, true);
        pdf_move_chars = 0;     /* warn only once */
    }
}

/*
To set PDF font we need to find out fonts with the same name, because \TeX\
can load the same font several times for various sizes. For such fonts we
define only one font resource. The array |pdf_font_num| holds the object
number of font resource. A negative value of an entry of |pdf_font_num|
indicates that the corresponding font shares the font resource with the font
*/

/* create a font object */
void pdf_init_font(internal_font_number f)
{
    internal_font_number k, b;
    integer i;
    assert(!font_used(f));

    /* if |f| is auto expanded then ensure the base font is initialized */
    if (pdf_font_auto_expand(f) && (pdf_font_blink(f) != null_font)) {
        b = pdf_font_blink(f);
        /* TODO: reinstate this check. disabled because wide fonts font have fmentries */
        if (false && (!hasfmentry(b)))
            pdf_error(maketexstring("font expansion"),
                      maketexstring
                      ("auto expansion is only possible with scalable fonts"));
        if (!font_used(b))
            pdf_init_font(b);
        set_font_map(f, font_map(b));
    }
    /* check whether |f| can share the font object with some |k|: we have 2 cases
       here: 1) |f| and |k| have the same tfm name (so they have been loaded at
       different sizes, eg 'cmr10' and 'cmr10 at 11pt'); 2) |f| has been auto
       expanded from |k|
     */
    if (hasfmentry(f) || true) {
        i = head_tab[obj_type_font];
        while (i != 0) {
            k = obj_info(i);
            if (font_shareable(f, k)) {
                assert(pdf_font_num(k) != 0);
                if (pdf_font_num(k) < 0)
                    pdf_use_font(f, pdf_font_num(k));
                else
                    pdf_use_font(f, -k);
                return;
            }
            i = obj_link(i);
        }
    }
    /* create a new font object for |f| */
    pdf_create_obj(obj_type_font, f);
    pdf_use_font(f, obj_ptr);
}


/* set the actual font on PDF page */
internal_font_number pdf_set_font(internal_font_number f)
{
    pointer p;
    internal_font_number k;
    if (!font_used(f))
        pdf_init_font(f);
    set_ff(f);                  /* set |ff| to the tfm number of the font sharing the font object
                                   with |f|; |ff| is either |f| or some font with the same tfm name
                                   at different size and/or expansion */
    k = ff;
    p = pdf_font_list;
    while (p != null) {
        set_ff(fixmem[p].hhlh); /* info(p) */
        if (ff == k)
            goto FOUND;
        p = fixmem[p].hhrh;     /* link(p) */
    }
    pdf_append_list(f, pdf_font_list);  /* |f| not found in |pdf_font_list|, append it now */
  FOUND:
    return k;
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

void pdf_literal(str_number s, integer literal_mode, boolean warn)
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
        pdf_goto_pagemode();
        synch_p_with_c(cur);
        pdf_set_pos(pos.h, pos.v);
        break;
    case direct_page:
        pdf_goto_pagemode();
        break;
    case direct_always:
        pdf_end_string_nl();
        break;
    default:
        tconfusion("literal1");
        break;
    }
    if (s > STRING_OFFSET) {
        while (j < str_start_macro(s + 1))
            pdf_out(str_pool[j++]);
    } else {
        pdf_out(s);
    }
    pdf_print_nl();
}
