/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"



integer internal_pen_inter; /* running \.{\\localinterlinepenalty} */
integer internal_pen_broken; /* running \.{\\localbrokenpenalty} */
halfword internal_left_box; /* running \.{\\localleftbox} */
integer internal_left_box_width; /* running \.{\\localleftbox} width */
halfword init_internal_left_box; /* running \.{\\localleftbox} */
integer init_internal_left_box_width; /* running \.{\\localleftbox} width */
halfword internal_right_box; /* running \.{\\localrightbox} */
integer internal_right_box_width; /* running \.{\\localrightbox} width */

scaled disc_width [10]; /* the length of discretionary material preceding a break*/

/* As we consider various ways to end a line at |cur_p|, in a given line number
   class, we keep track of the best total demerits known, in an array with
   one entry for each of the fitness classifications. For example,
   |minimal_demerits[tight_fit]| contains the fewest total demerits of feasible
   line breaks ending at |cur_p| with a |tight_fit| line; |best_place[tight_fit]|
   points to the passive node for the break before~|cur_p| that achieves such
   an optimum; and |best_pl_line[tight_fit]| is the |line_number| field in the
   active node corresponding to |best_place[tight_fit]|. When no feasible break
   sequence is known, the |minimal_demerits| entries will be equal to
   |awful_bad|, which is $2^{30}-1$. Another variable, |minimum_demerits|,
   keeps track of the smallest value in the |minimal_demerits| array.
*/

typedef enum {
  very_loose_fit = 0,
  loose_fit,
  decent_fit,
  tight_fit } fitness_value ;

integer minimal_demerits[4]; /* best total demerits known for current 
				line class and position, given the fitness */
integer minimum_demerits; /* best total demerits known for current line class
			     and position */
halfword best_place[4]; /* how to achieve  |minimal_demerits| */
halfword best_pl_line[4]; /*corresponding line number*/


/*
 The length of lines depends on whether the user has specified
\.{\\parshape} or \.{\\hangindent}. If |par_shape_ptr| is not null, it
points to a $(2n+1)$-word record in |mem|, where the |vinfo| in the first
word contains the value of |n|, and the other $2n$ words contain the left
margins and line lengths for the first |n| lines of the paragraph; the
specifications for line |n| apply to all subsequent lines. If
|par_shape_ptr=null|, the shape of the paragraph depends on the value of
|n=hang_after|; if |n>=0|, hanging indentation takes place on lines |n+1|,
|n+2|, \dots, otherwise it takes place on lines 1, \dots, $\vert
n\vert$. When hanging indentation is active, the left margin is
|hang_indent|, if |hang_indent>=0|, else it is 0; the line length is
$|hsize|-\vert|hang_indent|\vert$. The normal setting is
|par_shape_ptr=null|, |hang_after=1|, and |hang_indent=0|.
Note that if |hang_indent=0|, the value of |hang_after| is irrelevant.
@^length of lines@> @^hanging indentation@>
*/

halfword easy_line; /*line numbers |>easy_line| are equivalent in break nodes*/
halfword last_special_line; /*line numbers |>last_special_line| all have the same width */
scaled first_width; /*the width of all lines |<=last_special_line|, if
		      no \.{\\parshape} has been specified*/
scaled second_width; /*the width of all lines |>last_special_line|*/
scaled first_indent; /*left margin to go with |first_width|*/
scaled second_indent; /*left margin to go with |second_width|*/


/* \TeX\ makes use of the fact that |hlist_node|, |vlist_node|,
   |rule_node|, |ins_node|, |mark_node|, |adjust_node|, 
   |disc_node|, |whatsit_node|, and |math_node| are at the low end of the
   type codes, by permitting a break at glue in a list if and only if the
   |type| of the previous node is less than |math_node|. Furthermore, a
   node is discarded after a break if its type is |math_node| or~more.
*/

#define precedes_break(a) (type((a))<math_node)
#define non_discardable(a) (type((a))<math_node)

#define do_all_six(a) a(1);a(2);a(3);a(4);a(5);a(6);a(7)
#define do_seven_eight(a) if (pdf_adjust_spacing > 1) { a(8);a(9); }
#define do_all_eight(a) do_all_six(a); do_seven_eight(a)
#define do_one_seven_eight(a) a(1); do_seven_eight(a)

#define store_background(a) {active_width[a]=background[a];}

#define act_width active_width[1] /*length from first active node to current node*/

#define kern_break() {							\
    if ((!is_char_node(vlink(cur_p))) && auto_breaking)			\
      if (type(vlink(cur_p))==glue_node)				\
	ext_try_break(0,unhyphenated, pdf_adjust_spacing,		\
		      par_shape_ptr, adj_demerits,			\
		      tracing_paragraphs, pdf_protrude_chars,		\
		      line_penalty, last_line_fit,			\
		      double_hyphen_demerits,  final_hyphen_demerits);	\
    if (type(cur_p)!=math_node) act_width+=width(cur_p);		\
    else                        act_width+=surround(cur_p);		\
  }


#define clean_up_the_memory() {					\
    q=vlink(active);						\
    while (q!=last_active) {					\
      cur_p=vlink(q);						\
      if (type(q)==delta_node) 	free_node(q,delta_node_size);	\
      else                	free_node(q,active_node_size);	\
      q=cur_p;							\
    }								\
    q=passive;							\
    while (q!=null) {						\
      cur_p=vlink(q);						\
      free_node(q,passive_node_size);				\
      q=cur_p;							\
    }								\
  }

#define inf_bad 10000 /* infinitely bad value */

boolean do_last_line_fit; /* special algorithm for last line of paragraph? */
small_number active_node_size; /* number of words in active nodes */
scaled fill_width[4]; /* infinite stretch components of  |par_fill_skip| */
scaled best_pl_short[4]; /* |shortfall|  corresponding to |minimal_demerits| */
scaled best_pl_glue[4]; /*corresponding glue stretch or shrink */


#define active_node_size_normal 3 /* number of words in normal active nodes*/
#define fitness subtype /*|very_loose_fit..tight_fit| on final line for this break*/
#define break_node(a) vlink((a)+1) /*pointer to the corresponding passive node */
#define line_number(a) vinfo((a)+1) /*line that begins at this breakpoint*/
#define total_demerits(a) varmem[(a)+2].cint /* the quantity that \TeX\ minimizes*/

#define unhyphenated 50 /* the |type| of a normal active break node */
#define hyphenated 51 /* the |type| of an active node that breaks at a |disc_node| */
#define last_active active /* the active list ends where it begins */

#define active_node_size_extended 5 /*number of words in extended active nodes*/
#define active_short(a) varmem[(a)+3].cint /* |shortfall| of this line */
#define active_glue(a) varmem[(a)+4].cint /*corresponding glue stretch or shrink*/

#define awful_bad 07777777777 /* more than a billion demerits */

#define before 0 /* |subtype| for math node that introduces a formula */
#define after 1 /* |subtype| for math node that winds up a formula */

#define do_nothing 
#define null_font 0

#define push_dir_node(a)			\
  { dir_tmp=get_node(dir_node_size);		\
    type(dir_tmp)=whatsit_node;			\
    subtype(dir_tmp)=dir_node;			\
    dir_dir(dir_tmp)=dir_dir((a));		\
    dir_level(dir_tmp)=dir_level((a));		\
    dir_dvi_h(dir_tmp)=dir_dvi_h((a));		\
    dir_dvi_ptr(dir_tmp)=dir_dvi_ptr((a));	\
    vlink(dir_tmp)=dir_ptr; dir_ptr=dir_tmp;	\
  }

#define pop_dir_node()				\
  { dir_tmp=dir_ptr;				\
    dir_ptr=vlink(dir_tmp);			\
    free_node(dir_tmp,dir_node_size);		\
  }


