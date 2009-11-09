/* writeimg.h

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

#ifndef WRITEIMG_H
#  define WRITEIMG_H

#  include "../dvi/dvigen.h"    /* for scaled_whd only */
#  include "image.h"
#  include "../pdf/pdfpage.h"

typedef image *img_entry;
img_entry *img_array;

extern integer image_orig_x, image_orig_y;      /* origin of cropped PDF images */

image_dict *new_image_dict(void);
image *new_image(void);
integer img_to_array(image *);
integer read_image(PDF, integer, integer, char *, integer, char *,
                   char *, integer, integer, integer, integer);
void check_pdfstream_dict(image_dict *);
void dumpimagemeta(void);
void free_image_dict(image_dict * p);
void init_image_dict(image_dict *);
void init_image(image *);
void new_img_pdfstream_struct(image_dict *);
void pdf_print_resname_prefix(void);
void read_img(PDF, image_dict *, integer, integer);
scaled_whd scan_alt_rule(void);
void scale_img(image *);
#  define scale_image(a)        scale_img(img_array[a])
void undumpimagemeta(PDF, integer, integer);
void write_img(PDF, image_dict *);
#  define write_image(a, b)     write_img((a), img_dict(img_array[b]));
void pdf_write_image(PDF pdf, integer n);
void write_pdfstream(PDF, image_dict *);

#endif                          /* WRITEIMG_H */
