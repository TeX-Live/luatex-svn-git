/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"

#undef link /* defined by cpascal.h */
#define info(a)    fixmem[(a)].hhlh
#define link(a)    fixmem[(a)].hhrh

char * node_names[] = {
  "hlist", /* 0 */
  "vlist",  
  "rule",
  "ins", 
  "mark", 
  "adjust",
  "!",  /* used to be ligature */
  "disc", 
  "whatsit",
  "math", 
  "glue",  /* 10 */
  "kern",
  "penalty", 
  "unset", 
  "style",
  "choice",  
  "ord",
  "op",
  "bin",
  "rel",
  "open", /* 20 */
  "close",
  "punct",
  "inner",
  "radical",
  "fraction",
  "under",
  "over",
  "accent",
  "vcenter",
  "left", /* 30 */
  "right",
  "!","!","!","!","!",
  /* Thanh lost count, and set margin kern to 40 arbitrarily.
    I am now misusing that as a spine for my own new nodes*/
  "!",
  "!",
  "action",  
  "margin_kern", /* 40 */
  "glyph",
  "attribute",
  "glue_spec",
  "attribute_list",
   NULL };


char *whatsit_node_names[] = {
  "open",
  "write",
  "close",
  "special",
  "language",
  "!" /* "set_language" */,
  "local_par",
  "dir",
  "pdf_literal",
  "!" /* "pdf_obj" */,
  "pdf_refobj",
  "!" /* "pdf_xform" */,
  "pdf_refxform",
  "!" /* "pdf_ximage" */,
  "pdf_refximage",
  "pdf_annot",
  "pdf_start_link",
  "pdf_end_link",
  "!" /* "pdf_outline" */,
  "pdf_dest",
  "pdf_thread",
  "pdf_start_thread",
  "pdf_end_thread",
  "pdf_save_pos",
  "!" /* "pdf_info" */,
  "!" /* "pdf_catalog" */,
  "!" /* "pdf_names" */,
  "!" /* "pdf_font_attr" */,
  "!" /* "pdf_include_chars" */,
  "!" /* "pdf_map_file" */,
  "!" /* "pdf_map_line" */,
  "!" /* "pdf_trailer" */,
  "!" /* "pdf_font_expand" */,
  "!" /* "set_random_seed" */,
  "!" /* "pdf_glyph_to_unicode" */,
  "late_lua",
  "close_lua",
  "!" /* "save_cat_code_table" */,
  "!" /* "init_cat_code_table" */,
  "pdf_colorstack", 
  "pdf_setmatrix",
  "pdf_save",
  "pdf_restore",
  "cancel_boundary",
  "left_ghost_marker",
  "right_ghost_marker",
  "user_defined",
  NULL };

static char *group_code_names[] = {
  "",
  "simple",
  "hbox",
  "adjusted_hbox",
  "vbox",
  "vtop",
  "align",
  "no_align",
  "output",
  "math",
  "disc",
  "insert",
  "vcenter",
  "math_choice",
  "semi_simple",
  "math_shift",
  "math_left",
  "local_box" ,
  "split_off",
  "split_keep",
  "preamble",
  "align_set",
  "fin_row"};

char *pack_type_name[] = { "exactly", "additional"};


/* allocate a new whatsit */

