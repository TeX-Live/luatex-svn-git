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

/* WEB macros */

#define info(A) fixmem[(A)].hhlh

#define billion 1000000000.0
#define vet_glue(A) do { glue_temp=A;                   \
    if (glue_temp>billion) glue_temp=billion;           \
    else if (glue_temp<-billion) glue_temp=-billion;    \
  } while (0)
#define float_round round
#define float_cast (real)
#define set_to_zero(A) { A.h = 0; A.v = 0; }

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

/* begin a PDF object */
void pdf_begin_obj(integer i, integer pdf_os_level)
{
    check_pdfminorversion();
    pdf_os_prepare_obj(i, pdf_os_level);
    if (!pdf_os_mode) {
        pdf_printf("%d 0 obj\n", (int) i);
    } else if (pdf_compress_level == 0) {
        pdf_printf("%% %d 0 obj\n", (int) i);   /* debugging help */
    }
}

/* begin a new PDF object */
void pdf_new_obj(integer t, integer i, integer pdf_os)
{
    pdf_create_obj(t, i);
    pdf_begin_obj(obj_ptr, pdf_os);
}


/* end a PDF object */
void pdf_end_obj(void)
{
    if (pdf_os_mode) {
        if (pdf_os_objidx == pdf_os_max_objs - 1)
            pdf_os_write_objstream();
    } else {
        pdf_printf("endobj\n"); /* end a PDF object */
    }
}

/* begin a PDF dictionary object */
void pdf_begin_dict(integer i, integer pdf_os_level)
{
    check_pdfminorversion();
    pdf_os_prepare_obj(i, pdf_os_level);
    if (!pdf_os_mode) {
        pdf_printf("%d 0 obj <<\n", (int) i);
    } else {
      if (pdf_compress_level == 0)
        pdf_printf("%% %d 0 obj\n", (int) i);   /* debugging help */
      pdf_printf("<<\n");
    }
}

/* begin a new PDF dictionary object */
void pdf_new_dict(integer t, integer i, integer pdf_os)
{
    pdf_create_obj(t, i);
    pdf_begin_dict(obj_ptr, pdf_os);
}

/* end a PDF dictionary object */
void pdf_end_dict(void)
{
    if (pdf_os_mode) {
        pdf_printf(">>\n");
        if (pdf_os_objidx == pdf_os_max_objs - 1)
            pdf_os_write_objstream();
    } else {
        pdf_printf(">> endobj\n");
    }
}


/*
Write out an accumulated object stream.
First the object number and byte offset pairs are generated
and appended to the ready buffered object stream.
By this the value of \.{/First} can be calculated.
Then a new \.{/ObjStm} object is generated, and everything is
copied to the PDF output buffer, where also compression is done.
When calling this procedure, |pdf_os_mode| must be |true|.
*/

