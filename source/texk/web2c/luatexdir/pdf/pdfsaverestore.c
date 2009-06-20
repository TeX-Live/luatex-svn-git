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

static const char __svn_version[] =
    "$Id$ "
    "$URL$";

#include "ptexlib.h"

pos_entry *pos_stack = 0;       /* the stack */
int pos_stack_size = 0;         /* initially empty */
int pos_stack_used = 0;         /* used entries */

void checkpdfsave(scaledpos pos)
{
    pos_entry *new_stack;

    if (pos_stack_used >= pos_stack_size) {
        pos_stack_size += STACK_INCREMENT;
        new_stack = xtalloc(pos_stack_size, pos_entry);
        memcpy((void *) new_stack, (void *) pos_stack,
               pos_stack_used * sizeof(pos_entry));
        xfree(pos_stack);
        pos_stack = new_stack;
    }
    pos_stack[pos_stack_used].pos.h = pos.h;
    pos_stack[pos_stack_used].pos.v = pos.v;
    if (page_mode) {
        pos_stack[pos_stack_used].matrix_stack = matrix_stack_used;
    }
    pos_stack_used++;
}

void checkpdfrestore(scaledpos pos)
{
    scaledpos diff;
    if (pos_stack_used == 0) {
        pdftex_warn("%s", "\\pdfrestore: missing \\pdfsave");
        return;
    }
    pos_stack_used--;
    diff.h = pos.h - pos_stack[pos_stack_used].pos.h;
    diff.v = pos.v - pos_stack[pos_stack_used].pos.v;
    if (diff.h != 0 || diff.v != 0) {
        pdftex_warn("Misplaced \\pdfrestore by (%dsp, %dsp)", (int) diff.h,
                    (int) diff.v);
    }
    if (page_mode) {
        matrix_stack_used = pos_stack[pos_stack_used].matrix_stack;
    }
}


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
