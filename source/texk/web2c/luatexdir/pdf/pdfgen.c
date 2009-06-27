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
#include "md5.h"

#include "luatex-api.h"         /* for tokenlist_to_cstring */

static const char __svn_version[] =
    "$Id$"
    "$URL$";

#define is_hex_char isxdigit

#define check_nprintf(size_get, size_want) \
    if ((unsigned)(size_get) >= (unsigned)(size_want)) \
        pdftex_fail ("snprintf failed: file %s, line %d", __FILE__, __LINE__);

PDF static_pdf = NULL;

static char *jobname_cstr = NULL;

integer fixed_pdfoutput;        /* fixed output format */
boolean fixed_pdfoutput_set = false;    /* |fixed_pdfoutput| has been set? */

/* commandline interface */
integer pdf_output_option;
integer pdf_output_value;
integer pdf_draftmode_option;
integer pdf_draftmode_value;


PDF initialize_pdf(void)
{
    PDF pdf;
    pdf = xmalloc(sizeof(pdf_output_file));
    memset(pdf, 0, sizeof(pdf_output_file));

    pdf->os_obj = xmalloc(pdf_os_max_objs * sizeof(os_obj_data));
    pdf->os_buf_size = inf_pdf_os_buf_size;
    pdf->os_buf = xmalloc(pdf->os_buf_size * sizeof(unsigned char));
    pdf->op_buf_size = inf_pdf_op_buf_size;
    pdf->op_buf = xmalloc(pdf->op_buf_size * sizeof(unsigned char));

    pdf->buf_size = pdf->op_buf_size;
    pdf->buf = pdf->op_buf;

    /* Sometimes it is neccesary to allocate memory for PDF output that cannot
       be deallocated then, so we use |mem| for this purpose. */
    pdf->mem_size = inf_pdf_mem_size;   /* allocated size of |mem| array */
    pdf->mem = xmalloc(pdf->mem_size * sizeof(int));
    pdf->mem_ptr = 1;           /* the first word is not used so we can use zero as a value for testing
                                   whether a pointer to |mem| is valid  */
    pdf->pstruct = NULL;

    return pdf;
}

void initialize_pdfgen(void)
{
    static_pdf = initialize_pdf();
}

void initialize_pdf_output(PDF pdf)
{
    if ((pdf_minor_version < 0) || (pdf_minor_version > 9)) {
        char *hlp[] = { "The pdfminorversion must be between 0 and 9.",
            "I changed this to 4.", NULL
        };
        char msg[256];
        (void) snprintf(msg, 255, "LuaTeX error (illegal pdfminorversion %d)",
                        (int) pdf_minor_version);
        tex_error(msg, hlp);
        pdf_minor_version = 4;
    }
    pdf->minor_version = pdf_minor_version;
    pdf->decimal_digits = fix_int(pdf_decimal_digits, 0, 4);
    pdf->gamma = fix_int(pdf_gamma, 0, 1000000);
    pdf->image_gamma = fix_int(pdf_image_gamma, 0, 1000000);
    pdf->image_hicolor = fix_int(pdf_image_hicolor, 0, 1);
    pdf->image_apply_gamma = fix_int(pdf_image_apply_gamma, 0, 1);
    pdf->objcompresslevel = fix_int(pdf_objcompresslevel, 0, 3);
    pdf->draftmode = fix_int(pdf_draftmode, 0, 1);
    pdf->inclusion_copy_font = fix_int(pdf_inclusion_copy_font, 0, 1);
    pdf->replace_font = fix_int(pdf_replace_font, 0, 1);
    pdf->pk_resolution = fix_int(pdf_pk_resolution, 72, 8000);
    if ((pdf->minor_version >= 5) && (pdf->objcompresslevel > 0)) {
        pdf->os_enable = true;
    } else {
        if (pdf->objcompresslevel > 0) {
            pdf_warning(maketexstring("Object streams"),
                        maketexstring
                        ("\\pdfobjcompresslevel > 0 requires \\pdfminorversion > 4. Object streams disabled now."),
                        true, true);
            pdf->objcompresslevel = 0;
        }
        pdf->os_enable = false;
    }
    if (pdf->pk_resolution == 0)        /* if not set from format file or by user */
        pdf->pk_resolution = pk_dpi;    /* take it from \.{texmf.cnf} */
    pdf->pk_scale_factor =
        divide_scaled(72, pdf->pk_resolution, 5 + pdf->decimal_digits);
    if (!callback_defined(read_pk_file_callback)) {
        if (pdf_pk_mode != null) {
            char *s = tokenlist_to_cstring(pdf_pk_mode, true, NULL);
            kpseinitprog("PDFTEX", pdf->pk_resolution, s, nil);
            xfree(s);
        } else {
            kpseinitprog("PDFTEX", pdf->pk_resolution, nil, nil);
        }
        if (!kpsevarvalue("MKTEXPK"))
            kpsesetprogramenabled(kpsepkformat, 1, kpsesrccmdline);
    }
    set_job_id(int_par(param_year_code),
               int_par(param_month_code),
               int_par(param_day_code), int_par(param_time_code));

    if ((pdf_unique_resname > 0) && (pdf->resname_prefix == NULL))
        pdf->resname_prefix = get_resname_prefix();
    pdf_page_init(pdf);
}

/*
  We use |pdf_get_mem| to allocate memory in |mem|
*/

