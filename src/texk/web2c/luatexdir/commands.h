/* commands.h
   
   Copyright 2008 Taco Hoekwater <taco@luatex.org>

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


typedef enum {
  relax_cmd=0, /* do nothing ( \.{\\relax} ) */
#define escape_cmd          0 /* escape delimiter (called \.\\ in {\sl The \TeX book\/}) */
  left_brace_cmd=1, /* beginning of a group ( \.\{ ) */
  right_brace_cmd=2, /* ending of a group ( \.\} ) */
  math_shift_cmd=3, /* mathematics shift character ( \.\$ ) */
  tab_mark_cmd=4, /* alignment delimiter ( \.\&, \.{\\span} ) */
  car_ret_cmd=5, /* end of line ( |carriage_return|, \.{\\cr}, \.{\\crcr} ) */
#define out_param_cmd       5 /* output a macro parameter */
  mac_param_cmd=6, /* macro parameter symbol ( \.\# ) */
  sup_mark_cmd=7, /* superscript ( \.{\char'136} ) */
  sub_mark_cmd=8, /* subscript ( \.{\char'137} ) */
  ignore_cmd=9, /* characters to ignore ( \.{\^\^@@} ) */
  endv_cmd=9, /* end of \<v_j> list in alignment template */
#define ignore_cmd          9 /* characters to ignore ( \.{\^\^@@} )*/
  spacer_cmd=10, /* characters equivalent to blank space ( \.{\ } ) */
  letter_cmd=11, /* characters regarded as letters ( \.{A..Z}, \.{a..z} ) */
  other_char_cmd=12, /* none of the special character types */
  par_end_cmd=13, /* end of paragraph ( \.{\\par} ) */
#define active_char_cmd    13 /* characters that invoke macros ( \.{\char`\~} )*/
#define match_cmd          13 /* match a macro parameter*/
  stop_cmd=14, /* end of job ( \.{\\end}, \.{\\dump} ) */
#define comment_cmd        14 /* characters that introduce comments ( \.\% )*/
#define end_match_cmd      14 /* end of parameters to macro*/
  delim_num_cmd=15, /* specify delimiter numerically ( \.{\\delimiter} ) */
#define invalid_char_cmd   15 /* characters that shouldn't appear ( \.{\^\^?} )*/
#define max_char_code_cmd  15 /* largest catcode for individual characters */
  char_num_cmd=16, /* character specified numerically ( \.{\\char} ) */
  math_char_num_cmd=17, /* explicit math code ( \.{\\mathchar} ) */
  mark_cmd=18, /* mark definition ( \.{\\mark} ) */
  xray_cmd=19, /* peek inside of \TeX\ ( \.{\\show}, \.{\\showbox}, etc.~) */
  make_box_cmd=20, /* make a box ( \.{\\box}, \.{\\copy}, \.{\\hbox}, etc.~) */
  hmove_cmd=21, /* horizontal motion ( \.{\\moveleft}, \.{\\moveright} ) */
  vmove_cmd=22, /* vertical motion ( \.{\\raise}, \.{\\lower} ) */
  un_hbox_cmd=23, /* unglue a box ( \.{\\unhbox}, \.{\\unhcopy} ) */
  un_vbox_cmd=24, /* unglue a box ( \.{\\unvbox}, \.{\\unvcopy} or \.{\\pagediscards}, \.{\\splitdiscards} ) */
  remove_item_cmd=25, /* nullify last item ( \.{\\unpenalty}, \.{\\unkern}, \.{\\unskip} ) */
  hskip_cmd=26, /* horizontal glue ( \.{\\hskip}, \.{\\hfil}, etc.~) */
  vskip_cmd=27, /* vertical glue ( \.{\\vskip}, \.{\\vfil}, etc.~) */
  mskip_cmd=28, /* math glue ( \.{\\mskip} ) */
  kern_cmd=29, /* fixed space ( \.{\\kern}) */
  mkern_cmd=30, /* math kern ( \.{\\mkern} ) */
  leader_ship_cmd=31, /* use a box ( \.{\\shipout}, \.{\\leaders}, etc.~) */
  halign_cmd=32, /* horizontal table alignment ( \.{\\halign} ) */
  valign_cmd=33, /* vertical table alignment ( \.{\\valign} ) */
  no_align_cmd=34, /* temporary escape from alignment ( \.{\\noalign} ) */
  vrule_cmd=35, /* vertical rule ( \.{\\vrule} ) */
  hrule_cmd=36, /* horizontal rule ( \.{\\hrule} ) */
  insert_cmd=37, /* vlist inserted in box ( \.{\\insert} ) */
  vadjust_cmd=38, /* vlist inserted in enclosing paragraph ( \.{\\vadjust} ) */
  ignore_spaces_cmd=39, /* gobble |spacer| tokens ( \.{\\ignorespaces} ) */
  after_assignment_cmd=40, /* save till assignment is done ( \.{\\afterassignment} ) */
  after_group_cmd=41, /* save till group is done ( \.{\\aftergroup} ) */
  break_penalty_cmd=42, /* additional badness ( \.{\\penalty} ) */
  start_par_cmd=43, /* begin paragraph ( \.{\\indent}, \.{\\noindent} ) */
  ital_corr_cmd=44, /* italic correction ( \.{\\/} ) */
  accent_cmd=45, /* attach accent in text ( \.{\\accent} ) */
  math_accent_cmd=46, /* attach accent in math ( \.{\\mathaccent} ) */
  discretionary_cmd=47, /* discretionary texts ( \.{\\-}, \.{\\discretionary} ) */
  eq_no_cmd=48, /* equation number ( \.{\\eqno}, \.{\\leqno} ) */
  left_right_cmd=49, /* variable delimiter ( \.{\\left}, \.{\\right} or \.{\\middle} ) */
  math_comp_cmd=50, /* component of formula ( \.{\\mathbin}, etc.~) */
  limit_switch_cmd=51, /* diddle limit conventions ( \.{\\displaylimits}, etc.~) */
  above_cmd=52, /* generalized fraction ( \.{\\above}, \.{\\atop}, etc.~) */
  math_style_cmd=53, /* style specification ( \.{\\displaystyle}, etc.~) */
  math_choice_cmd=54, /* choice specification ( \.{\\mathchoice} ) */
  non_script_cmd=55, /* conditional math glue ( \.{\\nonscript} ) */
  vcenter_cmd=56, /* vertically center a vbox ( \.{\\vcenter} ) */
  case_shift_cmd=57, /* force specific case ( \.{\\lowercase}, \.{\\uppercase}~) */
  message_cmd=58, /* send to user ( \.{\\message}, \.{\\errmessage} ) */
  extension_cmd=59, /* extensions to \TeX\ ( \.{\\write}, \.{\\special}, etc.~) */
  in_stream_cmd=60, /* files for reading ( \.{\\openin}, \.{\\closein} ) */
  begin_group_cmd=61, /* begin local grouping ( \.{\\begingroup} ) */
  end_group_cmd=62, /* end local grouping ( \.{\\endgroup} ) */
  omit_cmd=63, /* omit alignment template ( \.{\\omit} ) */
  ex_space_cmd=64, /* explicit space ( \.{\\\ } ) */
  no_boundary_cmd=65, /* suppress boundary ligatures ( \.{\\noboundary} ) */
  radical_cmd=66, /* square root and similar signs ( \.{\\radical} ) */
  end_cs_name_cmd=67, /* end control sequence ( \.{\\endcsname} ) */
  char_ghost_cmd=68, /* \.{\\ghostleft}, \.{\\ghostright} character for kerning */
  assign_local_box_cmd=69, /* box for guillemets \.{\\localleftbox} or \.{\\localrightbox} */
  char_given_cmd=70, /* character code defined by \.{\\chardef} */
