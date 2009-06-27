/* pdfimage.c
   
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
#include "commands.h"
#include "luatex-api.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";

void out_img(PDF pdf, image * img, scaledpos pos)
{
    float a[6];                 /* transformation matrix */
    float xoff, yoff, tmp;
    pdfstructure *p = pdf->pstruct;
    assert(p != NULL);
    int r;                      /* number of digits after the decimal point */
    int k;
    scaled wd, ht, dp;
    scaledpos tmppos;
    pdffloat cm[6];
    image_dict *idict;
    integer groupref;           /* added from web for 1.40.8 */
    assert(img != 0);
    idict = img_dict(img);
    assert(idict != 0);
    wd = img_width(img);
    ht = img_height(img);
    dp = img_depth(img);
    a[0] = a[3] = 1.0e6;
    a[1] = a[2] = 0;
    if (img_type(idict) == IMG_TYPE_PDF
        || img_type(idict) == IMG_TYPE_PDFSTREAM) {
        a[0] /= img_xsize(idict);
        a[3] /= img_ysize(idict);
        xoff = (float) img_xorig(idict) / img_xsize(idict);
        yoff = (float) img_yorig(idict) / img_ysize(idict);
        r = 6;
    } else {
        /* added from web for 1.40.8 */
        if (img_type(idict) == IMG_TYPE_PNG) {
            groupref = img_group_ref(idict);
            if ((groupref > 0) && (pdf->img_page_group_val == 0))
                pdf->img_page_group_val = groupref;
        }
        /* /added from web */
        a[0] /= one_hundred_bp;
        a[3] = a[0];
        xoff = yoff = 0;
        r = 4;
    }
    if ((img_transform(img) & 7) > 3) { // mirror cases
        a[0] *= -1;
        xoff *= -1;
    }
    switch ((img_transform(img) + img_rotation(idict)) & 3) {
    case 0:                    /* no transform */
        break;
    case 1:                    /* rot. 90 deg. (counterclockwise) */
        a[1] = a[0];
        a[2] = -a[3];
        a[3] = a[0] = 0;
        tmp = yoff;
        yoff = xoff;
        xoff = -tmp;
        break;
    case 2:                    /* rot. 180 deg. (counterclockwise) */
        a[0] *= -1;
        a[3] *= -1;
        xoff *= -1;
        yoff *= -1;
        break;
    case 3:                    /* rot. 270 deg. (counterclockwise) */
        a[1] = -a[0];
        a[2] = a[3];
        a[3] = a[0] = 0;
        tmp = yoff;
        yoff = -xoff;
        xoff = tmp;
        break;
    default:;
    }
    xoff *= wd;
    yoff *= ht + dp;
    a[0] *= wd;
    a[1] *= ht + dp;
    a[2] *= wd;
    a[3] *= ht + dp;
    a[4] = pos.h - xoff;
    a[5] = pos.v - yoff;
    k = img_transform(img) + img_rotation(idict);
    if ((img_transform(img) & 7) > 3)
        k++;
    switch (k & 3) {
    case 0:                    /* no transform */
        break;
    case 1:                    /* rot. 90 deg. (counterclockwise) */
        a[4] += wd;
        break;
    case 2:                    /* rot. 180 deg. */
        a[4] += wd;
        a[5] += ht + dp;
        break;
    case 3:                    /* rot. 270 deg. */
        a[5] += ht + dp;
        break;
    default:;
    }
    /* the following is a kludge, TODO: use pdfpage.c functions */
    setpdffloat(cm[0], round(a[0]), r);
    setpdffloat(cm[1], round(a[1]), r);
    setpdffloat(cm[2], round(a[2]), r);
    setpdffloat(cm[3], round(a[3]), r);
    tmppos.h = round(a[4]);
    tmppos.v = round(a[5]);
    (void) calc_pdfpos(p, tmppos);
    cm[4] = p->cm[4];
    cm[5] = p->cm[5];
    pdf_goto_pagemode(pdf);
    if (pdf->img_page_group_val == 0)
        pdf->img_page_group_val = img_group_ref(idict); /* added from web for 1.40.8 */
    pdf_printf(pdf, "q\n");
    pdf_print_cm(pdf, cm);
    pdf_printf(pdf, "/Im");
    pdf_print_int(pdf, img_index(idict));
    pdf_print_resname_prefix(pdf);
    pdf_printf(pdf, " Do\nQ\n");
    if (img_state(idict) < DICT_OUTIMG)
        img_state(idict) = DICT_OUTIMG;
}

void output_image(PDF pdf, integer idx, scaledpos pos)
{
    pdf_goto_pagemode(pdf);
    out_image(pdf, idx, pos);
    if (pdf_lookup_list(pdf_ximage_list, image_objnum(idx)) == null)
        pdf_append_list(image_objnum(idx), pdf_ximage_list);
}