integer pdf_get_mem(PDF pdf, integer s)
{                               /* allocate |s| words in |mem| */
    integer a;
    integer ret;
    if (s > sup_pdf_mem_size - pdf->mem_ptr)
        overflow(maketexstring("PDF memory size (pdf_mem_size)"),
                 pdf->mem_size);
    if (pdf->mem_ptr + s > pdf->mem_size) {
        a = 0.2 * pdf->mem_size;
        if (pdf->mem_ptr + s > pdf->mem_size + a) {
            pdf->mem_size = pdf->mem_ptr + s;
        } else if (pdf->mem_size < sup_pdf_mem_size - a) {
            pdf->mem_size = pdf->mem_size + a;
        } else {
            pdf->mem_size = sup_pdf_mem_size;
        }
        pdf->mem = xreallocarray(pdf->mem, int, pdf->mem_size);
    }
    ret = pdf->mem_ptr;
    pdf->mem_ptr = pdf->mem_ptr + s;
    return ret;
}


/*
This ensures that |pdfminorversion| is set only before any bytes have
been written to the generated \.{PDF} file. Here also all variables for
\.{PDF} output are initialized, the \.{PDF} file is opened by |ensure_pdf_open|,
and the \.{PDF} header is written.
*/

void do_check_pdfminorversion(PDF pdf)
{
    fix_pdfoutput();
    assert(fixed_pdfoutput > 0);
    if (!pdf->minor_version_set) {
        pdf->minor_version_set = true;
        /* Initialize variables for \.{PDF} output */
        prepare_mag();
        initialize_pdf_output(pdf);
        /* Write \.{PDF} header */
        ensure_pdf_open(pdf);
        pdf_printf(pdf, "%%PDF-1.%d\n", pdf->minor_version);
        pdf_out(pdf, '%');
        pdf_out(pdf, 'P' + 128);
        pdf_out(pdf, 'T' + 128);
        pdf_out(pdf, 'E' + 128);
        pdf_out(pdf, 'X' + 128);
        pdf_print_nl(pdf);

    } else {
        /* Check that variables for \.{PDF} output are unchanged */
        if (pdf->minor_version != pdf_minor_version)
            pdf_error(maketexstring("setup"),
                      maketexstring
                      ("\\pdfminorversion cannot be changed after data is written to the PDF file"));
        if (pdf->draftmode != pdf_draftmode)
            pdf_error(maketexstring("setup"),
                      maketexstring
                      ("\\pdfdraftmode cannot be changed after data is written to the PDF file"));

    }
    if (pdf->draftmode != 0) {
        pdf_compress_level = 0; /* re-fix it, might have been changed inbetween */
        pdf->objcompresslevel = 0;
    }
}

/* Checks that we have a name for the generated PDF file and that it's open. */

void ensure_pdf_open(PDF pdf)
{
    if (output_file_name != 0)
        return;
    if (job_name == 0)
        open_log_file();
    pack_job_name(".pdf");
    if (pdf->draftmode == 0) {
        while (!lua_b_open_out(pdf->file))
            prompt_file_name("file name for output", ".pdf");
    }
    pdf->file = name_file_pointer;      /* hm ? */
    output_file_name = make_name_string();
}

/*
The PDF buffer is flushed by calling |pdf_flush|, which checks the
variable |zip_write_state| and will compress the buffer before flushing if
neccesary. We call |pdf_begin_stream| to begin a stream  and |pdf_end_stream|
to finish it. The stream contents will be compressed if compression is turn on.
*/

/* writepdf() always writes by fwrite() */

static void write_pdf(PDF pdf, integer a, int b)
{
    (void) fwrite((char *) &pdf->buf[a], sizeof(pdf->buf[a]),
                  (int) ((b) - (a) + 1), pdf->file);
}


void pdf_flush(PDF pdf)
{                               /* flush out the |pdf_buf| */
    off_t saved_pdf_gone;
    if (!pdf->os_mode) {
        saved_pdf_gone = pdf->gone;
        switch (pdf->zip_write_state) {
        case no_zip:
            if (pdf->ptr > 0) {
                if (pdf->draftmode == 0)
                    write_pdf(pdf, 0, pdf->ptr - 1);
                pdf->gone += pdf->ptr;
                pdf_last_byte = pdf->buf[pdf->ptr - 1];
            }
            break;
        case zip_writing:
            if (pdf->draftmode == 0)
                write_zip(pdf, false);
            break;
        case zip_finish:
            if (pdf->draftmode == 0)
                write_zip(pdf, true);
            pdf->zip_write_state = no_zip;
            break;
        }
        pdf->ptr = 0;
        if (saved_pdf_gone > pdf->gone)
            pdf_error(maketexstring("file size"),
                      maketexstring
                      ("File size exceeds architectural limits (pdf_gone wraps around)"));
    }
}


/* switch between PDF stream and object stream mode */
void pdf_os_switch(PDF pdf, boolean pdf_os)
{
    if (pdf_os && pdf->os_enable) {
        if (!pdf->os_mode) {    /* back up PDF stream variables */
            pdf->op_ptr = pdf->ptr;
            pdf->ptr = pdf->os_ptr;
            pdf->buf = pdf->os_buf;
            pdf->buf_size = pdf->os_buf_size;
            pdf->os_mode = true;        /* switch to object stream */
        }
    } else {
        if (pdf->os_mode) {     /* back up object stream variables */
            pdf->os_ptr = pdf->ptr;
            pdf->ptr = pdf->op_ptr;
            pdf->buf = pdf->op_buf;
            pdf->buf_size = pdf->op_buf_size;
            pdf->os_mode = false;       /* switch to PDF stream */
        }
    }
}

/* create new \.{/ObjStm} object if required, and set up cross reference info */
void pdf_os_prepare_obj(PDF pdf, integer i, integer pdf_os_level)
{
    pdf_os_switch(pdf, ((pdf_os_level > 0)
                        && (pdf->objcompresslevel >= pdf_os_level)));
    if (pdf->os_mode) {
        if (pdf->os_cur_objnum == 0) {
            pdf->os_cur_objnum = pdf_new_objnum();
            decr(obj_ptr);      /* object stream is not accessible to user */
            incr(pdf->os_cntr); /* only for statistics */
            pdf->os_idx = 0;
            pdf->ptr = 0;       /* start fresh object stream */
        } else {
            incr(pdf->os_idx);
        }
        obj_os_idx(i) = pdf->os_idx;
        obj_offset(i) = pdf->os_cur_objnum;
        pdf->os_obj[pdf->os_idx].num = i;
        pdf->os_obj[pdf->os_idx].off = pdf->ptr;
    } else {
        obj_offset(i) = pdf_offset(pdf);
        obj_os_idx(i) = -1;     /* mark it as not included in object stream */
    }
}

