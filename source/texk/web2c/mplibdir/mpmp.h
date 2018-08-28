/*4:*/
#line 113 "./mp.w"

#ifndef MPMP_H
#define MPMP_H 1
#include "avl.h"
#include "mplib.h"
#include <setjmp.h> 
typedef struct psout_data_struct*psout_data;
typedef struct svgout_data_struct*svgout_data;
typedef struct pngout_data_struct*pngout_data;
#ifndef HAVE_BOOLEAN
typedef int boolean;
#endif
#ifndef INTEGER_TYPE
typedef int integer;
#endif
/*170:*/
#line 2810 "./mp.w"

extern void mp_xfree(void*x);
extern void*mp_xrealloc(MP mp,void*p,size_t nmem,size_t size);
extern void*mp_xmalloc(MP mp,size_t nmem,size_t size);
extern void mp_do_snprintf(char*str,int size,const char*fmt,...);
extern void*do_alloc_node(MP mp,size_t size);

/*:170*/
#line 128 "./mp.w"
;
/*190:*/
#line 3205 "./mp.w"

typedef enum{
mp_start_tex= 1,
mp_etex_marker,
mp_mpx_break,
mp_if_test,
mp_fi_or_else,
mp_input,
mp_iteration,
mp_repeat_loop,
mp_exit_test,
mp_relax,
mp_scan_tokens,
mp_runscript,
mp_maketext,
mp_expand_after,
mp_defined_macro,
mp_save_command,
mp_interim_command,
mp_let_command,
mp_new_internal,
mp_macro_def,
mp_ship_out_command,
mp_add_to_command,
mp_bounds_command,
mp_tfm_command,
mp_protection_command,
mp_show_command,
mp_mode_command,
mp_random_seed,
mp_message_command,
mp_every_job_command,
mp_delimiters,
mp_special_command,
mp_write_command,
mp_type_name,
mp_left_delimiter,
mp_begin_group,
mp_nullary,
mp_unary,
mp_str_op,
mp_cycle,
mp_primary_binary,
mp_capsule_token,
mp_string_token,
mp_internal_quantity,
mp_tag_token,
mp_numeric_token,
mp_plus_or_minus,
mp_tertiary_secondary_macro,
mp_tertiary_binary,
mp_left_brace,
mp_path_join,
mp_ampersand,
mp_expression_tertiary_macro,
mp_expression_binary,
mp_equals,
mp_and_command,
mp_secondary_primary_macro,
mp_slash,
mp_secondary_binary,
mp_param_type,
mp_controls,
mp_tension,
mp_at_least,
mp_curl_command,
mp_macro_special,
mp_right_delimiter,
mp_left_bracket,
mp_right_bracket,
mp_right_brace,
mp_with_option,
mp_thing_to_add,
mp_of_token,
mp_to_token,
mp_step_token,
mp_until_token,
mp_within_token,
mp_lig_kern_token,
mp_assignment,
mp_skip_to,
mp_bchar_label,
mp_double_colon,
mp_colon,

mp_comma,
mp_semicolon,
mp_end_group,
mp_stop,
mp_outer_tag,
mp_undefined_cs,
}mp_command_code;

/*:190*//*191:*/
#line 3312 "./mp.w"

typedef enum{
mp_undefined= 0,
mp_vacuous,
mp_boolean_type,
mp_unknown_boolean,
mp_string_type,
mp_unknown_string,
mp_pen_type,
mp_unknown_pen,
mp_path_type,
mp_unknown_path,
mp_picture_type,
mp_unknown_picture,
mp_transform_type,
mp_color_type,
mp_cmykcolor_type,
mp_pair_type,
mp_numeric_type,
mp_known,
mp_dependent,
mp_proto_dependent,
mp_independent,
mp_token_list,
mp_structured,
mp_unsuffixed_macro,
mp_suffixed_macro,

mp_symbol_node,
mp_token_node_type,
mp_value_node_type,
mp_attr_node_type,
mp_subscr_node_type,
mp_pair_node_type,
mp_transform_node_type,
mp_color_node_type,
mp_cmykcolor_node_type,

mp_fill_node_type,
mp_stroked_node_type,
mp_text_node_type,
mp_start_clip_node_type,
mp_start_bounds_node_type,
mp_stop_clip_node_type,
mp_stop_bounds_node_type,
mp_dash_node_type,
mp_dep_node_type,
mp_if_node_type,
mp_edge_header_node_type,
}mp_variable_type;

/*:191*//*194:*/
#line 3527 "./mp.w"

