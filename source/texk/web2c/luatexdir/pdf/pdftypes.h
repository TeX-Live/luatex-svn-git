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

/* 
 *  this stucture holds everything that is needed for the actual pdf generation 
 */

typedef struct pdf_output_file_ {
    FILE *file;  /* the PDF output file */
    int gamma;
    int image_gamma;
    int image_hicolor; /* boolean */
    int image_apply_gamma;
    int draftmode;
    int pk_resolution;
    int decimal_digits;
    int gen_tounicode;
    int inclusion_copy_font;
    int replace_font;
    int minor_version;        /* fixed minor part of the PDF version */
    int minor_version_set;    /* flag if the PDF version has been set */
    int objcompresslevel;     /* fixed level for activating PDF object streams */
} pdf_output_file;

typedef pdf_output_file *PDF;

#endif
