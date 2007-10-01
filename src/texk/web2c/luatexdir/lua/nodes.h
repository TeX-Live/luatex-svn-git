
#include <stdarg.h>

#define max_halfword  0x3FFFFFFF
#define null         -0x3FFFFFFF
#define null_flag    -0x40000000
#define zero_glue 0
#define normal 0

#define vinfo(a)           varmem[(a)].hh.v.LH 
#define node_size(a)       varmem[(a)].hh.v.LH
#define vlink(a)           varmem[(a)].hh.v.RH 
#define type(a)            varmem[(a)].hh.u.B0
#define subtype(a)         varmem[(a)].hh.u.B1
#define node_attr(a)       vinfo((a)+1)
#define alink(a)           vlink((a)+1)

/* really special head node pointers that only need vlink */

#define temp_node_size 1

/* attribute lists */
#define attribute_node_size 2
#define attribute_list_node_size 2

#define attr_list_ref(a)   vinfo((a)+1)
#define attribute_id(a)    vlink((a)+1)
#define attribute_value(a) vinfo((a)+1)

/* a glue spec */
#define glue_spec_size 4
#define stretch(a)        vlink((a)+1)
/* width == a+2 */
#define shrink(a)         vinfo((a)+1)
#define stretch_order(a)  type((a)+3)
#define shrink_order(a)   subtype((a)+3)
#define glue_ref_count(a) vlink((a)+3)

/* pdf action spec */

#define pdf_action_size 4

typedef enum {
  pdf_action_page = 0,
  pdf_action_goto,
  pdf_action_thread,
  pdf_action_user } pdf_action_types;

#define pdf_action_type(a)        type((a) + 1)
#define pdf_action_named_id(a)    subtype((a) + 1)
#define pdf_action_id(a)          vlink((a) + 1)
#define pdf_action_file(a)        vinfo((a) + 2)
#define pdf_action_new_window(a)  vlink((a) + 2)
#define pdf_action_tokens(a)      vinfo((a) + 3)
#define pdf_action_refcount(a)    vlink((a) + 3)

/* normal nodes */

#define penalty_node_size 3 
#define penalty(a)       vlink((a)+2)

#define glue_node_size 3
#define glue_ptr(a)      vinfo((a)+2)
#define leader_ptr(a)    vlink((a)+2)

#define disc_node_size 3
#define replace_count    subtype
#define pre_break(a)     vinfo((a)+2)
#define post_break(a)    vlink((a)+2)

#define kern_node_size 3
#define box_node_size 8
#define width(a)         varmem[(a+2)].cint
#define depth(a)         varmem[(a+3)].cint
#define height(a)        varmem[(a+4)].cint
#define shift_amount(a)  vlink((a)+5)
#define box_dir(a)       vinfo((a)+5)
#define list_ptr(a)      vlink((a)+6)
#define glue_order(a)    subtype((a)+6)
#define glue_sign(a)     type((a)+6)
#define glue_set(a)      varmem[(a+7)].gr

/* unset nodes */
#define glue_stretch(a)  varmem[(a)+7].cint
#define glue_shrink      shift_amount
#define span_count       subtype

#define rule_node_size 6
#define rule_dir(a)      vlink((a)+5)

#define mark_node_size 3
#define mark_ptr(a)      vlink((a)+2)
#define mark_class(a)    vinfo((a)+2)

#define adjust_node_size 3
#define adjust_ptr(a)    vlink(a+2)

#define glyph_node_size 5 /* and ligatures */

#define font(a)         vlink((a)+2)
#define character(a)    vinfo((a)+2)
#define lig_ptr(a)      vlink((a)+3)
#define x_displace(a)   vinfo((a)+4)
#define y_displace(a)   vlink((a)+4)
#define is_char_node(a) (a!=null && type(a)==glyph_node)

#define margin_kern_node_size 4
#define margin_char(a)  vlink((a)+3)

#define math_node_size 3
#define surround(a)      vlink((a)+2)

#define ins_node_size 6
#define float_cost(a)    varmem[(a)+2].cint
#define ins_ptr(a)       vinfo((a)+5)
#define split_top_ptr(a) vlink((a)+5)

typedef enum {
  hlist_node = 0, 
  vlist_node = 1, 
  rule_node,      
  ins_node,       
  mark_node,      
  adjust_node,    
  /* 6 used to be ligatures */
  disc_node=7,     
  whatsit_node,
  math_node,      
  glue_node,      
  kern_node,      
  penalty_node,   
  unset_node,   /* 13 */
  right_noad = 31,
  action_node = 39,
  margin_kern_node = 40,
  glyph_node = 41,
  attribute_node = 42,
  glue_spec_node = 43,
  attribute_list_node = 44,
  last_known_node = 45, /* used by \lastnodetype */
  unhyphenated_node = 50, 
  hyphenated_node = 51,
  delta_node = 52,
  passive_node = 53 } node_types ;

typedef enum {
  open_node = 0,
  write_node,
  close_node,
  special_node,
  language_node,
  set_language_code,
  local_par_node,
  dir_node,
  pdf_literal_node,
  pdf_obj_code,
  pdf_refobj_node, /* 10 */
  pdf_xform_code,
  pdf_refxform_node,
  pdf_ximage_code,
  pdf_refximage_node,
  pdf_annot_node,
  pdf_start_link_node,
  pdf_end_link_node,
  pdf_outline_code,
  pdf_dest_node,
  pdf_thread_node, /* 20 */
  pdf_start_thread_node,
  pdf_end_thread_node,
  pdf_save_pos_node,
  pdf_info_code,
  pdf_catalog_code,
  pdf_names_code,
  pdf_font_attr_code,
  pdf_include_chars_code,
  pdf_map_file_code,
  pdf_map_line_code, /* 30 */
  pdf_trailer_code,
  pdf_font_expand_code,
  set_random_seed_code,
  pdf_glyph_to_unicode_code,
  late_lua_node, /* 35 */
  close_lua_node,
  save_cat_code_table_code,
  init_cat_code_table_code,
  pdf_colorstack_node,
  pdf_setmatrix_node, /* 40 */
  pdf_save_node,
  pdf_restore_node,
  user_defined_node /* 43 */ } whatsit_types ;

