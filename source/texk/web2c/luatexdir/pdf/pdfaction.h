/* pdfaction.h

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

#ifndef PDFACTION_H
#  define PDFACTION_H

#  define set_pdf_action_type(A,B) pdf_action_type(A)=B
#  define set_pdf_action_tokens(A,B) pdf_action_tokens(A)=B
#  define set_pdf_action_file(A,B) pdf_action_file(A)=B
#  define set_pdf_action_id(A,B) pdf_action_id(A)=B
#  define set_pdf_action_named_id(A,B) pdf_action_named_id(A)=B
#  define set_pdf_action_new_window(A,B) pdf_action_new_window(A)=B

extern void write_action(halfword p);

#endif
