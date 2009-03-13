/* primitive.c

   Copyright 2008-2009 Taco Hoekwater <taco@luatex.org>

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
#include "primitive.h"
#include "tokens.h"

static const char _svn_version[] =
    "$Id$ $URL$";

/* \primitive support needs a few extra variables and definitions */

#define prim_base 1
#define level_one 1
#define flush_string() do { decr(str_ptr); pool_ptr=str_start_macro(str_ptr); } while (0)
#define cur_length (pool_ptr - str_start_macro(str_ptr))
#define append_char(a) str_pool[pool_ptr++]=(a)

#define next(a) hash[(a)].lhfield       /* link for coalesced lists */
#define text(a) hash[(a)].rh    /* string number for control sequence name */
#define hash_is_full (hash_used==hash_base)     /* test if all positions are occupied */
#define hash_size 65536

#define prim_next(a) prim[(a)].lhfield  /* link for coalesced lists */
#define prim_text(a) prim[(a)].rh       /* string number for control sequence name */
#define prim_is_full (prim_used==prim_base)     /* test if all positions are occupied */
#define prim_eq_level_field(a) (a).hh.b1
#define prim_eq_type_field(a)  (a).hh.b0
#define prim_equiv_field(a) (a).hh.rh
#define prim_eq_level(a) prim_eq_level_field(prim_eqtb[(a)])    /* level of definition */
#define prim_eq_type(a) prim_eq_type_field(prim_eqtb[(a)])      /* command code for equivalent */
#define prim_equiv(a) prim_equiv_field(prim_eqtb[(a)])  /* equivalent value */

static two_halves prim[(prim_size + 1)];        /* the primitives table */
static pointer prim_used;       /* allocation pointer for |prim| */
static memory_word prim_eqtb[(prim_size + 1)];

/* initialize the memory arrays */


void init_primitives(void)
{
    int k;
    memset(prim, 0, (sizeof(two_halves) * (prim_size + 1)));
    memset(prim_eqtb, 0, (sizeof(memory_word) * (prim_size + 1)));
    for (k = 0; k <= prim_size; k++)
        prim_eq_type(k) = undefined_cs_cmd;
}

void ini_init_primitives(void)
{
    prim_used = prim_size;      /* nothing is used */
}

/* The value of |hash_prime| should be roughly 85\pct! of |hash_size|, and it
   should be a prime number.  The theory of hashing tells us to expect fewer
   than two table probes, on the average, when the search is successful.
   [See J.~S. Vitter, {\sl Journal of the ACM\/ \bf30} (1983), 231--258.]
   @^Vitter, Jeffrey Scott@>
*/

halfword compute_hash(char *j, pool_pointer l, halfword prime_number)
{
    pool_pointer k;
    halfword h = (unsigned char) *j;
    for (k = 1; k <= l - 1; k++) {
        h = h + h + (unsigned char) *(j + k);
        while (h >= prime_number)
            h = h - prime_number;
    }
    return h;
}


/*  Here is the subroutine that searches the primitive table for an identifier */

pointer prim_lookup(str_number s)
{                               /* search the primitives table */
    integer h;                  /* hash code */
    pointer p;                  /* index in |hash| array */
    pool_pointer j, l;
    if (s < string_offset) {
        p = s;
        if ((p < 0) || (get_prim_eq_type(p) == undefined_cs_cmd)) {
            p = undefined_primitive;
        }
    } else {
        j = str_start_macro(s);
        if (s == str_ptr)
            l = cur_length;
        else
            l = length(s);
        h = compute_hash((char *) (str_pool + j), l, prim_prime);
        p = h + prim_base;      /* we start searching here; note that |0<=h<hash_prime| */
        while (1) {
            if (prim_text(p) > 0)
                if (length(prim_text(p)) == l)
                    if (str_eq_str(prim_text(p), s))
                        goto FOUND;
            if (prim_next(p) == 0) {
                if (no_new_control_sequence) {
                    p = undefined_primitive;
                } else {
                    /* Insert a new primitive after |p|, then make |p| point to it */
                    if (prim_text(p) > 0) {
                        do {    /* search for an empty location in |prim| */
                            if (prim_is_full)
                                overflow_string("primitive size", prim_size);
                            decr(prim_used);
                        } while (prim_text(prim_used) != 0);
                        prim_next(p) = prim_used;
                        p = prim_used;
                    }
                    prim_text(p) = s;
                }
                goto FOUND;
            }
            p = prim_next(p);
        }
    }
  FOUND:
    return p;
}

/* how to test a csname for primitive-ness */

boolean is_primitive(str_number csname)
{
    integer n, m;
    m = prim_lookup(csname);
    n = string_lookup(csname);
    return ((n != undefined_cs_cmd) &&
            (m != undefined_primitive) &&
            (eq_type(n) == prim_eq_type(m)) && (equiv(n) == prim_equiv(m)));
}


/* a few simple accessors */

quarterword get_prim_eq_type(integer p)
{
    return prim_eq_type(p);
}

halfword get_prim_equiv(integer p)
{
    return prim_equiv(p);
}

str_number get_prim_text(integer p)
{
    return prim_text(p);
}


/* dumping and undumping */

void dump_primitives(void)
{
    int p;
    for (p = 0; p <= prim_size; p++)
        dump_hh(prim[p]);
    for (p = 0; p <= prim_size; p++)
        dump_wd(prim_eqtb[p]);
}

