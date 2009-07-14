/* mainbody.c
   
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
% pdfTeX is copyright (C) 1996-2006 Han The Thanh, <thanh@@pdftex.org>.
% e-TeX is copyright (C) 1994,98 by Peter Breitenlohner.

% This program is directly derived from Donald E. Knuth's TeX;
% the change history which follows and the reward offered for finders of
% bugs refer specifically to TeX; they should not be taken as referring
% to e-TeX, although the change history is relevant in that it
% demonstrates the evolutionary path followed.  This program is not TeX;
% that name is reserved strictly for the program which is the creation
% and sole responsibility of Professor Knuth.

% Version 0 was released in September 1982 after it passed a variety of tests.
% Version 1 was released in November 1983 after thorough testing.
% Version 1.1 fixed ``disappearing font identifiers'' et alia (July 1984).
% Version 1.2 allowed `0' in response to an error, et alia (October 1984).
% Version 1.3 made memory allocation more flexible and local (November 1984).
% Version 1.4 fixed accents right after line breaks, et alia (April 1985).
% Version 1.5 fixed \the\toks after other expansion in \edefs (August 1985).
% Version 2.0 (almost identical to 1.5) corresponds to "Volume B" (April 1986).
% Version 2.1 corrected anomalies in discretionary breaks (January 1987).
% Version 2.2 corrected "(Please type...)" with null \endlinechar (April 1987).
% Version 2.3 avoided incomplete page in premature termination (August 1987).
% Version 2.4 fixed \noaligned rules in indented displays (August 1987).
% Version 2.5 saved cur_order when expanding tokens (September 1987).
% Version 2.6 added 10sp slop when shipping leaders (November 1987).
% Version 2.7 improved rounding of negative-width characters (November 1987).
% Version 2.8 fixed weird bug if no \patterns are used (December 1987).
% Version 2.9 made \csname\endcsname's "relax" local (December 1987).
% Version 2.91 fixed \outer\def\a0{}\a\a bug (April 1988).
% Version 2.92 fixed \patterns, also file names with complex macros (May 1988).
% Version 2.93 fixed negative halving in allocator when mem_min<0 (June 1988).
% Version 2.94 kept open_log_file from calling fatal_error (November 1988).
% Version 2.95 solved that problem a better way (December 1988).
% Version 2.96 corrected bug in "Infinite shrinkage" recovery (January 1989).
% Version 2.97 corrected blunder in creating 2.95 (February 1989).
% Version 2.98 omitted save_for_after at outer level (March 1989).
% Version 2.99 caught $$\begingroup\halign..$$ (June 1989).
% Version 2.991 caught .5\ifdim.6... (June 1989).
% Version 2.992 introduced major changes for 8-bit extensions (September 1989).
% Version 2.993 fixed a save_stack synchronization bug et alia (December 1989).
% Version 3.0 fixed unusual displays; was more \output robust (March 1990).
% Version 3.1 fixed nullfont, disabled \write{\the\prevgraf} (September 1990).
% Version 3.14 fixed unprintable font names and corrected typos (March 1991).
% Version 3.141 more of same; reconstituted ligatures better (March 1992).
% Version 3.1415 preserved nonexplicit kerns, tidied up (February 1993).
% Version 3.14159 allowed fontmemsize to change; bulletproofing (March 1995).
% Version 3.141592 fixed \xleaders, glueset, weird alignments (December 2002).
% Version 3.1415926 was a general cleanup with minor fixes (February 2008).


% Although considerable effort has been expended to make the LuaTeX program
% correct and reliable, no warranty is implied; the authors disclaim any
% obligation or liability for damages, including but not limited to
% special, indirect, or consequential damages arising out of or in
% connection with the use or performance of this software. This work has
% been a ``labor of love'' and the authors hope that users enjoy it.

% Here is TeX material that gets inserted after \input webmac
\def\hang{\hangindent 3em\noindent\ignorespaces}
\def\hangg#1 {\hang\hbox{#1 }}
\def\textindent#1{\hangindent2.5em\noindent\hbox to2.5em{\hss#1 }\ignorespaces}
\font\ninerm=cmr9
\let\mc=\ninerm % medium caps for names like SAIL
\def\PASCAL{Pascal}
\def\pdfTeX{pdf\TeX}
\def\pdfeTeX{pdf\eTeX}
\def\PDF{PDF}
\def\Aleph{Aleph}
\def\eTeX{e\TeX}
\def\LuaTeX{Lua\TeX}
\def\THANH{H\`an Th\^e\llap{\raise 0.5ex\hbox{\'{}}} Th\`anh}
\def\ph{\hbox{Pascal-H}}
\def\pct!{{\char`\%}} % percent sign in ordinary text
\def\grp{\.{\char'173...\char'175}}
\font\logo=logo10 % font used for the METAFONT logo
\def\MF{{\logo META}\-{\logo FONT}}
\def\<#1>{$\langle#1\rangle$}
\def\section{\mathhexbox278}

\def\(#1){} % this is used to make section names sort themselves better
\def\9#1{} % this is used for sort keys in the index via @@:sort key}{entry@@>

\outer\def\N#1. \[#2]#3.{\MN#1.\vfil\eject % begin starred section
  \def\rhead{PART #2:\uppercase{#3}} % define running headline
  \message{*\modno} % progress report
  \edef\next{\write\cont{\Z{\?#2]#3}{\modno}{\the\pageno}}}\next
  \ifon\startsection{\bf\ignorespaces#3.\quad}\ignorespaces}
\let\?=\relax % we want to be able to \write a \?

\def\title{LuaTeX}
\let\maybe=\iffalse % print only changed modules
\def\topofcontents{\hsize 5.5in
  \vglue 0pt plus 1fil minus 1.5in
  \def\?##1]{\hbox to 1in{\hfil##1.\ }}
  }
\def\botofcontents{\vskip 0pt plus 1fil minus 1.5in}
\pageno=3
\def\glob{13} % this should be the section number of "<Global...>"
\def\gglob{20, 26} % this should be the next two sections of "<Global...>"


This is LuaTeX, a continuation of $\pdfTeX$ and $\Aleph$.  LuaTeX is a
document compiler intended to simplify high-quality typesetting for
many of the world's languages.  It is an extension of D. E. Knuth's
\TeX, which was designed essentially for the typesetting of languages
using the Latin alphabet.

The $\Aleph$ subsystem loosens many of the restrictions imposed by~\TeX:
register numbers are no longer limited to 8~bits;  fonts may have more
than 256~characters;  more than 256~fonts may be used;  etc.

The \PASCAL\ program that follows is the definition of \TeX82, a standard
@:PASCAL}{\PASCAL@>
@!@:TeX82}{\TeX82@>
version of \TeX\ that is designed to be highly portable so that
identical output will be obtainable on a great variety of computers.

The main purpose of the following program is to explain the algorithms of \TeX\
as clearly as possible. As a result, the program will not necessarily be very
efficient when a particular \PASCAL\ compiler has translated it into a
particular machine language. However, the program has been written so that it
can be tuned to run efficiently in a wide variety of operating environments
by making comparatively few changes. Such flexibility is possible because
the documentation that follows is written in the \.{WEB} language, which is
at a higher level than \PASCAL; the preprocessing step that converts \.{WEB}
to \PASCAL\ is able to introduce most of the necessary refinements.
Semi-automatic translation to other languages is also feasible, because the
program below does not make extensive use of features that are peculiar to
\PASCAL.

A large piece of software like \TeX\ has inherent complexity that cannot
be reduced below a certain level of difficulty, although each individual
part is fairly simple by itself. The \.{WEB} language is intended to make
the algorithms as readable as possible, by reflecting the way the
individual program pieces fit together and by providing the
cross-references that connect different parts. Detailed comments about
what is going on, and about why things were done in certain ways, have
been liberally sprinkled throughout the program.  These comments explain
features of the implementation, but they rarely attempt to explain the
\TeX\ language itself, since the reader is supposed to be familiar with
{\sl The \TeX book}.
@.WEB@>
@:TeXbook}{\sl The \TeX book@>

@ The present implementation has a long ancestry, beginning in the summer
of~1977, when Michael~F. Plass and Frank~M. Liang designed and coded
a prototype
@^Plass, Michael Frederick@>
@^Liang, Franklin Mark@>
@^Knuth, Donald Ervin@>
based on some specifications that the author had made in May of that year.
This original proto\TeX\ included macro definitions and elementary
manipulations on boxes and glue, but it did not have line-breaking,
page-breaking, mathematical formulas, alignment routines, error recovery,
or the present semantic nest; furthermore,
it used character lists instead of token lists, so that a control sequence
like \.{\\halign} was represented by a list of seven characters. A
complete version of \TeX\ was designed and coded by the author in late
1977 and early 1978; that program, like its prototype, was written in the
{\mc SAIL} language, for which an excellent debugging system was
available. Preliminary plans to convert the {\mc SAIL} code into a form
somewhat like the present ``web'' were developed by Luis Trabb~Pardo and
@^Trabb Pardo, Luis Isidoro@>
the author at the beginning of 1979, and a complete implementation was
created by Ignacio~A. Zabala in 1979 and 1980. The \TeX82 program, which
@^Zabala Salelles, Ignacio Andr\'es@>
was written by the author during the latter part of 1981 and the early
part of 1982, also incorporates ideas from the 1979 implementation of
@^Guibas, Leonidas Ioannis@>
@^Sedgewick, Robert@>
@^Wyatt, Douglas Kirk@>
\TeX\ in {\mc MESA} that was written by Leonidas Guibas, Robert Sedgewick,
and Douglas Wyatt at the Xerox Palo Alto Research Center.  Several hundred
refinements were introduced into \TeX82 based on the experiences gained with
the original implementations, so that essentially every part of the system
has been substantially improved. After the appearance of ``Version 0'' in
September 1982, this program benefited greatly from the comments of
many other people, notably David~R. Fuchs and Howard~W. Trickey.
A final revision in September 1989 extended the input character set to
eight-bit codes and introduced the ability to hyphenate words from
different languages, based on some ideas of Michael~J. Ferguson.
@^Fuchs, David Raymond@>
@^Trickey, Howard Wellington@>
@^Ferguson, Michael John@>

No doubt there still is plenty of room for improvement, but the author
is firmly committed to keeping \TeX82 ``frozen'' from now on; stability
and reliability are to be its main virtues.

On the other hand, the \.{WEB} description can be extended without changing
the core of \TeX82 itself, and the program has been designed so that such
extensions are not extremely difficult to make.
The |banner| string defined here should be changed whenever \TeX\
undergoes any modifications, so that it will be clear which version of
\TeX\ might be the guilty party when a problem arises.
@^extensions to \TeX@>
@^system dependencies@>

This program contains code for various features extending \TeX,
therefore this program is called `\eTeX' and not
`\TeX'; the official name `\TeX' by itself is reserved
for software systems that are fully compatible with each other.
A special test suite called the ``\.{TRIP} test'' is available for
helping to determine whether a particular implementation deserves to be
known as `\TeX' [cf.~Stanford Computer Science report CS1027,
November 1984].

A similar test suite called the ``\.{e-TRIP} test'' is available for
helping to determine whether a particular implementation deserves to be
known as `\eTeX'.
		*/

