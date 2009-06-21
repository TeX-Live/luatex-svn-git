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


integer open_subentries(halfword p)
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
halfword scan_action (void) 
{
  integer p;
  p = new_node(action_node,0);
  if (scan_keyword("user"))
    set_pdf_action_type(p, pdf_action_user);
  else if (scan_keyword("goto"))
    set_pdf_action_type(p, pdf_action_goto);
  else if (scan_keyword("thread"))
    set_pdf_action_type(p, pdf_action_thread);
  else
    pdf_error(maketexstring("ext1"), 
	      maketexstring("action type missing"));

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
      pdf_error(maketexstring("ext1"), maketexstring("only GoTo action can be used with `page'"));
    set_pdf_action_type(p, pdf_action_page);
    scan_int();
    if (cur_val <= 0)
      pdf_error(maketexstring("ext1"), maketexstring("page number must be positive"));
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
		maketexstring("`goto' option cannot be used with both `file' and `num'"));
    scan_int();
    if (cur_val <= 0)
      pdf_error(maketexstring("ext1"), maketexstring("num identifier must be positive"));
    set_pdf_action_named_id(p, 0);
    set_pdf_action_id(p, cur_val);
  } else {
    pdf_error(maketexstring("ext1"), maketexstring("identifier type missing"));
  }  
  if (scan_keyword("newwindow")) {
    set_pdf_action_new_window(p, 1);
    /* Scan an optional space */
    get_x_token(); if (cur_cmd!=spacer_cmd) back_input();
  } else if (scan_keyword("nonewwindow")) {
    set_pdf_action_new_window(p, 2);
    /* Scan an optional space */
    get_x_token(); if (cur_cmd!=spacer_cmd) back_input();
  } else {
    set_pdf_action_new_window(p, 0);
  }
  if ((pdf_action_new_window(p) > 0) &&
        (((pdf_action_type(p) != pdf_action_goto) &&
          (pdf_action_type(p) != pdf_action_page)) ||
         (pdf_action_file(p) == null)))
    pdf_error(maketexstring("ext1"),
	      maketexstring("`newwindow'/`nonewwindow' must be used with `goto' and `file' option"));
  return p;
}
 
/* return number of outline entries in the same level with |p| */
integer outline_list_count( pointer p)
{ 
  integer k = 1;
  while (obj_outline_prev(p) != 0) {
    incr(k);
    p = obj_outline_prev(p);
  }
  return k;
}

void scan_pdfoutline(void)
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
    pdf_new_obj(obj_type_others, 0, 1);
    j = obj_ptr;
    write_action(p);
    pdf_end_obj();
    delete_action_ref(p);
    pdf_create_obj(obj_type_outline, 0);
    k = obj_ptr;
    set_obj_outline_ptr(k, pdf_get_mem(pdfmem_outline_size));
    set_obj_outline_action_objnum(k, j);
    set_obj_outline_count(k, i);
    pdf_new_obj(obj_type_others, 0, 1);
    {
        char *s = tokenlist_to_cstring(q, true, NULL);
        pdf_print_str_ln(s);
        xfree(s);
    }
    delete_token_ref(q);
    pdf_end_obj();
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
