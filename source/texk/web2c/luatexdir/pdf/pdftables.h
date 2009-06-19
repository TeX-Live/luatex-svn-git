/* pdftables.h

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

#ifndef PDFTABLES_H
#  define PDFTABLES_H

/*
The cross-reference table |obj_tab| is an array of |obj_tab_size| of
|obj_entry|. Each entry contains five integer fields and represents an object
in PDF file whose object number is the index of this entry in |obj_tab|.
Objects in |obj_tab| maybe linked into list; objects in such a linked list have
the same type.
*/

typedef struct obj_entry {
    integer int0;
    integer int1;
    longinteger int2;
    integer int3;
    integer int4;
} obj_entry;

/*
The first field contains information representing identifier of this object.
It is usally a number for most of object types, but it may be a string number
for named destination or named thread.

The second field of |obj_entry| contains link to the next
object in |obj_tab| if this object is linked in a list.

The third field holds the byte offset of the object in the output PDF file,
or its byte offset within an object stream. As long as the object is not
written, this field is used for flags about the write status of the object;
then it has a negative value.

The fourth field holds the object number of the object stream, into which
the object is included.

The last field usually represents the pointer to some auxiliary data
structure depending on the object type; however it may be used as a counter as
well.
*/

#  define obj_info(A)  obj_tab[(A)].int0/* information representing identifier of this object */
#  define obj_link(A)  obj_tab[(A)].int1/* link to the next entry in linked list */
#  define obj_offset(A) obj_tab[(A)].int2
                                        /* negative (flags), or byte offset for this object in PDF 
                                           output file, or object stream number for this object */
#  define obj_os_idx(A) obj_tab[(A)].int3
                                        /* index of this object in object stream */
#  define obj_aux(A)  obj_tab[(A)].int4 /* auxiliary pointer */

#  define set_obj_fresh(A)      obj_offset((A))=-2
#  define set_obj_scheduled(A)  if (intcast(obj_offset(A))==-2) obj_offset(A)=-1
#  define is_obj_scheduled(A)  (intcast(obj_offset(A))>-2)
#  define is_obj_written(A)    (intcast(obj_offset(A))>-1)

/* types of objects */
typedef enum {
    obj_type_others = 0,        /* objects which are not linked in any list */
    obj_type_page = 1,          /* index of linked list of Page objects */
    obj_type_font = 2,          /* index of linked list of Fonts objects */
    obj_type_outline = 3,       /* index of linked list of outline objects */
    obj_type_dest = 4,          /* index of linked list of destination objects */
    obj_type_obj = 5,           /* index of linked list of raw objects */
    obj_type_xform = 6,         /* index of linked list of XObject forms */
    obj_type_ximage = 7,        /* index of linked list of XObject image */
    obj_type_thread = 8         /* index of linked list of num article threads */
} pdf_obj_types;

#  define head_tab_max obj_type_thread  /* max index of |head_tab| */

/* max number of kids for balanced trees */
#  define name_tree_kids_max 6  /* max number of kids of node of name tree for name destinations */

/*  NOTE: The data structure definitions for the nodes on the typesetting side are 
    inside |nodes.h| */

/* data structure for \.{\\pdfobj} and \.{\\pdfrefobj} */

#  define obj_data_ptr             obj_aux      /* pointer to |pdf_mem| */
#  define pdfmem_obj_size          4    /* size of memory in |pdf_mem| which |obj_data_ptr| holds */

#  define obj_obj_data(A)          pdf_mem[obj_data_ptr(A) + 0] /* object data */
#  define obj_obj_is_stream(A)     pdf_mem[obj_data_ptr(A) + 1] /* will this object
                                                                   be written as a stream instead of a dictionary? */
#  define obj_obj_stream_attr(A)   pdf_mem[obj_data_ptr(A) + 2] /* additional
                                                                   object attributes for streams */
#  define obj_obj_is_file(A)       pdf_mem[obj_data_ptr(A) + 3] /* data should be
                                                                   read from an external file? */

/* data structure for \.{\\pdfxform} and \.{\\pdfrefxform} */

#  define pdfmem_xform_size        6    /* size of memory in |pdf_mem| which |obj_data_ptr| holds */

#  define obj_xform_width(A)       pdf_mem[obj_data_ptr(A) + 0]
#  define obj_xform_height(A)      pdf_mem[obj_data_ptr(A) + 1]
#  define obj_xform_depth(A)       pdf_mem[obj_data_ptr(A) + 2]
#  define obj_xform_box(A)         pdf_mem[obj_data_ptr(A) + 3] /* this field holds
                                                                   pointer to the corresponding box */
