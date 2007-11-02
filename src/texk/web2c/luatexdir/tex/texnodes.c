/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"

#define MAX_CHAIN_SIZE 12

memory_word *varmem = NULL;

#ifndef NDEBUG
signed char *varmem_sizes = NULL;
#endif

halfword var_mem_max = 0;
halfword rover = 0;

halfword free_chain[MAX_CHAIN_SIZE] = {null};

static int prealloc=0;

void check_node_mem(void ) ;

#undef link /* defined by cpascal.h */
#define info(a)    fixmem[(a)].hhlh
#define link(a)    fixmem[(a)].hhrh

#define fake_node 100
#define fake_node_size 2
#define fake_node_name ""

node_info node_data[] = {
  { hlist_node,           box_node_size,             "hlist"          },
  { vlist_node,           box_node_size,             "vlist"          },
  { rule_node,            rule_node_size,            "rule"           },
  { ins_node,             ins_node_size,             "ins"            },
  { mark_node,            mark_node_size,            "mark"           },
  { adjust_node,          adjust_node_size,          "adjust"         },
  { fake_node,            fake_node_size,             fake_node_name  }, /* don't touch this! */
  { disc_node,            disc_node_size,            "disc"           },
  { whatsit_node,         -1,                        "whatsit"        },
  { math_node,            math_node_size,            "math"           },
  { glue_node,            glue_node_size,            "glue"           },
  { kern_node,            kern_node_size,            "kern"           },
  { penalty_node,         penalty_node_size,         "penalty"        },
  { unset_node,           box_node_size,             "unset"          },
  { style_node,           style_node_size,           "style"          },
  { choice_node,          style_node_size,           "choice"         },
  { ord_noad,             noad_size,                 "ord"            },
  { op_noad,              noad_size,                 "op"             },
  { bin_noad,             noad_size,                 "bin"            },
  { rel_noad,             noad_size,                 "rel"            },
  { open_noad,            noad_size,                 "open"           },
  { close_noad,           noad_size,                 "close"          },
  { punct_noad,           noad_size,                 "punct"          },
  { inner_noad,           noad_size,                 "inner"          },
  { radical_noad,         radical_noad_size,         "radical"        },
  { fraction_noad,        fraction_noad_size,        "fraction"       },
  { under_noad,           noad_size,                 "under"          },
  { over_noad,            noad_size,                 "over"           },
  { accent_noad,          accent_noad_size,          "accent"         },
  { vcenter_noad,         noad_size,                 "vcenter"        },
  { left_noad,            noad_size,                 "left"           },
  { right_noad,           noad_size,                 "right"          },
  { margin_kern_node,     margin_kern_node_size,     "margin_kern"    },
  { glyph_node,           glyph_node_size,           "glyph"          },
  { align_record_node,    box_node_size,             "align_record"   },
  { pseudo_file_node,     pseudo_file_node_size,     "pseudo_file"    },
  { pseudo_line_node,     fake_node_size,            "pseudo_line"    },
  { fake_node,            fake_node_size,             fake_node_name  },
  { fake_node,            fake_node_size,             fake_node_name  },
  { fake_node,            fake_node_size,             fake_node_name  },
  { nesting_node,         nesting_node_size,         "nested_list"    },
  { span_node,            span_node_size,            "span"           },
  { attribute_node,       attribute_node_size,       "attribute"      },
  { glue_spec_node,       glue_spec_size,            "glue_spec"      },
  { attribute_list_node,  attribute_list_node_size,  "attribute_list" },
  { action_node,          pdf_action_size,           "action"         },
  { temp_node,            temp_node_size,            "temp"           },
  { align_stack_node,     align_stack_node_size,     "alignstack"     },
  { movement_node,        movement_node_size,        "movement"       },
  { if_node,              if_node_size,              "ifstack"        },
  { unhyphenated_node,    active_node_size,          "unhyphenated"   },
  { hyphenated_node,      active_node_size,          "hyphenated"     },
  { delta_node,           delta_node_size,           "delta"          },
  { passive_node,         passive_node_size,         "passive"        },
  { -1,                   -1,                        NULL             }};

#define last_normal_node passive_node
 