void pdf_os_write_objstream(void)
{
    halfword i, j, p, q;
    if (pdf_os_cur_objnum == 0) /* no object stream started */
        return;
    p = pdf_ptr;
    i = 0;
    j = 0;
    while (i <= pdf_os_objidx) {        /* assemble object number and byte offset pairs */
        pdf_printf("%d %d", (int) pdf_os_objnum[i], (int) pdf_os_objoff[i]);
        if (j == 9) {           /* print out in groups of ten for better readability */
            pdf_out(pdf_new_line_char);
            j = 0;
        } else {
            pdf_printf(" ");
            incr(j);
        }
        incr(i);
    }
    pdf_buf[pdf_ptr - 1] = pdf_new_line_char;   /* no risk of flush, as we are in |pdf_os_mode| */
    q = pdf_ptr;
    pdf_begin_dict(pdf_os_cur_objnum, 0);       /* switch to PDF stream writing */
    pdf_printf("/Type /ObjStm\n");
    pdf_printf("/N %d\n", (int) (pdf_os_objidx + 1));
    pdf_printf("/First %d\n", (int) (q - p));
    pdf_begin_stream();
    pdf_room(q - p);            /* should always fit into the PDF output buffer */
    i = p;
    while (i < q) {             /* write object number and byte offset pairs */
        pdf_quick_out(pdf_os_buf[i]);
        incr(i);
    }
    i = 0;
    while (i < p) {
        q = i + pdf_buf_size;
        if (q > p)
            q = p;
        pdf_room(q - i);
        while (i < q) {         /* write the buffered objects */
            pdf_quick_out(pdf_os_buf[i]);
            incr(i);
        }
    }
    pdf_end_stream();
    pdf_os_cur_objnum = 0;      /* to force object stream generation next time */
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


/* Subroutines to print out various PDF objects */

/* print out an integer with fixed width; used for outputting cross-reference table */
void pdf_print_fw_int(longinteger n, integer w)
{
    integer k;                  /* $0\le k\le23$ */
    k = 0;
    do {
        dig[k] = n % 10;
        n = n / 10;
        k++;
    } while (k != w);
    pdf_room(k);
    while (k-- > 0)
        pdf_quick_out('0' + dig[k]);
}

/* print out an integer as a number of bytes; used for outputting \.{/XRef} cross-reference stream */
void pdf_out_bytes(longinteger n, integer w)
{
    integer k;
    integer bytes[8];           /* digits in a number being output */
    k = 0;
    do {
        bytes[k] = n % 256;
        n = n / 256;
        k++;
    } while (k != w);
    pdf_room(k);
    while (k-- > 0)
        pdf_quick_out(bytes[k]);
}

/* print out an entry in dictionary with integer value to PDF buffer */

void pdf_int_entry(str_number s, integer v)
{
    pdf_out('/');
    pdf_print(s);
    pdf_out(' ');
    pdf_print_int(v);
}

void pdf_int_entry_ln(str_number s, integer v)
{
    pdf_int_entry(s, v);
    pdf_print_nl();
}


/* print out an indirect entry in dictionary */
void pdf_indirect(str_number s, integer o)
{
    pdf_out('/');
    pdf_print(s);
    pdf_printf(" %d 0 R", (int) o);
}

void pdf_indirect_ln(str_number s, integer o)
{
    pdf_indirect(s, o);
    pdf_print_nl();
}

/* print out |s| as string in PDF output */

void pdf_print_str_ln(str_number s)
{
    pdf_print_str(s);
    pdf_print_nl();
}

/* print out an entry in dictionary with string value to PDF buffer */

void pdf_str_entry(str_number s, str_number v)
{
    if (v == 0)
        return;
    pdf_out('/');
    pdf_print(s);
    pdf_out(' ');
    pdf_print_str(v);
}

void pdf_str_entry_ln(str_number s, str_number v)
{
    if (v == 0)
        return;
    pdf_str_entry(s, v);
    pdf_print_nl();
}

/*
To ship out a \TeX\ box to PDF page description we need to implement
|pdf_hlist_out|, |pdf_vlist_out| and |pdf_ship_out|, which are equivalent to
the \TeX' original |hlist_out|, |vlist_out| and |ship_out| resp. But first we
need to declare some procedures needed in |pdf_hlist_out| and |pdf_vlist_out|.
*/

void pdf_out_literal(halfword p)
{
    integer old_setting;        /* holds print |selector| */
    str_number s;
    if (pdf_literal_type(p) == normal) {
        old_setting = selector;
        selector = new_string;
        show_token_list(fixmem[(pdf_literal_data(p))].hhrh, null,
                        pool_size - pool_ptr);
        selector = old_setting;
        s = make_string();
        pdf_literal(s, pdf_literal_mode(p), false);
        flush_str(s);
    } else {
        assert(pdf_literal_mode(p) != scan_special);
        switch (pdf_literal_mode(p)) {
        case set_origin:
            pdf_goto_pagemode();
            pos = synch_p_with_c(cur);
            pdf_set_pos(pos.h, pos.v);
            break;
        case direct_page:
            pdf_goto_pagemode();
            break;
        case direct_always:
            pdf_end_string_nl();
            break;
        default:
            tconfusion("literal1");
            break;
        }
        lua_pdf_literal(pdf_literal_data(p));
    }
}

void pdf_out_colorstack(halfword p)
{
    integer old_setting;
    str_number s;
    integer cmd;
    integer stack_no;
    integer literal_mode;
    cmd = pdf_colorstack_cmd(p);
    stack_no = pdf_colorstack_stack(p);
    literal_mode = 0;
    if (stack_no >= colorstackused()) {
        tprint_nl("");
        tprint("Color stack ");
        print_int(stack_no);
        tprint(" is not initialized for use!");
        tprint_nl("");
        return;
    }
    switch (cmd) {
    case colorstack_set:
    case colorstack_push:
        old_setting = selector;
        selector = new_string;
        show_token_list(fixmem[(pdf_colorstack_data(p))].hhrh, null,
                        pool_size - pool_ptr);
        selector = old_setting;
        s = make_string();
        if (cmd == colorstack_set)
            literal_mode = colorstackset(stack_no, s);
        else
            literal_mode = colorstackpush(stack_no, s);
        if (str_length(s) > 0)
            pdf_literal(s, literal_mode, false);
        flush_str(s);
        return;
        break;
    case colorstack_pop:
        literal_mode = colorstackpop(stack_no);
        break;
    case colorstack_current:
        literal_mode = colorstackcurrent(stack_no);
        break;
    default:
        break;
    }
    if (cur_length > 0) {
        s = make_string();
        pdf_literal(s, literal_mode, false);
        flush_str(s);
    }
}

void pdf_out_colorstack_startpage(void)
{
    integer i;
    integer max;
    integer start_status;
    integer literal_mode;
    str_number s;
    i = 0;
    max = colorstackused();
    while (i < max) {
        start_status = colorstackskippagestart(i);
        if (start_status == 0) {
            literal_mode = colorstackcurrent(i);
            if (cur_length > 0) {
                s = make_string();
                pdf_literal(s, literal_mode, false);
                flush_str(s);
            }
        }
        incr(i);
    }
}

void pdf_out_setmatrix(halfword p)
{
    integer old_setting;        /* holds print |selector| */
    str_number s;
    old_setting = selector;
    selector = new_string;
    show_token_list(fixmem[(pdf_setmatrix_data(p))].hhrh, null,
                    pool_size - pool_ptr);
    selector = old_setting;
    str_room(7);
    str_pool[pool_ptr] = 0;     /* make C string for pdfsetmatrix  */
    pos = synch_p_with_c(cur);
    pdfsetmatrix(str_start_macro(str_ptr), pos);
    str_room(7);
    append_char(' ');
    append_char('0');
    append_char(' ');
    append_char('0');
    append_char(' ');
    append_char('c');
    append_char('m');
    s = make_string();
    pdf_literal(s, set_origin, false);
    flush_str(s);
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

void pdf_special(halfword p)
{
    integer old_setting;        /* holds print |selector| */
    str_number s;
    old_setting = selector;
    selector = new_string;
    show_token_list(fixmem[(write_tokens(p))].hhrh, null, pool_size - pool_ptr);
    selector = old_setting;
    s = make_string();
    pdf_literal(s, scan_special, true);
    flush_str(s);
}

void pdf_print_toks(halfword p)
{
    str_number s = tokens_to_string(p);
    if (str_length(s) > 0)
        pdf_print(s);
    flush_str(s);
}


void pdf_print_toks_ln(halfword p)
{
    str_number s = tokens_to_string(p);
    if (str_length(s) > 0)
        pdf_print_ln(s);
    flush_str(s);
}

void output_form(halfword p)
{
    pdf_goto_pagemode();
    pdf_place_form(pos.h, pos.v, obj_info(pdf_xform_objnum(p)));
    if (pdf_lookup_list(pdf_xform_list, pdf_xform_objnum(p)) == null)
        pdf_append_list(pdf_xform_objnum(p), pdf_xform_list);
}

void output_image(integer idx)
{
    pdf_goto_pagemode();
    out_image(idx, pos.h, pos.v);
    if (pdf_lookup_list(pdf_ximage_list, image_objnum(idx)) == null)
        pdf_append_list(image_objnum(idx), pdf_ximage_list);
}


/*
This code scans forward to the ending |dir_node| while keeping
track of the needed width in |w|. When it finds the node that will end
this segment, it stores the accumulated with in the |dir_dvi_h| field
of that end node, so that when that node is found later in the
processing, the correct glue correction can be applied.
*/

scaled simple_advance_width(halfword p)
{
    halfword q = p;
    scaled w = 0;
    while ((q != null) && (vlink(q) != null)) {
        q = vlink(q);
        if (is_char_node(q)) {
            w = w + glyph_width(q);
        } else {
            switch (type(q)) {
            case hlist_node:
            case vlist_node:
            case rule_node:
            case margin_kern_node:
            case kern_node:
                w = w + width(q);
                break;
            case disc_node:
                if (vlink(no_break(q)) != null)
                    w = w + simple_advance_width(no_break(q));
            default:
                break;
            }
        }
    }
    return w;
}


void calculate_width_to_enddir(halfword p, real cur_glue, scaled cur_g,
                               halfword this_box, scaled * setw,
                               halfword * settemp_ptr)
{
    int dir_nest = 1;
    halfword q = p;
    scaled w = 0;
    halfword temp_ptr = *settemp_ptr;
    halfword g;                 /* this is normally a global variable, but that is just too hideous */
    /* to me, it looks like the next is needed. but Aleph doesn't do that, so let us not do it either */
    real glue_temp;             /* glue value before rounding */
    /* |w:=w-cur_g; cur_glue:=0;| */
    integer g_sign = glue_sign(this_box);
    integer g_order = glue_order(this_box);
    while ((q != null) && (vlink(q) != null)) {
        q = vlink(q);
        if (is_char_node(q)) {
            w = w + glyph_width(q);     /* TODO no vertical support for now */
        } else {
            switch (type(q)) {
            case hlist_node:
            case vlist_node:
            case rule_node:
            case margin_kern_node:
            case kern_node:
                w = w + width(q);
                break;
            case glue_node:
                g = glue_ptr(q);
                w = w + width(g) - cur_g;
                if (g_sign != normal) {
                    if (g_sign == stretching) {
                        if (stretch_order(g) == g_order) {
                            cur_glue = cur_glue + stretch(g);
                            vet_glue(float_cast(glue_set(this_box)) * cur_glue);
                            cur_g = float_round(glue_temp);
                        }
                    } else if (shrink_order(g) == g_order) {
                        cur_glue = cur_glue - shrink(g);
                        vet_glue(float_cast(glue_set(this_box)) * cur_glue);
                        cur_g = float_round(glue_temp);
                    }
                }
                w = w + cur_g;
                break;
            case disc_node:
                if (vlink(no_break(q)) != null) {
                    w = w + simple_advance_width(no_break(q));
                }
                break;
            case math_node:
                w = w + surround(q);
                break;
            case whatsit_node:
                if (subtype(q) == dir_node) {
                    if (dir_dir(q) >= 0)
                        incr(dir_nest);
                    else
                        decr(dir_nest);
                    if (dir_nest == 0) {
                        dir_dvi_h(q) = w;
                        temp_ptr = q;
                        q = null;
                    }
                } else if ((subtype(q) == pdf_refxform_node) ||
                           (subtype(q) == pdf_refximage_node)) {
                    w = w + pdf_width(q);
                }
                break;
            default:
                break;
            }
        }
    }
    *setw = w;
    *settemp_ptr = temp_ptr;
}


/* write a raw PDF object */
void pdf_write_obj(integer n)
{
    str_number s;
    byte_file f;
    integer data_size;          /* total size of the data file */
    integer data_cur;           /* index into |data_buffer| */
    real_eight_bits *data_buffer;       /* byte buffer for data files */
    boolean file_opened;
    integer k;
    boolean res;
    str_number fnam;
    integer callback_id;
    s = tokens_to_string(obj_obj_data(n));
    delete_token_ref(obj_obj_data(n));
    obj_obj_data(n) = null;
    if (obj_obj_is_stream(n) > 0) {
        pdf_begin_dict(n, 0);
        if (obj_obj_stream_attr(n) != null) {
            pdf_print_toks_ln(obj_obj_stream_attr(n));
            delete_token_ref(obj_obj_stream_attr(n));
            obj_obj_stream_attr(n) = null;
        }
        pdf_begin_stream();
    } else {
        pdf_begin_obj(n, 1);
    }
    if (obj_obj_is_file(n) > 0) {
        data_size = 0;
        data_cur = 0;
        data_buffer = 0;
        pack_file_name(s, get_nullstr(), get_nullstr());
        callback_id = callback_defined(find_data_file_callback);
        if (callback_id > 0) {
            res =
                run_callback(callback_id, "S->s", stringcast(nameoffile + 1),
                             addressof(fnam));
            if ((res) && (fnam != 0) && (str_length(fnam) > 0)) {
                /* Fixup |nameoffile| after callback */
                xfree(nameoffile);
                nameoffile =
                    xmallocarray(packed_ASCII_code, str_length(fnam) + 2);
                for (k = str_start_macro(fnam);
                     k >= str_start_macro(fnam + 1) - 1; k++)
                    nameoffile[k - str_start_macro(fnam) + 1] = str_pool[k];
                nameoffile[str_length(fnam) + 1] = 0;
                namelength = str_length(fnam);
                flush_string();
            }
        }
        callback_id = callback_defined(read_data_file_callback);
        if (callback_id > 0) {
            file_opened = false;
            res =
                run_callback(callback_id, "S->bSd", stringcast(nameoffile + 1),
                             addressof(file_opened), addressof(data_buffer),
                             addressof(data_size));
            if (!file_opened)
                pdf_error(maketexstring("ext5"),
                          maketexstring("cannot open file for embedding"));
        } else {
            if (!tex_b_open_in(f))
                pdf_error(maketexstring("ext5"),
                          maketexstring("cannot open file for embedding"));
            res =
                read_data_file(f, addressof(data_buffer), addressof(data_size));
            b_close(f);
        }
        if (!data_size)
            pdf_error(maketexstring("ext5"),
                      maketexstring("empty file for embedding"));
        if (!res)
            pdf_error(maketexstring("ext5"),
                      maketexstring("error reading file for embedding"));
        tprint("<<");
        print(s);
        while (data_cur < data_size) {
            pdf_out(data_buffer[data_cur]);
            incr(data_cur);
        }
        if (data_buffer != 0)
            xfree(data_buffer);
        tprint(">>");
    } else if (obj_obj_is_stream(n) > 0) {
        pdf_print(s);
    } else {
        pdf_print_ln(s);
    }
    if (obj_obj_is_stream(n) > 0)
        pdf_end_stream();
    else
        pdf_end_obj();
    flush_str(s);
}

/* write an image */
void pdf_write_image(integer n)
{
    pdf_begin_dict(n, 0);
    if (fixed_pdf_draftmode == 0)
        write_image(obj_data_ptr(n));
}

/* prints a rect spec */
void pdf_print_rect_spec(halfword r)
{
    pdf_print_mag_bp(pdf_ann_left(r));
    pdf_out(' ');
    pdf_print_mag_bp(pdf_ann_bottom(r));
    pdf_out(' ');
    pdf_print_mag_bp(pdf_ann_right(r));
    pdf_out(' ');
    pdf_print_mag_bp(pdf_ann_top(r));
}

/* output a rectangle specification to PDF file */
void pdf_rectangle(halfword r)
{
    prepare_mag();
    pdf_printf("/Rect [");
    pdf_print_rect_spec(r);
    pdf_printf("]\n");
}


/* write an action specification */
void write_action(halfword p)
{
    str_number s;
    integer d = 0;
    if (pdf_action_type(p) == pdf_action_user) {
        pdf_print_toks_ln(pdf_action_tokens(p));
        return;
    }
    pdf_printf("<< ");
    if (pdf_action_file(p) != null) {
        pdf_printf("/F ");
        s = tokens_to_string(pdf_action_file(p));
        if ((str_pool[str_start_macro(s)] == 40) &&
            (str_pool[str_start_macro(s) + str_length(s) - 1] == 41)) {
            pdf_print(s);
        } else {
            pdf_print_str(s);
        }
        flush_str(s);
        pdf_printf(" ");
        if (pdf_action_new_window(p) > 0) {
            pdf_printf("/NewWindow ");
            if (pdf_action_new_window(p) == 1)
                pdf_printf("true ");
            else
                pdf_printf("false ");
        }
    }
    switch (pdf_action_type(p)) {
    case pdf_action_page:
        if (pdf_action_file(p) == null) {
            pdf_printf("/S /GoTo /D [");
            pdf_print_int(get_obj(obj_type_page, pdf_action_id(p), false));
            pdf_printf(" 0 R");
        } else {
            pdf_printf("/S /GoToR /D [");
            pdf_print_int(pdf_action_id(p) - 1);
        }
        pdf_out(' ');
        pdf_print(tokens_to_string(pdf_action_tokens(p)));
        flush_str(last_tokens_string);
        pdf_out(']');
        break;
    case pdf_action_goto:
        if (pdf_action_file(p) == null) {
            pdf_printf("/S /GoTo ");
            d = get_obj(obj_type_dest, pdf_action_id(p),
                        pdf_action_named_id(p));
        } else {
            pdf_printf("/S /GoToR ");
        }
        if (pdf_action_named_id(p) > 0) {
            pdf_str_entry('D', tokens_to_string(pdf_action_id(p)));
            flush_str(last_tokens_string);
        } else if (pdf_action_file(p) == null) {
            pdf_indirect('D', d);
        } else {
            pdf_error(maketexstring("ext4"),
                      maketexstring
                      ("`goto' option cannot be used with both `file' and `num'"));
        }
        break;
    case pdf_action_thread:
        pdf_printf("/S /Thread ");
        if (pdf_action_file(p) == null) {
            d = get_obj(obj_type_thread, pdf_action_id(p),
                        pdf_action_named_id(p));
            if (pdf_action_named_id(p) > 0) {
                pdf_str_entry('D', tokens_to_string(pdf_action_id(p)));
                flush_str(last_tokens_string);
            } else if (pdf_action_file(p) == null) {
                pdf_indirect('D', d);
            } else {
                pdf_int_entry('D', pdf_action_id(p));
            }
        }
        break;
    }
    pdf_printf(" >>\n");
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

void do_annot(halfword p, halfword parent_box, scaled x, scaled y)
{
    if (!is_shipping_page)
        pdf_error(maketexstring("ext4"),
                  maketexstring("annotations cannot be inside an XForm"));
    if (doing_leaders)
        return;
    if (is_obj_scheduled(pdf_annot_objnum(p)))
        pdf_annot_objnum(p) = pdf_new_objnum();
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p), 0);
    obj_annot_ptr(pdf_annot_objnum(p)) = p;
    pdf_append_list(pdf_annot_objnum(p), pdf_annot_list);
    set_obj_scheduled(pdf_annot_objnum(p));
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

/*
To implement nested link annotations, we need a stack to hold copy of
|pdf_start_link_node|'s that are being written out, together with their box
nesting level.
*/

pdf_link_stack_record pdf_link_stack[(pdf_max_link_level + 1)];
integer pdf_link_stack_ptr = 0;

void push_link_level(halfword p)
{
    if (pdf_link_stack_ptr >= pdf_max_link_level)
        overflow(maketexstring("pdf link stack size"), pdf_max_link_level);
    assert(((type(p) == whatsit_node) && (subtype(p) == pdf_start_link_node)));
    incr(pdf_link_stack_ptr);
    pdf_link_stack[pdf_link_stack_ptr].nesting_level = cur_s;
    pdf_link_stack[pdf_link_stack_ptr].link_node = copy_node_list(p);
    pdf_link_stack[pdf_link_stack_ptr].ref_link_node = p;
}

void pop_link_level(void)
{
    assert(pdf_link_stack_ptr > 0);
    flush_node_list(pdf_link_stack[pdf_link_stack_ptr].link_node);
    decr(pdf_link_stack_ptr);
}

void do_link(halfword p, halfword parent_box, scaled x, scaled y)
{
    if (!is_shipping_page)
        pdf_error(maketexstring("ext4"),
                  maketexstring("link annotations cannot be inside an XForm"));
    assert(type(parent_box) == hlist_node);
    if (is_obj_scheduled(pdf_link_objnum(p)))
        pdf_link_objnum(p) = pdf_new_objnum();
    push_link_level(p);
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p), pdf_link_margin);
    obj_annot_ptr(pdf_link_objnum(p)) = p;      /* the reference for the pdf annot object must be set here */
    pdf_append_list(pdf_link_objnum(p), pdf_link_list);
    set_obj_scheduled(pdf_link_objnum(p));
}

