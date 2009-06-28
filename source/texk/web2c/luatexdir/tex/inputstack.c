/* inputstack.c

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

#include "luatex-api.h"
#include <ptexlib.h>
#include "commands.h"

static const char _svn_version[] =
    "$Id$ $URL$";


#define end_line_char int_par(param_end_line_char_code)
#define error_context_lines int_par(param_error_context_lines_code)


in_state_record *input_stack = NULL;
integer input_ptr = 0;          /* first unused location of |input_stack| */
integer max_in_stack = 0;       /* largest value of |input_ptr| when pushing */
in_state_record cur_input;      /* the ``top'' input state */


integer in_open = 0;            /* the number of lines in the buffer, less one */
integer open_parens = 0;        /* the number of open text files */
alpha_file *input_file = NULL;
integer line = 0;               /* current line number in the current source file */
integer *line_stack = NULL;
str_number *source_filename_stack = NULL;
str_number *full_source_filename_stack = NULL;


integer scanner_status = 0;     /* can a subfile end now? */
pointer warning_index = null;   /* identifier relevant to non-|normal| scanner status */
pointer def_ref = null;         /* reference count of token list being defined */


/*
Here is a procedure that uses |scanner_status| to print a warning message
when a subfile has ended, and at certain other crucial times:
*/

void runaway(void)
{
    pointer p = null;           /* head of runaway list */
    if (scanner_status > skipping) {
        switch (scanner_status) {
        case defining:
            tprint_nl("Runaway definition");
            p = def_ref;
            break;
        case matching:
            tprint_nl("Runaway argument");
            p = temp_token_head;
            break;
        case aligning:
            tprint_nl("Runaway preamble");
            p = hold_token_head;
            break;
        case absorbing:
            tprint_nl("Runaway text");
            p = def_ref;
            break;
        default:               /* there are no other cases */
            break;
        }
        print_char('?');
        print_ln();
        show_token_list(token_link(p), null, error_line - 10);
    }
}

/*
The |param_stack| is an auxiliary array used to hold pointers to the token
lists for parameters at the current level and subsidiary levels of input.
This stack is maintained with convention (2), and it grows at a different
rate from the others.
*/

pointer *param_stack = NULL;    /* token list pointers for parameters */
integer param_ptr = 0;          /* first unused entry in |param_stack| */
integer max_param_stack = 0;    /* largest value of |param_ptr|, will be |<=param_size+9| */

/*
The input routines must also interact with the processing of
\.{\\halign} and \.{\\valign}, since the appearance of tab marks and
\.{\\cr} in certain places is supposed to trigger the beginning of special
\<v_j> template text in the scanner. This magic is accomplished by an
|align_state| variable that is increased by~1 when a `\.{\char'173}' is
scanned and decreased by~1 when a `\.{\char'175}' is scanned. The |align_state|
is nonzero during the \<u_j> template, after which it is set to zero; the
\<v_j> template begins when a tab mark or \.{\\cr} occurs at a time that
|align_state=0|.
*/

integer align_state = 0;        /* group level with respect to current alignment */

/*
Thus, the ``current input state'' can be very complicated indeed; there
can be many levels and each level can arise in a variety of ways. The
|show_context| procedure, which is used by \TeX's error-reporting routine to
print out the current input state on all levels down to the most recent
line of characters from an input file, illustrates most of these conventions.
The global variable |base_ptr| contains the lowest level that was
displayed by this procedure.
*/

integer base_ptr = 0;           /* shallowest level shown by |show_context| */

/*
The status at each level is indicated by printing two lines, where the first
line indicates what was read so far and the second line shows what remains
to be read. The context is cropped, if necessary, so that the first line
contains at most |half_error_line| characters, and the second contains
at most |error_line|. Non-current input levels whose |token_type| is
`|backed_up|' are shown only if they have not been fully read.
*/