/*
@ Different \PASCAL s have slightly different conventions, and the present
@!@:PASCAL H}{\ph@>
program expresses \TeX\ in terms of the \PASCAL\ that was
available to the author in 1982. Constructions that apply to
this particular compiler, which we shall call \ph, should help the
reader see how to make an appropriate interface for other systems
if necessary. (\ph\ is Charles Hedrick's modification of a compiler
@^Hedrick, Charles Locke@>
for the DECsystem-10 that was originally developed at the University of
Hamburg; cf.\ {\sl SOFTWARE---Practice \AM\ Experience \bf6} (1976),
29--42. The \TeX\ program below is intended to be adaptable, without
extensive changes, to most other versions of \PASCAL, so it does not fully
use the admirable features of \ph. Indeed, a conscious effort has been
made here to avoid using several idiosyncratic features of standard
\PASCAL\ itself, so that most of the code can be translated mechanically
into other high-level languages. For example, the `\&{with}' and `\\{new}'
features are not used, nor are pointer types, set types, or enumerated
scalar types; there are no `\&{var}' parameters, except in the case of files
--- \eTeX, however, does use `\&{var}' parameters for the |reverse| function;
there are no tag fields on variant records; there are no assignments
|real:=integer|; no procedures are declared local to other procedures.)

The portions of this program that involve system-dependent code, where
changes might be necessary because of differences between \PASCAL\ compilers
and/or differences between
operating systems, can be identified by looking at the sections whose
numbers are listed under `system dependencies' in the index. Furthermore,
the index entries for `dirty \PASCAL' list all places where the restrictions
of \PASCAL\ have not been followed perfectly, for one reason or another.
@!@^system dependencies@>
@!@^dirty \PASCAL@>

Incidentally, \PASCAL's standard |round| function can be problematical,
because it disagrees with the IEEE floating-point standard.
Many implementors have
therefore chosen to substitute their own home-grown rounding procedure.

@ The program begins with a normal \PASCAL\ program heading, whose
components will be filled in later, using the conventions of \.{WEB}.
@.WEB@>
For example, the portion of the program called `\X\glob:Global
variables\X' below will be replaced by a sequence of variable declarations
that starts in $\section\glob$ of this documentation. In this way, we are able
to define each individual global variable when we are prepared to
understand what it means; we do not have to define all of the globals at
once.  Cross references in $\section\glob$, where it says ``See also
sections \gglob, \dots,'' also make it possible to look at the set of
all global variables, if desired.  Similar remarks apply to the other
portions of the program heading.

*/

/*
The overall \TeX\ program begins with the heading just shown, after which
comes a bunch of procedure declarations and function declarations.
Finally we will get to the main program, which begins with the
comment `|start_here|'. If you want to skip down to the
main program now, you can look up `|start_here|' in the index.
But the author suggests that the best way to understand this program
is to follow pretty much the order of \TeX's components as they appear in the
\.{WEB} description you are now reading, since the present ordering is
intended to combine the advantages of the ``bottom up'' and ``top down''
approaches to the problem of understanding a somewhat complicated system.

@ Three labels must be declared in the main program, so we give them
symbolic names.

@ Some of the code below is intended to be used only when diagnosing the
strange behavior that sometimes occurs when \TeX\ is being installed or
when system wizards are fooling around with \TeX\ without quite knowing
what they are doing. Such code will not normally be compiled; it is
delimited by the codewords `$|debug|\ldots|gubed|$', with apologies
to people who wish to preserve the purity of English.

Similarly, there is some conditional code delimited by
`$|stat|\ldots|tats|$' that is intended for use when statistics are to be
kept about \TeX's memory usage.  The |stat| $\ldots$ |tats| code also
implements diagnostic information for \.{\\tracingparagraphs} and
\.{\\tracingpages}.
@^debugging@>

*/

/*
If the first character of a \PASCAL\ comment is a dollar sign,
\ph\ treats the comment as a list of ``compiler directives'' that will
affect the translation of this program into machine language.  The
directives shown below specify full checking and inclusion of the \PASCAL\
debugger when \TeX\ is being debugged, but they cause range checking and other
redundant code to be eliminated when the production system is being generated.
Arithmetic overflow will be detected in all cases.
@:PASCAL H}{\ph@>
@^system dependencies@>
@^overflow in arithmetic@>

@ This \TeX\ implementation conforms to the rules of the {\sl Pascal User
@:PASCAL}{\PASCAL@>
@^system dependencies@>
Manual} published by Jensen and Wirth in 1975, except where system-dependent
@^Wirth, Niklaus@>
@^Jensen, Kathleen@>
code is necessary to make a useful system program, and except in another
respect where such conformity would unnecessarily obscure the meaning
and clutter up the code: We assume that |case| statements may include a
default case that applies if no matching label is found. Thus, we shall use
constructions like
$$\vbox{\halign{\ignorespaces#\hfil\cr
|case x of|\cr
1: $\langle\,$code for $x=1\,\rangle$;\cr
3: $\langle\,$code for $x=3\,\rangle$;\cr
|othercases| $\langle\,$code for |x<>1| and |x<>3|$\,\rangle$\cr
|endcases|\cr}}$$
since most \PASCAL\ compilers have plugged this hole in the language by
incorporating some sort of default mechanism. For example, the \ph\
compiler allows `|others|:' as a default label, and other \PASCAL s allow
syntaxes like `\&{else}' or `\&{otherwise}' or `\\{otherwise}:', etc. The
definitions of |othercases| and |endcases| should be changed to agree with
local conventions.  Note that no semicolon appears before |endcases| in
this program, so the definition of |endcases| should include a semicolon
if the compiler wants one. (Of course, if no default mechanism is
available, the |case| statements of \TeX\ will have to be laboriously
extended by listing all remaining cases. People who are stuck with such
\PASCAL s have, in fact, done this, successfully but not happily!)
@:PASCAL H}{\ph@>
*/

/*
In case somebody has inadvertently made bad settings of the ``constants,''
\TeX\ checks them using a global variable called |bad|.

This is the first of many sections of \TeX\ where global variables are
defined.
*/

integer bad;                    /* is some ``constant'' wrong? */
boolean luainit;                /* are we using lua for initializations  */
boolean tracefilenames;         /* print file open-close  info? */

/*
Characters of text that have been converted to \TeX's internal form
are said to be of type |ASCII_code|, which is a subrange of the integers.
*/

/*
The original \PASCAL\ compiler was designed in the late 60s, when six-bit
character sets were common, so it did not make provision for lowercase
letters. Nowadays, of course, we need to deal with both capital and small
letters in a convenient way, especially in a program for typesetting;
so the present specification of \TeX\ has been written under the assumption
that the \PASCAL\ compiler and run-time system permit the use of text files
with more than 64 distinguishable characters. More precisely, we assume that
the character set contains at least the letters and symbols associated
with ASCII codes @'40 through @'176; all of these characters are now
available on most computer terminals.

Since we are dealing with more characters than were present in the first
\PASCAL\ compilers, we have to decide what to call the associated data
type. Some \PASCAL s use the original name |char| for the
characters in text files, even though there now are more than 64 such
characters, while other \PASCAL s consider |char| to be a 64-element
subrange of a larger data type that has some other name.

In order to accommodate this difference, we shall use the name |text_char|
to stand for the data type of the characters that are converted to and
from |ASCII_code| when they are input and output. We shall also assume
that |text_char| consists of the elements |chr(first_text_char)| through
|chr(last_text_char)|, inclusive. The following definitions should be
adjusted if necessary.
@^system dependencies@>

We are assuming that our runtime system is able to read and write UTF-8. 

Some of the ASCII codes without visible characters have been given symbolic
names in this program because they are used with a special meaning.
*/

/*
This program has two important variations: (1) There is a long and slow
version called \.{INITEX}, which does the extra calculations needed to
@.INITEX@>
initialize \TeX's internal tables; and (2)~there is a shorter and faster
production version, which cuts the initialization to a bare minimum.
*/

