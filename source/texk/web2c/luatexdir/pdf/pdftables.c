/* pdftables.c
   
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

/* Here is the first block of globals */

integer obj_tab_size = inf_obj_tab_size;        /* allocated size of |obj_tab| array */
obj_entry *obj_tab;
integer head_tab[(head_tab_max + 1)];   /* all zeroes */
integer pages_tail;
integer obj_ptr = 0;            /* user objects counter */
integer sys_obj_ptr = 0;        /* system objects counter, including object streams */
integer pdf_last_pages;         /* pointer to most recently generated pages object */
integer pdf_last_page;          /* pointer to most recently generated page object */
integer pdf_last_stream;        /* pointer to most recently generated stream */
longinteger pdf_stream_length;  /* length of most recently generated stream */
longinteger pdf_stream_length_offset;   /* file offset of the last stream length */
boolean pdf_seek_write_length = false;  /* flag whether to seek back and write \.{/Length} */
integer pdf_last_byte;          /* byte most recently written to PDF file; for \.{endstream} in new line */
integer ff;                     /* for use with |set_ff| */
integer pdf_box_spec_media = 1;
integer pdf_box_spec_crop = 2;
integer pdf_box_spec_bleed = 3;
integer pdf_box_spec_trim = 4;
integer pdf_box_spec_art = 5;

/* create an object with type |t| and identifier |i| */
void pdf_create_obj(integer t, integer i)
{
    integer a, p, q;
    if (sys_obj_ptr == sup_obj_tab_size)
        overflow(maketexstring("indirect objects table size"), obj_tab_size);
    if (sys_obj_ptr == obj_tab_size) {
        a = 0.2 * obj_tab_size;
        if (obj_tab_size < sup_obj_tab_size - a)
            obj_tab_size = obj_tab_size + a;
        else
            obj_tab_size = sup_obj_tab_size;
        obj_tab = xreallocarray(obj_tab, obj_entry, obj_tab_size);
    }
    incr(sys_obj_ptr);
    obj_ptr = sys_obj_ptr;
    obj_info(obj_ptr) = i;
    set_obj_fresh(obj_ptr);
    obj_aux(obj_ptr) = 0;
    avl_put_obj(obj_ptr, t);
    if (t == obj_type_page) {
        p = head_tab[t];
        /* find the right position to insert newly created object */
        if ((p == 0) || (obj_info(p) < i)) {
            obj_link(obj_ptr) = p;
            head_tab[t] = obj_ptr;
        } else {
            while (p != 0) {
                if (obj_info(p) < i)
                    break;
                q = p;
                p = obj_link(p);
            }
            obj_link(q) = obj_ptr;
            obj_link(obj_ptr) = p;
        }
    } else if (t != obj_type_others) {
        obj_link(obj_ptr) = head_tab[t];
        head_tab[t] = obj_ptr;
        if ((t == obj_type_dest) && (i < 0))
            append_dest_name(-obj_info(obj_ptr), obj_ptr);
    }
}

/* create a new object and return its number */
integer pdf_new_objnum(void)
{
    pdf_create_obj(obj_type_others, 0);
    return obj_ptr;
}

/* switch between PDF stream and object stream mode */
void pdf_os_switch(boolean pdf_os)
{
    if (pdf_os && pdf_os_enable) {
        if (!pdf_os_mode) {     /* back up PDF stream variables */
            pdf_op_ptr = pdf_ptr;
            pdf_ptr = pdf_os_ptr;
            pdf_buf = pdf_os_buf;
            pdf_buf_size = pdf_os_buf_size;
            pdf_os_mode = true; /* switch to object stream */
        }
    } else {
        if (pdf_os_mode) {      /* back up object stream variables */
            pdf_os_ptr = pdf_ptr;
            pdf_ptr = pdf_op_ptr;
            pdf_buf = pdf_op_buf;
            pdf_buf_size = pdf_op_buf_size;
            pdf_os_mode = false;        /* switch to PDF stream */
        }
    }
}

/* create new \.{/ObjStm} object if required, and set up cross reference info */
void pdf_os_prepare_obj(integer i, integer pdf_os_level)
{
    pdf_os_switch(((pdf_os_level > 0)
                   && (fixed_pdf_objcompresslevel >= pdf_os_level)));
    if (pdf_os_mode) {
        if (pdf_os_cur_objnum == 0) {
            pdf_os_cur_objnum = pdf_new_objnum();
            decr(obj_ptr);      /* object stream is not accessible to user */
            incr(pdf_os_cntr);  /* only for statistics */
            pdf_os_objidx = 0;
            pdf_ptr = 0;        /* start fresh object stream */
        } else {
            incr(pdf_os_objidx);
        }
        obj_os_idx(i) = pdf_os_objidx;
        obj_offset(i) = pdf_os_cur_objnum;
        pdf_os_objnum[pdf_os_objidx] = i;
        pdf_os_objoff[pdf_os_objidx] = pdf_ptr;
    } else {
        obj_offset(i) = pdf_offset;
        obj_os_idx(i) = -1;     /* mark it as not included in object stream */
    }
}


/* TODO: it is fairly horrible that this uses token memory */

