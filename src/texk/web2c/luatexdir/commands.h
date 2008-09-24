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
  relax_cmd=0,         /* do nothing ( \.{\\relax} ) */
#define escape_cmd  relax_cmd /* escape delimiter (called \.\\ in {\sl The \TeX book\/}) */
  left_brace_cmd,       /* beginning of a group ( \.\{ ) */
  right_brace_cmd,      /* ending of a group ( \.\} ) */
  math_shift_cmd,       /* mathematics shift character ( \.\$ ) */
  tab_mark_cmd,         /* alignment delimiter ( \.\&, \.{\\span} ) */
  car_ret_cmd,          /* end of line ( |carriage_return|, \.{\\cr}, \.{\\crcr} ) */
#define out_param_cmd  car_ret_cmd /* output a macro parameter */
  mac_param_cmd,        /* macro parameter symbol ( \.\# ) */
  sup_mark_cmd,         /* superscript ( \.{\char'136} ) */
  sub_mark_cmd,         /* subscript ( \.{\char'137} ) */
  endv_cmd,             /* end of \<v_j> list in alignment template */
#define ignore_cmd endv_cmd /* characters to ignore ( \.{\^\^@@} )*/
  spacer_cmd,           /* characters equivalent to blank space ( \.{\ } ) */
  letter_cmd,           /* characters regarded as letters ( \.{A..Z}, \.{a..z} ) */
  other_char_cmd,       /* none of the special character types */
  par_end_cmd,          /* end of paragraph ( \.{\\par} ) */
#define active_char_cmd par_end_cmd /* characters that invoke macros ( \.{\char`\~} )*/
#define match_cmd par_end_cmd /* match a macro parameter*/
  stop_cmd,             /* end of job ( \.{\\end}, \.{\\dump} ) */
#define comment_cmd stop_cmd /* characters that introduce comments ( \.\% )*/
#define end_match_cmd stop_cmd /* end of parameters to macro*/
  delim_num_cmd,        /* specify delimiter numerically ( \.{\\delimiter} ) */