typedef enum{
mp_root= 0,
mp_saved_root,
mp_structured_root,
mp_subscr,
mp_attr,
mp_x_part_sector,
mp_y_part_sector,
mp_xx_part_sector,
mp_xy_part_sector,
mp_yx_part_sector,
mp_yy_part_sector,
mp_red_part_sector,
mp_green_part_sector,
mp_blue_part_sector,
mp_cyan_part_sector,
mp_magenta_part_sector,
mp_yellow_part_sector,
mp_black_part_sector,
mp_grey_part_sector,
mp_capsule,
mp_token,

mp_normal_sym,
mp_internal_sym,
mp_macro_sym,
mp_expr_sym,
mp_suffix_sym,
mp_text_sym,
/*195:*/
#line 3575 "./mp.w"

mp_true_code,
mp_false_code,
mp_null_picture_code,
mp_null_pen_code,
mp_read_string_op,
mp_pen_circle,
mp_normal_deviate,
mp_read_from_op,
mp_close_from_op,
mp_odd_op,
mp_known_op,
mp_unknown_op,
mp_not_op,
mp_decimal,
mp_reverse,
mp_make_path_op,
mp_make_pen_op,
mp_oct_op,
mp_hex_op,
mp_ASCII_op,
mp_char_op,
mp_length_op,
mp_turning_op,
mp_color_model_part,
mp_x_part,
mp_y_part,
mp_xx_part,
mp_xy_part,
mp_yx_part,
mp_yy_part,
mp_red_part,
mp_green_part,
mp_blue_part,
mp_cyan_part,
mp_magenta_part,
mp_yellow_part,
mp_black_part,
mp_grey_part,
mp_font_part,
mp_text_part,
mp_path_part,
mp_pen_part,
mp_dash_part,
mp_prescript_part,
mp_postscript_part,
mp_sqrt_op,
mp_m_exp_op,
mp_m_log_op,
mp_sin_d_op,
mp_cos_d_op,
mp_floor_op,
mp_uniform_deviate,
mp_char_exists_op,
mp_font_size,
mp_ll_corner_op,
mp_lr_corner_op,
mp_ul_corner_op,
mp_ur_corner_op,
mp_arc_length,
mp_angle_op,
mp_cycle_op,
mp_filled_op,
mp_stroked_op,
mp_textual_op,
mp_clipped_op,
mp_bounded_op,
mp_plus,
mp_minus,
mp_times,
mp_over,
mp_pythag_add,
mp_pythag_sub,
mp_or_op,
mp_and_op,
mp_less_than,
mp_less_or_equal,
mp_greater_than,
mp_greater_or_equal,
mp_equal_to,
mp_unequal_to,
mp_concatenate,
mp_rotated_by,
mp_slanted_by,
mp_scaled_by,
mp_shifted_by,
mp_transformed_by,
mp_x_scaled,
mp_y_scaled,
mp_z_scaled,
mp_in_font,
mp_intersect,
mp_double_dot,
mp_substring_of,
mp_subpath_of,
mp_direction_time_of,
mp_point_of,
mp_precontrol_of,
mp_postcontrol_of,
mp_pen_offset_of,
mp_arc_time_of,
mp_version,
mp_envelope_of,
mp_boundingpath_of,
mp_glyph_infont,
mp_kern_flag

/*:195*/
#line 3557 "./mp.w"

}mp_name_type_type;

/*:194*/
#line 129 "./mp.w"
;
/*36:*/
#line 786 "./mp.w"

typedef unsigned char ASCII_code;

/*:36*//*37:*/
#line 794 "./mp.w"

typedef unsigned char text_char;

/*:37*//*44:*/
#line 882 "./mp.w"

typedef unsigned char eight_bits;

/*:44*//*166:*/
#line 2753 "./mp.w"

typedef struct mp_value_node_data*mp_value_node;
typedef struct mp_node_data*mp_node;
typedef struct mp_symbol_entry*mp_sym;
typedef short quarterword;
typedef int halfword;
typedef struct{
integer scale;
integer serial;
}mp_independent_data;
typedef struct{
mp_independent_data indep;
mp_number n;
mp_string str;
mp_sym sym;
mp_node node;
mp_knot p;
}mp_value_data;
typedef struct{
mp_variable_type type;
mp_value_data data;
}mp_value;
typedef struct{
quarterword b0,b1,b2,b3;
}four_quarters;
typedef union{
integer sc;
four_quarters qqqq;
}font_data;


/*:166*//*197:*/
#line 4008 "./mp.w"

enum mp_given_internal{
mp_output_template= 1,
mp_output_filename,
mp_output_format,
mp_output_format_options,
mp_number_system,
mp_number_precision,
mp_job_name,

mp_tracing_titles,
mp_tracing_equations,
mp_tracing_capsules,
mp_tracing_choices,
mp_tracing_specs,
mp_tracing_commands,
mp_tracing_restores,
mp_tracing_macros,
mp_tracing_output,
mp_tracing_stats,
mp_tracing_lost_chars,
mp_tracing_online,
mp_year,
mp_month,
mp_day,
mp_time,
mp_hour,
mp_minute,
mp_char_code,
mp_char_ext,
mp_char_wd,
mp_char_ht,
mp_char_dp,
mp_char_ic,
mp_design_size,
mp_pausing,
mp_showstopping,
mp_fontmaking,
mp_texscriptmode,
mp_linejoin,
mp_linecap,
mp_miterlimit,
mp_warning_check,
mp_boundary_char,
mp_prologues,
mp_true_corners,
mp_default_color_model,
mp_restore_clip_color,
mp_procset,
mp_hppp,
mp_vppp,
mp_gtroffmode,
};
typedef struct{
mp_value v;
char*intname;
}mp_internal;


