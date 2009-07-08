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

#  include <zlib.h>

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

typedef struct {
    long m;                     /* mantissa (significand) */
    int e;                      /* exponent * -1 */
} pdffloat;

typedef struct {
    pdffloat h;
    pdffloat v;
} pdfpos;

#  define scaled integer

typedef struct scaledpos_ {
    scaled h;
    scaled v;
} scaledpos;

typedef struct posstructure_ {
    scaledpos pos;              /* position on the page */
    int dir;                    /* direction of stuff to be put onto the page */
} posstructure;

typedef enum { PMODE_NONE, PMODE_PAGE, PMODE_TEXT, PMODE_CHARARRAY,
    PMODE_CHAR
} pos_mode;

typedef struct pdf_object_list_ {
    int info;
    struct pdf_object_list_ *link;
} pdf_object_list;

typedef enum { WMODE_H, WMODE_V } writing_mode; /* []TJ runs horizontal or vertical */

typedef struct {
    pdfpos pdf;                 /* pos. on page (PDF page raster) */
    pdfpos pdf_bt_pos;          /* pos. at begin of BT-ET group (PDF page raster) */
    pdfpos pdf_tj_pos;          /* pos. at begin of TJ array (PDF page raster) */
    pdffloat cw;                /* pos. within [(..)..]TJ array (glyph raster);
                                   cw.e = fractional digits in /Widths array */
    pdffloat tj_delta;          /* rel. movement in [(..)..]TJ array (glyph raster) */
    pdffloat fs;                /* font size in PDF units */
    pdffloat hz;                /* HZ expansion factor */
    pdffloat ext;               /* ExtendFont factor */
    pdffloat cm[6];             /* cm array */
    pdffloat tm[6];             /* Tm array */
    double k1;                  /* conv. factor from TeX sp to PDF page raster */
    double k2;                  /* conv. factor from PDF page raster to TJ array raster */
    int f_cur;                  /* TeX font number */
    int f_pdf;                  /* /F* font number, of unexpanded base font! */
    writing_mode wmode;         /* PDF writing mode WMode (horizontal/vertical) */
    pos_mode mode;              /* current positioning mode */
    int ishex;                  /* Whether the current char string is <> or () */
} pdfstructure;

typedef struct obj_entry_ {
    integer int0;
    integer int1;
    off_t int2;
    integer int3;
    integer int4;
} obj_entry;

/* types of objects */
typedef enum {
    obj_type_others = 0,        /* objects which are not linked in any list */
    obj_type_page = 1,          /* index of linked list of Page objects */
    obj_type_font = 2,          /* index of linked list of Fonts objects */
    obj_type_outline = 3,       /* index of linked list of outline objects */
    obj_type_dest = 4,          /* index of linked list of destination objects */
    obj_type_obj = 5,           /* index of linked list of raw objects */
    obj_type_xform = 6,         /* index of linked list of XObject forms */
    obj_type_ximage = 7,        /* index of linked list of XObject image */
    obj_type_thread = 8,        /* index of linked list of num article threads */
    /* |obj_type_thead| is the highest entry in |head_tab|, but there are a few
       more linked lists that are handy: */
    obj_type_link = 9,          /* index of linked list of link objects */
    obj_type_bead = 10,         /* index of linked list of thread bead objects */
    obj_type_annot = 11,        /* index of linked list of annotation objects */
} pdf_obj_type;

typedef struct pdf_output_file_ {
    FILE *file;                 /* the PDF output file handle */
    char *file_name;            /* the PDF output file name */
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
    int compress_level;         /* level for zlib object stream compression */
    int objcompresslevel;       /* fixed level for activating PDF object streams */
    char *job_id_string;        /* the full job string */

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

    char *printf_buf;           /* a scratch buffer for |pdf_printf| */

    time_t start_time;          /* when this job started */
    char *start_time_str;       /* minimum size for time_str is 24: "D:YYYYmmddHHMMSS+HH'MM'" */

    /* define fb_ptr, fb_array & fb_limit */
    char *fb_array;
    char *fb_ptr;
    size_t fb_limit;

    char *zipbuf;
    z_stream c_stream;          /* compression stream */
    int zip_write_state;        /* which state of compression we are in */

    int pk_scale_factor;        /* this is just a preprocessed value that depends on 
                                   |pk_resolution| and |decimal_digits| */

    int img_page_group_val;     /* page group information pointer from included pdf or png images */
    char *resname_prefix;       /* global prefix of resources name */

    int mem_size;               /* allocated size of |mem| array */
    int *mem;
    int mem_ptr;

    pdfstructure *pstruct;      /* utity structure keeping position status in PDF page stream */
    posstructure *posstruct;    /* structure for positioning within page */

    int obj_tab_size;           /* allocated size of |obj_tab| array */
    obj_entry *obj_tab;
    int head_tab[9];            /* heads of the object lists in |obj_tab| */
    struct avl_table *obj_tree[9];      /* this is useful for finding the objects back */

    int pages_tail;
    int obj_ptr;                /* user objects counter */
    int sys_obj_ptr;            /* system objects counter, including object streams */
    int last_pages;             /* pointer to most recently generated pages object */
    int last_page;              /* pointer to most recently generated page object */
    int last_stream;            /* pointer to most recently generated stream */
    off_t stream_length;        /* length of most recently generated stream */
    off_t stream_length_offset; /* file offset of the last stream length */
    int seek_write_length;      /* flag whether to seek back and write \.{/Length} */
    int last_byte;              /* byte most recently written to PDF file; for \.{endstream} in new line */

    integer obj_count;
    integer xform_count;
    integer ximage_count;
    pdf_object_list *font_list; /* the pdf fonts */
    pdf_object_list *obj_list;  /* the pdf raw objects */
    pdf_object_list *xform_list;        /* the pdf forms */
    pdf_object_list *ximage_list;       /* the pdf images */
    pdf_object_list *dest_list; /* the pdf destinations */
    pdf_object_list *link_list; /* the pdf links */
    pdf_object_list *annot_list;        /* the pdf annotations */
    pdf_object_list *bead_list; /* the thread beads */

    integer image_procset; /* collection of image types used in current page/form */
    int text_procset; /* mask of used ProcSet's in the current page/form */

} pdf_output_file;

typedef pdf_output_file *PDF;

#endif
