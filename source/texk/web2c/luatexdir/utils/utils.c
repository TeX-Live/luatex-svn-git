/* utils.c

   Copyright 1996-2006 Han The Thanh <thanh@pdftex.org>
   Copyright 2006-2009 Taco Hoekwater <taco@luatex.org>

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

#include "openbsd-compat.h"
#ifdef HAVE_ASPRINTF            /* asprintf is not defined in openbsd-compat.h, but in stdio.h */
#  include <stdio.h>
#endif

#include "sys/types.h"
#ifndef __MINGW32__
#  include "sysexits.h"
#else
#  define EX_SOFTWARE 70
#endif
#include <kpathsea/c-proto.h>
#include <kpathsea/c-stat.h>
#include <kpathsea/c-fopen.h>
#include <string.h>
#include <time.h>
#include <float.h>              /* for DBL_EPSILON */
#include "zlib.h"
#include "ptexlib.h"
#include "md5.h"

#include "png.h"
#ifdef POPPLER_VERSION
#  define xpdfString "poppler"
#  include "poppler-config.h"
#  define xpdfVersion POPPLER_VERSION
#else
#  define xpdfString "xpdf"
#  include "xpdf/config.h"      /* just to get the xpdf version */
#endif

static const char __svn_version[] =
    "$Id$ "
    "$URL$";

#define check_nprintf(size_get, size_want) \
    if ((unsigned)(size_get) >= (unsigned)(size_want)) \
        pdftex_fail ("snprintf failed: file %s, line %d", __FILE__, __LINE__);

char *cur_file_name = NULL;
str_number last_tex_string;
static char print_buf[PRINTF_BUF_SIZE];
char *job_id_string = NULL;
extern string ptexbanner;       /* from web2c/lib/texmfmp.c */
extern string versionstring;    /* from web2c/lib/version.c */
extern KPSEDLL string kpathsea_version_string;  /* from kpathsea/version.c */

size_t last_ptr_index;          /* for use with alloc_array */

extern PDF static_pdf;

/* define char_ptr, char_array & char_limit */
typedef char char_entry;
define_array(char);

#define SUBSET_TAG_LENGTH 6
void make_subset_tag(fd_entry * fd)
{
    int i, j = 0, a[SUBSET_TAG_LENGTH];
    md5_state_t pms;
    char *glyph;
    glw_entry *glw_glyph;
    struct avl_traverser t;
    md5_byte_t digest[16];
    void **aa;
    static struct avl_table *st_tree = NULL;
    if (st_tree == NULL)
        st_tree = avl_create(comp_string_entry, NULL, &avl_xallocator);
    assert(fd != NULL);
    assert(fd->gl_tree != NULL);
    assert(fd->fontname != NULL);
    assert(fd->subset_tag == NULL);
    fd->subset_tag = xtalloc(SUBSET_TAG_LENGTH + 1, char);
    do {
        md5_init(&pms);
        avl_t_init(&t, fd->gl_tree);
        if (is_cidkeyed(fd->fm)) {      /* glw_entry items */
            for (glw_glyph = (glw_entry *) avl_t_first(&t, fd->gl_tree);
                 glw_glyph != NULL; glw_glyph = (glw_entry *) avl_t_next(&t)) {
                glyph = malloc(24);
                sprintf(glyph, "%05u%05u ", glw_glyph->id, glw_glyph->wd);
                md5_append(&pms, (md5_byte_t *) glyph, strlen(glyph));
                free(glyph);
            }
        } else {
            for (glyph = (char *) avl_t_first(&t, fd->gl_tree); glyph != NULL;
                 glyph = (char *) avl_t_next(&t)) {
                md5_append(&pms, (md5_byte_t *) glyph, strlen(glyph));
                md5_append(&pms, (md5_byte_t *) " ", 1);
            }
        }
        md5_append(&pms, (md5_byte_t *) fd->fontname, strlen(fd->fontname));
        md5_append(&pms, (md5_byte_t *) & j, sizeof(int));      /* to resolve collision */
        md5_finish(&pms, digest);
        for (a[0] = 0, i = 0; i < 13; i++)
            a[0] += digest[i];
        for (i = 1; i < SUBSET_TAG_LENGTH; i++)
            a[i] = a[i - 1] - digest[i - 1] + digest[(i + 12) % 16];
        for (i = 0; i < SUBSET_TAG_LENGTH; i++)
            fd->subset_tag[i] = a[i] % 26 + 'A';
        fd->subset_tag[SUBSET_TAG_LENGTH] = '\0';
        j++;
        assert(j < 100);
    }
    while ((char *) avl_find(st_tree, fd->subset_tag) != NULL);
    aa = avl_probe(st_tree, fd->subset_tag);
    assert(aa != NULL);
    if (j > 2)
        pdftex_warn
            ("\nmake_subset_tag(): subset-tag collision, resolved in round %d.\n",
             j);
}