node_info whatsit_node_data[] = {
  { open_node,             open_node_size,             "open"            },
  { write_node,            write_node_size,            "write"           },
  { close_node,            close_node_size,            "close"           },
  { special_node,          special_node_size,          "special"         },
  { fake_node,             fake_node_size,             fake_node_name    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { local_par_node,        local_par_size,             "local_par"       },
  { dir_node,              dir_node_size,              "dir"             },
  { pdf_literal_node,      write_node_size,            "pdf_literal"     },
  { fake_node,             fake_node_size,             fake_node_name    },
  { pdf_refobj_node,       pdf_refobj_node_size,       "pdf_refobj"      },
  { fake_node,             fake_node_size,             fake_node_name    },
  { pdf_refxform_node,     pdf_refxform_node_size,     "pdf_refxform"    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { pdf_refximage_node,    pdf_refximage_node_size,    "pdf_refximage"   },
  { pdf_annot_node,        pdf_annot_node_size,        "pdf_annot"       },
  { pdf_start_link_node,   pdf_annot_node_size,        "pdf_start_link"  },
  { pdf_end_link_node,     pdf_end_link_node_size,     "pdf_end_link"    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { pdf_dest_node,         pdf_dest_node_size,         "pdf_dest"        },
  { pdf_thread_node,       pdf_thread_node_size,       "pdf_thread"      },
  { pdf_start_thread_node, pdf_thread_node_size,       "pdf_start_thread"},
  { pdf_end_thread_node,   pdf_end_thread_node_size,   "pdf_end_thread"  },
  { pdf_save_pos_node,     pdf_save_pos_node_size,     "pdf_save_pos"    },
  { pdf_thread_data_node,  pdf_thread_node_size,       "pdf_thread_data" },
  { pdf_link_data_node,    pdf_annot_node_size,        "pdf_link_data"   },
  { fake_node,             fake_node_size,             fake_node_name    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { late_lua_node,         write_node_size,            "late_lua"        },
  { close_lua_node,        write_node_size,            "close_lua"       },
  { fake_node,             fake_node_size,             fake_node_name    },
  { fake_node,             fake_node_size,             fake_node_name    },
  { pdf_colorstack_node,   pdf_colorstack_node_size,   "pdf_colorstack"  },
  { pdf_setmatrix_node,    pdf_setmatrix_node_size,    "pdf_setmatrix"   },
  { pdf_save_node,         pdf_save_node_size,         "pdf_save"        },
  { pdf_restore_node,      pdf_restore_node_size,      "pdf_restore"     },
  { cancel_boundary_node,  cancel_boundary_size,       "cancel_boundary" },
  { user_defined_node,     user_defined_node_size,     "user_defined"    },
  { -1,                    -1,                         NULL              }};

#define  get_node_size(i,j) (i!=whatsit_node ? node_data[i].size : whatsit_node_data[j].size)

halfword
new_node(int i, int j) {
  register halfword n;

  if (i==pseudo_line_node) {
    n = get_node(j);
    type(n)=i;
    subtype(n)=j;
    return n;
  } 
  n = get_node(get_node_size(i,j));
  /*
  if (get_node_size(i,j)==4)
	fprintf(stdout,"new_node(%d), %d,%d\n",n,i,j);
  */
  type(n)=i;
  subtype(n)=j;  
  if (i==glyph_node) {
    init_lang_data(n);
    return n;
  } else {
	switch (i) {
	case whatsit_node:  
	  if (j==open_node) {
	    open_name(n) = get_nullstr();
	    open_area(n) = open_name(n);
	    open_ext(n) = open_name(n);
	  }
	  break;
	case glue_node:
	case kern_node:
	  break;
	case hlist_node:
	case vlist_node:
	  box_dir(n)=-1; 
	  break;
	case rule_node:
	  width(n)=null_flag; 
	  depth(n)=null_flag; 
	  height(n)=null_flag;
	  rule_dir(n)=-1;
	  break;
	case disc_node: 
	  pre_break(n)=pre_break_head(n); 
	  type(pre_break(n))=255; 
	  subtype(pre_break(n))=pre_break_head(0);
	  post_break(n)=post_break_head(n);
	  type(post_break(n))=255;  
	  subtype(post_break(n))=post_break_head(0);
	  no_break(n)=no_break_head(n);
	  type(no_break(n))=255;  
	  subtype(no_break(n))=no_break_head(0);
	  break;
	case unset_node:        
	  width(n) = null_flag;
	  break; 
	default: 
	  break;
	}  
  }
  return n;
}

/* makes a duplicate of the node list that starts at |p| and returns a
   pointer to the new list */

halfword 
copy_node_list(halfword p) { 
  halfword q; /* previous position in new list */
  halfword h = null; /* head of the list */
  while (p!=null) {
    halfword s = copy_node(p);
    if (h==null) {
      h = s;
    } else {
      vlink(q)= s;
    }
    q=s; 
    p=vlink(p);
  }
  return h;
}

 /* make a dupe of a single node */
halfword 
copy_node(const halfword p) {
  halfword r; /* current node being fabricated for new list */
  halfword s; /* a helper variable for copying into variable mem  */
  int i = get_node_size(type(p), subtype(p));
  if (i<=0) {
	confusion(maketexstring("copying"));
	return null;
  }
  r = get_node(i);
  (void)memcpy((varmem+r), (varmem+p), (sizeof(memory_word)*i));
  if (nodetype_has_attributes(p)) {
	add_node_attr_ref(node_attr(p)); 
  }
  vlink(r)=null;
  alink(r)=null;
  switch (type(p)) {
  case glyph_node:  
	s=copy_node_list(lig_ptr(p));
	lig_ptr(r)=s;
	break;
  case hlist_node:
  case vlist_node:
  case unset_node:  
	s=copy_node_list(list_ptr(p));
	list_ptr(r)=s;
	break;
  case ins_node:  
	add_glue_ref(split_top_ptr(p)); 
	s=copy_node_list(ins_ptr(p));
	ins_ptr(r)=s;
	break;
  case whatsit_node:
	switch (subtype(p)) {
	case open_node:  
	case close_node:  
	case dir_node:  
	case pdf_save_node:  
	case pdf_restore_node:  
	case cancel_boundary_node:  
	case close_lua_node:  
	case pdf_refobj_node:   
	case pdf_refxform_node:  
	case pdf_refximage_node:  
	case pdf_end_link_node:
	case pdf_end_thread_node:  
	case pdf_save_pos_node:  
	case local_par_node:  
	  break;
	case write_node:
	case special_node:  
	  add_token_ref(write_tokens(p)); 
	  break;
	case pdf_literal_node:  
	  add_token_ref(pdf_literal_data(p)); 
	  break;
	case pdf_colorstack_node: 
	  if (pdf_colorstack_cmd(p) <= colorstack_data)
		add_token_ref(pdf_colorstack_data(p));
	  break;
	case pdf_setmatrix_node:
	  add_token_ref(pdf_setmatrix_data(p));
	  break;
	case late_lua_node:  
	  add_token_ref(late_lua_data(p));
	  break;
	case pdf_annot_node:  
	  add_token_ref(pdf_annot_data(p));
	  break;
	case pdf_start_link_node:  
	  if (pdf_link_attr(r) != null)
		add_token_ref(pdf_link_attr(r));
	  add_action_ref(pdf_link_action(r));
	  break;
	case pdf_dest_node:  
	  if (pdf_dest_named_id(p) > 0)
		add_token_ref(pdf_dest_id(p));
	  break;
	case pdf_thread_node:
	case pdf_start_thread_node:
	  if (pdf_thread_named_id(p) > 0)
		add_token_ref(pdf_thread_id(p));
	  if (pdf_thread_attr(p) != null)
		add_token_ref(pdf_thread_attr(p));
	  break;
	case user_defined_node:  
	  switch (user_node_type(p)) {
	  case 'a': add_node_attr_ref(user_node_value(p));
		break;
	  case 't': add_token_ref(user_node_value(p));
		break;
	  case 'n':  
		s=copy_node_list(user_node_value(p)); 
		user_node_value(r)=s; 
		break;
	  }
	  break;
	default: 
	  confusion(maketexstring("ext2"));
	  break;
	}
	break;
  case glue_node:  
	add_glue_ref(glue_ptr(p));
	s=copy_node_list(leader_ptr(p));
	leader_ptr(r)=s;
	break;
  case kern_node:  
  case math_node: 
  case penalty_node:  
  case rule_node:  
	break;
  case margin_kern_node:  
	s=copy_node(margin_char(p)); 
	margin_char(r)=s;
	break;
  case disc_node:
	pre_break(r)=pre_break_head(r); 
	if (vlink(pre_break(p))!=null) {
	  s=copy_node_list(vlink(pre_break(p)));
	  alink(s)=pre_break(r);
	  tlink(pre_break(r))=tail_of_list(s); 
	  vlink(pre_break(r))=s;
	} else {
	  assert(tlink(pre_break(r))==null);
	}
	post_break(r)=post_break_head(r);
	if (vlink(post_break(p))!=null) {
	  s=copy_node_list(vlink(post_break(p)));
	  alink(s)=post_break(r);
	  tlink(post_break(r))=tail_of_list(s); 
	  vlink(post_break(r))=s;
	} else {
	  assert(tlink(post_break(r))==null);
	}
	no_break(r)=no_break_head(r);
	if (vlink(no_break(p))!=null) {
	  s=copy_node_list(vlink(no_break(p)));
	  alink(s)=no_break(r);
	  tlink(no_break(r))=tail_of_list(s); 
	  vlink(no_break(r))=s;
	} else {
	  assert(tlink(no_break(r))==null);
    }
	break;
  case mark_node: 
	add_token_ref(mark_ptr(p));
	break;
  case adjust_node:  
	s=copy_node_list(adjust_ptr(p)); 
	adjust_ptr(r)=s;
	break;
  case attribute_list_node:  
  case attribute_node:  
	break;
  case glue_spec_node: 
	glue_ref_count(r)=null;
	break;
  }
  return r;
}

void 
flush_node (halfword p) {
  /*check_node_mem();*/
  assert(p>prealloc);
  assert(p<var_mem_max);
  assert(varmem_sizes[p]>0);

  switch(type(p)) {
  case pseudo_file_node:
    flush_node_list(pseudo_lines(p));
    break;
  case pseudo_line_node:
    free_node(p,subtype(p));
    return;
    break;
  case glyph_node:
    flush_node_list(lig_ptr(p));
    break;
  case attribute_node:
  case attribute_list_node:
    break;
  case glue_node: 
    delete_glue_ref(glue_ptr(p));
    flush_node_list(leader_ptr(p));
    break;
  case hlist_node:
  case vlist_node:
  case unset_node:
    flush_node_list(list_ptr(p));
    break;
  case ins_node:
    flush_node_list(ins_ptr(p));
    delete_glue_ref(split_top_ptr(p));
    break;
  case whatsit_node: 
    switch (subtype(p)) {
    case dir_node: 
    case open_node: 
    case write_node:
    case close_node: 
    case pdf_save_node: 
    case pdf_restore_node: 
    case cancel_boundary_node: 
    case close_lua_node: 
    case pdf_refobj_node:
    case pdf_refxform_node:
    case pdf_refximage_node:
    case pdf_end_link_node:
    case pdf_end_thread_node:
    case pdf_save_pos_node:
    case local_par_node: 
      break;
    case special_node: 
      delete_token_ref(write_tokens(p));
      break;
    case pdf_literal_node:
      delete_token_ref(pdf_literal_data(p));
      break;
    case pdf_colorstack_node: 
      if (pdf_colorstack_cmd(p) <= colorstack_data)
	delete_token_ref(pdf_colorstack_data(p));
      break;
    case pdf_setmatrix_node: 
      delete_token_ref(pdf_setmatrix_data(p));
      break;
    case late_lua_node:
      delete_token_ref(late_lua_data(p));
      break;
    case pdf_annot_node:
      delete_token_ref(pdf_annot_data(p));
      break;
    case pdf_link_data_node:
      break;
    case pdf_start_link_node: 
      if (pdf_link_attr(p) != null)
	delete_token_ref(pdf_link_attr(p));
      delete_action_ref(pdf_link_action(p));
      break;
    case pdf_dest_node: 
      if (pdf_dest_named_id(p) > 0)
	delete_token_ref(pdf_dest_id(p));
      break;
    case pdf_thread_data_node:
      break;
    case pdf_thread_node:
    case pdf_start_thread_node: 
      if (pdf_thread_named_id(p) > 0 ) delete_token_ref(pdf_thread_id(p));
      if (pdf_thread_attr(p) != null)  delete_token_ref(pdf_thread_attr(p));
      break;
    case user_defined_node: 
      switch( user_node_type(p)) {
      case 'a': delete_attribute_ref(user_node_value(p)); break;
      case 't': delete_token_ref(user_node_value(p));  break;
      case 'n': flush_node_list(user_node_value(p));  break;
      default:  confusion(maketexstring("extuser")); break;
      }
      break;
    default:
      confusion(maketexstring("ext3"));
      return;
    }
    break;
  case glue_spec_node:
  case rule_node: 
  case kern_node:
  case math_node:
  case penalty_node: 
    break;
  case margin_kern_node:
    flush_node(margin_char(p));
    break;
  case mark_node:
    delete_token_ref(mark_ptr(p)); 
    break;
  case disc_node:
    flush_node_list(vlink(pre_break(p)));
    flush_node_list(vlink(post_break(p)));
    flush_node_list(vlink(no_break(p)));
    break;
  case adjust_node:
    flush_node_list(adjust_ptr(p)); 
    break;
  case style_node:  
    break;
  case choice_node: 
    flush_node_list(display_mlist(p));
    flush_node_list(text_mlist(p));
    flush_node_list(script_mlist(p));
    flush_node_list(script_script_mlist(p));
    break;
  case ord_noad:
  case op_noad:
  case bin_noad:
  case rel_noad:
  case open_noad:
  case close_noad:
  case punct_noad:
  case inner_noad:
  case radical_noad:
  case over_noad:
  case under_noad:
  case vcenter_noad:
  case accent_noad:
    if (0) {
      if (math_type(nucleus(p))>=sub_box)
	flush_node_list(vinfo(nucleus(p)));
      if (math_type(supscr(p))>=sub_box)
	flush_node_list(vinfo(supscr(p)));
      if (math_type(subscr(p))>=sub_box)
	flush_node_list(vinfo(subscr(p)));
    }
    break;
  case left_noad:
  case right_noad: 
    break;
  case fraction_noad:  
    if (0) {
      flush_node_list(vinfo(numerator(p)));
      flush_node_list(vinfo(denominator(p)));
    }
    break;
  case temp_node:
  case align_stack_node:
  case movement_node: 
  case if_node:
  case nesting_node:
  case unhyphenated_node:
  case hyphenated_node:
  case delta_node:
  case passive_node:
  case action_node: 
    break;
  default:
    fprintf(stdout,"flush_node: type is %d\n", type(p)); 
    return;
  }
  if (nodetype_has_attributes(type(p)))
    delete_attribute_ref(node_attr(p));
  free_node(p,get_node_size(type(p),subtype(p)));
  return;
}

void 
flush_node_list(halfword pp) { /* erase list of nodes starting at |p| */
  halfword q;
  halfword p=pp; /*to prevent trashing of the argument pointer, for debugging*/
  while (p!=null) {
    q = vlink(p);
    flush_node(p);
    p=q;
  }
}

static int test_count = 1;

#define dorangetest(a,b,c) {						\
    if (!(b>=0 && b<c)) {						\
      fprintf(stdout,"For node p:=%d, 0<=%d<%d (l.%d,r.%d)\n",		\
	      (int)a, (int)b, (int)c, __LINE__,test_count);		\
      confusion(maketexstring("dorangetest"));				\
    }									\
  }

#define dotest(a,b,c) {							\
    if (b!=c) {								\
      fprintf(stdout,"For node p:=%d, %d==%d (l.%d,r.%d)\n",		\
	      (int)a, (int)b, (int)c, __LINE__,test_count);		\
      confusion(maketexstring("dotest"));				\
    }									\
  }

#define check_action_ref(a)     { dorangetest(p,a,var_mem_max); }
#define check_glue_ref(a)       { dorangetest(p,a,var_mem_max); }
#define check_attribute_ref(a)  { dorangetest(p,a,var_mem_max); }
#define check_token_ref(a)      

void 
check_node (halfword p) {

  switch(type(p)) {
  case pseudo_file_node:
    dorangetest(p,pseudo_lines(p),var_mem_max);
    break;
  case pseudo_line_node:
    break;
  case glyph_node:
    dorangetest(p,lig_ptr(p),var_mem_max);
    break;
  case glue_node: 
    check_glue_ref(glue_ptr(p));
    dorangetest(p,leader_ptr(p),var_mem_max);
    break;
  case hlist_node:
  case vlist_node:
  case unset_node:
  case align_record_node:
    dorangetest(p,list_ptr(p),var_mem_max);
    break;
  case ins_node:
    dorangetest(p,ins_ptr(p),var_mem_max);
    check_glue_ref(split_top_ptr(p));
    break;
  case whatsit_node: 
    switch (subtype(p)) {
    case dir_node: 
    case open_node: 
    case write_node:
    case close_node: 
    case pdf_save_node: 
    case pdf_restore_node: 
    case cancel_boundary_node: 
    case close_lua_node: 
    case pdf_refobj_node:
    case pdf_refxform_node:
    case pdf_refximage_node:
    case pdf_end_link_node:
    case pdf_end_thread_node:
    case pdf_save_pos_node:
    case local_par_node: 
      break;
    case special_node: 
      check_token_ref(write_tokens(p));
      break;
    case pdf_literal_node:
      check_token_ref(pdf_literal_data(p));
      break;
    case pdf_colorstack_node: 
      if (pdf_colorstack_cmd(p) <= colorstack_data)
	check_token_ref(pdf_colorstack_data(p));
      break;
    case pdf_setmatrix_node: 
      check_token_ref(pdf_setmatrix_data(p));
      break;
    case late_lua_node:
      check_token_ref(late_lua_data(p));
      break;
    case pdf_annot_node:
      check_token_ref(pdf_annot_data(p));
      break;
    case pdf_start_link_node: 
      if (pdf_link_attr(p) != null)
	check_token_ref(pdf_link_attr(p));
      check_action_ref(pdf_link_action(p));
      break;
    case pdf_dest_node: 
      if (pdf_dest_named_id(p) > 0)
	check_token_ref(pdf_dest_id(p));
      break;
    case pdf_thread_node:
    case pdf_start_thread_node: 
      if (pdf_thread_named_id(p) > 0 ) check_token_ref(pdf_thread_id(p));
      if (pdf_thread_attr(p) != null)  check_token_ref(pdf_thread_attr(p));
      break;
    case user_defined_node: 
      switch( user_node_type(p)) {
      case 'a': check_attribute_ref(user_node_value(p)); break;
      case 't': check_token_ref(user_node_value(p));  break;
      case 'n': dorangetest(p,user_node_value(p),var_mem_max); break;
      default:  confusion(maketexstring("extuser")); break;
      }
      break;
    default:
      confusion(maketexstring("ext3"));
      return;
    }
    break;
  case rule_node: 
  case kern_node:
  case math_node:
  case penalty_node: 
    break;
  case margin_kern_node:
    check_node(margin_char(p));
    break;
  case mark_node:
    break;
  case disc_node:
    dorangetest(p,vlink(pre_break(p)),var_mem_max);
    dorangetest(p,vlink(post_break(p)),var_mem_max);
    dorangetest(p,vlink(no_break(p)),var_mem_max);
    break;
  case adjust_node:
    dorangetest(p,adjust_ptr(p),var_mem_max);
    break;
  case style_node:  
    break;
  case choice_node: 
    dorangetest(p,display_mlist(p),var_mem_max);
    dorangetest(p,text_mlist(p),var_mem_max);
    dorangetest(p,script_mlist(p),var_mem_max);
    dorangetest(p,script_script_mlist(p),var_mem_max);
    break;
  case ord_noad:
  case op_noad:
  case bin_noad:
  case rel_noad:
  case open_noad:
  case close_noad:
  case punct_noad:
  case inner_noad:
  case radical_noad:
  case over_noad:
  case under_noad:
  case vcenter_noad:
  case accent_noad:
    if (0) { /* can't do this */
      if (math_type(nucleus(p))>=sub_box)
	dorangetest(p,vinfo(nucleus(p)),var_mem_max);
      if (math_type(supscr(p))>=sub_box)
	dorangetest(p,vinfo(supscr(p)),var_mem_max);
      if (math_type(subscr(p))>=sub_box)
	dorangetest(p,vinfo(subscr(p)),var_mem_max);
    }
    break;
  case left_noad:
  case right_noad: 
    break;
  case fraction_noad:  
    dorangetest(p,vinfo(numerator(p)),var_mem_max);
    dorangetest(p,vinfo(denominator(p)),var_mem_max);
    break;
  case attribute_list_node:  
    break;
  case attribute_node:  
    break;
  case glue_spec_node:  
    break;
  case temp_node:
  case align_stack_node:
  case movement_node: 
  case if_node:
  case nesting_node:
  case span_node:
  case unhyphenated_node:
  case hyphenated_node:
  case delta_node:
  case passive_node:
    break;
  default:
    fprintf(stdout,"check_node: type is %d\n", type(p)); 
    return;
  }
  return;
}

void 
check_node_mem(void ) { 
  int i;
  dotest(zero_glue,width(zero_glue),0);
  dotest(zero_glue,type(zero_glue),glue_spec_node);
  dotest(zero_glue,vlink(zero_glue),null);
  dotest(zero_glue,stretch(zero_glue),0);
  dotest(zero_glue,stretch_order(zero_glue),normal);
  dotest(zero_glue,shrink(zero_glue),0);
  dotest(zero_glue,shrink_order(zero_glue),normal);

  dotest(sfi_glue,width(sfi_glue),0);
  dotest(sfi_glue,type(sfi_glue),glue_spec_node);
  dotest(sfi_glue,vlink(sfi_glue),null);
  dotest(sfi_glue,stretch(sfi_glue),0);
  dotest(sfi_glue,stretch_order(sfi_glue),sfi);
  dotest(sfi_glue,shrink(sfi_glue),0);
  dotest(sfi_glue,shrink_order(sfi_glue),normal);

  dotest(fil_glue,width(fil_glue),0);
  dotest(fil_glue,type(fil_glue),glue_spec_node);
  dotest(fil_glue,vlink(fil_glue),null);
  dotest(fil_glue,stretch(fil_glue),unity);
  dotest(fil_glue,stretch_order(fil_glue),fil);
  dotest(fil_glue,shrink(fil_glue),0);
  dotest(fil_glue,shrink_order(fil_glue),normal);

  dotest(fill_glue,width(fill_glue),0);
  dotest(fill_glue,type(fill_glue),glue_spec_node);
  dotest(fill_glue,vlink(fill_glue),null);
  dotest(fill_glue,stretch(fill_glue),unity);
  dotest(fill_glue,stretch_order(fill_glue),fill);
  dotest(fill_glue,shrink(fill_glue),0);
  dotest(fill_glue,shrink_order(fill_glue),normal);

  dotest(ss_glue,width(ss_glue),0);
  dotest(ss_glue,type(ss_glue),glue_spec_node);
  dotest(ss_glue,vlink(ss_glue),null);
  dotest(ss_glue,stretch(ss_glue),unity);
  dotest(ss_glue,stretch_order(ss_glue),fil);
  dotest(ss_glue,shrink(ss_glue),unity);
  dotest(ss_glue,shrink_order(ss_glue),fil);

  dotest(fil_neg_glue,width(fil_neg_glue),0);
  dotest(fil_neg_glue,type(fil_neg_glue),glue_spec_node);
  dotest(fil_neg_glue,vlink(fil_neg_glue),null);
  dotest(fil_neg_glue,stretch(fil_neg_glue),-unity);
  dotest(fil_neg_glue,stretch_order(fil_neg_glue),fil);
  dotest(fil_neg_glue,shrink(fil_neg_glue),0);
  dotest(fil_neg_glue,shrink_order(fil_neg_glue),normal);
  
  i=(prealloc+1);
  while (i<var_mem_max) {
    if (varmem_sizes[i]>0) {
      check_node(i);
    }
    i++;
  }
  test_count++;
}


halfword 
get_node (integer s) {
  register halfword r;
  integer t,x;

  /*check_node_mem();*/

  if (s<MAX_CHAIN_SIZE && free_chain[s]!=null) {
	r = free_chain[s];
	free_chain[s] = vlink(r);
	
#ifndef NDEBUG
	if (varmem_sizes[r]>=0) {
	  fprintf(stdout,"node %d (size %d, type %d) is not free!\n",(int)r,varmem_sizes[r],type(r));
	  return null;
	}
	varmem_sizes[r] = abs(varmem_sizes[r]);
	assert(varmem_sizes[r]==s);
#endif
	vlink(r)=null;
	var_used += s; /* maintain usage statistics */
	return r;
  } 

  while (1) { 
	t=node_size(rover);
    /* TODO, BUG: 
	   \parshapes are not free()d right now, because this bit of code
	   assumes there is only one |rover|. free() ing \parshapes would
	   create a linked list of rovers instead, with the last free()d one
	   in front.  In that case, it is likely that |t<=s|, and therefore
	   the check may fail without reason. That way, a large percentage of
	   \parshape requests may generate a realloc() of the whole array,
	   running out of system memory quickly.
    */
    if (t>s) { /* not using >= insures that we never add to the zero-sized chain*/
	  r = rover;
	  rover += s;
      node_size(rover) = (t-s);
#ifndef NDEBUG
      varmem_sizes[r]=s;
#endif
      break;
    } else {
      if (t<MAX_CHAIN_SIZE) {
		/* move the rover's content to a chain if possible, and
		   otherwise this bit of memory will remain unused forever */
		vlink(rover) = free_chain[t];
		free_chain[t] = rover;
#ifndef NDEBUG
        varmem_sizes[rover]=-t;
#endif
      }
	  
      x = (var_mem_max/5)+s+1; /* this way |s| will always fit */

      if (var_mem_max<2500) {	x += 10000;  }
      t=var_mem_max+x;

      varmem = (memory_word *)realloc(varmem,sizeof(memory_word)*t);
      if (varmem==NULL) {
		overflow(maketexstring("node memory size"),var_mem_max);
      }
      memset ((void *)(varmem+var_mem_max),0,x*sizeof(memory_word));

#ifndef NDEBUG
      varmem_sizes = (signed char *)realloc(varmem_sizes,sizeof(signed char)*t);
      memset ((void *)(varmem_sizes+var_mem_max),0,x*sizeof(signed char));
#endif
      rover = var_mem_max;
      node_size(rover)=x;
      var_mem_max=t;
    }
  }
  vlink(r)=null;
  var_used += s; /* maintain usage statistics */
  return r;
}

void
free_node (halfword p, integer s) {

  /*  if (s==4)
	fprintf(stdout,"del_node(%d)\n",p);
	*/
  if (s>=MAX_CHAIN_SIZE) { /* to be removed */
    return;
  }
  if (p<=prealloc) { 
    fprintf(stdout,"node %d (type %d) should not be freed!\n",(int)p, type(p));
    return;
  }

#ifndef NDEBUG
  if (varmem_sizes[p]<=0) {
    fprintf(stdout,"%c",(type(p)==glyph_node ? (character(p)>31? (int)character(p) : '#'): '*'));
    return;
  }
  assert(varmem_sizes[p]==s);
  varmem_sizes[p] = -varmem_sizes[p];
#endif

  (void)memset((varmem+p), 0, (sizeof(memory_word)*s));
  if (s<MAX_CHAIN_SIZE) {
    vlink(p) = free_chain[s];
    free_chain[s] = p;
  } else {
    node_size(p)=s; vlink(p)=rover;  rover=p;
  }
  var_used -= s; /* maintain statistics */
}

void
init_node_mem (halfword prealloced, halfword t) {
  prealloc=prealloced;

  assert(whatsit_node_data[user_defined_node].id==user_defined_node);
  assert(node_data[passive_node].id==passive_node);

  varmem = (memory_word *)realloc(varmem,sizeof(memory_word)*t);
  if (varmem==NULL) {
    overflow("node memory size",var_mem_max);
  }
  memset ((void *)varmem,0,sizeof(memory_word)*t);
#ifndef NDEBUG
  varmem_sizes = (signed char *)realloc(varmem_sizes,sizeof(signed char)*t);
  memset ((void *)varmem_sizes,0,sizeof(signed char)*t);
#endif
  var_mem_max=t; 
  rover = prealloced+1; vlink(rover) = null;
  node_size(rover)=(t-prealloced-1);
  var_used = 0;
}

/* this belongs in the web but i couldn't find the correct syntactic place */

halfword 
new_span_node(halfword n, int s, scaled w) {
  halfword p=new_node(span_node,0);
  span_link(p)=n; 
  span_span(p)=s;
  width(p)=w;
  return p;
}

void
print_node_mem_stats (void) {
  int i,a;
  halfword j;
  integer free_chain_counts[MAX_CHAIN_SIZE] = {0};
  integer node_counts[256] = {0};
  a = node_size(rover);
  fprintf(stdout,"\nnode memory in use: %d words out of %d (%d untouched)\n",
		  (int)(var_used+prealloc), (int)var_mem_max, (int)a);
  fprintf(stdout,"currently available: ");
  for (i=1;i<MAX_CHAIN_SIZE;i++) {
	for(j = free_chain[i];j!=null;j = vlink(j)) 
	  free_chain_counts[i] ++;
	if (free_chain_counts[i]>0)
	  fprintf(stdout,"%s%d:%d",(i>1 ? ", " : ""),i, (int)free_chain_counts[i]);
  }
  fprintf(stdout," nodes\n");

  i=(prealloc+1);
  while (i<var_mem_max) {
    if (varmem_sizes[i]>0) {
      if (type(i)>255) {
	node_counts[255] ++;
      } else if (type(i)==whatsit_node){
	node_counts[(subtype(i)+last_normal_node+1)] ++;
      } else {
	node_counts[type(i)] ++;
      }
    }
    i++;
  }
  fprintf(stdout,"currently in use: ");
  for (i=0;i<256;i++) {
    if (node_counts[i]>0) {
      if (i>last_normal_node) {
	fprintf(stdout,"%d,%d:%d ",whatsit_node,(i-last_normal_node-1),node_counts[i]);
      } else {
	fprintf(stdout,"%d:%d ",i,node_counts[i]);
      }
    }
  }
  fprintf(stdout,"nodes\n");
}

void
dump_node_mem (void) {
  dump_int(var_mem_max);
  dump_int(rover);
  dump_things(varmem[0],var_mem_max);
#ifndef NDEBUG
  dump_things(varmem_sizes[0],var_mem_max);
#endif
  dump_things(free_chain[0],MAX_CHAIN_SIZE);
  dump_int(var_used);
  dump_int(prealloc);
}

void
undump_node_mem (void) {
  undump_int(var_mem_max);
  undump_int(rover);
  varmem = xmallocarray (memory_word, var_mem_max);
  memset ((void *)varmem,0,var_mem_max*sizeof(memory_word));
  undump_things(varmem[0],var_mem_max);
#ifndef NDEBUG
  varmem_sizes = xmallocarray (signed char, var_mem_max);
  memset ((void *)varmem_sizes,0,var_mem_max*sizeof(signed char));
  undump_things(varmem_sizes[0],var_mem_max);
#endif
  undump_things(free_chain[0],MAX_CHAIN_SIZE);
  undump_int(var_used); 
  undump_int(prealloc);
}

halfword 
string_to_pseudo(integer l,integer pool_ptr, integer nl) {
  halfword i, r, q;
  four_quarters w;
  int sz ;
  halfword h = new_node(pseudo_file_node,0);
  while (l<pool_ptr) {
    int m = l;
    while ((l<pool_ptr)&&(str_pool[l]!=nl)) l++;
    sz = (l-m+7) / 4;
    if (sz==1) 
      sz=2;
    r = new_node(pseudo_line_node,sz);
    i = r;
    while (sz-->1) {
      w.b0=str_pool[m++];   
      w.b1=str_pool[m++];
      w.b2=str_pool[m++]; 
      w.b3=str_pool[m++];
      varmem[++i].qqqq=w; 
    }
    w.b0 = (l>m ? str_pool[m++] : ' ');
    w.b1 = (l>m ? str_pool[m++] : ' ');
    w.b2 = (l>m ? str_pool[m++] : ' ');
    w.b3 = (l>m ? str_pool[m]   : ' ');
    varmem[++i].qqqq=w;
    if (pseudo_lines(h)==null) {
      pseudo_lines(h) = r;
      q = r;
    } else {
      couple_nodes(q,r);
    }
    q = vlink(q);
    if (str_pool[l]==nl) l++;
  }
  return h;
}

