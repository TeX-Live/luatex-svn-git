/* epdf.h

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

/* $Id$ */

extern "C" {

#include <kpathsea/c-auto.h>

    extern char *xstrdup(const char *);

/* the following code is extremly ugly but needed for including web2c/config.h */


    typedef const char *const_string;   /* including kpathsea/types.h doesn't work on some systems */

#define KPATHSEA_CONFIG_H       /* avoid including other kpathsea header files */
    /* from web2c/config.h */

#ifdef CONFIG_H                 /* CONFIG_H has been defined by some xpdf */
#  undef CONFIG_H               /* header file */
#endif

#include <web2c/c-auto.h>       /* define SIZEOF_LONG */
#include <web2c/config.h>       /* define type integer */

#define xfree(p)            do { if (p != NULL) free(p); p = NULL; } while (0)

#include "openbsd-compat.h"
#include "image.h"
#include "../utils/avlstuff.h"
#include "../pdf/pdftypes.h"

    extern void pdf_room(PDF, integer);
#define pdf_out(B,A) do { pdf_room(B,1); B->buf[B->ptr++] = A; } while (0)

    extern void unrefPdfDocument(char *);
    extern void *epdf_xref;
    extern integer epdf_lastGroupObjectNum;
    extern integer pool_ptr;
    extern char notdef[];

    extern int is_subsetable(struct fm_entry *);
    extern struct fm_entry *lookup_fontmap(char *);
    extern integer get_fontfile(struct fm_entry *);
    extern integer get_fontname(struct fm_entry *);
    extern integer pdf_new_objnum(PDF);
    extern void read_pdf_info(PDF, image_dict *, integer, integer);
    extern void embed_whole_font(struct fd_entry *);
    extern void epdf_check_mem(void);
    extern void epdf_free(void);
    __attribute__ ((format(printf, 2, 3)))
    extern void pdf_printf(PDF, const char *fmt, ...);
    extern void pdf_puts(PDF, const char *);
    extern void pdf_begin_stream(PDF);
    extern void pdf_end_obj(PDF);
    extern void pdf_end_stream(PDF);

    __attribute__ ((noreturn, format(printf, 1, 2)))
    extern void pdftex_fail(const char *fmt, ...);
    __attribute__ ((format(printf, 1, 2)))
    extern void pdftex_warn(const char *fmt, ...);

    extern void write_epdf(PDF, image_dict *);
    extern void write_additional_epdf_objects(PDF);
    extern void pdf_begin_obj(PDF, integer, bool);

/* epdf.c */
    extern void epdf_mark_glyphs(struct fd_entry *, char *);
    extern struct fd_entry *epdf_create_fontdescriptor(struct fm_entry *, int,
                                                       int);
    extern int get_fd_objnum(struct fd_entry *);
    extern int get_fn_objnum(PDF, struct fd_entry *);

/* write_enc.c */
    extern void epdf_write_enc(PDF, char **, integer);

/* utils.c */
    extern char *convertStringToPDFString(char *in, int len);
    extern char *stripzeros(char *a);

}