void end_link(void)
{
    halfword p;
    scaledpos tmp1, tmp2;
    if (pdf_link_stack_ptr < 1)
        pdf_error(maketexstring("ext4"),
                  maketexstring
                  ("pdf_link_stack empty, \\pdfendlink used without \\pdfstartlink?"));
    if (pdf_link_stack[pdf_link_stack_ptr].nesting_level != cur_s)
        pdf_error(maketexstring("ext4"),
                  maketexstring
                  ("\\pdfendlink ended up in different nesting level than \\pdfstartlink"));

    /* N.B.: test for running link must be done on |link_node| and not |ref_link_node|,
       as |ref_link_node| can be set by |do_link| or |append_link| already */

    if (is_running(pdf_width(pdf_link_stack[pdf_link_stack_ptr].link_node))) {
        p = pdf_link_stack[pdf_link_stack_ptr].ref_link_node;
        if (is_shipping_page && matrixused()) {
            matrixrecalculate(cur.h + pdf_link_margin);
            pdf_ann_left(p) = getllx() - pdf_link_margin;
            pdf_ann_top(p) = cur_page_size.v - getury() - pdf_link_margin;
            pdf_ann_right(p) = geturx() + pdf_link_margin;
            pdf_ann_bottom(p) = cur_page_size.v - getlly() + pdf_link_margin;
        } else {
            tmp1.h = cur.h;
            tmp1.v = cur.v;
            tmp2 = synch_p_with_c(tmp1);
            switch (box_direction(dvi_direction)) {
            case dir_TL_:
            case dir_BL_:
                pdf_ann_right(p) = tmp2.h + pdf_link_margin;
                break;
            case dir_TR_:
            case dir_BR_:
                pdf_ann_left(p) = tmp2.h - pdf_link_margin;
                break;
            case dir_LT_:
            case dir_RT_:
                pdf_ann_bottom(p) = tmp2.v - pdf_link_margin;
                break;
            case dir_LB_:
            case dir_RB_:
                pdf_ann_top(p) = tmp2.v + pdf_link_margin;
                break;
            }
        }
    }
    pop_link_level();
}

