/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"

#define MAX_CHAIN_SIZE 12

memory_word *varmem = NULL;

#ifndef NDEBUG
char *varmem_sizes = NULL;
#endif

halfword var_mem_max = 0;
halfword rover = 0;

halfword free_chain[MAX_CHAIN_SIZE] = {null};

static int prealloc=0;

halfword slow_get_node (integer s) ; /* defined below */

#undef link /* defined by cpascal.h */
#define info(a)    fixmem[(a)].hhlh
#define link(a)    fixmem[(a)].hhrh

#define fake_node 100
#define fake_node_size 2
#define fake_node_name ""

#define pseudo_line_node_size 2
#define shape_node_size 2

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
  { pseudo_line_node,     pseudo_line_node_size,     "pseudo_line"    },
  { inserting_node,       page_ins_node_size,        "insert_head"    },
  { split_up_node,        page_ins_node_size,        "split_head"     },
  { expr_node,            expr_node_size,            "expr_stack"     },
  { nesting_node,         nesting_node_size,         "nested_list"    },
  { span_node,            span_node_size,            "span"           },
  { attribute_node,       attribute_node_size,       "attribute"      },
  { glue_spec_node,       glue_spec_size,            "glue_spec"      },
  { attribute_list_node,  attribute_list_node_size,  "attribute_list" },
  { action_node,          pdf_action_size,           "action"         },
  { temp_node,            temp_node_size,            "temp"           },
  { align_stack_node,     align_stack_node_size,     "align_stack"    },
  { movement_node,        movement_node_size,        "movement_stack" },
  { if_node,              if_node_size,              "if_stack"       },
  { unhyphenated_node,    active_node_size,          "unhyphenated"   },
  { hyphenated_node,      active_node_size,          "hyphenated"     },
  { delta_node,           delta_node_size,           "delta"          },
  { passive_node,         passive_node_size,         "passive"        },
  { shape_node,           shape_node_size,           "shape"          },
  { -1,                   -1,                        NULL             }};

#define last_normal_node shape_node
 
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

#define last_whatsit_node user_defined_node

#define  get_node_size(i,j) (i!=whatsit_node ? node_data[i].size : whatsit_node_data[j].size)
#define  get_node_name(i,j) (i!=whatsit_node ? node_data[i].name : whatsit_node_data[j].name)

halfword
new_node(int i, int j) {
  register int s;
  register halfword n;
  s = get_node_size(i,j);
  n = get_node(s);
  /* it should be possible to do this memset at free_node()  */
  /* type() and subtype() will be set below, and vlink() is 
     set to null by get_node(), so we can do we clearing one 
     word less than |s| */
  (void)memset((varmem+n+1),0, (sizeof(memory_word)*(s-1)));
  switch (i) {
  case glyph_node:
    init_lang_data(n);
    break;
  case glue_node:
  case kern_node:
  case glue_spec_node:
    break;
  case hlist_node:
  case vlist_node:
    box_dir(n)=-1; 
    break;
  case whatsit_node:  
    if (j==open_node) {
      open_name(n) = get_nullstr();
      open_area(n) = open_name(n);
      open_ext(n) = open_name(n);
    }
    break;
  case disc_node: 
    pre_break(n)=pre_break_head(n); 
    type(pre_break(n))= nesting_node; 
    subtype(pre_break(n))=pre_break_head(0);
    post_break(n)=post_break_head(n);
    type(post_break(n))= nesting_node;  
    subtype(post_break(n))=post_break_head(0);
    no_break(n)=no_break_head(n);
    type(no_break(n))= nesting_node;  
    subtype(no_break(n))=no_break_head(0);
    break;
  case rule_node:
    depth(n)=null_flag; 
    height(n)=null_flag;
    rule_dir(n)=-1;
    /* fall through */
  case unset_node:        
    width(n) = null_flag;
    break; 
  case pseudo_line_node: 
    /* this is a trick that makes pseudo_files slightly slower,
     * but the overall allocation faster then an explicit test
     * at the top of new_node().
     */
    free_node(n,pseudo_line_node_size);
    n = slow_get_node(j);
    (void)memset((varmem+n+1),0, (sizeof(memory_word)*(j-1)));
    break;
  case shape_node: 
    /* likewise for (par)shapes */
    free_node(n,shape_node_size);
    n = slow_get_node(j);
    (void)memset((varmem+n+1),0, (sizeof(memory_word)*(j-1)));
    break;
  default: 
    break;
  }  
  if (nodetype_has_attributes(i)) {
    build_attribute_list(n); 
  }
  type(n)=i;
  subtype(n)=j;  
  return n;
}