/*:197*//*220:*/
#line 4587 "./mp.w"

typedef struct mp_symbol_entry{
halfword type;
mp_value v;
mp_string text;
void*parent;
}mp_symbol_entry;

/*:220*//*255:*/
#line 5566 "./mp.w"

typedef enum{
mp_general_macro,
mp_primary_macro,
mp_secondary_macro,
mp_tertiary_macro,
mp_expr_macro,
mp_of_macro,
mp_suffix_macro,
mp_text_macro,
mp_expr_param,
mp_suffix_param,
mp_text_param
}mp_macro_info;

/*:255*//*297:*/
#line 6872 "./mp.w"

typedef struct mp_save_data{
quarterword type;
mp_internal value;
struct mp_save_data*link;
}mp_save_data;

/*:297*//*390:*/
#line 9262 "./mp.w"

enum mp_bb_code{
mp_x_code= 0,
mp_y_code
};

/*:390*//*485:*/
#line 11648 "./mp.w"

typedef struct mp_dash_node_data*mp_dash_node;

/*:485*//*681:*/
#line 17487 "./mp.w"

typedef struct{
char*long_name_field;
halfword start_field,loc_field,limit_field;
mp_node nstart_field,nloc_field;
mp_string name_field;
quarterword index_field;
}in_state_record;

/*:681*//*755:*/
#line 19123 "./mp.w"

typedef struct mp_subst_list_item{
mp_name_type_type info_mod;
quarterword value_mod;
mp_sym info;
halfword value_data;
struct mp_subst_list_item*link;
}mp_subst_list_item;

/*:755*//*833:*/
#line 20981 "./mp.w"

typedef struct mp_loop_data{
mp_sym var;
mp_node info;
mp_node type;

mp_node list;
mp_node list_start;
mp_number old_value;
mp_number value;
mp_number step_size;
mp_number final_value;
struct mp_loop_data*link;
}mp_loop_data;

/*:833*//*904:*/
#line 22226 "./mp.w"

typedef unsigned int readf_index;
typedef unsigned int write_index;

/*:904*//*1070:*/
#line 30385 "./mp.w"

typedef struct File{
FILE*f;
}File;

/*:1070*//*1238:*/
#line 34127 "./mp.w"

typedef unsigned int font_number;

/*:1238*/
#line 130 "./mp.w"
;
/*26:*/
#line 700 "./mp.w"

#define bistack_size 1500       


