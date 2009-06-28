/* printing.c
   
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
#include "luatexextra.h"        /* for BANNER */

static const char _svn_version[] =
    "$Id$"
    "$URL$";

#define wlog(A)      fputc(A,log_file)
#define wterm(A)     fputc(A,term_out)

/*
Messages that are sent to a user's terminal and to the transcript-log file
are produced by several `|print|' procedures. These procedures will
direct their output to a variety of places, based on the setting of
the global variable |selector|, which has the following possible
values:

\yskip
\hang |term_and_log|, the normal setting, prints on the terminal and on the
  transcript file.

\hang |log_only|, prints only on the transcript file.

\hang |term_only|, prints only on the terminal.

\hang |no_print|, doesn't print at all. This is used only in rare cases
  before the transcript file is open.

\hang |pseudo|, puts output into a cyclic buffer that is used
  by the |show_context| routine; when we get to that routine we shall discuss
  the reasoning behind this curious mode.

\hang |new_string|, appends the output to the current string in the
  string pool.

\hang 0 to 15, prints on one of the sixteen files for \.{\\write} output.

\yskip
\noindent The symbolic names `|term_and_log|', etc., have been assigned
numeric codes that satisfy the convenient relations |no_print+1=term_only|,
|no_print+2=log_only|, |term_only+2=log_only+1=term_and_log|.

Three additional global variables, |tally| and |term_offset| and
|file_offset|, record the number of characters that have been printed
since they were most recently cleared to zero. We use |tally| to record
the length of (possibly very long) stretches of printing; |term_offset|
and |file_offset|, on the other hand, keep track of how many characters
have appeared so far on the current line that has been output to the
terminal or to the transcript file, respectively.
*/

alpha_file log_file;            /* transcript of \TeX\ session */
int selector = term_only;       /* where to print a message */
int dig[23];                    /* digits in a number being output */
integer tally = 0;              /* the number of characters recently printed */
int term_offset = 0;            /* the number of characters on the current terminal line */
int file_offset = 0;            /* the number of characters on the current file line */
packed_ASCII_code trick_buf[(ssup_error_line + 1)];     /* circular buffer for pseudoprinting */
integer trick_count;            /* threshold for pseudoprinting, explained later */
integer first_count;            /* another variable for pseudoprinting */
boolean inhibit_par_tokens = false;     /*  for minor adjustments to |show_token_list|  */

/* To end a line of text output, we call |print_ln| */

void print_ln(void)
{                               /* prints an end-of-line */
    switch (selector) {
    case term_and_log:
        wterm_cr();
        wlog_cr();
        term_offset = 0;
        file_offset = 0;
        break;
    case log_only:
        wlog_cr();
        file_offset = 0;
        break;
    case term_only:
        wterm_cr();
        term_offset = 0;
        break;
    case no_print:
    case pseudo:
    case new_string:
        break;
    default:
        fprintf(write_file[selector], "\n");
        break;
    }
}                               /* |tally| is not affected */

/*
  The |print_char| procedure sends one byte to the desired destination.
  All printing comes through |print_ln| or |print_char|.
*/

#define wterm_char(A) do {				\
    if ((A>=0x20)||(A==0x0A)||(A==0x0D)||(A==0x09)) {	\
      wterm(A);					\
    } else {						\
      if (term_offset+2>=max_print_line) {		\
	wterm_cr(); term_offset=0;			\
      }							\
      incr(term_offset); wterm('^');			\
      incr(term_offset); wterm('^');			\
      wterm(A+64);					\
    }							\
  } while (0)

#define needs_wrapping(A,B)				\
  (((A>=0xC0)&&(A<=0xDF)&&(B+2>=max_print_line))||	\
   ((A>=0xE0)&&(A<=0xEF)&&(B+3>=max_print_line))||	\
   ((A>=0xF0)&&(B+4>=max_print_line)))

#define fix_term_offset(A)	 do {			\
    if (needs_wrapping(A,term_offset)){			\
      wterm_cr(); term_offset=0;			\
    }							\
  } while (0)

#define fix_log_offset(A)	 do {			\
    if (needs_wrapping(A,file_offset)){			\
      wlog_cr(); file_offset=0;				\
    }							\
  } while (0)