#define min_internal_cmd char_given_cmd /* the smallest code that can follow \.{\\the} */
  math_given_cmd=71, /* math code defined by \.{\\mathchardef} */
  omath_given_cmd=72, /* math code defined by \.{\\omathchardef} */
  last_item_cmd=73, /* most recent item ( \.{\\lastpenalty}, \.{\\lastkern}, \.{\\lastskip} ) */
#define max_non_prefixed_command_cmd last_item_cmd /* largest command code that can't be \.{\\global} */
  toks_register_cmd=74, /* token list register ( \.{\\toks} ) */
  assign_toks_cmd=75, /* special token list ( \.{\\output}, \.{\\everypar}, etc.~) */
  assign_int_cmd=76, /* user-defined integer ( \.{\\tolerance}, \.{\\day}, etc.~) */
  assign_attr_cmd=77, /*  user-defined attributes  */
  assign_dimen_cmd=78, /* user-defined length ( \.{\\hsize}, etc.~) */
  assign_glue_cmd=79, /* user-defined glue ( \.{\\baselineskip}, etc.~) */
  assign_mu_glue_cmd=80, /* user-defined muglue ( \.{\\thinmuskip}, etc.~) */
  assign_font_dimen_cmd=81, /* user-defined font dimension ( \.{\\fontdimen} ) */
  assign_font_int_cmd=82, /* user-defined font integer ( \.{\\hyphenchar}, \.{\\skewchar} ) */
  set_aux_cmd=83, /* specify state info ( \.{\\spacefactor}, \.{\\prevdepth} ) */
  set_prev_graf_cmd=84, /* specify state info ( \.{\\prevgraf} ) */
  set_page_dimen_cmd=85, /* specify state info ( \.{\\pagegoal}, etc.~) */
  set_page_int_cmd=86, /* specify state info ( \.{\\deadcycles},  \.{\\insertpenalties} ) */
  set_box_dimen_cmd=87, /* change dimension of box ( \.{\\wd}, \.{\\ht}, \.{\\dp} ) */
  set_shape_cmd=88, /* specify fancy paragraph shape ( \.{\\parshape} ) */
  def_code_cmd=89, /* define a character code ( \.{\\catcode}, etc.~) */
  extdef_code_cmd=90, /* define an extended character code ( \.{\\omathcode}, etc.~) */
  def_family_cmd=91, /* declare math fonts ( \.{\\textfont}, etc.~) */
  set_font_cmd=92, /* set current font ( font identifiers ) */
  def_font_cmd=93, /* define a font file ( \.{\\font} ) */
  register_cmd=94, /* internal register ( \.{\\count}, \.{\\dimen}, etc.~) */
  assign_box_dir_cmd=95, /* (\.{\\boxdir}) */
  assign_dir_cmd=96, /* (\.{\\pagedir}, \.{\\textdir}) */