#define invalid_char_cmd delim_num_cmd /* characters that shouldn't appear ( \.{\^\^?} )*/
#define max_char_code_cmd delim_num_cmd /* largest catcode for individual characters */
  char_num_cmd,         /* character specified numerically ( \.{\\char} ) */
  math_char_num_cmd,    /* explicit math code ( \.{\\mathchar} ) */
  mark_cmd,             /* mark definition ( \.{\\mark} ) */
  xray_cmd,             /* peek inside of \TeX\ ( \.{\\show}, \.{\\showbox}, etc.~) */
  make_box_cmd,         /* make a box ( \.{\\box}, \.{\\copy}, \.{\\hbox}, etc.~) */
  hmove_cmd,            /* horizontal motion ( \.{\\moveleft}, \.{\\moveright} ) */
  vmove_cmd,            /* vertical motion ( \.{\\raise}, \.{\\lower} ) */
  un_hbox_cmd,          /* unglue a box ( \.{\\unhbox}, \.{\\unhcopy} ) */
  un_vbox_cmd,          /* unglue a box ( \.{\\unvbox}, \.{\\unvcopy} or 
                                          \.{\\pagediscards}, \.{\\splitdiscards} ) */
  remove_item_cmd,      /* nullify last item ( \.{\\unpenalty}, \.{\\unkern}, \.{\\unskip} ) */
  hskip_cmd,            /* horizontal glue ( \.{\\hskip}, \.{\\hfil}, etc.~) */
  vskip_cmd,            /* vertical glue ( \.{\\vskip}, \.{\\vfil}, etc.~) */
  mskip_cmd,            /* math glue ( \.{\\mskip} ) */
  kern_cmd,             /* fixed space ( \.{\\kern}) */
  mkern_cmd,            /* math kern ( \.{\\mkern} ) */
  leader_ship_cmd,      /* use a box ( \.{\\shipout}, \.{\\leaders}, etc.~) */
  halign_cmd,           /* horizontal table alignment ( \.{\\halign} ) */
  valign_cmd,           /* vertical table alignment ( \.{\\valign} ) */
  no_align_cmd,         /* temporary escape from alignment ( \.{\\noalign} ) */
  vrule_cmd,            /* vertical rule ( \.{\\vrule} ) */
  hrule_cmd,            /* horizontal rule ( \.{\\hrule} ) */
  insert_cmd,           /* vlist inserted in box ( \.{\\insert} ) */
  vadjust_cmd,          /* vlist inserted in enclosing paragraph ( \.{\\vadjust} ) */
  ignore_spaces_cmd,    /* gobble |spacer| tokens ( \.{\\ignorespaces} ) */
  after_assignment_cmd, /* save till assignment is done ( \.{\\afterassignment} ) */
  after_group_cmd,      /* save till group is done ( \.{\\aftergroup} ) */
  break_penalty_cmd,    /* additional badness ( \.{\\penalty} ) */
  start_par_cmd,        /* begin paragraph ( \.{\\indent}, \.{\\noindent} ) */
  ital_corr_cmd,        /* italic correction ( \.{\\/} ) */
  accent_cmd,           /* attach accent in text ( \.{\\accent} ) */
  math_accent_cmd,      /* attach accent in math ( \.{\\mathaccent} ) */
  discretionary_cmd,    /* discretionary texts ( \.{\\-}, \.{\\discretionary} ) */
  eq_no_cmd,            /* equation number ( \.{\\eqno}, \.{\\leqno} ) */
  left_right_cmd,       /* variable delimiter ( \.{\\left}, \.{\\right} or \.{\\middle} ) */
  math_comp_cmd,        /* component of formula ( \.{\\mathbin}, etc.~) */
  limit_switch_cmd,     /* diddle limit conventions ( \.{\\displaylimits}, etc.~) */
  above_cmd,            /* generalized fraction ( \.{\\above}, \.{\\atop}, etc.~) */
  math_style_cmd,       /* style specification ( \.{\\displaystyle}, etc.~) */
  math_choice_cmd,      /* choice specification ( \.{\\mathchoice} ) */
  non_script_cmd,       /* conditional math glue ( \.{\\nonscript} ) */
  vcenter_cmd,          /* vertically center a vbox ( \.{\\vcenter} ) */
  case_shift_cmd,       /* force specific case ( \.{\\lowercase}, \.{\\uppercase}~) */
  message_cmd,          /* send to user ( \.{\\message}, \.{\\errmessage} ) */
  extension_cmd,        /* extensions to \TeX\ ( \.{\\write}, \.{\\special}, etc.~) */
  in_stream_cmd,        /* files for reading ( \.{\\openin}, \.{\\closein} ) */
  begin_group_cmd,      /* begin local grouping ( \.{\\begingroup} ) */
  end_group_cmd,        /* end local grouping ( \.{\\endgroup} ) */
  omit_cmd,             /* omit alignment template ( \.{\\omit} ) */
  ex_space_cmd,         /* explicit space ( \.{\\\ } ) */
  no_boundary_cmd,      /* suppress boundary ligatures ( \.{\\noboundary} ) */
  radical_cmd,          /* square root and similar signs ( \.{\\radical} ) */
  end_cs_name_cmd,      /* end control sequence ( \.{\\endcsname} ) */
  char_ghost_cmd,       /* \.{\\ghostleft}, \.{\\ghostright} character for kerning */
  assign_local_box_cmd, /* box for guillemets \.{\\localleftbox} or \.{\\localrightbox} */
  char_given_cmd,       /* character code defined by \.{\\chardef} */
#define min_internal_cmd char_given_cmd /* the smallest code that can follow \.{\\the} */
  math_given_cmd,       /* math code defined by \.{\\mathchardef} */
  omath_given_cmd,      /* math code defined by \.{\\omathchardef} */
  last_item_cmd,        /* most recent item ( \.{\\lastpenalty}, \.{\\lastkern}, \.{\\lastskip} ) */
