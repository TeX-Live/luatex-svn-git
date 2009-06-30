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
#include "luatex-api.h"         /* for tokenlist_to_cstring */

static const char __svn_version[] =
    "$Id$"
    "$URL$";

integer pdf_box_spec_media = 1;
integer pdf_box_spec_crop = 2;
integer pdf_box_spec_bleed = 3;
integer pdf_box_spec_trim = 4;
integer pdf_box_spec_art = 5;

/**********************************************************************/
/* One AVL tree for each obj_type 0...pdf_objtype_max */

typedef struct oentry_ {
    integer int0;
    integer objptr;
} oentry;

/* AVL sort oentry into avl_table[] */

static int compare_info(const void *pa, const void *pb, void *param)
{
    integer a, b;
    int as, ae, bs, be, al, bl;
    (void) param;
    a = ((const oentry *) pa)->int0;
    b = ((const oentry *) pb)->int0;
    if (a == 0 && b == 0)
        return 0;               /* this happens a lot */
    if (a < 0 && b < 0) {       /* string comparison */
        a = -a;
        b = -b;
        if (a >= 2097152 && b >= 2097152) {
            a -= 2097152;
            b -= 2097152;
            as = str_start[a];
            ae = str_start[a + 1];      /* start of next string in pool */
            bs = str_start[b];
            be = str_start[b + 1];
            al = ae - as;
            bl = be - bs;
            if (al < bl)        /* compare first by string length */
                return -1;
            if (al > bl)
                return 1;
            for (; as < ae; as++, bs++) {
                if (str_pool[as] < str_pool[bs])
                    return -1;
                if (str_pool[as] > str_pool[bs])
                    return 1;
            }
        } else {
            if (a < b)
                return -1;
            if (a > b)
                return 1;
        }
    } else {                    /* integer comparison */
        if (a < b)
            return -1;
        if (a > b)
            return 1;
    }
    return 0;
}

void avl_put_obj(PDF pdf, integer objptr, integer t)
{
    static void **pp;
    static oentry *oe;

    if (pdf->obj_tree[t] == NULL) {
        pdf->obj_tree[t] = avl_create(compare_info, NULL, &avl_xallocator);
        if (pdf->obj_tree[t] == NULL)
            pdftex_fail("avlstuff.c: avl_create() pdf->obj_tree failed");
    }
    oe = xtalloc(1, oentry);
    oe->int0 = obj_info(pdf, objptr);
    oe->objptr = objptr;        /* allows to relocate obj_tab */
    pp = avl_probe(pdf->obj_tree[t], oe);
    if (pp == NULL)
        pdftex_fail("avlstuff.c: avl_probe() out of memory in insertion");
}

/* replacement for linear search pascal function "find_obj()" */

integer avl_find_obj(PDF pdf, integer t, integer i, integer byname)
{
    static oentry *p;
    static oentry tmp;

    if (byname > 0)
        tmp.int0 = -i;
    else
        tmp.int0 = i;
    if (pdf->obj_tree[t] == NULL)
        return 0;
    p = (oentry *) avl_find(pdf->obj_tree[t], &tmp);
    if (p == NULL)
        return 0;
    return p->objptr;
}

/**********************************************************************/


/* create an object with type |t| and identifier |i| */
void pdf_create_obj(PDF pdf, integer t, integer i)
{
    integer a, p, q;
    if (pdf->sys_obj_ptr == sup_obj_tab_size)
        overflow("indirect objects table size", pdf->obj_tab_size);
    if (pdf->sys_obj_ptr == pdf->obj_tab_size) {
        a = 0.2 * pdf->obj_tab_size;
        if (pdf->obj_tab_size < sup_obj_tab_size - a)
            pdf->obj_tab_size = pdf->obj_tab_size + a;
        else
            pdf->obj_tab_size = sup_obj_tab_size;
        pdf->obj_tab =
            xreallocarray(pdf->obj_tab, obj_entry, pdf->obj_tab_size);
    }
    incr(pdf->sys_obj_ptr);
    pdf->obj_ptr = pdf->sys_obj_ptr;
    obj_info(pdf, pdf->obj_ptr) = i;
    set_obj_fresh(pdf, pdf->obj_ptr);
    obj_aux(pdf, pdf->obj_ptr) = 0;
    avl_put_obj(pdf, pdf->obj_ptr, t);
    if (t == obj_type_page) {
        p = pdf->head_tab[t];
        /* find the right position to insert newly created object */
        if ((p == 0) || (obj_info(pdf, p) < i)) {
            obj_link(pdf, pdf->obj_ptr) = p;
            pdf->head_tab[t] = pdf->obj_ptr;
        } else {
            while (p != 0) {
                if (obj_info(pdf, p) < i)
                    break;
                q = p;
                p = obj_link(pdf, p);
            }
            obj_link(pdf, q) = pdf->obj_ptr;
            obj_link(pdf, pdf->obj_ptr) = p;
        }
    } else if (t != obj_type_others) {
        obj_link(pdf, pdf->obj_ptr) = pdf->head_tab[t];
        pdf->head_tab[t] = pdf->obj_ptr;
        if ((t == obj_type_dest) && (i < 0))
            append_dest_name(-obj_info(pdf, pdf->obj_ptr), pdf->obj_ptr);
    }
}

