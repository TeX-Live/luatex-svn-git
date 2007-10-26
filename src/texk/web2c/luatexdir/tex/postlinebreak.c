/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"

/* So far we have gotten a little way into the |line_break| routine, having
covered its important |try_break| subroutine. Now let's consider the
rest of the process.

The main loop of |line_break| traverses the given hlist,
starting at |vlink(temp_head)|, and calls |try_break| at each legal
breakpoint. A variable called |auto_breaking| is set to true except
within math formulas, since glue nodes are not legal breakpoints when
they appear in formulas.

The current node of interest in the hlist is pointed to by |cur_p|. Another
variable, |prev_p|, is usually one step behind |cur_p|, but the real
meaning of |prev_p| is this: If |type(cur_p)=glue_node| then |cur_p| is a legal
breakpoint if and only if |auto_breaking| is true and |prev_p| does not
point to a glue node, penalty node, explicit kern node, or math node.

*/

halfword 
do_push_dir_node (halfword p, halfword a ) {
  halfword n;
  assert (type(a)==whatsit_node && subtype(a)==dir_node);
  n = copy_node(a);
  vlink(n) = p; 
  return n;
}

halfword 
do_pop_dir_node ( halfword p ) {
  halfword n = vlink(p); 
  free_node(p,dir_node_size);
  return n;
}

halfword add_dir_nodes (halfword r, halfword q) {
  halfword p;
  for (p=dir_ptr;p!=null;p=vlink(p)) {
    halfword tmp=new_dir(dir_dir(p)-64);
    couple_nodes(tmp,q);
    couple_nodes(r,tmp);
    r=vlink(r);
  }
  return r;
}

void
insert_dir_nodes_at_end (halfword *qq, halfword *rr, halfword final_par_glue) {
  halfword q,r; /* temporary registers for list manipulation */
  r = *rr;
  q = *qq;
  if (dir_ptr!=null) {
    if (vlink(r)==q) {
      assert(r!=null);
      *rr = add_dir_nodes(r,q);
    } else if (r==final_par_glue) {
      assert(alink(r)!=null && vlink(alink(r))==r);
      (void)add_dir_nodes(alink(r), q);
    } else {
      assert(q!=null);
      *qq = add_dir_nodes(q, vlink(q));
      *rr = *qq;
    }
  }
  return;
}

/* The total number of lines that will be set by |post_line_break|
is |best_line-prev_graf-1|. The last breakpoint is specified by
|break_node(best_bet)|, and this passive node points to the other breakpoints
via the |prev_break| links. The finishing-up phase starts by linking the
relevant passive nodes in forward order, changing |prev_break| to
|next_break|. (The |next_break| fields actually reside in the same memory
space as the |prev_break| fields did, but we give them a new name because
of their new significance.) Then the lines are justified, one by one.

The |post_line_break| must also keep an dir stack, so that it can
output end direction instructions at the ends of lines
and begin direction instructions at the beginnings of lines.

*/

#define next_break prev_break /*new name for |prev_break| after links are reversed*/

#define append_list(a,b)						\
  { vlink(cur_list.tail_field)=vlink((a)); cur_list.tail_field = b; }

#define left_skip_code 7 /*glue at left of justified lines*/
#define right_skip_code 8 /*glue at right of justified lines*/

