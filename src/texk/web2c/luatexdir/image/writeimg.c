/*
Copyright (c) 1996-2002, 2005 Han The Thanh, <thanh@pdftex.org>

This file is part of pdfTeX.
pdfTeX is free software;
you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

pdfTeX is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pdfTeX; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

$Id$
*/

#include <assert.h>
#include "ptexlib.h"
#include <kpathsea/c-auto.h>
#include <kpathsea/c-memstr.h>

#include "image.h"

#include <../lua51/lua.h>
#include <../lua51/lauxlib.h>

/**********************************************************************/

#ifdef DEBUG
static void stackDump(lua_State * L, char *s)
{
    int i;
    int top = lua_gettop(L);
    printf("\n=== stackDump <%s>: ", s);
    for (i = 1; i <= top; i++) {        /* repeat for each level */
        int t = lua_type(L, i);
        printf("%d: ", i);
        switch (t) {
        case LUA_TSTRING:      /* strings */
            printf("`%s'", lua_tostring(L, i));
            break;
        case LUA_TBOOLEAN:     /* booleans */
            printf(lua_toboolean(L, i) ? "true" : "false");
            break;
        case LUA_TNUMBER:      /* numbers */
            printf("%g", lua_tonumber(L, i));
            break;
        default:               /* other values */
            printf("%s", lua_typename(L, t));
            break;
        }
        printf("  ");           /* put a separator */
    }
    printf("\n");
}
#endif

/**********************************************************************/
/*
  Patch ImageTypeDetection 2003/02/08 by Heiko Oberdiek.

  Function "readimage" performs some basic initializations.
  Then it looks at the file extension to determine the
  image type and calls specific code/functions.
    The main disadvantage is that standard file extensions
  have to be used, otherwise pdfTeX is not able to detect
  the correct image type.

  The patch now looks at the file header first regardless of
  the file extension. This is implemented in function
  "check_type_by_header". If this check fails, the traditional
  test of standard file extension is tried, done in function
  "check_type_by_extension".

  Magic headers:

  * "PNG (Portable Network Graphics) Specification", Version 1.2
    (http://www.libpng.org/pub/png):

  |   3.1. PNG file signature
  |
  |      The first eight bytes of a PNG file always contain the following
  |      (decimal) values:
  |
  |         137 80 78 71 13 10 26 10

  Translation to C: "\x89PNG\r\n\x1A\n"

  * "JPEG File Interchange Format", Version 1.02:

  | o you can identify a JFIF file by looking for the following
  |   sequence: X'FF', SOI X'FF', APP0, <2 bytes to be skipped>,
  |   "JFIF", X'00'.

  Function "check_type_by_header" only looks at the first two bytes:
    "\xFF\xD8"

  * ISO/IEC JTC 1/SC 29/WG 1
    (ITU-T SG8)
    Coding of Still Pictures
    Title: 14492 FCD
    Source: JBIG Committee
    Project: JTC 1.29.10
    Status: Final Committee Draft

  | D.4.1, ID string
  |
  | This is an 8-byte sequence containing 0x97 0x4A 0x42 0x32 0x0D 0x0A
  | 0x1A 0x0A.

  * "PDF Reference", third edition:
    * The first line should contain "%PDF-1.0" until "%PDF-1.4"
      (section 3.4.1 "File Header").
    * The "implementation notes" say:

    | 3.4.1,  File Header
    |   12. Acrobat viewers require only that the header appear
    |       somewhere within the first 1024 bytes of the file.
    |   13. Acrobat viewers will also accept a header of the form
    |           %!PS-Adobe-N.n PDF-M.m

    The check in function "check_type_by_header" only implements
    the first issue. The implementation notes are not considered.
    Therefore files with garbage at start of file must have the
    standard extension.

    Functions "check_type_by_header" and "check_type_by_extension":
    img_type(img) is set to IMAGE_TYPE_NONE by new_image_dict().
    Both functions try to detect a type and set img_type(img).
    Thus a value other than IMAGE_TYPE_NONE indicates that a
    type has been found.
*/

#define HEADER_JPG "\xFF\xD8"
#define HEADER_PNG "\x89PNG\r\n\x1A\n"
#define HEADER_JBIG2 "\x97\x4A\x42\x32\x0D\x0A\x1A\x0A"
#define HEADER_PDF "%PDF-1."
#define MAX_HEADER (sizeof(HEADER_PNG)-1)

