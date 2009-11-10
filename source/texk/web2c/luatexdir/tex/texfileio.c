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

#define end_line_char int_par(end_line_char_code)

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
int namelength;                 /* this many characters are actually  relevant in |nameoffile| */
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

static void fixup_nameoffile(str_number fnam)
{
    integer k;
    xfree(nameoffile);
    namelength = str_length(fnam);
    nameoffile = xmallocarray(packed_ASCII_code, namelength + 2);
    for (k = str_start_macro(fnam); k <= str_start_macro(fnam + 1) - 1; k++)
        nameoffile[k - str_start_macro(fnam) + 1] = str_pool[k];
    nameoffile[namelength + 1] = 0;
    flush_string();
}


char *luatex_find_file (char *s, int callback_index)
{
    char *ftemp = NULL;
    int callback_id = callback_defined(callback_index);
    if (callback_id > 0) {
        (void)run_callback (callback_id, "S->S", s, &ftemp);
    } else {
        /* use kpathsea here */
        switch (callback_index) {
        case find_enc_file_callback:
            ftemp = kpse_find_file(s, kpse_enc_format, 0);
            break;
        case find_sfd_file_callback:
            ftemp = kpse_find_file(s, kpse_sfd_format, 0);
            break;
        case find_map_file_callback:
            ftemp = kpse_find_file(s, kpse_fontmap_format, 0);
            break;
        case find_type1_file_callback:
            ftemp = kpse_find_file(s, kpse_type1_format, 0);
            break;
        case find_font_file_callback:
            ftemp = kpse_find_file(s, kpse_ofm_format, 0);
            if (ftemp == NULL)
                ftemp = kpse_find_file(s, kpse_tfm_format, 0);
            break;
        case find_vf_file_callback:
            ftemp = kpse_find_file(s, kpse_ovf_format, 0);
            if (ftemp == NULL)
                ftemp = kpse_find_file(s, kpse_vf_format, 0);
            break;
        default:
            break;
        }
    }
    return ftemp;
}


/* Open an input file F, using the kpathsea format FILEFMT and passing
   FOPEN_MODE to fopen.  The filename is in `nameoffile+1'.  We return
   whether or not the open succeeded.  If it did, `nameoffile' is set to
   the full filename opened, and `namelength' to its length.  */

extern string fullnameoffile;
extern int ocptemp;

boolean
luatex_open_input (FILE **f_ptr, char *fn, int filefmt, const_string fopen_mode)
{
    string fname = NULL;
    /* We havent found anything yet. */
    *f_ptr = NULL;
    if (fullnameoffile)
        free(fullnameoffile);
    fullnameoffile = NULL;
    /* Handle -output-directory.
       FIXME: We assume that it is OK to look here first.  Possibly it
       would be better to replace lookups in "." with lookups in the
       output_directory followed by "." but to do this requires much more
       invasive surgery in libkpathsea.  */
    /* FIXME: This code assumes that the filename of the input file is
       not an absolute filename. */
    if (output_directory) {
        fname = concat3(output_directory, DIR_SEP_STRING, fn);
        *f_ptr = fopen(fname, fopen_mode);
        if (*f_ptr) {
            fullnameoffile = fname;
        } else {
            free(fname);
        }
    }
    /* No file means do the normal search. */
    if (*f_ptr == NULL) {
        /* The only exception to `must_exist' being true is \openin, for
           which we set `tex_input_type' to 0 in the change file.  */
        /* According to the pdfTeX people, pounding the disk for .vf files
           is overkill as well.  A more general solution would be nice. */
        boolean must_exist = (filefmt != kpse_tex_format || texinputtype)
            && (filefmt != kpse_vf_format);
        fname = kpse_find_file (fn, (kpse_file_format_type)filefmt, must_exist);
        if (fname) {
            fullnameoffile = xstrdup(fname);
            /* If we found the file in the current directory, don't leave
               the `./' at the beginning of `nameoffile', since it looks
               dumb when `tex foo' says `(./foo.tex ... )'.  On the other
               hand, if the user said `tex ./foo', and that's what we
               opened, then keep it -- the user specified it, so we
               shouldn't remove it.  */
            if (fname[0] == '.' && IS_DIR_SEP (fname[1])
                && (fn[0] != '.' || !IS_DIR_SEP (nameoffile[1])))
            {
                unsigned i = 0;
                while (fname[i + 2] != 0) {
                    fname[i] = fname[i + 2];
                    i++;
                }
                fname[i] = 0;
            }
            /* This fopen is not allowed to fail. */
            *f_ptr = xfopen (fn, fopen_mode);
        }
    }

    if (*f_ptr) {
        recorder_record_input (fn);
        if (filefmt == kpse_tfm_format || 
            filefmt == kpse_ofm_format ) {
            tfmtemp = getc (*f_ptr);
        } else if (filefmt == kpse_ocp_format) {
            ocptemp = getc (*f_ptr);
        }
    }            
    return *f_ptr != NULL;
}


