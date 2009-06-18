/* stringpool.c
   
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

static const char _svn_version[] =
    "$Id$ $URL$";

/*
Control sequence names and diagnostic messages are variable-length strings
of eight-bit characters. Since \PASCAL\ did not have a well-developed string
mechanism, \TeX\ did all of its string processing by homegrown methods.

Elaborate facilities for dynamic strings are not needed, so all of the
necessary operations can be handled with a simple data structure.
The array |str_pool| contains all of the (eight-bit) bytes off all
of the strings, and the array |str_start| contains indices of the starting
points of each string. Strings are referred to by integer numbers, so that
string number |s| comprises the characters |str_pool[j]| for
|str_start_macro(s)<=j<str_start_macro(s+1)|. Additional integer variables
|pool_ptr| and |str_ptr| indicate the number of entries used so far
in |str_pool| and |str_start|, respectively; locations
|str_pool[pool_ptr]| and |str_start_macro(str_ptr)| are
ready for the next string to be allocated.

String numbers 0 to |biggest_char| are reserved for strings that correspond to 
single UNICODE characters. This is in accordance with the conventions of \.{WEB}
which converts single-character strings into the ASCII code number of the
single character involved, while it converts other strings into integers
and builds a string pool file. Thus, when the string constant \.{"."} appears
in the \PASCAL\ program, \.{WEB} converts it into the integer 46, which is the
ASCII code for a period, while \.{WEB} will convert a string like \.{"hello"}
into some integer greater than~|STRING_OFFSET|.
*/

packed_ASCII_code *str_pool;    /* the characters */
pool_pointer *str_start;        /* the starting pointers */
pool_pointer pool_ptr;          /* first unused position in |str_pool| */
str_number str_ptr;             /* number of the current string being created */
pool_pointer init_pool_ptr;     /* the starting value of |pool_ptr| */
str_number init_str_ptr;        /* the starting value of |str_ptr| */


/*
Once a sequence of characters has been appended to |str_pool|, it
officially becomes a string when the function |make_string| is called.
This function returns the identification number of the new string as its
value.
*/


/* current string enters the pool */
str_number make_string(void)
{
    if (str_ptr == (max_strings + STRING_OFFSET))
        overflow(maketexstring("number of strings"),
                 max_strings - init_str_ptr);
    incr(str_ptr);
    str_start_macro(str_ptr) = pool_ptr;
    return (str_ptr - 1);
}


/*
|buffer_to_unichar| converts a sequence of bytes in the |buffer|
into a unicode character value. It does not check for overflow
of the |buffer|.
*/

#define test_sequence_byte(A) do {					\
    if (((A)<0x80) || ((A)>=0xC0)) {					\
      deletions_allowed=false;						\
      tex_error("Text line contains an invalid UTF-8 sequence", hlp);	\
      deletions_allowed=true;						\
      return 0xFFFD;							\
    }									\
  } while (0)


static integer buffer_to_unichar(integer k)
{
    integer a;                  /* a utf char */
    integer b;                  /* a utf nibble */
    char *hlp[] = {
        "A funny symbol that I can't read has just been input.",
        "Just continue, I'll change it to 0xFFFD.",
        NULL
    };
    b = buffer[k];
    if (b < 0x80) {
        a = b;
    } else if (b >= 0xF8) {
        /* the 5- and 6-byte UTF-8 sequences generate integers 
           that are outside of the valid UCS range, and therefore
           unsupported 
         */
        test_sequence_byte(-1);
    } else if (b >= 0xF0) {
        a = (b - 0xF0) * 64;
        b = buffer[k + 1];
        test_sequence_byte(b);
        a = (a + (b - 128)) * 64;
        b = buffer[k + 2];
        test_sequence_byte(b);
        a = (a + (b - 128)) * 64;
        b = buffer[k + 3];
        test_sequence_byte(b);
        a = a + (b - 128);
    } else if (b >= 0xE0) {
        a = (b - 0xE0) * 64;
        b = buffer[k + 1];
        test_sequence_byte(b);
        a = (a + (b - 128)) * 64;
        b = buffer[k + 2];
        test_sequence_byte(b);
        a = a + (b - 128);
    } else if (b >= 0xC0) {
        a = (b - 0xC0) * 64;
        b = buffer[k + 1];
        test_sequence_byte(b);
        a = a + (b - 128);
    } else {
        /* This is an encoding error */
        test_sequence_byte(-1);
    }
    return a;
}