static void check_type_by_header(image_dict * idict)
{
    int i;
    FILE *file = NULL;
    char header[MAX_HEADER];

    assert(idict != NULL);
    if (img_type(idict) != IMAGE_TYPE_NONE)     /* nothing to do */
        return;
    /* read the header */
    file = xfopen(img_filepath(idict), FOPEN_RBIN_MODE);
    for (i = 0; (unsigned) i < MAX_HEADER; i++) {
        header[i] = xgetc(file);
        if (feof(file))
            pdftex_fail("reading image file failed");
    }
    xfclose(file, img_filepath(idict));
    /* tests */
    if (strncmp(header, HEADER_JPG, sizeof(HEADER_JPG) - 1) == 0)
        img_type(idict) = IMAGE_TYPE_JPG;
    else if (strncmp(header, HEADER_PNG, sizeof(HEADER_PNG) - 1) == 0)
        img_type(idict) = IMAGE_TYPE_PNG;
    else if (strncmp(header, HEADER_JBIG2, sizeof(HEADER_JBIG2) - 1) == 0)
        img_type(idict) = IMAGE_TYPE_JBIG2;
    else if (strncmp(header, HEADER_PDF, sizeof(HEADER_PDF) - 1) == 0)
        img_type(idict) = IMAGE_TYPE_PDF;
}

static void check_type_by_extension(image_dict * idict)
{
    char *image_suffix;

    assert(idict != NULL);
    if (img_type(idict) != IMAGE_TYPE_NONE)     /* nothing to do */
        return;
    /* tests */
    if ((image_suffix = strrchr(img_filename(idict), '.')) == 0)
        img_type(idict) = IMAGE_TYPE_NONE;
    else if (strcasecmp(image_suffix, ".png") == 0)
        img_type(idict) = IMAGE_TYPE_PNG;
    else if (strcasecmp(image_suffix, ".jpg") == 0 ||
             strcasecmp(image_suffix, ".jpeg") == 0)
        img_type(idict) = IMAGE_TYPE_JPG;
    else if (strcasecmp(image_suffix, ".jbig2") == 0 ||
             strcasecmp(image_suffix, ".jb2") == 0)
        img_type(idict) = IMAGE_TYPE_JBIG2;
    else if (strcasecmp(image_suffix, ".pdf") == 0)
        img_type(idict) = IMAGE_TYPE_PDF;
}

/**********************************************************************/

pdf_img_struct *new_pdf_img_struct()
{
    pdf_img_struct *p;
    p = xtalloc(1, pdf_img_struct);
    p->width = 0;
    p->height = 0;
    p->orig_x = 0;
    p->orig_y = 0;
    p->page_box = 0;
    p->page_name = NULL;
    p->doc = NULL;
    return p;
}

void init_image_size(image * p)
{
    assert(p != NULL);
    set_wd_running(p);
    set_ht_running(p);
    set_dp_running(p);
    img_flags(p) = 0;
    img_unset_refered(p);       /* wd/ht/dp may be modified */
    img_unset_scaled(p);
}

void init_image(image * p)
{
    assert(p != NULL);
    img_dict(p) = NULL;
    img_dictref(p) = LUA_NOREF;
    init_image_size(p);
}

image *new_image()
{
    image *p = xtalloc(1, image);
    init_image(p);
    return p;
}

void init_image_dict(image_dict * p)
{
    assert(p != NULL);
    img_objnum(p) = 0;
    img_index(p) = 0;
    img_xsize(p) = 0;
    img_ysize(p) = 0;
    img_xres(p) = 0;
    img_yres(p) = 0;
    img_colorspace_obj(p) = 0;
    img_pagenum(p) = 1;
    img_totalpages(p) = 0;
    img_pagename(p) = NULL;
    img_filename(p) = NULL;
    img_filepath(p) = NULL;
    img_attr(p) = NULL;
    img_file(p) = NULL;
    img_type(p) = IMAGE_TYPE_NONE;
    img_color(p) = 0;
    img_colordepth(p) = 0;
    img_pagebox(p) = PDF_BOX_SPEC_MEDIA;
    img_state(p) = DICT_NEW;
    img_pdf_ptr(p) = NULL;      /* union */
}

