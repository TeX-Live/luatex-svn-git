/* luatexfont.h General font functions
   
   Copyright 2008 Taco Hoekwater <taco@luatex.org>

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

/* $Id: texfont.h 1350 2008-06-30 15:00:53Z taco $ */

#ifndef LUATEXFONT_H
#  define LUATEXFONT_H 1

#  include "ptexlib.h"
#  include "sfnt.h"

/* texfont.c */
extern charinfo *copy_charinfo(charinfo * ci);

/* tounicode.c */
extern integer write_cid_tounicode(fo_entry * fo, internalfontnumber f);

/* vfpacket.c */
extern void replace_packet_fonts(internal_font_number f, integer * old_fontid,
                                 integer * new_fontid, int count);
extern integer *packet_local_fonts(internal_font_number f, integer * num);

/* writeccf.c */
extern void writetype1w(fd_entry * fd);

/* writetype0.c */
extern void writetype0(fd_entry * fd);

/* writetype2.c */
extern void writetype2(fd_entry * fd);
extern unsigned long cidtogid_obj;
extern pdf_obj *pdf_new_stream(void);
extern void pdf_add_stream(pdf_obj * stream, unsigned char *buf, long len);
extern void pdf_release_obj(pdf_obj * stream);

/* writet3.c */
extern FILE *t3_file;

#endif