static int
lua_node_whatsit_new(int j) {
  halfword p  = null;
  switch (j) {
  case open_node:
    p = get_node(open_node_size);  
    write_stream(p)=0;
    open_name(p) = get_nullstr();
    open_area(p) = open_name(p);
    open_ext(p) = open_name(p);
    break;
  case write_node:
    p = get_node(write_node_size);
    write_stream(p) = null;
    write_tokens(p) = null;
    break;
  case close_node:
    p = get_node(close_node_size);
    write_stream(p) = null;
    break;
  case special_node:
    p = get_node(write_node_size);
    write_stream(p) = null;
    write_tokens(p) = null;
    break;
  case language_node:
    p = get_node(language_node_size);
    what_lang(p) = 0;
    what_lhm(p) = 0;
    what_rhm(p) = 0;
    break;
  case local_par_node:
    p =get_node(local_par_size);
    local_pen_inter(p) = 0;
    local_pen_broken(p) = 0;
    local_par_dir(p) = 0;
    local_box_left(p) = null;
    local_box_left_width(p) = 0;
    local_box_right(p) = null;
    local_box_right_width(p) = 0;
    break;
  case dir_node:
    p = get_node(dir_node_size);
    dir_dir(p) = 0;
    dir_level(p) = 0;
    dir_dvi_ptr(p) = 0;
    dir_dvi_h(p) = 0;
    break;
  case pdf_literal_node: 
    p = get_node(write_node_size);
    pdf_literal_mode(p) = 0;
    pdf_literal_data(p) = null;
    break;
  case pdf_refobj_node:
    p =get_node(pdf_refobj_node_size);
    pdf_obj_objnum(p) = 0;
    break;
  case pdf_refxform_node:
    p =get_node(pdf_refxform_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_xform_objnum(p) = 0;
    break;
  case pdf_refximage_node:
    p =get_node(pdf_refximage_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_ximage_objnum(p) = 0;
    break;
  case pdf_annot_node:
    p =get_node(pdf_annot_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_annot_objnum(p) = 0;
    pdf_annot_data(p) = null;
    break;
  case pdf_start_link_node:
    p =get_node(pdf_annot_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_link_objnum(p) = 0;
    pdf_link_attr(p) = null;
    pdf_link_action(p) = null;
    break;
  case pdf_end_link_node:
    p =get_node(pdf_end_link_node_size);
    break;
  case pdf_dest_node:
    p =get_node(pdf_dest_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_dest_named_id(p) = 0;
    pdf_dest_id(p) = 0;
    pdf_dest_type(p) = 0;
    pdf_dest_xyz_zoom(p) = 0;
    pdf_dest_objnum(p) = 0;
    break;
  case pdf_thread_node:
  case pdf_start_thread_node:
    p =get_node(pdf_thread_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_thread_named_id(p) = 0;
    pdf_thread_id(p) = 0;
    pdf_thread_attr(p) = null;
    break;
  case pdf_end_thread_node:
    p =get_node(pdf_end_thread_node_size);
    break;
  case pdf_save_pos_node:
    p =get_node(pdf_save_pos_node_size);
    break;
  case late_lua_node:
    p =get_node(write_node_size);
    late_lua_reg(p) = 0;
    late_lua_data(p) = null;
    break;
  case close_lua_node:
    p =get_node(write_node_size);
    late_lua_reg(p) = 0;
    break;
  case pdf_colorstack_node:
    p =get_node(pdf_colorstack_node_size);
    pdf_colorstack_stack(p) = 0;
    pdf_colorstack_cmd(p) = 0;
    pdf_colorstack_data(p) = null;
    break;
  case pdf_setmatrix_node:
    p =get_node(pdf_setmatrix_node_size);
    pdf_setmatrix_data(p) = null;
    break;
  case pdf_save_node:
    p =get_node(pdf_save_node_size);
    break;
  case pdf_restore_node:
    p =get_node(pdf_restore_node_size);
    break;
  case cancel_boundary_node:
    p =get_node(cancel_boundary_size);
    break;
  case left_ghost_marker_node:
    p =get_node(left_ghost_marker_size);
    break;
  case right_ghost_marker_node:
    p =get_node(right_ghost_marker_size);
    break;
  case user_defined_node:
    p = get_node(user_defined_node_size);
    user_node_id(p) = 0;
    user_node_type(p) = 0;
    user_node_value(p)= null;
    break;
  default:
    fprintf(stdout,"<unknown whatsit type %d>\n",j);
  }
  subtype(p) = j;
  return p;
}

halfword
lua_node_new(int i, int j) {
  halfword n  = null;
  switch (i) {
  case hlist_node:
  case vlist_node:
    n=get_node(box_node_size); 
    width(n)=0; depth(n)=0; height(n)=0; 
    shift_amount(n)=0; 
    list_ptr(n)=null;
    glue_sign(n)=normal; 
    glue_order(n)=normal; 
    glue_set(n)=0.0;
    box_dir(n)=-1; 
    break;
  case rule_node:
    n=get_node(rule_node_size);
    width(n)=null_flag; 
    depth(n)=null_flag; 
    height(n)=null_flag;
    rule_dir(n)=-1;
    break;
  case ins_node:
    n = get_node(ins_node_size); 
    float_cost(n)=0; height(n)=0; depth(n)=0;
    ins_ptr(n)=null; split_top_ptr(n)=null;
    break;
  case mark_node: 
    n = get_node(mark_node_size);  
    mark_ptr(n)=null;
    mark_class(n)=0;
    break;
  case adjust_node: 
    n = get_node(adjust_node_size); 
    adjust_ptr(n)=null;
    break;
  case glyph_node:          
    n = get_node(glyph_node_size); 
    lig_ptr(n) = null; 
    character(n) = 0;
    font(n) = 0;
	x_displace(n) = 0;
	y_displace(n) = 0;
    break;
  case disc_node: 
    n = get_node(disc_node_size);  
    replace_count(n)=0;
    pre_break(n)=null;
    post_break(n)=null;
    break;
  case whatsit_node:        
    n = lua_node_whatsit_new(j);
    break;
  case math_node: 
    n = get_node(math_node_size);  
    node_attr(n) = null;
    surround(n) = 0;
    break;
  case glue_node: 
    n = get_node(glue_node_size);  
    glue_ptr(n)=null;
    leader_ptr(n)=null;
    break;
  case glue_spec_node: 
    n = get_node(glue_spec_size);  
    glue_ref_count(n)=null;
    width(n)=0; stretch(n)=0; shrink(n)=0;
    stretch_order(n)=normal; 
    shrink_order(n)=normal;
    break;
  case kern_node: 
    n = get_node(kern_node_size);  
    width(n)=null;
    break;
  case penalty_node: 
    n = get_node(penalty_node_size); 
    penalty(n)=null;
    break;
  case unset_node:        
    n = get_node(box_node_size); 
    span_count(n)=0;
    width(n) = null_flag;
    height(n) = 0;
    depth(n) = 0 ;	
    list_ptr(n) = null;
    glue_shrink(n) = 0;
    glue_stretch(n) = 0;
    glue_order(n)=normal;
    glue_sign(n)=normal;
    box_dir(n) = 0;
    break; 
  case action_node:
    n = get_node(pdf_action_size);
    pdf_action_type(n) = pdf_action_page;
    pdf_action_named_id(n)=0;
    pdf_action_id(n)=0;
    pdf_action_file(n) = null;
    pdf_action_new_window(n) = 0;
    pdf_action_tokens(n) = null;
    pdf_action_refcount(n)  = null;
    break;
  case margin_kern_node:          
    n = get_node(margin_kern_node_size); 
    margin_char(n) = null; 
    width(n) = 0;
    break;
    /* glyph_node: see above         */
  case attribute_node:
    n = get_node(attribute_node_size);
    attribute_id(n)=0;
    attribute_value(n)=0;
    break;
    /* glue_spec_node: see above         */
  case attribute_list_node:
    n = get_node(attribute_list_node_size);
    attr_list_ref(n) = 0;
    break;
  default: 
    fprintf(stdout,"<node type %s not supported yet>\n",node_names[i]);
    break;
  }  
  type(n)=i;
  if (i!=whatsit_node) {
    subtype(n)=0;    
    if (j>=0) 
      subtype(n)=j;
  }
  return n;
}

#define MAX_CHAIN_SIZE 12

memory_word *varmem = NULL;

signed char *varmem_sizes = NULL;
halfword var_mem_max = 0;
halfword rover = 0;

halfword free_chain[MAX_CHAIN_SIZE];
integer free_chain_counts[MAX_CHAIN_SIZE];

static int prealloc=0;

#define TEST_CHAIN(s)  if (free_chain[s]>=0) {	\
    assert(free_chain[s]>prealloc);		\
    assert(free_chain[s]<var_mem_max);		\
  } else { assert(free_chain[s]==null);  }


halfword 
get_node (integer s) {
  halfword p,q,r;
  integer t,x;
  if (s==010000000000)
    return max_halfword;
  
  while (1) {
    if (s<MAX_CHAIN_SIZE && free_chain[s]!=null) {
      TEST_CHAIN(s);
      r = free_chain[s];
      free_chain[s] = vlink(r);
      TEST_CHAIN(s);
      if (varmem_sizes[r]>=0) {
		fprintf(stdout,"node %d (size %d, type %d) is not free!\n",(int)r,varmem_sizes[r],type(r));
	/*assert(varmem_sizes[r]<0);*/
	break;
      }

      varmem_sizes[r] = abs(varmem_sizes[r]);
      assert(varmem_sizes[r]==s);
      break;
    } 
    t=node_size(rover);
    /* TODO, BUG: 
       
    \parshapes are not free()d right now, because this bit of code
    assumes there is only one |rover|. free() ing \parshapes would
    create a linked list of those instead, with the last free()d one
    in front.  In that case, it is likely that |t<=s|, and therefore
    the check may fail without reason. That way, a large percentage of
    \parshape requests may generate a realloc() of the whole array,
    running out of system memory quickly.
    */
    if (t>s) {
      node_size(rover) = t-s;
      r=rover+node_size(rover);
      varmem_sizes[r]=s;
      /*fprintf(stdout,"get_node+(%d), %d (%p)\n",s,r, varmem);*/
      break;
    } else {
      /*fprintf(stdout,"get_node(%d), t=%d, rover=%d, (%p)\n",s, t, rover,varmem);*/
      if (t<MAX_CHAIN_SIZE) {
	TEST_CHAIN(t);
	vlink(rover) = free_chain[t];
	free_chain[t] = rover;
	TEST_CHAIN(t);
        varmem_sizes[rover]=-t;
	q = vlink(rover);
      } else {
	q = rover;
      }
      x = (var_mem_max/5)+s+1; /* this way |s| will always fit */
      /* make sure we get up to speed quickly */
      if (var_mem_max<2500) {	x += 100000;  }
      t=var_mem_max+x;
      /*fprintf(stdout,"\nallocating %d extra nodes for %d requested\n",x,s); */
      varmem = (memory_word *)realloc(varmem,sizeof(memory_word)*t);
      varmem_sizes = (signed char *)realloc(varmem_sizes,sizeof(signed char)*t);
      if (varmem==NULL) {
		overflow(maketexstring("node memory size"),var_mem_max);
      }
      memset ((void *)(varmem+var_mem_max),0,x*sizeof(memory_word));
      memset ((void *)(varmem_sizes+var_mem_max),0,x*sizeof(signed char));
      p = var_mem_max;
      node_size(p) = x; vlink(p) = q;
      rover = p;
      var_mem_max=t;
      continue;
    }
  }
  vlink(r)=null; /* this node is now nonempty */
  type(r)=255; subtype(r)=255;
  /*fprintf(stdout,"get_node(%d), %d (%p)\n",s,r, varmem);*/
  if (s>1) { node_attr(r)=null; alink(r)=null; }
  var_used=var_used+s; /* maintain usage statistics */
  return r;
}

void
free_node (halfword p, integer s) {
  /*fprintf(stdout,"free_node(%d), %d (%p)\n",s,p,varmem);*/
  if (s>MAX_CHAIN_SIZE) {
    /*fprintf(stdout,"node %d will not be free()d!\n",p,varmem_sizes[p],s);*/
    return;
  }
  if (p<=prealloc) { 
    fprintf(stdout,"node %d (type %d) should not be freed!\n",(int)p, type(p));
    return;
  }

  if (varmem_sizes[p]<=0) {
    fprintf(stdout,"node %d (size %d, type %d) is already free!\n",(int)p,varmem_sizes[p],type(p));
    /*assert(varmem_sizes[p]>0);*/
    return;
  }
  assert(varmem_sizes[p]==s);
  if (s<MAX_CHAIN_SIZE) {
    varmem_sizes[p] = -varmem_sizes[p];
    TEST_CHAIN(s);
    /* this seemed like an interesting idea for debugging, but it doesn't work
       (found too late) */
    /* type(p) = 254; */
    vlink(p) = free_chain[s];
    free_chain[s] = p;
    TEST_CHAIN(s);
  } else {
    varmem_sizes[p] = -varmem_sizes[p];
    node_size(p)=s; vlink(p)=rover;
    rover=p;
  }
  var_used=var_used-s; /* maintain statistics */
}

void
init_node_mem (halfword prealloced, halfword t) {
  int i;
  prealloc=prealloced;
  for (i=0;i<MAX_CHAIN_SIZE;i++) {
    free_chain[i]=null;
  }
  varmem = (memory_word *)realloc(varmem,sizeof(memory_word)*t);
  varmem_sizes = (signed char *)realloc(varmem_sizes,sizeof(signed char)*t);
  if (varmem==NULL) {
    overflow("node memory size",var_mem_max);
  }
  memset ((void *)varmem,0,sizeof(memory_word)*t);
  memset ((void *)varmem_sizes,0,sizeof(signed char)*t);
  var_mem_max=t; 
  rover = prealloced+1; vlink(rover) = null;
  node_size(rover)=(t-prealloced-1);
  var_used = 0;
}

void
print_node_mem_stats (void) {
  int i,a;
  halfword j;
  a = node_size(rover);
  fprintf(stdout,"\nnode memory in use: %d words out of %d (%d untouched)\n",
		  (int)(var_used+prealloc), (int)var_mem_max, (int)a);
  fprintf(stdout,"currently available: ");
  for (i=1;i<MAX_CHAIN_SIZE;i++) {
	j = free_chain[i];
    free_chain_counts[i]=0;
    while(j!=null) {
	  free_chain_counts[i] ++;
	  j = vlink(j);
	}
	if (free_chain_counts[i]>0)
	  fprintf(stdout,"%s%d:%d",(i>1 ? ", " : ""),i, (int)free_chain_counts[i]);
  }
  fprintf(stdout," nodes\n");
}

void
dump_node_mem (void) {
  dump_int(var_mem_max);
  dump_int(rover);
  dump_things(varmem[0],var_mem_max);
  dump_things(varmem_sizes[0],var_mem_max);
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
  varmem_sizes = xmallocarray (signed char, var_mem_max);
  memset ((void *)varmem_sizes,0,var_mem_max*sizeof(signed char));
  undump_things(varmem_sizes[0],var_mem_max);
  undump_things(free_chain[0],MAX_CHAIN_SIZE);
  undump_int(var_used); 
  undump_int(prealloc);
}


void
lua_node_filter_s (int filterid, char *extrainfo, halfword head_node, halfword *tail_node) {
  halfword ret,r;  
  int a;
  integer callback_id ; 
  int glyph_count;
  int nargs = 2;
  lua_State *L = Luas[0];
  callback_id = callback_defined(filterid);
  if (head_node==null || vlink(head_node)==null)
	return;
  if (callback_id==0) {
    return;
  }
  lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
  lua_rawgeti(L,-1, callback_id);
  if (!lua_isfunction(L,-1)) {
    lua_pop(L,2);
    return;
  }
  nodelist_to_lua(L,vlink(head_node)); /* arg 1 */
  lua_pushstring(L,extrainfo);         /* arg 2 */
  if (filterid==linebreak_filter_callback) {
    glyph_count = 0;
    r = vlink(head_node);
    while (r!=null) {
      if (type(r)==glyph_node) 
	glyph_count++;
      r=vlink(r);
    }
    nargs++;
    lua_pushnumber(L,glyph_count); /* arg 3 */
  }
  if (lua_pcall(L,nargs,1,0) != 0) { /* no arg, 1 result */
    fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
    lua_pop(L,2);
    error();
    return;
  }
  if (lua_isboolean(L,-1)) {
    if (lua_toboolean(L,-1)!=1) {
      flush_node_list(vlink(head_node));
      vlink(head_node) = null;
    }
  } else {
    a = nodelist_from_lua(L);
    vlink(head_node)= a;
  }
  lua_pop(L,2); /* result and callback container table */
  ret = vlink(head_node); 
  if (ret!=null) {
    while (vlink(ret)!=null)
      ret=vlink(ret); 
    *tail_node=ret;
  } else {
    *tail_node=head_node;
  }
  return ;
}

void
lua_node_filter (int filterid, int extrainfo, halfword head_node, halfword *tail_node) {
  lua_node_filter_s(filterid, group_code_names[extrainfo], head_node, tail_node);
  return ;
}



halfword
lua_hpack_filter (halfword head_node, scaled size, int pack_type, int extrainfo) {
  halfword ret,r;  
  integer callback_id ; 
  int glyph_count;
  lua_State *L = Luas[0];
  callback_id = callback_defined(hpack_filter_callback);
  if (head_node==null || vlink(head_node)==null)
	return head_node;
  if (callback_id==0) {
    return head_node;
  }
  lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
  lua_rawgeti(L,-1, callback_id);
  if (!lua_isfunction(L,-1)) {
    lua_pop(L,2);
    return head_node;
  }

  r =head_node;
  glyph_count = 0;
  while (r!=null) {
    if (type(r)==glyph_node) 
      glyph_count++;
    r=vlink(r);
  }
  nodelist_to_lua(L,head_node);
  lua_pushnumber(L,size);
  lua_pushstring(L,pack_type_name[pack_type]);
  lua_pushstring(L,group_code_names[extrainfo]);
  lua_pushnumber(L,glyph_count);
  if (lua_pcall(L,5,1,0) != 0) { /* no arg, 1 result */
    fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
    lua_pop(L,2);
    error();
    return head_node;
  }
  ret = head_node;
  if (lua_isboolean(L,-1)) {
    if (lua_toboolean(L,-1)!=1) {
      flush_node_list(head_node);
      ret = null;
    }
  } else {
    ret = nodelist_from_lua(L);
  }
  lua_pop(L,2); /* result and callback container table */
  /*  lua_gc(L,LUA_GCSTEP, LUA_GC_STEP_SIZE);*/
  return ret;
}

halfword
lua_vpack_filter (halfword head_node, scaled size, int pack_type, scaled maxd, int extrainfo) {
  halfword ret;  
  integer callback_id ; 
  lua_State *L = Luas[0];
  if (head_node==null || vlink(head_node)==null)
	return head_node;
  if (strcmp("output",group_code_names[extrainfo])==0) {
    callback_id = callback_defined(pre_output_filter_callback);
  } else {
    callback_id = callback_defined(vpack_filter_callback);
  }
  if (callback_id==0) {
    return head_node;
  }
  lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
  lua_rawgeti(L,-1, callback_id);
  if (!lua_isfunction(L,-1)) {
    lua_pop(L,2);
    return head_node;
  }
  nodelist_to_lua(L,head_node);
  lua_pushnumber(L,size);
  lua_pushstring(L,pack_type_name[pack_type]);
  lua_pushnumber(L,maxd);
  lua_pushstring(L,group_code_names[extrainfo]);
  if (lua_pcall(L,5,1,0) != 0) { /* no arg, 1 result */
    fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
    lua_pop(L,2);
    error();
    return head_node;
  }
  ret = head_node;
  if (lua_isboolean(L,-1)) {
    if (lua_toboolean(L,-1)!=1) {
      flush_node_list(head_node);
      ret = null;
    }
  } else {
    ret = nodelist_from_lua(L);
  }
  lua_pop(L,2); /* result and callback container table */
  /*  lua_gc(L,LUA_GCSTEP, LUA_GC_STEP_SIZE);*/
  return ret;
}

/* This callback does not actually work yet! */

boolean 
lua_hyphenate_callback (int callback_id, int lang, halfword ha, halfword hb) {
  halfword ret,p,q,r;
  lua_State *L = Luas[0];

  lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
  lua_rawgeti(L,-1, callback_id);
  if (!lua_isfunction(L,-1)) {
    lua_pop(L,2);
    return false;
  } 

  p = vlink(hb);  /* for safe keeping */
  vlink(hb)=null; 
  r=vlink(ha); 
  nodelist_to_lua(L,r);
  lua_pushnumber(L,lang);
  if (lua_pcall(L,2,1,0) != 0) { /* no arg, 1 result */
    fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
    lua_pop(L,2);
    error();
    return false;
  }
  ret = nodelist_from_lua(L);
  if (ret!=null) {
    flush_node_list(r);
    vlink(ha)=ret;
    q = ret;
    while(vlink(q)!=null) {
      q=vlink(q);
    }
    vlink(q) = p;
  } else {
    vlink(hb) = p;
  }
  lua_pop(L,2); /* result and callback container table */
  return true;
}

/* This is a quick hack to fix etex's \lastnodetype now that
 * there are many more visible node types. TODO: check the
 * eTeX manual for the expected return values.
 */

int 
visible_last_node_type (int n) {
  int i = type(n);
  if ((i!=math_node) && (i<=unset_node))  
    return i+1;
  if (i==glyph_node)
    return -1; 
  if (i==whatsit_node && subtype(n)==local_par_node)
    return -1;  
  if (i==255)
    return -1 ; /* this is not right, probably dir nodes! */
  return last_known_node +1 ;
}

