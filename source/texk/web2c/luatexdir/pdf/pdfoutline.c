/* pdfoutline.c
   
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
#include "luatex-api.h"
#include "commands.h"

static const char __svn_version[] =
    "$Id$"
    "$URL$";

/* Data structure of outlines; it's not able to write out outline entries
before all outline entries are defined, so memory allocated for outline
entries can't not be deallocated and will stay in memory. For this reason we
will store data of outline entries in |pdf_mem| instead of |mem|
*/

#  define pdfmem_outline_size      8    /* size of memory in |pdf_mem| which |obj_outline_ptr| points to */

#  define obj_outline_count         obj_info    /* count of all opened children */
#  define obj_outline_ptr           obj_aux     /* pointer to |pdf_mem| */
#  define obj_outline_title(A)      pdf_mem[obj_outline_ptr(A)]
#  define obj_outline_parent(A)     pdf_mem[obj_outline_ptr(A) + 1]
#  define obj_outline_prev(A)       pdf_mem[obj_outline_ptr(A) + 2]
#  define obj_outline_next(A)       pdf_mem[obj_outline_ptr(A) + 3]
#  define obj_outline_first(A)      pdf_mem[obj_outline_ptr(A) + 4]
#  define obj_outline_last(A)       pdf_mem[obj_outline_ptr(A) + 5]
#  define obj_outline_action_objnum(A)  pdf_mem[obj_outline_ptr(A) + 6] /* object number of action */
#  define obj_outline_attr(A)       pdf_mem[obj_outline_ptr(A) + 7]

#  define set_obj_outline_ptr(A,B) obj_outline_ptr(A)=B
#  define set_obj_outline_action_objnum(A,B) obj_outline_action_objnum(A)=B
#  define set_obj_outline_count(A,B) obj_outline_count(A)=B
#  define set_obj_outline_title(A,B) obj_outline_title(A)=B
#  define set_obj_outline_prev(A,B) obj_outline_prev(A)=B
#  define set_obj_outline_next(A,B) obj_outline_next(A)=B
#  define set_obj_outline_first(A,B) obj_outline_first(A)=B
#  define set_obj_outline_last(A,B) obj_outline_last(A)=B
#  define set_obj_outline_parent(A,B) obj_outline_parent(A)=B
#  define set_obj_outline_attr(A,B) obj_outline_attr(A)=B


static integer pdf_first_outline = 0;
static integer pdf_last_outline = 0;
static integer pdf_parent_outline = 0;

static integer open_subentries(halfword p)
{
    integer k, c;
    integer l, r;
    k = 0;
    if (obj_outline_first(p) != 0) {
        l = obj_outline_first(p);
        do {
            incr(k);
            c = open_subentries(l);
            if (obj_outline_count(l) > 0)
                k = k + c;
            obj_outline_parent(l) = p;
            r = obj_outline_next(l);
            if (r == 0)
                obj_outline_last(p) = l;
            l = r;
        } while (l != 0);
    }
    if (obj_outline_count(p) > 0)
        obj_outline_count(p) = k;
    else
        obj_outline_count(p) = -k;
    return k;
}