void print_char(int s)
{                               /* prints a single byte */
    assert(s >= 0 && s < 256);
    if (s == int_par(param_new_line_char_code)) {
        if (selector < pseudo) {
            print_ln();
            return;
        }
    }
    switch (selector) {
    case term_and_log:
        fix_term_offset(s);
        fix_log_offset(s);
        wterm_char(s);
        wlog(s);
        incr(term_offset);
        incr(file_offset);
        if (term_offset == max_print_line) {
            wterm_cr();
            term_offset = 0;
        }
        if (file_offset == max_print_line) {
            wlog_cr();
            file_offset = 0;
        }
        break;
    case log_only:
        fix_log_offset(s);
        wlog(s);
        incr(file_offset);
        if (file_offset == max_print_line) {
            wlog_cr();
            file_offset = 0;
        }
        break;
    case term_only:
        fix_term_offset(s);
        wterm_char(s);
        incr(term_offset);
        if (term_offset == max_print_line)
            wterm_cr();
        term_offset = 0;
        break;
    case no_print:
        break;
    case pseudo:
        if (tally < trick_count)
            trick_buf[tally % error_line] = s;
        break;
    case new_string:
        if (pool_ptr < pool_size)
            append_char(s);
        break;                  /* we drop characters if the string space is full */
    default:
        fprintf(write_file[selector], "%c", s);
    }
    incr(tally);
}

/*
An entire string is output by calling |print|. Note that if we are outputting
the single standard ASCII character \.c, we could call |print("c")|, since
|"c"=99| is the number of a single-character string, as explained above. But
|print_char("c")| is quicker, so \TeX\ goes directly to the |print_char|
routine when it knows that this is safe. (The present implementation
assumes that it is always safe to print a visible ASCII character.)
@^system dependencies@>

The first 256 entries above the 17th unicode plane are used for a
special trick: when \TeX\ has to print items in that range, it will
instead print the character that results from substracting 0x110000
from that value. This allows byte-oriented output to things like
\.{\\specials} and \.{\\pdfliterals}. Todo: Perhaps it would be useful
to do the same substraction while typesetting.
*/

void print(integer s)
{                               /* prints string |s| */
    pool_pointer j,l;             /* current character code position */
    if (s >= str_ptr) {
        /* this can't happen */
        print_char('?');
        print_char('?');
        print_char('?');
        return;
    } else if (s < STRING_OFFSET) {
        if (s < 0) {
            /* can't happen */
            print_char('?');
            print_char('?');
            print_char('?');
        } else {
            /* TH not sure about this, disabled for now! */
            if ((false) && (selector > pseudo)) {
                print_char(s);
                return;         /* internal strings are not expanded */
            }
            if (s == int_par(param_new_line_char_code)) {
                if (selector < pseudo) {
                    print_ln();
                    return;
                }
            }
            if (s <= 0x7F) {
                print_char(s);
            } else if (s <= 0x7FF) {
                print_char(0xC0 + (s / 0x40));
                print_char(0x80 + (s % 0x40));
            } else if (s <= 0xFFFF) {
                print_char(0xE0 + (s / 0x1000));
                print_char(0x80 + ((s % 0x1000) / 0x40));
                print_char(0x80 + ((s % 0x1000) % 0x40));
            } else if (s >= 0x110000) {
                int c = s - 0x110000;
                assert(c < 256);
                print_char(c);
            } else {
                print_char(0xF0 + (s / 0x40000));
                print_char(0x80 + ((s % 0x40000) / 0x1000));
                print_char(0x80 + (((s % 0x40000) % 0x1000) / 0x40));
                print_char(0x80 + (((s % 0x40000) % 0x1000) % 0x40));
            }
        }
        return;
    }
    j = str_start_macro(s);
    l = str_start_macro(s + 1);
    while (j < l) {
        /* 0x110000 in utf=8: 0xF4 0x90 0x80 0x80  */
        /* I don't bother checking the last two bytes explicitly */
        if ((j < l-4) && (str_pool[j] == 0xF4) && (str_pool[j + 1] == 0x90)) {
            int c = (str_pool[j + 2] - 128) * 64 + (str_pool[j + 3] - 128);
            assert(c >= 0 && c < 256);
            print_char(c);
            j = j + 4;
        } else {
            print_char(str_pool[j]);
            incr(j);
        }
    }
}

/*
The procedure |print_nl| is like |print|, but it makes sure that the
string appears at the beginning of a new line. 
*/

void print_nl(str_number s)
{                               /* prints string |s| at beginning of line */
    if (((term_offset > 0) && (odd(selector))) ||
        ((file_offset > 0) && (selector >= log_only)))
        print_ln();
    print(s);
}

void print_nlp(void)
{                               /* move to beginning of a line */
    if (((term_offset > 0) && (odd(selector))) ||
        ((file_offset > 0) && (selector >= log_only)))
        print_ln();
}

/* char * versions */