/*
For ``running'' annotations we must append a new node when the end of
annotation is in other box than its start. The new created node is identical to
corresponding whatsit node representing the start of annotation,  but its
|info| field is |max_halfword|. We set |info| field just before destroying the
node, in order to use |flush_node_list| to do the job.
*/

/* append a new pdf annot to |pdf_link_list| */

void append_link(halfword parent_box, scaled x, scaled y, small_number i)
{
    halfword p;
    assert(type(parent_box) == hlist_node);
    p = copy_node(pdf_link_stack[(int) i].link_node);
    pdf_link_stack[(int) i].ref_link_node = p;
    subtype(p) = pdf_link_data_node;    /* this node is not a normal link node */
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p), pdf_link_margin);
    pdf_create_obj(obj_type_others, 0);
    obj_annot_ptr(obj_ptr) = p;
    pdf_append_list(obj_ptr, pdf_link_list);
}

/* Threads are handled in similar way as link annotations */

void append_bead(halfword p)
{
    integer a, b, c, t;
    if (!is_shipping_page)
        pdf_error(maketexstring("ext4"),
                  maketexstring("threads cannot be inside an XForm"));
    t = get_obj(obj_type_thread, pdf_thread_id(p), pdf_thread_named_id(p));
    b = pdf_new_objnum();
    obj_bead_ptr(b) = pdf_get_mem(pdfmem_bead_size);
    obj_bead_page(b) = pdf_last_page;
    obj_bead_data(b) = p;
    if (pdf_thread_attr(p) != null)
        obj_bead_attr(b) = tokens_to_string(pdf_thread_attr(p));
    else
        obj_bead_attr(b) = 0;
    if (obj_thread_first(t) == 0) {
        obj_thread_first(t) = b;
        obj_bead_next(b) = b;
        obj_bead_prev(b) = b;
    } else {
        a = obj_thread_first(t);
        c = obj_bead_prev(a);
        obj_bead_prev(b) = c;
        obj_bead_next(b) = a;
        obj_bead_prev(a) = b;
        obj_bead_next(c) = b;
    }
    pdf_append_list(b, pdf_bead_list);
}

void do_thread(halfword parent_box, halfword p, scaled x, scaled y)
{
    if (doing_leaders)
        return;
    if (subtype(p) == pdf_start_thread_node) {
        pdf_thread_wd = pdf_width(p);
        pdf_thread_ht = pdf_height(p);
        pdf_thread_dp = pdf_depth(p);
        pdf_last_thread_id = pdf_thread_id(p);
        pdf_last_thread_named_id = (pdf_thread_named_id(p) > 0);
        if (pdf_last_thread_named_id)
            add_token_ref(pdf_thread_id(p));
        pdf_thread_level = cur_s;
    }
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p),
                    pdf_thread_margin);
    append_bead(p);
    last_thread = p;
}

void append_thread(halfword parent_box, scaled x, scaled y)
{
    halfword p;
    p = new_node(whatsit_node, pdf_thread_data_node);
    pdf_width(p) = pdf_thread_wd;
    pdf_height(p) = pdf_thread_ht;
    pdf_depth(p) = pdf_thread_dp;
    pdf_thread_attr(p) = null;
    pdf_thread_id(p) = pdf_last_thread_id;
    if (pdf_last_thread_named_id) {
        add_token_ref(pdf_thread_id(p));
        pdf_thread_named_id(p) = 1;
    } else {
        pdf_thread_named_id(p) = 0;
    }
    set_rect_dimens(p, parent_box, x, y,
                    pdf_width(p), pdf_height(p), pdf_depth(p),
                    pdf_thread_margin);
    append_bead(p);
    last_thread = p;
}

void end_thread(void)
{
    scaledpos tmp1, tmp2;
    if (pdf_thread_level != cur_s)
        pdf_error(maketexstring("ext4"),
                  maketexstring
                  ("\\pdfendthread ended up in different nesting level than \\pdfstartthread"));
    if (is_running(pdf_thread_dp) && (last_thread != null)) {
        tmp1.h = cur.h;
        tmp1.v = cur.v;
        tmp2 = synch_p_with_c(tmp1);
        switch (box_direction(dvi_direction)) {
        case dir_TL_:
        case dir_TR_:
            pdf_ann_bottom(last_thread) = tmp2.v - pdf_thread_margin;
            break;
        case dir_BL_:
        case dir_BR_:
            pdf_ann_top(last_thread) = tmp2.v + pdf_thread_margin;
            break;
        case dir_LT_:
        case dir_LB_:
            pdf_ann_right(last_thread) = tmp2.h + pdf_thread_margin;
            break;
        case dir_RT_:
        case dir_RB_:
            pdf_ann_left(last_thread) = tmp2.h - pdf_thread_margin;
            break;
        }
    }
    if (pdf_last_thread_named_id)
        delete_token_ref(pdf_last_thread_id);
    last_thread = null;
}


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


/* The implementation of procedure |pdf_hlist_out| is similiar to |hlist_out| */