#define max_non_prefixed_command_cmd last_item_cmd /* largest command code that can't be \.{\\global} */
  toks_register_cmd,    /* token list register ( \.{\\toks} ) */
  assign_toks_cmd,      /* special token list ( \.{\\output}, \.{\\everypar}, etc.~) */
  assign_int_cmd,       /* user-defined integer ( \.{\\tolerance}, \.{\\day}, etc.~) */
  assign_attr_cmd,      /*  user-defined attributes  */
  assign_dimen_cmd,     /* user-defined length ( \.{\\hsize}, etc.~) */
  assign_glue_cmd,      /* user-defined glue ( \.{\\baselineskip}, etc.~) */
  assign_mu_glue_cmd,   /* user-defined muglue ( \.{\\thinmuskip}, etc.~) */
  assign_font_dimen_cmd,/* user-defined font dimension ( \.{\\fontdimen} ) */
  assign_font_int_cmd,  /* user-defined font integer ( \.{\\hyphenchar}, \.{\\skewchar} ) */
  set_aux_cmd,          /* specify state info ( \.{\\spacefactor}, \.{\\prevdepth} ) */
  set_prev_graf_cmd,    /* specify state info ( \.{\\prevgraf} ) */
  set_page_dimen_cmd,   /* specify state info ( \.{\\pagegoal}, etc.~) */
  set_page_int_cmd,     /* specify state info ( \.{\\deadcycles},  \.{\\insertpenalties} ) */
  set_box_dimen_cmd,    /* change dimension of box ( \.{\\wd}, \.{\\ht}, \.{\\dp} ) */
  set_shape_cmd,        /* specify fancy paragraph shape ( \.{\\parshape} ) */
  def_code_cmd,         /* define a character code ( \.{\\catcode}, etc.~) */
  extdef_code_cmd,      /* define an extended character code ( \.{\\omathcode}, etc.~) */
  def_family_cmd,       /* declare math fonts ( \.{\\textfont}, etc.~) */
  set_font_cmd,         /* set current font ( font identifiers ) */
  def_font_cmd,         /* define a font file ( \.{\\font} ) */
  register_cmd,         /* internal register ( \.{\\count}, \.{\\dimen}, etc.~) */
  assign_box_dir_cmd,   /* (\.{\\boxdir}) */
  assign_dir_cmd,       /* (\.{\\pagedir}, \.{\\textdir}) */
#define max_internal_cmd assign_dir_cmd  /* the largest code that can follow \.{\\the} */
  advance_cmd,          /* advance a register or parameter ( \.{\\advance} ) */
  multiply_cmd,         /* multiply a register or parameter ( \.{\\multiply} ) */
  divide_cmd,           /* divide a register or parameter ( \.{\\divide} ) */
  prefix_cmd,           /* qualify a definition ( \.{\\global}, \.{\\long}, \.{\\outer} ) */
  let_cmd,              /* assign a command code ( \.{\\let}, \.{\\futurelet} ) */
  shorthand_def_cmd,    /* code definition ( \.{\\chardef}, \.{\\countdef}, etc.~) */
  read_to_cs_cmd,       /* read into a control sequence ( \.{\\read} ) */
  def_cmd,              /* macro definition ( \.{\\def}, \.{\\gdef}, \.{\\xdef}, \.{\\edef} ) */
  set_box_cmd,          /* set a box ( \.{\\setbox} ) */
  hyph_data_cmd,        /* hyphenation data ( \.{\\hyphenation}, \.{\\patterns} ) */
  set_interaction_cmd,  /* define level of interaction ( \.{\\batchmode}, etc.~) */
  letterspace_font_cmd, /* letterspace a font ( \.{\\letterspacefont} ) */
  pdf_copy_font_cmd,    /* create a new font instance ( \.{\\pdfcopyfont} ) */
  set_ocp_cmd,          /* Place a translation process in the stream */
  def_ocp_cmd,          /* Define and load a translation process */
  set_ocp_list_cmd,     /* Place a list of OCPs in the stream */
  def_ocp_list_cmd,     /* Define a list of OCPs */
  clear_ocp_lists_cmd,  /* Remove all active OCP lists */
  push_ocp_list_cmd,    /* Add to the sequence of active OCP lists */
  pop_ocp_list_cmd,     /* Remove from the sequence of active OCP lists */
  ocp_list_op_cmd,      /* Operations for building a list of OCPs */
  ocp_trace_level_cmd,  /* Tracing of active OCPs, either 0 or 1 */