#  define obj_xform_attr(A)        pdf_mem[obj_data_ptr(A) + 4] /* additional xform
                                                                   attributes */
#  define obj_xform_resources(A)   pdf_mem[obj_data_ptr(A) + 5] /* additional xform
                                                                   Resources */

/* data structure of annotations; words 1..4 represent the coordinates of the annotation */

#  define obj_annot_ptr            obj_aux      /* pointer to corresponding whatsit node */

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

/* types of destinations */

typedef enum {
    pdf_dest_xyz = 0,
    pdf_dest_fit = 1,
    pdf_dest_fith = 2,
    pdf_dest_fitv = 3,
    pdf_dest_fitb = 4,
    pdf_dest_fitbh = 5,
    pdf_dest_fitbv = 6,
    pdf_dest_fitr = 7
} pdf_destination_types;

/* data structure of destinations */

#  define obj_dest_ptr              obj_aux     /* pointer to |pdf_dest_node| */

/* data structure of threads; words 1..4 represent the coordinates of the  corners */

#  define obj_thread_first           obj_aux    /* pointer to the first bead */

/* data structure of beads */
#  define pdfmem_bead_size          5   /* size of memory in |pdf_mem| which  |obj_bead_ptr| points to */

#  define obj_bead_ptr              obj_aux     /* pointer to |pdf_mem| */
#  define obj_bead_rect(A)          pdf_mem[obj_bead_ptr(A)]
#  define obj_bead_page(A)          pdf_mem[obj_bead_ptr(A) + 1]
#  define obj_bead_next(A)          pdf_mem[obj_bead_ptr(A) + 2]
#  define obj_bead_prev(A)          pdf_mem[obj_bead_ptr(A) + 3]
#  define obj_bead_attr(A)          pdf_mem[obj_bead_ptr(A) + 4]

/* pointer to the corresponding whatsit node; |obj_bead_rect| is needed only when the bead
   rectangle has been written out and after that |obj_bead_data| is not needed any more
   so we can use this field for both */
#  define obj_bead_data             obj_bead_rect


/* Some constants */
#  define inf_obj_tab_size 1000 /* min size of the cross-reference table for PDF output */
#  define sup_obj_tab_size 8388607      /* max size of the cross-reference table for PDF output */
#  define inf_dest_names_size 1000      /* min size of the destination names table for PDF output */
#  define sup_dest_names_size 131072    /* max size of the destination names table for PDF output */
#  define inf_pk_dpi 72         /* min PK pixel density value from \.{texmf.cnf} */
#  define sup_pk_dpi 8000       /* max PK pixel density value from \.{texmf.cnf} */
#  define pdf_objtype_max head_tab_max

extern integer obj_tab_size;
extern obj_entry *obj_tab;
extern integer head_tab[(head_tab_max + 1)];
extern integer pages_tail;
extern integer obj_ptr;
extern integer sys_obj_ptr;
extern integer pdf_last_pages;
extern integer pdf_last_page;
extern integer pdf_last_stream;
extern longinteger pdf_stream_length;
extern longinteger pdf_stream_length_offset;
extern boolean pdf_seek_write_length;
extern integer pdf_last_byte;
extern integer pdf_box_spec_media;
extern integer pdf_box_spec_crop;
extern integer pdf_box_spec_bleed;
extern integer pdf_box_spec_trim;
extern integer pdf_box_spec_art;
extern integer dest_names_size;

#  define set_ff(A)  do {                       \
        if (pdf_font_num(A) < 0)                \
            ff = -pdf_font_num(A);              \
        else                                    \
            ff = A;                             \
    } while (0)

extern integer ff;

extern void append_dest_name(str_number s, integer n);
extern void pdf_create_obj(integer t, integer i);
extern integer pdf_new_objnum(void);
extern void pdf_os_switch(boolean pdf_os);
extern void pdf_os_prepare_obj(integer i, integer pdf_os_level);
extern void pdf_begin_obj(integer i, integer pdf_os_level);
extern void pdf_new_obj(integer t, integer i, integer pdf_os);
extern void pdf_end_obj(void);
extern void pdf_begin_dict(integer i, integer pdf_os_level);
extern void pdf_new_dict(integer t, integer i, integer pdf_os);
extern void pdf_end_dict(void);

extern void pdf_os_write_objstream(void);
extern halfword append_ptr(halfword p, integer i);
extern halfword pdf_lookup_list(halfword p, integer i);