/* The following function finds object with identifier |i| and type |t|.
   |i < 0| indicates that |-i| should be treated as a string number. If no
   such object exists then it will be created. This function is used mainly to
   find destination for link annotations and outlines; however it is also used
   in |pdf_ship_out| (to check whether a Page object already exists) so we need
   to declare it together with subroutines needed in |pdf_hlist_out| and
   |pdf_vlist_out|.
*/

integer find_obj(PDF pdf, integer t, integer i, boolean byname)
{
    return avl_find_obj(pdf, t, i, byname);
}


integer get_obj(PDF pdf, integer t, integer i, boolean byname)
{
    integer r;
    str_number s;
    if (byname > 0) {
        s = tokens_to_string(i);
        r = find_obj(pdf, t, s, true);
    } else {
        s = 0;
        r = find_obj(pdf, t, i, false);
    }
    if (r == 0) {
        if (byname > 0) {
            pdf_create_obj(pdf, t, -s);
            s = 0;
        } else {
            pdf_create_obj(pdf, t, i);
        }
        r = pdf->obj_ptr;
        if (t == obj_type_dest)
            set_obj_dest_ptr(pdf, r, null);
    }
    if (s != 0)
        flush_str(s);
    return r;
}

/* create a new object and return its number */
integer pdf_new_objnum(PDF pdf)
{
    pdf_create_obj(pdf, obj_type_others, 0);
    return pdf->obj_ptr;
}

/* TODO: it is fairly horrible that this uses token memory */

/* appends a pointer with info |i| to the end of linked list with head |p| */
halfword append_ptr(halfword p, integer i)
{
    halfword q;
    q = get_avail();
    token_link(q) = 0;          /* link */
    token_info(q) = i;          /* info */
    if (p == 0) {
        return q;
    }
    while (token_link(p) != 0)
        p = token_link(p);
    token_link(p) = q;
    return p;
}

