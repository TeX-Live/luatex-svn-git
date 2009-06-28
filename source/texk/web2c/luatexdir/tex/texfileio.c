/* texfileio.c
   
   Copyright 2009 Taco Hoekwater <taco@luatex.org>

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

#include <ptexlib.h>

static const char _svn_version[] =
    "$Id$"
    "$URL$";

/*
The bane of portability is the fact that different operating systems treat
input and output quite differently, perhaps because computer scientists
have not given sufficient attention to this problem. People have felt somehow
that input and output are not part of ``real'' programming. Well, it is true
that some kinds of programming are more fun than others. With existing
input/output conventions being so diverse and so messy, the only sources of
joy in such parts of the code are the rare occasions when one can find a
way to make the program a little less bad than it might have been. We have
two choices, either to attack I/O now and get it over with, or to postpone
I/O until near the end. Neither prospect is very attractive, so let's
get it over with.

The basic operations we need to do are (1)~inputting and outputting of
text, to or from a file or the user's terminal; (2)~inputting and
outputting of eight-bit bytes, to or from a file; (3)~instructing the
operating system to initiate (``open'') or to terminate (``close'') input or
output from a specified file; (4)~testing whether the end of an input
file has been reached.

\TeX\ needs to deal with two kinds of files.
We shall use the term |alpha_file| for a file that contains textual data,
and the term |byte_file| for a file that contains eight-bit binary information.
These two types turn out to be the same on many computers, but
sometimes there is a significant distinction, so we shall be careful to
distinguish between them. Standard protocols for transferring
such files from computer to computer, via high-speed networks, are
now becoming available to more and more communities of users.

The program actually makes use also of a third kind of file, called a
|word_file|, when dumping and reloading base information for its own
initialization.  We shall define a word file later; but it will be possible
for us to specify simple operations on word files before they are defined.
*/

/*
Most of what we need to do with respect to input and output can be handled
by the I/O facilities that are standard in \PASCAL, i.e., the routines
called |get|, |put|, |eof|, and so on. But
standard \PASCAL\ does not allow file variables to be associated with file
names that are determined at run time, so it cannot be used to implement
\TeX; some sort of extension to \PASCAL's ordinary |reset| and |rewrite|
is crucial for our purposes. We shall assume that |nameoffile| is a variable
of an appropriate type such that the \PASCAL\ run-time system being used to
implement \TeX\ can open a file whose external name is specified by
|nameoffile|.
@^system dependencies@>
*/

packed_ASCII_code *nameoffile;
int namelength; /* this many characters are actually  relevant in |nameoffile| */
alpha_file name_file_pointer;

/*
When input files are opened via a callback, they will also be read using
callbacks. for that purpose, the |open_read_file_callback| returns an
integer to uniquely identify a callback table. This id replaces the file
point |f| in this case, because the input does not have to be a file
in the traditional sense.

Signalling this fact is achieved by having two arrays of integers.
*/

integer *input_file_callback_id;
integer read_file_callback_id[17];

static void fixup_nameoffile (str_number fnam)
{
    integer k;
    xfree (nameoffile);
    namelength = str_length(fnam);
    nameoffile = xmallocarray (packed_ASCII_code, namelength+2);
    for (k=str_start_macro(fnam);k<=str_start_macro(fnam+1)-1;k++)
	nameoffile[k-str_start_macro(fnam)+1] = str_pool[k];
    nameoffile[namelength+1]=0;
    flush_string();
}