image_dict *new_image_dict()
{
    image_dict *p = xtalloc(1, image_dict);
    init_image_dict(p);
    return p;
}

void clear_dict_strings(image_dict * p)
{
    if (img_filename(p) != NULL)
        xfree(img_filename(p));
    img_filename(p) = NULL;
    if (img_filepath(p) != NULL)
        xfree(img_filepath(p));
    img_filepath(p) = NULL;
    if (img_attr(p) != NULL)
        xfree(img_attr(p));
    img_attr(p) = NULL;
    if (img_pagename(p) != NULL)
        xfree(img_pagename(p));
    img_pagename(p) = NULL;
}

void free_image_dict(image_dict * p)
{
    clear_dict_strings(p);
    if (img_type(p) == IMAGE_TYPE_PDF)
        epdf_delete(p);
    assert(img_file(p) == NULL);
    xfree(p);
}

/**********************************************************************/

void pdf_print_resname_prefix()
{
    if (pdf_resname_prefix != 0)
        pdf_printf(makecstring(pdf_resname_prefix));
}

void read_img(image_dict * idict, integer pdf_minor_version,
              integer pdf_inclusion_errorlevel)
{
    char *filepath;
    int callback_id;
    assert(idict != NULL);
    callback_id = callback_defined(find_image_file_callback);
    if (img_filepath(idict) == NULL) {
        if (callback_id > 0
            && run_callback(callback_id, "S->S", img_filename(idict),
                            &filepath)) {
            if (filepath && (strlen(filepath) > 0))
                img_filepath(idict) = strdup(filepath);
        } else
            img_filepath(idict) =
                kpse_find_file(img_filename(idict), kpse_tex_format, true);
    }
    if (img_filepath(idict) == NULL)
        pdftex_fail("cannot find image file");
    /* type checks */
    check_type_by_header(idict);
    check_type_by_extension(idict);
    /* read image */
    switch (img_type(idict)) {
    case IMAGE_TYPE_PDF:
        read_pdf_info(idict, pdf_minor_version, pdf_inclusion_errorlevel);
        break;
    case IMAGE_TYPE_PNG:
        read_png_info(idict, true);
        break;
    case IMAGE_TYPE_JPG:
        read_jpg_info(idict, true);
        break;
    case IMAGE_TYPE_JBIG2:
        if (pdf_minor_version < 4) {
            pdftex_fail
                ("JBIG2 images only possible with at least PDF 1.4; you are generating PDF 1.%i",
                 (int) pdf_minor_version);
        }
        read_jbig2_info(idict);
        break;
    default:
        pdftex_fail("internal error: unknown image type");
    }
    cur_file_name = NULL;
    if (img_state(idict) < DICT_FILESCANNED)
        img_state(idict) = DICT_FILESCANNED;
}

void scale_img(image * img)
{
    integer x, y, xr, yr;       /* size and resolution of image */
    scaled w, h;                /* indeed size corresponds to image resolution */
    integer default_res;
    assert(img != NULL);
    image_dict *idict = img_dict(img);
    assert(idict != NULL);
    x = img_xsize(idict);       /* dimensions, resolutions from image file */
    y = img_ysize(idict);
    xr = img_xres(idict);
    yr = img_yres(idict);
    if (xr > 65535 || yr > 65535) {
        xr = 0;
        yr = 0;
        pdftex_warn("ext1", "too large image resolution ignored", true, true);
    }
    if (x <= 0 || y <= 0 || xr < 0 || yr < 0)
        pdftex_fail("ext1", "invalid image dimensions");
    if (img_type(idict) == IMAGE_TYPE_PDF) {
        w = x;
        h = y;
    } else {
        default_res = fix_int(get_pdf_image_resolution(), 0, 65535);
        if (default_res > 0 && (xr == 0 || yr == 0)) {
            xr = default_res;
            yr = default_res;
        }
        if (is_wd_running(img) && is_ht_running(img)) {
            if (xr > 0 && yr > 0) {
                w = ext_xn_over_d(one_hundred_inch, x, 100 * xr);
                h = ext_xn_over_d(one_hundred_inch, y, 100 * yr);
            } else {
                w = ext_xn_over_d(one_hundred_inch, x, 7200);
                h = ext_xn_over_d(one_hundred_inch, y, 7200);
            }
        }
    }
    if (is_wd_running(img) && is_ht_running(img) && is_dp_running(img)) {
        img_width(img) = w;
        img_height(img) = h;
        img_depth(img) = 0;
    } else if (is_wd_running(img)) {
        /* image depth or height is explicitly specified */
        if (is_ht_running(img)) {
            /* image depth is explicitly specified */
            img_width(img) = ext_xn_over_d(h, x, y);
            img_height(img) = h - img_depth(img);
        } else if (is_dp_running(img)) {
            /* image height is explicitly specified */
            img_width(img) = ext_xn_over_d(img_height(img), x, y);
            img_depth(img) = 0;
        } else {
            /* both image depth and height are explicitly specified */
            img_width(img) =
                ext_xn_over_d(img_height(img) + img_depth(img), x, y);
        }
    } else {
        /* image width is explicitly specified */
        if (is_ht_running(img) && is_dp_running(img)) {
            /* both image depth and height are not specified */
            img_height(img) = ext_xn_over_d(img_width(img), y, x);
            img_depth(img) = 0;
        }
        /* image depth is explicitly specified */
        else if (is_ht_running(img)) {
            img_height(img) =
                ext_xn_over_d(img_width(img), y, x) - img_depth(img);
        }
        /* image height is explicitly specified */
        else if (is_dp_running(img)) {
            img_depth(img) = 0;
        }
        /* else both image depth and height are explicitly specified */
    }
    img_set_scaled(img);
}