#define dir_parallel(a,b) (((a) % 2)==((b) % 2))
#define dir_orthogonal(a,b) (((a) % 2)!=((b) % 2))

#define is_rotated(a) dir_parallel(dir_secondary[(a)],dir_tertiary[(a)])

#define check_shrinkage(a) if ((shrink_order(a)!=normal)&&(shrink(a)!=0)) a=finite_shrink(a)


#define reset_disc_width(a) disc_width[(a)] = 0

#define add_disc_width_to_break_width(a)     break_width[(a)] += disc_width[(a)]
#define add_disc_width_to_active_width(a)    active_width[(a)] += disc_width[(a)]
#define sub_disc_width_from_active_width(a)  active_width[(a)] -= disc_width[(a)]

#define add_char_shrink(a,b)  a += char_shrink((b))
#define add_char_stretch(a,b) a += char_stretch((b))
#define sub_char_shrink(a,b)  a -= char_shrink((b))
#define sub_char_stretch(a,b) a -= char_stretch((b))

#define add_kern_shrink(a,b)  a += kern_shrink((b))
#define add_kern_stretch(a,b) a += kern_stretch((b))
#define sub_kern_shrink(a,b)  a -= kern_shrink((b))
#define sub_kern_stretch(a,b) a -= kern_stretch((b))


#define update_width(a) cur_active_width[a] += varmem[(r+(a))].cint

#define set_break_width_to_background(a) break_width[(a)]=background[(a)]

#define convert_to_break_width(a)					\
  varmem[(prev_r+(a))].cint -= (cur_active_width[(a)]+break_width[(a)])

#define store_break_width(a)      active_width[(a)]=break_width[(a)]

#define new_delta_to_break_width(a)							\
  varmem[(q+(a))].cint=break_width[(a)]-cur_active_width[(a)]

#define new_delta_from_break_width(a)						\
  varmem[(q+(a))].cint=cur_active_width[(a)]-break_width[(a)]

#define copy_to_cur_active(a) cur_active_width[(a)]=active_width[(a)]

/* When we insert a new active node for a break at |cur_p|, suppose this
   new node is to be placed just before active node |a|; then we essentially
   want to insert `$\delta\,|cur_p|\,\delta^\prime$' before |a|, where
   $\delta=\alpha(a)-\alpha(|cur_p|)$ and $\delta^\prime=\alpha(|cur_p|)-\alpha(a)$
   in the notation explained above.  The |cur_active_width| array now holds
   $\gamma+\beta(|cur_p|)-\alpha(a)$; so $\delta$ can be obtained by
   subtracting |cur_active_width| from the quantity $\gamma+\beta(|cur_p|)-
   \alpha(|cur_p|)$. The latter quantity can be regarded as the length of a
   line ``from |cur_p| to |cur_p|''; we call it the |break_width| at |cur_p|.
   
   The |break_width| is usually negative, since it consists of the background
   (which is normally zero) minus the width of nodes following~|cur_p| that are
   eliminated after a break. If, for example, node |cur_p| is a glue node, the
   width of this glue is subtracted from the background; and we also look
   ahead to eliminate all subsequent glue and penalty and kern and math
   nodes, subtracting their widths as well.
     
   Kern nodes do not disappear at a line break unless they are |explicit|.
*/

/* assigned-to globals: 
   prev_char_p
   break_width[] 
*/
/* used globals: 
   disc_width[]
   line_break_dir
*/
void 
compute_break_width (int break_type, int pdf_adjust_spacing, halfword p) {
  halfword s; /*runs through nodes ahead of |cur_p|*/
  halfword v; /*points to a glue specification or a node ahead of |cur_p|*/
  integer t; /*node count, if |cur_p| is a discretionary node*/
  s=p;
  if (break_type>unhyphenated) {
    if (p!=null) {
      /*@<Compute the discretionary |break_width| values@>;*/
      /* When |p| is a discretionary break, the length of a line
	 ``from |p| to |p|'' has to be defined properly so
	 that the other calculations work out.  Suppose that the
	 pre-break text at |p| has length $l_0$, the post-break
	 text has length $l_1$, and the replacement text has length
	 |l|. Suppose also that |q| is the node following the
	 replacement text. Then length of a line from |p| to |q|
	 will be computed as $\gamma+\beta(q)-\alpha(|p|)$, where
	 $\beta(q)=\beta(|p|)-l_0+l$. The actual length will be
	 the background plus $l_1$, so the length from |p| to
	 |p| should be $\gamma+l_0+l_1-l$.  If the post-break text
	 of the discretionary is empty, a break may also discard~|q|;
	 in that unusual case we subtract the length of~|q| and any
	 other nodes that will be discarded after the discretionary
	 break.
                 
	 The value of $l_0$ need not be computed, since |line_break|
	 will put it into the global variable |disc_width| before
	 calling |try_break|.
      */

      t=replace_count(p); 
      v=p; 
      s=post_break(p);
      while (t>0) {
	decr(t); 
	v=vlink(v);
	/* @<Subtract the width of node |v| from |break_width|@>; */
	/* Replacement texts and discretionary texts are supposed to contain
	   only character nodes, kern nodes, and box or rule nodes.*/
	if (is_char_node(v)) {
	  if (is_rotated(line_break_dir)) {
	    break_width[1] -= (glyph_height(v)+glyph_depth(v));
	  } else {
	    break_width[1] -= glyph_width(v);
	  }
	  if ((pdf_adjust_spacing > 1) && check_expand_pars(font(v))) {
	    prev_char_p = v;
	    sub_char_stretch(break_width[8],v);
	    sub_char_shrink(break_width[9],v);
	  }
	} else {
	  switch (type(v)) {
	  case hlist_node:
	  case vlist_node:
	    if (!(dir_orthogonal(dir_primary[box_dir(v)],
				 dir_primary[line_break_dir])))
	      break_width[1] -= width(v);
	    else
	      break_width[1] -= (depth(v)+height(v));
	    break;
	  case kern_node: 
	    if ((pdf_adjust_spacing > 1) && (subtype(v) == normal)) {
	      sub_kern_stretch(break_width[8],v);
	      sub_kern_shrink(break_width[9],v);
	    }
	    /* fall through */
	  case rule_node:
	    break_width[1] -= width(v);
	    break;
	  default:
	    confusion(maketexstring("disc1"));
	    break;
	  }
	}
      }
      while (s!=null) {
	/* @<Add the width of node |s| to |break_width|@>; */
	if (is_char_node(s)) { 
	  if (is_rotated(line_break_dir)) 
	    break_width[1] += (glyph_height(s)+glyph_depth(s));
	  else
	    break_width[1] += glyph_width(s);
	  if ((pdf_adjust_spacing > 1) && check_expand_pars(font(s))) {
	    prev_char_p = s;
	    add_char_stretch(break_width[8],s);
	    add_char_shrink(break_width[9],s);
	  }
	} else  { 
	  switch (type(s)) {
	  case hlist_node:
	  case vlist_node: 
	    if (!(dir_orthogonal(dir_primary[box_dir(s)],
				 dir_primary[line_break_dir])))
	      break_width[1] += width(s);
	    else
	      break_width[1] += (depth(s)+height(s));
	    break;
	  case kern_node:
	    if ((pdf_adjust_spacing > 1) && (subtype(s) == normal)) {
	      add_kern_stretch(break_width[8],s);
	      add_kern_shrink(break_width[9],s);
	    }
	    /* fall through */
	  case rule_node:
	    break_width[1] += width(s);
	    break;
	  default:
	    confusion(maketexstring("disc2"));
	  }
	}
	s=vlink(s);
      };
      do_one_seven_eight(add_disc_width_to_break_width);
      if (post_break(p)==null) {
	/* nodes may be discardable after the break */
	s=vlink(v);
      }
    }
  }
  while (s!=null) {
    switch (type(s)) {
    case glue_node:
      /*@<Subtract glue from |break_width|@>;*/
      v=glue_ptr(s);
      break_width[1] -= width(v);
      break_width[2+stretch_order(v)] -= stretch(v);
      break_width[7] -= shrink(v);
      break;
    case penalty_node: 
      break;
    case math_node: 
      break_width[1] -= surround(s);
      break;
    case kern_node: 
      if (subtype(s)!=explicit) 
	return;
      else 
	break_width[1] -= width(s);
      break;
    default: 
      return;
    };
    s=vlink(s);
  }
}