str_number maketexstring(const char *s)
{
    if (s == NULL || *s == 0)
        return get_nullstr();
    return maketexlstring(s, strlen(s));
}

str_number maketexlstring(const char *s, size_t l)
{
    if (s == NULL || l == 0)
        return get_nullstr();
    check_pool_overflow(pool_ptr + l);
    while (l-- > 0)
        str_pool[pool_ptr++] = *s++;
    last_tex_string = make_string();
    return last_tex_string;
}

/* append a C string to a TeX string */
void append_string(char *s)
{
    if (s == NULL || *s == 0)
        return;
    check_buf(pool_ptr + strlen(s), pool_size);
    while (*s)
        str_pool[pool_ptr++] = *s++;
    return;
}

__attribute__ ((format(printf, 1, 2)))
void tex_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(print_buf, PRINTF_BUF_SIZE, fmt, args);
    tprint(print_buf);
    xfflush(stdout);
    va_end(args);
}

/* pdftex_fail may be called when a buffer overflow has happened/is
   happening, therefore may not call mktexstring.  However, with the
   current implementation it appears that error messages are misleading,
   possibly because pool overflows are detected too late.

   The output format of this fuction must be the same as pdf_error in
   pdftex.web! */

__attribute__ ((noreturn, format(printf, 1, 2)))
void pdftex_fail(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    print_ln();
    tprint("!LuaTeX error");
    if (cur_file_name) {
        tprint(" (file ");
        tprint(cur_file_name);
        tprint(")");
    }
    tprint(": ");
    vsnprintf(print_buf, PRINTF_BUF_SIZE, fmt, args);
    tprint(print_buf);
    va_end(args);
    print_ln();
    remove_pdffile(static_pdf);
    tprint(" ==> Fatal error occurred, no output PDF file produced!");
    print_ln();
    if (kpathsea_debug) {
        abort();
    } else {
        exit(EX_SOFTWARE);
    }
}

/* The output format of this fuction must be the same as pdf_warn in
   pdftex.web! */

__attribute__ ((format(printf, 1, 2)))
void pdftex_warn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    print_ln();
    tex_printf("LuaTeX warning");
    if (cur_file_name)
        tex_printf(" (file %s)", cur_file_name);
    tex_printf(": ");
    vsnprintf(print_buf, PRINTF_BUF_SIZE, fmt, args);
    tprint(print_buf);
    va_end(args);
    print_ln();
}

void garbage_warning(void)
{
    pdftex_warn("dangling objects discarded, no output file produced.");
    remove_pdffile(static_pdf);
}

void tex_error(char *msg, char **hlp)
{
    str_number aa = 0, bb = 0, cc = 0, dd = 0, ee = 0;
    int k = 0;
    if (hlp != NULL) {
        while (hlp[k] != NULL)
            k++;
        if (k > 0)
            aa = maketexstring(hlp[0]);
        if (k > 1)
            bb = maketexstring(hlp[1]);
        if (k > 2)
            cc = maketexstring(hlp[2]);
        if (k > 3)
            dd = maketexstring(hlp[3]);
        if (k > 4)
            ee = maketexstring(hlp[4]);
    }
    do_print_err(maketexstring(msg));
    flush_str(last_tex_string);

/* *INDENT-OFF* */
    switch (k) {
    case 0: dohelp0(); break;
    case 1: dohelp1(aa); break;
    case 2: dohelp2(aa, bb); break;
    case 3: dohelp3(aa, bb, cc); break;
    case 4: dohelp4(aa, bb, cc, dd); break;
    case 5: dohelp5(aa, bb, cc, dd, ee); break;
    }
/* *INDENT-ON* */
    error();

    if (ee)
        flush_str(ee);
    if (dd)
        flush_str(dd);
    if (cc)
        flush_str(cc);
    if (bb)
        flush_str(bb);
    if (aa)
        flush_str(aa);
}