void out_img(image * img, scaled hpos, scaled vpos)
{
    assert(img != 0);
    image_dict *idict = img_dict(img);
    assert(idict != 0);
    integer wd = img_width(img);
    integer ht = img_height(img);
    integer dp = img_depth(img);
    pdf_end_text();
    pdf_printf("q\n");
    if (img_type(idict) == IMAGE_TYPE_PDF) {
        pdf_print_real((integer) (wd * 1.0e6 / img_xsize(idict)), 6);
        pdf_printf(" 0 0 ");
        pdf_print_real((integer) ((ht + dp) * 1.0e6 / img_ysize(idict)), 6);
        pdfout(' ');
        pdf_print_bp(hpos -
                     (integer) (wd * (float) bp2int(img_pdf_orig_x(idict)) /
                                img_xsize(idict)));
        pdfout(' ');
        pdf_print_bp(vpos -
                     (integer) (ht * (float) bp2int(img_pdf_orig_y(idict)) /
                                img_ysize(idict)));
    } else {
        pdf_print_real((integer) (wd * 1.0e6 / one_hundred_bp), 4);
        pdf_printf(" 0 0 ");
        pdf_print_real((integer) ((ht + dp) * 1.0e6 / one_hundred_bp), 4);
        pdfout(' ');
        pdf_print_bp(hpos);
        pdfout(' ');
        pdf_print_bp(vpos);
    }
    pdf_printf(" cm\n/Im");
    pdf_print_int(img_index(idict));
    pdf_print_resname_prefix();
    pdf_printf(" Do\nQ\n");
    if (img_state(idict) < DICT_OUTIMG)
        img_state(idict) = DICT_OUTIMG;
}

void write_img(image_dict * idict)
{
    assert(idict != NULL);
    if (img_state(idict) < DICT_WRITTEN) {
        if (tracefilenames)
            tex_printf(" <%s", img_filepath(idict));
        switch (img_type(idict)) {
        case IMAGE_TYPE_PNG:
            write_png(idict);
            break;
        case IMAGE_TYPE_JPG:
            write_jpg(idict);
            break;
        case IMAGE_TYPE_JBIG2:
            write_jbig2(idict);
            break;
        case IMAGE_TYPE_PDF:
            write_epdf(idict);
            break;
        default:
            pdftex_fail("internal error: unknown image type");
        }
        if (tracefilenames)
            tex_printf(">");
    }
    if (img_state(idict) < DICT_WRITTEN)
        img_state(idict) = DICT_WRITTEN;
}

/**********************************************************************/

typedef image *img_entry;
/* define img_ptr, img_array, & img_limit */
define_array(img);              /* array of pointers to image structures */

integer img_to_array(image * img)
{
    assert(img != NULL);
    alloc_array(img, 1, SMALL_BUF_SIZE);
    *img_ptr = img;
    return img_ptr++ - img_array;       /* now img is read-only */
}

/**********************************************************************/
/* stuff to be accessible from TeX */