void pdf_hlist_out(void)
{
    /*label move_past, fin_rule, next_p; */
    scaled base_line;           /* the baseline coordinate for this box */
    scaled w;                   /*  temporary value for directional width calculation  */
    scaled edge_v;
    scaled edge_h;
    scaled effective_horizontal;
    scaled basepoint_horizontal;
    scaled basepoint_vertical;
    integer save_direction;
    halfword dvi_ptr, dir_ptr, dir_tmp;
    scaled left_edge;           /* the left coordinate for this box */
    scaled save_h;              /* what |cur.h| should pop to */
    scaledpos save_box_pos;     /* what |box_pos| should pop to */
    halfword this_box;          /* pointer to containing box */
    integer g_order;            /* applicable order of infinity for glue */
    integer g_sign;             /* selects type of glue */
    halfword p, q;              /* current position in the hlist */
    halfword leader_box;        /* the leader box being replicated */
    scaled leader_wd;           /* width of leader box being replicated */
    scaled lx;                  /* extra space between leader boxes */
    boolean outer_doing_leaders;        /* were we doing leaders? */
    scaled edge;                /* right edge of sub-box or leader space */
    halfword prev_p;            /* one step behind |p| */
    real glue_temp;             /* glue value before rounding */
    real cur_glue;              /* glue seen so far */
    scaled cur_g;               /* rounded equivalent of |cur_glue| times the glue ratio */
    int i;                      /* index to scan |pdf_link_stack| */
    cur_g = 0;
    cur_glue = 0.0;
    this_box = temp_ptr;
    g_order = glue_order(this_box);
    g_sign = glue_sign(this_box);
    p = list_ptr(this_box);
    set_to_zero(cur);
    box_pos = pos;
    save_direction = dvi_direction;
    dvi_direction = box_dir(this_box);
    dvi_ptr = 0;
    lx = 0;
    /* Initialize |dir_ptr| for |ship_out| */
    {
        dir_ptr = null;
        push_dir(dvi_direction);
        dir_dvi_ptr(dir_ptr) = dvi_ptr;
    }
    incr(cur_s);
    base_line = cur.v;
    left_edge = cur.h;
    prev_p = this_box + list_offset;
    /* Create link annotations for the current hbox if needed */
    for (i = 1; i <= pdf_link_stack_ptr; i++) {
        assert(is_running(pdf_width(pdf_link_stack[i].link_node)));
        if (pdf_link_stack[i].nesting_level == cur_s)
            append_link(this_box, left_edge, base_line, i);
    }

    /* Start hlist {\sl Sync\TeX} information record */
    synctex_hlist(this_box);

    while (p != null) {
        if (is_char_node(p)) {
            do {
                if (x_displace(p) != 0)
                    cur.h = cur.h + x_displace(p);
                if (y_displace(p) != 0)
                    cur.v = cur.v - y_displace(p);
                output_one_char(font(p), character(p));
                if (x_displace(p) != 0)
                    cur.h = cur.h - x_displace(p);
                if (y_displace(p) != 0)
                    cur.v = cur.v + y_displace(p);
                p = vlink(p);
            } while (is_char_node(p));
            /* Record current point {\sl Sync\TeX} information */
            synctex_current();
        } else {
            /* Output the non-|char_node| |p| for |pdf_hlist_out|
               and move to the next node */
            switch (type(p)) {
            case hlist_node:
            case vlist_node:
                /* (\pdfTeX) Output a box in an hlist */

                if (!
                    (dir_orthogonal
                     (dir_primary[box_dir(p)], dir_primary[dvi_direction]))) {
                    effective_horizontal = width(p);
                    basepoint_vertical = 0;
                    if (dir_opposite
                        (dir_secondary[box_dir(p)],
                         dir_secondary[dvi_direction]))
                        basepoint_horizontal = width(p);
                    else
                        basepoint_horizontal = 0;
                } else {
                    effective_horizontal = height(p) + depth(p);
                    if (!is_mirrored(box_dir(p))) {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[dvi_direction]))
                            basepoint_horizontal = height(p);
                        else
                            basepoint_horizontal = depth(p);
                    } else {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[dvi_direction]))
                            basepoint_horizontal = depth(p);
                        else
                            basepoint_horizontal = height(p);
                    }
                    if (dir_eq
                        (dir_secondary[box_dir(p)], dir_primary[dvi_direction]))
                        basepoint_vertical = -(width(p) / 2);
                    else
                        basepoint_vertical = (width(p) / 2);
                }
                if (!is_mirrored(dvi_direction))
                    basepoint_vertical = basepoint_vertical + shift_amount(p);  /* shift the box `down' */
                else
                    basepoint_vertical = basepoint_vertical - shift_amount(p);  /* shift the box `up' */
                if (list_ptr(p) == null) {
                    /* Record void list {\sl Sync\TeX} information */
                    if (type(p) == vlist_node)
                        synctex_void_vlist(p, this_box);
                    else
                        synctex_void_hlist(p, this_box);

                    cur.h = cur.h + effective_horizontal;
                } else {
                    temp_ptr = p;
                    edge = cur.h;
                    cur.h = cur.h + basepoint_horizontal;
                    edge_v = cur.v;
                    cur.v = base_line + basepoint_vertical;
                    pos = synch_p_with_c(cur);
                    save_box_pos = box_pos;
                    if (type(p) == vlist_node)
                        pdf_vlist_out();
                    else
                        pdf_hlist_out();
                    box_pos = save_box_pos;
                    cur.h = edge + effective_horizontal;
                    cur.v = base_line;
                }
                break;
            case disc_node:
                if (vlink(no_break(p)) != null) {
                    if (subtype(p) != select_disc) {
                        q = tail_of_list(vlink(no_break(p)));   /* TODO, this should be a tlink */
                        vlink(q) = vlink(p);
                        q = vlink(no_break(p));
                        vlink(no_break(p)) = null;
                        vlink(p) = q;
                    }
                }
                break;
            case rule_node:
                if (!
                    (dir_orthogonal
                     (dir_primary[rule_dir(p)], dir_primary[dvi_direction]))) {
                    rule_ht = height(p);
                    rule_dp = depth(p);
                    rule_wd = width(p);
                } else {
                    rule_ht = width(p) / 2;
                    rule_dp = width(p) / 2;
                    rule_wd = height(p) + depth(p);
                }
                goto FIN_RULE;
                break;
            case whatsit_node:
                /* Output the whatsit node |p| in |pdf_hlist_out| */

                switch (subtype(p)) {
                case pdf_literal_node:
                    pdf_out_literal(p);
                    break;
                case pdf_colorstack_node:
                    pdf_out_colorstack(p);
                    break;
                case pdf_setmatrix_node:
                    pdf_out_setmatrix(p);
                    break;
                case pdf_save_node:
                    pdf_out_save();
                    break;
                case pdf_restore_node:
                    pdf_out_restore();
                    break;
                case late_lua_node:
                    do_late_lua(p);
                    break;
                case pdf_refobj_node:
                    if (!is_obj_scheduled(pdf_obj_objnum(p))) {
                        pdf_append_list(pdf_obj_objnum(p), pdf_obj_list);
                        set_obj_scheduled(pdf_obj_objnum(p));
                    }
                    break;
                case pdf_refxform_node:
                    /* Output a Form node in a hlist */
                    cur.v = base_line;
                    pos = synch_p_with_c(cur);
                    switch (box_direction(dvi_direction)) {
                    case dir_TL_:
                        pos_down(pdf_depth(p));
                        break;
                    case dir_TR_:
                        pos_down(pdf_depth(p));
                        pos_left(pdf_width(p));
                        break;
                    }
                    output_form(p);
                    edge = cur.h + pdf_width(p);
                    cur.h = edge;
                    break;
                case pdf_refximage_node:
                    /* Output an Image node in a hlist */
                    cur.v = base_line;
                    pos = synch_p_with_c(cur);
                    switch (box_direction(dvi_direction)) {
                    case dir_TL_:
                        pos_down(pdf_depth(p));
                        break;
                    case dir_TR_:
                        pos_left(pdf_width(p));
                        pos_down(pdf_depth(p));
                        break;
                    case dir_BL_:
                        pos_down(pdf_height(p));
                        break;
                    case dir_BR_:
                        pos_left(pdf_width(p));
                        pos_down(pdf_height(p));
                        break;
                    case dir_LT_:
                        pos_left(pdf_height(p));
                        pos_down(pdf_width(p));
                        break;
                    case dir_RT_:
                        pos_left(pdf_depth(p));
                        pos_down(pdf_width(p));
                        break;
                    case dir_LB_:
                        pos_left(pdf_height(p));
                        break;
                    case dir_RB_:
                        pos_left(pdf_depth(p));
                        break;
                    }
                    output_image(pdf_ximage_idx(p));
                    edge = cur.h + pdf_width(p);
                    cur.h = edge;
                    break;
                case pdf_annot_node:
                    do_annot(p, this_box, left_edge, base_line);
                    break;
                case pdf_start_link_node:
                    do_link(p, this_box, left_edge, base_line);
                    break;
                case pdf_end_link_node:
                    end_link();
                    break;
                case pdf_dest_node:
                    do_dest(p, this_box, left_edge, base_line);
                    break;
                case pdf_thread_node:
                    do_thread(p, this_box, left_edge, base_line);
                    break;
                case pdf_start_thread_node:
                    pdf_error(maketexstring("ext4"),
                              maketexstring
                              ("\\pdfstartthread ended up in hlist"));
                    break;
                case pdf_end_thread_node:
                    pdf_error(maketexstring("ext4"),
                              maketexstring
                              ("\\pdfendthread ended up in hlist"));
                    break;
                case pdf_save_pos_node:
                    /* Save current position */
                    pos = synch_p_with_c(cur);
                    pdf_last_x_pos = pos.h;
                    pdf_last_y_pos = pos.v;
                    break;
                case special_node:
                    pdf_special(p);
                    break;
                case dir_node:
                    /* Output a reflection instruction if the direction has changed */
                    /* TODO: this whole case code block is the same in DVI mode */
                    if (dir_dir(p) >= 0) {
                        push_dir_node(p);
                        /* (PDF) Calculate the needed width to the matching |enddir|, and store it in |w|, as
                           well as in the enddirs |dir_dvi_h| */
                        calculate_width_to_enddir(p, cur_glue, cur_g, this_box,
                                                  &w, &temp_ptr);

                        if ((dir_opposite
                             (dir_secondary[dir_dir(dir_ptr)],
                              dir_secondary[dvi_direction]))
                            ||
                            (dir_eq
                             (dir_secondary[dir_dir(dir_ptr)],
                              dir_secondary[dvi_direction]))) {
                            dir_cur_h(temp_ptr) = cur.h + w;
                            if (dir_opposite
                                (dir_secondary[dir_dir(dir_ptr)],
                                 dir_secondary[dvi_direction]))
                                cur.h = cur.h + w;
                        } else {
                            dir_cur_h(temp_ptr) = cur.h;
                        }
                        dir_cur_v(temp_ptr) = cur.v;
                        dir_box_pos_h(temp_ptr) = box_pos.h;
                        dir_box_pos_v(temp_ptr) = box_pos.v;
                        pos = synch_p_with_c(cur);      /* no need for |synch_dvi_with_cur|, as there is no DVI grouping */
                        box_pos = pos;  /* fake a nested |hlist_out| */
                        set_to_zero(cur);
                        dvi_direction = dir_dir(dir_ptr);
                    } else {
                        pop_dir_node();
                        box_pos.h = dir_box_pos_h(p);
                        box_pos.v = dir_box_pos_v(p);
                        cur.h = dir_cur_h(p);
                        cur.v = dir_cur_v(p);
                        if (dir_ptr != null)
                            dvi_direction = dir_dir(dir_ptr);
                        else
                            dvi_direction = 0;
                    }
                    break;
                default:
                    out_what(p);
                }
                break;
            case glue_node:
                /* (\pdfTeX) Move right or output leaders */

                g = glue_ptr(p);
                rule_wd = width(g) - cur_g;
                if (g_sign != normal) {
                    if (g_sign == stretching) {
                        if (stretch_order(g) == g_order) {
                            cur_glue = cur_glue + stretch(g);
                            vet_glue(float_cast(glue_set(this_box)) * cur_glue);        /* real multiplication */
                            cur_g = float_round(glue_temp);
                        }
                    } else if (shrink_order(g) == g_order) {
                        cur_glue = cur_glue - shrink(g);
                        vet_glue(float_cast(glue_set(this_box)) * cur_glue);
                        cur_g = float_round(glue_temp);
                    }
                }
                rule_wd = rule_wd + cur_g;
                if (subtype(p) >= a_leaders) {
                    /* (\pdfTeX) Output leaders in an hlist, |goto fin_rule| if a rule
                       or to |next_p| if done */
                    leader_box = leader_ptr(p);
                    if (type(leader_box) == rule_node) {
                        rule_ht = height(leader_box);
                        rule_dp = depth(leader_box);
                        goto FIN_RULE;
                    }
                    if (!
                        (dir_orthogonal
                         (dir_primary[box_dir(leader_box)],
                          dir_primary[dvi_direction])))
                        leader_wd = width(leader_box);
                    else
                        leader_wd = height(leader_box) + depth(leader_box);
                    if ((leader_wd > 0) && (rule_wd > 0)) {
                        rule_wd = rule_wd + 10; /* compensate for floating-point rounding */
                        edge = cur.h + rule_wd;
                        lx = 0;
                        /* Let |cur.h| be the position of the first box, and set |leader_wd+lx|
                           to the spacing between corresponding parts of boxes */
                        {
                            if (subtype(p) == a_leaders) {
                                save_h = cur.h;
                                cur.h =
                                    left_edge +
                                    leader_wd * ((cur.h - left_edge) /
                                                 leader_wd);
                                if (cur.h < save_h)
                                    cur.h = cur.h + leader_wd;
                            } else {
                                lq = rule_wd / leader_wd;       /* the number of box copies */
                                lr = rule_wd % leader_wd;       /* the remaining space */
                                if (subtype(p) == c_leaders) {
                                    cur.h = cur.h + (lr / 2);
                                } else {
                                    lx = lr / (lq + 1);
                                    cur.h = cur.h + ((lr - (lq - 1) * lx) / 2);
                                }
                            }
                        }
                        while (cur.h + leader_wd <= edge) {
                            /* (\pdfTeX) Output a leader box at |cur.h|,
                               then advance |cur.h| by |leader_wd+lx| */

                            if (!
                                (dir_orthogonal
                                 (dir_primary[box_dir(leader_box)],
                                  dir_primary[dvi_direction]))) {
                                basepoint_vertical = 0;
                                if (dir_opposite
                                    (dir_secondary[box_dir(leader_box)],
                                     dir_secondary[dvi_direction]))
                                    basepoint_horizontal = width(leader_box);
                                else
                                    basepoint_horizontal = 0;
                            } else {
                                if (!is_mirrored(box_dir(leader_box))) {
                                    if (dir_eq
                                        (dir_primary[box_dir(leader_box)],
                                         dir_secondary[dvi_direction]))
                                        basepoint_horizontal =
                                            height(leader_box);
                                    else
                                        basepoint_horizontal =
                                            depth(leader_box);
                                } else {
                                    if (dir_eq
                                        (dir_primary[box_dir(leader_box)],
                                         dir_secondary[dvi_direction]))
                                        basepoint_horizontal =
                                            depth(leader_box);
                                    else
                                        basepoint_horizontal =
                                            height(leader_box);
                                }
                                if (dir_eq
                                    (dir_secondary[box_dir(leader_box)],
                                     dir_primary[dvi_direction]))
                                    basepoint_vertical =
                                        -(width(leader_box) / 2);
                                else
                                    basepoint_vertical =
                                        (width(leader_box) / 2);
                            }
                            if (!is_mirrored(dvi_direction))
                                basepoint_vertical = basepoint_vertical + shift_amount(leader_box);     /* shift the box `down' */
                            else
                                basepoint_vertical = basepoint_vertical - shift_amount(leader_box);     /* shift the box `up' */
                            temp_ptr = leader_box;
                            edge_h = cur.h;
                            cur.h = cur.h + basepoint_horizontal;
                            edge_v = cur.v;
                            cur.v = base_line + basepoint_vertical;
                            pos = synch_p_with_c(cur);
                            save_box_pos = box_pos;
                            outer_doing_leaders = doing_leaders;
                            doing_leaders = true;
                            if (type(leader_box) == vlist_node)
                                pdf_vlist_out();
                            else
                                pdf_hlist_out();
                            doing_leaders = outer_doing_leaders;
                            box_pos = save_box_pos;
                            cur.h = edge_h + leader_wd + lx;
                            cur.v = base_line;
                        }
                        cur.h = edge - 10;
                        goto NEXTP;
                    }
                }
                goto MOVE_PAST;
                break;
            case margin_kern_node:
                cur.h = cur.h + width(p);
                break;
            case kern_node:
                /* Record |kern_node| {\sl Sync\TeX} information */
                synctex_kern(p, this_box);
                cur.h = cur.h + width(p);
                break;
            case math_node:
                /* Record |math_node| {\sl Sync\TeX} information */
                synctex_math(p, this_box);
                cur.h = cur.h + surround(p);
                break;
            default:
                break;
            }
            goto NEXTP;
          FIN_RULE:
            /* (\pdfTeX) Output a rule in an hlist */
            if (is_running(rule_ht))
                rule_ht = height(this_box);
            if (is_running(rule_dp))
                rule_dp = depth(this_box);
            if (((rule_ht + rule_dp) > 0) && (rule_wd > 0)) {   /* we don't output empty rules */
                pos = synch_p_with_c(cur);
                /* *INDENT-OFF* */
                switch (box_direction(dvi_direction)) {
                case dir_TL_: 
                    pdf_place_rule(pos.h,           pos.v - rule_dp, rule_wd,           rule_ht + rule_dp);
                    break;
                case dir_BL_: 
                    pdf_place_rule(pos.h,           pos.v - rule_ht, rule_wd,           rule_ht + rule_dp);
                    break;
                case dir_TR_: 
                    pdf_place_rule(pos.h - rule_wd, pos.v - rule_dp, rule_wd,           rule_ht + rule_dp);
                    break;
                case dir_BR_: 
                    pdf_place_rule(pos.h - rule_wd, pos.v - rule_ht, rule_wd,           rule_ht + rule_dp);
                    break;
                case dir_LT_:
                    pdf_place_rule(pos.h - rule_ht, pos.v - rule_wd, rule_ht + rule_dp, rule_wd);
                    break;
                case dir_RT_: 
                    pdf_place_rule(pos.h - rule_dp, pos.v - rule_wd, rule_ht + rule_dp, rule_wd);
                    break;
                case dir_LB_: 
                    pdf_place_rule(pos.h - rule_ht, pos.v,           rule_ht + rule_dp, rule_wd);
                    break;
                case dir_RB_: 
                    pdf_place_rule(pos.h - rule_dp, pos.v,           rule_ht + rule_dp, rule_wd);
                    break;
                }
                /* *INDENT-ON* */
            }
          MOVE_PAST:
            cur.h = cur.h + rule_wd;
            /* Record horizontal |rule_node| or |glue_node| {\sl Sync\TeX} information */
            synctex_horizontal_rule_or_glue(p, this_box);
          NEXTP:
            prev_p = p;
            p = vlink(p);
        }
    }

    /* Finish hlist {\sl Sync\TeX} information record */
    synctex_tsilh(this_box);

    decr(cur_s);
    dvi_direction = save_direction;
    /* DIR: Reset |dir_ptr| */
    {
        while (dir_ptr != null)
            pop_dir_node();
    }
}