boolean ini_version;            /* are we \.{INITEX}? */
boolean dump_option;            /* was the dump name option used? */
boolean dump_line;              /* was a \.{\%\AM format} line seen? */
integer bound_default;          /* temporary for setup */
char *bound_name;               /* temporary for setup */
integer error_line;             /* width of context lines on terminal error messages */
integer half_error_line;        /* width of first lines of contexts in terminal
                                   error messages; should be between 30 and |error_line-15| */
integer max_print_line;         /* width of longest text lines output; should be at least 60 */
integer ocp_list_size;
integer ocp_buf_size;
integer ocp_stack_size;
integer max_strings;            /* maximum number of strings; must not exceed |max_halfword| */
integer strings_free;           /* strings available after format loaded */
integer string_vacancies;       /* the minimum number of characters that should be
                                   available for the user's control sequences and font names,
                                   after \TeX's own error messages are stored */
integer pool_size;              /* maximum number of characters in strings, including all
                                   error messages and help texts, and the names of all fonts and
                                   control sequences; must exceed |string_vacancies| by the total
                                   length of \TeX's own strings, which is currently about 23000 */
integer pool_free;              /* pool space free after format loaded */
integer font_k;                 /* loop variable for initialization */
integer buf_size;               /* maximum number of characters simultaneously present in
                                   current lines of open files and in control sequences between
                                   \.{\\csname} and \.{\\endcsname}; must not exceed |max_halfword| */
integer stack_size;             /* maximum number of simultaneous input sources */
integer max_in_open;            /* maximum number of input files and error insertions that
                                   can be going on simultaneously */
integer param_size;             /* maximum number of simultaneous macro parameters */
integer nest_size;              /* maximum number of semantic levels simultaneously active */
integer save_size;              /* space for saving values outside of current group; must be
                                   at most |max_halfword| */
integer expand_depth;           /* limits recursive calls of the |expand| procedure */
int parsefirstlinep;            /* parse the first line for options */
int filelineerrorstylep;        /* format messages as file:line:error */
int haltonerrorp;               /* stop at first error */
boolean quoted_filename;        /* current filename is quoted */

/*
In order to make efficient use of storage space, \TeX\ bases its major data
structures on a |memory_word|, which contains either a (signed) integer,
possibly scaled, or a (signed) |glue_ratio|, or a small number of
fields that are one half or one quarter of the size used for storing
integers.

If |x| is a variable of type |memory_word|, it contains up to four
fields that can be referred to as follows:
$$\vbox{\halign{\hfil#&#\hfil&#\hfil\cr
|x|&.|int|&(an |integer|)\cr
|x|&.|sc|\qquad&(a |scaled| integer)\cr
|x|&.|gr|&(a |glue_ratio|)\cr
|x.hh.lh|, |x.hh|&.|rh|&(two halfword fields)\cr
|x.hh.b0|, |x.hh.b1|, |x.hh|&.|rh|&(two quarterword fields, one halfword
  field)\cr
|x.qqqq.b0|, |x.qqqq.b1|, |x.qqqq|&.|b2|, |x.qqqq.b3|\hskip-100pt
  &\qquad\qquad\qquad(four quarterword fields)\cr}}$$
This is somewhat cumbersome to write, and not very readable either, but
macros will be used to make the notation shorter and more transparent.
The \PASCAL\ code below gives a formal definition of |memory_word| and
its subsidiary types, using packed variant records. \TeX\ makes no
assumptions about the relative positions of the fields within a word.

We are assuming 32-bit integers, a halfword must contain at least
32 bits, and a quarterword must contain at least 16 bits.
@^system dependencies@>

N.B.: Valuable memory space will be dreadfully wasted unless \TeX\ is compiled
by a \PASCAL\ that packs all of the |memory_word| variants into
the space of a single integer. This means, for example, that |glue_ratio|
words should be |short_real| instead of |real| on some computers. Some
\PASCAL\ compilers will pack an integer whose subrange is `|0..255|' into
an eight-bit field, but others insist on allocating space for an additional
sign bit; on such systems you can get 256 values into a quarterword only
if the subrange is `|-128..127|'.

The present implementation tries to accommodate as many variations as possible,
so it makes few assumptions. If integers having the subrange
`|min_quarterword..max_quarterword|' can be packed into a quarterword,
and if integers having the subrange `|min_halfword..max_halfword|'
can be packed into a halfword, everything should work satisfactorily.

It is usually most efficient to have |min_quarterword=min_halfword=0|,
so one should try to achieve this unless it causes a severe problem.
The values defined here are recommended for most 32-bit computers.

We cannot use the full range of 32 bits in a halfword, because we have
to allow negative values for potential backend tricks like web2c's
dynamic allocation, and parshapes pointers have to be able to store at
least twice the value |max_halfword| (see below). Therefore,
|max_halfword| is $2^{30}-1$
*/