integer read_image(integer objnum, integer index, strnumber filename,
                   integer page_num,
                   strnumber attr,
                   strnumber page_name,
                   integer colorspace_obj, integer page_box,
                   integer pdf_minor_version, integer pdf_inclusion_errorlevel)
{
    integer ref;
    char *dest = NULL;
    image *a = new_image();
    ref = img_to_array(a);
    image_dict *idict = img_dict(a) = new_image_dict();
    assert(idict != NULL);
    img_objnum(idict) = objnum;
    img_index(idict) = index;
    /* img_xsize, img_ysize, img_xres, img_yres set by read_img() */
    img_colorspace_obj(idict) = colorspace_obj;
    img_pagenum(idict) = page_num;
    /* img_totalpages set by read_img() */
    if (page_name != 0)
        img_pagename(idict) = xstrdup(makecstring(page_name));
    cur_file_name = makecfilename(filename);
    assert(cur_file_name != NULL);
    img_filename(idict) = xstrdup(cur_file_name);
    if (attr != 0)
        img_attr(idict) = xstrdup(makecstring(attr));
    img_pagebox(idict) = page_box;
    read_img(idict, pdf_minor_version, pdf_inclusion_errorlevel);
    img_unset_scaled(a);
    img_set_refered(a);
    return ref;
}

void set_image_dimensions(integer ref, integer wd, integer ht, integer dp)
{
    image *a = img_array[ref];
    img_width(a) = wd;
    img_height(a) = ht;
    img_depth(a) = dp;
}

void set_image_index(integer ref, integer index)
{
    image *a = img_array[ref];
    image_dict *idict = img_dict(a);
    assert(idict != NULL);
    img_index(idict) = index;
}

void scale_image(integer ref)
{
    image *a = img_array[ref];
    scale_img(a);
}

void out_image(integer ref, scaled hpos, scaled vpos)
{
    image *a = img_array[ref];
    out_img(a, hpos, vpos);
}

void write_image(integer ref)
{
    image *a = img_array[ref];
    write_img(img_dict(a));
}

integer image_color(integer ref)
{
    image *a = img_array[ref];
    return img_color(img_dict(a));
}

integer image_xsize(integer ref)
{
    image *a = img_array[ref];
    return img_xsize(img_dict(a));
}

integer image_ysize(integer ref)
{
    image *a = img_array[ref];
    return img_ysize(img_dict(a));
}

integer image_xres(integer ref)
{
    image *a = img_array[ref];
    return img_xres(img_dict(a));
}

integer image_yres(integer ref)
{
    image *a = img_array[ref];
    return img_yres(img_dict(a));
}

boolean is_pdf_image(integer ref)
{
    image *a = img_array[ref];
    return img_type(img_dict(a)) == IMAGE_TYPE_PDF;
}

integer image_pages(integer ref)
{
    image *a = img_array[ref];
    return img_totalpages(img_dict(a));
}

integer image_colordepth(integer ref)
{
    image *a = img_array[ref];
    return img_colordepth(img_dict(a));
}

integer epdf_orig_x(integer ref)
{
    image *a = img_array[ref];
    return bp2int(img_pdf_orig_x(img_dict(a)));
}

integer epdf_orig_y(integer ref)
{
    image *a = img_array[ref];
    return bp2int(img_pdf_orig_y(img_dict(a)));
}

integer image_objnum(integer ref)
{
    image *a = img_array[ref];
    return img_objnum(img_dict(a));
}

integer image_index(integer ref)
{
    image *a = img_array[ref];
    return img_index(img_dict(a));
}

integer image_width(integer ref)
{
    image *a = img_array[ref];
    return img_width(a);
}

integer image_height(integer ref)
{
    image *a = img_array[ref];
    return img_height(a);
}

integer image_depth(integer ref)
{
    image *a = img_array[ref];
    return img_depth(a);
}

void update_image_procset(integer ref)
{
    image *a = img_array[ref];
    pdf_image_procset |= img_color(img_dict(a));
}

/**********************************************************************/

boolean check_image_b(integer procset)
{
    return procset & IMAGE_COLOR_B;
}

boolean check_image_c(integer procset)
{
    return procset & IMAGE_COLOR_C;
}

boolean check_image_i(integer procset)
{
    return procset & IMAGE_COLOR_I;
}
