/* pdfsaverestore.c
   
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

static const char __svn_version[] = "$Id$" "$URL$";

void pdf_out_save(void)
{
    pos = synch_p_with_c(cur);
    checkpdfsave(pos);
    pdf_literal('q', set_origin, false);
}

void pdf_out_restore(void)
{
    pos = synch_p_with_c(cur);
    checkpdfrestore(pos);
    pdf_literal('Q', set_origin, false);
}