boolean lua_a_open_in(alpha_file f, quarterword n)
{
    integer k;
    str_number fnam;            /* string returned by find callback */
    integer callback_id;
    boolean ret = true;         /* return value */
    boolean file_ok = true;     /* the status so far  */
    if (n == 0) {
        texinputtype = 1;       /* Tell |open_input| we are \.{\\input}. */
        input_file_callback_id[iindex] = 0;
    } else {
        texinputtype = 0;
        read_file_callback_id[n] = 0;
    }
    callback_id = callback_defined(find_read_file_callback);
    if (callback_id > 0) {
        fnam = 0;
        file_ok =
            run_callback(callback_id, "dS->s", n, (char *) (nameoffile + 1),
                         &fnam);
        if ((file_ok) && (fnam != 0) && (str_length(fnam) > 0)) {
            /* Fixup |nameoffile| after callback */
            fixup_nameoffile(fnam);
        } else {
            file_ok = false;    /* file not found */
        }
    }
    if (file_ok) {
        callback_id = callback_defined(open_read_file_callback);
        if (callback_id > 0) {
            k = run_and_save_callback(callback_id, "S->",
                                      (char *) (nameoffile + 1));
            if (k > 0) {
                ret = true;
                if (n == 0)
                    input_file_callback_id[iindex] = k;
                else
                    read_file_callback_id[n] = k;
            } else {
                file_ok = false;        /* read failed */
            }
        } else {                /* no read callback */
            if (openinnameok((char *) (nameoffile + 1))) {
                ret = a_open_in(f, kpsetexformat);
                name_file_pointer = f;
            } else {
                file_ok = false;        /* open failed */
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
    callback_id = callback_defined(find_write_file_callback);
    if (callback_id > 0) {
        fnam = 0;
        test =
            run_callback(callback_id, "dS->s", n, (char *) (nameoffile + 1),
                         &fnam);
        if ((test) && (fnam != 0) && (str_length(fnam) > 0)) {
            /* Fixup |nameoffile| after callback */
            fixup_nameoffile(fnam);
            ret = do_a_open_out(f);
            name_file_pointer = f;
        }
    } else {
        if (openoutnameok((char *) (nameoffile + 1))) {
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
    callback_id = callback_defined(find_output_file_callback);
    if (callback_id > 0) {
        fnam = 0;
        test =
            run_callback(callback_id, "S->s", (char *) (nameoffile + 1), &fnam);
        if ((test) && (fnam != 0) && (str_length(fnam) > 0)) {
            /* Fixup |nameoffile| after callback */
            fixup_nameoffile(fnam);
            ret = do_b_open_out(f);
            name_file_pointer = f;
        }
    } else {
        if (openoutnameok((char *) (nameoffile + 1))) {
            ret = b_open_out(f);
            name_file_pointer = f;
        }
    }
    return ret;
}

void lua_a_close_in(alpha_file f, quarterword n)
{                               /* close a text file */
    boolean ret;
    integer callback_id;
    if (n == 0)
        callback_id = input_file_callback_id[iindex];
    else
        callback_id = read_file_callback_id[n];
    if (callback_id > 0) {
        ret = run_saved_callback(callback_id, "close", "->");
        destroy_saved_callback(callback_id);
        if (n == 0)
            input_file_callback_id[iindex] = 0;
        else
            read_file_callback_id[n] = 0;
    } else {
        a_close(f);
    }
}

void lua_a_close_out(alpha_file f)
{                               /* close a text file */
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

packed_ASCII_code *buffer;      /* lines of characters being read */
integer first;                  /* the first unused position in |buffer| */
integer last;                   /* end of the line just input to |buffer| */
integer max_buf_stack;          /* largest index used in |buffer| */

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

boolean lua_input_ln(alpha_file f, quarterword n, boolean bypass_eoln)
{
    boolean lua_result;
    integer last_ptr;
    integer callback_id;
    (void) bypass_eoln;         /* todo: variable can be removed */
    if (n == 0)
        callback_id = input_file_callback_id[iindex];
    else
        callback_id = read_file_callback_id[n];
    if (callback_id > 0) {
        last = first;
        last_ptr = first;
        lua_result =
            run_saved_callback(callback_id, "reader", "->l", &last_ptr);
        if ((lua_result == true) && (last_ptr != 0)) {
            last = last_ptr;
            if (last > max_buf_stack)
                max_buf_stack = last;
        } else {
            lua_result = false;
        }
    } else {
        lua_result = input_ln(f, bypass_eoln);
    }
    if (lua_result == true) {
        /* Fix up the input buffer using callbacks */
        if (last >= first) {
            callback_id = callback_defined(process_input_buffer_callback);
            if (callback_id > 0) {
                last_ptr = first;
                lua_result =
                    run_callback(callback_id, "l->l", (last - first),
                                 &last_ptr);
                if ((lua_result == true) && (last_ptr != 0)) {
                    last = last_ptr;
                    if (last > max_buf_stack)
                        max_buf_stack = last;
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


boolean init_terminal(void)
{                               /* gets the terminal input started */
    t_open_in();
    if (last > first) {
        iloc = first;
        while ((iloc < last) && (buffer[iloc] == ' '))
            incr(iloc);
        if (iloc < last) {
            return true;
        }
    }
    while (1) {
        wake_up_terminal();
        fputs("**", term_out);
        update_terminal();
        if (!input_ln(term_in, true)) {
            /* this shouldn't happen */
            fputs("! End of file on the terminal... why?\n", term_out);
            return false;
        }
        iloc = first;
        while ((iloc < last) && (buffer[iloc] == ' '))
            incr(iloc);
        if (iloc < last) {
            return true;        /* return unless the line was all blank */
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

void term_input(void)
{                               /* gets a line from the terminal */
    integer k;                  /* index into |buffer| */
    update_terminal();          /* now the user sees the prompt for sure */
    if (!input_ln(term_in, true))
        fatal_error("End of file on the terminal!");
    term_offset = 0;            /* the user's line ended with \<\rm return> */
    decr(selector);             /* prepare to echo the input */
    if (last != first) {
        for (k = first; k <= last - 1; k++)
            print_char(buffer[k]);
    }
    print_ln();
    incr(selector);             /* restore previous status */
}

/*
It's time now to fret about file names.  Besides the fact that different
operating systems treat files in different ways, we must cope with the
fact that completely different naming conventions are used by different
groups of people. The following programs show what is required for one
particular operating system; similar routines for other systems are not
difficult to devise.
@^fingers@>
@^system dependencies@>

\TeX\ assumes that a file name has three parts: the name proper; its
``extension''; and a ``file area'' where it is found in an external file
system.  The extension of an input file or a write file is assumed to be
`\.{.tex}' unless otherwise specified; it is `\.{.log}' on the
transcript file that records each run of \TeX; it is `\.{.tfm}' on the font
metric files that describe characters in the fonts \TeX\ uses; it is
`\.{.dvi}' on the output files that specify typesetting information; and it
is `\.{.fmt}' on the format files written by \.{INITEX} to initialize \TeX.
The file area can be arbitrary on input files, but files are usually
output to the user's current area.  If an input file cannot be
found on the specified area, \TeX\ will look for it on a special system
area; this special area is intended for commonly used input files like
\.{webmac.tex}.

Simple uses of \TeX\ refer only to file names that have no explicit
extension or area. For example, a person usually says `\.{\\input} \.{paper}'
or `\.{\\font\\tenrm} \.= \.{helvetica}' instead of `\.{\\input}
\.{paper.new}' or `\.{\\font\\tenrm} \.= \.{<csd.knuth>test}'. Simple file
names are best, because they make the \TeX\ source files portable;
whenever a file name consists entirely of letters and digits, it should be
treated in the same way by all implementations of \TeX. However, users
need the ability to refer to other files in their environment, especially
when responding to error messages concerning unopenable files; therefore
we want to let them use the syntax that appears in their favorite
operating system.

The following procedures don't allow spaces to be part of
file names; but some users seem to like names that are spaced-out.
System-dependent changes to allow such things should probably
be made with reluctance, and only when an entire file name that
includes spaces is ``quoted'' somehow.

Here are the global values that file names will be scanned into.
*/

str_number cur_name;            /* name of file just scanned */
str_number cur_area;            /* file area just scanned, or \.{""} */
str_number cur_ext;             /* file extension just scanned, or \.{""} */

/*
The file names we shall deal with have the
following structure:  If the name contains `\./' or `\.:'
(for Amiga only), the file area
consists of all characters up to and including the final such character;
otherwise the file area is null.  If the remaining file name contains
`\..', the file extension consists of all such characters from the last
`\..' to the end, otherwise the file extension is null.

We can scan such file names easily by using two global variables that keep track
of the occurrences of area and extension delimiters:
*/

pool_pointer area_delimiter;    /* the most recent `\./', if any */
pool_pointer ext_delimiter;     /* the relevant `\..', if any */

/*
Input files that can't be found in the user's area may appear in a standard
system area called |TEX_area|. Font metric files whose areas are not given
explicitly are assumed to appear in a standard system area called
|TEX_font_area|.  $\Omega$'s compiled translation process files whose areas
are not given explicitly are assumed to appear in a standard system area. 
These system area names will, of course, vary from place to place.
*/

/*
Another system-dependent routine is needed to convert three internal
\TeX\ strings
into the |nameoffile| value that is used to open files. The present code
allows both lowercase and uppercase letters in the file name.
*/

void pack_file_name(str_number n, str_number a, str_number e)
{
    integer k;                  /* number of positions filled in |nameoffile| */
    ASCII_code c;               /* character being packed */
    pool_pointer j;             /* index into |str_pool| */
    k = 0;
    if (nameoffile)
        xfree(nameoffile);
    nameoffile =
        xmallocarray(packed_ASCII_code,
                     str_length(a) + str_length(n) + str_length(e) + 1);
    for (j = str_start_macro(a); j <= str_start_macro(a + 1) - 1; j++)
        append_to_name(str_pool[j]);
    for (j = str_start_macro(n); j <= str_start_macro(n + 1) - 1; j++)
        append_to_name(str_pool[j]);
    for (j = str_start_macro(e); j <= str_start_macro(e + 1) - 1; j++)
        append_to_name(str_pool[j]);
    if (k <= file_name_size)
        namelength = k;
    else
        namelength = file_name_size;
    nameoffile[namelength + 1] = 0;
}


/*
A messier routine is also needed, since format file names must be scanned
before \TeX's string mechanism has been initialized. We shall use the
global variable |TEX_format_default| to supply the text for default system areas
and extensions related to format files.
@^system dependencies@>

Under {\mc UNIX} we don't give the area part, instead depending
on the path searching that will happen during file opening.  Also, the
length will be set in the main program.
*/

integer format_default_length;
char *TEX_format_default;

/*
We set the name of the default format file and the length of that name
in C, instead of Pascal, since we want them to depend on the name of the
program.
*/

/*
Here is the messy routine that was just mentioned. It sets |nameoffile|
from the first |n| characters of |TEX_format_default|, followed by
|buffer[a..b]|, followed by the last |format_ext_length| characters of
|TEX_format_default|.

We dare not give error messages here, since \TeX\ calls this routine before
the |error| routine is ready to roll. Instead, we simply drop excess characters,
since the error will be detected in another way when a strange file name
isn't found.
*/

void pack_buffered_name(integer n, integer a, integer b)
{
    integer k;                  /* number of positions filled in |nameoffile| */
    ASCII_code c;               /* character being packed */
    integer j;                  /* index into |buffer| or |TEX_format_default| */
    if (n + b - a + 1 + format_ext_length > file_name_size)
        b = a + file_name_size - n - 1 - format_ext_length;
    k = 0;
    if (nameoffile)
        xfree(nameoffile);
    nameoffile =
        xmallocarray(packed_ASCII_code,
                     n + (b - a + 1) + format_ext_length + 1);
    for (j = 1; j <= n; j++)
        append_to_name(TEX_format_default[j]);
    for (j = a; j <= b; j++)
        append_to_name(buffer[j]);
    for (j = format_default_length - format_ext_length + 1;
         j <= format_default_length; j++)
        append_to_name(TEX_format_default[j]);
    if (k <= file_name_size)
        namelength = k;
    else
        namelength = file_name_size;
    nameoffile[namelength + 1] = 0;
}

/*
Here is the only place we use |pack_buffered_name|. This part of the program
becomes active when a ``virgin'' \TeX\ is trying to get going, just after
the preliminary initialization, or when the user is substituting another
format file by typing `\.\&' after the initial `\.{**}' prompt.  The buffer
contains the first line of input in |buffer[loc..(last-1)]|, where
|loc<last| and |buffer[loc]<>" "|.
*/

boolean open_fmt_file(void)
{
    int j;                      /* the first space after the format file name */
    j = iloc;
    if (buffer[iloc] == '&') {
        incr(iloc);
        j = iloc;
        buffer[last] = ' ';
        while (buffer[j] != ' ')
            incr(j);
        pack_buffered_name(0, iloc, j - 1);     /* Kpathsea does everything */
        if (w_open_in(fmt_file))
            goto FOUND;
        wake_up_terminal();
        fputs("Sorry, I can't find the format `", stdout);
        fputs(stringcast(nameoffile + 1), stdout);
        fputs("'; will try `", stdout);
        fputs(TEX_format_default + 1, stdout);
        fputs("'.", stdout);
        wterm_cr();
        update_terminal();
    }
    /* now pull out all the stops: try for the system \.{plain} file */
    pack_buffered_name(format_default_length - format_ext_length, 1, 0);
    if (!w_open_in(fmt_file)) {
        wake_up_terminal();
        fputs("I can't find the format file `", stdout);
        fputs(TEX_format_default + 1, stdout);
        fputs("'!", stdout);
        wterm_cr();
        return false;
    }
  FOUND:
    iloc = j;
    return true;
}

/*
Operating systems often make it possible to determine the exact name (and
possible version number) of a file that has been opened. The following routine,
which simply makes a \TeX\ string from the value of |nameoffile|, should
ideally be changed to deduce the full name of file~|f|, which is the file
most recently opened, if it is possible to do this in a \PASCAL\ program.


This routine might be called after string memory has overflowed, hence
we dare not use `|str_room|'.
*/

/*
The global variable |name_in_progress| is used to prevent recursive
use of |scan_file_name|, since the |begin_name| and other procedures
communicate via global variables. Recursion would arise only by
devious tricks like `\.{\\input\\input f}'; such attempts at sabotage
must be thwarted. Furthermore, |name_in_progress| prevents \.{\\input}
@^recursion@>
from being initiated when a font size specification is being scanned.

Another global variable, |job_name|, contains the file name that was first
\.{\\input} by the user. This name is extended by `\.{.log}' and `\.{.dvi}'
and `\.{.fmt}' in the names of \TeX's output files.
*/


boolean name_in_progress;       /* is a file name being scanned? */
str_number job_name;            /* principal file name */
boolean log_opened;             /* has the transcript file been opened? */

/*
Initially |job_name=0|; it becomes nonzero as soon as the true name is known.
We have |job_name=0| if and only if the `\.{log}' file has not been opened,
except of course for a short time just after |job_name| has become nonzero.
*/

str_number texmf_log_name;      /* full name of the log file */

/*
The |open_log_file| routine is used to open the transcript file and to help
it catch up to what has previously been printed on the terminal.
*/

void open_log_file(void)
{
    int old_setting;            /* previous |selector| setting */
    int k;                      /* index into |buffer| */
    int l;                      /* end of first input line */
    old_setting = selector;
    if (job_name == 0)
        job_name = getjobname(maketexstring("texput")); /* TODO */
    pack_job_name(".fls");
    recorder_change_filename(stringcast(nameoffile + 1));
    pack_job_name(".log");
    while (!lua_a_open_out(log_file, 0)) {
        /* Try to get a different log file name */
        /* Sometimes |open_log_file| is called at awkward moments when \TeX\ is
           unable to print error messages or even to |show_context|.
           The |prompt_file_name| routine can result in a |fatal_error|, but the |error|
           routine will not be invoked because |log_opened| will be false.

           The normal idea of |batch_mode| is that nothing at all should be written
           on the terminal. However, in the unusual case that
           no log file could be opened, we make an exception and allow
           an explanatory message to be seen.

           Incidentally, the program always refers to the log file as a `\.{transcript
           file}', because some systems cannot use the extension `\.{.log}' for
           this file.
         */
        selector = term_only;
        prompt_file_name("transcript file name", ".log");
    }
    log_file = name_file_pointer;
    texmf_log_name = a_make_name_string(log_file);
    selector = log_only;
    log_opened = true;
    if (callback_defined(start_run_callback) == 0) {
        /* Print the banner line, including the date and time */
        log_banner(luatex_version_string, luatex_date_info);

        input_stack[input_ptr] = cur_input;     /* make sure bottom level is in memory */
        tprint_nl("**");
        l = input_stack[0].limit_field; /* last position of first line */
        if (buffer[l] == end_line_char)
            decr(l);            /* TODO: multichar endlinechar */
        for (k = 1; k <= l; k++)
            print_char(buffer[k]);
        print_ln();             /* now the transcript file contains the first line of input */
    }
    flush_loggable_info();      /* should be done always */
    selector = old_setting + 2; /* |log_only| or |term_and_log| */
}

/*
Let's turn now to the procedure that is used to initiate file reading
when an `\.{\\input}' command is being processed.
*/

void start_input(void)
{                               /* \TeX\ will \.{\\input} something */
    str_number temp_str;
    do {
        get_x_token();
    } while ((cur_cmd == spacer_cmd) || (cur_cmd == relax_cmd));

    back_input();
    if (cur_cmd != left_brace_cmd) {
        scan_file_name();           /* set |cur_name| to desired file name */
    } else {
        scan_file_name_toks();
    }
    pack_cur_name();
    while (1) {
        begin_file_reading();   /* set up |cur_file| and new level of input */
        if (lua_a_open_in(cur_file, 0))
            break;
        end_file_reading();     /* remove the level that didn't work */
        prompt_file_name("input file name", "");
    }
    cur_file = name_file_pointer;
    iname = a_make_name_string(cur_file);
    source_filename_stack[in_open] = iname;
    full_source_filename_stack[in_open] = makefullnamestring();
    if (iname == str_ptr - 1) { /* we can try to conserve string pool space now */
        temp_str = search_string(iname);
        if (temp_str > 0) {
            iname = temp_str;
            flush_string();
        }
    }
    if (job_name == 0) {
        job_name = getjobname(cur_name);
        open_log_file();
    }
    /* |open_log_file| doesn't |show_context|, so |limit|
       and |loc| needn't be set to meaningful values yet */
    if (tracefilenames) {
        if (term_offset + str_length(iname) > max_print_line - 2)
            print_ln();
        else if ((term_offset > 0) || (file_offset > 0))
            print_char(' ');
        print_char('(');
        print_file_name(0, iname, 0);
    }
    incr(open_parens);
    update_terminal();
    istate = new_line;
    /* Prepare new file {\sl Sync\TeX} information */
    synctex_start_input();      /* Give control to the {\sl Sync\TeX} controller */

    /* Read the first line of the new file */
    /* Here we have to remember to tell the |lua_input_ln| routine not to
       start with a |get|. If the file is empty, it is considered to
       contain a single blank line. */

    line = 1;
    if (lua_input_ln(cur_file, 0, false)) {
        ;
    }
    firm_up_the_line();
    if (end_line_char_inactive())
        decr(ilimit);
    else
        buffer[ilimit] = end_line_char;
    first = ilimit + 1;
    iloc = istart;
}