char *makecstring(integer s)
{
    size_t l;
    return makeclstring(s, &l);
}

char *makeclstring(integer s, size_t * len)
{
    static char *cstrbuf = NULL;
    char *p;
    static int allocsize;
    int allocgrow, i, l;
    if (s >= 2097152) {
        s -= 2097152;
        l = str_start[s + 1] - str_start[s];
        *len = l;
        check_buf(l + 1, MAX_CSTRING_LEN);
        if (cstrbuf == NULL) {
            allocsize = l + 1;
            cstrbuf = xmallocarray(char, allocsize);
        } else if (l + 1 > allocsize) {
            allocgrow = allocsize * 0.2;
            if (l + 1 - allocgrow > allocsize)
                allocsize = l + 1;
            else if (allocsize < MAX_CSTRING_LEN - allocgrow)
                allocsize += allocgrow;
            else
                allocsize = MAX_CSTRING_LEN;
            cstrbuf = xreallocarray(cstrbuf, char, allocsize);
        }
        p = cstrbuf;
        for (i = 0; i < l; i++)
            *p++ = str_pool[i + str_start[s]];
        *p = 0;
    } else {
        if (cstrbuf == NULL) {
            allocsize = 5;
            cstrbuf = xmallocarray(char, allocsize);
        }
        if (s <= 0x7F) {
            cstrbuf[0] = s;
            cstrbuf[1] = 0;
            *len = 1;
        } else if (s <= 0x7FF) {
            cstrbuf[0] = 0xC0 + (s / 0x40);
            cstrbuf[1] = 0x80 + (s % 0x40);
            cstrbuf[2] = 0;
            *len = 2;
        } else if (s <= 0xFFFF) {
            cstrbuf[0] = 0xE0 + (s / 0x1000);
            cstrbuf[1] = 0x80 + ((s % 0x1000) / 0x40);
            cstrbuf[2] = 0x80 + ((s % 0x1000) % 0x40);
            cstrbuf[3] = 0;
            *len = 3;
        } else {
            if (s >= 0x10FF00) {
                cstrbuf[0] = s - 0x10FF00;
                cstrbuf[1] = 0;
                *len = 1;
            } else {
                cstrbuf[0] = 0xF0 + (s / 0x40000);
                cstrbuf[1] = 0x80 + ((s % 0x40000) / 0x1000);
                cstrbuf[2] = 0x80 + (((s % 0x40000) % 0x1000) / 0x40);
                cstrbuf[3] = 0x80 + (((s % 0x40000) % 0x1000) % 0x40);
                cstrbuf[4] = 0;
                *len = 4;
            }
        }
    }
    return cstrbuf;
}

void set_job_id(int year, int month, int day, int time)
{
    char *name_string, *format_string, *s;
    size_t slen;
    int i;

    if (job_id_string != NULL)
        return;

    name_string = xstrdup(makecstring(job_name));
    format_string = xstrdup(makecstring(format_ident));
    slen = SMALL_BUF_SIZE +
        strlen(name_string) +
        strlen(format_string) +
        strlen(ptexbanner) +
        strlen(versionstring) + strlen(kpathsea_version_string);
    s = xtalloc(slen, char);
    /* The Web2c version string starts with a space.  */
    i = snprintf(s, slen,
                 "%.4d/%.2d/%.2d %.2d:%.2d %s %s %s%s %s",
                 year, month, day, time / 60, time % 60,
                 name_string, format_string, ptexbanner,
                 versionstring, kpathsea_version_string);
    check_nprintf(i, slen);
    job_id_string = xstrdup(s);
    xfree(s);
    xfree(name_string);
    xfree(format_string);
}

void make_pdftex_banner(void)
{
    static boolean pdftexbanner_init = false;
    char *s;
    size_t slen;
    int i;

    if (pdftexbanner_init)
        return;

    slen = SMALL_BUF_SIZE +
        strlen(ptexbanner) +
        strlen(versionstring) + strlen(kpathsea_version_string);
    s = xtalloc(slen, char);
    /* The Web2c version string starts with a space.  */
    i = snprintf(s, slen,
                 "%s%s %s", ptexbanner, versionstring, kpathsea_version_string);
    check_nprintf(i, slen);
    pdftex_banner = maketexstring(s);
    xfree(s);
    pdftexbanner_init = true;
}