#define max_internal_cmd assign_dir_cmd  /* the largest code that can follow \.{\\the} */
  advance_cmd=97, /* advance a register or parameter ( \.{\\advance} ) */
  multiply_cmd=98, /* multiply a register or parameter ( \.{\\multiply} ) */
  divide_cmd=99, /* divide a register or parameter ( \.{\\divide} ) */
  prefix_cmd=100, /* qualify a definition ( \.{\\global}, \.{\\long}, \.{\\outer} ) */
  let_cmd=101, /* assign a command code ( \.{\\let}, \.{\\futurelet} ) */
  shorthand_def_cmd=102, /* code definition ( \.{\\chardef}, \.{\\countdef}, etc.~) */
  read_to_cs_cmd=103, /* read into a control sequence ( \.{\\read} ) */
  def_cmd=104, /* macro definition ( \.{\\def}, \.{\\gdef}, \.{\\xdef}, \.{\\edef} ) */
  set_box_cmd=105, /* set a box ( \.{\\setbox} ) */
  hyph_data_cmd=106, /* hyphenation data ( \.{\\hyphenation}, \.{\\patterns} ) */
  set_interaction_cmd=107, /* define level of interaction ( \.{\\batchmode}, etc.~) */
  letterspace_font_cmd=108, /* letterspace a font ( \.{\\letterspacefont} ) */
  pdf_copy_font_cmd=109, /* create a new font instance ( \.{\\pdfcopyfont} ) */
  set_ocp_cmd=110, /* Place a translation process in the stream */
  def_ocp_cmd=111, /* Define and load a translation process */
  set_ocp_list_cmd=112, /* Place a list of OCPs in the stream */
  def_ocp_list_cmd=113, /* Define a list of OCPs */
  clear_ocp_lists_cmd=114, /* Remove all active OCP lists */
  push_ocp_list_cmd=115, /* Add to the sequence of active OCP lists */
  pop_ocp_list_cmd=116, /* Remove from the sequence of active OCP lists */
  ocp_list_op_cmd=117, /* Operations for building a list of OCPs */
  ocp_trace_level_cmd=118, /* Tracing of active OCPs, either 0 or 1 */
#define max_command_cmd ocp_trace_level_cmd /* the largest command code seen at |big_switch| */
  undefined_cs_cmd=119, /* initial state of most |eq_type| fields */
  expand_after_cmd=120, /* special expansion ( \.{\\expandafter} ) */
  no_expand_cmd=121, /* special nonexpansion ( \.{\\noexpand} ) */
  input_cmd=122, /* input a source file ( \.{\\input}, \.{\\endinput} or \.{\\scantokens} or \.{\\scantextokens} ) */
  if_test_cmd=123, /* conditional text ( \.{\\if}, \.{\\ifcase}, etc.~) */
  fi_or_else_cmd=124, /* delimiters for conditionals ( \.{\\else}, etc.~) */
  cs_name_cmd=125, /* make a control sequence from tokens ( \.{\\csname} ) */
  convert_cmd=126, /* convert to text ( \.{\\number}, \.{\\string}, etc.~) */
  the_cmd=127, /* expand an internal quantity ( \.{\\the} or \.{\\unexpanded}, \.{\\detokenize} ) */
  top_bot_mark_cmd=128, /* inserted mark ( \.{\\topmark}, etc.~) */
  call_cmd=129, /* non-long, non-outer control sequence */
  long_call_cmd=130, /* long, non-outer control sequence */
  outer_call_cmd=131, /* non-long, outer control sequence */
  long_outer_call_cmd=132, /* long, outer control sequence */
  end_template_cmd=133, /* end of an alignment template */
  dont_expand_cmd=134, /* the following token was marked by \.{\\noexpand} */
  glue_ref_cmd=135, /* the equivalent points to a glue specification */
  shape_ref_cmd=136, /* the equivalent points to a parshape specification */
  box_ref_cmd=137, /* the equivalent points to a box node, or is |null| */
  data_cmd=138, /* the equivalent is simply a halfword number */
} tex_command_code;



typedef enum {
  int_val_level=0,  /* integer values */
  attr_val_level=1, /* integer values */
  dimen_val_level=2, /* dimension values */
  glue_val_level=3, /* glue specifications */
  mu_val_level=4, /* math glue specifications */
  dir_val_level=5, /* directions */
  ident_val_level=6, /* font identifier */
  tok_val_level=7, /* token lists */
} value_level_code;