/*:26*/
#line 131 "./mp.w"
;
typedef struct MP_instance{
/*29:*/
#line 714 "./mp.w"

int error_line;
int half_error_line;

int halt_on_error;
int max_print_line;
void*userdata;
char*banner;
int ini_version;

/*:29*//*46:*/
#line 912 "./mp.w"

mp_file_finder find_file;
mp_file_opener open_file;
mp_script_runner run_script;
mp_text_maker make_text;
mp_file_reader read_ascii_file;
mp_binfile_reader read_binary_file;
mp_file_closer close_file;
mp_file_eoftest eof_file;
mp_file_flush flush_file;
mp_file_writer write_ascii_file;
mp_binfile_writer write_binary_file;

/*:46*//*53:*/
#line 997 "./mp.w"

int print_found_names;

/*:53*//*55:*/
#line 1015 "./mp.w"

int file_line_error_style;

/*:55*//*71:*/
#line 1256 "./mp.w"

char*command_line;

/*:71*//*104:*/
#line 1849 "./mp.w"

int interaction;
int noninteractive;
int extensions;

/*:104*//*124:*/
#line 2084 "./mp.w"

mp_editor_cmd run_editor;

/*:124*//*156:*/
#line 2587 "./mp.w"

int random_seed;

/*:156*//*168:*/
#line 2797 "./mp.w"

int math_mode;

/*:168*//*200:*/
#line 4098 "./mp.w"

int troff_mode;

/*:200*//*865:*/
#line 21619 "./mp.w"

char*mem_name;

/*:865*//*878:*/
#line 21814 "./mp.w"

char*job_name;

/*:878*//*899:*/
#line 22179 "./mp.w"

mp_makempx_cmd run_make_mpx;

/*:899*//*1289:*/
#line 35129 "./mp.w"

mp_backend_writer shipout_backend;

/*:1289*/
#line 133 "./mp.w"

/*16:*/
#line 353 "./mp.w"

void*math;

/*:16*//*28:*/
#line 707 "./mp.w"

int pool_size;

int max_in_open;

int param_size;

/*:28*//*32:*/
#line 749 "./mp.w"

integer bad;

/*:32*//*40:*/
#line 808 "./mp.w"

ASCII_code xord[256];
text_char xchr[256];

/*:40*//*52:*/
#line 990 "./mp.w"

char*name_of_file;

/*:52*//*65:*/
#line 1157 "./mp.w"

size_t buf_size;

ASCII_code*buffer;
size_t first;
size_t last;
size_t max_buf_stack;

/*:65*//*70:*/
#line 1231 "./mp.w"

void*term_in;
void*term_out;
void*err_out;

/*:70*//*78:*/
#line 1392 "./mp.w"

avl_tree strings;
unsigned char*cur_string;
size_t cur_length;
size_t cur_string_size;

/*:78*//*81:*/
#line 1406 "./mp.w"

integer pool_in_use;
integer max_pl_used;
integer strs_in_use;
integer max_strs_used;


/*:81*//*82:*/
#line 1465 "./mp.w"

void*log_file;
void*output_file;
unsigned int selector;
integer tally;
unsigned int term_offset;

unsigned int file_offset;

ASCII_code*trick_buf;
integer trick_count;
integer first_count;

/*:82*//*110:*/
#line 1925 "./mp.w"

int history;
int error_count;

/*:110*//*114:*/
#line 1954 "./mp.w"

boolean use_err_help;
mp_string err_help;

/*:114*//*116:*/
#line 1970 "./mp.w"

jmp_buf*jump_buf;

/*:116*//*143:*/
#line 2425 "./mp.w"

integer interrupt;
boolean OK_to_interrupt;
integer run_state;
boolean finished;
boolean reading_preload;

/*:143*//*147:*/
#line 2484 "./mp.w"

boolean arith_error;

/*:147*//*155:*/
#line 2583 "./mp.w"

mp_number randoms[55];
int j_random;

/*:155*//*171:*/
#line 2825 "./mp.w"

mp_node token_nodes;
int num_token_nodes;
mp_node pair_nodes;
int num_pair_nodes;
mp_knot knot_nodes;
int num_knot_nodes;
mp_node value_nodes;
int num_value_nodes;
mp_node symbolic_nodes;
int num_symbolic_nodes;

/*:171*//*180:*/
#line 2977 "./mp.w"

size_t var_used;
size_t var_used_max;

/*:180*//*186:*/
#line 3119 "./mp.w"

mp_dash_node null_dash;
mp_value_node dep_head;
mp_node inf_val;
mp_node zero_val;
mp_node temp_val;
mp_node end_attr;
mp_node bad_vardef;
mp_node temp_head;
mp_node hold_head;
mp_node spec_head;

/*:186*//*199:*/
#line 4093 "./mp.w"

mp_internal*internal;
int int_ptr;
int max_internal;

/*:199*//*213:*/
#line 4407 "./mp.w"

unsigned int old_setting;

/*:213*//*215:*/
#line 4449 "./mp.w"

#define digit_class 0 
int char_class[256];

/*:215*//*221:*/
#line 4595 "./mp.w"

integer st_count;
avl_tree symbols;
avl_tree frozen_symbols;
mp_sym frozen_bad_vardef;
mp_sym frozen_colon;
mp_sym frozen_end_def;
mp_sym frozen_end_for;
mp_sym frozen_end_group;
mp_sym frozen_etex;
mp_sym frozen_fi;
mp_sym frozen_inaccessible;
mp_sym frozen_left_bracket;
mp_sym frozen_mpx_break;
mp_sym frozen_repeat_loop;
mp_sym frozen_right_delimiter;
mp_sym frozen_semicolon;
mp_sym frozen_slash;
mp_sym frozen_undefined;
mp_sym frozen_dump;


/*:221*//*230:*/
#line 4724 "./mp.w"

mp_sym id_lookup_test;

/*:230*//*298:*/
#line 6879 "./mp.w"

mp_save_data*save_ptr;

/*:298*//*332:*/
#line 7580 "./mp.w"

mp_knot path_tail;

/*:332*//*347:*/
#line 7894 "./mp.w"

int path_size;
mp_number*delta_x;
mp_number*delta_y;
mp_number*delta;
mp_number*psi;

/*:347*//*352:*/
#line 8049 "./mp.w"

mp_number*theta;
mp_number*uu;
mp_number*vv;
mp_number*ww;

/*:352*//*374:*/
#line 8659 "./mp.w"

mp_number st;
mp_number ct;
mp_number sf;
mp_number cf;

/*:374*//*391:*/
#line 9274 "./mp.w"

mp_number bbmin[mp_y_code+1];
mp_number bbmax[mp_y_code+1];


/*:391*//*437:*/
#line 10530 "./mp.w"

mp_number half_cos[8];
mp_number d_cos[8];

/*:437*//*454:*/
#line 10857 "./mp.w"

mp_number cur_x;
mp_number cur_y;

/*:454*//*550:*/
#line 13227 "./mp.w"

integer spec_offset;

/*:550*//*553:*/
#line 13390 "./mp.w"

mp_knot spec_p1;
mp_knot spec_p2;

/*:553*//*612:*/
#line 15586 "./mp.w"

unsigned int tol_step;

/*:612*//*613:*/
#line 15653 "./mp.w"

mp_number*bisect_stack;
integer bisect_ptr;

/*:613*//*618:*/
#line 15740 "./mp.w"

mp_number cur_t;
mp_number cur_tt;
integer time_to_go;
mp_number max_t;

/*:618*//*622:*/
#line 15859 "./mp.w"

mp_number delx;
mp_number dely;
integer tol;
integer uv;
integer xy;
integer three_l;
mp_number appr_t;
mp_number appr_tt;

/*:622*//*631:*/
#line 16095 "./mp.w"

integer serial_no;

/*:631*//*642:*/
#line 16295 "./mp.w"

boolean fix_needed;
boolean watch_coefs;
mp_value_node dep_final;

/*:642*//*675:*/
#line 17434 "./mp.w"

mp_node cur_mod_;

/*:675*//*682:*/
#line 17496 "./mp.w"

in_state_record*input_stack;
integer input_ptr;
integer max_in_stack;
in_state_record cur_input;
int stack_size;

/*:682*//*687:*/
#line 17603 "./mp.w"

integer in_open;
integer in_open_max;
unsigned int open_parens;
void**input_file;
integer*line_stack;
char**inext_stack;
char**iname_stack;
char**iarea_stack;
mp_string*mpx_name;

/*:687*//*693:*/
#line 17732 "./mp.w"

mp_node*param_stack;
integer param_ptr;
integer max_param_stack;

/*:693*//*699:*/
#line 17787 "./mp.w"

integer file_ptr;

/*:699*//*727:*/
#line 18385 "./mp.w"

#define tex_flushing 7 
integer scanner_status;
mp_sym warning_info;

integer warning_line;
mp_node warning_info_node;

/*:727*//*738:*/
#line 18751 "./mp.w"

boolean force_eof;

/*:738*//*770:*/
#line 19569 "./mp.w"

mp_sym bg_loc;
mp_sym eg_loc;

/*:770*//*774:*/
#line 19623 "./mp.w"

int expand_depth_count;
int expand_depth;

/*:774*//*819:*/
#line 20697 "./mp.w"

mp_node cond_ptr;
integer if_limit;
quarterword cur_if;
integer if_line;

/*:819*//*834:*/
#line 20996 "./mp.w"

mp_loop_data*loop_ptr;

/*:834*//*853:*/
#line 21444 "./mp.w"

char*cur_name;
char*cur_area;
char*cur_ext;

/*:853*//*856:*/
#line 21472 "./mp.w"

integer area_delimiter;

integer ext_delimiter;
boolean quoted_filename;

/*:856*//*877:*/
#line 21810 "./mp.w"

boolean log_opened;
char*log_name;

/*:877*//*905:*/
#line 22230 "./mp.w"

readf_index max_read_files;
void**rd_file;
char**rd_fname;
readf_index read_files;
write_index max_write_files;
void**wr_file;
char**wr_fname;
write_index write_files;

/*:905*//*911:*/
#line 22373 "./mp.w"

mp_value cur_exp;

/*:911*//*938:*/
#line 23301 "./mp.w"

mp_number max_c[mp_proto_dependent+1];
mp_value_node max_ptr[mp_proto_dependent+1];
mp_value_node max_link[mp_proto_dependent+1];


/*:938*//*941:*/
#line 23342 "./mp.w"

int var_flag;

/*:941*//*998:*/
#line 27051 "./mp.w"

mp_string eof_line;

/*:998*//*1011:*/
#line 28240 "./mp.w"

mp_number txx;
mp_number txy;
mp_number tyx;
mp_number tyy;
mp_number tx;
mp_number ty;

/*:1011*//*1069:*/
#line 30378 "./mp.w"

mp_run_data run_data;

/*:1069*//*1142:*/
#line 31995 "./mp.w"

quarterword last_add_type;


/*:1142*//*1153:*/
#line 32254 "./mp.w"

mp_sym start_sym;

/*:1153*//*1162:*/
#line 32385 "./mp.w"

boolean long_help_seen;

/*:1162*//*1170:*/
#line 32542 "./mp.w"

void*tfm_file;
char*metric_file_name;

/*:1170*//*1179:*/
#line 32801 "./mp.w"

#define TFM_ITEMS 257
eight_bits bc;
eight_bits ec;
mp_node tfm_width[TFM_ITEMS];
mp_node tfm_height[TFM_ITEMS];
mp_node tfm_depth[TFM_ITEMS];
mp_node tfm_ital_corr[TFM_ITEMS];
boolean char_exists[TFM_ITEMS];
int char_tag[TFM_ITEMS];
int char_remainder[TFM_ITEMS];
char*header_byte;
int header_last;
int header_size;
four_quarters*lig_kern;
short nl;
mp_number*kern;
short nk;
four_quarters exten[TFM_ITEMS];
short ne;
mp_number*param;
short np;
short nw;
short nh;
short nd;
short ni;
short skip_table[TFM_ITEMS];
boolean lk_started;
integer bchar;
short bch_label;
short ll;
short lll;
short label_loc[257];
eight_bits label_char[257];
short label_ptr;

/*:1179*//*1209:*/
#line 33511 "./mp.w"

mp_number perturbation;
integer excess;

/*:1209*//*1217:*/
#line 33661 "./mp.w"

mp_node dimen_head[5];

/*:1217*//*1223:*/
#line 33808 "./mp.w"

mp_number max_tfm_dimen;
integer tfm_changed;

/*:1223*//*1237:*/
#line 34118 "./mp.w"

void*tfm_infile;

/*:1237*//*1239:*/
#line 34134 "./mp.w"

font_number font_max;
size_t font_mem_size;
font_data*font_info;
char**font_enc_name;
boolean*font_ps_name_fixed;
size_t next_fmem;
font_number last_fnum;
integer*font_dsize;
char**font_name;
char**font_ps_name;
font_number last_ps_fnum;
eight_bits*font_bc;
eight_bits*font_ec;
int*char_base;
int*width_base;
int*height_base;
int*depth_base;
mp_node*font_sizes;

/*:1239*//*1259:*/
#line 34450 "./mp.w"

integer ten_pow[10];
integer scaled_out;

/*:1259*//*1267:*/
#line 34719 "./mp.w"

char*first_file_name;
char*last_file_name;
integer first_output_code;
integer last_output_code;

integer total_shipped;

/*:1267*//*1275:*/
#line 34792 "./mp.w"

mp_node last_pending;


/*:1275*//*1291:*/
#line 35135 "./mp.w"

psout_data ps;
svgout_data svg;
pngout_data png;

/*:1291*//*1294:*/
#line 35162 "./mp.w"

void*mem_file;

/*:1294*/
#line 134 "./mp.w"

}MP_instance;
/*12:*/
#line 306 "./mp.w"