/*
@ Region 3 of |eqtb| contains the |number_regs| \.{\\skip} registers, as well as the
glue parameters defined here. It is important that the ``muskip''
parameters have larger numbers than the others.

@d skip(#)==equiv(skip_base+#) {|mem| location of glue specification}
@d mu_skip(#)==equiv(mu_skip_base+#) {|mem| location of math glue spec}
@d glue_par(#)==equiv(glue_base+#) {|mem| location of glue specification}
@d line_skip==glue_par(line_skip_code)
@d baseline_skip==glue_par(baseline_skip_code)
@d par_skip==glue_par(par_skip_code)
@d above_display_skip==glue_par(above_display_skip_code)
@d below_display_skip==glue_par(below_display_skip_code)
@d above_display_short_skip==glue_par(above_display_short_skip_code)
@d below_display_short_skip==glue_par(below_display_short_skip_code)
@d left_skip==glue_par(left_skip_code)
@d right_skip==glue_par(right_skip_code)
@d top_skip==glue_par(top_skip_code)
@d split_top_skip==glue_par(split_top_skip_code)
@d tab_skip==glue_par(tab_skip_code)
@d space_skip==glue_par(space_skip_code)
@d xspace_skip==glue_par(xspace_skip_code)
@d par_fill_skip==glue_par(par_fill_skip_code)
@d thin_mu_skip==glue_par(thin_mu_skip_code)
@d med_mu_skip==glue_par(med_mu_skip_code)
@d thick_mu_skip==glue_par(thick_mu_skip_code)


@ Region 4 of |eqtb| contains the local quantities defined here. The
bulk of this region is taken up by five tables that are indexed by eight-bit
characters; these tables are important to both the syntactic and semantic
portions of \TeX. There are also a bunch of special things like font and
token parameters, as well as the tables of \.{\\toks} and \.{\\box}
registers.

@d par_shape_ptr==equiv(par_shape_loc)
@d output_routine==equiv(output_routine_loc)
@d every_par==equiv(every_par_loc)
@d every_math==equiv(every_math_loc)
@d every_display==equiv(every_display_loc)
@d every_hbox==equiv(every_hbox_loc)
@d every_vbox==equiv(every_vbox_loc)
@d every_job==equiv(every_job_loc)
@d every_cr==equiv(every_cr_loc)
@d err_help==equiv(err_help_loc)
@d pdf_pages_attr==equiv(pdf_pages_attr_loc)
@d pdf_page_attr==equiv(pdf_page_attr_loc)
@d pdf_page_resources==equiv(pdf_page_resources_loc)
@d pdf_pk_mode==equiv(pdf_pk_mode_loc)
@d toks(#)==equiv(toks_base+#)
@d local_left_box==equiv(local_left_box_base)
@d local_right_box==equiv(local_right_box_base)
@d box(#)==equiv(box_base+#)
@d cur_font==equiv(cur_font_loc)


@
@d null_font==font_base
@d var_code==7 {math code meaning ``use the current family''}

@ Region 5 of |eqtb| contains the integer parameters and registers defined
here, as well as the |del_code| table. The latter table differs from the
|cat_code..math_code| tables that precede it, since delimiter codes are
fullword integers while the other kinds of codes occupy at most a
halfword. This is what makes region~5 different from region~4. We will
store the |eq_level| information in an auxiliary array of quarterwords
that will be defined later.

@#
@#
@d count(#)==eqtb[count_base+#].int
@d attribute(#)==eqtb[attribute_base+#].int
@d int_par(#)==eqtb[int_base+#].int {an integer parameter}
@d pretolerance==int_par(pretolerance_code)
@d tolerance==int_par(tolerance_code)
@d line_penalty==int_par(line_penalty_code)
@d hyphen_penalty==int_par(hyphen_penalty_code)
@d ex_hyphen_penalty==int_par(ex_hyphen_penalty_code)
@d club_penalty==int_par(club_penalty_code)
@d widow_penalty==int_par(widow_penalty_code)
@d display_widow_penalty==int_par(display_widow_penalty_code)
@d broken_penalty==int_par(broken_penalty_code)
@d bin_op_penalty==int_par(bin_op_penalty_code)
@d rel_penalty==int_par(rel_penalty_code)
@d pre_display_penalty==int_par(pre_display_penalty_code)
@d post_display_penalty==int_par(post_display_penalty_code)
@d inter_line_penalty==int_par(inter_line_penalty_code)
@d double_hyphen_demerits==int_par(double_hyphen_demerits_code)
@d final_hyphen_demerits==int_par(final_hyphen_demerits_code)
@d adj_demerits==int_par(adj_demerits_code)
@d mag==int_par(mag_code)
@d delimiter_factor==int_par(delimiter_factor_code)
@d looseness==int_par(looseness_code)
@d time==int_par(time_code)
@d day==int_par(day_code)
@d month==int_par(month_code)
@d year==int_par(year_code)
@d show_box_breadth==int_par(show_box_breadth_code)
@d show_box_depth==int_par(show_box_depth_code)
@d hbadness==int_par(hbadness_code)
@d vbadness==int_par(vbadness_code)
@d pausing==int_par(pausing_code)
@d tracing_online==int_par(tracing_online_code)
@d tracing_macros==int_par(tracing_macros_code)
@d tracing_stats==int_par(tracing_stats_code)
@d tracing_paragraphs==int_par(tracing_paragraphs_code)
@d tracing_pages==int_par(tracing_pages_code)
@d tracing_output==int_par(tracing_output_code)
@d tracing_lost_chars==int_par(tracing_lost_chars_code)
@d tracing_commands==int_par(tracing_commands_code)
@d tracing_restores==int_par(tracing_restores_code)
@d uc_hyph==int_par(uc_hyph_code)
@d output_penalty==int_par(output_penalty_code)
@d max_dead_cycles==int_par(max_dead_cycles_code)
@d hang_after==int_par(hang_after_code)
@d floating_penalty==int_par(floating_penalty_code)
@d global_defs==int_par(global_defs_code)
@d cur_fam==int_par(cur_fam_code)
@d escape_char==int_par(escape_char_code)
@d default_hyphen_char==int_par(default_hyphen_char_code)
@d default_skew_char==int_par(default_skew_char_code)
@d end_line_char==int_par(end_line_char_code)
@d new_line_char==int_par(new_line_char_code)
@d local_inter_line_penalty==int_par(local_inter_line_penalty_code)
@d local_broken_penalty==int_par(local_broken_penalty_code)
@d no_local_whatsits==int_par(no_local_whatsits_code)
@d no_local_dirs==int_par(no_local_dirs_code)
@d level_local_dir==int_par(level_local_dir_code)
@d dir_par(#)==eqtb[dir_base+#].int {a direction parameter}
@d page_direction==dir_par(page_direction_code)
@d body_direction==dir_par(body_direction_code)
@d par_direction==dir_par(par_direction_code)
@d text_direction==dir_par(text_direction_code)
@d math_direction==dir_par(math_direction_code)
@d language==int_par(language_code)
@d cur_lang==int_par(cur_lang_code)
@d ex_hyphen_char==int_par(ex_hyphen_char_code)
@d left_hyphen_min==int_par(left_hyphen_min_code)
@d right_hyphen_min==int_par(right_hyphen_min_code)
@d holding_inserts==int_par(holding_inserts_code)
@d error_context_lines==int_par(error_context_lines_code)
@d luastartup_id==int_par(luastartup_id_code)
@d disable_lig==int_par(disable_lig_code)
@d disable_kern==int_par(disable_kern_code)
@d cat_code_table==int_par(cat_code_table_code)
@d output_box==int_par(output_box_code)
@#
@d pdf_adjust_spacing   == int_par(pdf_adjust_spacing_code)
@d pdf_protrude_chars   == int_par(pdf_protrude_chars_code)
@d pdf_tracing_fonts    == int_par(pdf_tracing_fonts_code)
@d pdf_gen_tounicode    == int_par(pdf_gen_tounicode_code)
@d pdf_output           == int_par(pdf_output_code)
@d pdf_compress_level   == int_par(pdf_compress_level_code)
@d pdf_objcompresslevel == int_par(pdf_objcompresslevel_code)
@d pdf_decimal_digits   == int_par(pdf_decimal_digits_code)
@d pdf_move_chars       == int_par(pdf_move_chars_code)
@d pdf_image_resolution == int_par(pdf_image_resolution_code)
@d pdf_pk_resolution    == int_par(pdf_pk_resolution_code)
@d pdf_unique_resname   == int_par(pdf_unique_resname_code)
@d pdf_minor_version == int_par(pdf_minor_version_code)
@d pdf_pagebox == int_par(pdf_pagebox_code)
@d pdf_inclusion_errorlevel == int_par(pdf_inclusion_errorlevel_code)
@d pdf_gamma            == int_par(pdf_gamma_code)
@d pdf_image_gamma      == int_par(pdf_image_gamma_code)
@d pdf_image_hicolor    == int_par(pdf_image_hicolor_code)
@d pdf_image_apply_gamma == int_par(pdf_image_apply_gamma_code)
@d pdf_draftmode        == int_par(pdf_draftmode_code)
@d pdf_inclusion_copy_font == int_par(pdf_inclusion_copy_font_code)
@d pdf_replace_font == int_par(pdf_replace_font_code)
@#
@d tracing_assigns==int_par(tracing_assigns_code)
@d tracing_groups==int_par(tracing_groups_code)
@d tracing_ifs==int_par(tracing_ifs_code)
@d tracing_scan_tokens==int_par(tracing_scan_tokens_code)
@d tracing_nesting==int_par(tracing_nesting_code)
@d pre_display_direction==int_par(pre_display_direction_code)
@d last_line_fit==int_par(last_line_fit_code)
@d saving_vdiscards==int_par(saving_vdiscards_code)
@d saving_hyph_codes==int_par(saving_hyph_codes_code)
@d suppress_fontnotfound_error==int_par(suppress_fontnotfound_error_code)
@d suppress_long_error==int_par(suppress_long_error_code)
@d suppress_ifcsname_error==int_par(suppress_ifcsname_error_code)
@d synctex==int_par(synctex_code)


@ The integer parameters should really be initialized by a macro package;
the following initialization does the minimum to keep \TeX\ from
complete failure.
@^null delimiter@>
*/

/*
The following procedure, which is called just before \TeX\ initializes its
input and output, establishes the initial values of the date and time.
It calls a macro-defined |date_and_time| routine.  |date_and_time|
in turn is a C macro, which calls |get_date_and_time|, passing
it the addresses of the day, month, etc., so they can be set by the
routine.  |get_date_and_time| also sets up interrupt catching if that
is conditionally compiled in the C code.
@^system dependencies@>
*/

/*
@ The final region of |eqtb| contains the dimension parameters defined
here, and the |number_regs| \.{\\dimen} registers.

@d dimen(#)==eqtb[scaled_base+#].sc
@d dimen_par(#)==eqtb[dimen_base+#].sc {a scaled quantity}
@d par_indent==dimen_par(par_indent_code)
@d math_surround==dimen_par(math_surround_code)
@d line_skip_limit==dimen_par(line_skip_limit_code)
@d hsize==dimen_par(hsize_code)
@d vsize==dimen_par(vsize_code)
@d max_depth==dimen_par(max_depth_code)
@d split_max_depth==dimen_par(split_max_depth_code)
@d box_max_depth==dimen_par(box_max_depth_code)
@d hfuzz==dimen_par(hfuzz_code)
@d vfuzz==dimen_par(vfuzz_code)
@d delimiter_shortfall==dimen_par(delimiter_shortfall_code)
@d null_delimiter_space==dimen_par(null_delimiter_space_code)
@d script_space==dimen_par(script_space_code)
@d pre_display_size==dimen_par(pre_display_size_code)
@d display_width==dimen_par(display_width_code)
@d display_indent==dimen_par(display_indent_code)
@d overfull_rule==dimen_par(overfull_rule_code)
@d hang_indent==dimen_par(hang_indent_code)
@d h_offset==dimen_par(h_offset_code)
@d v_offset==dimen_par(v_offset_code)
@d emergency_stretch==dimen_par(emergency_stretch_code)
@d page_left_offset==dimen_par(page_left_offset_code)
@d page_top_offset==dimen_par(page_top_offset_code)
@d page_right_offset==dimen_par(page_right_offset_code)
@d page_bottom_offset==dimen_par(page_bottom_offset_code)
@d pdf_h_origin      == dimen_par(pdf_h_origin_code)
@d pdf_v_origin      == dimen_par(pdf_v_origin_code)
@d page_width    == dimen_par(page_width_code)
@d page_height   == dimen_par(page_height_code)
@d pdf_link_margin   == dimen_par(pdf_link_margin_code)
@d pdf_dest_margin   == dimen_par(pdf_dest_margin_code)
@d pdf_thread_margin == dimen_par(pdf_thread_margin_code)
@d pdf_first_line_height == dimen_par(pdf_first_line_height_code)
@d pdf_last_line_depth   == dimen_par(pdf_last_line_depth_code)
@d pdf_each_line_height  == dimen_par(pdf_each_line_height_code)
@d pdf_each_line_depth   == dimen_par(pdf_each_line_depth_code)
@d pdf_ignored_dimen     == dimen_par(pdf_ignored_dimen_code)
@d pdf_px_dimen      == dimen_par(pdf_px_dimen_code)

*/

/*
Control sequences are stored and retrieved by means of a fairly standard hash
table algorithm called the method of ``coalescing lists'' (cf.\ Algorithm 6.4C
in {\sl The Art of Computer Programming\/}). Once a control sequence enters the
table, it is never removed, because there are complicated situations
involving \.{\\gdef} where the removal of a control sequence at the end of
a group would be a mistake preventable only by the introduction of a
complicated reference-count mechanism.

The actual sequence of letters forming a control sequence identifier is
stored in the |str_pool| array together with all the other strings. An
auxiliary array |hash| consists of items with two halfword fields per
word. The first of these, called |next(p)|, points to the next identifier
belonging to the same coalesced list as the identifier corresponding to~|p|;
and the other, called |text(p)|, points to the |str_start| entry for
|p|'s identifier. If position~|p| of the hash table is empty, we have
|text(p)=0|; if position |p| is either empty or the end of a coalesced
hash list, we have |next(p)=0|. An auxiliary pointer variable called
|hash_used| is maintained in such a way that all locations |p>=hash_used|
are nonempty. The global variable |cs_count| tells how many multiletter
control sequences have been defined, if statistics are being kept.

A global boolean variable called |no_new_control_sequence| is set to
|true| during the time that new hash table entries are forbidden.
*/