void tprint(char *s)
{
    unsigned char *ss = (unsigned char *) s;
    while (*ss)
        print_char(*ss++);
}

void tprint_nl(char *s)
{
    print_nlp();
    tprint(s);
}

/* |slow_print| is the same as |print| nowadays, but the name is kept for now. */

void slow_print(integer s)
{                               /* prints string |s| */
    print(s);
}

/*
Here is the very first thing that \TeX\ prints: a headline that identifies
the version number and format package. The |term_offset| variable is temporarily
incorrect, but the discrepancy is not serious since we assume that the banner
and format identifier together will occupy at most |max_print_line|
character positions.
*/

void print_banner(char *v, int e)
{
    boolean res;
    integer callback_id;
    callback_id = callback_defined(start_run_callback);
    if (callback_id == 0) {
        fprintf(term_out, "This is LuaTeX, Version %s%d", v, e);
        fprintf(term_out, versionstring);
        if (format_ident > 0)
            slow_print(format_ident);
        print_ln();
        if (shellenabledp) {
            wterm(' ');
            if (restrictedshell)
                fprintf(term_out, "restricted ");
            fprintf(term_out, "\\write18 enabled.\n");
        }
    } else if (callback_id > 0) {
        res = run_callback(callback_id, "->");
    }
}

void log_banner(char *v, int e)
{
    char *months[] = { "   ",
		       "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
		       "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };
    unsigned month = (unsigned) int_par(param_month_code);
    if (month > 12)
        month = 0;
    fprintf(log_file, "This is LuaTeX, Version %s%d", v, e);
    fprintf(log_file, versionstring);
    slow_print(format_ident);
    print_char(' ');
    print_char(' ');
    print_int(int_par(param_day_code));
    print_char(' ');
    fprintf(log_file, months[month]);
    print_char(' ');
    print_int(int_par(param_year_code));
    print_char(' ');
    print_two(int_par(param_time_code) / 60);
    print_char(':');
    print_two(int_par(param_time_code) % 60);
    if (shellenabledp) {
        wlog_cr();
        wlog(' ');
        if (restrictedshell)
            fprintf(log_file, "restricted ");
        fprintf(log_file, "\\write18 enabled.");
    }
    if (filelineerrorstylep) {
        wlog_cr();
        fprintf(log_file, " file:line:error style messages enabled.");
    }
    if (parsefirstlinep) {
        wlog_cr();
        fprintf(log_file, " %%&-line parsing enabled.");
    }
}

void print_version_banner(void)
{
  fprintf(term_out, "%s%s", BANNER, luatex_version_string); /* todo: get the extra info in here */
    fprintf(term_out, versionstring);
    /* write_svnversion(luatex_svnversion); */
}

/*
The procedure |print_esc| prints a string that is preceded by
the user's escape character (which is usually a backslash).
*/

void print_esc(str_number s)
{                               /* prints escape character, then |s| */
    integer c;                  /* the escape character code */
    /* Set variable |c| to the current escape character */
    c = int_par(param_escape_char_code);
    if (c >= 0 && c < STRING_OFFSET)
        print(c);
    print(s);
}

void tprint_esc(char *s)
{                               /* prints escape character, then |s| */
    integer c;                  /* the escape character code */
    /* Set variable |c| to the current escape character */
    c = int_par(param_escape_char_code);
    if (c >= 0 && c < STRING_OFFSET)
        print(c);
    tprint(s);
}

/* An array of digits in the range |0..15| is printed by |print_the_digs|.*/

void print_the_digs(eight_bits k)
{
    /* prints |dig[k-1]|$\,\ldots\,$|dig[0]| */
    while (k-- > 0) {
        if (dig[k] < 10)
            print_char('0' + dig[k]);
        else
            print_char('A' - 10 + dig[k]);
    }
}

/*
The following procedure, which prints out the decimal representation of a
given integer |n|, has been written carefully so that it works properly
if |n=0| or if |(-n)| would cause overflow. It does not apply |mod| or |div|
to negative arguments, since such operations are not implemented consistently
by all \PASCAL\ compilers.
*/

void print_int(longinteger n)
{                               /* prints an integer in decimal form */
    int k;                      /* index to current digit; we assume that $|n|<10^{23}$ */
    longinteger m;              /* used to negate |n| in possibly dangerous cases */
    k = 0;
    if (n < 0) {
        print_char('-');
        if (n > -100000000) {
            n = -n;
        } else {
            m = -1 - n;
            n = m / 10;
            m = (m % 10) + 1;
            k = 1;
            if (m < 10)
                dig[0] = m;
            else {
                dig[0] = 0;
                incr(n);
            }
        }
    }
    do {
        dig[k] = n % 10;
        n = n / 10;
        incr(k);
    } while (n != 0);
    print_the_digs(k);
}

/*
Here is a trivial procedure to print two digits; it is usually called with
a parameter in the range |0<=n<=99|.
*/

void print_two(integer n)
{                               /* prints two least significant digits */
    n = abs(n) % 100;
    print_char('0' + (n / 10));
    print_char('0' + (n % 10));
}

/*
Hexadecimal printing of nonnegative integers is accomplished by |print_hex|.
*/

void print_hex(integer n)
{                               /* prints a positive integer in hexadecimal form */
    int k;                      /* index to current digit; we assume that $0\L n<16^{22}$ */
    k = 0;
    print_char('"');
    do {
        dig[k] = n % 16;
        n = n / 16;
        incr(k);
    } while (n != 0);
    print_the_digs(k);
}

/*
Roman numerals are produced by the |print_roman_int| routine.  Readers
who like puzzles might enjoy trying to figure out how this tricky code
works; therefore no explanation will be given. Notice that 1990 yields
\.{mcmxc}, not \.{mxm}.
*/

void print_roman_int(integer n)
{
    char *j, *k;                /* mysterious indices */
    nonnegative_integer u, v;   /* mysterious numbers */
    char mystery[] = "m2d5c2l5x2v5i";
    j = (char *) mystery;
    v = 1000;
    while (1) {
      while (n >= (int)v) {
            print_char(*j);
            n = n - v;
        }
        if (n <= 0)
            return;             /* nonpositive input produces no output */
        k = j + 2;
        u = v / (*(k - 1) - '0');
        if (*(k - 1) == '2') {
            k = k + 2;
            u = u / (*(k - 1) - '0');
        }
        if (n + u >= v) {
            print_char(*k);
            n = n + u;
        } else {
            j = j + 2;
            v = v / (*(j - 1) - '0');
        }
    }
}

/*
The |print| subroutine will not print a string that is still being
created. The following procedure will.
*/

void print_current_string(void)
{                               /* prints a yet-unmade string */
    pool_pointer j;             /* points to current character code */
    j = str_start_macro(str_ptr);
    while (j < pool_ptr)
        print_char(str_pool[j++]);
}

/*
The procedure |print_cs| prints the name of a control sequence, given
a pointer to its address in |eqtb|. A space is printed after the name
unless it is a single nonletter or an active character. This procedure
might be invoked with invalid data, so it is ``extra robust.'' The
individual characters must be printed one at a time using |print|, since
they may be unprintable.
*/

static int null_cs = 0;
static int eqtb_size = 0;

void print_cs(integer p)
{                               /* prints a purported control sequence */
    str_number t = zget_cs_text(p);
    if (null_cs == 0) {
        null_cs = get_nullcs();
        eqtb_size = get_eqtb_size();
    }
    if (p < hash_base) {        /* nullcs */
        if (p == null_cs) {
            tprint_esc("csname");
            tprint_esc("endcsname");
        } else {
            tprint_esc("IMPOSSIBLE.");
        }
    } else if ((p >= static_undefined_control_sequence) &&
               ((p <= eqtb_size) || p > eqtb_size + hash_extra)) {
        tprint_esc("IMPOSSIBLE.");
    } else if (t >= str_ptr) {
        tprint_esc("NONEXISTENT.");
    } else {
        if (is_active_cs(t)) {
            print(active_cs_value(t));
        } else {
            print_esc(t);
            if (single_letter(t)) {
                if (get_cat_code(int_par(param_cat_code_table_code),
                                 pool_to_unichar(str_start_macro(t))) ==
                    letter_cmd)
                    print_char(' ');
            } else {
                print_char(' ');
            }
        }
    }
}

/*
Here is a similar procedure; it avoids the error checks, and it never
prints a space after the control sequence.
*/

void sprint_cs(pointer p)
{                               /* prints a control sequence */
    str_number t;
    if (null_cs == 0) {
        null_cs = get_nullcs();
        eqtb_size = get_eqtb_size();
    }
    if (p == null_cs) {
        tprint_esc("csname");
        tprint_esc("endcsname");
    } else {
        t = zget_cs_text(p);
        if (is_active_cs(t))
            print(active_cs_value(t));
        else
            print_esc(t);
    }
}


/* This procedure is never called when |interaction<scroll_mode|. */

void prompt_input(char *s)
{
    wake_up_terminal(); tprint(s); term_input();
}