#define open_node_size 4
#define write_node_size 3
#define close_node_size 3
#define special_node_size 3
#define language_node_size 4
#define dir_node_size 4

#define dir_dir(a)       vinfo((a)+2)
#define dir_level(a)     vlink((a)+2)
#define dir_dvi_ptr(a)   vinfo((a)+3)
#define dir_dvi_h(a)     vlink((a)+3)
#define what_lang(a)   vlink((a)+2)
#define what_lhm(a)    type((a)+2)
#define what_rhm(a)    subtype((a)+2)
#define write_tokens(a)  vlink(a+2)
#define write_stream(a)  vinfo(a+2)
#define open_name(a)   vlink((a)+2)
#define open_area(a)   vinfo((a)+3)
#define open_ext(a)    vlink((a)+3)

#define late_lua_data(a)        vlink((a)+2)
#define late_lua_reg(a)         vinfo((a)+2)

#define local_par_size 6

#define local_pen_inter(a)       vinfo((a)+2)
#define local_pen_broken(a)      vlink((a)+2)
#define local_box_left(a)        vlink((a)+3)
#define local_box_left_width(a)  vinfo((a)+3)
#define local_box_right(a)       vlink((a)+4)
#define local_box_right_width(a) vinfo((a)+4)
#define local_par_dir(a)         vinfo((a)+5)


#define pdf_literal_data(a)  vlink(a+2)
#define pdf_literal_mode(a)  vinfo(a+2)

#define pdf_refobj_node_size 3

#define pdf_obj_objnum(a)    vinfo((a) + 2)

#define pdf_refxform_node_size  6
#define pdf_refximage_node_size 6
#define pdf_annot_node_size 8
#define pdf_dest_node_size 8
#define pdf_thread_node_size 8

#define pdf_width(a)         varmem[(a) + 2].cint
#define pdf_height(a)        varmem[(a) + 3].cint
#define pdf_depth(a)         varmem[(a) + 4].cint

#define pdf_ximage_objnum(a) vinfo((a) + 5)
#define pdf_xform_objnum(a)  vinfo((a) + 5)

#define pdf_annot_data(a)       vinfo((a) + 6)
#define pdf_link_attr(a)        vinfo((a) + 6)
#define pdf_link_action(a)      vlink((a) + 6)
#define pdf_annot_objnum(a)     varmem[(a) + 7].cint
#define pdf_link_objnum(a)      varmem[(a) + 7].cint

#define pdf_dest_type(a)          type((a) + 6)
#define pdf_dest_named_id(a)      subtype((a) + 6)
#define pdf_dest_id(a)            vlink((a) + 6)
#define pdf_dest_xyz_zoom(a)      vinfo((a) + 7)
#define pdf_dest_objnum(a)        vlink((a) + 7)

#define pdf_thread_named_id(a)    subtype((a) + 6)
#define pdf_thread_id(a)          vlink((a) + 6)
#define pdf_thread_attr(a)        vinfo((a) + 7)

#define pdf_end_link_node_size 3
#define pdf_end_thread_node_size 3
#define pdf_save_pos_node_size 3

#define pdf_colorstack_node_size 4
#define pdf_setmatrix_node_size 3

#define pdf_colorstack_stack(a)  vlink((a)+2)
#define pdf_colorstack_cmd(a)    vinfo((a)+2)
#define pdf_colorstack_data(a)   vlink((a)+3)
#define pdf_setmatrix_data(a)    vlink((a)+2)

#define pdf_save_node_size     3
#define pdf_restore_node_size  3

typedef enum {
  colorstack_set=0,
  colorstack_push,
  colorstack_pop,
  colorstack_current } colorstack_commands;

#define user_defined_node_size 4
#define user_node_type(a)  vinfo((a)+2)
#define user_node_id(a)    vlink((a)+2)
#define user_node_value(a) vinfo((a)+3)

#define NODE_METATABLE "luatex.node"

#define check_isnode(L,b) (halfword *)luaL_checkudata(L,b,NODE_METATABLE)

/* from luanode.c */

extern char * node_names[];
extern char * whatsit_node_names[];
extern halfword lua_node_new(int i, int j);

#define zero_glue       0
#define sfi_glue        zero_glue+glue_spec_size
#define fil_glue        sfi_glue+glue_spec_size
#define fill_glue       fil_glue+glue_spec_size
#define ss_glue         fill_glue+glue_spec_size
#define fil_neg_glue    ss_glue+glue_spec_size
#define page_ins_head   fil_neg_glue+glue_spec_size
#define contrib_head    page_ins_head+temp_node_size
#define page_head       contrib_head+temp_node_size
#define temp_head       page_head+temp_node_size
#define hold_head       temp_head+temp_node_size
#define adjust_head     hold_head+temp_node_size
#define pre_adjust_head adjust_head+temp_node_size
#define active          pre_adjust_head+temp_node_size
#define align_head      active+temp_node_size
#define end_span        align_head+temp_node_size


