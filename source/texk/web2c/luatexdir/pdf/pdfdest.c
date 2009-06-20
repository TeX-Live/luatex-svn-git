/* pdfdest.c
   
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

static const char __svn_version[] =
    "$Id$"
    "$URL$";

#define info(A) fixmem[(A)].hhlh
#define pdf_dest_margin          dimen_par(param_pdf_dest_margin_code)

integer dest_names_size = inf_dest_names_size;  /* maximum number of names in name tree of PDF output file */

/*
Here we implement subroutines for work with objects and related things.
Some of them are used in former parts too, so we need to declare them
forward.
*/


void append_dest_name(str_number s, integer n)
{
    integer a;
    if (pdf_dest_names_ptr == sup_dest_names_size)
        overflow(maketexstring("number of destination names (dest_names_size)"),
                 dest_names_size);
    if (pdf_dest_names_ptr == dest_names_size) {
        a = 0.2 * dest_names_size;
        if (dest_names_size < sup_dest_names_size - a)
            dest_names_size = dest_names_size + a;
        else
            dest_names_size = sup_dest_names_size;
        dest_names =
            xreallocarray(dest_names, dest_name_entry, dest_names_size);
    }
    dest_names[pdf_dest_names_ptr].objname = s;
    dest_names[pdf_dest_names_ptr].objnum = n;
    incr(pdf_dest_names_ptr);
}

/*
When a destination is created we need to check whether another destination
with the same identifier already exists and give a warning if needed.
*/

void warn_dest_dup(integer id, small_number byname, str_number s1,
                   str_number s2)
{
    pdf_warning(s1, maketexstring("destination with the same identifier ("),
                false, false);
    if (byname > 0) {
        tprint("name");
        print_mark(id);
    } else {
        tprint("num");
        print_int(id);
    }
    tprint(") ");
    print(s2);
    print_ln();
    show_context();
}


void do_dest(halfword p, halfword parent_box, scaled x, scaled y)
{
    scaledpos tmp1, tmp2;
    integer k;
    if (!is_shipping_page)
        pdf_error(maketexstring("ext4"),
                  maketexstring("destinations cannot be inside an XForm"));
    if (doing_leaders)
        return;
    k = get_obj(obj_type_dest, pdf_dest_id(p), pdf_dest_named_id(p));
    if (obj_dest_ptr(k) != null) {
        warn_dest_dup(pdf_dest_id(p), pdf_dest_named_id(p),
                      maketexstring("ext4"),
                      maketexstring
                      ("has been already used, duplicate ignored"));
        return;
    }
    obj_dest_ptr(k) = p;
    pdf_append_list(k, pdf_dest_list);
    switch (pdf_dest_type(p)) {
    case pdf_dest_xyz:
        if (matrixused()) {
            set_rect_dimens(p, parent_box, x, y,
                            pdf_width(p), pdf_height(p), pdf_depth(p),
                            pdf_dest_margin);
        } else {
            tmp1.h = cur.h;
            tmp1.v = cur.v;
            tmp2 = synch_p_with_c(tmp1);
            pdf_ann_left(p) = tmp2.h;
            pdf_ann_top(p) = tmp2.v;
        }
        break;
    case pdf_dest_fith:
    case pdf_dest_fitbh:
        if (matrixused()) {
            set_rect_dimens(p, parent_box, x, y,
                            pdf_width(p), pdf_height(p), pdf_depth(p),
                            pdf_dest_margin);
        } else {
            tmp1.h = cur.h;
            tmp1.v = cur.v;
            tmp2 = synch_p_with_c(tmp1);
            pdf_ann_top(p) = tmp2.v;
        }
        break;
    case pdf_dest_fitv:
    case pdf_dest_fitbv:
        if (matrixused()) {
            set_rect_dimens(p, parent_box, x, y,
                            pdf_width(p), pdf_height(p), pdf_depth(p),
                            pdf_dest_margin);
        } else {
            tmp1.h = cur.h;
            tmp1.v = cur.v;
            tmp2 = synch_p_with_c(tmp1);
            pdf_ann_left(p) = tmp2.h;
        }
        break;
    case pdf_dest_fit:
    case pdf_dest_fitb:
        break;
    case pdf_dest_fitr:
        set_rect_dimens(p, parent_box, x, y,
                        pdf_width(p), pdf_height(p), pdf_depth(p),
                        pdf_dest_margin);
    }
}

void write_out_pdf_mark_destinations(void)
{
    halfword k;
    if (pdf_dest_list != null) {
        k = pdf_dest_list;
        while (k != null) {
            if (is_obj_written(fixmem[k].hhlh)) {
                pdf_error(maketexstring("ext5"),
                          maketexstring
                          ("destination has been already written (this shouldn't happen)"));
            } else {
                integer i;
                i = obj_dest_ptr(info(k));
                if (pdf_dest_named_id(i) > 0) {
                    pdf_begin_dict(info(k), 1);
                    pdf_printf("/D ");
                } else {
                    pdf_begin_obj(info(k), 1);
                }
                pdf_out('[');
                pdf_print_int(pdf_last_page);
                pdf_printf(" 0 R ");
                switch (pdf_dest_type(i)) {
                case pdf_dest_xyz:
                    pdf_printf("/XYZ ");
                    pdf_print_mag_bp(pdf_ann_left(i));
                    pdf_out(' ');
                    pdf_print_mag_bp(pdf_ann_top(i));
                    pdf_out(' ');
                    if (pdf_dest_xyz_zoom(i) == null) {
                        pdf_printf("null");
                    } else {
                        pdf_print_int(pdf_dest_xyz_zoom(i) / 1000);
                        pdf_out('.');
                        pdf_print_int((pdf_dest_xyz_zoom(i) % 1000));
                    }
                    break;
                case pdf_dest_fit:
                    pdf_printf("/Fit");
                    break;
                case pdf_dest_fith:
                    pdf_printf("/FitH ");
                    pdf_print_mag_bp(pdf_ann_top(i));
                    break;
                case pdf_dest_fitv:
                    pdf_printf("/FitV ");
                    pdf_print_mag_bp(pdf_ann_left(i));
                    break;
                case pdf_dest_fitb:
                    pdf_printf("/FitB");
                    break;
                case pdf_dest_fitbh:
                    pdf_printf("/FitBH ");
                    pdf_print_mag_bp(pdf_ann_top(i));
                    break;
                case pdf_dest_fitbv:
                    pdf_printf("/FitBV ");
                    pdf_print_mag_bp(pdf_ann_left(i));
                    break;
                case pdf_dest_fitr:
                    pdf_printf("/FitR ");
                    pdf_print_rect_spec(i);
                    break;
                default:
                    pdf_error(maketexstring("ext5"),
                              maketexstring("unknown dest type"));
                    break;
                }
                pdf_printf("]\n");
                if (pdf_dest_named_id(i) > 0)
                    pdf_end_dict();
                else
                    pdf_end_obj();
            }
            k = fixmem[k].hhrh;
        }
    }
}