/* The |pdf_vlist_out| routine is similar to |pdf_hlist_out|, but a bit simpler. */

void pdf_vlist_out(void)
{
    scaled left_edge;           /* the left coordinate for this box */
    scaled top_edge;            /* the top coordinate for this box */
    scaled save_v;              /* what |cur.v| should pop to */
    scaledpos save_box_pos;     /* what |box_pos| should pop to */
    halfword this_box;          /* pointer to containing box */
    glue_ord g_order;           /* applicable order of infinity for glue */
    integer g_sign;             /* selects type of glue */
    halfword p;                 /* current position in the vlist */
    halfword leader_box;        /* the leader box being replicated */
    scaled leader_ht;           /* height of leader box being replicated */
    scaled lx;                  /* extra space between leader boxes */
    boolean outer_doing_leaders;        /* were we doing leaders? */
    scaled edge;                /* bottom boundary of leader space */
    real glue_temp;             /* glue value before rounding */
    real cur_glue;              /* glue seen so far */
    scaled cur_g;               /* rounded equivalent of |cur_glue| times the glue ratio */
    integer save_direction;
    scaled effective_vertical;
    scaled basepoint_horizontal;
    scaled basepoint_vertical;
    scaled edge_v;
    cur_g = 0;
    cur_glue = 0.0;
    this_box = temp_ptr;
    g_order = glue_order(this_box);
    g_sign = glue_sign(this_box);
    p = list_ptr(this_box);
    set_to_zero(cur);
    box_pos = pos;
    save_direction = dvi_direction;
    dvi_direction = box_dir(this_box);
    incr(cur_s);
    left_edge = cur.h;
    /* Start vlist {\sl Sync\TeX} information record */
    synctex_vlist(this_box);

    cur.v = cur.v - height(this_box);
    top_edge = cur.v;
    /* Create thread for the current vbox if needed */
    if ((last_thread != null) && is_running(pdf_thread_dp)
        && (pdf_thread_level == cur_s))
        append_thread(this_box, left_edge, top_edge + height(this_box));

    while (p != null) {
        if (is_char_node(p)) {
            tconfusion("pdfvlistout");  /* this can't happen */
        } else {
            /* Output the non-|char_node| |p| for |pdf_vlist_out| */

            switch (type(p)) {
            case hlist_node:
            case vlist_node:
                /* (\pdfTeX) Output a box in a vlist */

                /* TODO: the direct test to switch between |width(p)| and |-width(p)|
                   is definately wrong, because it does not nest properly. But at least
                   it fixes a very obvious problem that otherwise occured with
                   \.{\\pardir TLT} in a document with \.{\\bodydir TRT}, and so it
                   will have to do for now.
                 */
                if (!
                    (dir_orthogonal
                     (dir_primary[box_dir(p)], dir_primary[dvi_direction]))) {
                    effective_vertical = height(p) + depth(p);
                    if ((type(p) == hlist_node) && is_mirrored(box_dir(p)))
                        basepoint_vertical = depth(p);
                    else
                        basepoint_vertical = height(p);
                    if (dir_opposite
                        (dir_secondary[box_dir(p)],
                         dir_secondary[dvi_direction]))
                        basepoint_horizontal = width(p);
                    else
                        basepoint_horizontal = 0;
                } else {
                    effective_vertical = width(p);
                    if (!is_mirrored(box_dir(p))) {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[dvi_direction]))
                            basepoint_horizontal = height(p);
                        else
                            basepoint_horizontal = depth(p);
                    } else {
                        if (dir_eq
                            (dir_primary[box_dir(p)],
                             dir_secondary[dvi_direction]))
                            basepoint_horizontal = depth(p);
                        else
                            basepoint_horizontal = height(p);
                    }
                    if (dir_eq
                        (dir_secondary[box_dir(p)], dir_primary[dvi_direction]))
                        basepoint_vertical = 0;
                    else
                        basepoint_vertical = width(p);
                }
                basepoint_horizontal = basepoint_horizontal + shift_amount(p);  /* shift the box `right' */
                if (list_ptr(p) == null) {
                    cur.v = cur.v + effective_vertical;
                    /* Record void list {\sl Sync\TeX} information */
                    if (type(p) == vlist_node)
                        synctex_void_vlist(p, this_box);
                    else
                        synctex_void_hlist(p, this_box);

                } else {
                    edge_v = cur.v;
                    cur.h = left_edge + basepoint_horizontal;
                    cur.v = cur.v + basepoint_vertical;
                    pos = synch_p_with_c(cur);
                    save_box_pos = box_pos;
                    temp_ptr = p;
                    if (type(p) == vlist_node)
                        pdf_vlist_out();
                    else
                        pdf_hlist_out();
                    box_pos = save_box_pos;
                    cur.h = left_edge;
                    cur.v = edge_v + effective_vertical;
                }
                break;
            case rule_node:
                if (!
                    (dir_orthogonal
                     (dir_primary[rule_dir(p)], dir_primary[dvi_direction]))) {
                    rule_ht = height(p);
                    rule_dp = depth(p);
                    rule_wd = width(p);
                } else {
                    rule_ht = width(p) / 2;
                    rule_dp = width(p) / 2;
                    rule_wd = height(p) + depth(p);
                }
                goto FIN_RULE;
                break;
            case whatsit_node:
                /* Output the whatsit node |p| in |pdf_vlist_out| */
                switch (subtype(p)) {
                case pdf_literal_node:
                    pdf_out_literal(p);
                    break;
                case pdf_colorstack_node:
                    pdf_out_colorstack(p);
                    break;
                case pdf_setmatrix_node:
                    pdf_out_setmatrix(p);
                    break;
                case pdf_save_node:
                    pdf_out_save();
                    break;
                case pdf_restore_node:
                    pdf_out_restore();
                    break;
                case late_lua_node:
                    do_late_lua(p);
                    break;
                case pdf_refobj_node:
                    if (!is_obj_scheduled(pdf_obj_objnum(p))) {
                        pdf_append_list(pdf_obj_objnum(p), pdf_obj_list);
                        set_obj_scheduled(pdf_obj_objnum(p));
                    }
                    break;
                case pdf_refxform_node:
                    /* Output a Form node in a vlist */
                    cur.h = left_edge;
                    pos = synch_p_with_c(cur);
                    switch (box_direction(dvi_direction)) {
                    case dir_TL_:
                        pos_down(pdf_height(p) + pdf_depth(p));
                        break;
                    case dir_TR_:
                        pos_left(pdf_width(p));
                        pos_down(pdf_height(p) + pdf_depth(p));
                        break;
                    default:
                        break;
                    }
                    output_form(p);
                    cur.v = cur.v + pdf_height(p) + pdf_depth(p);
                    break;
                case pdf_refximage_node:
                    /* Output an Image node in a vlist */
                    cur.h = left_edge;
                    pos = synch_p_with_c(cur);
                    switch (box_direction(dvi_direction)) {
                    case dir_TL_:
                        pos_down(pdf_height(p) + pdf_depth(p));
                        break;
                    case dir_TR_:
                        pos_left(pdf_width(p));
                        pos_down(pdf_height(p) + pdf_depth(p));
                        break;
                    case dir_BL_:
                        break;
                    case dir_BR_:
                        pos_left(pdf_width(p));
                        break;
                    case dir_LT_:
                        pos_down(pdf_width(p));
                        break;
                    case dir_RT_:
                        pos_left(pdf_height(p) + pdf_depth(p));
                        pos_down(pdf_width(p));
                        break;
                    case dir_LB_:
                        break;
                    case dir_RB_:
                        pos_left(pdf_height(p) + pdf_depth(p));
                        break;
                    default:
                        break;
                    }
                    output_image(pdf_ximage_idx(p));
                    cur.v = cur.v + pdf_height(p) + pdf_depth(p);
                    break;
                case pdf_annot_node:
                    do_annot(p, this_box, left_edge,
                             top_edge + height(this_box));
                    break;
                case pdf_start_link_node:
                    pdf_error(maketexstring("ext4"),
                              maketexstring
                              ("\\pdfstartlink ended up in vlist"));
                    break;
                case pdf_end_link_node:
                    pdf_error(maketexstring("ext4"),
                              maketexstring("\\pdfendlink ended up in vlist"));
                    break;
                case pdf_dest_node:
                    do_dest(p, this_box, left_edge,
                            top_edge + height(this_box));
                    break;
                case pdf_thread_node:
                case pdf_start_thread_node:
                    do_thread(p, this_box, left_edge,
                              top_edge + height(this_box));
                    break;
                case pdf_end_thread_node:
                    end_thread();
                    break;
                case pdf_save_pos_node:
                    /* Save current position */
                    pos = synch_p_with_c(cur);
                    pdf_last_x_pos = pos.h;
                    pdf_last_y_pos = pos.v;
                    break;
                case special_node:
                    pdf_special(p);
                    break;
                default:
                    out_what(p);
                    break;
                }
                break;
            case glue_node:
                /* (\pdfTeX) Move down or output leaders */
                g = glue_ptr(p);
                rule_ht = width(g) - cur_g;
                if (g_sign != normal) {
                    if (g_sign == stretching) {
                        if (stretch_order(g) == g_order) {
                            cur_glue = cur_glue + stretch(g);
                            vet_glue(float_cast(glue_set(this_box)) * cur_glue);        /* real multiplication */
                            cur_g = float_round(glue_temp);
                        }
                    } else if (shrink_order(g) == g_order) {
                        cur_glue = cur_glue - shrink(g);
                        vet_glue(float_cast(glue_set(this_box)) * cur_glue);
                        cur_g = float_round(glue_temp);
                    }
                }
                rule_ht = rule_ht + cur_g;
                if (subtype(p) >= a_leaders) {
                    /* (\pdfTeX) Output leaders in a vlist, |goto fin_rule| if a rule or to |next_p| if done */
                    leader_box = leader_ptr(p);
                    if (type(leader_box) == rule_node) {
                        rule_wd = width(leader_box);
                        rule_dp = 0;
                        goto FIN_RULE;
                    }
                    leader_ht = height(leader_box) + depth(leader_box);
                    if ((leader_ht > 0) && (rule_ht > 0)) {
                        rule_ht = rule_ht + 10; /* compensate for floating-point rounding */
                        edge = cur.v + rule_ht;
                        lx = 0;
                        /* Let |cur.v| be the position of the first box, and set |leader_ht+lx|
                           to the spacing between corresponding parts of boxes */
                        /* TODO: module can be shared with DVI */
                        if (subtype(p) == a_leaders) {
                            save_v = cur.v;
                            cur.v =
                                top_edge +
                                leader_ht * ((cur.v - top_edge) / leader_ht);
                            if (cur.v < save_v)
                                cur.v = cur.v + leader_ht;
                        } else {
                            lq = rule_ht / leader_ht;   /* the number of box copies */
                            lr = rule_ht % leader_ht;   /* the remaining space */
                            if (subtype(p) == c_leaders) {
                                cur.v = cur.v + (lr / 2);
                            } else {
                                lx = lr / (lq + 1);
                                cur.v = cur.v + ((lr - (lq - 1) * lx) / 2);
                            }
                        }

                        while (cur.v + leader_ht <= edge) {
                            /* (\pdfTeX) Output a leader box at |cur.v|,
                               then advance |cur.v| by |leader_ht+lx| */
                            cur.h = left_edge + shift_amount(leader_box);
                            cur.v = cur.v + height(leader_box);
                            save_v = cur.v;
                            pos = synch_p_with_c(cur);
                            save_box_pos = box_pos;
                            temp_ptr = leader_box;
                            outer_doing_leaders = doing_leaders;
                            doing_leaders = true;
                            if (type(leader_box) == vlist_node)
                                pdf_vlist_out();
                            else
                                pdf_hlist_out();
                            doing_leaders = outer_doing_leaders;
                            box_pos = save_box_pos;
                            cur.h = left_edge;
                            cur.v =
                                save_v - height(leader_box) + leader_ht + lx;
                        }
                        cur.v = edge - 10;
                        goto NEXTP;
                    }
                }
                goto MOVE_PAST;
                break;
            case kern_node:
                cur.v = cur.v + width(p);
                break;
            default:
                break;
            }
            goto NEXTP;
          FIN_RULE:
            /* (\pdfTeX) Output a rule in a vlist, |goto next_p| */
            if (is_running(rule_wd))
                rule_wd = width(this_box);
            rule_ht = rule_ht + rule_dp;        /* this is the rule thickness */
            if ((rule_ht > 0) && (rule_wd > 0)) {       /* we don't output empty rules */
                pos = synch_p_with_c(cur);
                /* *INDENT-OFF* */
                switch (box_direction(dvi_direction)) {
                case dir_TL_: 
                    pdf_place_rule(pos.h,           pos.v - rule_ht, rule_wd, rule_ht);
                    break;
                case dir_BL_: 
                    pdf_place_rule(pos.h,           pos.v,           rule_wd, rule_ht);
                    break;
                case dir_TR_: 
                    pdf_place_rule(pos.h - rule_wd, pos.v - rule_ht, rule_wd, rule_ht);
                    break;
                case dir_BR_: 
                    pdf_place_rule(pos.h - rule_wd, pos.v,           rule_wd, rule_ht);
                    break;
                case dir_LT_: 
                    pdf_place_rule(pos.h,           pos.v - rule_wd, rule_ht, rule_wd);
                    break;
                case dir_RT_: 
                    pdf_place_rule(pos.h - rule_ht, pos.v - rule_wd, rule_ht, rule_wd);
                    break;
                case dir_LB_: 
                    pdf_place_rule(pos.h,           pos.v,           rule_ht, rule_wd);
                    break;
                case dir_RB_: 
                    pdf_place_rule(pos.h - rule_ht, pos.v,           rule_ht, rule_wd);
                    break;
                }
                /* *INDENT-ON* */
                cur.v = cur.v + rule_ht;
            }
            goto NEXTP;
          MOVE_PAST:
            cur.v = cur.v + rule_ht;
          NEXTP:
            p = vlink(p);
        }
    }
    /* Finish vlist {\sl Sync\TeX} information record */
    synctex_tsilv(this_box);

    decr(cur_s);
    dvi_direction = save_direction;
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