void undump_primitives(void)
{
    int p;
    for (p = 0; p <= prim_size; p++)
        undump_hh(prim[p]);
    for (p = 0; p <= prim_size; p++)
        undump_wd(prim_eqtb[p]);

}

/* 
   We need to put \TeX's ``primitive'' control sequences into the hash
   table, together with their command code (which will be the |eq_type|)
   and an operand (which will be the |equiv|). The |primitive| procedure
   does this, in a way that no \TeX\ user can. The global value |cur_val|
   contains the new |eqtb| pointer after |primitive| has acted.
*/

void primitive(str_number ss, quarterword c, halfword o, int cmd_origin)
{
    pool_pointer k;             /* index into |str_pool| */
    str_number s;               /* actual |str_number| used */
    int j;                      /* index into |buffer| */
    small_number l;             /* length of the string */
    integer prim_val;           /* needed to fill |prim_eqtb| */

    if (ss < string_offset) {
        if (ss > 127)
            tconfusion("prim"); /* should be ASCII */
        append_char(ss);
        s = make_string();
    } else {
        s = ss;
    }
    k = str_start_macro(s);
    l = str_start_macro(s + 1) - k;
    /* ee will move |s| into the (possibly non-empty) |buffer| */
    if (first + l > buf_size + 1)
        check_buffer_overflow(first + l);
    for (j = 0; j <= l - 1; j++)
        buffer[first + j] = str_pool[k + j];
    cur_val = id_lookup(first, l);      /* |no_new_control_sequence| is |false| */
    flush_string();
    text(cur_val) = s;          /* we don't want to have the string twice */
    prim_val = prim_lookup(s);
    eq_level(cur_val) = level_one;
    eq_type(cur_val) = c;
    equiv(cur_val) = o;
    prim_eq_level(prim_val) = level_one;
    prim_eq_type(prim_val) = c;
    prim_equiv(prim_val) = o;
}


/* 
 * Here is a helper that does the actual hash insertion.
 */

halfword insert_id(halfword p, unsigned char *j, pool_pointer l)
{
    integer d;
    unsigned char *k;
    if (text(p) > 0) {
        if (hash_high < hash_extra) {
            incr(hash_high);
            next(p) = hash_high + get_eqtb_size();
            p = hash_high + get_eqtb_size();
        } else {
            do {
                if (hash_is_full)
                    overflow_string("hash size", hash_size + hash_extra);
                decr(hash_used);
            } while (text(hash_used) != 0);     /* search for an empty location in |hash| */
            next(p) = hash_used;
            p = hash_used;
        }
    }
    check_pool_overflow((pool_ptr + l));
    d = cur_length;
    while (pool_ptr > str_start_macro(str_ptr)) {
        /* move current string up to make room for another */
        decr(pool_ptr);
        str_pool[pool_ptr + l] = str_pool[pool_ptr];
    }
    for (k = j; k <= j + l - 1; k++)
        append_char(*k);
    text(p) = make_string();
    pool_ptr = pool_ptr + d;
    incr(cs_count);
    return p;
}


/*
 Here is the subroutine that searches the hash table for an identifier
 that matches a given string of length |l>1| appearing in |buffer[j..
 (j+l-1)]|. If the identifier is found, the corresponding hash table address
 is returned. Otherwise, if the global variable |no_new_control_sequence|
 is |true|, the dummy address |undefined_control_sequence| is returned.
 Otherwise the identifier is inserted into the hash table and its location
 is returned.
*/


pointer id_lookup(integer j, integer l)
{                               /* search the hash table */
    integer h;                  /* hash code */
    pointer p;                  /* index in |hash| array */

    h = compute_hash((char *) (buffer + j), l, hash_prime);
    p = h + hash_base;          /* we start searching here; note that |0<=h<hash_prime| */
    while (1) {
        if (text(p) > 0)
            if (length(text(p)) == l)
                if (str_eq_buf(text(p), j))
                    goto FOUND;
        if (next(p) == 0) {
            if (no_new_control_sequence) {
                p = get_undefined_control_sequence();
            } else {
                p = insert_id(p, (buffer + j), l);
            }
            goto FOUND;
        }
        p = next(p);
    }
  FOUND:
    return p;
}

/*
 * Finding a primitive in the hash, based on a finished string.
 */


pointer string_lookup(str_number s)
{                               /* search the hash table */
    integer h;                  /* hash code */
    pointer p;                  /* index in |hash| array */
    pool_pointer l;
    if (s < string_offset) {
        if (no_new_control_sequence) {
            p = get_undefined_control_sequence();
        } else {
            if (s <= 0x7F)
                l = 1;
            else if (s <= 0x7FF)
                l = 2;
            else if (s <= 0xFFFF)
                l = 3;
            else
                l = 4;
            p = insert_id(p, (str_pool + str_start_macro(s)), l);
        }
    } else {
        pool_pointer j;
        j = str_start_macro(s);
        if (s == str_ptr)
            l = cur_length;
        else
            l = length(s);
        h = compute_hash((char *) (str_pool + j), l, hash_prime);
        p = h + hash_base;      /* we start searching here; note that |0<=h<hash_prime| */
        while (1) {
            if (text(p) > 0)
                if (length(text(p)) == l)
                    if (str_eq_str(text(p), s))
                        goto FOUND;
            if (next(p) == 0) {
                if (no_new_control_sequence) {
                    p = get_undefined_control_sequence();
                } else {
                    p = insert_id(p, (str_pool + str_start_macro(s)), l);
                }
                goto FOUND;
            }
            p = next(p);
        }
    }
  FOUND:
    return p;
}