boolean lua_a_open_in (alpha_file f, quarterword n)
{
    str_number fnam; /* string returned by find callback */
    integer callback_id;
    boolean ret=true; /* return value */
    boolean file_ok=true; /* the status so far  */
    if (n==0) {
	texinputtype = 1; /* Tell |open_input| we are \.{\\input}. */
	input_file_callback_id[iindex] = 0;
    } else {
	texinputtype=0;
	read_file_callback_id[n] = 0;
    }
    callback_id=callback_defined(find_read_file_callback);
    if (callback_id>0) {
	fnam=0;
	file_ok=run_callback(callback_id,"dS->s",n,(char *)(nameoffile+1),&fnam);
	if ((file_ok)&&(fnam!=0)&&(str_length(fnam)>0)) {
	    /* Fixup |nameoffile| after callback */
	    fixup_nameoffile(fnam);
	} else {
	    file_ok=false; /* file not found */
	}
    }
    if (file_ok) {
	callback_id=callback_defined(open_read_file_callback);
	if (callback_id>0) {
	    k = run_and_save_callback(callback_id,"S->",(char *)(nameoffile+1));
	    if (k>0) {
		ret = true;
		if (n==0)
		    input_file_callback_id[iindex] = k;
		else
		    read_file_callback_id[n] = k;
	    } else {
		file_ok=false; /* read failed */
	    }
	} else  { /* no read callback */
	    if (openinnameok((char *)(nameoffile+1))) {
		ret = a_open_in(f,kpsetexformat);
		name_file_pointer = f;
	    } else {
		file_ok=false; /* open failed */
	    }
	}
    }
    if (!file_ok) {
	name_file_pointer = 0;
	ret = false;
    }
    return ret;
}

boolean lua_a_open_out(alpha_file f, quarterword n)
{
    boolean test;
    str_number fnam;
    integer callback_id;
    boolean ret = false;
    name_file_pointer = 0;
    callback_id=callback_defined(find_write_file_callback);
    if (callback_id>0) {
	fnam=0;
	test=run_callback(callback_id,"dS->s",n,(char *)(nameoffile+1),&fnam);
	if ((test)&&(fnam!=0)&&(str_length(fnam)>0)) {
	    /* Fixup |nameoffile| after callback */
	    fixup_nameoffile(fnam);
	    ret = do_a_open_out(f);
	    name_file_pointer = f;
	}
    } else {
        if (openoutnameok((char *)(nameoffile+1))) {
	    ret = a_open_out(f);
	    name_file_pointer = f;
	}
    }
    return ret;
}

boolean lua_b_open_out(alpha_file f)
{
    boolean test;
    str_number fnam;
    integer callback_id;
    boolean ret = false;
    name_file_pointer = 0;
    callback_id=callback_defined(find_output_file_callback);
    if (callback_id>0) {
	fnam=0;
	test=run_callback(callback_id,"S->s",(char *)(nameoffile+1),&fnam);
	if ((test)&&(fnam!=0)&&(str_length(fnam)>0)) {
	    /* Fixup |nameoffile| after callback */
	    fixup_nameoffile(fnam);
	    ret = do_b_open_out(f);
	    name_file_pointer = f;
	}
    } else {
        if (openoutnameok((char *)(nameoffile+1))) {
	    ret = b_open_out(f);
	    name_file_pointer = f;
	}
    }
    return ret;
}

void lua_a_close_in(alpha_file f, quarterword n) /* close a text file */
{
    boolean ret;
    integer callback_id;
    if (n==0) 
	callback_id=input_file_callback_id[iindex];
    else
	callback_id=read_file_callback_id[n];
    if (callback_id>0) {
	ret=run_saved_callback(callback_id,"close","->");
	destroy_saved_callback(callback_id);
	if (n==0)
	    input_file_callback_id[iindex] = 0;
	else
	    read_file_callback_id[n] = 0;
    } else {
	a_close(f);
    }
}

void lua_a_close_out(alpha_file f) /* close a text file */
{
    a_close(f);
}

/*
Binary input and output are done with \PASCAL's ordinary |get| and |put|
procedures, so we don't have to make any other special arrangements for
binary~I/O. Text output is also easy to do with standard \PASCAL\ routines.
The treatment of text input is more difficult, however, because
of the necessary translation to |ASCII_code| values.
\TeX's conventions should be efficient, and they should
blend nicely with the user's operating environment.

@ Input from text files is read one line at a time, using a routine called
|lua_input_ln|. This function is defined in terms of global variables called
|buffer|, |first|, and |last| that will be described in detail later; for
now, it suffices for us to know that |buffer| is an array of |ASCII_code|
values, and that |first| and |last| are indices into this array
representing the beginning and ending of a line of text.
*/