/* appends a pointer with info |i| to the end of linked list with head |p| */
halfword append_ptr(halfword p, integer i)
{
    halfword q;
    q = get_avail();
    fixmem[q].hhrh = 0;         /* link */
    fixmem[q].hhlh = i;         /* info */
    if (p == 0) {
        return q;
    }
    while (fixmem[p].hhrh != 0)
        p = fixmem[p].hhrh;
    fixmem[p].hhrh = q;
    return p;
}

/* looks up for pointer with info |i| in list |p| */
halfword pdf_lookup_list(halfword p, integer i)
{
    while (p != null) {
        if (fixmem[p].hhlh == i)
            return p;
        p = fixmem[p].hhrh;
    }
    return null;
}

void set_rect_dimens(halfword p, halfword parent_box, scaled x, scaled y,
                     scaled wd, scaled ht, scaled dp, scaled margin)
{
    scaledpos ll, ur, pos_ll, pos_ur, tmp;
    ll.h = cur.h;
    if (is_running(dp))
        ll.v = y + depth(parent_box);
    else
        ll.v = cur.v + dp;
    if (is_running(wd))
        ur.h = x + width(parent_box);
    else
        ur.h = cur.h + wd;
    if (is_running(ht))
        ur.v = y - height(parent_box);
    else
        ur.v = cur.v - ht;
    pos_ll = synch_p_with_c(ll);
    pos_ur = synch_p_with_c(ur);
    if (pos_ll.h > pos_ur.h) {
        tmp.h = pos_ll.h;
        pos_ll.h = pos_ur.h;
        pos_ur.h = tmp.h;
    }
    if (pos_ll.v > pos_ur.v) {
        tmp.v = pos_ll.v;
        pos_ll.v = pos_ur.v;
        pos_ur.v = tmp.v;
    }
    if (is_shipping_page && matrixused()) {
        matrixtransformrect(pos_ll.h, pos_ll.v, pos_ur.h, pos_ur.v);
        pos_ll.h = getllx();
        pos_ll.v = getlly();
        pos_ur.h = geturx();
        pos_ur.v = getury();
    }
    pdf_ann_left(p) = pos_ll.h - margin;
    pdf_ann_bottom(p) = pos_ll.v - margin;
    pdf_ann_right(p) = pos_ur.h + margin;
    pdf_ann_top(p) = pos_ur.v + margin;
}

/*
Store some of the pdftex data structures in the format. The idea here is
to ensure that any data structures referenced from pdftex-specific whatsit
nodes are retained. For the sake of simplicity and speed, all the filled parts
of |pdf_mem| and |obj_tab| are retained, in the present implementation. We also
retain three of the linked lists that start from |head_tab|, so that it is
possible to, say, load an image in the \.{INITEX} run and then reference it in a
\.{VIRTEX} run that uses the dumped format.
*/

void dump_pdftex_data(void)
{
    integer k;
    dumpimagemeta();            /* the image information array */
    dump_int(pdf_mem_size);
    dump_int(pdf_mem_ptr);
    for (k = 1; k <= pdf_mem_ptr - 1; k++)
        dump_int(pdf_mem[k]);
    print_ln();
    print_int(pdf_mem_ptr - 1);
    tprint(" words of pdf memory");
    dump_int(obj_tab_size);
    dump_int(obj_ptr);
    dump_int(sys_obj_ptr);
    for (k = 1; k <= sys_obj_ptr; k++) {
        dump_int(obj_info(k));
        dump_int(obj_link(k));
        dump_int(obj_os_idx(k));
        dump_int(obj_aux(k));
    }
    print_ln();
    print_int(sys_obj_ptr);
    tprint(" indirect objects");
    dump_int(pdf_obj_count);
    dump_int(pdf_xform_count);
    dump_int(pdf_ximage_count);
    dump_int(head_tab[obj_type_obj]);
    dump_int(head_tab[obj_type_xform]);
    dump_int(head_tab[obj_type_ximage]);
    dump_int(pdf_last_obj);
    dump_int(pdf_last_xform);
    dump_int(pdf_last_ximage);
}

/*
And restoring the pdftex data structures from the format. The
two function arguments to |undumpimagemeta| have been restored
already in an earlier module.
*/

void undump_pdftex_data(void)
{
    integer k;
    undumpimagemeta(pdf_minor_version, pdf_inclusion_errorlevel);       /* the image information array */
    undump_int(pdf_mem_size);
    pdf_mem = xreallocarray(pdf_mem, integer, pdf_mem_size);
    undump_int(pdf_mem_ptr);
    for (k = 1; k <= pdf_mem_ptr - 1; k++)
        undump_int(pdf_mem[k]);
    undump_int(obj_tab_size);
    undump_int(obj_ptr);
    undump_int(sys_obj_ptr);
    for (k = 1; k <= sys_obj_ptr; k++) {
        undump_int(obj_info(k));
        undump_int(obj_link(k));
        set_obj_offset(k, -1);
        undump_int(obj_os_idx(k));
        undump_int(obj_aux(k));
    }
    undump_int(pdf_obj_count);
    undump_int(pdf_xform_count);
    undump_int(pdf_ximage_count);
    undump_int(head_tab[obj_type_obj]);
    undump_int(head_tab[obj_type_xform]);
    undump_int(head_tab[obj_type_ximage]);
    undump_int(pdf_last_obj);
    undump_int(pdf_last_xform);
    undump_int(pdf_last_ximage);
}