/* makes a duplicate of the node list that starts at |p| and returns a
   pointer to the new list */

halfword 
copy_node_list(halfword p) { 
  halfword q; /* previous position in new list */
  halfword h = null; /* head of the list */
  while (p!=null) {
    register halfword s = copy_node(p);
    if (h==null) {
      h = s;
    } else {
      couple_nodes(q,s);
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
  register halfword s; /* a helper variable for copying into variable mem  */
  register int i = get_node_size(type(p), subtype(p));
  r = get_node(i);
  (void)memcpy((varmem+r),(varmem+p),(sizeof(memory_word)*i));

  if (nodetype_has_attributes(type(p))) {
    add_node_attr_ref(node_attr(p)); 
  }
  vlink(r)=null;
  alink(r)=null;
  switch (type(p)) {
  case glyph_node:  
    s=copy_node_list(lig_ptr(p));
    lig_ptr(r)=s;
    break;
  case glue_node:  
    add_glue_ref(glue_ptr(p));
    s=copy_node_list(leader_ptr(p));
    leader_ptr(r)=s;
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
  case glue_spec_node: 
    glue_ref_count(r)=null;
    break;
  case whatsit_node:
    switch (subtype(p)) {
    case dir_node:  
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
      break;
    }
    break;
  }
  return r;
}

void 
flush_node (halfword p) {
  /*check_node_mem();*/
  assert(p>prealloc);
  assert(p<var_mem_max);

  if (varmem_sizes[p]==0) {
    check_node_mem();
    halfword r = null;    
    fprintf(stdout,"attempt to double-free %s node %d\n", get_node_name(type(p),subtype(p)), (int)p);
    for (r=prealloc+1;r<var_mem_max;r++) {
      if (vlink(r)==p) {
	halfword s = r;
	while (s>prealloc && varmem_sizes[s]==0)
	  s--;
	if (s!=null && s!=prealloc && s!=var_mem_max) {
	  fprintf(stdout,"  pointed to from %s node %d\n", get_node_name(type(s),subtype(s)), (int)s);
	}
      }
    }
    return ; /* double free */
  }

  switch(type(p)) {
  case glyph_node:
    flush_node_list(lig_ptr(p));
    break;
  case glue_node: 
    delete_glue_ref(glue_ptr(p));
    flush_node_list(leader_ptr(p));
    break;

  case attribute_node:
  case attribute_list_node:
  case temp_node:
  case glue_spec_node:
  case rule_node: 
  case kern_node:
  case math_node:
  case penalty_node: 
    break;

  case hlist_node:
  case vlist_node:
  case unset_node:
    flush_node_list(list_ptr(p));
    break;
  case whatsit_node: 
    switch (subtype(p)) {

    case dir_node: 
      break;
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
  case ins_node:
    flush_node_list(ins_ptr(p));
    delete_glue_ref(split_top_ptr(p));
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
  case choice_node: 
    flush_node_list(display_mlist(p));
    flush_node_list(text_mlist(p));
    flush_node_list(script_mlist(p));
    flush_node_list(script_script_mlist(p));
    break;

  case style_node:  
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
  case left_noad:
  case right_noad: 
  case fraction_noad:  
    break;

  case pseudo_file_node:
    flush_node_list(pseudo_lines(p));
    break;
  case pseudo_line_node:
  case shape_node:
    free_node(p,subtype(p));
    return;
    break;

  case align_stack_node:
  case movement_node: 
  case if_node:
  case nesting_node:
  case unhyphenated_node:
  case hyphenated_node:
  case delta_node:
  case passive_node:
  case action_node: 
  case inserting_node: 
  case split_up_node: 
  case expr_node:
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
  register halfword q;
  halfword p=pp; /*to prevent trashing of the argument pointer, for debugging*/
  while (p!=null) {
    q = vlink(p);
    flush_node(p);
    p=q;
  }
}

static int test_count = 1;

#define dorangetest(a,b,c) {                                            \
    if (!(b>=0 && b<c)) {                                               \
      fprintf(stdout,"For node p:=%d, 0<=%d<%d (l.%d,r.%d)\n",          \
              (int)a, (int)b, (int)c, __LINE__,test_count);             \
      confusion(maketexstring("dorangetest"));                          \
    }                                                                   \
  }

#define dotest(a,b,c) {                                                 \
    if (b!=c) {                                                         \
      fprintf(stdout,"For node p:=%d, %d==%d (l.%d,r.%d)\n",            \
              (int)a, (int)b, (int)c, __LINE__,test_count);             \
      confusion(maketexstring("dotest"));                               \
    }                                                                   \
  }

#define check_action_ref(a)     { dorangetest(p,a,var_mem_max); }
#define check_glue_ref(a)       { dorangetest(p,a,var_mem_max); }
#define check_attribute_ref(a)  { dorangetest(p,a,var_mem_max); }
#define check_token_ref(a)      

void 
check_node (halfword p) {

  switch(type(p)) {
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
    default:
      confusion(maketexstring("ext3"));
    }
    break;
  case margin_kern_node:
    check_node(margin_char(p));
    break;
  case disc_node:
    dorangetest(p,vlink(pre_break(p)),var_mem_max);
    dorangetest(p,vlink(post_break(p)),var_mem_max);
    dorangetest(p,vlink(no_break(p)),var_mem_max);
    break;
  case adjust_node:
    dorangetest(p,adjust_ptr(p),var_mem_max);
    break;
  case pseudo_file_node:
    dorangetest(p,pseudo_lines(p),var_mem_max);
    break;
  case pseudo_line_node:
    break;
  case shape_node:
    break;
  case choice_node: 
    dorangetest(p,display_mlist(p),var_mem_max);
    dorangetest(p,text_mlist(p),var_mem_max);
    dorangetest(p,script_mlist(p),var_mem_max);
    dorangetest(p,script_script_mlist(p),var_mem_max);
    break;
  case fraction_noad:  
    dorangetest(p,vinfo(numerator(p)),var_mem_max);
    dorangetest(p,vinfo(denominator(p)),var_mem_max);
    break;
  case rule_node: 
  case kern_node:
  case math_node:
  case penalty_node: 
  case mark_node:
  case style_node:  
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
  case left_noad:
  case right_noad: 
  case attribute_list_node:  
  case attribute_node:  
  case glue_spec_node:  
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
  case expr_node:
    break;
  default:
    fprintf(stdout,"check_node: type is %d\n", type(p)); 
  }
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
  
  
  for (i=(prealloc+1);i<var_mem_max;i++) {
    if (varmem_sizes[i]>0) {
      check_node(i);
    }
  }
  test_count++;
}

halfword 
get_node (integer s) {
  register halfword r;
  assert(s<MAX_CHAIN_SIZE);

  r = free_chain[s];
  if (r!=null) {
    free_chain[s] = vlink(r);
#ifndef NDEBUG
    varmem_sizes[r] = s;
#endif
    vlink(r)=null;
    var_used += s; /* maintain usage statistics */
    return r;
  } 
  /* this is the end of the 'inner loop' */
  return slow_get_node(s);
}

void
free_node (halfword p, integer s) {

  if (p<=prealloc) { 
    fprintf(stdout,"node %d (type %d) should not be freed!\n",(int)p, type(p));
    return;
  }
#ifndef NDEBUG
  if (varmem_sizes[p]!=s) {
    fprintf(stdout,"%c",(type(p)==glyph_node ? (character(p)>31? (int)character(p) : '#'): '*'));
    return;
  }
  varmem_sizes[p] = 0;
#endif
  if (s<MAX_CHAIN_SIZE) {
    vlink(p) = free_chain[s];
    free_chain[s] = p;
  } else { 
    /* todo ? it is perhaps possible to merge this node with an existing rover */
    node_size(p)=s;
    vlink(p) = rover;
    while (vlink(rover)!=vlink(p)){ 
      rover=vlink(rover);
    }
    vlink(rover) = p;
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
    overflow(maketexstring("node memory size"),var_mem_max);
  }
  /*memset ((void *)varmem,0,sizeof(memory_word)*t);*/
#ifndef NDEBUG
  varmem_sizes = (char *)realloc(varmem_sizes,sizeof(char)*t);
  if (varmem_sizes==NULL) {
    overflow(maketexstring("node memory size"),var_mem_max);
  }
  memset ((void *)varmem_sizes,0,sizeof(char)*t);
#endif
  var_mem_max=t; 
  rover = prealloced+1; vlink(rover) = rover;
  node_size(rover)=(t-rover);
  var_used = 0;
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

/* it makes sense to enlarge the varmem array immediately */

void
undump_node_mem (void) {
  int x;
  undump_int(x);
  undump_int(rover);
  var_mem_max = (x<100000 ? 100000 : x);
  varmem = xmallocarray (memory_word, var_mem_max);
  /*memset ((void *)varmem,0,x*sizeof(memory_word));*/
  undump_things(varmem[0],x);
#ifndef NDEBUG
  varmem_sizes = xmallocarray (char, var_mem_max);
  memset ((void *)varmem_sizes,0,x*sizeof(char));
  undump_things(varmem_sizes[0],x);
#endif
  undump_things(free_chain[0],MAX_CHAIN_SIZE);
  undump_int(var_used); 
  undump_int(prealloc);
  if (var_mem_max>x) {
    /* todo ? it is perhaps possible to merge the new node with an existing rover */
    vlink (x) = rover;
    node_size(x) = (var_mem_max-x);
    while(vlink(rover)!=vlink(x)) {rover=vlink(rover);}
    vlink (rover) = x;
  }
}

#if 0
void test_rovers (char *s) {
  int q = rover;
  int r = q;
  fprintf(stdout,"%s: {rover=%d,size=%d,link=%d}",s,r,node_size(r),vlink(r));
  while (vlink(r)!=q) {
    r = vlink(r);
    fprintf(stdout,",{rover=%d,size=%d,link=%d}",r,node_size(r),vlink(r));
  }
  fprintf(stdout,"\n");
}
#else
#define test_rovers(a)
#endif

halfword 
slow_get_node (integer s) {
  register halfword r;
  int x, t;
 RETRY:
  t = node_size(rover);
  assert(vlink(rover)<var_mem_max);
  assert(vlink(rover)!=0);
  test_rovers("entry");
  if (t>s) {
    /* allocating from the bottom helps decrease page faults */
    r = rover;
    rover += s;
    vlink(rover)=vlink(r);
    node_size(rover) = node_size(r) - s;
    if (vlink(rover)!=r) { /* list is longer than one */
      halfword q = r;
      while (vlink(q)!=r) { q=vlink(q);}
      vlink(q) += s;
    } else {
      vlink(rover) += s;
    }
    test_rovers("taken");
    assert(vlink(rover)<var_mem_max);
#ifndef NDEBUG
    varmem_sizes[r]=(s>127 ? 127 : s);
#endif
    vlink(r)=null;
    var_used += s; /* maintain usage statistics */
    return r; /* this is the only exit */
  } else {

    /* attempt to keep the free list small */
    if (vlink(rover)!=rover) {
      if (t<MAX_CHAIN_SIZE) {
	halfword l = vlink(rover);
	vlink(rover) = free_chain[t];
	free_chain[t] = rover;
	rover = l;
	while (vlink(l)!=free_chain[t]) { l=vlink(l); }
	vlink(l) = rover;
	test_rovers("outtake");
	goto RETRY;
      } else {
	r = rover;
	while (vlink(rover)!=r) { 
	  if (node_size(rover)>s) {
	    goto RETRY;
	  }
	  rover=vlink(rover);
	}
      }
    }
    /* if we are still here, it was apparently impossible to get a match */
    x = (var_mem_max>>2)+s;
    varmem = (memory_word *)realloc(varmem,sizeof(memory_word)*(var_mem_max+x));
    if (varmem==NULL) {
      overflow(maketexstring("node memory size"),var_mem_max);
    }
    /*memset ((void *)(varmem+var_mem_max),0,x*sizeof(memory_word));*/
    
#ifndef NDEBUG
    varmem_sizes = (char *)realloc(varmem_sizes,sizeof(char)*(var_mem_max+x));
    if (varmem_sizes==NULL) {
      overflow(maketexstring("node memory size"),var_mem_max);
    }
    memset ((void *)(varmem_sizes+var_mem_max),0,x*sizeof(char));
#endif

    /* todo ? it is perhaps possible to merge the new memory with an existing rover */
    vlink(var_mem_max) = rover;
    node_size(var_mem_max) = x;
    while(vlink(rover)!=vlink(var_mem_max)) {rover=vlink(rover);}
    vlink (rover) = var_mem_max;
    rover = var_mem_max;
    test_rovers("realloc");
    var_mem_max += x;
    goto RETRY;
  }
}

void
print_node_mem_stats (int num, int online) {
  int i,b;
  halfword j;
  integer free_chain_counts[MAX_CHAIN_SIZE] = {0};
  int node_counts[last_normal_node+last_whatsit_node+2] = {0};

  fprintf(log_file,"\nnode memory in use: %d words out of %d\n", (int)(var_used+prealloc), (int)var_mem_max);
  if (online)
    fprintf(stdout,"\nnode memory in use: %d words out of %d\n", (int)(var_used+prealloc), (int)var_mem_max);

  fprintf(log_file,"rapidly available: ");
  if (online) fprintf(stdout,"rapidly available: ");
  b=0;
  for (i=1;i<MAX_CHAIN_SIZE;i++) {
    for(j = free_chain[i];j!=null;j = vlink(j)) 
      free_chain_counts[i] ++;
    if (free_chain_counts[i]>0) {
      fprintf(log_file,"%s%d:%d",(b ? ", " : ""),i, (int)free_chain_counts[i]);
      if (online) 
	fprintf(stdout,"%s%d:%d",(b ? ", " : ""),i, (int)free_chain_counts[i]);
      b=1;
    }
  }
  fprintf(log_file," nodes\n");
  if (online)
    fprintf(stdout," nodes\n");
  for (i=(var_mem_max-1);i>prealloc;i--) {
    if (varmem_sizes[i]>0) {
      /*fprintf(stdout,"%s at %d\n",get_node_name(type(i),subtype(i)),i);*/
      if (type(i)>last_normal_node+last_whatsit_node) {
        node_counts[last_normal_node+last_whatsit_node+1] ++;
      } else if (type(i)==whatsit_node){
        node_counts[(subtype(i)+last_normal_node+1)] ++;
      } else {
        node_counts[type(i)] ++;
      }
    }
  }
  b=0;
  fprintf(log_file,"current usage: ");
  if (online)
    fprintf(stdout,"current usage: ");
  for (i=0;i<last_normal_node+last_whatsit_node+2;i++) {
    if (node_counts[i]>0) {
      fprintf(log_file,"%s%d %s%s",(b ? ", " : ""),
	      (int)node_counts[i],
	      get_node_name((i>last_normal_node ? whatsit_node : i),
			    (i>last_normal_node ? (i-last_normal_node-1) : 0)),
	      (node_counts[i]>1 ? "s" : ""));
      if (online)
	fprintf(stdout,"%s%d %s%s",(b ? ", " : ""),
		(int)node_counts[i],
		get_node_name((i>last_normal_node ? whatsit_node : i),
			      (i>last_normal_node ? (i-last_normal_node-1) : 0)),
		(node_counts[i]>1 ? "s" : ""));
      b=1;
    }
  }
  fprintf(log_file,"\n");
  if (online)
    fprintf(stdout,"\n");
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

halfword 
string_to_pseudo(integer l,integer pool_ptr, integer nl) {
  halfword i, r, q = null;
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
    while (--sz>1) {
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

/* attribute stuff */

halfword 
new_attribute_node(integer i, integer c) {
  halfword p; /* the new node */
  p=new_node(attribute_node,0);
  attribute_id(p)=i;  
  attribute_value(p)=c; 
  return p;
}

void 
build_attribute_list(halfword b) {
  halfword p,q,r;
  integer i,v;
  assert(type(b)!=attribute_list_node);
  node_attr(b)=null;
  if (max_used_attr!=0) {
    if (attr_list_cache==cache_disabled) {
      q=new_node(attribute_list_node,0); 
      p=q;
      for (i=0;i<=max_used_attr;i++) {
	v=get_attribute(i);
	if (v>=0) {
	  r=new_attribute_node(i,v); 
	  vlink(p)=r;
	  p=r;
	};
      }
      if (q!=p) {
	attr_list_cache=q;
	attr_list_ref(attr_list_cache)=0;
      } else {
	flush_node(q);
	attr_list_cache=null;
      }
    }
    if (attr_list_cache!=null) {
      attr_list_ref(attr_list_cache)++;
      node_attr(b)=attr_list_cache;   
    }
  }
}

void 
delete_attribute_ref(halfword b) {
  halfword r,q;
  if (b!=null){
    if (type(b)!=attribute_list_node ) {
      fprintf(stdout,"the type of %d is %d\n",b,type(b));
      check_node_mem();
    }
    assert(type(b)==attribute_list_node);
    assert(attr_list_ref(b)>0);
    attr_list_ref(b)--;
    if (attr_list_ref(b)==0) {
      if (b==attr_list_cache) 
	attr_list_cache=cache_disabled;
      q=vlink(b);
      while (q!=null){
        r=q; 
	q=vlink(q);
        flush_node(r);
      }
      flush_node(b);
    }
  }
}


int
unset_attribute (halfword n, int i, int val) {
  halfword head,t;
  int seen, ret;
  if (!nodetype_has_attributes(type(n)))
    return -1;

  head=node_attr(n);
  if (head==null) {
    return -1; /* done. nothing to erase */
  } else if (vlink(head)==null) {
    flush_node(head);
    node_attr(n)=null;
    return -1; /* done. nothing to erase */
  }
  /* check if even present */
  t = vlink(head);
  seen = 0;
  while (t!=null) {
    if (attribute_id(t)==i) {
      if (val==-1 || attribute_value(t)==val)
	seen = 1 ;
      break;
    }
    t = vlink(t);
  }
  if (!seen) {
    return -1; /* done. nothing to erase */
  }
  head = copy_node_list(head); /* attr_list_ref = 0 */
  attr_list_ref(head) = 1; /* used once */
  if (node_attr(n)!=null)
    delete_attribute_ref(node_attr(n));
  node_attr(n) = head;
  /* */
  t = head;
  while (vlink(t)!=null) {
    if (attribute_id(vlink(t))==i) {
      if (val==-1 || attribute_value(vlink(t))==val) {
	/* for retval */
	ret = attribute_value(vlink(t));
	/* destroy this node, reuse seen */
	seen = vlink(vlink(t));
	flush_node(vlink(t));
	vlink(t) = seen;
	/* if we just deleted the *only* attribute, kill the list */
	if (vlink(head)==null) {
	  flush_node(head);
	  node_attr(n)=null;
	}
	return ret;
      }
      break;
    }
    t = vlink(t);
  }
  return -1; /* not reached */
}

/* TODO: check that nodes with type n can have node attributes ! */
void
set_attribute (halfword n, int i, int val) {
  halfword head,p,t;
  if (!nodetype_has_attributes(type(n)))
    return ;
  head = node_attr(n);
  if (head==null) {   /* create a new one */
    head = new_node(attribute_list_node,0);
  } else {
    /* check for duplicate def */
    t = vlink(head);
    while (t!=null) {
      if (attribute_id(t)==i && attribute_value(t)==val) {
	return;
      } else if (attribute_id(t)>i) {
	break;
      }
      t = vlink(t);
    }
    head = copy_node_list(head);
  }
  if (node_attr(n)!=null)
    delete_attribute_ref(node_attr(n));
  attr_list_ref(head) = 1; /* used once */
  node_attr(n) = head;
  /* */
  t = head;
  if (vlink(t)!=null) {
    t = vlink(t);
    while (1) {
      if (attribute_id(t)==i) {
	attribute_value(t)=val;
	return;
      } else if (attribute_id(t)>i) {
	p = new_node(attribute_node,0);
	vlink(p) = vlink(t);
	vlink(t) = p;
	attribute_id(p) = attribute_id(t) ;
	attribute_value(p) = attribute_value(t) ;
	attribute_id(t) = i;
	attribute_value(t) = val;
	return;
      }
      if (vlink(t)==null)
	break;
      t = vlink(t);
    }
    /* this point is reached if the new id is higher 
       than the last id in the original list */
  }
  p = new_node(attribute_node,0);
  attribute_id(p) = i;
  attribute_value(p) = val;
  vlink(t) = p;
  return;
}

int 
has_attribute (halfword n,int i, int val) {
  register halfword t;
  if (!nodetype_has_attributes(type(n)))
    return -1;
  t=node_attr(n);
  if (t!=null)  {
    t = vlink(t);
    while (t!=null) {
      if (attribute_id(t)==i) {
	if (val==-1 || attribute_value(t)==val) {
	  return attribute_value(t);
	}
      } else if (attribute_id(t)>i) {
	break;
      }
      t = vlink(t);
    }
  }
  return -1;
}