void dump_pdftex_data (void)
{
    integer k;
    dumpimagemeta(); /* the image information array */
    dump_int(pdf_mem_size);
    dump_int(pdf_mem_ptr);
    for (k=1;k<=pdf_mem_ptr-1;k++) 
        dump_int(pdf_mem[k]);
    print_ln(); 
    print_int(pdf_mem_ptr-1); 
    tprint(" words of pdf memory");
    dump_int(obj_tab_size);
    dump_int(obj_ptr);
    dump_int(sys_obj_ptr);
    for (k=1;k<=sys_obj_ptr;k++) {
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

void undump_pdftex_data (void)
{
    integer k;
    undumpimagemeta(pdf_minor_version,pdf_inclusion_errorlevel);  /* the image information array */
    undump_int(pdf_mem_size);
    pdf_mem = xreallocarray(pdf_mem, integer, pdf_mem_size);
    undump_int(pdf_mem_ptr);
    for (k=1;k<=pdf_mem_ptr-1;k++)
        undump_int(pdf_mem[k]);
    undump_int(obj_tab_size);
    undump_int(obj_ptr);
    undump_int(sys_obj_ptr);
    for (k=1;k<=sys_obj_ptr;k++) {
        undump_int(obj_info(k));
        undump_int(obj_link(k));
        set_obj_offset(k,-1);
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