str_number get_resname_prefix(void)
{
/*     static char name_str[] = */
/* "!\"$&'*+,-.0123456789:;=?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\" */
/* "^_`abcdefghijklmnopqrstuvwxyz|~"; */
    static char name_str[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    char prefix[7];             /* make a tag of 6 chars long */
    unsigned long crc;
    short i;
    size_t base = strlen(name_str);
    crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (Bytef *) job_id_string, strlen(job_id_string));
    for (i = 0; i < 6; i++) {
        prefix[i] = name_str[crc % base];
        crc /= base;
    }
    prefix[6] = 0;
    return maketexstring(prefix);
}

size_t xfwrite(void *ptr, size_t size, size_t nmemb, FILE * stream)
{
    if (fwrite(ptr, size, nmemb, stream) != nmemb)
        pdftex_fail("fwrite() failed");
    return nmemb;
}

int xfflush(FILE * stream)
{
    if (fflush(stream) != 0)
        pdftex_fail("fflush() failed (%s)", strerror(errno));
    return 0;
}

int xgetc(FILE * stream)
{
    int c = getc(stream);
    if (c < 0 && c != EOF)
        pdftex_fail("getc() failed (%s)", strerror(errno));
    return c;
}

int xputc(int c, FILE * stream)
{
    int i = putc(c, stream);
    if (i < 0)
        pdftex_fail("putc() failed (%s)", strerror(errno));
    return i;
}

scaled ext_xn_over_d(scaled x, scaled n, scaled d)
{
    double r = (((double) x) * ((double) n)) / ((double) d);
    if (r > DBL_EPSILON)
        r += 0.5;
    else
        r -= 0.5;
    if (r >= (double) max_integer || r <= -(double) max_integer)
        pdftex_warn("arithmetic: number too big");
    return (scaled) r;
}

/* Convert any given string in a PDF hexadecimal string. The
   result does not include the angle brackets.

   This procedure uses uppercase hexadecimal letters.

   See escapename for description of parameters.
*/
void escapehex(poolpointer in)
{
    const poolpointer out = pool_ptr;
    unsigned char ch;
    int i;

    while (in < out) {
        if (pool_ptr + 2 >= pool_size) {
            pool_ptr = pool_size;
            /* error by str_toks that calls str_room(1) */
            return;
        }

        ch = (unsigned char) str_pool[in++];

        i = snprintf((char *) &str_pool[pool_ptr], 3, "%.2X",
                     (unsigned int) ch);
        check_nprintf(i, 3);
        pool_ptr += 2;
    }
}

/* Unescape any given hexadecimal string.

   Last hex digit can be omitted, it is replaced by zero, see
   PDF specification.

   Invalid digits are silently ignored.

   See escapename for description of parameters.
*/
void unescapehex(poolpointer in)
{
    const poolpointer out = pool_ptr;
    unsigned char ch;
    boolean first = true;
    unsigned char a = 0;        /* to avoid warning about uninitialized use of a */
    while (in < out) {
        if (pool_ptr + 1 >= pool_size) {
            pool_ptr = pool_size;
            /* error by str_toks that calls str_room(1) */
            return;
        }

        ch = (unsigned char) str_pool[in++];

        if ((ch >= '0') && (ch <= '9')) {
            ch -= '0';
        } else if ((ch >= 'A') && (ch <= 'F')) {
            ch -= 'A' - 10;
        } else if ((ch >= 'a') && (ch <= 'f')) {
            ch -= 'a' - 10;
        } else {
            continue;           /* ignore wrong character */
        }

        if (first) {
            a = ch << 4;
            first = false;
            continue;
        }

        str_pool[pool_ptr++] = a + ch;
        first = true;
    }
    if (!first) {               /* last hex digit is omitted */
        str_pool[pool_ptr++] = a;
    }
}

/* makecfilename
  input/ouput same as makecstring:
    input: string number
    output: C string with quotes removed.
    That means, file names that are legal on some operation systems
    cannot any more be used since pdfTeX version 1.30.4.
*/
char *makecfilename(str_number s)
{
    char *name = makecstring(s);
    char *p = name;
    char *q = name;

    while (*p) {
        if (*p != '"')
            *q++ = *p;
        p++;
    }
    *q = '\0';
    return name;
}