/* low-level buffer checkers */

/* check that |s| bytes more fit into |pdf_os_buf|; increase it if required */
static void pdf_os_get_os_buf(PDF pdf, integer s)
{
    integer a;
    if (s > sup_pdf_os_buf_size - pdf->ptr)
        overflow(maketexstring("PDF object stream buffer"), pdf->os_buf_size);
    if (pdf->ptr + s > pdf->os_buf_size) {
        a = 0.2 * pdf->os_buf_size;
        if (pdf->ptr + s > pdf->os_buf_size + a)
            pdf->os_buf_size = pdf->ptr + s;
        else if (pdf->os_buf_size < sup_pdf_os_buf_size - a)
            pdf->os_buf_size = pdf->os_buf_size + a;
        else
            pdf->os_buf_size = sup_pdf_os_buf_size;
        pdf->os_buf =
            xreallocarray(pdf->os_buf, unsigned char, pdf->os_buf_size);
        pdf->buf = pdf->os_buf;
        pdf->buf_size = pdf->os_buf_size;
    }
}

/* make sure that there are at least |n| bytes free in PDF buffer */
void pdf_room(PDF pdf, integer n)
{
    if (pdf->os_mode && (n + pdf->ptr > pdf->buf_size))
        pdf_os_get_os_buf(pdf, n);
    else if ((!pdf->os_mode) && (n > pdf->buf_size))
        overflow(maketexstring("PDF output buffer"), pdf->op_buf_size);
    else if ((!pdf->os_mode) && (n + pdf->ptr > pdf->buf_size))
        pdf_flush(pdf);
}


/* print out a character to PDF buffer; the character will be printed in octal
 * form in the following cases: chars <= 32, backslash (92), left parenthesis
 * (40) and  right parenthesis (41) 
 */

#define pdf_print_escaped(c)                                            \
  if ((c)<=32||(c)=='\\'||(c)=='('||(c)==')'||(c)>127) {                \
    pdf_room(pdf,4);                                                    \
    pdf_quick_out(pdf,'\\');                                                \
    pdf_quick_out(pdf,'0' + (((c)>>6) & 0x3));                              \
    pdf_quick_out(pdf,'0' + (((c)>>3) & 0x7));                              \
    pdf_quick_out(pdf,'0' + ( (c)     & 0x7));                              \
  } else {                                                              \
    pdf_out(pdf,(c));                                                       \
  }

void pdf_print_char(PDF pdf, internal_font_number f, integer cc)
{
    pdf_mark_char(f, cc);
    if (font_encodingbytes(f) == 2) {
        char hex[5];
        snprintf(hex, 5, "%04X", char_index(f, cc));
        pdf_room(pdf, 4);
        pdf_quick_out(pdf, hex[0]);
        pdf_quick_out(pdf, hex[1]);
        pdf_quick_out(pdf, hex[2]);
        pdf_quick_out(pdf, hex[3]);
    } else {
        if (cc > 255)
            return;
        pdf_print_escaped(cc);
    }
}

void pdf_puts(PDF pdf, const char *s)
{
    pdf_room(pdf, strlen(s));
    while (*s != '\0')
        pdf_quick_out(pdf, *s++);
}

static char pdf_printf_buf[PRINTF_BUF_SIZE];

__attribute__ ((format(printf, 2, 3)))
void pdf_printf(PDF pdf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(pdf_printf_buf, PRINTF_BUF_SIZE, fmt, args);
    pdf_puts(pdf, pdf_printf_buf);
    va_end(args);
}


/* print out a string to PDF buffer */

void pdf_print(PDF pdf, str_number s)
{
    if (s < number_chars) {
        assert(s < 256);
        pdf_out(pdf, s);
    } else {
        register pool_pointer j = str_start_macro(s);
        while (j < str_start_macro(s + 1)) {
            pdf_out(pdf, str_pool[j++]);
        }
    }
}

/* print out a integer to PDF buffer */