/* looks up for pointer with info |i| in list |p| */
halfword pdf_lookup_list(halfword p, integer i)
{
    while (p != null) {
        if (token_info(p) == i)
            return p;
        p = token_link(p);
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


void initialize_pdf_output(PDF pdf)
{
    if ((pdf_minor_version < 0) || (pdf_minor_version > 9)) {
        char *hlp[] = { "The pdfminorversion must be between 0 and 9.",
            "I changed this to 4.", NULL
        };
        char msg[256];
        (void) snprintf(msg, 255, "LuaTeX error (illegal pdfminorversion %d)",
                        (int) pdf_minor_version);
        tex_error(msg, hlp);
        pdf_minor_version = 4;
    }
    pdf->minor_version = pdf_minor_version;
    pdf->decimal_digits = fix_int(pdf_decimal_digits, 0, 4);
    pdf->gamma = fix_int(pdf_gamma, 0, 1000000);
    pdf->image_gamma = fix_int(pdf_image_gamma, 0, 1000000);
    pdf->image_hicolor = fix_int(pdf_image_hicolor, 0, 1);
    pdf->image_apply_gamma = fix_int(pdf_image_apply_gamma, 0, 1);
    pdf->objcompresslevel = fix_int(pdf_objcompresslevel, 0, 3);
    pdf->draftmode = fix_int(pdf_draftmode, 0, 1);
    pdf->inclusion_copy_font = fix_int(pdf_inclusion_copy_font, 0, 1);
    pdf->replace_font = fix_int(pdf_replace_font, 0, 1);
    pdf->pk_resolution = fix_int(pdf_pk_resolution, 72, 8000);
    if ((pdf->minor_version >= 5) && (pdf->objcompresslevel > 0)) {
        pdf->os_enable = true;
    } else {
        if (pdf->objcompresslevel > 0) {
            pdf_warning("Object streams",
                        "\\pdfobjcompresslevel > 0 requires \\pdfminorversion > 4. Object streams disabled now.",
                        true, true);
            pdf->objcompresslevel = 0;
        }
        pdf->os_enable = false;
    }
    if (pdf->pk_resolution == 0)        /* if not set from format file or by user */
        pdf->pk_resolution = pk_dpi;    /* take it from \.{texmf.cnf} */
    pdf->pk_scale_factor =
        divide_scaled(72, pdf->pk_resolution, 5 + pdf->decimal_digits);
    if (!callback_defined(read_pk_file_callback)) {
        if (pdf_pk_mode != null) {
            char *s = tokenlist_to_cstring(pdf_pk_mode, true, NULL);
            kpseinitprog("PDFTEX", pdf->pk_resolution, s, nil);
            xfree(s);
        } else {
            kpseinitprog("PDFTEX", pdf->pk_resolution, nil, nil);
        }
        if (!kpsevarvalue("MKTEXPK"))
            kpsesetprogramenabled(kpsepkformat, 1, kpsesrccmdline);
    }
    set_job_id(pdf, int_par(param_year_code),
               int_par(param_month_code),
               int_par(param_day_code), int_par(param_time_code));

    if ((pdf_unique_resname > 0) && (pdf->resname_prefix == NULL))
        pdf->resname_prefix = get_resname_prefix(pdf);
    pdf_page_init(pdf);
}

/* temp here */

void pdfshipoutbegin(boolean shipping_page)
{
    pos_stack_used = 0;         /* start with empty stack */

    page_mode = shipping_page;
    if (shipping_page) {
        colorstackpagestart();
    }
}

void pdfshipoutend(boolean shipping_page)
{
    if (pos_stack_used > 0) {
        pdftex_fail("%u unmatched \\pdfsave after %s shipout",
                    (unsigned int) pos_stack_used,
                    ((shipping_page) ? "page" : "form"));
    }
}

void libpdffinish(void)
{
    fb_free();
    /* xfree(job_id_string); */
    fm_free();
    t1_free();
    enc_free();
    epdf_free();
    ttf_free();
    sfd_free();
    glyph_unicode_free();
    zip_free();
}



/*
Store some of the pdftex data structures in the format. The idea here is
to ensure that any data structures referenced from pdftex-specific whatsit
nodes are retained. For the sake of simplicity and speed, all the filled parts
of |pdf->mem| and |obj_tab| are retained, in the present implementation. We also
retain three of the linked lists that start from |head_tab|, so that it is
possible to, say, load an image in the \.{INITEX} run and then reference it in a
\.{VIRTEX} run that uses the dumped format.
*/

void dump_pdftex_data(PDF pdf)
{
    integer k, x;
    dumpimagemeta();            /* the image information array */
    dump_int(pdf->mem_size);
    dump_int(pdf->mem_ptr);
    for (k = 1; k <= pdf->mem_ptr - 1; k++) {
        x = pdf->mem[k];
        dump_int(x);
    }
    print_ln();
    print_int(pdf->mem_ptr - 1);
    tprint(" words of pdf memory");
    x = pdf->obj_tab_size;
    dump_int(x);
    x = pdf->obj_ptr;
    dump_int(x);
    x = pdf->sys_obj_ptr;
    dump_int(x);
    for (k = 1; k <= pdf->sys_obj_ptr; k++) {
        x = obj_info(pdf, k);
        dump_int(x);
        x = obj_link(pdf, k);
        dump_int(x);
        x = obj_os_idx(pdf, k);
        dump_int(x);
        x = obj_aux(pdf, k);
        dump_int(x);
    }
    print_ln();
    print_int(pdf->sys_obj_ptr);
    tprint(" indirect objects");
    dump_int(pdf_obj_count);
    dump_int(pdf_xform_count);
    dump_int(pdf_ximage_count);
    x = pdf->head_tab[obj_type_obj];
    dump_int(x);
    x = pdf->head_tab[obj_type_xform];
    dump_int(x);
    x = pdf->head_tab[obj_type_ximage];
    dump_int(x);
    dump_int(pdf_last_obj);
    dump_int(pdf_last_xform);
    dump_int(pdf_last_ximage);
}

/*
And restoring the pdftex data structures from the format. The
two function arguments to |undumpimagemeta| have been restored
already in an earlier module.
*/

void undump_pdftex_data(PDF pdf)
{
    integer k, x;
    undumpimagemeta(pdf, pdf_minor_version, pdf_inclusion_errorlevel);  /* the image information array */
    undump_int(pdf->mem_size);
    pdf->mem = xreallocarray(pdf->mem, int, pdf->mem_size);
    undump_int(pdf->mem_ptr);
    for (k = 1; k <= pdf->mem_ptr - 1; k++) {
        undump_int(x);
        pdf->mem[k] = (int) x;
    }
    undump_int(x);
    pdf->obj_tab_size = x;
    undump_int(x);
    pdf->obj_ptr = x;
    undump_int(x);
    pdf->sys_obj_ptr = x;
    for (k = 1; k <= pdf->sys_obj_ptr; k++) {
        undump_int(x);
        obj_info(pdf, k) = x;
        undump_int(x);
        obj_link(pdf, k) = x;
        set_obj_offset(pdf, k, -1);
        undump_int(x);
        obj_os_idx(pdf, k) = x;
        undump_int(x);
        obj_aux(pdf, k) = x;
    }
    undump_int(pdf_obj_count);
    undump_int(pdf_xform_count);
    undump_int(pdf_ximage_count);
    undump_int(x);
    pdf->head_tab[obj_type_obj] = x;
    undump_int(x);
    pdf->head_tab[obj_type_xform] = x;
    undump_int(x);
    pdf->head_tab[obj_type_ximage] = x;
    undump_int(pdf_last_obj);
    undump_int(pdf_last_xform);
    undump_int(pdf_last_ximage);
}