/*873:*/
#line 21766 "./mp.w"

void mp_str_scan_file(MP mp,mp_string s);

/*:873*//*875:*/
#line 21786 "./mp.w"

extern void mp_ptr_scan_file(MP mp,char*s);

/*:875*/
#line 307 "./mp.w"



/*:12*//*88:*/
#line 1536 "./mp.w"

void mp_print(MP mp,const char*s);
void mp_printf(MP mp,const char*ss,...);
void mp_print_ln(MP mp);
void mp_print_char(MP mp,ASCII_code k);
void mp_print_str(MP mp,mp_string s);
void mp_print_nl(MP mp,const char*s);
void mp_print_two(MP mp,mp_number x,mp_number y);

/*:88*//*98:*/
#line 1767 "./mp.w"

void mp_print_int(MP mp,integer n);
void mp_print_pointer(MP mp,void*n);

/*:98*//*113:*/
#line 1951 "./mp.w"

void mp_normalize_selector(MP mp);

/*:113*//*118:*/
#line 1984 "./mp.w"

void mp_jump_out(MP mp);

/*:118*//*139:*/
#line 2353 "./mp.w"

void mp_overflow(MP mp,const char*s,integer n);


/*:139*//*141:*/
#line 2384 "./mp.w"

void mp_confusion(MP mp,const char*s);