packed_ASCII_code *buffer; /* lines of characters being read */
integer first; /* the first unused position in |buffer| */
integer last; /* end of the line just input to |buffer| */
integer max_buf_stack; /* largest index used in |buffer| */

/*
The |lua_input_ln| function brings the next line of input from the specified
file into available positions of the buffer array and returns the value
|true|, unless the file has already been entirely read, in which case it
returns |false| and sets |last:=first|.  In general, the |ASCII_code|
numbers that represent the next line of the file are input into
|buffer[first]|, |buffer[first+1]|, \dots, |buffer[last-1]|; and the
global variable |last| is set equal to |first| plus the length of the
line. Trailing blanks are removed from the line; thus, either |last=first|
(in which case the line was entirely blank) or |buffer[last-1]<>" "|.

An overflow error is given, however, if the normal actions of |lua_input_ln|
would make |last>=buf_size|; this is done so that other parts of \TeX\
can safely look at the contents of |buffer[last+1]| without overstepping
the bounds of the |buffer| array. Upon entry to |lua_input_ln|, the condition
|first<buf_size| will always hold, so that there is always room for an
``empty'' line.

The variable |max_buf_stack|, which is used to keep track of how large
the |buf_size| parameter must be to accommodate the present job, is
also kept up to date by |lua_input_ln|.

If the |bypass_eoln| parameter is |true|, |lua_input_ln| will do a |get|
before looking at the first character of the line; this skips over
an |eoln| that was in |f^|. The procedure does not do a |get| when it
reaches the end of the line; therefore it can be used to acquire input
from the user's terminal as well as from ordinary text files.

Standard \PASCAL\ says that a file should have |eoln| immediately
before |eof|, but \TeX\ needs only a weaker restriction: If |eof|
occurs in the middle of a line, the system function |eoln| should return
a |true| result (even though |f^| will be undefined).

Since the inner loop of |lua_input_ln| is part of \TeX's ``inner loop''---each
character of input comes in at this place---it is wise to reduce system
overhead by making use of special routines that read in an entire array
of characters at once, if such routines are available. The following
code uses standard \PASCAL\ to illustrate what needs to be done, but
finer tuning is often possible at well-developed \PASCAL\ sites.
@^inner loop@>
*/

boolean lua_input_ln(alpha_file f,quarterword n, boolean bypass_eoln)
{
    boolean lua_result;
    integer last_ptr;
    integer callback_id;
    (void)bypass_eoln; /* todo: variable can be removed */
    if (n==0)
	callback_id=input_file_callback_id[iindex];
    else
	callback_id=read_file_callback_id[n];
    if (callback_id>0) {
	last=first;
        last_ptr = first;
	lua_result = run_saved_callback(callback_id,"reader","->l", &last_ptr);
        if ((lua_result==true)&&(last_ptr!=0)) {
	    last = last_ptr;
	    if (last>max_buf_stack) max_buf_stack=last;
	} else {
	    lua_result = false;
	}
    }  else {
	lua_result = input_ln(f,bypass_eoln);
    }
    if (lua_result==true) {
	/* Fix up the input buffer using callbacks */
	if (last>=first) {
	    callback_id = callback_defined(process_input_buffer_callback);
	    if (callback_id>0) {
		last_ptr = first;
		lua_result = run_callback(callback_id, "l->l", (last-first), &last_ptr);
		if ((lua_result==true)&&(last_ptr!=0)) {
		    last = last_ptr;
		    if (last>max_buf_stack) max_buf_stack=last;
		}
	    }
	}
	return true;
    }
    return false;
}