#define next(A) hash[(A)].lhfield       /* link for coalesced lists */
#define text(A) hash[(A)].rh    /* string number for control sequence name */

two_halves *hash;               /* the hash table */
halfword hash_used;             /* allocation pointer for |hash| */
integer hash_extra;             /* |hash_extra=hash| above |eqtb_size| */
halfword hash_top;              /* maximum of the hash array */
halfword hash_high;             /* pointer to next high hash location */
boolean no_new_control_sequence;        /* are new identifiers legal? */
integer cs_count;               /* total number of known identifiers */

/*
The extra set of functions make sure we can query the
primitive meanings of thing s from C code
*/

str_number get_cs_text(integer cs)
{
    if (cs == null_cs)
        return maketexstring("\\csname\\endcsname");
    else if ((text(cs) < 0) || (text(cs) >= str_ptr))
        return get_nullstr();
    else
        return text(cs);
}

/*
Each primitive has a corresponding inverse, so that it is possible to
display the cryptic numeric contents of |eqtb| in symbolic form.
Every call of |primitive| in this program is therefore accompanied by some
straightforward code that forms part of the |print_cmd_chr| routine
below.

@ We will deal with the other primitives later, at some point in the program
where their |eq_type| and |equiv| values are more meaningful.  For example,
the primitives for math mode will be loaded when we consider the routines
that deal with formulas. It is easy to find where each particular
primitive was treated by looking in the index at the end; for example, the
section where |"radical"| entered |eqtb| is listed under `\.{\\radical}
primitive'. (Primitives consisting of a single nonalphabetic character,
@!like `\.{\\/}', are listed under `Single-character primitives'.)
@!@^Single-character primitives@>
*/

/*
@ Here are the group codes that are used to discriminate between different
kinds of groups. They allow \TeX\ to decide what special actions, if any,
should be performed when a group ends.
\def\grp{\.{\char'173...\char'175}}

Some groups are not supposed to be ended by right braces. For example,
the `\.\$' that begins a math formula causes a |math_shift_group| to
be started, and this should be terminated by a matching `\.\$'. Similarly,
a group that starts with \.{\\left} should end with \.{\\right}, and
one that starts with \.{\\begingroup} should end with \.{\\endgroup}.

@d bottom_level=0 {group code for the outside world}
@d simple_group=1 {group code for local structure only}
@d hbox_group=2 {code for `\.{\\hbox}\grp'}
@d adjusted_hbox_group=3 {code for `\.{\\hbox}\grp' in vertical mode}
@d vbox_group=4 {code for `\.{\\vbox}\grp'}
@d vtop_group=5 {code for `\.{\\vtop}\grp'}
@d align_group=6 {code for `\.{\\halign}\grp', `\.{\\valign}\grp'}
@d no_align_group=7 {code for `\.{\\noalign}\grp'}
@d output_group=8 {code for output routine}
@d math_group=9 {code for, e.g., `\.{\char'136}\grp'}
@d disc_group=10 {code for `\.{\\discretionary}\grp\grp\grp'}
@d insert_group=11 {code for `\.{\\insert}\grp', `\.{\\vadjust}\grp'}
@d vcenter_group=12 {code for `\.{\\vcenter}\grp'}
@d math_choice_group=13 {code for `\.{\\mathchoice}\grp\grp\grp\grp'}
@d semi_simple_group=14 {code for `\.{\\begingroup...\\endgroup}'}
@d math_shift_group=15 {code for `\.{\$...\$}'}
@d math_left_group=16 {code for `\.{\\left...\\right}'}
@d local_box_group=17 {code for `\.{\\localleftbox...\\localrightbox}'}
@d max_group_code=17
@d split_off_group=18 {box code for the top part of a \.{\\vsplit}}
@d split_keep_group=19 {box code for the bottom part of a \.{\\vsplit}}
@d preamble_group=20 {box code for the preamble processing  in an alignment}
@d align_set_group=21 {box code for the final item pass in an alignment}
@d fin_row_group=22 {box code for a provisory line in an alignment}

*/

/*
@ Most of the parameters kept in |eqtb| can be changed freely, but there's
an exception:  The magnification should not be used with two different
values during any \TeX\ job, since a single magnification is applied to an
entire run. The global variable |mag_set| is set to the current magnification
whenever it becomes necessary to ``freeze'' it at a particular value.
*/

integer mag_set;                /* if nonzero, this magnification should be used henceforth */

/*
The |prepare_mag| subroutine is called whenever \TeX\ wants to use |mag|
for magnification.
*/

#define mag int_par(mag_code)

void prepare_mag(void)
{
    if ((mag_set > 0) && (mag != mag_set)) {
        print_err("Incompatible magnification (");
        print_int(mag);
        tprint(");");
        tprint_nl(" the previous value will be retained");
        help2("I can handle only one magnification ratio per job. So I've",
              "reverted to the magnification you used earlier on this run.");
        int_error(mag_set);
        geq_word_define(int_base + mag_code, mag_set);  /* |mag:=mag_set| */
    }
    if ((mag <= 0) || (mag > 32768)) {
        print_err("Illegal magnification has been changed to 1000");
        help1("The magnification ratio must be between 1 and 32768.");
        int_error(mag);
        geq_word_define(int_base + mag_code, 1000);
    }
    if ((mag_set == 0) && (mag != mag_set)) {
        if (mag != 1000)
            one_true_inch = xn_over_d(one_hundred_inch, 10, mag);
        else
            one_true_inch = one_inch;
    }
    mag_set = mag;
}

/*
Let's pause a moment now and try to look at the Big Picture.
The \TeX\ program consists of three main parts: syntactic routines,
semantic routines, and output routines. The chief purpose of the
syntactic routines is to deliver the user's input to the semantic routines,
one token at a time. The semantic routines act as an interpreter
responding to these tokens, which may be regarded as commands. And the
output routines are periodically called on to convert box-and-glue
lists into a compact set of instructions that will be sent
to a typesetter. We have discussed the basic data structures and utility
routines of \TeX, so we are good and ready to plunge into the real activity by
considering the syntactic routines.

Our current goal is to come to grips with the |get_next| procedure,
which is the keystone of \TeX's input mechanism. Each call of |get_next|
sets the value of three variables |cur_cmd|, |cur_chr|, and |cur_cs|,
representing the next input token.
$$\vbox{\halign{#\hfil\cr
  \hbox{|cur_cmd| denotes a command code from the long list of codes
   given above;}\cr
  \hbox{|cur_chr| denotes a character code or other modifier of the command
   code;}\cr
  \hbox{|cur_cs| is the |eqtb| location of the current control sequence,}\cr
  \hbox{\qquad if the current token was a control sequence,
   otherwise it's zero.}\cr}}$$
Underlying this external behavior of |get_next| is all the machinery
necessary to convert from character files to tokens. At a given time we
may be only partially finished with the reading of several files (for
which \.{\\input} was specified), and partially finished with the expansion
of some user-defined macros and/or some macro parameters, and partially
finished with the generation of some text in a template for \.{\\halign},
and so on. When reading a character file, special characters must be
classified as math delimiters, etc.; comments and extra blank spaces must
be removed, paragraphs must be recognized, and control sequences must be
found in the hash table. Furthermore there are occasions in which the
scanning routines have looked ahead for a word like `\.{plus}' but only
part of that word was found, hence a few characters must be put back
into the input and scanned again.

To handle these situations, which might all be present simultaneously,
\TeX\ uses various stacks that hold information about the incomplete
activities, and there is a finite state control for each level of the
input mechanism. These stacks record the current state of an implicitly
recursive process, but the |get_next| procedure is not recursive.
Therefore it will not be difficult to translate these algorithms into
low-level languages that do not support recursion.
*/

integer cur_cmd;                /* current command set by |get_next| */
halfword cur_chr;               /* operand of current command */
halfword cur_cs;                /* control sequence found here, zero if none found */
halfword cur_tok;               /* packed representative of |cur_cmd| and |cur_chr| */

/* Here is a procedure that displays the current command. */

#define mode cur_list.mode_field

void show_cur_cmd_chr(void)
{
    integer n;                  /* level of \.{\\if...\\fi} nesting */
    integer l;                  /* line where \.{\\if} started */
    halfword p;
    begin_diagnostic();
    tprint_nl("{");
    if (mode != shown_mode) {
        print_mode(mode);
        tprint(": ");
        shown_mode = mode;
    }
    print_cmd_chr(cur_cmd, cur_chr);
    if (int_par(tracing_ifs_code) > 0) {
        if (cur_cmd >= if_test_cmd) {
            if (cur_cmd <= fi_or_else_cmd) {
                tprint(": ");
                if (cur_cmd == fi_or_else_cmd) {
                    print_cmd_chr(if_test_cmd, cur_if);
                    print_char(' ');
                    n = 0;
                    l = if_line;
                } else {
                    n = 1;
                    l = line;
                }
                p = cond_ptr;
                while (p != null) {
                    incr(n);
                    p = vlink(p);
                }
                tprint("(level ");
                print_int(n);
                print_char(')');
                print_if_line(l);
            }
        }
    }
    print_char('}');
    end_diagnostic(false);
}