/*:141*//*159:*/
#line 2607 "./mp.w"

void mp_new_randoms(MP mp);

/*:159*//*177:*/
#line 2935 "./mp.w"

#  define mp_snprintf (void)snprintf

/*:177*//*185:*/
#line 3110 "./mp.w"

void mp_free_node(MP mp,mp_node p,size_t siz);
void mp_free_symbolic_node(MP mp,mp_node p);
void mp_free_value_node(MP mp,mp_node p);

/*:185*//*336:*/
#line 7677 "./mp.w"

void mp_make_choices(MP mp,mp_knot knots);

/*:336*//*864:*/
#line 21616 "./mp.w"

void mp_pack_file_name(MP mp,const char*n,const char*a,const char*e);

/*:864*//*882:*/
#line 21859 "./mp.w"

void mp_pack_job_name(MP mp,const char*s);

/*:882*//*884:*/
#line 21881 "./mp.w"

void mp_prompt_file_name(MP mp,const char*s,const char*e);

/*:884*//*1108:*/
#line 31091 "./mp.w"

void mp_grow_internals(MP mp,int l);

/*:1108*//*1243:*/
#line 34218 "./mp.w"

void mp_reallocate_fonts(MP mp,font_number l);


/*:1243*//*1262:*/
#line 34474 "./mp.w"

void mp_open_output_file(MP mp);
char*mp_get_output_file_name(MP mp);
char*mp_set_output_file_name(MP mp,integer c);

/*:1262*//*1265:*/
#line 34700 "./mp.w"

void mp_store_true_output_filename(MP mp,int c);

/*:1265*//*1273:*/
#line 34780 "./mp.w"

boolean mp_has_font_size(MP mp,font_number f);

/*:1273*/
#line 136 "./mp.w"

/*8:*/
#line 232 "./mp.w"