/* function strips trailing zeros in string with numbers; */
/* leading zeros are not stripped (as in real life) */
char *stripzeros(char *a)
{
    enum { NONUM, DOTNONUM, INT, DOT, LEADDOT, FRAC } s = NONUM, t = NONUM;
    char *p, *q, *r;
    for (p = q = r = a; *p != '\0';) {
        switch (s) {
        case NONUM:
            if (*p >= '0' && *p <= '9')
                s = INT;
            else if (*p == '.')
                s = LEADDOT;
            break;
        case DOTNONUM:
            if (*p != '.' && (*p < '0' || *p > '9'))
                s = NONUM;
            break;
        case INT:
            if (*p == '.')
                s = DOT;
            else if (*p < '0' || *p > '9')
                s = NONUM;
            break;
        case DOT:
        case LEADDOT:
            if (*p >= '0' && *p <= '9')
                s = FRAC;
            else if (*p == '.')
                s = DOTNONUM;
            else
                s = NONUM;
            break;
        case FRAC:
            if (*p == '.')
                s = DOTNONUM;
            else if (*p < '0' || *p > '9')
                s = NONUM;
            break;
        default:;
        }
        switch (s) {
        case DOT:
            r = q;
            break;
        case LEADDOT:
            r = q + 1;
            break;
        case FRAC:
            if (*p > '0')
                r = q + 1;
            break;
        case NONUM:
            if ((t == FRAC || t == DOT) && r != a) {
                q = r--;
                if (*r == '.')  /* was a LEADDOT */
                    *r = '0';
                r = a;
            }
            break;
        default:;
        }
        *q++ = *p++;
        t = s;
    }
    *q = '\0';
    return a;
}

void initversionstring(char **versions)
{
    (void) asprintf(versions,
                    "Compiled with libpng %s; using libpng %s\n"
                    "Compiled with zlib %s; using zlib %s\n"
                    "Compiled with %s version %s\n",
                    PNG_LIBPNG_VER_STRING, png_libpng_ver,
                    ZLIB_VERSION, zlib_version, xpdfString, xpdfVersion);
}

void check_buffer_overflow(int wsize)
{
    int nsize;
    if (wsize > buf_size) {
        nsize = buf_size + buf_size / 5 + 5;
        if (nsize < wsize) {
            nsize = wsize + 5;
        }
        buffer = (unsigned char *) xreallocarray(buffer, char, nsize);
        buf_size = nsize;
    }
}

#define EXTRA_STRING 500

void check_pool_overflow(int wsize)
{
    int nsize;
    if ((wsize - 1) > pool_size) {
        nsize = pool_size + pool_size / 5 + EXTRA_STRING;
        if (nsize < wsize) {
            nsize = wsize + EXTRA_STRING;
        }
        str_pool = (unsigned char *) xreallocarray(str_pool, char, nsize);
        pool_size = nsize;
    }
}

#define max_integer 0x7FFFFFFF

/* the return value is a decimal number with the point |dd| places from the back,
   |scaled_out| is the number of scaled points corresponding to that.
*/

scaled divide_scaled(scaled s, scaled m, integer dd)
{
    register scaled q;
    register scaled r;
    int i;
    int sign = 1;
    if (s < 0) {
        sign = -sign;
        s = -s;
    }
    if (m < 0) {
        sign = -sign;
        m = -m;
    }
    if (m == 0) {
        pdf_error(maketexstring("arithmetic"),
                  maketexstring("divided by zero"));
    } else if (m >= (max_integer / 10)) {
        pdf_error(maketexstring("arithmetic"), maketexstring("number too big"));
    }
    q = s / m;
    r = s % m;
    for (i = 1; i <= (int) dd; i++) {
        q = 10 * q + (10 * r) / m;
        r = (10 * r) % m;
    }
    /* rounding */
    if (2 * r >= m) {
        q++;
        r -= m;
    }
    return sign * q;
}

/* Same function, but using doubles instead of integers (faster) */

scaled divide_scaled_n(double sd, double md, double n)
{
    double dd, di = 0.0;
    dd = sd / md * n;
    if (dd > 0.0)
        di = floor(dd + 0.5);
    else if (dd < 0.0)
        di = -floor((-dd) + 0.5);
    return (scaled) di;
}


/* C print interface */

void tconfusion(char *s)
{
    confusion(maketexstring(s));
}

#ifdef MSVC

#  include <math.h>
double rint(double x)
{
    double c, f, d1, d2;

    c = ceil(x);
    f = floor(x);
    d1 = fabs(c - x);
    d2 = fabs(x - f);
    if (d1 > d2)
        return f;
    else
        return c;
}

#endif