/*
@ Users refer to `\.{\\the\\spacefactor}' only in horizontal
mode, and to `\.{\\the\\prevdepth}' only in vertical mode; so we put the
associated mode in the modifier part of the |set_aux| command.
The |set_page_int| command has modifier 0 or 1, for `\.{\\deadcycles}' and
`\.{\\insertpenalties}', respectively. The |set_box_dimen| command is
modified by either |width_offset|, |height_offset|, or |depth_offset|.
And the |last_item| command is modified by either |int_val|, |dimen_val|,
|glue_val|, |input_line_no_code|, or |badness_code|.
\pdfTeX\ adds the codes for its extensions: |pdftex_version_code|, \dots\ .
\eTeX\ inserts |last_node_type_code| after |glue_val| and adds
the codes for its extensions: |eTeX_version_code|, \dots\ .

@d glue_val=3 {this is a temp hack }
@d last_node_type_code=glue_val+1 {code for \.{\\lastnodetype}}
@d input_line_no_code=glue_val+2 {code for \.{\\inputlineno}}
@d badness_code=input_line_no_code+1 {code for \.{\\badness}}
@#
@d pdftex_first_rint_code     = badness_code + 1 {base for \pdfTeX's command codes}
@d pdftex_version_code        = pdftex_first_rint_code + 0 {code for \.{\\pdftexversion}}
@d pdf_last_obj_code          = pdftex_first_rint_code + 1 {code for \.{\\pdflastobj}}
@d pdf_last_xform_code        = pdftex_first_rint_code + 2 {code for \.{\\pdflastxform}}
@d pdf_last_ximage_code       = pdftex_first_rint_code + 3 {code for \.{\\pdflastximage}}
@d pdf_last_ximage_pages_code = pdftex_first_rint_code + 4 {code for \.{\\pdflastximagepages}}
@d pdf_last_annot_code        = pdftex_first_rint_code + 5 {code for \.{\\pdflastannot}}
@d pdf_last_x_pos_code        = pdftex_first_rint_code + 6 {code for \.{\\pdflastxpos}}
@d pdf_last_y_pos_code        = pdftex_first_rint_code + 7 {code for \.{\\pdflastypos}}
@d pdf_retval_code            = pdftex_first_rint_code + 8 {global multi-purpose return value}
@d pdf_last_ximage_colordepth_code = pdftex_first_rint_code + 9 {code for \.{\\pdflastximagecolordepth}}
@d random_seed_code           = pdftex_first_rint_code + 10 {code for \.{\\pdfrandomseed}}
@d pdf_last_link_code         = pdftex_first_rint_code + 11 {code for \.{\\pdflastlink}}
@d luatex_version_code        = pdftex_first_rint_code + 12 {code for \.{\\luatexversion}}
@d pdftex_last_item_codes     = pdftex_first_rint_code + 12 {end of \pdfTeX's command codes}
@#
@d Aleph_int=pdftex_last_item_codes+1 {first of \Aleph\ codes for integers}
@d Aleph_int_num=5 {number of \Aleph\ integers}
@d eTeX_int=Aleph_int+Aleph_int_num {first of \eTeX\ codes for integers}
@d eTeX_dim=eTeX_int+8 {first of \eTeX\ codes for dimensions}
@d eTeX_glue=eTeX_dim+9 {first of \eTeX\ codes for glue}
@d eTeX_mu=eTeX_glue+1 {first of \eTeX\ codes for muglue}
@d eTeX_expr=eTeX_mu+1 {first of \eTeX\ codes for expressions}

@d last_item_lastpenalty_code==int_val_level
@d last_item_lastkern_code==dimen_val_level
@d last_item_lastskip_code==glue_val_level

@ Inside an \.{\\output} routine, a user may wish to look at the page totals
that were present at the moment when output was triggered.
*/

/*
@ The primitives \.{\\number}, \.{\\romannumeral}, \.{\\string}, \.{\\meaning},
\.{\\fontname}, and \.{\\jobname} are defined as follows.

\eTeX\ adds \.{\\eTeXrevision} such that |job_name_code| remains last.

\pdfTeX\ adds \.{\\eTeXrevision}, \.{\\pdftexrevision}, \.{\\pdftexbanner},
\.{\\pdffontname}, \.{\\pdffontobjnum}, \.{\\pdffontsize}, and \.{\\pdfpageref}
such that |job_name_code| remains last.

@d number_code=0 {command code for \.{\\number}}
@d roman_numeral_code=1 {command code for \.{\\romannumeral}}
@d string_code=2 {command code for \.{\\string}}
@d meaning_code=3 {command code for \.{\\meaning}}
@d font_name_code=4 {command code for \.{\\fontname}}
@d etex_code=5 {command code for \.{\\eTeXVersion}}
@d omega_code=6 {command code for \.{\\OmegaVersion}}
@d aleph_code=7 {command code for \.{\\AlephVersion}}
@d format_name_code=8 {command code for \.{\\AlephVersion}}
@d pdftex_first_expand_code = 9 {base for \pdfTeX's command codes}
@d pdftex_revision_code     = pdftex_first_expand_code + 0 {command code for \.{\\pdftexrevision}}
@d pdftex_banner_code       = pdftex_first_expand_code + 1 {command code for \.{\\pdftexbanner}}
@d pdf_font_name_code       = pdftex_first_expand_code + 2 {command code for \.{\\pdffontname}}
@d pdf_font_objnum_code     = pdftex_first_expand_code + 3 {command code for \.{\\pdffontobjnum}}
@d pdf_font_size_code       = pdftex_first_expand_code + 4 {command code for \.{\\pdffontsize}}
@d pdf_page_ref_code        = pdftex_first_expand_code + 5 {command code for \.{\\pdfpageref}}
@d pdf_xform_name_code      = pdftex_first_expand_code + 6 {command code for \.{\\pdfxformname}}
@d left_margin_kern_code    = pdftex_first_expand_code + 7 {command code for \.{\\leftmarginkern}}
@d right_margin_kern_code   = pdftex_first_expand_code + 8 {command code for \.{\\rightmarginkern}}
@d pdf_creation_date_code   = pdftex_first_expand_code + 9 {command code for \.{\\pdfcreationdate}}
@d uniform_deviate_code     = pdftex_first_expand_code + 10 {command code for \.{\\uniformdeviate}}
@d normal_deviate_code      = pdftex_first_expand_code + 11 {command code for \.{\\normaldeviate}}
@d pdf_insert_ht_code       = pdftex_first_expand_code + 12 {command code for \.{\\pdfinsertht}}
@d pdf_ximage_bbox_code     = pdftex_first_expand_code + 13 {command code for \.{\\pdfximagebbox}}
@d lua_code                 = pdftex_first_expand_code + 14 {command code for \.{\\directlua}}
@d lua_escape_string_code   = pdftex_first_expand_code + 15 {command code for \.{\\luaescapestring}}
@d pdf_colorstack_init_code = pdftex_first_expand_code + 16 {command code for \.{\\pdfcolorstackinit}}
@d luatex_revision_code     = pdftex_first_expand_code + 17 {command code for \.{\\luatexrevision}}
@d luatex_date_code         = pdftex_first_expand_code + 18 {command code for \.{\\luatexdate}}
@d math_style_code          = pdftex_first_expand_code + 19 {command code for \.{\\mathstyle}}
@d expanded_code            = pdftex_first_expand_code + 20 {command code for \.{\\expanded}}
@d pdftex_convert_codes     = pdftex_first_expand_code + 21 {end of \pdfTeX's command codes}
@d job_name_code=pdftex_convert_codes {command code for \.{\\jobname}}


*/

/*
When the user defines \.{\\font\\f}, say, \TeX\ assigns an internal number
to the user's font~\.{\\f}. Adding this number to |font_id_base| gives the
|eqtb| location of a ``frozen'' control sequence that will always select
the font.
*/

integer font_bytes;

void set_cur_font(internal_font_number f)
{
    int a = 0;                  /* never global */
    define(cur_font_loc, data_cmd, f);
}

integer get_luatexversion(void)
{
    return the_luatex_version;
}

str_number get_luatexrevision(void)
{
    return the_luatex_revision;
}

integer get_luatex_date_info(void)
{
    return luatex_date_info;    /* todo, silly value */
}

/*
Now we are ready to declare our new procedure |ship_out|.  It will call
|pdf_ship_out| if the integer parameter |pdf_output| is positive; otherwise it
will call |dvi_ship_out|, which is the \TeX\ original |ship_out|.
*/

void ship_out(halfword p)
{                               /* output the box |p| */
    fix_pdfoutput();
    if (int_par(pdf_output_code) > 0)
        pdf_ship_out(static_pdf, p, true);
    else
        dvi_ship_out(p);
}

/*
This is it: the part of \TeX\ that executes all those procedures we have
written.

@ We have noted that there are two versions of \TeX82. One, called \.{INITEX},
@.INITEX@>
has to be run first; it initializes everything from scratch, without
reading a format file, and it has the capability of dumping a format file.
The other one is called `\.{VIRTEX}'; it is a ``virgin'' program that needs
@.VIRTEX@>
to input a format file in order to get started. \.{VIRTEX} typically has
more memory capacity than \.{INITEX}, because it does not need the space
consumed by the auxiliary hyphenation tables and the numerous calls on
|primitive|, etc.

The \.{VIRTEX} program cannot read a format file instantaneously, of course;
the best implementations therefore allow for production versions of \TeX\ that
not only avoid the loading routine for \PASCAL\ object code, they also have
a format file pre-loaded. This is impossible to do if we stick to standard
\PASCAL; but there is a simple way to fool many systems into avoiding the
initialization, as follows:\quad(1)~We declare a global integer variable
called |ready_already|. The probability is negligible that this
variable holds any particular value like 314159 when \.{VIRTEX} is first
loaded.\quad(2)~After we have read in a format file and initialized
everything, we set |ready_already:=314159|.\quad(3)~Soon \.{VIRTEX}
will print `\.*', waiting for more input; and at this point we
interrupt the program and save its core image in some form that the
operating system can reload speedily.\quad(4)~When that core image is
activated, the program starts again at the beginning; but now
|ready_already=314159| and all the other global variables have
their initial values too. The former chastity has vanished!

In other words, if we allow ourselves to test the condition
|ready_already=314159|, before |ready_already| has been
assigned a value, we can avoid the lengthy initialization. Dirty tricks
rarely pay off so handsomely.
@^dirty \PASCAL@>
@^system dependencies@>
*/