#if DEBUG
#define debug_number(A) printf("%d: %s=%.32f (%d)\n", __LINE__, #A, number_to_double(A), number_to_scaled(A))
#else
#define debug_number(A)
#endif
#if DEBUG> 1
void do_debug_printf(MP mp,const char*prefix,const char*fmt,...);
#  define debug_printf(a1,a2,a3) do_debug_printf(mp, "", a1,a2,a3)
#  define FUNCTION_TRACE1(a1) do_debug_printf(mp, "FTRACE: ", a1)
#  define FUNCTION_TRACE2(a1,a2) do_debug_printf(mp, "FTRACE: ", a1,a2)
#  define FUNCTION_TRACE3(a1,a2,a3) do_debug_printf(mp, "FTRACE: ", a1,a2,a3)
#  define FUNCTION_TRACE3X(a1,a2,a3) (void)mp
#  define FUNCTION_TRACE4(a1,a2,a3,a4) do_debug_printf(mp, "FTRACE: ", a1,a2,a3,a4)
#else
#  define debug_printf(a1,a2,a3)
#  define FUNCTION_TRACE1(a1) (void)mp
#  define FUNCTION_TRACE2(a1,a2) (void)mp
#  define FUNCTION_TRACE3(a1,a2,a3) (void)mp
#  define FUNCTION_TRACE3X(a1,a2,a3) (void)mp
#  define FUNCTION_TRACE4(a1,a2,a3,a4) (void)mp
#endif

/*:8*//*39:*/
#line 804 "./mp.w"

#define xchr(A) mp->xchr[(A)]
#define xord(A) mp->xord[(A)]

/*:39*//*72:*/
#line 1273 "./mp.w"

#define update_terminal()  (mp->flush_file)(mp,mp->term_out)      
#define clear_terminal()          
#define wake_up_terminal() (mp->flush_file)(mp,mp->term_out)


/*:72*//*87:*/
#line 1519 "./mp.w"

#define mp_fputs(b,f) (mp->write_ascii_file)(mp,f,b)
#define wterm(A)     mp_fputs((A), mp->term_out)
#define wterm_chr(A) { unsigned char ss[2]; ss[0]= (A); ss[1]= '\0'; wterm((char *)ss);}
#define wterm_cr     mp_fputs("\n", mp->term_out)
#define wterm_ln(A)  { wterm_cr; mp_fputs((A), mp->term_out); }
#define wlog(A)        mp_fputs((A), mp->log_file)
#define wlog_chr(A)  { unsigned char ss[2]; ss[0]= (A); ss[1]= '\0'; wlog((char *)ss);}
#define wlog_cr      mp_fputs("\n", mp->log_file)
#define wlog_ln(A)   { wlog_cr; mp_fputs((A), mp->log_file); }


/*:87*//*179:*/
#line 2956 "./mp.w"

#define NODE_BODY                       \
  mp_variable_type type;                \
  mp_name_type_type name_type;          \
  unsigned short has_number;  \
  struct mp_node_data *link
typedef struct mp_node_data{
NODE_BODY;
mp_value_data data;
}mp_node_data;
typedef struct mp_node_data*mp_symbolic_node;

/*:179*//*198:*/
#line 4067 "./mp.w"

#define internal_value(A) mp->internal[(A)].v.data.n
#define set_internal_from_number(A,B) do { \
  number_clone (internal_value ((A)),(B));\
} while (0)
#define internal_string(A) (mp_string)mp->internal[(A)].v.data.str
#define set_internal_string(A,B) mp->internal[(A)].v.data.str= (B)
#define internal_name(A) mp->internal[(A)].intname
#define set_internal_name(A,B) mp->internal[(A)].intname= (B)
#define internal_type(A) (mp_variable_type)mp->internal[(A)].v.type
#define set_internal_type(A,B) mp->internal[(A)].v.type= (B)
#define set_internal_from_cur_exp(A) do { \
  if (internal_type ((A)) == mp_string_type) { \
      add_str_ref (cur_exp_str ()); \
      set_internal_string ((A), cur_exp_str ()); \
  } else { \
      set_internal_from_number ((A), cur_exp_value_number ()); \
  } \
} while (0)



/*:198*//*242:*/
#line 5206 "./mp.w"

typedef struct mp_node_data*mp_token_node;

/*:242*//*258:*/
#line 5700 "./mp.w"

typedef struct mp_value_node_data{
NODE_BODY;
mp_value_data data;
mp_number subscript_;
mp_sym hashloc_;
mp_node parent_;
mp_node attr_head_;
mp_node subscr_head_;
}mp_value_node_data;

/*:258*//*269:*/
#line 5984 "./mp.w"

typedef struct mp_pair_node_data{
NODE_BODY;
mp_node x_part_;
mp_node y_part_;
}mp_pair_node_data;
typedef struct mp_pair_node_data*mp_pair_node;

/*:269*//*274:*/
#line 6063 "./mp.w"

