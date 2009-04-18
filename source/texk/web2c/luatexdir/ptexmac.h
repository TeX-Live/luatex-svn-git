/* ptexmac.h
   
   Copyright 1996-2006 Han The Thanh <thanh@pdftex.org>
   Copyright 2006-2008 Taco Hoekwater <taco@luatex.org>

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

#ifndef PTEXMAC_H
#  define PTEXMAC_H

/* Not all systems define it. */
#  ifndef M_PI
#    define M_PI           3.14159265358979323846
                                                /* pi */
#    define M_PI_2         1.5707963267948966192E0
                                                /*Hex  2^ 0 * 1.921FB54442D18 */
#    define M_PI_4         7.8539816339744830962E-1
                                                /*Hex  2^-1 * 1.921FB54442D18 */
#  endif

#  ifdef WIN32
/* Why relying on gmalloc() ???*/
#    define gmalloc(n) xmalloc(n)
#    define gfree(p) free(p)
#    define inline __inline
#    define srandom(n) srand(n)
#    define random() rand()
#  endif

/* Pascal WEB macros */
#  define maxinteger 0x7FFFFFFF
#  define maxdimen   0x3FFFFFFF

#  define objinfo(n) objtab[n].int0

#  define pdfroom(n) do {                                    \
    if ((unsigned)(n + pdf_ptr) > (unsigned)pdf_buf_size) {  \
        if (pdf_os_mode)                                     \
            zpdf_os_get_os_buf(n);                           \
        else {                                               \
            if ((unsigned)(n) > (unsigned)pdf_buf_size)      \
                pdftex_fail("PDF output buffer overflowed"); \
            else                                             \
                pdf_flush();                                 \
        }                                                    \
    }                                                        \
} while (0)

#  define pdfout(c)  do {   \
    pdfroom(1);             \
    pdf_buf[pdf_ptr++] = c; \
} while (0)

#  define pdfoffset()     (pdf_gone + pdf_ptr)

#  define MAX_CHAR_CODE       255
#  define PRINTF_BUF_SIZE     1024
#  define MAX_CSTRING_LEN     1024 * 1024
#  define MAX_PSTRING_LEN     1024
#  define SMALL_BUF_SIZE      256
#  define SMALL_ARRAY_SIZE    256
#  define FONTNAME_BUF_SIZE   128
                                /* a PDF name can be maximum 127 chars long */

#  define pdftex_debug    tex_printf

extern void check_buffer_overflow(int wsize);
extern void check_pool_overflow(int wsize);

#  define check_buf(size, buf_size)                                 \
  if ((unsigned)(size) > (unsigned)(buf_size))                      \
    pdftex_fail("buffer overflow: %d > %d at file %s, line %d",     \
                (int)(size), (int)(buf_size), __FILE__,  __LINE__ )

#  define append_char_to_buf(c, p, buf, buf_size) do { \
    if (c == 9)                                        \
        c = 32;                                        \
    if (c == 13 || c == EOF)                           \
        c = 10;                                        \
    if (c != ' ' || (p > buf && p[-1] != 32)) {        \
        check_buf(p - buf + 1, (buf_size));            \
        *p++ = c;                                      \
    }                                                  \
} while (0)

#  define append_eol(p, buf, buf_size) do {            \
    check_buf(p - buf + 2, (buf_size));                \
    if (p - buf > 1 && p[-1] != 10)                    \
        *p++ = 10;                                     \
    if (p - buf > 2 && p[-2] == 32) {                  \
        p[-2] = 10;                                    \
        p--;                                           \
    }                                                  \
    *p = 0;                                            \
} while (0)

#  define remove_eol(p, buf) do {                      \
    p = strend(buf) - 1;                               \
    if (*p == 10)                                      \
        *p = 0;                                        \
} while (0)

#  define skip(p, c)   if (*p == c)  p++

#  define alloc_array(T, n, s) do {                    \
    if (T##_array == NULL) {                           \
        T##_limit = (s);                               \
        if ((unsigned)(n) > T##_limit)                 \
            T##_limit = (n);                           \
        T##_array = xtalloc(T##_limit, T##_entry);     \
        T##_ptr = T##_array;                           \
    }                                                  \
    else if ((unsigned)(T##_ptr - T##_array + (n)) > (unsigned)(T##_limit)) { \
        last_ptr_index = T##_ptr - T##_array;          \
        T##_limit *= 2;                                \
        if ((unsigned)(T##_ptr - T##_array + (n)) > (unsigned)(T##_limit)) \
            T##_limit = T##_ptr - T##_array + (n);     \
        xretalloc(T##_array, T##_limit, T##_entry);    \
        T##_ptr = T##_array + last_ptr_index;          \
    }                                                  \
} while (0)

#  define is_cfg_comment(c) \
    (c == 10 || c == '*' || c == '#' || c == ';' || c == '%')

#  define define_array(T)                   \
T##_entry      *T##_ptr, *T##_array = NULL; \
size_t          T##_limit

#  define xfree(p)            do { if (p != NULL) free(p); p = NULL; } while (0)
#  define strend(s)           strchr(s, 0)
#  define xtalloc             XTALLOC
#  define xretalloc           XRETALLOC

#  define ASCENT_CODE         0
#  define CAPHEIGHT_CODE      1
#  define DESCENT_CODE        2
#  define ITALIC_ANGLE_CODE   3
#  define STEMV_CODE          4
#  define XHEIGHT_CODE        5
#  define FONTBBOX1_CODE      6
#  define FONTBBOX2_CODE      7
#  define FONTBBOX3_CODE      8
#  define FONTBBOX4_CODE      9
#  define FONTNAME_CODE       10
#  define GEN_KEY_NUM         (XHEIGHT_CODE + 1)
#  define MAX_KEY_CODE        (FONTBBOX1_CODE + 1)
#  define INT_KEYS_NUM        (FONTBBOX4_CODE + 1)
#  define FONT_KEYS_NUM       (FONTNAME_CODE + 1)

#  define set_cur_file_name(s) \
    cur_file_name = s;         \
    pack_file_name(maketexstring(cur_file_name), get_nullstr(), get_nullstr())

#  define cmp_return(a, b) \
    if ((a) > (b))         \
        return 1;          \
    if ((a) < (b))         \
        return -1

#  define str_prefix(s1, s2)  (strncmp((s1), (s2), strlen(s2)) == 0)

#endif                          /* PTEXMAC_H */