extern void pdf_print_fw_int(longinteger n, integer w);
extern void pdf_out_bytes(longinteger n, integer w);
extern void pdf_int_entry(str_number s, integer v);
extern void pdf_int_entry_ln(str_number s, integer v);
extern void pdf_indirect(str_number s, integer o);
extern void pdf_indirect_ln(str_number s, integer o);
extern void pdf_print_str_ln(str_number s);
extern void pdf_str_entry(str_number s, str_number v);
extern void pdf_str_entry_ln(str_number s, str_number v);

extern void pdf_out_literal(halfword p);
extern void pdf_out_colorstack(halfword p);
extern void pdf_out_colorstack_startpage(void);
extern void pdf_out_setmatrix(halfword p);
extern void pdf_out_save(void);
extern void pdf_out_restore(void);
extern void pdf_special(halfword p);
extern void pdf_print_toks(halfword p);
extern void pdf_print_toks_ln(halfword p);

extern void output_form(halfword p);
extern void output_image(integer idx);

/* a few small helpers */
#  define pos_right(A) pos.h = pos.h + (A)
#  define pos_left(A)  pos.h = pos.h - (A)
#  define pos_up(A)    pos.v = pos.v + (A)
#  define pos_down(A)  pos.v = pos.v - (A)

extern scaled simple_advance_width(halfword p);
extern void calculate_width_to_enddir(halfword p, real cur_glue, scaled cur_g,
                                      halfword this_box, scaled * setw,
                                      halfword * settemp_ptr);

extern void pdf_write_image(integer n);
extern void pdf_write_obj(integer n);

extern void pdf_print_rect_spec(halfword r);
extern void pdf_rectangle(halfword r);

extern void write_action(halfword p);
extern void set_rect_dimens(halfword p, halfword parent_box,
                            scaled x, scaled y, scaled wd, scaled ht, scaled dp,
                            scaled margin);
extern void do_annot(halfword p, halfword parent_box, scaled x, scaled y);
extern void do_dest(halfword p, halfword parent_box, scaled x, scaled y);

extern void pdf_hlist_out(void);
extern void pdf_vlist_out(void);
extern void write_out_pdf_mark_destinations(void);

#  define pdf_max_link_level  10/* maximum depth of link nesting */

typedef struct pdf_link_stack_record {
    integer nesting_level;
    halfword link_node;         /* holds a copy of the corresponding |pdf_start_link_node| */
    halfword ref_link_node;     /* points to original |pdf_start_link_node|, or a
                                   copy of |link_node| created by |append_link| in
                                   case of multi-line link */
} pdf_link_stack_record;

extern pdf_link_stack_record pdf_link_stack[(pdf_max_link_level + 1)];
extern integer pdf_link_stack_ptr;

extern void push_link_level(halfword p);
extern void pop_link_level(void);
extern void warn_dest_dup(integer id, small_number byname, str_number s1,
                          str_number s2);
extern void do_link(halfword p, halfword parent_box, scaled x, scaled y);
extern void end_link(void);
extern void append_link(halfword parent_box, scaled x, scaled y,
                        small_number i);

extern void append_bead(halfword p);
extern void do_thread(halfword parent_box, halfword p, scaled x, scaled y);
extern void append_thread(halfword parent_box, scaled x, scaled y);
extern void end_thread(void);
extern integer open_subentries(halfword p);

/* interface definitions for eqtb locations */

#  define pdf_minor_version        int_par(param_pdf_minor_version_code)
#  define pdf_decimal_digits       int_par(param_pdf_decimal_digits_code)
#  define pdf_gamma                int_par(param_pdf_gamma_code)
#  define pdf_image_gamma          int_par(param_pdf_image_gamma_code)
#  define pdf_image_hicolor        int_par(param_pdf_image_hicolor_code)
#  define pdf_image_apply_gamma    int_par(param_pdf_image_apply_gamma_code)
#  define pdf_objcompresslevel     int_par(param_pdf_objcompresslevel_code)
#  define pdf_draftmode            int_par(param_pdf_draftmode_code)
#  define pdf_inclusion_copy_font  int_par(param_pdf_inclusion_copy_font_code)
#  define pdf_inclusion_errorlevel int_par(param_pdf_inclusion_errorlevel_code)
#  define pdf_replace_font         int_par(param_pdf_replace_font_code)
#  define pdf_pk_resolution        int_par(param_pdf_pk_resolution_code)
#  define pdf_pk_mode              int_par(param_pdf_pk_mode_code)
#  define pdf_unique_resname       int_par(param_pdf_unique_resname_code)
#  define pdf_compress_level       int_par(param_pdf_compress_level_code)
#  define pdf_move_chars           int_par(param_pdf_move_chars_code)
#  define pdf_thread_margin        dimen_par(param_pdf_thread_margin_code)
#  define pdf_link_margin          dimen_par(param_pdf_link_margin_code)
#  define pdf_dest_margin          dimen_par(param_pdf_dest_margin_code)