/*
The following subroutine compares string |s| with another string of the
same length that appears in |buffer| starting at position |k|;
the result is |true| if and only if the strings are equal.
Empirical tests indicate that |str_eq_buf| is used in such a way that
it tends to return |true| about 80 percent of the time.
*/

boolean str_eq_buf(str_number s, integer k)
{                               /* test equality of strings */
    integer a;                  /* a unicode character */
    if (s < STRING_OFFSET) {
        a = buffer_to_unichar(k);
        if (a != s)
            return false;
    } else {
        pool_pointer j = str_start_macro(s);
        while (j < str_start_macro(s + 1)) {
            if (str_pool[j] != buffer[k])
                return false;
            incr(j);
            incr(k);
        }
    }
    return true;
}

/*
Here is a similar routine, but it compares two strings in the string pool,
and it does not assume that they have the same length.
*/

boolean str_eq_str(str_number s, str_number t)
{                               /* test equality of strings */
    integer a = 0;              /* a utf char */
    if (s < STRING_OFFSET) {
        if (t >= STRING_OFFSET) {
            if (s <= 0x7F && (str_length(t) == 1)
                && str_pool[str_start_macro(t)] == s)
                return true;
            a = pool_to_unichar(str_start_macro(t));
            if (a != s)
                return false;
        } else {
            if (t != s)
                return false;
        }
    } else if (t < STRING_OFFSET) {
        if (t <= 0x7F && (str_length(s) == 1)
            && str_pool[str_start_macro(s)] == t)
            return true;
        a = pool_to_unichar(str_start_macro(s));
        if (a != t)
            return false;
    } else {
        pool_pointer j, k;      /* running indices */
        if (str_length(s) != str_length(t))
            return false;
        j = str_start_macro(s);
        k = str_start_macro(t);
        while (j < str_start_macro(s + 1)) {
            if (str_pool[j++] != str_pool[k++])
                return false;
        }
    }
    return true;
}

/*
The initial values of |str_pool|, |str_start|, |pool_ptr|,
and |str_ptr| are computed by the \.{INITEX} program, based in part
on the information that \.{WEB} has output while processing \TeX.

The first |string_offset| strings are single-characters strings matching
Unicode. There is no point in generating all of these. But |str_ptr| has
initialized properly, otherwise |print_char| cannot see the difference
between characters and strings.
*/




/* initializes the string pool, but returns |false| if something goes wrong */
boolean get_strings_started(void)
{
    str_number g;               /* garbage */
    pool_ptr = 0;
    str_ptr = STRING_OFFSET;
    str_start[0] = 0;
    /* Read the other strings from the \.{TEX.POOL} file and return |true|,
       or give an error message and return |false| */
    g = loadpoolstrings((pool_size - string_vacancies));
    if (g == 0) {
        fprintf(stdout, "! You have to increase POOLSIZE.\n");
        return false;
    }
    return true;
}

/* Declare additional routines for string recycling */
/* from the change file */

/* The string recycling routines.  \TeX{} uses 2
   upto 4 {\it new\/} strings when scanning a filename in an \.{\\input},
   \.{\\openin}, or \.{\\openout} operation.  These strings are normally
   lost because the reference to them are not saved after finishing the
   operation.  |search_string| searches through the string pool for the
   given string and returns either 0 or the found string number.
*/

str_number search_string(str_number search)
{
    str_number s;               /* running index */
    integer len;                /* length of searched string */
    len = str_length(search);
    if (len == 0) {
        return get_nullstr();
    } else {
        s = search - 1;         /* start search with newest string below |s|; |search>1|! */
        while (s >= STRING_OFFSET) {
            /* first |string_offset| strings depend on implementation!! */
            if (str_length(s) == len)
                if (str_eq_str(s, search))
                    return s;
            s--;
        }
    }
    return 0;
}

/*
The following routine is a variant of |make_string|.  It searches
the whole string pool for a string equal to the string currently built
and returns a found string.  Otherwise a new string is created and
returned.  Be cautious, you can not apply |flush_string| to a replaced
string!
*/

str_number slow_make_string(void)
{
    str_number s;               /* result of |search_string| */
    str_number t;               /* new string */
    t = make_string();
    s = search_string(t);
    if (s > 0) {
        flush_string();
        return s;
    }
    return t;
}