typedef struct mp_transform_node_data{
NODE_BODY;
mp_node tx_part_;
mp_node ty_part_;
mp_node xx_part_;
mp_node yx_part_;
mp_node xy_part_;
mp_node yy_part_;
}mp_transform_node_data;
typedef struct mp_transform_node_data*mp_transform_node;

/*:274*//*277:*/
#line 6129 "./mp.w"

typedef struct mp_color_node_data{
NODE_BODY;
mp_node red_part_;
mp_node green_part_;
mp_node blue_part_;
}mp_color_node_data;
typedef struct mp_color_node_data*mp_color_node;

/*:277*//*280:*/
#line 6179 "./mp.w"

typedef struct mp_cmykcolor_node_data{
NODE_BODY;
mp_node cyan_part_;
mp_node magenta_part_;
mp_node yellow_part_;
mp_node black_part_;
}mp_cmykcolor_node_data;
typedef struct mp_cmykcolor_node_data*mp_cmykcolor_node;

/*:280*//*462:*/
#line 11019 "./mp.w"

#define mp_fraction mp_number
#define mp_angle mp_number
#define new_number(A) (((math_data *)(mp->math))->allocate)(mp, &(A), mp_scaled_type)
#define new_fraction(A) (((math_data *)(mp->math))->allocate)(mp, &(A), mp_fraction_type)
#define new_angle(A) (((math_data *)(mp->math))->allocate)(mp, &(A), mp_angle_type)
#define free_number(A) (((math_data *)(mp->math))->free)(mp, &(A))

/*:462*//*465:*/
#line 11148 "./mp.w"

typedef struct mp_fill_node_data{
NODE_BODY;
halfword color_model_;
mp_number red;
mp_number green;
mp_number blue;
mp_number black;
mp_string pre_script_;
mp_string post_script_;
mp_knot path_p_;
mp_knot pen_p_;
unsigned char ljoin;
mp_number miterlim;
}mp_fill_node_data;
typedef struct mp_fill_node_data*mp_fill_node;

/*:465*//*469:*/
#line 11229 "./mp.w"

typedef struct mp_stroked_node_data{
NODE_BODY;
halfword color_model_;
mp_number red;
mp_number green;
mp_number blue;
mp_number black;
mp_string pre_script_;
mp_string post_script_;
mp_knot path_p_;
mp_knot pen_p_;
unsigned char ljoin;
mp_number miterlim;
unsigned char lcap;
mp_node dash_p_;
mp_number dash_scale;
}mp_stroked_node_data;
typedef struct mp_stroked_node_data*mp_stroked_node;


/*:469*//*476:*/
#line 11429 "./mp.w"

typedef struct mp_text_node_data{
NODE_BODY;
halfword color_model_;
mp_number red;
mp_number green;
mp_number blue;
mp_number black;
mp_string pre_script_;
mp_string post_script_;
mp_string text_p_;
halfword font_n_;
mp_number width;
mp_number height;
mp_number depth;
mp_number tx;
mp_number ty;
mp_number txx;
mp_number txy;
mp_number tyx;
mp_number tyy;
}mp_text_node_data;
typedef struct mp_text_node_data*mp_text_node;

/*:476*//*480:*/
#line 11530 "./mp.w"

typedef struct mp_start_clip_node_data{
NODE_BODY;
mp_knot path_p_;
}mp_start_clip_node_data;
typedef struct mp_start_clip_node_data*mp_start_clip_node;
typedef struct mp_start_bounds_node_data{
NODE_BODY;
mp_knot path_p_;
}mp_start_bounds_node_data;
typedef struct mp_start_bounds_node_data*mp_start_bounds_node;
typedef struct mp_stop_clip_node_data{
NODE_BODY;
}mp_stop_clip_node_data;
typedef struct mp_stop_clip_node_data*mp_stop_clip_node;
typedef struct mp_stop_bounds_node_data{
NODE_BODY;
}mp_stop_bounds_node_data;
typedef struct mp_stop_bounds_node_data*mp_stop_bounds_node;


/*:480*//*484:*/
#line 11639 "./mp.w"

typedef struct mp_dash_node_data{
NODE_BODY;
mp_number start_x;
mp_number stop_x;
mp_number dash_y;
mp_node dash_info_;
}mp_dash_node_data;

/*:484*//*489:*/
#line 11695 "./mp.w"

typedef struct mp_edge_header_node_data{
NODE_BODY;
mp_number start_x;
mp_number stop_x;
mp_number dash_y;
mp_node dash_info_;
mp_number minx;
mp_number miny;
mp_number maxx;
mp_number maxy;
mp_node bblast_;
int bbtype;
mp_node list_;
mp_node obj_tail_;
halfword ref_count_;
}mp_edge_header_node_data;
typedef struct mp_edge_header_node_data*mp_edge_header_node;

/*:489*//*817:*/
#line 20679 "./mp.w"

typedef struct mp_if_node_data{
NODE_BODY;
int if_line_field_;
}mp_if_node_data;
typedef struct mp_if_node_data*mp_if_node;

/*:817*/
#line 137 "./mp.w"

#endif

/*:4*/