#  define set_obj_data_ptr(A,B) obj_data_ptr(A)=B
#  define set_pdf_action_type(A,B) pdf_action_type(A)=B
#  define set_pdf_action_tokens(A,B) pdf_action_tokens(A)=B
#  define set_pdf_action_file(A,B) pdf_action_file(A)=B
#  define set_pdf_action_id(A,B) pdf_action_id(A)=B
#  define set_pdf_action_named_id(A,B) pdf_action_named_id(A)=B
#  define set_pdf_action_new_window(A,B) pdf_action_new_window(A)=B
#  define set_pdf_width(A,B) pdf_width(A)=B
#  define set_pdf_height(A,B) pdf_height(A)=B
#  define set_pdf_depth(A,B) pdf_depth(A)=B
#  define set_pdf_thread_attr(A,B) pdf_thread_attr(A)=B
#  define set_pdf_thread_id(A,B) pdf_thread_id(A)=B
#  define set_pdf_thread_named_id(A,B) pdf_thread_named_id(A)=B
#  define set_pdf_annot_objnum(A,B) pdf_annot_objnum(A)=B
#  define set_pdf_annot_data(A,B) pdf_annot_data(A)=B
#  define set_pdf_dest_id(A,B) pdf_dest_id(A)=B
#  define set_pdf_dest_named_id(A,B) pdf_dest_named_id(A)=B
#  define set_pdf_dest_type(A,B) pdf_dest_type(A)=B
#  define set_pdf_dest_xyz_zoom(A,B) pdf_dest_xyz_zoom(A)=B
#  define set_pdf_literal_mode(A,B) pdf_literal_mode(A)=B
#  define set_pdf_literal_type(A,B) pdf_literal_type(A)=B
#  define set_pdf_literal_data(A,B) pdf_literal_data(A)=B
#  define set_pdf_colorstack_stack(A,B) pdf_colorstack_stack(A)=B
#  define set_pdf_colorstack_cmd(A,B) pdf_colorstack_cmd(A)=B
#  define set_pdf_colorstack_data(A,B) pdf_colorstack_data(A)=B
#  define set_pdf_setmatrix_data(A,B) pdf_setmatrix_data(A)=B
#  define set_obj_obj_is_stream(A,B) obj_obj_is_stream(A)=B
#  define set_obj_obj_stream_attr(A,B) obj_obj_stream_attr(A)=B
#  define set_obj_obj_is_file(A,B) obj_obj_is_file(A)=B
#  define set_obj_obj_data(A,B) obj_obj_data(A)=B
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
#  define set_pdf_obj_objnum(A,B) pdf_obj_objnum(A)=B
#  define set_pdf_xform_objnum(A,B) pdf_xform_objnum(A)=B
#  define set_pdf_ximage_idx(A,B) pdf_ximage_idx(A)=B
#  define set_pdf_link_attr(A,B) pdf_link_attr(A)=B
#  define set_pdf_link_action(A,B) pdf_link_action(A)=B
#  define set_pdf_link_objnum(A,B) pdf_link_objnum(A)=B
#  define set_obj_xform_width(A,B) obj_xform_width(A)=B
#  define set_obj_xform_height(A,B) obj_xform_height(A)=B
#  define set_obj_xform_depth(A,B) obj_xform_depth(A)=B
#  define set_obj_xform_box(A,B) obj_xform_box(A)=B
#  define set_obj_link(A,B) obj_link(A)=B
#  define set_obj_info(A,B) obj_info(A)=B

#  define set_obj_offset(A,B) obj_offset(A)=B
#  define set_obj_dest_ptr(A,B) obj_dest_ptr(A)=B
#  define set_obj_xform_attr(A,B) obj_xform_attr(A)=B
#  define set_obj_aux(A,B) obj_aux(A)=B
#  define set_obj_bead_rect(A,B) obj_bead_rect(A)=B
#  define set_obj_xform_resources(A,B) obj_xform_resources(A)=B
#endif

void dump_pdftex_data (void);
void undump_pdftex_data (void);