#define combine_two_deltas(a) varmem[(prev_r+(a))].cint += varmem[(r+(a))].cint
#define downdate_width(a) cur_active_width[(a)] -= varmem[(prev_r+(a))].cint
#define update_active(a) active_width[(a)]+=varmem[(r+(a))].cint

#define total_font_stretch cur_active_width[8]
#define total_font_shrink cur_active_width[9]

#define max_dimen 07777777777 /* $2^{30}-1$ */

#define left_side 0
#define right_side 1


#define left_pw(a) char_pw((a), left_side)
#define right_pw(a) char_pw((a), right_side)

#define cal_margin_kern_var(a) {                                        \
character(cp) = character((a));						\
font(cp) = font((a));							\
do_subst_font(cp, 1000);						\
if (font(cp) != font((a))) 						\
  margin_kern_stretch += left_pw((a)) - left_pw(cp);			\
font(cp) = font((a));							\
do_subst_font(cp, -1000);						\
if (font(cp) != font((a)))						\
  margin_kern_shrink += left_pw(cp) - left_pw((a));			\
}

void 
ext_try_break(integer pi,
	      quarterword break_type, 
	      int pdf_adjust_spacing,
	      int par_shape_ptr, 
	      int adj_demerits, 
	      int tracing_paragraphs, 
	      int pdf_protrude_chars,
	      int line_penalty,
	      int last_line_fit, 
	      int double_hyphen_demerits, 
	      int final_hyphen_demerits) {
  /* EXIT,CONTINUE,DEACTIVATE,FOUND,NOT_FOUND; */
  pointer r;  /* runs through the active list */
  scaled margin_kern_stretch;
  scaled margin_kern_shrink;
  halfword lp, rp, cp;
  halfword prev_r;  /* stays a step behind |r| */
  halfword old_l; /* maximum line number in current equivalence class of lines */
  boolean no_break_yet; /* have we found a feasible break at |cur_p|? */
  halfword prev_prev_r; /*a step behind |prev_r|, if |type(prev_r)=delta_node|*/
  halfword q; /*points to a new node being created*/
  halfword l; /*line number of current active node*/
  boolean node_r_stays_active; /*should node |r| remain in the active list?*/
  scaled line_width; /*the current line will be justified to this width*/
  fitness_value fit_class; /*possible fitness class of test line*/
  halfword b; /*badness of test line*/
  integer d; /*demerits of test line*/
  boolean artificial_demerits; /*has |d| been forced to zero?*/
  halfword save_link; /*temporarily holds value of |vlink(cur_p)|*/
  scaled shortfall; /*used in badness calculations*/
  scaled g; /*glue stretch or shrink of test line, adjustment for last line*/

  line_width=0;  g=0; prev_prev_r=null;
  /*@<Make sure that |pi| is in the proper range@>;*/
  if (pi>=inf_penalty) {
    goto EXIT; /* this breakpoint is inhibited by infinite penalty */
  } else if (pi<=-inf_penalty) {
    pi=eject_penalty; /*this breakpoint will be forced*/
  }

  no_break_yet=true; 
  prev_r=active; 
  old_l=0;
  do_all_eight(copy_to_cur_active);

 CONTINUE: 
  while (1)  {
    r=vlink(prev_r);
    /* @<If node |r| is of type |delta_node|, update |cur_active_width|,
       set |prev_r| and |prev_prev_r|, then |goto continue|@>; */
    /* The following code uses the fact that |type(last_active)<>delta_node| */
    if (type(r)==delta_node) {
      do_all_eight(update_width);
      prev_prev_r=prev_r; 
      prev_r=r; 
      goto CONTINUE;
    }
    /* @<If a line number class has ended, create new active nodes for
       the best feasible breaks in that class; then |return|
       if |r=last_active|, otherwise compute the new |line_width|@>; */
    /* The first part of the following code is part of \TeX's inner loop, so
       we don't want to waste any time. The current active node, namely node |r|,
       contains the line number that will be considered next. At the end of the
       list we have arranged the data structure so that |r=last_active| and
       |line_number(last_active)>old_l|.
    */
    l=line_number(r);
    if (l>old_l) { /* now we are no longer in the inner loop */
      if ((minimum_demerits<awful_bad)&&((old_l!=easy_line)||(r==last_active))) {
        /*@<Create new active nodes for the best feasible breaks just found@>*/
	/* It is not necessary to create new active nodes having |minimal_demerits|
           greater than
           |minimum_demerits+abs(adj_demerits)|, since such active nodes will never
           be chosen in the final paragraph breaks. This observation allows us to
           omit a substantial number of feasible breakpoints from further consideration.
        */
        if (no_break_yet) {
	  no_break_yet=false; 
	  do_all_eight(set_break_width_to_background);
	  compute_break_width(break_type, pdf_adjust_spacing, cur_p);
        }
        /* @<Insert a delta node to prepare for breaks at |cur_p|@>;*/
	/* We use the fact that |type(active)<>delta_node|.*/
	if (type(prev_r)==delta_node) {/* modify an existing delta node */
	  do_all_eight(convert_to_break_width);			       
	} else if (prev_r==active) { /* no delta node needed at the beginning */
	  do_all_eight(store_break_width);				
	} else {							
	  q=get_node(delta_node_size); 
	  vlink(q)=r; 
	  type(q)=delta_node;	
	  subtype(q)=0; /* the |subtype| is not used */			
	  do_all_eight(new_delta_to_break_width);			
	  vlink(prev_r)=q;  
	  prev_prev_r=prev_r; 
	  prev_r=q;		
	}

        if (abs(adj_demerits)>=awful_bad-minimum_demerits)
          minimum_demerits=awful_bad-1;
        else 
          minimum_demerits += abs(adj_demerits);
        for (fit_class=very_loose_fit;fit_class<=tight_fit;fit_class++) {
          if (minimal_demerits[fit_class]<=minimum_demerits) {
            /* @<Insert a new active node from |best_place[fit_class]|
	       to |cur_p|@>; */
	    /* When we create an active node, we also create the corresponding
	       passive node.
	    */
	    q=get_node(passive_node_size); 
	    type(q)=passive_node;
	    vlink(q)=passive; 
	    passive=q; 
	    cur_break(q)=cur_p;
	    incr(pass_number); 
	    serial(q)=pass_number;
	    prev_break(q)=best_place[fit_class];
	    /*Here we keep track of the subparagraph penalties in the break nodes*/
	    passive_pen_inter(q)=internal_pen_inter;
	    passive_pen_broken(q)=internal_pen_broken;
	    passive_last_left_box(q)=internal_left_box;
	    passive_last_left_box_width(q)=internal_left_box_width;
	    if (prev_break(q)!=null) {
	      passive_left_box(q)=passive_last_left_box(prev_break(q));
	      passive_left_box_width(q)=passive_last_left_box_width(prev_break(q));
	    } else {
	      passive_left_box(q)=init_internal_left_box;
	      passive_left_box_width(q)=init_internal_left_box_width;
	    }
	    passive_right_box(q)=internal_right_box;
	    passive_right_box_width(q)=internal_right_box_width;
	    q=get_node(active_node_size); 
	    break_node(q)=passive;
	    line_number(q)=best_pl_line[fit_class]+1;
	    fitness(q)=fit_class; 
	    type(q)=break_type;
	    total_demerits(q)=minimal_demerits[fit_class];
	    if (do_last_line_fit) {
	      /*@<Store \(a)additional data in the new active node@>*/
	      /* Here we save these data in the active node
		 representing a potential line break.*/
	      active_short(q)=best_pl_short[fit_class];
	      active_glue(q)=best_pl_glue[fit_class];
	    }
	    vlink(q)=r;
	    vlink(prev_r)=q; 
	    prev_r=q;
	    if (tracing_paragraphs>0) {
	      /* @<Print a symbolic description of the new break node@> */
	      print_nl(maketexstring("@@")); 
	      print_int(serial(passive));
	      print(maketexstring(": line ")); 
	      print_int(line_number(q)-1);
	      print_char('.'); 
	      print_int(fit_class);
	      if (break_type==hyphenated) 
		print_char('-');
	      print(maketexstring(" t=")); 
	      print_int(total_demerits(q));
	      if (do_last_line_fit) {
		/*@<Print additional data in the new active node@>; */
		print(maketexstring(" s=")); 
		print_scaled(active_short(q));
		if (cur_p==null) 
		  print(maketexstring(" a="));
		else 
		  print(maketexstring(" g="));
		print_scaled(active_glue(q));
	      }
	      print(maketexstring(" -> @@"));
	      if (prev_break(passive)==null) 
		print_char('0');
	      else 
		print_int(serial(prev_break(passive)));
	    }
          }
          minimal_demerits[fit_class]=awful_bad;
        }
        minimum_demerits=awful_bad;
        /* @<Insert a delta node to prepare for the next active node@>;*/
	/* When the following code is performed, we will have just inserted at
	   least one active node before |r|, so |type(prev_r)<>delta_node|. 
	*/
	if (r!=last_active) {						
	  q=get_node(delta_node_size); 
	  vlink(q)=r; 
	  type(q)=delta_node;	
	  subtype(q)=0; /* the |subtype| is not used */			
	  do_all_eight(new_delta_from_break_width);				
	  vlink(prev_r)=q; 
	  prev_prev_r=prev_r; 
	  prev_r=q;			
	}
      }
      if (r==last_active) 
	goto EXIT;
      /*@<Compute the new line width@>;*/
      /* When we come to the following code, we have just encountered
	 the first active node~|r| whose |line_number| field contains
	 |l|. Thus we want to compute the length of the $l\mskip1mu$th
	 line of the current paragraph. Furthermore, we want to set
	 |old_l| to the last number in the class of line numbers
	 equivalent to~|l|.
      */
      if (l>easy_line) {					
	old_l=max_halfword-1;	
	line_width=second_width; 
      } else  {						
	old_l=l;						
	if (l>last_special_line) {
	  line_width=second_width;
	} else if (par_shape_ptr==null) {
	  line_width=first_width;				
	} else {
	  line_width=varmem[(par_shape_ptr+2*l)].cint;	
	}
      }
    }
    /* /If a line number class has ended, create new active nodes for
       the best feasible breaks in that class; then |return|
       if |r=last_active|, otherwise compute the new |line_width|@>; */

    /* @<Consider the demerits for a line from |r| to |cur_p|;
       deactivate node |r| if it should no longer be active;
       then |goto continue| if a line from |r| to |cur_p| is infeasible,
       otherwise record a new feasible break@>; */
    artificial_demerits=false;
    shortfall = line_width-cur_active_width[1]; /*we're this much too short*/
    if (break_node(r)==null)
      shortfall -= init_internal_left_box_width;
    else 
      shortfall -= passive_last_left_box_width(break_node(r));
    shortfall -= internal_right_box_width;
    if (pdf_protrude_chars > 1) 
      shortfall += total_pw(r, cur_p);
    if ((shortfall != 0) && (pdf_adjust_spacing > 1)) {
      margin_kern_stretch = 0;
      margin_kern_shrink = 0;
      if (pdf_protrude_chars > 1) {
        /* @<Calculate variations of marginal kerns@>;*/
	lp = last_leftmost_char;
	rp = last_rightmost_char;
	cp = new_char_node(0,0);
	if (lp != null) {
	  cal_margin_kern_var(lp);
	}
	if (rp != null) {
	  cal_margin_kern_var(rp);
	}
	ext_free_node(cp,glyph_node_size);
      }
      if ((shortfall > 0) && ((total_font_stretch + margin_kern_stretch) > 0)) {
        if ((total_font_stretch + margin_kern_stretch) > shortfall)
	  shortfall = ((total_font_stretch + margin_kern_stretch) /
		       (max_stretch_ratio / cur_font_step)) / 2;
	else
	  shortfall -= (total_font_stretch + margin_kern_stretch);
      } else if ((shortfall < 0) && ((total_font_shrink + margin_kern_shrink) > 0)) {
        if ((total_font_shrink + margin_kern_shrink) > -shortfall)
	  shortfall = -((total_font_shrink + margin_kern_shrink) /
			(max_shrink_ratio / cur_font_step)) / 2;
        else
  shortfall += (total_font_shrink + margin_kern_shrink);
      }
    }
    if (shortfall>0) {
      /* @<Set the value of |b| to the badness for stretching the line,
	 and compute the corresponding |fit_class|@> */

      /* When a line must stretch, the available stretchability can be
	 found in the subarray |cur_active_width[2..6]|, in units of
	 points, sfi, fil, fill and filll.

	 The present section is part of \TeX's inner loop, and it is
	 most often performed when the badness is infinite; therefore
	 it is worth while to make a quick test for large width excess
	 and small stretchability, before calling the |badness|
	 subroutine.  @^inner loop@> */

      if ((cur_active_width[3]!=0)||(cur_active_width[4]!=0)||
	  (cur_active_width[5]!=0)||(cur_active_width[6]!=0)) {
	if (do_last_line_fit) {
	  if (cur_p==null)  { /* the last line of a paragraph */
	    /* @<Perform computations for last line and |goto found|@>;*/

	    /* Here we compute the adjustment |g| and badness |b| for
	       a line from |r| to the end of the paragraph.  When any
	       of the criteria for adjustment is violated we fall
	       through to the normal algorithm.
	       
	       The last line must be too short, and have infinite
	       stretch entirely due to |par_fill_skip|.*/
	    if ((active_short(r)==0)||(active_glue(r)<=0)) 
	      /* previous line was neither stretched nor shrunk, or
		 was infinitely bad*/
	      goto NOT_FOUND;  
	    if ((cur_active_width[3]!=fill_width[0])||
		(cur_active_width[4]!=fill_width[1])||
		(cur_active_width[5]!=fill_width[2])||
		(cur_active_width[6]!=fill_width[3]))
	      /* infinite stretch of this line not entirely due to
		 |par_fill_skip| */
	      goto NOT_FOUND;
	    if (active_short(r)>0) 
	      g=cur_active_width[2];
	    else 
	      g=cur_active_width[7];
	    if (g<=0) 
	      /*no finite stretch resp.\ no shrink*/
	      goto NOT_FOUND; 
	    arith_error=false; 
	    g=fract(g,active_short(r),active_glue(r),max_dimen);
	    if (last_line_fit<1000)  
	      g=fract(g,last_line_fit,1000,max_dimen);
	    if (arith_error) {
	      if (active_short(r)>0) 
		g=max_dimen; 
	      else 
		g=-max_dimen;
	    }
	    if (g>0) {
	      /*@<Set the value of |b| to the badness of the last line
		for stretching, compute the corresponding |fit_class,
		and |goto found||@>*/
	      /* These badness computations are rather similar to
		 those of the standard algorithm, with the adjustment
		 amount |g| replacing the |shortfall|.*/
	      if (g>shortfall) 
		g=shortfall;
	      if (g>7230584) {
		if (cur_active_width[2]<1663497) {
		  b=inf_bad; 
		  fit_class=very_loose_fit; 
		  goto FOUND;
		}
	      }
	      b=badness(g,cur_active_width[2]);
	      if (b>99)      { fit_class=very_loose_fit; }
	      else if (b>12) { fit_class=loose_fit;  } 
	      else           { fit_class=decent_fit; }
	      goto FOUND;
	    } else if (g<0)  {
	      /*@<Set the value of |b| to the badness of the last line
		for shrinking, compute the corresponding |fit_class,
		and |goto found||@>;*/
	      if (-g>cur_active_width[7]) 
		g=-cur_active_width[7];
	      b=badness(-g,cur_active_width[7]);
	      if (b>12) fit_class=tight_fit; 
	      else      fit_class=decent_fit;
	      goto FOUND;
	    }
	  }
	NOT_FOUND:
	  shortfall=0;
	}
	b=0; 
	fit_class=decent_fit; /* infinite stretch */
      } else {
	if (shortfall>7230584 && cur_active_width[2]<1663497) {
	  b=inf_bad; 
	  fit_class=very_loose_fit;
	} else {
	  b=badness(shortfall,cur_active_width[2]);
	  if (b>99)      { fit_class=very_loose_fit; }
	  else if (b>12) { fit_class=loose_fit;  } 
	  else           { fit_class=decent_fit; }
	}
      }
    } else { 
      /* Set the value of |b| to the badness for shrinking the line,
	 and compute the corresponding |fit_class|@>; */
      /* Shrinkability is never infinite in a paragraph; we can shrink
	 the line from |r| to |cur_p| by at most
	 |cur_active_width[7]|.*/
      if (-shortfall>cur_active_width[7]) 
	b=inf_bad+1;
      else 
	b=badness(-shortfall,cur_active_width[7]);
      if (b>12) 
	fit_class=tight_fit; 
      else 
	fit_class=decent_fit;
    }
    if (do_last_line_fit) {
      /* @<Adjust \(t)the additional data for last line@>; */
      if (cur_p==null) shortfall=0;
      if (shortfall>0)      { g=cur_active_width[2]; }
      else if (shortfall<0) { g=cur_active_width[7]; }
      else                  { g=0;}
    }
  FOUND:
    if ((b>inf_bad)||(pi==eject_penalty)) {
      /* @<Prepare to deactivate node~|r|, and |goto deactivate| unless
	 there is a reason to consider lines of text from |r| to |cur_p|@> */
      /* During the final pass, we dare not lose all active nodes, lest we lose
	 touch with the line breaks already found. The code shown here makes sure
	 that such a catastrophe does not happen, by permitting overfull boxes as
	 a last resort. This particular part of \TeX\ was a source of several subtle
	 bugs before the correct program logic was finally discovered; readers
	 who seek to ``improve'' \TeX\ should therefore think thrice before daring
	 to make any changes here.
	 @^overfull boxes@>
      */
      if (final_pass && (minimum_demerits==awful_bad) && 
	  (vlink(r)==last_active) && (prev_r==active)) {
	artificial_demerits=true; /* set demerits zero, this break is forced */
      } else if (b>threshold) { 
	goto DEACTIVATE;
      }
      node_r_stays_active=false;
    } else { 
      prev_r=r;
      if (b>threshold) 
	goto CONTINUE;
      node_r_stays_active=true;
    }
    /* @<Record a new feasible break@>;*/
    /* When we get to this part of the code, the line from |r| to |cur_p| is
       feasible, its badness is~|b|, and its fitness classification is |fit_class|.
       We don't want to make an active node for this break yet, but we will
       compute the total demerits and record them in the |minimal_demerits| array,
       if such a break is the current champion among all ways to get to |cur_p|
       in a given line-number class and fitness class.
    */
    if (artificial_demerits) { 
      d=0;
    } else { 
      /* @<Compute the demerits, |d|, from |r| to |cur_p|@>;*/
      d=line_penalty+b;
      if (abs(d)>=10000) d=100000000; else d=d*d;
      if (pi!=0) {
	if (pi>0) {
	  d += (pi*pi);
	} else if (pi>eject_penalty) {
	  d -= (pi*pi);
	}
      }
      if ((break_type==hyphenated)&&(type(r)==hyphenated)) {
	if (cur_p!=null) 
	  d += double_hyphen_demerits;
	else 
	  d += final_hyphen_demerits;
      }
      if (abs(fit_class-fitness(r))>1) d=d+adj_demerits;
    }
    if (tracing_paragraphs>0) {
      /* @<Print a symbolic description of this feasible break@>;*/
      if (printed_node!=cur_p) {
	/* @<Print the list between |printed_node| and |cur_p|, then
	   set |printed_node:=cur_p|@>;*/
	print_nl(maketexstring(""));
	if (cur_p==null) {
	  short_display(vlink(printed_node));
	} else {
	  save_link=vlink(cur_p);
	  vlink(cur_p)=null; 
	  print_nl(maketexstring("")); 
	  short_display(vlink(printed_node));
	  vlink(cur_p)=save_link;
	}
	printed_node=cur_p;
      }
      print_nl(maketexstring("@"));
      if (cur_p==null) { 
	print_esc(maketexstring("par")); 
      } else if (type(cur_p)!=glue_node) {
	if      (type(cur_p)==penalty_node)  print_esc(maketexstring("penalty"));
	else if (type(cur_p)==disc_node)     print_esc(maketexstring("discretionary"));
	else if (type(cur_p)==kern_node)     print_esc(maketexstring("kern"));
	else                                 print_esc(maketexstring("math"));
      }
      print(maketexstring(" via @@"));
      if (break_node(r)==null) 
	print_char('0');
      else 
	print_int(serial(break_node(r)));
      print(maketexstring(" b="));
      if (b>inf_bad)  
	print_char('*');
      else 
	print_int(b);
      print(maketexstring(" p=")); 
      print_int(pi); 
      print(maketexstring(" d="));
      if (artificial_demerits) 
	print_char('*');
      else 
	print_int(d);
    }
    d += total_demerits(r); /*this is the minimum total demerits
			      from the beginning to |cur_p| via |r| */
    if (d<=minimal_demerits[fit_class]) {
      minimal_demerits[fit_class]=d;
      best_place[fit_class]=break_node(r); 
      best_pl_line[fit_class]=l;
      if (do_last_line_fit) {
	/* Store \(a)additional data for this feasible break@>; */
	/* For each feasible break we record the shortfall and glue stretch or
	   shrink (or adjustment).*/
	best_pl_short[fit_class]=shortfall; 
	best_pl_glue[fit_class]=g;
      }
      if (d<minimum_demerits) 
	minimum_demerits=d;
    }
    /* /Record a new feasible break */
    if (node_r_stays_active) 
      goto CONTINUE; /*|prev_r| has been set to |r| */
  DEACTIVATE: 
    /* @<Deactivate node |r|@>; */
    /* When an active node disappears, we must delete an adjacent delta node if the
       active node was at the beginning or the end of the active list, or if it
       was surrounded by delta nodes. We also must preserve the property that
       |cur_active_width| represents the length of material from |vlink(prev_r)|
       to~|cur_p|.*/
    
    vlink(prev_r)=vlink(r); 
    free_node(r,active_node_size);
    if (prev_r==active) {
      /*@<Update the active widths, since the first active node has been deleted@>*/
      /* The following code uses the fact that |type(last_active)<>delta_node|. If the
	 active list has just become empty, we do not need to update the
	 |active_width| array, since it will be initialized when an active
	 node is next inserted.
      */
      r=vlink(active);
      if (type(r)==delta_node) {
	do_all_eight(update_active);
	do_all_eight(copy_to_cur_active);
	vlink(active)=vlink(r); 
	free_node(r,delta_node_size);
      };
    } else if (type(prev_r)==delta_node){
      r=vlink(prev_r);
      if (r==last_active) {
	do_all_eight(downdate_width);
	vlink(prev_prev_r)=last_active;
	free_node(prev_r,delta_node_size); 
	prev_r=prev_prev_r;
      } else if (type(r)==delta_node) {
	do_all_eight(update_width);
	do_all_eight(combine_two_deltas);
	vlink(prev_r)=vlink(r); 
	free_node(r,delta_node_size);
      }
    }   
  }
 EXIT: 
  /* @<Update the value of |printed_node| for symbolic displays@>;*/
  /* When the data for a discretionary break is being displayed, we will have
     printed the |pre_break| and |post_break| lists; we want to skip over the
     third list, so that the discretionary data will not appear twice.  The
     following code is performed at the very end of |try_break|.*/
  if (cur_p==printed_node &&  cur_p!=null && type(cur_p)==disc_node) {
    int t=replace_count(cur_p);
    while (t-->0) {
      printed_node=vlink(printed_node);
    }
  }
};