void print_token_list_type(int t)
{
    switch (t) {
    case parameter:
        tprint_nl("<argument> ");
        break;
    case u_template:
    case v_template:
        tprint_nl("<template> ");
        break;
    case backed_up:
        if (iloc == null)
            tprint_nl("<recently read> ");
        else
            tprint_nl("<to be read again> ");
        break;
    case inserted:
        tprint_nl("<inserted text> ");
        break;
    case macro:
        print_ln();
        print_cs(iname);
        break;
    case output_text:
        tprint_nl("<output> ");
        break;
    case every_par_text:
        tprint_nl("<everypar> ");
        break;
    case every_math_text:
        tprint_nl("<everymath> ");
        break;
    case every_display_text:
        tprint_nl("<everydisplay> ");
        break;
    case every_hbox_text:
        tprint_nl("<everyhbox> ");
        break;
    case every_vbox_text:
        tprint_nl("<everyvbox> ");
        break;
    case every_job_text:
        tprint_nl("<everyjob> ");
        break;
    case every_cr_text:
        tprint_nl("<everycr> ");
        break;
    case mark_text:
        tprint_nl("<mark> ");
        break;
    case every_eof_text:
        tprint_nl("<everyeof> ");
        break;
    case write_text:
        tprint_nl("<write> ");
        break;
    default:
        tprint_nl("?");
        break;                  /* this should never happen */
    }
}

/*
Here it is necessary to explain a little trick. We don't want to store a long
string that corresponds to a token list, because that string might take up
lots of memory; and we are printing during a time when an error message is
being given, so we dare not do anything that might overflow one of \TeX's
tables. So `pseudoprinting' is the answer: We enter a mode of printing
that stores characters into a buffer of length |error_line|, where character
$k+1$ is placed into \hbox{|trick_buf[k mod error_line]|} if
|k<trick_count|, otherwise character |k| is dropped. Initially we set
|tally:=0| and |trick_count:=1000000|; then when we reach the
point where transition from line 1 to line 2 should occur, we
set |first_count:=tally| and |trick_count:=@tmax@>(error_line,
tally+1+error_line-half_error_line)|. At the end of the
pseudoprinting, the values of |first_count|, |tally|, and
|trick_count| give us all the information we need to print the two lines,
and all of the necessary text is in |trick_buf|.

Namely, let |l| be the length of the descriptive information that appears
on the first line. The length of the context information gathered for that
line is |k=first_count|, and the length of the context information
gathered for line~2 is $m=\min(|tally|, |trick_count|)-k$. If |l+k<=h|,
where |h=half_error_line|, we print |trick_buf[0..k-1]| after the
descriptive information on line~1, and set |n:=l+k|; here |n| is the
length of line~1. If $l+k>h$, some cropping is necessary, so we set |n:=h|
and print `\.{...}' followed by
$$\hbox{|trick_buf[(l+k-h+3)..k-1]|,}$$
where subscripts of |trick_buf| are circular modulo |error_line|. The
second line consists of |n|~spaces followed by |trick_buf[k..(k+m-1)]|,
unless |n+m>error_line|; in the latter case, further cropping is done.
This is easier to program than to explain.
*/

/*
The following code sets up the print routines so that they will gather
the desired information.
*/

void set_trick_count(void)
{
    first_count = tally;
    trick_count = tally + 1 + error_line - half_error_line;
    if (trick_count < error_line)
        trick_count = error_line;
}

#define begin_pseudoprint() do {		\
    l=tally; tally=0; selector=pseudo;		\
    trick_count=1000000;			\
  } while (0)

#define PSEUDO_PRINT_THE_LINE()	do {					\
    begin_pseudoprint();						\
    if (buffer[ilimit]==end_line_char) j=ilimit;			\
    else j=ilimit+1; /* determine the effective end of the line */	\
    if (j>0) {								\
      for (i=istart;i<=j-1;i++) {					\
	if (i==iloc) set_trick_count();					\
	print_char(buffer[i]);						\
      }									\
    }									\
  } while (0)