void pdf_print_int(PDF pdf, longinteger n)
{
    register integer k = 0;     /*  current digit; we assume that $|n|<10^{23}$ */
    if (n < 0) {
        pdf_out(pdf, '-');
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
    pdf_room(pdf, k);
    while (k-- > 0) {
        pdf_quick_out(pdf, '0' + dig[k]);
    }
}


/* print $m/10^d$ as real */
void pdf_print_real(PDF pdf, integer m, integer d)
{
    if (m < 0) {
        pdf_out(pdf, '-');
        m = -m;
    };
    pdf_print_int(pdf, m / ten_pow[d]);
    m = m % ten_pow[d];
    if (m > 0) {
        pdf_out(pdf, '.');
        d--;
        while (m < ten_pow[d]) {
            pdf_out(pdf, '0');
            d--;
        }
        while (m % 10 == 0)
            m = m / 10;
        pdf_print_int(pdf, m);
    }
}

/* print out |s| as string in PDF output */

void pdf_print_str(PDF pdf, char *s)
{
    char *orig = s;
    int l = strlen(s) - 1;      /* last string index */
    if (l < 0) {
        pdf_printf(pdf, "()");
        return;
    }
    /* the next is not really safe, the string could be "(a)xx(b)" */
    if ((s[0] == '(') && (s[l] == ')')) {
        pdf_printf(pdf, "%s", s);
        return;
    }
    if ((s[0] != '<') || (s[l] != '>') || odd((l + 1))) {
        pdf_printf(pdf, "(%s)", s);
        return;
    }
    s++;
    while (is_hex_char(*s))
        s++;
    if (s != orig + l) {
        pdf_printf(pdf, "(%s)", orig);
        return;
    }
    pdf_printf(pdf, "%s", orig);        /* it was a hex string after all  */
}


/* begin a stream */
void pdf_begin_stream(PDF pdf)
{
    assert(pdf->os_mode == false);
    pdf_printf(pdf, "/Length           \n");
    pdf_seek_write_length = true;       /* fill in length at |pdf_end_stream| call */
    pdf_stream_length_offset = pdf_offset(pdf) - 11;
    pdf_stream_length = 0;
    pdf_last_byte = 0;
    if (pdf_compress_level > 0) {
        pdf_printf(pdf, "/Filter /FlateDecode\n");
        pdf_printf(pdf, ">>\n");
        pdf_printf(pdf, "stream\n");
        pdf_flush(pdf);
        pdf->zip_write_state = zip_writing;
    } else {
        pdf_printf(pdf, ">>\n");
        pdf_printf(pdf, "stream\n");
        pdf_save_offset(pdf);
    }
}

/* end a stream */
void pdf_end_stream(PDF pdf)
{
    if (pdf->zip_write_state == zip_writing)
        pdf->zip_write_state = zip_finish;
    else
        pdf_stream_length = pdf_offset(pdf) - pdf_saved_offset(pdf);
    pdf_flush(pdf);
    if (pdf_seek_write_length)
        write_stream_length(pdf, pdf_stream_length, pdf_stream_length_offset);
    pdf_seek_write_length = false;
    if (pdf_last_byte != pdf_new_line_char)
        pdf_out(pdf, pdf_new_line_char);
    pdf_printf(pdf, "endstream\n");
    pdf_end_obj(pdf);
}

void pdf_remove_last_space(PDF pdf)
{
    if ((pdf->ptr > 0) && (pdf->buf[pdf->ptr - 1] == ' '))
        decr(pdf->ptr);
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

#define lround(a) (long) floor((a) + 0.5)

void pdf_print_bp(PDF pdf, scaled s)
{                               /* print scaled as |bp| */
    pdffloat a;
    pdfstructure *p = pdf->pstruct;
    assert(p != NULL);
    a.m = lround(s * p->k1);
    a.e = pdf->decimal_digits;
    print_pdffloat(pdf, &a);
}

void pdf_print_mag_bp(PDF pdf, scaled s)
{                               /* take |mag| into account */
    pdffloat a;
    pdfstructure *p = pdf->pstruct;
    prepare_mag();
    if (int_par(param_mag_code) != 1000)
        a.m = lround(s * (long) int_par(param_mag_code) / 1000.0 * p->k1);
    else
        a.m = lround(s * p->k1);
    a.e = pdf->decimal_digits;
    print_pdffloat(pdf, &a);
}

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

/* Subroutines to print out various PDF objects */

/* print out an integer with fixed width; used for outputting cross-reference table */
void pdf_print_fw_int(PDF pdf, longinteger n, integer w)
{
    integer k;                  /* $0\le k\le23$ */
    k = 0;
    do {
        dig[k] = n % 10;
        n = n / 10;
        k++;
    } while (k != w);
    pdf_room(pdf, k);
    while (k-- > 0)
        pdf_quick_out(pdf, '0' + dig[k]);
}

/* print out an integer as a number of bytes; used for outputting \.{/XRef} cross-reference stream */
void pdf_out_bytes(PDF pdf, longinteger n, integer w)
{
    integer k;
    integer bytes[8];           /* digits in a number being output */
    k = 0;
    do {
        bytes[k] = n % 256;
        n = n / 256;
        k++;
    } while (k != w);
    pdf_room(pdf, k);
    while (k-- > 0)
        pdf_quick_out(pdf, bytes[k]);
}

/* print out an entry in dictionary with integer value to PDF buffer */

void pdf_int_entry(PDF pdf, char *s, integer v)
{
    pdf_printf(pdf, "/%s ", s);
    pdf_print_int(pdf, v);
}

void pdf_int_entry_ln(PDF pdf, char *s, integer v)
{
    pdf_int_entry(pdf, s, v);
    pdf_print_nl(pdf);
}

/* print out an indirect entry in dictionary */
void pdf_indirect(PDF pdf, char *s, integer o)
{
    pdf_printf(pdf, "/%s %d 0 R", s, (int) o);
}

void pdf_indirect_ln(PDF pdf, char *s, integer o)
{
    pdf_indirect(pdf, s, o);
    pdf_print_nl(pdf);
}

/* print out |s| as string in PDF output */

void pdf_print_str_ln(PDF pdf, char *s)
{
    pdf_print_str(pdf, s);
    pdf_print_nl(pdf);
}

/* print out an entry in dictionary with string value to PDF buffer */

void pdf_str_entry(PDF pdf, char *s, char *v)
{
    if (v == 0)
        return;
    pdf_printf(pdf, "/%s ", s);
    pdf_print_str(pdf, v);
}

void pdf_str_entry_ln(PDF pdf, char *s, char *v)
{
    if (v == 0)
        return;
    pdf_str_entry(pdf, s, v);
    pdf_print_nl(pdf);
}

void pdf_print_toks(PDF pdf, halfword p)
{
    int len = 0;
    char *s = tokenlist_to_cstring(p, true, &len);
    if (len > 0)
        pdf_printf(pdf, "%s", s);
    xfree(s);
}


void pdf_print_toks_ln(PDF pdf, halfword p)
{
    int len = 0;
    char *s = tokenlist_to_cstring(p, true, &len);
    if (len > 0)
        pdf_printf(pdf, "%s\n", s);
    xfree(s);
}

/* prints a rect spec */
void pdf_print_rect_spec(PDF pdf, halfword r)
{
    pdf_print_mag_bp(pdf, pdf_ann_left(r));
    pdf_out(pdf, ' ');
    pdf_print_mag_bp(pdf, pdf_ann_bottom(r));
    pdf_out(pdf, ' ');
    pdf_print_mag_bp(pdf, pdf_ann_right(r));
    pdf_out(pdf, ' ');
    pdf_print_mag_bp(pdf, pdf_ann_top(r));
}

/* output a rectangle specification to PDF file */
void pdf_rectangle(PDF pdf, halfword r)
{
    prepare_mag();
    pdf_printf(pdf, "/Rect [");
    pdf_print_rect_spec(pdf, r);
    pdf_printf(pdf, "]\n");
}


/* begin a PDF dictionary object */
void pdf_begin_dict(PDF pdf, integer i, integer pdf_os_level)
{
    check_pdfminorversion(pdf);
    pdf_os_prepare_obj(pdf, i, pdf_os_level);
    if (!pdf->os_mode) {
        pdf_printf(pdf, "%d 0 obj <<\n", (int) i);
    } else {
        if (pdf_compress_level == 0)
            pdf_printf(pdf, "%% %d 0 obj\n", (int) i);  /* debugging help */
        pdf_printf(pdf, "<<\n");
    }
}

/* begin a new PDF dictionary object */
void pdf_new_dict(PDF pdf, integer t, integer i, integer pdf_os)
{
    pdf_create_obj(t, i);
    pdf_begin_dict(pdf, obj_ptr, pdf_os);
}

/* end a PDF dictionary object */
void pdf_end_dict(PDF pdf)
{
    if (pdf->os_mode) {
        pdf_printf(pdf, ">>\n");
        if (pdf->os_idx == pdf_os_max_objs - 1)
            pdf_os_write_objstream(pdf);
    } else {
        pdf_printf(pdf, ">> endobj\n");
    }
}


/*
Write out an accumulated object stream.
First the object number and byte offset pairs are generated
and appended to the ready buffered object stream.
By this the value of \.{/First} can be calculated.
Then a new \.{/ObjStm} object is generated, and everything is
copied to the PDF output buffer, where also compression is done.
When calling this procedure, |pdf_os_mode| must be |true|.
*/

void pdf_os_write_objstream(PDF pdf)
{
    halfword i, j, p, q;
    if (pdf->os_cur_objnum == 0)        /* no object stream started */
        return;
    p = pdf->ptr;
    i = 0;
    j = 0;
    while (i <= pdf->os_idx) {  /* assemble object number and byte offset pairs */
        pdf_printf(pdf, "%d %d", (int) pdf->os_obj[i].num,
                   (int) pdf->os_obj[i].off);
        if (j == 9) {           /* print out in groups of ten for better readability */
            pdf_out(pdf, pdf_new_line_char);
            j = 0;
        } else {
            pdf_printf(pdf, " ");
            incr(j);
        }
        incr(i);
    }
    pdf->buf[pdf->ptr - 1] = pdf_new_line_char; /* no risk of flush, as we are in |pdf_os_mode| */
    q = pdf->ptr;
    pdf_begin_dict(pdf, pdf->os_cur_objnum, 0); /* switch to PDF stream writing */
    pdf_printf(pdf, "/Type /ObjStm\n");
    pdf_printf(pdf, "/N %d\n", (int) (pdf->os_idx + 1));
    pdf_printf(pdf, "/First %d\n", (int) (q - p));
    pdf_begin_stream(pdf);
    pdf_room(pdf, q - p);       /* should always fit into the PDF output buffer */
    i = p;
    while (i < q) {             /* write object number and byte offset pairs */
        pdf_quick_out(pdf, pdf->os_buf[i]);
        incr(i);
    }
    i = 0;
    while (i < p) {
        q = i + pdf->buf_size;
        if (q > p)
            q = p;
        pdf_room(pdf, q - i);
        while (i < q) {         /* write the buffered objects */
            pdf_quick_out(pdf, pdf->os_buf[i]);
            incr(i);
        }
    }
    pdf_end_stream(pdf);
    pdf->os_cur_objnum = 0;     /* to force object stream generation next time */
}

/* begin a PDF object */
void pdf_begin_obj(PDF pdf, integer i, integer pdf_os_level)
{
    check_pdfminorversion(pdf);
    pdf_os_prepare_obj(pdf, i, pdf_os_level);
    if (!pdf->os_mode) {
        pdf_printf(pdf, "%d 0 obj\n", (int) i);
    } else if (pdf_compress_level == 0) {
        pdf_printf(pdf, "%% %d 0 obj\n", (int) i);      /* debugging help */
    }
}

/* begin a new PDF object */
void pdf_new_obj(PDF pdf, integer t, integer i, integer pdf_os)
{
    pdf_create_obj(t, i);
    pdf_begin_obj(pdf, obj_ptr, pdf_os);
}


/* end a PDF object */
void pdf_end_obj(PDF pdf)
{
    if (pdf->os_mode) {
        if (pdf->os_idx == pdf_os_max_objs - 1)
            pdf_os_write_objstream(pdf);
    } else {
        pdf_printf(pdf, "endobj\n");    /* end a PDF object */
    }
}


void write_stream_length(PDF pdf, integer length, longinteger offset)
{
    if (jobname_cstr == NULL)
        jobname_cstr = xstrdup(makecstring(job_name));
    if (pdf->draftmode == 0) {
        xfseeko(pdf->file, (off_t) offset, SEEK_SET, jobname_cstr);
        fprintf(pdf->file, "%li", (long int) length);
        xfseeko(pdf->file, pdf_offset(pdf), SEEK_SET, jobname_cstr);
    }
}


/* Converts any string given in in in an allowed PDF string which can be
 * handled by printf et.al.: \ is escaped to \\, parenthesis are escaped and
 * control characters are octal encoded.
 * This assumes that the string does not contain any already escaped
 * characters!
 */
char *convertStringToPDFString(const char *in, int len)
{
    static char pstrbuf[MAX_PSTRING_LEN];
    char *out = pstrbuf;
    int i, j, k;
    char buf[5];
    j = 0;
    for (i = 0; i < len; i++) {
        check_buf(j + sizeof(buf), MAX_PSTRING_LEN);
        if (((unsigned char) in[i] < '!') || ((unsigned char) in[i] > '~')) {
            /* convert control characters into oct */
            k = snprintf(buf, sizeof(buf),
                         "\\%03o", (unsigned int) (unsigned char) in[i]);
            check_nprintf(k, sizeof(buf));
            out[j++] = buf[0];
            out[j++] = buf[1];
            out[j++] = buf[2];
            out[j++] = buf[3];
        } else if ((in[i] == '(') || (in[i] == ')')) {
            /* escape paranthesis */
            out[j++] = '\\';
            out[j++] = in[i];
        } else if (in[i] == '\\') {
            /* escape backslash */
            out[j++] = '\\';
            out[j++] = '\\';
        } else {
            /* copy char :-) */
            out[j++] = in[i];
        }
    }
    out[j] = '\0';
    return pstrbuf;
}

/* Converts any string given in in in an allowed PDF string which is
 * hexadecimal encoded;
 * sizeof(out) should be at least lin*2+1.
 */

static void convertStringToHexString(const char *in, char *out, int lin)
{
    int i, j, k;
    char buf[3];
    j = 0;
    for (i = 0; i < lin; i++) {
        k = snprintf(buf, sizeof(buf),
                     "%02X", (unsigned int) (unsigned char) in[i]);
        check_nprintf(k, sizeof(buf));
        out[j++] = buf[0];
        out[j++] = buf[1];
    }
    out[j] = '\0';
}


/* Converts any string given in in in an allowed PDF string which can be
 * handled by printf et.al.: \ is escaped to \\, parenthesis are escaped and
 * control characters are octal encoded.
 * This assumes that the string does not contain any already escaped
 * characters!
 *
 * See escapename for parameter description.
 */
void escapestring(poolpointer in)
{
    const poolpointer out = pool_ptr;
    unsigned char ch;
    int i;
    while (in < out) {
        if (pool_ptr + 4 >= pool_size) {
            pool_ptr = pool_size;
            /* error by str_toks that calls str_room(1) */
            return;
        }

        ch = (unsigned char) str_pool[in++];

        if ((ch < '!') || (ch > '~')) {
            /* convert control characters into oct */
            i = snprintf((char *) &str_pool[pool_ptr], 5,
                         "\\%.3o", (unsigned int) ch);
            check_nprintf(i, 5);
            pool_ptr += i;
            continue;
        }
        if ((ch == '(') || (ch == ')') || (ch == '\\')) {
            /* escape parenthesis and backslash */
            str_pool[pool_ptr++] = '\\';
        }
        /* copy char :-) */
        str_pool[pool_ptr++] = ch;
    }
}

/* Convert any given string in a PDF name using escaping mechanism
   of PDF 1.2. The result does not include the leading slash.

   PDF specification 1.6, section 3.2.6 "Name Objects" explains:
   <blockquote>
    Beginning with PDF 1.2, any character except null (character code 0) may
    be included in a name by writing its 2-digit hexadecimal code, preceded
    by the number sign character (#); see implementation notes 3 and 4 in
    Appendix H. This syntax is required to represent any of the delimiter or
    white-space characters or the number sign character itself; it is
    recommended but not required for characters whose codes are outside the
    range 33 (!) to 126 (~).
   </blockquote>
   The following table shows the conversion that are done by this
   function:
     code      result   reason
     -----------------------------------
     0         ignored  not allowed
     1..32     escaped  must for white-space:
                          9 (tab), 10 (lf), 12 (ff), 13 (cr), 32 (space)
                        recommended for the other control characters
     35        escaped  escape char "#"
     37        escaped  delimiter "%"
     40..41    escaped  delimiters "(" and ")"
     47        escaped  delimiter "/"
     60        escaped  delimiter "<"
     62        escaped  delimiter ">"
     91        escaped  delimiter "["
     93        escaped  delimiter "]"
     123       escaped  delimiter "{"
     125       escaped  delimiter "}"
     127..255  escaped  recommended
     else      copy     regular characters

   Parameter "in" is a pointer into the string pool where
   the input string is located. The output string is written
   as temporary string right after the input string.
   Thus at the begin of the procedure the global variable
   "pool_ptr" points to the start of the output string and
   after the end when the procedure returns.
*/
void escapename(poolpointer in)
{
    const poolpointer out = pool_ptr;
    unsigned char ch;
    int i;

    while (in < out) {
        if (pool_ptr + 3 >= pool_size) {
            pool_ptr = pool_size;
            /* error by str_toks that calls str_room(1) */
            return;
        }

        ch = (unsigned char) str_pool[in++];

        if ((ch >= 1 && ch <= 32) || ch >= 127) {
            /* escape */
            i = snprintf((char *) &str_pool[pool_ptr], 4,
                         "#%.2X", (unsigned int) ch);
            check_nprintf(i, 4);
            pool_ptr += i;
            continue;
        }
        switch (ch) {
        case 0:
            /* ignore */
            break;
        case 35:
        case 37:
        case 40:
        case 41:
        case 47:
        case 60:
        case 62:
        case 91:
        case 93:
        case 123:
        case 125:
            /* escape */
            i = snprintf((char *) &str_pool[pool_ptr], 4,
                         "#%.2X", (unsigned int) ch);
            check_nprintf(i, 4);
            pool_ptr += i;
            break;
        default:
            /* copy */
            str_pool[pool_ptr++] = ch;
        }
    }
}

/* Compute the ID string as per PDF1.4 9.3:
  <blockquote>
    File identifers are defined by the optional ID entry in a PDF file's
    trailer dictionary (see Section 3.4.4, "File Trailer"; see also
    implementation note 105 in Appendix H). The value of this entry is an
    array of two strings. The first string is a permanent identifier based
    on the contents of the file at the time it was originally created, and
    does not change when the file is incrementally updated. The second
    string is a changing identifier based on the file's contents at the
    time it was last updated. When a file is first written, both
    identifiers are set to the same value. If both identifiers match when a
    file reference is resolved, it is very likely that the correct file has
    been found; if only the first identifier matches, then a different
    version of the correct file has been found.
        To help ensure the uniqueness of file identifiers, it is recommend
    that they be computed using a message digest algorithm such as MD5
    (described in Internet RFC 1321, The MD5 Message-Digest Algorithm; see
    the Bibliography), using the following information (see implementation
    note 106 in Appendix H):
    - The current time
    - A string representation of the file's location, usually a pathname
    - The size of the file in bytes
    - The values of all entries in the file's document information
      dictionary (see Section 9.2.1,  Document Information Dictionary )
  </blockquote>
  This stipulates only that the two IDs must be identical when the file is
  created and that they should be reasonably unique. Since it's difficult
  to get the file size at this point in the execution of pdfTeX and
  scanning the info dict is also difficult, we start with a simpler
  implementation using just the first two items.
 */
void print_ID(PDF pdf, str_number filename)
{
    time_t t;
    size_t size;
    char time_str[32];
    md5_state_t state;
    md5_byte_t digest[16];
    char id[64];
    char *file_name;
    char pwd[4096];
    /* start md5 */
    md5_init(&state);
    /* get the time */
    t = time(NULL);
    size = strftime(time_str, sizeof(time_str), "%Y%m%dT%H%M%SZ", gmtime(&t));
    md5_append(&state, (const md5_byte_t *) time_str, size);
    /* get the file name */
    if (getcwd(pwd, sizeof(pwd)) == NULL)
        pdftex_fail("getcwd() failed (%s), (path too long?)", strerror(errno));
    file_name = makecstring(filename);
    md5_append(&state, (const md5_byte_t *) pwd, strlen(pwd));
    md5_append(&state, (const md5_byte_t *) "/", 1);
    md5_append(&state, (const md5_byte_t *) file_name, strlen(file_name));
    /* finish md5 */
    md5_finish(&state, digest);
    /* write the IDs */
    convertStringToHexString((char *) digest, id, 16);
    pdf_printf(pdf, "/ID [<%s> <%s>]", id, id);
}

/* Print the /CreationDate entry.

  PDF Reference, third edition says about the expected date format:
  <blockquote>
    3.8.2 Dates

      PDF defines a standard date format, which closely follows that of
      the international standard ASN.1 (Abstract Syntax Notation One),
      defined in ISO/IEC 8824 (see the Bibliography). A date is a string
      of the form

        (D:YYYYMMDDHHmmSSOHH'mm')

      where

        YYYY is the year
        MM is the month
        DD is the day (01-31)
        HH is the hour (00-23)
        mm is the minute (00-59)
        SS is the second (00-59)
        O is the relationship of local time to Universal Time (UT),
          denoted by one of the characters +, -, or Z (see below)
        HH followed by ' is the absolute value of the offset from UT
          in hours (00-23)
        mm followed by ' is the absolute value of the offset from UT
          in minutes (00-59)

      The apostrophe character (') after HH and mm is part of the syntax.
      All fields after the year are optional. (The prefix D:, although also
      optional, is strongly recommended.) The default values for MM and DD
      are both 01; all other numerical fields default to zero values.  A plus
      sign (+) as the value of the O field signifies that local time is
      later than UT, a minus sign (-) that local time is earlier than UT,
      and the letter Z that local time is equal to UT. If no UT information
      is specified, the relationship of the specified time to UT is
      considered to be unknown. Whether or not the time zone is known, the
      rest of the date should be specified in local time.

      For example, December 23, 1998, at 7:52 PM, U.S. Pacific Standard
      Time, is represented by the string

        D:199812231952-08'00'
  </blockquote>

  The main difficulty is get the time zone offset. strftime() does this in ISO
  C99 (e.g. newer glibc) with %z, but we have to work with other systems (e.g.
  Solaris 2.5).
*/

static time_t start_time = 0;
#define TIME_STR_SIZE 30
static char start_time_str[TIME_STR_SIZE];      /* minimum size for time_str is 24: "D:YYYYmmddHHMMSS+HH'MM'" */

static void makepdftime(time_t t, char *time_str)
{
    struct tm lt, gmt;
    size_t size;
    int i, off, off_hours, off_mins;

    /* get the time */
    lt = *localtime(&t);
    size = strftime(time_str, TIME_STR_SIZE, "D:%Y%m%d%H%M%S", &lt);
    /* expected format: "YYYYmmddHHMMSS" */
    if (size == 0) {
        /* unexpected, contents of time_str is undefined */
        time_str[0] = '\0';
        return;
    }

    /* correction for seconds: %S can be in range 00..61,
       the PDF reference expects 00..59,
       therefore we map "60" and "61" to "59" */
    if (time_str[14] == '6') {
        time_str[14] = '5';
        time_str[15] = '9';
        time_str[16] = '\0';    /* for safety */
    }

    /* get the time zone offset */
    gmt = *gmtime(&t);

    /* this calculation method was found in exim's tod.c */
    off = 60 * (lt.tm_hour - gmt.tm_hour) + lt.tm_min - gmt.tm_min;
    if (lt.tm_year != gmt.tm_year) {
        off += (lt.tm_year > gmt.tm_year) ? 1440 : -1440;
    } else if (lt.tm_yday != gmt.tm_yday) {
        off += (lt.tm_yday > gmt.tm_yday) ? 1440 : -1440;
    }

    if (off == 0) {
        time_str[size++] = 'Z';
        time_str[size] = 0;
    } else {
        off_hours = off / 60;
        off_mins = abs(off - off_hours * 60);
        i = snprintf(&time_str[size], 9, "%+03d'%02d'", off_hours, off_mins);
        check_nprintf(i, 9);
    }
}

void init_start_time(void)
{
    if (start_time == 0) {
        start_time = time((time_t *) NULL);
        makepdftime(start_time, start_time_str);
    }
}

void print_creation_date(PDF pdf)
{
    init_start_time();
    pdf_printf(pdf, "/CreationDate (%s)\n", start_time_str);
}

void print_mod_date(PDF pdf)
{
    init_start_time();
    pdf_printf(pdf, "/ModDate (%s)\n", start_time_str);
}

void getcreationdate(void)
{
    /* put creation date on top of string pool and update pool_ptr */
    size_t len = strlen(start_time_str);

    init_start_time();

    if ((unsigned) (pool_ptr + len) >= (unsigned) pool_size) {
        pool_ptr = pool_size;
        /* error by str_toks that calls str_room(1) */
        return;
    }

    memcpy(&str_pool[pool_ptr], start_time_str, len);
    pool_ptr += len;
}

void remove_pdffile(PDF pdf)
{
    if (pdf != NULL) {
        if (!kpathsea_debug && output_file_name && (pdf->draftmode != 0)) {
            xfclose(pdf->file, makecstring(output_file_name));
            remove(makecstring(output_file_name));
        }
    }
}

/* define fb_ptr, fb_array & fb_limit */
typedef char fb_entry;
define_array(fb);
integer fb_offset(void)
{
    return fb_ptr - fb_array;
}

void fb_seek(integer offset)
{
    fb_ptr = fb_array + offset;
}

void fb_putchar(eight_bits b)
{
    alloc_array(fb, 1, SMALL_ARRAY_SIZE);
    *fb_ptr++ = b;
}

void fb_flush(PDF pdf)
{
    fb_entry *p;
    integer n;
    for (p = fb_array; p < fb_ptr;) {
        n = pdf->buf_size - pdf->ptr;
        if (fb_ptr - p < n)
            n = fb_ptr - p;
        memcpy(pdf->buf + pdf->ptr, p, (unsigned) n);
        pdf->ptr += n;
        if (pdf->ptr == pdf->buf_size)
            pdf_flush(pdf);
        p += n;
    }
    fb_ptr = fb_array;
}

void fb_free(void)
{
    xfree(fb_array);
}


#define ZIP_BUF_SIZE  32768

#define check_err(f, fn)                        \
  if (f != Z_OK)                                \
    pdftex_fail("zlib: %s() failed (error code %d)", fn, f)

static char *zipbuf = NULL;
static z_stream c_stream;       /* compression stream */

void write_zip(PDF pdf, boolean finish)
{
    int err;
    static int level_old = 0;
    int level = pdf_compress_level;
    assert(level > 0);
    cur_file_name = NULL;
    if (pdf_stream_length == 0) {
        if (zipbuf == NULL) {
            zipbuf = xtalloc(ZIP_BUF_SIZE, char);
            c_stream.zalloc = (alloc_func) 0;
            c_stream.zfree = (free_func) 0;
            c_stream.opaque = (voidpf) 0;
            check_err(deflateInit(&c_stream, level), "deflateInit");
        } else {
            if (level != level_old) {   /* \pdfcompresslevel change in mid document */
                check_err(deflateEnd(&c_stream), "deflateEnd");
                c_stream.zalloc = (alloc_func) 0;       /* these 3 lines no need, just to be safe */
                c_stream.zfree = (free_func) 0;
                c_stream.opaque = (voidpf) 0;
                check_err(deflateInit(&c_stream, level), "deflateInit");
            } else
                check_err(deflateReset(&c_stream), "deflateReset");
        }
        level_old = level;
        c_stream.next_out = (Bytef *) zipbuf;
        c_stream.avail_out = ZIP_BUF_SIZE;
    }
    assert(zipbuf != NULL);
    c_stream.next_in = pdf->buf;
    c_stream.avail_in = pdf->ptr;
    for (;;) {
        if (c_stream.avail_out == 0) {
            pdf->gone += xfwrite(zipbuf, 1, ZIP_BUF_SIZE, pdf->file);
            pdf_last_byte = zipbuf[ZIP_BUF_SIZE - 1];   /* not needed */
            c_stream.next_out = (Bytef *) zipbuf;
            c_stream.avail_out = ZIP_BUF_SIZE;
        }
        err = deflate(&c_stream, finish ? Z_FINISH : Z_NO_FLUSH);
        if (finish && err == Z_STREAM_END)
            break;
        check_err(err, "deflate");
        if (!finish && c_stream.avail_in == 0)
            break;
    }
    if (finish) {
        if (c_stream.avail_out < ZIP_BUF_SIZE) {        /* at least one byte has been output */
            pdf->gone +=
                xfwrite(zipbuf, 1, ZIP_BUF_SIZE - c_stream.avail_out,
                        pdf->file);
            pdf_last_byte = zipbuf[ZIP_BUF_SIZE - c_stream.avail_out - 1];
        }
        xfflush(pdf->file);
    }
    pdf_stream_length = c_stream.total_out;
}

void zip_free(void)
{
    if (zipbuf != NULL) {
        check_err(deflateEnd(&c_stream), "deflateEnd");
        free(zipbuf);
    }
}