void
ext_do_line_break (int d, int pretolerance, int tracing_paragraphs, 
		   int tolerance, scaled emergency_stretch,
		   int prev_graf, 
		   int looseness, 
		   int hyphen_penalty, 
		   int ex_hyphen_penalty,
		   int pdf_adjust_spacing, int par_shape_ptr, int adj_demerits,
		   int pdf_protrude_chars,
		   int line_penalty, 
		   int last_line_fit, 
		   int double_hyphen_demerits,  
		   int final_hyphen_demerits,
		   int hang_indent, int hsize, int hang_after,
		   halfword left_skip, halfword right_skip) {
  /* DONE,DONE1,DONE2,DONE3,DONE4,DONE5,CONTINUE;*/
  halfword q,r,s; /* miscellaneous nodes of temporary interest */
  internal_font_number f;  /* used when calculating character widths */


  /* Get ready to start ... */
  minimum_demerits=awful_bad;
  minimal_demerits[tight_fit]=awful_bad;
  minimal_demerits[decent_fit]=awful_bad;
  minimal_demerits[loose_fit]=awful_bad;
  minimal_demerits[very_loose_fit]=awful_bad;

  /* We compute the values of |easy_line| and the other local variables relating
     to line length when the |line_break| procedure is initializing itself. */
  if (par_shape_ptr==null) {
    if (hang_indent==0) {
      last_special_line=0; 
      second_width=hsize;
      second_indent=0;
    } else { 
      /*  @<Set line length parameters in preparation for hanging indentation@>*/
      /* We compute the values of |easy_line| and the other local variables relating
	 to line length when the |line_break| procedure is initializing itself. */
      last_special_line=abs(hang_after);
      if (hang_after<0) {
	first_width=hsize-abs(hang_indent);
	if (hang_indent>=0) 
	  first_indent=hang_indent;
	else 
	  first_indent=0;
	second_width=hsize; 
	second_indent=0;
      } else  {
	first_width=hsize; 
	first_indent=0;
	second_width=hsize-abs(hang_indent);
	if (hang_indent>=0) 
	  second_indent=hang_indent;
	else 
	  second_indent=0;
      }
    }
  } else {
    last_special_line=vinfo(par_shape_ptr)-1;
    second_width=varmem[(par_shape_ptr+2*(last_special_line+1))].cint;
    second_indent=varmem[(par_shape_ptr+2*last_special_line+1)].cint;
  }
  if (looseness==0) 
    easy_line=last_special_line;
  else 
    easy_line=max_halfword; 

  no_shrink_error_yet=true;
  check_shrinkage(left_skip); 
  check_shrinkage(right_skip);
  q=left_skip; 
  r=right_skip; 
  background[1]=width(q)+width(r);
  background[2]=0; 
  background[3]=0; 
  background[4]=0; 
  background[5]=0;
  background[6]=0;
  background[2+stretch_order(q)] = stretch(q);
  background[2+stretch_order(r)] += stretch(r);
  background[7]=shrink(q)+shrink(r);
  if (pdf_adjust_spacing > 1) {
    background[8] = 0;
    background[9] = 0;
    max_stretch_ratio = -1;
    max_shrink_ratio = -1;
    cur_font_step = -1;
    prev_char_p = null;
  }
  /* @<Check for special treatment of last line of paragraph@>;*/
  /* The new algorithm for the last line requires that the stretchability
     |par_fill_skip| is infinite and the stretchability of |left_skip| plus
     |right_skip| is finite.
  */
  do_last_line_fit=false; 
  active_node_size=active_node_size_normal; /*just in case*/
  if (last_line_fit>0) {
    q=glue_ptr(last_line_fill);
    if ((stretch(q)>0)&&(stretch_order(q)>normal)) {
      if ((background[3]==0)&&(background[4]==0)&&
	  (background[5]==0)&&(background[6]==0)) {
	do_last_line_fit=true;
	active_node_size=active_node_size_extended;
	fill_width[0]=0; 
	fill_width[1]=0; 
	fill_width[2]=0; 
	fill_width[3]=0;
	fill_width[stretch_order(q)-1]=stretch(q);
      }
    }
  }

  /* @<Find optimal breakpoints@>;*/
  threshold=pretolerance;
  if (threshold>=0) {
    if (tracing_paragraphs>0) {
      begin_diagnostic(); 
      print_nl(maketexstring("@firstpass"));
    }
    second_pass=false; 
    final_pass=false;
  } else  { 
    threshold=tolerance; 
    second_pass=true;
    final_pass=(emergency_stretch<=0);
    if (tracing_paragraphs>0) 
      begin_diagnostic();
  }
    
  while (1)  { 
    if (threshold>inf_bad)
      threshold=inf_bad;
    /* Create an active breakpoint representing the beginning of the paragraph */
    q=get_node(active_node_size);
    type(q)=unhyphenated; 
    fitness(q)=decent_fit;
    vlink(q)=last_active; 
    break_node(q)=null;
    line_number(q)=prev_graf+1; 
    total_demerits(q)=0; 
    vlink(active)=q;
    if (do_last_line_fit) {
      /* Initialize additional fields of the first active node */;
      /* Here we initialize the additional fields of the first active node
	 representing the beginning of the paragraph.*/
      active_short(q)=0; 
      active_glue(q)=0;
    }
    do_all_eight(store_background);
    passive=null; 
    printed_node=temp_head; 
    pass_number=0;
    font_in_short_display=null_font;
    /* /Create an active breakpoint representing the beginning of the paragraph */
    cur_p=vlink(temp_head); 
    auto_breaking=true;
    prev_p=cur_p; /* glue at beginning is not a legal breakpoint */
    /* LOCAL: Initialize with first |local_paragraph| node */
    if ((type(cur_p)==whatsit_node)&&(subtype(cur_p)==local_par_node)) {
      internal_pen_inter=local_pen_inter(cur_p);
      internal_pen_broken=local_pen_broken(cur_p);
      init_internal_left_box=local_box_left(cur_p);
      init_internal_left_box_width=local_box_left_width(cur_p);
      internal_left_box=init_internal_left_box;
      internal_left_box_width=init_internal_left_box_width;
      internal_right_box=local_box_right(cur_p);
      internal_right_box_width=local_box_right_width(cur_p);
    } else {
      internal_pen_inter=0; 
      internal_pen_broken=0;
      init_internal_left_box=null; 
      init_internal_left_box_width=0;
      internal_left_box=init_internal_left_box;
      internal_left_box_width=init_internal_left_box_width;
      internal_right_box=null; 
      internal_right_box_width=0;
    }
    /* /LOCAL: Initialize with first |local_paragraph| node */
    prev_char_p = null;
    prev_legal = null;
    rejected_cur_p = null;
    try_prev_break = false;
    before_rejected_cur_p = false;
    first_p = cur_p; 
    /* to access the first node of paragraph as the first active node
			has |break_node=null| */
    while ((cur_p!=null)&&(vlink(active)!=last_active)) {
      /* Call |try_break| if |cur_p| is a legal breakpoint; on the
	 second pass, also try to hyphenate the next word, if |cur_p|
	 is a glue node; then advance |cur_p| to the next node of the
	 paragraph that could possibly be a legal breakpoint */

      /* Here is the main switch in the |line_break| routine, where
	 legal breaks are determined. As we move through the hlist, we
	 need to keep the |active_width| array up to date, so that the
	 badness of individual lines is readily calculated by
	 |try_break|. It is convenient to use the short name
	 |act_width| for the component of active width that represents
	 real width as opposed to glue. */

      if (is_char_node(cur_p)) {
	/* Advance \(c)|cur_p| to the node following the present
	   string of characters ;*/
	/* The code that passes over the characters of words in a
	   paragraph is part of \TeX's inner loop, so it has been
	   streamlined for speed. We use the fact that
	   `\.{\\parfillskip}' glue appears at the end of each
	   paragraph; it is therefore unnecessary to check if
	   |vlink(cur_p)=null| when |cur_p| is a character node.
	*/
	prev_p=cur_p;
	do {
	  f=font(cur_p);
	  if (is_rotated(line_break_dir))
	    act_width += (glyph_height(cur_p)+glyph_depth(cur_p));
	  else
	    act_width += glyph_width(cur_p);
	  if ((pdf_adjust_spacing > 1) && check_expand_pars(f)) {
	    prev_char_p = cur_p;
	    add_char_stretch(active_width[8],cur_p);
	    add_char_shrink(active_width[9],cur_p);
	  }
	  cur_p=vlink(cur_p);
	} while (is_char_node(cur_p));
      }

      switch(type(cur_p)){
      case hlist_node:
      case vlist_node:
	if (!(dir_orthogonal(dir_primary[box_dir(cur_p)],
			     dir_primary[line_break_dir])))
	  act_width += width(cur_p);
	else 
	  act_width += (depth(cur_p)+height(cur_p));
	break;
      case rule_node: 
	act_width += width(cur_p);
	break;
      case whatsit_node: 
	/* @<Advance \(p)past a whatsit node in the \(l)|line_break| loop@>;*/
	if (subtype(cur_p)==local_par_node) {
	  /* @<LOCAL: Advance past a |local_paragraph| node@>;*/
	  internal_pen_inter=local_pen_inter(cur_p);
	  internal_pen_broken=local_pen_broken(cur_p);
	  internal_left_box=local_box_left(cur_p);
	  internal_left_box_width=local_box_left_width(cur_p);
	  internal_right_box=local_box_right(cur_p);
	  internal_right_box_width=local_box_right_width(cur_p);
	} else if (subtype(cur_p)==dir_node) {
	  /* @<DIR: Adjust the dir stack for the |line_break| routine@>;*/
	  if (dir_dir(cur_p)>=0) {
	    line_break_dir=dir_dir(cur_p);
	    push_dir_node(cur_p);
	  } else {
	    pop_dir_node();
	    if (dir_ptr!=null)
	      line_break_dir=dir_dir(dir_ptr);
	  }
	} else if ((subtype(cur_p) == pdf_refxform_node) || 
		   (subtype(cur_p) == pdf_refximage_node)) {
	  act_width += pdf_width(cur_p);
	}
	/* / Advance \(p)past a whatsit node in the \(l)|line_break| loop/; */
	break;
      case glue_node: 
	/* @<If node |cur_p| is a legal breakpoint, call |try_break|;
	   then update the active widths by including the glue in
	   |glue_ptr(cur_p)|@>; */
	/* When node |cur_p| is a glue node, we look at |prev_p| to
	   see whether or not a breakpoint is legal at |cur_p|, as
	   explained above. */
	if (auto_breaking) {
	  if (is_char_node(prev_p) ||
	      precedes_break(prev_p)|| 
	      ((type(prev_p)==kern_node)&&(subtype(prev_p)!=explicit))) {
	    ext_try_break(0,unhyphenated, pdf_adjust_spacing, par_shape_ptr, 
			  adj_demerits, tracing_paragraphs, pdf_protrude_chars,
			  line_penalty, last_line_fit,  double_hyphen_demerits,  
			  final_hyphen_demerits);
	  }
	}
	check_shrinkage(glue_ptr(cur_p)); 
	q=glue_ptr(cur_p);
	act_width += width(q);
	active_width[2+stretch_order(q)] += stretch(q);
	active_width[7] += shrink(q);
	break;
      case kern_node: 
	if (subtype(cur_p)==explicit) {
	  kern_break();
	} else {
	  act_width += width(cur_p);
	  if ((pdf_adjust_spacing > 1) && (subtype(cur_p) == normal)) {
	    add_kern_stretch(active_width[8],cur_p);
	    add_kern_shrink(active_width[9],cur_p);
	  }
	}
	break;
      case disc_node: 
	/* @<Try to break after a discretionary fragment, then |goto done5|@>;*/
	/* The following code knows that discretionary texts contain
	   only character nodes, kern nodes, box nodes, and rule
	   nodes. */
	if (second_pass) {
	  s=pre_break(cur_p);
	  do_one_seven_eight(reset_disc_width);
	  if (s==null) {
	    /* TH this is supposed to indicate an explicit hyphen,
	       like those found in compound words. Tex82 auto-inserted
	       empty discretionaries after seeing the current font's 
	       \.{\\hyphenchar} in the input stream.
	    */
	    ext_try_break(ex_hyphen_penalty,hyphenated, pdf_adjust_spacing, 
			  par_shape_ptr, adj_demerits, tracing_paragraphs, 
			  pdf_protrude_chars, line_penalty, last_line_fit,  
			  double_hyphen_demerits,  final_hyphen_demerits);
	  } else {
	    do { 
	      /* @<Add the width of node |s| to |disc_width|@>;*/
	      if (is_char_node(s)) { 
		if (is_rotated(line_break_dir)) {
		  disc_width[1] += (glyph_height(s)+glyph_depth(s));
		}  else {
		  disc_width[1] += glyph_width(s);
		}
		if ((pdf_adjust_spacing > 1) && check_expand_pars(font(s))) {
		  prev_char_p = s;
		  add_char_stretch(disc_width[8],s);
		  add_char_shrink(disc_width[9],s);
		}
	      } else { 
		switch (type(s)) {
		case hlist_node: 
		case vlist_node: 
		  if (!(dir_orthogonal(dir_primary[box_dir(s)],
				       dir_primary[line_break_dir]))) { 
		    disc_width[1] += width(s); 
		  } else { 
		    disc_width[1] += (depth(s)+height(s));
		  }
		  break;
		case kern_node: 
		  if ((pdf_adjust_spacing > 1) && (subtype(s) == normal)) {
		    add_kern_stretch(disc_width[8],s);
		    add_kern_shrink(disc_width[9],s);
		  }
		  /* fall through */
		case rule_node:
		  disc_width[1] += width(s);
		  break;
		default:
		  confusion(maketexstring("disc3"));
		} 
	      }
	      /* /Add the width of node |s| to |disc_width| */
	      s=vlink(s);
	    } while (s!=null);
	    do_one_seven_eight(add_disc_width_to_active_width);
	    ext_try_break(hyphen_penalty,hyphenated, pdf_adjust_spacing, 
			  par_shape_ptr, adj_demerits, tracing_paragraphs, 
			  pdf_protrude_chars, line_penalty, last_line_fit,  
			  double_hyphen_demerits,  final_hyphen_demerits);
	    do_one_seven_eight(sub_disc_width_from_active_width);
	  }
	}

	r=replace_count(cur_p); 
	s=vlink(cur_p);
	while (r>0) { 
	  /* @<Add the width of node |s| to |act_width|@>;*/
	  if (is_char_node(s)) { 
	    if (is_rotated(line_break_dir)) {
	      act_width += glyph_height(s)+glyph_depth(s);
	    } else {
	      act_width += glyph_width(s);
	    }
	    if ((pdf_adjust_spacing > 1) && check_expand_pars(font(s))) {
	      prev_char_p = s;
	      add_char_stretch(active_width[8],s);
	      add_char_shrink(active_width[9],s);
	    };
	  } else  {
	    switch (type(s)) {
	    case hlist_node:
	    case vlist_node:
	      if (!(dir_orthogonal(dir_primary[box_dir(s)],
				   dir_primary[line_break_dir])))
		act_width += width(s);
	      else 
		act_width += (depth(s)+height(s));
	      break;
	    case kern_node: 
	      if ((pdf_adjust_spacing > 1) && (subtype(s) == normal)) {
		add_kern_stretch(active_width[8],s);
		add_kern_shrink(active_width[9],s);
	      }
	      /* fall through */
	    case rule_node:
	      act_width += width(s);
	      break;
	    default: 
	      confusion(maketexstring("disc4"));
	    }
	  }
	  /* /Add the width of node |s| to |act_width|;*/
	  decr(r); 
	  s=vlink(s);
	}
	prev_p=cur_p; 
	cur_p=s; 
	continue;
	break;
      case math_node: 
	auto_breaking=(subtype(cur_p)==after); 
	kern_break();
	break;
      case penalty_node: 
	ext_try_break(penalty(cur_p),unhyphenated, pdf_adjust_spacing, 
		      par_shape_ptr, adj_demerits, tracing_paragraphs, 
		      pdf_protrude_chars, line_penalty, last_line_fit,
		      double_hyphen_demerits,  final_hyphen_demerits);
	break;
      case mark_node:
      case ins_node:
      case adjust_node: 
	break;
      case glue_spec_node: 
	fprintf(stdout, "\nfound a glue_spec in a paragraph!"); 
	break;
      default: 
	fprintf(stdout, "\ntype=%d",type(cur_p));
	confusion(maketexstring("paragraph")) ;
      }
      prev_p=cur_p; 
      cur_p=vlink(cur_p); 
    }
    if (cur_p==null) {
      /* Try the final line break at the end of the paragraph,
	 and |goto done| if the desired breakpoints have been found */

      /* The forced line break at the paragraph's end will reduce the list of
	 breakpoints so that all active nodes represent breaks at |cur_p=null|.
	 On the first pass, we insist on finding an active node that has the
	 correct ``looseness.'' On the final pass, there will be at least one active
	 node, and we will match the desired looseness as well as we can.
	 
	 The global variable |best_bet| will be set to the active node for the best
	 way to break the paragraph, and a few other variables are used to
	 help determine what is best.
      */
	 
      ext_try_break(eject_penalty,hyphenated, pdf_adjust_spacing, 
		    par_shape_ptr, adj_demerits, tracing_paragraphs, 
		    pdf_protrude_chars,line_penalty, last_line_fit,
		    double_hyphen_demerits,  final_hyphen_demerits);
      if (vlink(active)!=last_active) {
	/* @<Find an active node with fewest demerits@>;*/
	r=vlink(active); 
	fewest_demerits=awful_bad;
	do {
	  if (type(r)!=delta_node) {
	    if (total_demerits(r)<fewest_demerits) {
	      fewest_demerits=total_demerits(r); 
	      best_bet=r;
	    }
	  }
	  r=vlink(r);
	} while (r!=last_active);
	best_line=line_number(best_bet);

	/* /Find an active node with fewest demerits;*/
	if (looseness==0) 
	  goto DONE;
	/*@<Find the best active node for the desired looseness@>;*/

	/* The adjustment for a desired looseness is a slightly more complicated
	   version of the loop just considered. Note that if a paragraph is broken
	   into segments by displayed equations, each segment will be subject to the
	   looseness calculation, independently of the other segments.
	*/
	r=vlink(active); 
	actual_looseness=0;
	do {
	  if (type(r)!=delta_node) {
	    line_diff=line_number(r)-best_line;
	    if (((line_diff<actual_looseness)&&(looseness<=line_diff))||
		((line_diff>actual_looseness)&&(looseness>=line_diff))) {
	      best_bet=r; 
	      actual_looseness=line_diff;
	      fewest_demerits=total_demerits(r);
	    } else if ((line_diff==actual_looseness)&&
		       (total_demerits(r)<fewest_demerits)) {
	      best_bet=r; 
	      fewest_demerits=total_demerits(r);
	    }
	  }
	  r=vlink(r);
	} while (r!=last_active);
	best_line=line_number(best_bet);

	/* /Find the best active node for the desired looseness;*/
	if ((actual_looseness==looseness)|| final_pass) 
	  goto DONE;
      }
      /* /Try the final line break at the end of the paragraph,
	 and |goto done| if the desired breakpoints have been found */
    }
    
    /* Clean up the memory by removing the break nodes; */
    clean_up_the_memory();
    /* /Clean up the memory by removing the break nodes; */

    if (!second_pass)  { 
      if (tracing_paragraphs>0) 
	print_nl(maketexstring("@secondpass"));
      threshold=tolerance; 
      second_pass=true; 
      final_pass=(emergency_stretch<=0);
    }  else {  /* if at first you do not succeed, \dots */
      if (tracing_paragraphs>0)
	print_nl(maketexstring("@emergencypass"));
      background[2] += emergency_stretch; 
      final_pass=true;
    }
  }
 DONE:  
  if (tracing_paragraphs>0) { 
    end_diagnostic(true); 
    normalize_selector(); 
  }
  if (do_last_line_fit) {
    /* Adjust \(t)the final line of the paragraph;*/
    /* Here we either reset |do_last_line_fit| or adjust the |par_fill_skip| glue.
     */
    if (active_short(best_bet)==0) {
      do_last_line_fit=false;
    } else { 
      q=new_spec(glue_ptr(last_line_fill));
      delete_glue_ref(glue_ptr(last_line_fill));
      width(q) += (active_short(best_bet)-active_glue(best_bet));
      stretch(q)=0; 
      glue_ptr(last_line_fill)=q;
    }
    /* /Adjust \(t)the final line of the paragraph;*/
  }

  /* @<Break the paragraph at the chosen...@>; */
  /* Once the best sequence of breakpoints has been found (hurray), we call on the
     procedure |post_line_break| to finish the remainder of the work.
     (By introducing this subprocedure, we are able to keep |line_break|
     from getting extremely long.)
  */
  post_line_break(d);
  /* /Break the paragraph at the chosen... */
  /* Clean up the memory by removing the break nodes; */
  clean_up_the_memory();
  /* /Clean up the memory by removing the break nodes; */
}