/*
We need a special routine to read the first line of \TeX\ input from
the user's terminal. This line is different because it is read before we
have opened the transcript file; there is sort of a ``chicken and
egg'' problem here. If the user types `\.{\\input paper}' on the first
line, or if some macro invoked by that line does such an \.{\\input},
the transcript file will be named `\.{paper.log}'; but if no \.{\\input}
commands are performed during the first line of terminal input, the transcript
file will acquire its default name `\.{texput.log}'. (The transcript file
will not contain error messages generated by the first line before the
first \.{\\input} command.)
@.texput@>

The first line is even more special if we are lucky enough to have an operating
system that treats \TeX\ differently from a run-of-the-mill \PASCAL\ object
program. It's nice to let the user start running a \TeX\ job by typing
a command line like `\.{tex paper}'; in such a case, \TeX\ will operate
as if the first line of input were `\.{paper}', i.e., the first line will
consist of the remainder of the command line, after the part that invoked
\TeX.

The first line is special also because it may be read before \TeX\ has
input a format file. In such cases, normal error messages cannot yet
be given. The following code uses concepts that will be explained later.
(If the \PASCAL\ compiler does not support non-local |@!goto|\unskip, the
@^system dependencies@>
statement `|goto final_end|' should be replaced by something that
quietly terminates the program.)
*/

/*
Different systems have different ways to get started. But regardless of
what conventions are adopted, the routine that initializes the terminal
should satisfy the following specifications:

\yskip\textindent{1)}It should open file |term_in| for input from the
  terminal. (The file |term_out| will already be open for output to the
  terminal.)

\textindent{2)}If the user has given a command line, this line should be
  considered the first line of terminal input. Otherwise the
  user should be prompted with `\.{**}', and the first line of input
  should be whatever is typed in response.

\textindent{3)}The first line of input, which might or might not be a
  command line, should appear in locations |first| to |last-1| of the
  |buffer| array.

\textindent{4)}The global variable |loc| should be set so that the
  character to be read next by \TeX\ is in |buffer[loc]|. This
  character should not be blank, and we should have |loc<last|.

\yskip\noindent(It may be necessary to prompt the user several times
before a non-blank line comes in. The prompt is `\.{**}' instead of the
later `\.*' because the meaning is slightly different: `\.{\\input}' need
not be typed immediately after~`\.{**}'.)

@ The following program does the required initialization.
Iff anything has been specified on the command line, then |t_open_in|
will return with |last > first|.
@^system dependencies@>
*/


boolean init_terminal (void) /* gets the terminal input started */
{
    t_open_in();
    if (last > first) {
	iloc = first;
	while ((iloc < last) && (buffer[iloc]==' ')) 
	    incr(iloc);
	if (iloc < last) {
	    return true; 
	}
    }
    while (1) {
	wake_up_terminal(); 
	fputs("**",term_out); 
	update_terminal();
	if (!input_ln(term_in,true)) {
	  /* this shouldn't happen */
	  fputs("! End of file on the terminal... why?\n", term_out);
	  return false;
	}
	iloc=first;
	while ((iloc<last)&&(buffer[iloc]==' '))
	  incr(iloc);
	if (iloc<last) {
	  return true; /* return unless the line was all blank */
	}
	fputs("Please type the name of your input file.\n", term_out);
    }
}



/*
Here is a procedure that asks the user to type a line of input,
assuming that the |selector| setting is either |term_only| or |term_and_log|.
The input is placed into locations |first| through |last-1| of the
|buffer| array, and echoed on the transcript file if appropriate.
*/

void term_input (void) /* gets a line from the terminal */
{
    integer k; /* index into |buffer| */
    update_terminal(); /* now the user sees the prompt for sure */
    if (!input_ln(term_in,true)) 
	fatal_error("End of file on the terminal!");
    term_offset=0; /* the user's line ended with \<\rm return> */
    decr(selector); /* prepare to echo the input */
    if (last!=first) {
	for (k=first;k<=last-1;k++) 
	    print_char(buffer[k]);
    }
    print_ln(); 
    incr(selector); /* restore previous status */
}