/*
@ Now this is really it: \TeX\ starts and ends here.

The initial test involving |ready_already| should be deleted if the
\PASCAL\ runtime system is smart enough to detect such a ``mistake.''
@^system dependencies@>

*/

#define const_chk(A) do {			\
	if (A < inf_##A) A = inf_##A;		\
	if (A > sup_##A) A = sup_##A;		\
    } while (0)

#define setup_bound_var(A,B,C) do {				\
	if (luainit>0) {					\
	    get_lua_number("texconfig",B,&C);			\
	    if (C==0) C=A;					\
	} else {						\
	    setupboundvariable(&C, B, A);			\
	}							\
    } while (0)


int ready_already = 0;

void main_body(void)
{

    /*Bounds that may be set from the configuration file. We want the user to
       be able to specify the names with underscores, but \.{TANGLE} removes
       underscores, so we're stuck giving the names twice, once as a string,
       once as the identifier. How ugly. */

    setup_bound_var(100000, "pool_size", pool_size);
    setup_bound_var(75000, "string_vacancies", string_vacancies);
    setup_bound_var(5000, "pool_free", pool_free);      /* min pool avail after fmt */
    setup_bound_var(15000, "max_strings", max_strings);
    setup_bound_var(100, "strings_free", strings_free);
    setup_bound_var(3000, "buf_size", buf_size);
    setup_bound_var(50, "nest_size", nest_size);
    setup_bound_var(15, "max_in_open", max_in_open);
    setup_bound_var(60, "param_size", param_size);
    setup_bound_var(4000, "save_size", save_size);
    setup_bound_var(300, "stack_size", stack_size);
    setup_bound_var(16384, "dvi_buf_size", dvi_buf_size);
    setup_bound_var(79, "error_line", error_line);
    setup_bound_var(50, "half_error_line", half_error_line);
    setup_bound_var(79, "max_print_line", max_print_line);
    setup_bound_var(1000, "ocp_list_size", ocp_list_size);
    setup_bound_var(1000, "ocp_buf_size", ocp_buf_size);
    setup_bound_var(1000, "ocp_stack_size", ocp_stack_size);
    setup_bound_var(0, "hash_extra", hash_extra);
    setup_bound_var(72, "pk_dpi", pk_dpi);
    setup_bound_var(10000, "expand_depth", expand_depth);

    /* Check other constants against their sup and inf. */
    const_chk(buf_size);
    const_chk(nest_size);
    const_chk(max_in_open);
    const_chk(param_size);
    const_chk(save_size);
    const_chk(stack_size);
    const_chk(dvi_buf_size);
    const_chk(pool_size);
    const_chk(string_vacancies);
    const_chk(pool_free);
    const_chk(max_strings);
    const_chk(strings_free);
    const_chk(hash_extra);
    const_chk(pk_dpi);
    if (error_line > ssup_error_line)
        error_line = ssup_error_line;

    /* array memory allocation */
    buffer = xmallocarray(packed_ASCII_code, buf_size);
    nest = xmallocarray(list_state_record, nest_size);
    save_stack = xmallocarray(memory_word, save_size);
    input_stack = xmallocarray(in_state_record, stack_size);
    input_file = xmallocarray(alpha_file, max_in_open);
    input_file_callback_id = xmallocarray(integer, max_in_open);
    line_stack = xmallocarray(integer, max_in_open);
    eof_seen = xmallocarray(boolean, max_in_open);
    grp_stack = xmallocarray(save_pointer, max_in_open);
    if_stack = xmallocarray(pointer, max_in_open);
    source_filename_stack = xmallocarray(str_number, max_in_open);
    full_source_filename_stack = xmallocarray(str_number, max_in_open);
    param_stack = xmallocarray(halfword, param_size);
    dvi_buf = xmallocarray(eight_bits, dvi_buf_size);
    initialize_ocplist_arrays(ocp_list_size);
    initialize_ocp_buffers(ocp_buf_size, ocp_stack_size);

    if (ini_version) {
        fixmem = xmallocarray(smemory_word, fix_mem_init + 1);
        memset(voidcast(fixmem), 0, (fix_mem_init + 1) * sizeof(smemory_word));
        fix_mem_min = 0;
        fix_mem_max = fix_mem_init;
        eqtb_top = eqtb_size + hash_extra;
        if (hash_extra == 0)
            hash_top = undefined_control_sequence;
        else
            hash_top = eqtb_top;
        hash = xmallocarray(two_halves, (hash_top + 1));
        memset(hash, 0, sizeof(two_halves) * (hash_top + 1));
        eqtb = xmallocarray(memory_word, (eqtb_top + 1));
        memset(eqtb, 0, sizeof(memory_word) * (eqtb_top + 1));
        str_start = xmallocarray(pool_pointer, max_strings);
        memset(str_start, 0, max_strings * sizeof(pool_pointer));
        str_pool = xmallocarray(packed_ASCII_code, pool_size);
        memset(str_pool, 0, pool_size * sizeof(packed_ASCII_code));
    }

    history = fatal_error_stop; /* in case we quit during initialization */
    t_open_out();               /* open the terminal for output */
    /* Check the ``constant'' values... */
    bad = 0;
    if (!luainit)
        tracefilenames = true;
    if ((half_error_line < 30) || (half_error_line > error_line - 15))
        bad = 1;
    if (max_print_line < 60)
        bad = 2;
    if (dvi_buf_size % 8 != 0)
        bad = 3;
    if (hash_prime > hash_size)
        bad = 5;
    if (max_in_open >= 128)
        bad = 6;
    /* Here are the inequalities that the quarterword and halfword values
       must satisfy (or rather, the inequalities that they mustn't satisfy): */
    if ((min_quarterword > 0) || (max_quarterword < 0x7FFF))
        bad = 11;
    if ((min_halfword > 0) || (max_halfword < 0x3FFFFFFF))
        bad = 12;
    if ((min_quarterword < min_halfword) || (max_quarterword > max_halfword))
        bad = 13;
    if (font_base < min_quarterword)
        bad = 15;
    if ((save_size > max_halfword) || (max_strings > max_halfword))
        bad = 17;
    if (buf_size > max_halfword)
        bad = 18;
    if (max_quarterword - min_quarterword < 0xFFFF)
        bad = 19;

    if (cs_token_flag + eqtb_size + hash_extra > max_halfword)
        bad = 21;
    if (format_default_length > file_name_size)
        bad = 31;

    if (bad > 0) {
        wterm_cr();
        fprintf(term_out,
                "Ouch---my internal constants have been clobbered! ---case %d",
                (int) bad);
        goto FINAL_END;
    }
    initialize();               /* set global variables to their starting values */
    if (ini_version) {
        if (!get_strings_started())
            goto FINAL_END;
        init_prim();            /* call |primitive| for each primitive */
        init_str_ptr = str_ptr;
        init_pool_ptr = pool_ptr;
        fix_date_and_time();
    }
    ready_already = 314159;
    print_banner(luatex_version_string, luatex_date_info);
    /* Get the first line of input and prepare to start */
    /* When we begin the following code, \TeX's tables may still contain garbage;
       the strings might not even be present. Thus we must proceed cautiously to get
       bootstrapped in.

       But when we finish this part of the program, \TeX\ is ready to call on the
       |main_control| routine to do its work.
     */
    initialize_inputstack();
    enable_etex();
    if (!no_new_control_sequence)       /* just entered extended mode ? */
        no_new_control_sequence = true;
    else if ((format_ident == 0) || (buffer[iloc] == '&') || dump_line) {
        if (format_ident != 0)
            initialize();       /* erase preloaded format */
        if (!open_fmt_file())
            goto FINAL_END;
        if (!load_fmt_file()) {
            w_close(fmt_file);
            goto FINAL_END;
        }
        w_close(fmt_file);
        while ((iloc < ilimit) && (buffer[iloc] == ' '))
            incr(iloc);
    }
    if (pdf_output_option != 0)
        int_par(pdf_output_code) = pdf_output_value;
    if (pdf_draftmode_option != 0)
        int_par(pdf_draftmode_code) = pdf_draftmode_value;
    pdf_init_map_file("pdftex.map");
    if (end_line_char_inactive())
        decr(ilimit);
    else
        buffer[ilimit] = int_par(end_line_char_code);
    fix_date_and_time();
    if (ini_version)
        make_pdftex_banner();
    random_seed = (microseconds * 1000) + (epochseconds % 1000000);
    init_randoms(random_seed);
    initialize_math();
    fixup_selector(log_opened);
    check_texconfig_init();
    if ((iloc < ilimit) && (get_cat_code(int_par(cat_code_table_code),
                                         buffer[iloc]) !=
                            int_par(escape_char_code)))
        start_input();          /* \.{\\input} assumed */
    /* DIR: Initialize |text_dir_ptr| */
    text_dir_ptr = new_dir(0);

    history = spotless;         /* ready to go! */
    /* Initialize synctex primitive */
    synctex_init_command();
    main_control();             /* come to life */
    final_cleanup();            /* prepare for death */
    close_files_and_terminate();
  FINAL_END:
    do_final_end();
}