void show_context(void)
{                               /* prints where the scanner is */
    integer old_setting;        /* saved |selector| setting */
    integer nn;                 /* number of contexts shown so far, less one */
    boolean bottom_line;        /* have we reached the final context to be shown? */
    integer i;                  /* index into |buffer| */
    integer j;                  /* end of current line in |buffer| */
    integer l;                  /* length of descriptive information on line 1 */
    integer m;                  /* context information gathered for line 2 */
    integer n;                  /* length of line 1 */
    integer p;                  /* starting or ending place in |trick_buf| */
    integer q;                  /* temporary index */

    base_ptr = input_ptr;
    input_stack[base_ptr] = cur_input;
    /* store current state */
    nn = -1;
    bottom_line = false;
    while (true) {
        cur_input = input_stack[base_ptr];      /* enter into the context */
        if (istate != token_list) {
            if ((iname > 21) || (base_ptr == 0))
                bottom_line = true;
        }
        if ((base_ptr == input_ptr) || bottom_line
            || (nn < error_context_lines)) {
            /* Display the current context */
            if ((base_ptr == input_ptr) || (istate != token_list) ||
                (token_type != backed_up) || (iloc != null)) {
                /* we omit backed-up token lists that have already been read */
                tally = 0;      /* get ready to count characters */
                old_setting = selector;
                if (current_ocp_lstack > 0) {
                    tprint_nl("OCP stack ");
                    print_scaled(current_ocp_lstack);
                    tprint(" entry ");
                    print_int(current_ocp_no);
                    tprint(":");
                    PSEUDO_PRINT_THE_LINE();
                } else if (istate != token_list) {
                    /* Print location of current line */
                    /*
                       This routine should be changed, if necessary, to give the best possible
                       indication of where the current line resides in the input file.
                       For example, on some systems it is best to print both a page and line number.
                       @^system dependencies@>
                     */
                    if (iname <= 17) {
                        if (terminal_input) {
                            if (base_ptr == 0)
                                tprint_nl("<*>");
                            else
                                tprint_nl("<insert> ");
                        } else {
                            tprint_nl("<read ");
                            if (iname == 17)
                                print_char('*');
                            else
                                print_int(iname - 1);
                            print_char('>');
                        };
                    } else if (iindex != in_open) {     /* input from a pseudo file */
                        tprint_nl("l.");
                        print_int(line_stack[iindex + 1]);
                    } else {
                        tprint_nl("l.");
                        print_int(line);
                    }
                    print_char(' ');
                    PSEUDO_PRINT_THE_LINE();
                } else {
                    print_token_list_type(token_type);

                    begin_pseudoprint();
                    if (token_type < macro)
                        show_token_list(istart, iloc, 100000);
                    else
                        show_token_list(token_link(istart), iloc, 100000);    /* avoid reference count */
                }
                selector = old_setting; /* stop pseudoprinting */
                /* Print two lines using the tricky pseudoprinted information */
                if (trick_count == 1000000)
                    set_trick_count();
                /* |set_trick_count| must be performed */
                if (tally < trick_count)
                    m = tally - first_count;
                else
                    m = trick_count - first_count;      /* context on line 2 */
                if (l + first_count <= half_error_line) {
                    p = 0;
                    n = l + first_count;
                } else {
                    tprint("...");
                    p = l + first_count - half_error_line + 3;
                    n = half_error_line;
                }
                for (q = p; q <= first_count - 1; q++)
                    print_char(trick_buf[(q % error_line)]);
                print_ln();
                for (q = 1; q <= n; q++)
                    print_char(' ');    /* print |n| spaces to begin line~2 */
                if (m + n <= error_line)
                    p = first_count + m;
                else
                    p = first_count + (error_line - n - 3);
                for (q = first_count; q <= p - 1; q++)
                    print_char(trick_buf[(q % error_line)]);
                if (m + n > error_line)
                    tprint("...");

                incr(nn);
            }
        } else if (nn == error_context_lines) {
            tprint_nl("...");
            incr(nn);           /* omitted if |error_context_lines<0| */
        }
        if (bottom_line)
            break;
        decr(base_ptr);
    }
    cur_input = input_stack[input_ptr]; /* restore original state */
}

/*
The following subroutines change the input status in commonly needed ways.

First comes |push_input|, which stores the current state and creates a
new level (having, initially, the same properties as the old).
*/

/* enter a new input level, save the old */

void push_input(void)
{
    if (input_ptr > max_in_stack) {
        max_in_stack = input_ptr;
        if (input_ptr == stack_size)
            overflow("input stack size", stack_size);
    }
    input_stack[input_ptr] = cur_input; /* stack the record */
    nofilter = false;
    incr(input_ptr);
}