/* the ints are actually halfwords */
void ext_post_line_break(boolean d, 
			 int right_skip,
			 int left_skip,
			 int pdf_protrude_chars,
			 halfword par_shape_ptr,
			 int pdf_adjust_spacing,
			 int pdf_each_line_height,
			 int pdf_each_line_depth,
			 int pdf_first_line_height,
			 int pdf_last_line_depth,
			 halfword inter_line_penalties_ptr,
			 int inter_line_penalty,
			 int club_penalty,
			 halfword club_penalties_ptr,
			 halfword display_widow_penalties_ptr,
			 halfword widow_penalties_ptr,
			 int display_widow_penalty,
			 int widow_penalty,
			 int broken_penalty,
			 halfword final_par_glue,
			 halfword best_bet,
			 halfword last_special_line,
			 scaled second_width,
			 scaled second_indent,
			 scaled first_width,
			 scaled first_indent,
			 halfword best_line ) {

  halfword q,r,s; /* temporary registers for list manipulation */
  halfword  p, k;
  scaled w;
  boolean glue_break; /* was a break at glue? */
  boolean disc_break; /*was the current break at a discretionary node?*/
  boolean post_disc_break; /*and did it have a nonempty post-break part?*/
  scaled cur_width; /*width of line number |cur_line|*/
  scaled cur_indent; /*left margin of line number |cur_line|*/
  integer pen; /*use when calculating penalties between lines */
  halfword cur_p; /* cur_p, but localized */
  halfword cur_line; /*the current line number being justified*/

  dir_ptr = cur_list.dirs_field;
  /* @<Reverse the links of the relevant passive nodes, setting |cur_p| to 
     the first breakpoint@>; */
  /* The job of reversing links in a list is conveniently regarded as the job
     of taking items off one stack and putting them on another. In this case we
     take them off a stack pointed to by |q| and having |prev_break| fields;
     we put them on a stack pointed to by |cur_p| and having |next_break| fields.
     Node |r| is the passive node being moved from stack to stack.
  */
  q=break_node(best_bet); 
  cur_p=null;
  do { 
    r=q; 
    q=prev_break(q); 
    next_break(r)=cur_p; 
    cur_p=r;
  } while (q!=null);  
  /* |cur_p| is now the first breakpoint; */

  cur_line=cur_list.pg_field+1; /* prevgraf+1 */

  do {
    /* @<Justify the line ending at breakpoint |cur_p|, and append it to the
       current vertical list, together with associated penalties and other
       insertions@>;	*/
    /* The current line to be justified appears in a horizontal list starting
       at |vlink(temp_head)| and ending at |cur_break(cur_p)|. If |cur_break(cur_p)| is
       a glue node, we reset the glue to equal the |right_skip| glue; otherwise
       we append the |right_skip| glue at the right. If |cur_break(cur_p)| is a
       discretionary node, we modify the list so that the discretionary break
       is compulsory, and we set |disc_break| to |true|. We also append
       the |left_skip| glue at the left of the line, unless it is zero. */
    
    /* DIR: Insert dir nodes at the beginning of the current line;*/
    for (q=dir_ptr;q!=null;q=vlink(q)) {
      halfword tmp=new_dir(dir_dir(q));
      couple_nodes(tmp,vlink(temp_head));
      couple_nodes(temp_head,tmp);
    }
    /* DIR: Adjust the dir stack based on dir nodes in this line; */
    if (dir_ptr!=null) {
      flush_node_list(dir_ptr); dir_ptr=null;
    }
    q=vlink(temp_head);
    while (q!=cur_break(cur_p)) {
      if (type(q)==whatsit_node && subtype(q)==dir_node) {
	if (dir_dir(q)>=0) {
	  dir_ptr = do_push_dir_node(dir_ptr,q);
	} else if (dir_ptr!=null) {
	  if (dir_dir(dir_ptr)==(dir_dir(q)+64)) {
	    dir_ptr = do_pop_dir_node(dir_ptr);
	  }
	}
      }
      if (q==null || vlink(q)==0) {
	break;
      }
      q=vlink(q);
    }

    /* Modify the end of the line to reflect the nature of the break and to include
       \.{\\rightskip}; also set the proper value of |disc_break|; */
    /* At the end of the following code, |q| will point to the final node on the
       list about to be justified. */
    assert(q==cur_break(cur_p)); 
    disc_break=false; 
    post_disc_break=false;
    glue_break = false;

    if (q!=null && type(q)==glue_node) {
      r=alink(q);
      assert(vlink(r)==q);
      /* @<DIR: Insert dir nodes at the end of the current line@>;*/
      insert_dir_nodes_at_end(&q,&r,final_par_glue);
      if (passive_right_box(cur_p)!=null) {
	s=copy_node_list(passive_right_box(cur_p));
	vlink(r)=s;
	vlink(s)=q;
      }
      delete_glue_ref(glue_ptr(q));
      glue_ptr(q)=right_skip;
      subtype(q)=right_skip_code+1; 
      incr(glue_ref_count(right_skip));
      glue_break = true;
    } else {
      if (q==null) { 
	q=temp_head;
	while (vlink(q)!=null)  
	  q=vlink(q);
      } else if (type(q)==disc_node) {
	/* @<Change discretionary to compulsory and set |disc_break:=true|@>*/
	r = vlink(q);
	if (vlink(no_break(q))!=null) {
	  flush_node_list(vlink(no_break(q))); 
	  vlink(no_break(q))=null;
	  tlink(no_break(q))=null;
	}
	if (vlink(post_break(q))!=null) {
	  /* @<Transplant the post-break list@>;*/
	  /* We move the post-break list from inside node |q| to the main list by
	     re\-attaching it just before the present node |r|, then resetting |r|. */
	  couple_nodes(q,vlink(post_break(q)));
	  couple_nodes(tlink(post_break(q)),r);
	  vlink(post_break(q))=null; 
	  tlink(post_break(q))=null; 
	  r=vlink(q);
	}
	if (vlink(pre_break(q))!=null) {
	  /* @<Transplant the pre-break list@>;*/
	  /* We move the pre-break list from inside node |q| to the main list by
	     re\-attaching it just after the present node |q|, then resetting |q|. */
	  couple_nodes(q,vlink(pre_break(q)));
	  couple_nodes(tlink(pre_break(q)),r);
	  vlink(pre_break(q))=null; 
	  tlink(pre_break(q))=null; 
	  q=alink(r);
	}
	disc_break=true;
      } else if (type(q)==kern_node) {
	width(q)=0;
      } else if (type(q)==math_node) {
	surround(q)=0;
      }
      r=q;
      /* @<DIR: Insert dir nodes at the end of the current line@>;*/
      insert_dir_nodes_at_end(&q,&r,final_par_glue);
      if (passive_right_box(cur_p)!=null) {
	r=copy_node_list(passive_right_box(cur_p));
	vlink(r)=vlink(q);
	vlink(q)=r;
	q=r;
      }
    }
    /* at this point |q| is the rightmost breakpoint; the only exception is the case
       of a discretionary break with non-empty |pre_break|, then |q| has been changed
       to the last node of the |pre_break| list */
    if (pdf_protrude_chars > 0) {
      halfword ptmp;
      if (disc_break && (is_char_node(q) || (type(q) != disc_node))) {
        p = q; /* |q| has been reset to the last node of |pre_break| */
        ptmp = p;
      } else {
        p = alink(q); /* get |vlink(p) = q| */
	assert(vlink(p)==q);
        ptmp = p;
        p = find_protchar_right(vlink(temp_head), p);
      }
      w = char_pw(p, right_side);
      if (w!=0) { /* we have found a marginal kern, append it after |ptmp| */
	k = new_margin_kern(-w, last_rightmost_char, right_side);
        vlink(k) = vlink(ptmp);
        vlink(ptmp) = k;
        if (ptmp == q) 
	  q = vlink(q);
      }
    }
    /* if |q| was not a breakpoint at glue and has been reset to |rightskip| then
       we append |rightskip| after |q| now */
    if (!glue_break) {
      /* @<Put the \(r)\.{\\rightskip} glue after node |q|@>;*/
      r=new_param_glue(right_skip_code); 
      vlink(r)=vlink(q); 
      vlink(q)=r; 
      q=r;
    }

    /* /Modify the end of the line to reflect the nature of the break and to include
       \.{\\rightskip}; also set the proper value of |disc_break|; */

    /* Put the \(l)\.{\\leftskip} glue at the left and detach this line;*/
    /* The following code begins with |q| at the end of the list to be
       justified. It ends with |q| at the beginning of that list, and with
       |vlink(temp_head)| pointing to the remainder of the paragraph, if any. */
    r=vlink(q); 
    vlink(q)=null; 
    q=vlink(temp_head); 
    vlink(temp_head)=r;
    if (passive_left_box(cur_p)!=null && passive_left_box(cur_p)!=0) {
      /* omega bits: */
      r=copy_node_list(passive_left_box(cur_p));
      s=vlink(q);
      vlink(r)=q;
      q=r;
      if ((cur_line==cur_list.pg_field+1) && (s!=null)) {
	if (type(s)==hlist_node) {
	  if (list_ptr(s)==null) {
	    q=vlink(q);
	    vlink(r)=vlink(s);
	    vlink(s)=r;
	  }
	}
      }
    }
    /*at this point |q| is the leftmost node; all discardable nodes have been discarded */
    if (pdf_protrude_chars > 0) {
      p = q;
      p = find_protchar_left(p, false); /* no more discardables */
      w =  char_pw(p, left_side);
      if (w != 0) {
        k = new_margin_kern(-w, last_leftmost_char, left_side);
        vlink(k) = q;
        q = k;
      }
    };
    if (left_skip!=zero_glue) {
      r=new_param_glue(left_skip_code);
      vlink(r)=q; 
      q=r;
    }
    /* /Put the \(l)\.{\\leftskip} glue at the left and detach this line;*/

    /* Call the packaging subroutine, setting |just_box| to the justified box;*/
    /* Now |q| points to the hlist that represents the current line of the
       paragraph. We need to compute the appropriate line width, pack the
       line into a box of this size, and shift the box by the appropriate
       amount of indentation. */
    if (cur_line>last_special_line) {
      cur_width=second_width; 
      cur_indent=second_indent;
    } else if (par_shape_ptr==null) {
      cur_width=first_width; 
      cur_indent=first_indent;
    } else  { 
      cur_width=varmem[(par_shape_ptr+2*cur_line)].cint;
      cur_indent=varmem[(par_shape_ptr+2*cur_line-1)].cint;
    }
    adjust_tail=adjust_head;
    pack_direction=paragraph_dir;
    pre_adjust_tail = pre_adjust_head;
    if (pdf_adjust_spacing > 0 ) {
      just_box = hpack(q, cur_width, cal_expand_ratio);
    } else {
      just_box = hpack(q, cur_width, exactly);
    }
    shift_amount(just_box)=cur_indent;
    /* /Call the packaging subroutine, setting |just_box| to the justified box;*/

    /* Append the new box to the current vertical list, followed by the list of
       special nodes taken out of the box by the packager;*/
    if (pdf_each_line_height!=0)
      height(just_box) = pdf_each_line_height;
    if (pdf_each_line_depth != 0)
      depth(just_box) = pdf_each_line_depth;
    if ((pdf_first_line_height != 0) && (cur_line ==  cur_list.pg_field + 1))
      height(just_box) = pdf_first_line_height;
    if ((pdf_last_line_depth != 0) && (cur_line + 1 == best_line))
      depth(just_box) = pdf_last_line_depth;

    if (pre_adjust_head != pre_adjust_tail) 
      append_list(pre_adjust_head,pre_adjust_tail);
    pre_adjust_tail = null;
    append_to_vlist(just_box);
    if (adjust_head != adjust_tail) 
      append_list(adjust_head,adjust_tail);
    adjust_tail = null;

    /* /Append the new box to the current vertical list, followed by the list of
       special nodes taken out of the box by the packager;*/

    /* Append a penalty node, if a nonzero penalty is appropriate */
    /* Penalties between the lines of a paragraph come from club and widow lines,
       from the |inter_line_penalty| parameter, and from lines that end at
       discretionary breaks.  Breaking between lines of a two-line paragraph gets
       both club-line and widow-line penalties. The local variable |pen| will
       be set to the sum of all relevant penalties for the current line, except
       that the final line is never penalized. */
    if (cur_line+1!=best_line) {
      q=inter_line_penalties_ptr;
      if (q!=null) {
	r=cur_line;
	if (r>penalty(q))
	  r=penalty(q);
	pen=penalty(q+r);
      } else {
	if (passive_pen_inter(cur_p)!=0) {
	  pen=passive_pen_inter(cur_p);
	} else {
	  pen=inter_line_penalty;
	}
      }
      q=club_penalties_ptr;
      if (q!=null) {
	r=cur_line-cur_list.pg_field; /* prevgraf */
	if (r>penalty(q))  
	  r=penalty(q);
	pen += penalty(q+r);
      } else if (cur_line==cur_list.pg_field+1) { /* prevgraf */
	pen += club_penalty;
      }
      if (d)
	q = display_widow_penalties_ptr;
      else 
	q = widow_penalties_ptr;
      if (q!=null) {
	r=best_line-cur_line-1;
	if (r>penalty(q)) 
	  r=penalty(q);
	pen += penalty(q+r);
      } else if (cur_line+2==best_line) {
	if (d)
	  pen += display_widow_penalty;
	else
	  pen += widow_penalty;
      }
      if (disc_break) {
	if (passive_pen_broken(cur_p)!=0) {
	  pen += passive_pen_broken(cur_p);
	} else {
	  pen += broken_penalty;
	}
      }
      if (pen!=0) {
	r=new_penalty(pen);
	vlink(cur_list.tail_field)=r; 
	cur_list.tail_field=r;
      }
    }
    /* /Append a penalty node, if a nonzero penalty is appropriate */
    
    /* /Justify the line ending at breakpoint |cur_p|, and append it to the
       current vertical list, together with associated penalties and other
       insertions@>;   */
    incr(cur_line); 
    cur_p=next_break(cur_p);
    if (cur_p!=null && !post_disc_break) {
      /* @<Prune unwanted nodes at the beginning of the next line@>; */
      /* Glue and penalty and kern and math nodes are deleted at the
	 beginning of a line, except in the anomalous case that the
	 node to be deleted is actually one of the chosen
	 breakpoints. Otherwise the pruning done here is designed to
	 match the lookahead computation in |try_break|, where the
	 |break_width| values are computed for non-discretionary
	 breakpoints. */	  
      r=temp_head;
      while(1) {
	q=vlink(r);
	if (q==cur_break(cur_p) || is_char_node(q))
	  break;
	if (!((type(q)==whatsit_node)&&(subtype(q)==local_par_node))) {
	  if (non_discardable(q) 
	      || (type(q)==kern_node && subtype(q)!=explicit ))
	    break;
	}
	r=q; 
      };
      if (r!=temp_head) { 
	vlink(r)=null; 
	flush_node_list(vlink(temp_head));
	vlink(temp_head)=q;
      }
    }
  } while (cur_p!=null);
  if ((cur_line!=best_line)||(vlink(temp_head)!=null)) 
	confusion(maketexstring("line breaking"));
  cur_list.pg_field=best_line-1;  /* prevgraf */
  cur_list.dirs_field=dir_ptr; /* dir_save */
}

