%  luatex.ch
%  
%  Copyright 2006-2008 Taco Hoekwater <taco@luatex.org>
%
%  This file is part of LuaTeX.
%
%  LuaTeX is free software; you can redistribute it and/or modify it under
%  the terms of the GNU General Public License as published by the Free
%  Software Foundation; either version 2 of the License, or (at your
%  option) any later version.
%
%  LuaTeX is distributed in the hope that it will be useful, but WITHOUT
%  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
%  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
%  License for more details.
%
%  You should have received a copy of the GNU General Public License along
%  with LuaTeX; if not, see <http://www.gnu.org/licenses/>.
%
% $Id$
%
@x
  \def\?##1]{\hbox to 1in{\hfil##1.\ }}
  }
@y
  \def\?##1]{\hbox{Changes to \hbox to 1em{\hfil##1}.\ }}
  }
\let\maybe=\iftrue
@z

@x
Actually the heading shown here is not quite normal: The |program| line
does not mention any |output| file, because \ph\ would ask the \TeX\ user
to specify a file name if |output| were specified here.
@:PASCAL H}{\ph@>
@^system dependencies@>
@y
@z

@x
program TEX; {all file names are defined dynamically}
label @<Labels in the outer block@>@/
@y
program TEX; {all file names are defined dynamically}
@z

@x
@ Three labels must be declared in the main program, so we give them
symbolic names.

@d start_of_TEX=1 {go here when \TeX's variables are initialized}
@d end_of_TEX=9998 {go here to close files and terminate gracefully}
@d final_end=9999 {this label marks the ending of the program}

@<Labels in the out...@>=
start_of_TEX@t\hskip-2pt@>, end_of_TEX@t\hskip-2pt@>,@,final_end;
  {key control points}
@y
@ For Web2c, labels are not declared in the main program, but
we still have to declare the symbolic names.

@d start_of_TEX=1 {go here when \TeX's variables are initialized}
@d final_end=9999 {this label marks the ending of the program}
@z

@x
@d debug==@{ {change this to `$\\{debug}\equiv\null$' when debugging}
@d gubed==@t@>@} {change this to `$\\{gubed}\equiv\null$' when debugging}
@y
@d debug== ifdef('TEXMF_DEBUG')
@d gubed== endif('TEXMF_DEBUG')
@z

@x
@d stat==@{ {change this to `$\\{stat}\equiv\null$' when gathering
  usage statistics}
@d tats==@t@>@} {change this to `$\\{tats}\equiv\null$' when gathering
  usage statistics}
@y
@d stat==ifdef('STAT')
@d tats==endif('STAT')
@z

@x
the codewords `$|init|\ldots|tini|$'.

@d init== {change this to `$\\{init}\equiv\.{@@\{}$' in the production version}
@d tini== {change this to `$\\{tini}\equiv\.{@@\}}$' in the production version}
@y
the codewords `$|init|\ldots|tini|$' for declarations and by the codewords
`$|Init|\ldots|Tini|$' for executable code.  This distinction is helpful for
implementations where a run-time switch differentiates between the two
versions of the program.

@d init==ifdef('INITEX')
@d tini==endif('INITEX')
@d Init==init if ini_version then begin
@d Tini==end;@+tini
@f Init==begin
@f Tini==end
@z

@x
@!init @<Initialize table entries (done by \.{INITEX} only)@>@;@+tini
@y
@!Init @<Initialize table entries (done by \.{INITEX} only)@>@;@+Tini
@z

@x
@<Constants...@>=
@!buf_size=500; {maximum number of characters simultaneously present in
  current lines of open files and in control sequences between
  \.{\\csname} and \.{\\endcsname}; must not exceed |max_halfword|}
@!error_line=72; {width of context lines on terminal error messages}
@!half_error_line=42; {width of first lines of contexts in terminal
  error messages; should be between 30 and |error_line-15|}
@!max_print_line=79; {width of longest text lines output; should be at least 60}
@!stack_size=200; {maximum number of simultaneous input sources}
@!max_in_open=6; {maximum number of input files and error insertions that
  can be going on simultaneously}
@!param_size=60; {maximum number of simultaneous macro parameters}
@!nest_size=40; {maximum number of semantic levels simultaneously active}
@!max_strings=3000; {maximum number of strings; must not exceed |max_halfword|}
@!string_vacancies=8000; {the minimum number of characters that should be
  available for the user's control sequences and font names,
  after \TeX's own error messages are stored}
@!pool_size=32000; {maximum number of characters in strings, including all
  error messages and help texts, and the names of all fonts and
  control sequences; must exceed |string_vacancies| by the total
  length of \TeX's own strings, which is currently about 23000}
@!save_size=600; {space for saving values outside of current group; must be
  at most |max_halfword|}
@!dvi_buf_size=800; {size of the output buffer; must be a multiple of 8}
@!expand_depth=10000; {limits recursive calls of the |expand| procedure}
@!file_name_size=40; {file names shouldn't be longer than this}
@!pool_name='TeXformats:TEX.POOL                     ';
  {string of length |file_name_size|; tells where the string pool appears}
@y
@d file_name_size == max_halfword-1 { has to be big enough to force namelength into integer }
@d ssup_max_strings == 262143
{Larger values than 65536 cause the arrays consume much more memory.}

@<Constants...@>=
@!hash_offset=0; {smallest index in hash array, i.e., |hash_base| }
  {Use |hash_offset=0| for compilers which cannot decrement pointers.}
@!engine_name='luatex'; {the name of this engine}
@#

@!inf_main_memory = 2000000;
@!sup_main_memory = 32000000;

@!inf_max_strings = 100000;
@!sup_max_strings = ssup_max_strings;
@!inf_strings_free = 100;
@!sup_strings_free = sup_max_strings;

@!inf_buf_size = 500;
@!sup_buf_size = 100000000;

@!inf_nest_size = 40;
@!sup_nest_size = 4000;

@!inf_max_in_open = 6;
@!sup_max_in_open = 127;

@!inf_param_size = 60;
@!sup_param_size = 6000;

@!inf_save_size = 600;
@!sup_save_size = 80000;

@!inf_stack_size = 200;
@!sup_stack_size = 30000;

@!inf_dvi_buf_size = 800;
@!sup_dvi_buf_size = 65536;

@!inf_pool_size = 128000;
@!sup_pool_size = 40000000;
@!inf_pool_free = 1000;
@!sup_pool_free = sup_pool_size;
@!inf_string_vacancies = 8000;
@!sup_string_vacancies = sup_pool_size - 23000;

@!sup_hash_extra = sup_max_strings;
@!inf_hash_extra = 0;

@!sup_ocp_list_size  = 1000000;
@!inf_ocp_list_size  = 1000;
@!sup_ocp_buf_size   = 100000000;
@!inf_ocp_buf_size   = 1000;
@!sup_ocp_stack_size = 1000000;
@!inf_ocp_stack_size = 1000;

@!inf_expand_depth = 100;
@!sup_expand_depth = 1000000;
@z

@x
@d incr(#) == #:=#+1 {increase a variable by unity}
@d decr(#) == #:=#-1 {decrease a variable by unity}
@y
@z

@x
@d text_char == char {the data type of characters in text files}
@y
@d text_char == ASCII_code {the data type of characters in text files}
@z

@x
|remainder|, holds the remainder after a division.

@<Glob...@>=
@y
|remainder|, holds the remainder after a division.

@d remainder==tex_remainder

@<Glob...@>=
@z

@x
@!glue_ratio=real; {one-word representation of a glue expansion factor}
@y
@z

@x
macros are simplified in the obvious way when |min_quarterword=0|.
@^inner loop@>@^system dependencies@>

@d qi(#)==#+min_quarterword
  {to put an |eight_bits| item into a quarterword}
@d qo(#)==#-min_quarterword
  {to take an |eight_bits| item out of a quarterword}
@d hi(#)==#+min_halfword
  {to put a sixteen-bit item into a halfword}
@d ho(#)==#-min_halfword
  {to take a sixteen-bit item from a halfword}
@y
macros are simplified in the obvious way when |min_quarterword=0|.
So they have been simplified here in the obvious way.
@^inner loop@>@^system dependencies@>

@d qi(#)==# {to put an |eight_bits| item into a quarterword}
@d qo(#)==# {to take an |eight_bits| item from a quarterword}
@d hi(#)==# {to put a sixteen-bit item into a halfword}
@d ho(#)==# {to take a sixteen-bit item from a halfword}
@z

@x
@!quarterword = min_quarterword..max_quarterword; {1/4 of a word}
@!halfword=min_halfword..max_halfword; {1/2 of a word}
@!two_choices = 1..2; {used when there are two variants in a record}
@!four_choices = 1..4; {used when there are four variants in a record}
@!two_halves = packed record@;@/
  @!rh:halfword;
  case two_choices of
  1: (@!lh:halfword);
  2: (@!b0:quarterword; @!b1:quarterword);
  end;
@!four_quarters = packed record@;@/
  @!b0:quarterword;
  @!b1:quarterword;
  @!b2:quarterword;
  @!b3:quarterword;
  end;
@!memory_word = record@;@/
  case four_choices of
  1: (@!int:integer);
  2: (@!gr:glue_ratio);
  3: (@!hh:two_halves);
  4: (@!qqqq:four_quarters);
  end;
@y
@!quarterword = min_quarterword..max_quarterword;
@!halfword = min_halfword..max_halfword;
@!two_choices = 1..2; {used when there are two variants in a record}
@!four_choices = 1..4; {used when there are four variants in a record}
@=#include "texmfmem.h";@>
@z

@x
begin if m>0 then
  case m div (max_command+1) of
  0:print("vertical");
  1:print("horizontal");
  2:print("display math");
  end
else if m=0 then print("no")
else  case (-m) div (max_command+1) of
  0:print("internal vertical");
  1:print("restricted horizontal");
  2:print("math");
  end;
print(" mode");
end;
@y
begin if m>0 then
  case m div (max_command+1) of
  0:print("vertical mode");
  1:print("horizontal mode");
  2:print("display math mode");
  end
else if m=0 then print("no mode")
else  case (-m) div (max_command+1) of
  0:print("internal vertical mode");
  1:print("restricted horizontal mode");
  2:print("math mode");
  end;
end;

procedure print_in_mode(@!m:integer); {prints the mode represented by |m|}
begin if m>0 then
  case m div (max_command+1) of
  0:print("' in vertical mode");
  1:print("' in horizontal mode");
  2:print("' in display math mode");
  end
else if m=0 then print("' in no mode")
else  case (-m) div (max_command+1) of
  0:print("' in internal vertical mode");
  1:print("' in restricted horizontal mode");
  2:print("' in math mode");
  end;
end;
@z

@x
@!nest:array[0..nest_size] of list_state_record;
@y
@!nest:^list_state_record;
@z

@x
@<Start a new current page@>;
@y
@/{The following piece of code is a copy of module 991:}
page_contents:=empty; page_tail:=page_head; {|link(page_head):=null;|}@/
last_glue:=max_halfword; last_penalty:=0; last_kern:=0;
page_depth:=0; page_max_depth:=0;
@z

@x
    print_int(nest[p].pg_field); print(" line");
    if nest[p].pg_field<>1 then print_char("s");
@y
    print_int(nest[p].pg_field);
    if nest[p].pg_field<>1 then print(" lines")
    else print(" line");
@z

@x
for k:=null_cs to undefined_control_sequence-1 do
  eqtb[k]:=eqtb[undefined_control_sequence];
@y
for k:=null_cs to eqtb_top do
  eqtb[k]:=eqtb[undefined_control_sequence];
@z

@x
@ The following procedure, which is called just before \TeX\ initializes its
input and output, establishes the initial values of the date and time.
@^system dependencies@>
Since standard \PASCAL\ cannot provide such information, something special
is needed. The program here simply specifies July 4, 1776, at noon; but
users probably want a better approximation to the truth.

@p procedure fix_date_and_time;
begin time:=12*60; {minutes since midnight}
day:=4; {fourth day of the month}
month:=7; {seventh month of the year}
year:=1776; {Anno Domini}
end;
@y
@ The following procedure, which is called just before \TeX\ initializes its
input and output, establishes the initial values of the date and time.
It calls a macro-defined |date_and_time| routine.  |date_and_time|
in turn is a C macro, which calls |get_date_and_time|, passing
it the addresses of the day, month, etc., so they can be set by the
routine.  |get_date_and_time| also sets up interrupt catching if that
is conditionally compiled in the C code.
@^system dependencies@>

@d fix_date_and_time==dateandtime(time,day,month,year)
@z

@x
else if n<glue_base then @<Show equivalent |n|, in region 1 or 2@>
@y
else if (n<glue_base) or ((n>eqtb_size)and(n<=eqtb_top)) then
  @<Show equivalent |n|, in region 1 or 2@>
@z

@x
@!eqtb:array[null_cs..eqtb_size] of memory_word;
@y
@!zeqtb:^memory_word;
@z

@x
@!hash: array[hash_base..undefined_control_sequence-1] of two_halves;
  {the hash table}
@!hash_used:pointer; {allocation pointer for |hash|}
@y
@!hash: ^two_halves; {the hash table}
@!yhash: ^two_halves; {auxiliary pointer for freeing hash}
@!hash_used:pointer; {allocation pointer for |hash|}
@!hash_extra:pointer; {|hash_extra=hash| above |eqtb_size|}
@!hash_top:pointer; {maximum of the hash array}
@!eqtb_top:pointer; {maximum of the |eqtb|}
@!hash_high:pointer; {pointer to next high hash location}
@z

@x
next(hash_base):=0; text(hash_base):=0;
for k:=hash_base+1 to undefined_control_sequence-1 do hash[k]:=hash[hash_base];
@y
@z

@x
hash_used:=frozen_control_sequence; {nothing is used}
@y
hash_used:=frozen_control_sequence; {nothing is used}
hash_high:=0;
@z

@x
@!save_stack : array[0..save_size] of memory_word;
@y
@!save_stack : ^memory_word;
@z

@x
if p<int_base then
@y
if (p<int_base)or(p>eqtb_size) then
@z

@x
if cs_token_flag+undefined_control_sequence>max_halfword then bad:=21;
@y
if cs_token_flag+eqtb_size+hash_extra>max_halfword then bad:=21;
if (hash_offset<0)or(hash_offset>hash_base) then bad:=42;
@z

@x
incr(in_open); push_input; iindex:=in_open;
@y
incr(in_open); push_input; iindex:=in_open;
source_filename_stack[iindex]:=0;full_source_filename_stack[iindex]:=0;
@z

@x
begin input_ptr:=0; max_in_stack:=0;
@y
begin input_ptr:=0; max_in_stack:=0;
source_filename_stack[0]:=0;full_source_filename_stack[0]:=0;
@z

@x
if_eof_code: begin scan_four_bit_int; b:=(read_open[cur_val]=closed);
  end;
@y
if_eof_code: begin scan_four_bit_int_or_18;
  if cur_val=18 then b:=not shellenabledp
  else b:=(read_open[cur_val]=closed);
  end;
@z

@x
@ The file names we shall deal with for illustrative purposes have the
following structure:  If the name contains `\.>' or `\.:', the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the first
remaining `\..' to the end, otherwise the file extension is null.
@y
@ The file names we shall deal with have the
following structure:  If the name contains `\./' or `\.:'
(for Amiga only), the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the last
`\..' to the end, otherwise the file extension is null.
@z

@x
@!area_delimiter:pool_pointer; {the most recent `\.>' or `\.:', if any}
@!ext_delimiter:pool_pointer; {the relevant `\..', if any}
@y
@!area_delimiter:pool_pointer; {the most recent `\./', if any}
@!ext_delimiter:pool_pointer; {the most recent `\..', if any}
@z

@x
@d TEX_area=="TeXinputs:"
@.TeXinputs@>
@d TEX_font_area=="TeXfonts:"
@.TeXfonts@>
@y
In C, the default paths are specified separately.
@z

@x
@d append_to_name(#)==begin c:=#; incr(k);
  if k<=file_name_size then nameoffile[k]:=c;
  end
@y
@d append_to_name(#)==begin c:=#; if not (c="""") then begin incr(k);
  if k<=file_name_size then nameoffile[k]:=c;
  end end
@z

@x
for j:=str_start_macro(a) to str_start_macro(a+1)-1 do append_to_name(str_pool[j]);
@y
if nameoffile then libcfree (nameoffile);
nameoffile:= xmallocarray (packed_ASCII_code, length(a)+length(n)+length(e)+1);
for j:=str_start_macro(a) to str_start_macro(a+1)-1 do append_to_name(str_pool[j]);
@z

@x
for k:=namelength+1 to file_name_size do nameoffile[k]:=' ';
@y
nameoffile[namelength+1]:=0;
@z

@x
@d format_default_length=20 {length of the |TEX_format_default| string}
@d format_area_length=11 {length of its area part}
@d format_ext_length=4 {length of its `\.{.fmt}' part}
@y
Under {\mc UNIX} we don't give the area part, instead depending
on the path searching that will happen during file opening.  Also, the
length will be set in the main program.

@d format_area_length=0 {length of its area part}
@d format_ext_length=4 {length of its `\.{.fmt}' part}
@z

@x
@!TEX_format_default:packed array[1..format_default_length] of char;

@ @<Set init...@>=
TEX_format_default:='TeXformats:plain.fmt';
@y
@!format_default_length: integer;
@!TEX_format_default: ^char;

@ We set the name of the default format file and the length of that name
in C, instead of Pascal, since we want them to depend on the name of the
program.
@z

@x
for j:=1 to n do append_to_name(TEX_format_default[j]);
@y
if nameoffile then libcfree (nameoffile);
nameoffile := xmallocarray (packed_ASCII_code, n+(b-a+1)+format_ext_length+1);
for j:=1 to n do append_to_name(TEX_format_default[j]);
@z

@x
for k:=namelength+1 to file_name_size do nameoffile[k]:=' ';
@y
nameoffile[namelength+1]:=0;
@z

@x
  pack_buffered_name(0,iloc,j-1); {try first without the system file area}
  if w_open_in(fmt_file) then goto found;
  pack_buffered_name(format_area_length,iloc,j-1);
    {now try the system format file area}
  if w_open_in(fmt_file) then goto found;
@y
  pack_buffered_name(0,iloc,j-1); {Kpathsea does everything}
  if w_open_in(fmt_file) then goto found;
@z

@x
if job_name=0 then job_name:="texput";
@.texput@>
@y
if job_name=0 then job_name:=getjobname("texput");
@.texput@>
pack_job_name('.fls');
recorder_change_filename(stringcast(nameoffile+1));
@z

@x
  prompt_file_name('input file name','.tex');
@y
  prompt_file_name('input file name','');
@z

@x
iname:=a_make_name_string(cur_file);
@y
iname:=a_make_name_string(cur_file);
source_filename_stack[in_open]:=iname;
full_source_filename_stack[in_open]:=makefullnamestring;
if iname=str_ptr-1 then {we can try to conserve string pool space now}
  begin temp_str:=search_string(iname);
  if temp_str>0 then
    begin iname:=temp_str; flush_string;
    end;
  end;
@z

@x
  begin job_name:=cur_name; open_log_file;
@y
  begin job_name:=getjobname(cur_name); open_log_file;
@z

@x
if iname=str_ptr-1 then {we can conserve string pool space now}
  begin flush_string; iname:=cur_name;
  end;
@y
@z

@x
@<Append character |cur_chr|...@>=
continue: adjust_space_factor;@/
@y
@<Append character |cur_chr|...@>=
continue: adjust_space_factor;@/
@z

@x
print("' in "); print_mode(mode);
@y
print_in_mode(mode);
@z

@x
if indented then begin
  p:=new_null_box; box_dir(p):=par_direction;
  width(p):=par_indent;@+
  tail_append(p);
@y
if indented then begin
  p:=new_null_box; box_dir(p):=par_direction;
  width(p):=par_indent;@+
  tail_append(p);
@z

@x
if (cur_cs=0)or(cur_cs>frozen_control_sequence) then
@y
if (cur_cs=0)or(cur_cs>eqtb_top)or
  ((cur_cs>frozen_control_sequence)and(cur_cs<=eqtb_size)) then
@z

@x
interaction:=cur_chr;
@y
interaction:=cur_chr;
if interaction = batch_mode
then kpsemaketexdiscarderrors := 1
else kpsemaketexdiscarderrors := 0;
@z

@x
slow_print(s); update_terminal;
@y
print(s); update_terminal;
@z

@x
begin print_err(''); slow_print(s);
@y
begin print_err(''); print(s);
@z

@x
format_ident:=" (INITEX)";
@y
if ini_version then format_ident:=" (INITEX)";
@z

@x
@!w: four_quarters; {four ASCII codes}
@y
@!format_engine: ^packed_ASCII_code;
@z

@x
@!w: four_quarters; {four ASCII codes}
@y
@!format_engine: ^packed_ASCII_code;
@z

@x
@d dump_wd(#)==begin fmt_file^:=#; put(fmt_file);@+end
@d dump_int(#)==begin fmt_file^.int:=#; put(fmt_file);@+end
@d dump_hh(#)==begin fmt_file^.hh:=#; put(fmt_file);@+end
@d dump_qqqq(#)==begin fmt_file^.qqqq:=#; put(fmt_file);@+end
@y
@z

@x
@d undump_wd(#)==begin get(fmt_file); #:=fmt_file^;@+end
@d undump_int(#)==begin get(fmt_file); #:=fmt_file^.int;@+end
@d undump_hh(#)==begin get(fmt_file); #:=fmt_file^.hh;@+end
@d undump_qqqq(#)==begin get(fmt_file); #:=fmt_file^.qqqq;@+end
@y
@z

@x
@d undump_size_end_end(#)==too_small(#)@+else undump_end_end
@y
@d format_debug_end(#)==
    writeln (stderr, ' = ', #);
  end;
@d format_debug(#)==
  if debug_format_file then begin
    write (stderr, 'fmtdebug:', #);
    format_debug_end
@d undump_size_end_end(#)==
  too_small(#)@+else format_debug (#)(x); undump_end_end
@z

@x
dump_int(@$);@/
@y
dump_int(@"57325458);  {Web2C \TeX's magic constant: "W2TX"}
{Align engine to 4 bytes with one or more trailing NUL}
x:=strlen(engine_name);
format_engine:=xmallocarray(packed_ASCII_code,x+4);
strcpy(stringcast(format_engine), stringcast(engine_name));
for k:=x to x+3 do format_engine[k]:=0;
x:=x+4-(x mod 4);
dump_int(x);dump_things(format_engine[0], x);
libcfree(format_engine);@/
dump_int(@$);@/
dump_int(max_halfword);@/
dump_int(hash_high);
@z

@x
x:=fmt_file^.int;
if x<>@$ then goto bad_fmt; {check that strings are the same}
@y
@+Init
libcfree(str_pool); libcfree(str_start);
libcfree(yhash); libcfree(zeqtb); libcfree(fixmem); libcfree(varmemcast(varmem));
@+Tini
undump_int(x);
format_debug('format magic number')(x);
if x<>@"57325458 then goto bad_fmt; {not a format file}
undump_int(x);
format_debug('engine name size')(x);
if (x<0) or (x>256) then goto bad_fmt; {corrupted format file}
format_engine:=xmallocarray(packed_ASCII_code, x);
undump_things(format_engine[0], x);
format_engine[x-1]:=0; {force string termination, just in case}
if strcmp(stringcast(engine_name), stringcast(format_engine)) then
  begin wake_up_terminal;
  wterm_cr;
  fprintf(term_out,'---! %s was written by %s',stringcast(nameoffile+1), format_engine);
  libcfree(format_engine);
  goto bad_fmt;
end;
libcfree(format_engine);
undump_int(x);
format_debug('string pool checksum')(x);
if x<>@$ then begin {check that strings are the same}
  wake_up_terminal;
  wterm_cr;
  fprintf(term_out,'---! %s was written by an older version',stringcast(nameoffile+1));
  goto bad_fmt;
end;
undump_int(x);
if x<>max_halfword then goto bad_fmt; {check |max_halfword|}
undump_int(hash_high);
  if (hash_high<0)or(hash_high>sup_hash_extra) then goto bad_fmt;
  if hash_extra<hash_high then hash_extra:=hash_high;
  eqtb_top:=eqtb_size+hash_extra;
  if hash_extra=0 then hash_top:=undefined_control_sequence else
        hash_top:=eqtb_top;
  yhash:=xmallocarray(two_halves,1+hash_top-hash_offset);
  hash:=yhash - hash_offset;
  next(hash_base):=0; text(hash_base):=0;
  for x:=hash_base+1 to hash_top do hash[x]:=hash[hash_base];
  zeqtb:=xmallocarray (memory_word,eqtb_top+1);
  eqtb:=zeqtb;

  eq_type(undefined_control_sequence):=undefined_cs;
  equiv(undefined_control_sequence):=null;
  eq_level(undefined_control_sequence):=level_zero;
  for x:=eqtb_size+1 to eqtb_top do
    eqtb[x]:=eqtb[undefined_control_sequence];
@z

@x
dump_int(str_ptr);
for k:=string_offset to str_ptr do dump_int(str_start_macro(k));
k:=0;
while k+4<pool_ptr do
  begin dump_four_ASCII; k:=k+4;
  end;
k:=pool_ptr-4; dump_four_ASCII;
@y
dump_int((str_ptr-string_offset));
dump_things(str_start[0], (str_ptr-string_offset)+1);
dump_things(str_pool[0], pool_ptr);
@z

@x
undump_size(0)(pool_size)('string pool size')(pool_ptr);
undump_size(0)(max_strings)('max strings')(str_ptr);
for k:=string_offset to str_ptr do undump(0)(pool_ptr)(str_start_macro(k));
k:=0;
while k+4<pool_ptr do
  begin undump_four_ASCII; k:=k+4;
  end;
k:=pool_ptr-4; undump_four_ASCII;
@y
undump_size(0)(sup_pool_size-pool_free)('string pool size')(pool_ptr);
if pool_size<pool_ptr+pool_free then
  pool_size:=pool_ptr+pool_free;
undump_size(0)(sup_max_strings-strings_free)('sup strings')(str_ptr);@/
if max_strings<str_ptr+strings_free then
  max_strings:=str_ptr+strings_free;
str_start:=xmallocarray(pool_pointer, max_strings);
str_ptr:=str_ptr + string_offset;
undump_checked_things(0, pool_ptr, str_start[0], (str_ptr-string_offset)+1);@/
str_pool:=xmallocarray(packed_ASCII_code, pool_size);
undump_things(str_pool[0], pool_ptr);
@z

@x
for k:=fix_mem_min to fix_mem_end do dump_wd(mem(k));
@y
dump_things(mem(fix_mem_min), fix_mem_end-fix_mem_min+1);
@z

@x
for k:=fix_mem_min to fix_mem_end do undump_wd(mem(k));
@y
undump_things (mem(fix_mem_min), fix_mem_end-fix_mem_min+1);
@z

@x
undump(hash_base)(frozen_control_sequence)(par_loc);
par_token:=cs_token_flag+par_loc;@/
undump(hash_base)(frozen_control_sequence)(write_loc);@/
@y
undump(hash_base)(hash_top)(par_loc);
par_token:=cs_token_flag+par_loc;@/
undump(hash_base)(hash_top)(write_loc);@/
@z

@x
while k<l do
  begin dump_wd(eqtb[k]); incr(k);
  end;
@y
dump_things(eqtb[k], l-k);
@z

@x
while k<l do
  begin dump_wd(eqtb[k]); incr(k);
  end;
@y
dump_things(eqtb[k], l-k);
@z

@x
k:=j+1; dump_int(k-l);
until k>eqtb_size
@y
k:=j+1; dump_int(k-l);
until k>eqtb_size;
if hash_high>0 then dump_things(eqtb[eqtb_size+1],hash_high);
  {dump |hash_extra| part}
@z

@x
for j:=k to k+x-1 do undump_wd(eqtb[j]);
@y
undump_things(eqtb[k], x);
@z

@x
until k>eqtb_size
@y
until k>eqtb_size;
if hash_high>0 then undump_things(eqtb[eqtb_size+1],hash_high);
  {undump |hash_extra| part}
@z

@x
dump_int(hash_used); cs_count:=frozen_control_sequence-1-hash_used;
@y
dump_int(hash_used); cs_count:=frozen_control_sequence-1-hash_used+hash_high;
@z

@x
for p:=hash_used+1 to undefined_control_sequence-1 do dump_hh(hash[p]);
@y
dump_things(hash[hash_used+1], undefined_control_sequence-1-hash_used);
if hash_high>0 then dump_things(hash[eqtb_size+1], hash_high);
@z

@x
for p:=hash_used+1 to undefined_control_sequence-1 do undump_hh(hash[p]);
@y
undump_things (hash[hash_used+1], undefined_control_sequence-1-hash_used);
if debug_format_file then begin
  print_csnames (hash_base, undefined_control_sequence - 1);
end;
if hash_high > 0 then begin
  undump_things (hash[eqtb_size+1], hash_high);
  if debug_format_file then begin
    print_csnames (eqtb_size + 1, hash_high - (eqtb_size + 1));
  end;
end;
@z

@x
@ @<Undump the array info for internal font number |k|@>=
begin undump_font(k);@/
end
@y
@ @<Undump the array info for internal font number |k|@>=
begin undump_font(k);@/
end;
make_pdftex_banner
@z

@x
undump(batch_mode)(error_stop_mode)(interaction);
@y
undump(batch_mode)(error_stop_mode)(interaction);
if interactionoption<>unspecified_mode then interaction:=interactionoption;
@z

@x
if (x<>69069)or eof(fmt_file) then goto bad_fmt
@y
if x<>69069 then goto bad_fmt
@z

@x
print(" (preloaded format="); print(job_name); print_char(" ");
@y
print(" (format="); print(job_name); print_char(" ");
@z

@x
@p begin @!{|start_here|}
@y
@d const_chk(#)==begin if # < inf@&_@&# then # := inf@&_@&# else
                         if # > sup_@&# then # := sup_@&# end

{|setup_bound_var| stuff duplicated in \.{mf.ch}.}
@d setup_bound_var(#)==bound_default:=#; setup_bound_var_end
@d setup_bound_var_end(#)==bound_name:=#; setup_bound_var_end_end
@d setup_bound_var_end_end(#)==if luainit>0 then begin
       get_lua_number('texconfig',bound_name,addressof(#));
       if #=0 then #:=bound_default;
    end
  else
    setupboundvariable(addressof(#), bound_name, bound_default);

@p procedure main_body;
begin @!{|start_here|}

{Bounds that may be set from the configuration file. We want the user to
 be able to specify the names with underscores, but \.{TANGLE} removes
 underscores, so we're stuck giving the names twice, once as a string,
 once as the identifier. How ugly.}

  setup_bound_var (100000)('pool_size')(pool_size);
  setup_bound_var (75000)('string_vacancies')(string_vacancies);
  setup_bound_var (5000)('pool_free')(pool_free); {min pool avail after fmt}
  setup_bound_var (15000)('max_strings')(max_strings);
  setup_bound_var (100)('strings_free')(strings_free);
  setup_bound_var (3000)('buf_size')(buf_size);
  setup_bound_var (50)('nest_size')(nest_size);
  setup_bound_var (15)('max_in_open')(max_in_open);
  setup_bound_var (60)('param_size')(param_size);
  setup_bound_var (4000)('save_size')(save_size);
  setup_bound_var (300)('stack_size')(stack_size);
  setup_bound_var (16384)('dvi_buf_size')(dvi_buf_size);
  setup_bound_var (79)('error_line')(error_line);
  setup_bound_var (50)('half_error_line')(half_error_line);
  setup_bound_var (79)('max_print_line')(max_print_line);
  setup_bound_var(1000)('ocp_list_size')(ocp_list_size);
  setup_bound_var(1000)('ocp_buf_size')(ocp_buf_size);
  setup_bound_var(1000)('ocp_stack_size')(ocp_stack_size);
  setup_bound_var (0)('hash_extra')(hash_extra);
  setup_bound_var (72)('pk_dpi')(pk_dpi);
  setup_bound_var (10000)('expand_depth')(expand_depth);

  {Check other constants against their sup and inf.}
  const_chk (buf_size);
  const_chk (nest_size);
  const_chk (max_in_open);
  const_chk (param_size);
  const_chk (save_size);
  const_chk (stack_size);
  const_chk (dvi_buf_size);
  const_chk (pool_size);
  const_chk (string_vacancies);
  const_chk (pool_free);
  const_chk (max_strings);
  const_chk (strings_free);
  const_chk (hash_extra);
  const_chk (obj_tab_size);
  const_chk (pk_dpi);
  if error_line > ssup_error_line then error_line := ssup_error_line;

  {array memory allocation}
  buffer:=xmallocarray (packed_ASCII_code, buf_size);
  nest:=xmallocarray (list_state_record, nest_size);
  save_stack:=xmallocarray (memory_word, save_size);
  input_stack:=xmallocarray (in_state_record, stack_size);
  input_file:=xmallocarray (alpha_file, max_in_open);
  input_file_callback_id:=xmallocarray (integer, max_in_open);
  line_stack:=xmallocarray (integer, max_in_open);
  eof_seen:=xmallocarray (boolean, max_in_open);
  grp_stack:=xmallocarray (save_pointer, max_in_open);
  if_stack:=xmallocarray (pointer, max_in_open);
  source_filename_stack:=xmallocarray (str_number, max_in_open);
  full_source_filename_stack:=xmallocarray (str_number, max_in_open);
  param_stack:=xmallocarray (halfword, param_size);
  dvi_buf:=xmallocarray (eight_bits, dvi_buf_size);
  initialize_ocplist_arrays(ocp_list_size);
  initialize_ocp_buffers(ocp_buf_size, ocp_stack_size);
  obj_tab:=xmallocarray (obj_entry, inf_obj_tab_size); {will grow dynamically}
  set_obj_offset(0,0);
  init_dest_names;
@+Init
  fixmem:=xmallocarray (smemory_word, fix_mem_init+1);
  memset (voidcast(fixmem), 0, (fix_mem_init+1)*sizeof(smemory_word));
  fix_mem_min:=0;
  fix_mem_max:=fix_mem_init;

  eqtb_top := eqtb_size+hash_extra;
  if hash_extra=0 then hash_top:=undefined_control_sequence else
        hash_top:=eqtb_top;
  yhash:=xmallocarray (two_halves,1+hash_top-hash_offset);
  hash:=yhash - hash_offset;   {Some compilers require |hash_offset=0|}
  next(hash_base):=0; text(hash_base):=0;
  for hash_used:=hash_base+1 to hash_top do hash[hash_used]:=hash[hash_base];
  zeqtb:=xmallocarray (memory_word, eqtb_top);
  eqtb:=zeqtb;

  str_start:=xmallocarray (pool_pointer, max_strings);
  str_pool:=xmallocarray (packed_ASCII_code, pool_size);
@+Tini
@z

@x
@!init if not get_strings_started then goto final_end;
init_prim; {call |primitive| for each primitive}
init_str_ptr:=str_ptr; init_pool_ptr:=pool_ptr; fix_date_and_time;
tini@/
@y
@!Init if not get_strings_started then goto final_end;
init_prim; {call |primitive| for each primitive}
init_str_ptr:=str_ptr; init_pool_ptr:=pool_ptr; fix_date_and_time;
Tini@/
@z

@x
end_of_TEX: close_files_and_terminate;
final_end: ready_already:=0;
end.
@y
close_files_and_terminate;
final_end: do_final_end;
end {|main_body|};
@z

@x
     slow_print(texmf_log_name); print_char(".");
@y
     print_file_name(0, texmf_log_name, 0); print_char(".");
@z

@x
  begin @!init for i:=0 to biggest_used_mark do begin
@y
  begin @!Init for i:=0 to biggest_used_mark do begin
@z

@x
  store_fmt_file; return;@+tini@/
@y
  store_fmt_file; return;@+Tini@/
@z

@x
if (format_ident=0)or(buffer[iloc]="&") then
@y
if (format_ident=0)or(buffer[iloc]="&")or dump_line then
@z

@x
  w_close(fmt_file);
@y
  w_close(fmt_file);
  eqtb:=zeqtb;
@z

@x
fix_date_and_time;@/
@y
fix_date_and_time;@/
@!init
make_pdftex_banner;
tini@/
@z

@x
    begin goto breakpoint;@\ {go to every label at least once}
    breakpoint: m:=0; @{'BREAKPOINT'@}@\
    end
@y
    abort {do something to cause a core dump}
@z

@x
  else if cur_val>15 then cur_val:=16;
@y
  else if (cur_val>15) and (cur_val <> 18) then cur_val:=16;
@z

@x
@!eof_seen : array[1..max_in_open] of boolean; {has eof been seen?}
@y
@!eof_seen : ^boolean; {has eof been seen?}
@z

@x
@!grp_stack : array[0..max_in_open] of save_pointer; {initial |cur_boundary|}
@!if_stack : array[0..max_in_open] of pointer; {initial |cond_ptr|}
@y
@!grp_stack : ^save_pointer; {initial |cur_boundary|}
@!if_stack : ^pointer; {initial |cond_ptr|}
@z

@x
@* \[54] System-dependent changes.
@y
@* \[54/web2c] System-dependent changes for Web2c.
Here are extra variables for Web2c.  (This numbering of the
system-dependent section allows easy integration of Web2c and e-\TeX, etc.)
@^<system dependencies@>

@<Glob...@>=
@!edit_name_start: pool_pointer; {where the filename to switch to starts}
@!edit_name_length,@!edit_line: integer; {what line to start editing at}
@!ipcon: cinttype; {level of IPC action, 0 for none [default]}
@!stop_at_space: boolean; {whether |more_name| returns false for space}

@ The |edit_name_start| will be set to point into |str_pool| somewhere after
its beginning if \TeX\ is supposed to switch to an editor on exit.

@<Set init...@>=
edit_name_start:=0;
stop_at_space:=true;

@ These are used when we regenerate the representation of the first 256
strings.

@<Global...@>=
@!save_str_ptr: str_number;
@!save_pool_ptr: pool_pointer;
@!shellenabledp: cinttype;
@!restrictedshell: cinttype;
@!output_comment: ^char;
@!k,l: 0..255; {used by `Make the first 256 strings', etc.}

@ When debugging a macro package, it can be useful to see the exact
control sequence names in the format file.  For example, if ten new
csnames appear, it's nice to know what they are, to help pinpoint where
they came from.  (This isn't a truly ``basic'' printing procedure, but
that's a convenient module in which to put it.)

@<Basic printing procedures@> =
procedure print_csnames (hstart:integer; hfinish:integer);
var c,h:integer;
begin
  writeln(stderr, 'fmtdebug:csnames from ', hstart, ' to ', hfinish, ':');
  for h := hstart to hfinish do begin
    if text(h) > 0 then begin {if have anything at this position}
      for c := str_start_macro(text(h)) to str_start_macro(text(h) + 1) - 1
      do begin
        putbyte(str_pool[c], stderr); {print the characters}
      end;
      writeln(stderr, '|');
    end;
  end;
end;

@ Are we printing extra info as we read the format file?

@<Glob...@>=
@!debug_format_file: boolean;


@ A helper for printing file:line:error style messages.  Look for a
filename in |full_source_filename_stack|, and if we fail to find
one fall back on the non-file:line:error style.

@<Basic print...@>=
procedure print_file_line;
var level: 0..max_in_open;
begin
  level:=in_open;
  while (level>0) and (full_source_filename_stack[level]=0) do
    decr(level);
  if level=0 then
    print_nl("! ")
  else begin
    print_nl (""); print (full_source_filename_stack[level]); print (":");
    if level=in_open then print_int (line)
    else print_int (line_stack[iindex+1-(in_open-level)]);
    print (": ");
  end;
end;

@ To be able to determine whether \.{\\write18} is enabled from within
\TeX\ we also implement \.{\\eof18}.  We sort of cheat by having an
additional route |scan_four_bit_int_or_18| which is the same as
|scan_four_bit_int| except it also accepts the value 18.

@<Declare procedures that scan restricted classes of integers@>=
procedure scan_four_bit_int_or_18;
begin scan_int;
if (cur_val<0)or((cur_val>15)and(cur_val<>18)) then
  begin print_err('Bad number');
@.Bad number@>
  help2('Since I expected to read a number between 0 and 15,',
    'I changed this one to zero.'); int_error(cur_val); cur_val:=0;
  end;
end;

@* \[54] System-dependent changes.
@z


@x
@* \[55] Index.
@y

@ This function used to be in pdftex, but is useful in tex too.

@p function get_nullstr: str_number;
begin
    get_nullstr := "";
end;

@* \[55] Index.
@z

