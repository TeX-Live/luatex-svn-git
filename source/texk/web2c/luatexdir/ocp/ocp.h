/* ocp.h
   
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

/* $Id $ */

#ifndef OCP_H

#  define OCP_H 1

typedef integer internal_ocp_number;
typedef integer ocp_index;

extern int **ocp_tables;

extern integer ocp_maxint;

extern internal_ocp_number ocp_ptr;     /* largest internal ocp number in use */

/* smallest internal ocp number; must not be less than |min_quarterword| */
#  define ocp_base 0
#  define number_ocps 32768

#  define null_ocp ocp_base

extern void new_ocp(small_number a);

#  define ocp_trace_level loc_par(param_ocp_trace_level_code)

extern void allocate_ocp_table(int ocp_number, int ocp_size);

extern void dump_ocp_info(void);
extern void undump_ocp_info(void);
extern void scan_ocp_ident(void);


#endif
