/* texfileio.h
   
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

/* $Id$ */

#ifndef TEXFILEIO_H
#  define TEXFILEIO_H

extern packed_ASCII_code *nameoffile;
extern int namelength; /* this many characters are actually  relevant in |nameoffile| */
extern alpha_file name_file_pointer;

extern integer *input_file_callback_id;
extern integer read_file_callback_id[17];

extern boolean lua_a_open_in (alpha_file f, quarterword n);
extern boolean lua_a_open_out(alpha_file f, quarterword n);
extern boolean lua_b_open_out(alpha_file f);
extern void lua_a_close_in(alpha_file f, quarterword n);
extern void lua_a_close_out(alpha_file f);

extern packed_ASCII_code *buffer;
extern integer first;
extern integer last;
extern integer max_buf_stack;

extern boolean lua_input_ln(alpha_file f,quarterword n, boolean bypass_eoln);

/*
The user's terminal acts essentially like other files of text, except
that it is used both for input and for output. When the terminal is
considered an input file, the file variable is called |term_in|, and when it
is considered an output file the file variable is |term_out|.
@^system dependencies@>
*/

#define term_in stdin /* the terminal as an input file */
#define term_out stdout /* the terminal as an output file */


/*
Here is how to open the terminal files.  |t_open_out| does nothing.
|t_open_in|, on the other hand, does the work of ``rescanning,'' or getting
any command line arguments the user has provided.  It's defined in C.
*/

#define t_open_out() /*  output already open for text output */

/*
Sometimes it is necessary to synchronize the input/output mixture that
happens on the user's terminal, and three system-dependent
procedures are used for this
purpose. The first of these, |update_terminal|, is called when we want
to make sure that everything we have output to the terminal so far has
actually left the computer's internal buffers and been sent.
The second, |clear_terminal|, is called when we wish to cancel any
input that the user may have typed ahead (since we are about to
issue an unexpected error message). The third, |wake_up_terminal|,
is supposed to revive the terminal if the user has disabled it by
some instruction to the operating system.  The following macros show how
these operations can be specified with {\mc UNIX}.  |update_terminal|
does an |fflush|. |clear_terminal| is redefined
to do nothing, since the user should control the terminal.
@^system dependencies@>
*/

#define update_terminal() fflush (term_out)
#define clear_terminal() do { ; } while (0)
#define wake_up_terminal() do { ; } while (0) /* cancel the user's cancellation of output */

extern boolean init_terminal (void);
extern void term_input (void);

#endif