/*
Here we do whatever is needed to complete \TeX's job gracefully on the
local operating system. The code here might come into play after a fatal
error; it must therefore consist entirely of ``safe'' operations that
cannot produce error messages. For example, it would be a mistake to call
|str_room| or |make_string| at this time, because a call on |overflow|
might lead to an infinite loop.
@^system dependencies@>

Actually there's one way to get error messages, via |prepare_mag|;
but that can't cause infinite recursion.
@^recursion@>

This program doesn't bother to close the input files that may still be open.
*/

void close_files_and_terminate(void)
{
    integer k;                  /* all-purpose index */
    integer callback_id;
    callback_id = callback_defined(stop_run_callback);
    /* Finish the extensions */
    for (k = 0; k <= 15; k++)
        if (write_open[k])
            lua_a_close_out(write_file[k]);
    if (int_par(tracing_stats_code) > 0) {
        if (callback_id == 0) {
            /* Output statistics about this job */
            /* The present section goes directly to the log file instead of using
               |print| commands, because there's no need for these strings to take
               up |str_pool| memory when a non-{\bf stat} version of \TeX\ is being used.
             */

            if (log_opened) {
                fprintf(log_file,
                        "\n\nHere is how much of LuaTeX's memory you used:\n");
                fprintf(log_file, " %d string%s out of %d\n",
                        (int) (str_ptr - init_str_ptr),
                        (str_ptr == (init_str_ptr + 1) ? "" : "s"),
                        (int) (max_strings - init_str_ptr + STRING_OFFSET));
                fprintf(log_file, " %d string characters out of %d\n",
                        (int) (pool_ptr - init_pool_ptr),
                        (int) (pool_size - init_pool_ptr));
                fprintf(log_file, " %d,%d words of node,token memory allocated",
                        (int) var_mem_max, (int) fix_mem_max);
                print_node_mem_stats();
                fprintf(log_file,
                        " %d multiletter control sequences out of %ld+%d\n",
                        (int) cs_count, (long) hash_size, (int) hash_extra);
                fprintf(log_file, " %d font%s using %d bytes\n",
                        (int) max_font_id(), (max_font_id() == 1 ? "" : "s"),
                        (int) font_bytes);
                fprintf(log_file,
                        " %di,%dn,%dp,%db,%ds stack positions out of %di,%dn,%dp,%db,%ds\n",
                        (int) max_in_stack, (int) max_nest_stack,
                        (int) max_param_stack, (int) max_buf_stack,
                        (int) max_save_stack + 6, (int) stack_size,
                        (int) nest_size, (int) param_size, (int) buf_size,
                        (int) save_size);
            }
        }
    }
    wake_up_terminal();
    if (!fixed_pdfoutput_set)   /* else there will be an infinite loop in error case */
        fix_pdfoutput();
    if (fixed_pdfoutput > 0) {
        if (history == fatal_error_stop) {
            remove_pdffile(static_pdf);
            print_err
                (" ==> Fatal error occurred, no output PDF file produced!");
        } else {
            finish_pdf_file(static_pdf, the_luatex_version,
                            get_luatexrevision());
        }
    } else {
        finish_dvi_file(the_luatex_version, get_luatexrevision());
    }
    /* Close {\sl Sync\TeX} file and write status */
    synctex_terminate(log_opened);      /* Let the {\sl Sync\TeX} controller close its files. */

    free_text_codes();
    free_math_codes();
    if (log_opened) {
        wlog_cr();
        selector = selector - 2;
        if ((selector == term_only) && (callback_id == 0)) {
            tprint_nl("Transcript written on ");
            print_file_name(0, texmf_log_name, 0);
            print_char('.');
            print_ln();
        }
        lua_a_close_out(log_file);
    }
}

/*
We get to the |final_cleanup| routine when \.{\\end} or \.{\\dump} has
been scanned and |its_all_over|\kern-2pt.
*/

void final_cleanup(void)
{
    int c;                      /* 0 for \.{\\end}, 1 for \.{\\dump} */
    halfword i;                 /*  for looping marks  */
    c = cur_chr;
    if (job_name == 0)
        open_log_file();
    while (input_ptr > 0)
        if (istate == token_list)
            end_token_list();
        else
            end_file_reading();
    while (open_parens > 0) {
        if (tracefilenames)
            tprint(" )");
        decr(open_parens);
    }
    if (cur_level > level_one) {
        tprint_nl("(\\end occurred inside a group at level ");
        print_int(cur_level - level_one);
        print_char(')');
        show_save_groups();
    }
    while (cond_ptr != null) {
        tprint_nl("(\\end occurred when ");
        print_cmd_chr(if_test_cmd, cur_if);
        if (if_line != 0) {
            tprint(" on line ");
            print_int(if_line);
        }
        tprint(" was incomplete)");
        if_line = if_line_field(cond_ptr);
        cur_if = subtype(cond_ptr);
        temp_ptr = cond_ptr;
        cond_ptr = vlink(cond_ptr);
        flush_node(temp_ptr);
    }
    if (callback_defined(stop_run_callback) == 0)
        if (history != spotless)
            if ((history == warning_issued) || (interaction < error_stop_mode))
                if (selector == term_and_log) {
                    selector = term_only;
                    tprint_nl
                        ("(see the transcript file for additional information)");
                    selector = term_and_log;
                }
    if (c == 1) {
        if (ini_version) {
            for (i = 0; i <= biggest_used_mark; i++) {
                delete_top_mark(i);
                delete_first_mark(i);
                delete_bot_mark(i);
                delete_split_first_mark(i);
                delete_split_bot_mark(i);
            }
            for (c = last_box_code; c <= vsplit_code; c++)
                flush_node_list(disc_ptr[c]);
            if (last_glue != max_halfword)
                delete_glue_ref(last_glue);
            while (pseudo_files != null)
                pseudo_close(); /* flush pseudo files */
            store_fmt_file();
            return;
        }
        tprint_nl("(\\dump is performed only by INITEX)");
        return;
    }
}

void init_prim(void)
{                               /* initialize all the primitives */
    no_new_control_sequence = false;
    first = 0;
    initialize_commands();
    no_new_control_sequence = true;
}

/*
Once \TeX\ is working, you should be able to diagnose most errors with
the \.{\\show} commands and other diagnostic features. But for the initial
stages of debugging, and for the revelation of really deep mysteries, you
can compile \TeX\ with a few more aids, including the \PASCAL\ runtime
checks and its debugger. An additional routine called |debug_help|
will also come into play when you type `\.D' after an error message;
|debug_help| also occurs just before a fatal error causes \TeX\ to succumb.
@^debugging@>
@^system dependencies@>

The interface to |debug_help| is primitive, but it is good enough when used
with a \PASCAL\ debugger that allows you to set breakpoints and to read
variables and change their values. After getting the prompt `\.{debug \#}', you
type either a negative number (this exits |debug_help|), or zero (this
goes to a location where you can set a breakpoint, thereby entering into
dialog with the \PASCAL\ debugger), or a positive number |m| followed by
an argument |n|. The meaning of |m| and |n| will be clear from the
program below. (If |m=13|, there is an additional argument, |l|.)
@.debug \#@>
*/

#ifdef DEBUG
void debug_help(void)
{                               /* routine to display various things */
    integer k;
    int m = 0, n = 0, l = 0;
    while (1) {
        wake_up_terminal();
        tprint_nl("debug # (-1 to exit):");
        update_terminal();
        (void) fscanf(term_in, "%d", &m);
        if (m < 0)
            return;
        else if (m == 0)
            abort();            /* go to every label at least once */
        else {
            (void) fscanf(term_in, "%d", &n);
            switch (m) {
            case 1:
                print_word(varmem[n]);  /* display |varmem[n]| in all forms */
                break;
            case 2:
                print_int(info(n));
                break;
            case 3:
                print_int(link(n));
                break;
            case 4:
                print_word(eqtb[n]);
                break;
            case 6:
                print_word(save_stack[n]);
                break;
            case 7:
                show_box(n);    /* show a box, abbreviated by |show_box_depth| and |show_box_breadth| */
                break;
            case 8:
                breadth_max = 10000;
                depth_threshold = pool_size - pool_ptr - 10;
                show_node_list(n);      /* show a box in its entirety */
                break;
            case 9:
                show_token_list(n, null, 1000);
                break;
            case 10:
                slow_print(n);
                break;
            case 13:
                (void) fscanf(term_in, "%d", &l);
                print_cmd_chr(n, l);
                break;
            case 14:
                for (k = 0; k <= n; k++)
                    print(buffer[k]);
                break;
            case 15:
                font_in_short_display = null_font;
                short_display(n);
                break;
            default:
                tprint("?");
                break;
            }
        }
    }
}
#endif

/*
This section should be replaced, if necessary, by any special
modifications of the program
that are necessary to make \TeX\ work at a particular installation.
It is usually best to design your change file so that all changes to
previous sections preserve the section numbering; then everybody's version
will be consistent with the published program. More extensive changes,
which introduce new sections, can be inserted here; then only the index
itself will get a new section number.
@^system dependencies@>
*/

/*
Here is where you can find all uses of each identifier in the program,
with underlined entries pointing to where the identifier was defined.
If the identifier is only one letter long, however, you get to see only
the underlined entries. {\sl All references are to section numbers instead of
page numbers.}

This index also lists error messages and other aspects of the program
that you might want to look up some day. For example, the entry
for ``system dependencies'' lists all sections that should receive
special attention from people who are installing \TeX\ in a new
operating environment. A list of various things that can't happen appears
under ``this can't happen''. Approximately 40 sections are listed under
``inner loop''; these account for about 60\pct! of \TeX's running time,
exclusive of input and output.
*/