/* read an action specification */
halfword scan_action(void)
{
    integer p;
    p = new_node(action_node, 0);
    if (scan_keyword("user"))
        set_pdf_action_type(p, pdf_action_user);
    else if (scan_keyword("goto"))
        set_pdf_action_type(p, pdf_action_goto);
    else if (scan_keyword("thread"))
        set_pdf_action_type(p, pdf_action_thread);
    else
        pdf_error(maketexstring("ext1"), maketexstring("action type missing"));

    if (pdf_action_type(p) == pdf_action_user) {
        scan_pdf_ext_toks();
        set_pdf_action_tokens(p, def_ref);
        return p;
    }
    if (scan_keyword("file")) {
        scan_pdf_ext_toks();
        set_pdf_action_file(p, def_ref);
    }
    if (scan_keyword("page")) {
        if (pdf_action_type(p) != pdf_action_goto)
            pdf_error(maketexstring("ext1"),
                      maketexstring
                      ("only GoTo action can be used with `page'"));
        set_pdf_action_type(p, pdf_action_page);
        scan_int();
        if (cur_val <= 0)
            pdf_error(maketexstring("ext1"),
                      maketexstring("page number must be positive"));
        set_pdf_action_id(p, cur_val);
        set_pdf_action_named_id(p, 0);
        scan_pdf_ext_toks();
        set_pdf_action_tokens(p, def_ref);
    } else if (scan_keyword("name")) {
        scan_pdf_ext_toks();
        set_pdf_action_named_id(p, 1);
        set_pdf_action_id(p, def_ref);
    } else if (scan_keyword("num")) {
        if ((pdf_action_type(p) == pdf_action_goto) &&
            (pdf_action_file(p) != null))
            pdf_error(maketexstring("ext1"),
                      maketexstring
                      ("`goto' option cannot be used with both `file' and `num'"));
        scan_int();
        if (cur_val <= 0)
            pdf_error(maketexstring("ext1"),
                      maketexstring("num identifier must be positive"));
        set_pdf_action_named_id(p, 0);
        set_pdf_action_id(p, cur_val);
    } else {
        pdf_error(maketexstring("ext1"),
                  maketexstring("identifier type missing"));
    }
    if (scan_keyword("newwindow")) {
        set_pdf_action_new_window(p, 1);
        /* Scan an optional space */
        get_x_token();
        if (cur_cmd != spacer_cmd)
            back_input();
    } else if (scan_keyword("nonewwindow")) {
        set_pdf_action_new_window(p, 2);
        /* Scan an optional space */
        get_x_token();
        if (cur_cmd != spacer_cmd)
            back_input();
    } else {
        set_pdf_action_new_window(p, 0);
    }
    if ((pdf_action_new_window(p) > 0) &&
        (((pdf_action_type(p) != pdf_action_goto) &&
          (pdf_action_type(p) != pdf_action_page)) ||
         (pdf_action_file(p) == null)))
        pdf_error(maketexstring("ext1"),
                  maketexstring
                  ("`newwindow'/`nonewwindow' must be used with `goto' and `file' option"));
    return p;
}

/* return number of outline entries in the same level with |p| */
static integer outline_list_count(pointer p)
{
    integer k = 1;
    while (obj_outline_prev(p) != 0) {
        incr(k);
        p = obj_outline_prev(p);
    }
    return k;
}

void scan_pdfoutline(PDF pdf)
{
    halfword p, q, r;
    integer i, j, k;
    if (scan_keyword("attr")) {
        scan_pdf_ext_toks();
        r = def_ref;
    } else {
        r = 0;
    }
    p = scan_action();
    if (scan_keyword("count")) {
        scan_int();
        i = cur_val;
    } else {
        i = 0;
    }
    scan_pdf_ext_toks();
    q = def_ref;
    pdf_new_obj(pdf, obj_type_others, 0, 1);
    j = obj_ptr;
    write_action(pdf, p);
    pdf_end_obj(pdf);
    delete_action_ref(p);
    pdf_create_obj(obj_type_outline, 0);
    k = obj_ptr;
    set_obj_outline_ptr(k, pdf_get_mem(pdfmem_outline_size));
    set_obj_outline_action_objnum(k, j);
    set_obj_outline_count(k, i);
    pdf_new_obj(pdf, obj_type_others, 0, 1);
    {
        char *s = tokenlist_to_cstring(q, true, NULL);
        pdf_print_str_ln(pdf, s);
        xfree(s);
    }
    delete_token_ref(q);
    pdf_end_obj(pdf);
    set_obj_outline_title(k, obj_ptr);
    set_obj_outline_prev(k, 0);
    set_obj_outline_next(k, 0);
    set_obj_outline_first(k, 0);
    set_obj_outline_last(k, 0);
    set_obj_outline_parent(k, pdf_parent_outline);
    set_obj_outline_attr(k, r);
    if (pdf_first_outline == 0)
        pdf_first_outline = k;
    if (pdf_last_outline == 0) {
        if (pdf_parent_outline != 0)
            set_obj_outline_first(pdf_parent_outline, k);
    } else {
        set_obj_outline_next(pdf_last_outline, k);
        set_obj_outline_prev(k, pdf_last_outline);
    }
    pdf_last_outline = k;
    if (obj_outline_count(k) != 0) {
        pdf_parent_outline = k;
        pdf_last_outline = 0;
    } else if ((pdf_parent_outline != 0) &&
               (outline_list_count(k) ==
                abs(obj_outline_count(pdf_parent_outline)))) {
        j = pdf_last_outline;
        do {
            set_obj_outline_last(pdf_parent_outline, j);
            j = pdf_parent_outline;
            pdf_parent_outline = obj_outline_parent(pdf_parent_outline);
        } while (!((pdf_parent_outline == 0) ||
                   (outline_list_count(j) <
                    abs(obj_outline_count(pdf_parent_outline)))));
        if (pdf_parent_outline == 0)
            pdf_last_outline = pdf_first_outline;
        else
            pdf_last_outline = obj_outline_first(pdf_parent_outline);
        while (obj_outline_next(pdf_last_outline) != 0)
            pdf_last_outline = obj_outline_next(pdf_last_outline);
    }
}