#define max_command_cmd ocp_trace_level_cmd /* the largest command code seen at |big_switch| */
  undefined_cs_cmd,     /* initial state of most |eq_type| fields */
  expand_after_cmd,     /* special expansion ( \.{\\expandafter} ) */
  no_expand_cmd,        /* special nonexpansion ( \.{\\noexpand} ) */
  input_cmd,            /* input a source file ( \.{\\input}, \.{\\endinput} or 
                                                 \.{\\scantokens} or \.{\\scantextokens} ) */
  if_test_cmd,          /* conditional text ( \.{\\if}, \.{\\ifcase}, etc.~) */
  fi_or_else_cmd,       /* delimiters for conditionals ( \.{\\else}, etc.~) */
  cs_name_cmd,          /* make a control sequence from tokens ( \.{\\csname} ) */
  convert_cmd,          /* convert to text ( \.{\\number}, \.{\\string}, etc.~) */
  the_cmd,              /* expand an internal quantity ( \.{\\the} or \.{\\unexpanded}, \.{\\detokenize} ) */
  top_bot_mark_cmd,     /* inserted mark ( \.{\\topmark}, etc.~) */
  call_cmd,             /* non-long, non-outer control sequence */
  long_call_cmd,        /* long, non-outer control sequence */
  outer_call_cmd,       /* non-long, outer control sequence */
  long_outer_call_cmd,  /* long, outer control sequence */
  end_template_cmd,     /* end of an alignment template */
  dont_expand_cmd,      /* the following token was marked by \.{\\noexpand} */
  glue_ref_cmd,         /* the equivalent points to a glue specification */
  shape_ref_cmd,        /* the equivalent points to a parshape specification */
  box_ref_cmd,          /* the equivalent points to a box node, or is |null| */
  data_cmd,             /* the equivalent is simply a halfword number */
} tex_command_code;


typedef enum {
  int_val_level = 0, /* integer values */
  attr_val_level,    /* integer values */
  dimen_val_level,   /* dimension values */
  glue_val_level,    /* glue specifications */
  mu_val_level,      /* math glue specifications */
  dir_val_level,     /* directions */
  ident_val_level,   /* font identifier */
  tok_val_level,     /* token lists */
} value_level_code;


typedef enum {
  convert_number_code = 0          , /* command code for \.{\\number} */
  convert_roman_numeral_code       , /* command code for \.{\\romannumeral} */
  convert_string_code              , /* command code for \.{\\string} */
  convert_meaning_code             , /* command code for \.{\\meaning} */
  convert_font_name_code           , /* command code for \.{\\fontname} */
  convert_etex_code                , /* command code for \.{\\eTeXVersion} */
  convert_omega_code               , /* command code for \.{\\OmegaVersion} */
  convert_aleph_code               , /* command code for \.{\\AlephVersion} */
  convert_format_name_code         , /* command code for \.{\\AlephVersion} */
  convert_pdftex_revision_code     , /* command code for \.{\\pdftexrevision} */
#define convert_pdftex_first_expand_code convert_pdftex_revision_code /* base for \pdfTeX's command codes */
  convert_pdftex_banner_code       , /* command code for \.{\\pdftexbanner} */
  convert_pdf_font_name_code       , /* command code for \.{\\pdffontname} */
  convert_pdf_font_objnum_code     , /* command code for \.{\\pdffontobjnum} */
  convert_pdf_font_size_code       , /* command code for \.{\\pdffontsize} */
  convert_pdf_page_ref_code        , /* command code for \.{\\pdfpageref} */
  convert_pdf_xform_name_code      , /* command code for \.{\\pdfxformname} */
  convert_left_margin_kern_code    , /* command code for \.{\\leftmarginkern} */
  convert_right_margin_kern_code   , /* command code for \.{\\rightmarginkern} */
  convert_pdf_creation_date_code   , /* command code for \.{\\pdfcreationdate} */
  convert_uniform_deviate_code     , /* command code for \.{\\uniformdeviate} */
  convert_normal_deviate_code      , /* command code for \.{\\normaldeviate} */
  convert_pdf_insert_ht_code       , /* command code for \.{\\pdfinsertht} */
  convert_pdf_ximage_bbox_code     , /* command code for \.{\\pdfximagebbox} */
  convert_lua_code                 , /* command code for \.{\\directlua} */
  convert_lua_escape_string_code   , /* command code for \.{\\luaescapestring} */
  convert_pdf_colorstack_init_code , /* command code for \.{\\pdfcolorstackinit} */
  convert_luatex_revision_code     , /* command code for \.{\\luatexrevision} */
  convert_luatex_date_code         , /* command code for \.{\\luatexdate} */
  convert_expanded_code            , /* command code for \.{\\expanded} */
  convert_job_name_code            , /* command code for \.{\\jobname} */
#define  convert_pdftex_convert_codes convert_job_name_code /* end of \pdfTeX's command codes */
  convert_Aleph_revision_code      , /* command code for \.{\\Alephrevision} */
  convert_Omega_revision_code      , /* command code for \.{\\Omegarevision} */
  convert_eTeX_revision_code       , /* command code for \.{\\eTeXrevision} */
  convert_font_identifier_code     , /* command code for \.{tex.fontidentifier} (virtual) */
} convert_codes;