/* In the end we must flush PDF objects that cannot be written out
   immediately after shipping out pages. */

integer print_outlines (PDF pdf) {
    integer k,l,a;
    integer outlines;
    if (pdf_first_outline != 0) {
        pdf_new_dict(pdf,obj_type_others, 0, 1);
        outlines = obj_ptr;
        l = pdf_first_outline;
        k = 0;
        do {
            incr(k);
            a = open_subentries(l);
            if (obj_outline_count(l) > 0)
                k = k + a;
            set_obj_outline_parent(l, obj_ptr);
            l = obj_outline_next(l);
        } while (l != 0);
        pdf_printf(pdf,"/Type /Outlines\n");
        pdf_indirect_ln(pdf,"First", pdf_first_outline);
        pdf_indirect_ln(pdf,"Last", pdf_last_outline);
        pdf_int_entry_ln(pdf,"Count", k);
        pdf_end_dict(pdf);
        /* Output PDF outline entries */
        
        k = head_tab[obj_type_outline];
        while (k != 0) {
            if (obj_outline_parent(k) == pdf_parent_outline) {
                if (obj_outline_prev(k) == 0)
                    pdf_first_outline = k;
                if (obj_outline_next(k) == 0)
                    pdf_last_outline = k;
            }
            pdf_begin_dict(pdf,k, 1);
            pdf_indirect_ln(pdf,"Title", obj_outline_title(k));
            pdf_indirect_ln(pdf,"A", obj_outline_action_objnum(k));
            if (obj_outline_parent(k) != 0)
                pdf_indirect_ln(pdf,"Parent", obj_outline_parent(k));
            if (obj_outline_prev(k) != 0)
                pdf_indirect_ln(pdf,"Prev", obj_outline_prev(k));
            if (obj_outline_next(k) != 0)
                pdf_indirect_ln(pdf,"Next", obj_outline_next(k));
            if (obj_outline_first(k) != 0)
                pdf_indirect_ln(pdf,"First", obj_outline_first(k));
            if (obj_outline_last(k) != 0)
                pdf_indirect_ln(pdf,"Last", obj_outline_last(k));
            if (obj_outline_count(k) != 0)
                pdf_int_entry_ln(pdf,"Count", obj_outline_count(k));
            if (obj_outline_attr(k) != 0) {
                pdf_print_toks_ln(pdf,obj_outline_attr(k));
                delete_token_ref(obj_outline_attr(k));
                set_obj_outline_attr(k, null);
            }
            pdf_end_dict(pdf);
            k = obj_link(k);
        }

    } else {
        outlines = 0;
    }
    return outlines;
}
