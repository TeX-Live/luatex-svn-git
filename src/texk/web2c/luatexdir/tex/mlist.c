/* mlist.c

   Copyright 2006-2008 Taco Hoekwater <taco@luatex.org>

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

#include "nodes.h"
#include "commands.h"

static const char _svn_version[] =
    "$Id$ $URL$";

#define delimiter_factor     int_par(param_delimiter_factor_code)
#define delimiter_shortfall  dimen_par(param_delimiter_shortfall_code)
#define bin_op_penalty       int_par(param_bin_op_penalty_code)
#define rel_penalty          int_par(param_rel_penalty_code)
#define null_delimiter_space dimen_par(param_null_delimiter_space_code)
#define script_space         dimen_par(param_script_space_code)
#define disable_lig          int_par(param_disable_lig_code)
#define disable_kern         int_par(param_disable_kern_code)

/* 
In order to convert mlists to hlists, i.e., noads to nodes, we need several
subroutines that are conveniently dealt with now.

Let us first introduce the macros that make it easy to get at the parameters and
other font information. A size code, which is a multiple of 256, is added to a
family number to get an index into the table of internal font numbers
for each combination of family and size.  (Be alert: Size codes get
larger as the type gets smaller.)
*/


void print_size(integer s)
{
    if (s == text_size)
        tprint_esc("textfont");
    else if (s == script_size)
        tprint_esc("scriptfont");
    else
        tprint_esc("scriptscriptfont");
}


/*
  @ When the style changes, the following piece of program computes associated
  information:
*/

#define setup_cur_size_and_mu() do {                                    \
    if (cur_style<script_style)  cur_size=text_size;                    \
    else cur_size=script_size*((cur_style-text_style)/2);               \
    cur_mu=x_over_n(math_quad(cur_size),18);                            \
  } while (0)

/*
  @ Here is a function that returns a pointer to a rule node having a given
  thickness |t|. The rule will extend horizontally to the boundary of the vlist
  that eventually contains it.
*/

pointer fraction_rule(scaled t)
{
    pointer p;                  /* the new node */
    p = new_rule();
    rule_dir(p) = math_direction;
    height(p) = t;
    depth(p) = 0;
    return p;
}

/*
  @ The |overbar| function returns a pointer to a vlist box that consists of
  a given box |b|, above which has been placed a kern of height |k| under a
  fraction rule of thickness |t| under additional space of height |t|.
*/

pointer overbar(pointer b, scaled k, scaled t)
{
    pointer p, q;               /* nodes being constructed */
    p = new_kern(k);
    vlink(p) = b;
    q = fraction_rule(t);
    vlink(q) = p;
    p = new_kern(t);
    vlink(p) = q;
    pack_direction = math_direction;
    return vpackage(p, 0, additional, max_dimen);
}

/*
  Here is a subroutine that creates a new box, whose list contains a
  single character, and whose width includes the italic correction for
  that character. The height or depth of the box will be negative, if
  the height or depth of the character is negative; thus, this routine
  may deliver a slightly different result than |hpack| would produce.
*/

pointer char_box(internal_font_number f, quarterword c)
{
    pointer b, p;               /* the new box and its character node */
    b = new_null_box();
    width(b) = char_width(f, c) + char_italic(f, c);
    height(b) = char_height(f, c);
    depth(b) = char_depth(f, c);
    p = new_char(f, c);
    list_ptr(b) = p;
    return b;
}

/*
  When we build an extensible character, it's handy to have the
  following subroutine, which puts a given character on top
  of the characters already in box |b|:
*/

void stack_into_box(pointer b, internal_font_number f, quarterword c)
{
    pointer p;                  /* new node placed into |b| */
    p = char_box(f, c);
    vlink(p) = list_ptr(b);
    list_ptr(b) = p;
    height(b) = height(p);
}


/*
 Another handy subroutine computes the height plus depth of
 a given character:
*/

scaled height_plus_depth(internal_font_number f, quarterword c)
{
    return (char_height(f, c) + char_depth(f, c));
}


/*
  @ The |var_delimiter| function, which finds or constructs a sufficiently
  large delimiter, is the most interesting of the auxiliary functions that
  currently concern us. Given a pointer |d| to a delimiter field in some noad,
  together with a size code |s| and a vertical distance |v|, this function
  returns a pointer to a box that contains the smallest variant of |d| whose
  height plus depth is |v| or more. (And if no variant is large enough, it
  returns the largest available variant.) In particular, this routine will
  construct arbitrarily large delimiters from extensible components, if
  |d| leads to such characters.
  
  The value returned is a box whose |shift_amount| has been set so that
  the box is vertically centered with respect to the axis in the given size.
  If a built-up symbol is returned, the height of the box before shifting
  will be the height of its topmost component.
*/

pointer var_delimiter(pointer d, integer s, scaled v)
{
    /* label found,continue; */
    pointer b;                  /* the box that will be constructed */
    internal_font_number f, g;  /* best-so-far and tentative font codes */
    integer c, x, y;            /* best-so-far and tentative character codes */
    integer cc;                 /* a temporary character code for extensibles  */
    integer m, n;               /* the number of extensible pieces */
    scaled u;                   /* height-plus-depth of a tentative character */
    scaled w;                   /* largest height-plus-depth so far */
    integer z;                  /* runs through font family members */
    boolean large_attempt;      /* are we trying the ``large'' variant? */
    f = null_font;
    c = 0;
    w = 0;
    large_attempt = false;
    if (d==null)
      goto FOUND; 
    z = small_fam(d);
    x = small_char(d);
    while (true) {
        /* The search process is complicated slightly by the facts that some of the
           characters might not be present in some of the fonts, and they might not
           be probed in increasing order of height. */
        if ((z != 0) || (x != min_quarterword)) {
            z = z + s + script_size;
            do {
                z = z - script_size;
                g = fam_fnt(z);
                if (g != null_font) {
                    y = x;
                    if ((y >= font_bc(g)) && (y <= font_ec(g))) {
                      CONTINUE:
                        if (char_exists(g, y)) {
                            if (char_tag(g, y) == ext_tag) {
                                f = g;
                                c = y;
                                goto FOUND;
                            }
                            u = height_plus_depth(g, y);
                            if (u > w) {
                                f = g;
                                c = y;
                                w = u;
                                if (u >= v)
                                    goto FOUND;
                            }
                            if (char_tag(g, y) == list_tag) {
                                y = char_remainder(g, y);
                                goto CONTINUE;
                            }
                        }
                    }
                }
            } while (z >= script_size);
        }
        if (large_attempt)
            goto FOUND;         /* there were none large enough */
        large_attempt = true;
        z = large_fam(d);
        x = large_char(d);
    }
  FOUND:
    flush_node(d);
    if (f != null_font) {
        /* When the following code is executed, |char_tag(q)| will be equal to
           |ext_tag| if and only if a built-up symbol is supposed to be returned.
         */
        if (char_tag(f, c) == ext_tag) {
            b = new_null_box();
            type(b) = vlist_node;
            /* The width of an extensible character is the width of the repeatable
               module. If this module does not have positive height plus depth,
               we don't use any copies of it, otherwise we use as few as possible
               (in groups of two if there is a middle part).
             */
            cc = ext_rep(f, c);
            u = height_plus_depth(f, cc);
            w = 0;
            width(b) = char_width(f, cc) + char_italic(f, cc);
            cc = ext_bot(f, c);
            if (cc != min_quarterword)
                w = w + height_plus_depth(f, cc);
            cc = ext_mid(f, c);
            if (cc != min_quarterword)
                w = w + height_plus_depth(f, cc);
            cc = ext_top(f, c);
            if (cc != min_quarterword)
                w = w + height_plus_depth(f, cc);
            n = 0;
            if (u > 0) {
                while (w < v) {
                    w = w + u;
                    incr(n);
                    if ((ext_mid(f, c)) != min_quarterword)
                        w = w + u;
                }
            }
            cc = ext_bot(f, c);
            if (cc != min_quarterword)
                stack_into_box(b, f, cc);
            cc = ext_rep(f, c);
            for (m = 1; m <= n; m++)
                stack_into_box(b, f, cc);
            cc = ext_mid(f, c);
            if (cc != min_quarterword) {
                stack_into_box(b, f, cc);
                cc = ext_rep(f, c);
                for (m = 1; m <= n; m++)
                    stack_into_box(b, f, cc);
            }
            cc = ext_top(f, c);
            if (cc != min_quarterword)
                stack_into_box(b, f, cc);
            depth(b) = w - height(b);
        } else {
            b = char_box(f, c);
        }
    } else {
        b = new_null_box();
        width(b) = null_delimiter_space;        /* use this width if no delimiter was found */
    }
    z = cur_size;
    cur_size = s;
    shift_amount(b) = half(height(b) - depth(b)) - axis_height(cur_size);
    cur_size = z;
    return b;
}

/*
The next subroutine is much simpler; it is used for numerators and
denominators of fractions as well as for displayed operators and
their limits above and below. It takes a given box~|b| and
changes it so that the new box is centered in a box of width~|w|.
The centering is done by putting \.{\\hss} glue at the left and right
of the list inside |b|, then packaging the new box; thus, the
actual box might not really be centered, if it already contains
infinite glue.


The given box might contain a single character whose italic correction
has been added to the width of the box; in this case a compensating
kern is inserted.
*/

pointer rebox(pointer b, scaled w)
{
    pointer p, q;               /* temporary registers for list manipulation */
    internal_font_number f;     /* font in a one-character box */
    scaled v;                   /* width of a character without italic correction */

    if ((width(b) != w) && (list_ptr(b) != null)) {
        if (type(b) == vlist_node)
            b = hpack(b, 0, additional);
        p = list_ptr(b);
        if ((is_char_node(p)) && (vlink(p) == null)) {
            f = font(p);
            v = char_width(f, character(p));
            if (v != width(b)) {
                q = new_kern(width(b) - v);
                vlink(p) = q;
            }
        }
        list_ptr(b) = null;
        flush_node(b);
        b = new_glue(ss_glue);
        vlink(b) = p;
        while (vlink(p) != null)
            p = vlink(p);
        q = new_glue(ss_glue);
        vlink(p) = q;
        return hpack(b, w, exactly);
    } else {
        width(b) = w;
        return b;
    }
}

/*
 Here is a subroutine that creates a new glue specification from another
one that is expressed in `\.{mu}', given the value of the math unit.
*/

#define mu_mult(A) mult_and_add(n,(A),xn_over_d((A),f,unity),max_dimen)

pointer math_glue(pointer g, scaled m)
{
    pointer p;                  /* the new glue specification */
    integer n;                  /* integer part of |m| */
    scaled f;                   /* fraction part of |m| */
    n = x_over_n(m, unity);
    f = tex_remainder;
    if (f < 0) {
        decr(n);
        f = f + unity;
    }
    p = new_node(glue_spec_node, 0);
    width(p) = mu_mult(width(g));       /* convert \.{mu} to \.{pt} */
    stretch_order(p) = stretch_order(g);
    if (stretch_order(p) == normal)
        stretch(p) = mu_mult(stretch(g));
    else
        stretch(p) = stretch(g);
    shrink_order(p) = shrink_order(g);
    if (shrink_order(p) == normal)
        shrink(p) = mu_mult(shrink(g));
    else
        shrink(p) = shrink(g);
    return p;
}

/*
The |math_kern| subroutine removes |mu_glue| from a kern node, given
the value of the math unit.
*/

void math_kern(pointer p, scaled m)
{
    integer n;                  /* integer part of |m| */
    scaled f;                   /* fraction part of |m| */
    if (subtype(p) == mu_glue) {
        n = x_over_n(m, unity);
        f = tex_remainder;
        if (f < 0) {
            decr(n);
            f = f + unity;
        }
        width(p) = mu_mult(width(p));
        subtype(p) = explicit;
    }
}

/*
 \TeX's most important routine for dealing with formulas is called
|mlist_to_hlist|.  After a formula has been scanned and represented as an
mlist, this routine converts it to an hlist that can be placed into a box
or incorporated into the text of a paragraph. There are three implicit
parameters, passed in global variables: |cur_mlist| points to the first
node or noad in the given mlist (and it might be |null|); |cur_style| is a
style code; and |mlist_penalties| is |true| if penalty nodes for potential
line breaks are to be inserted into the resulting hlist. After
|mlist_to_hlist| has acted, |vlink(temp_head)| points to the translated hlist.

Since mlists can be inside mlists, the procedure is recursive. And since this
is not part of \TeX's inner loop, the program has been written in a manner
that stresses compactness over efficiency.
@^recursion@>
*/

pointer cur_mlist;              /* beginning of mlist to be translated */
integer cur_style;              /* style code at current place in the list */
integer cur_size;               /* size code corresponding to |cur_style|  */
scaled cur_mu;                  /* the math unit width corresponding to |cur_size| */
boolean mlist_penalties;        /* should |mlist_to_hlist| insert penalties? */

/*
@ The recursion in |mlist_to_hlist| is due primarily to a subroutine
called |clean_box| that puts a given noad field into a box using a given
math style; |mlist_to_hlist| can call |clean_box|, which can call
|mlist_to_hlist|.
@^recursion@>


The box returned by |clean_box| is ``clean'' in the
sense that its |shift_amount| is zero.
*/

pointer clean_box(pointer p, integer s)
{
    pointer q;                  /* beginning of a list to be boxed */
    integer save_style;         /* |cur_style| to be restored */
    pointer x;                  /* box to be returned */
    pointer r;                  /* temporary pointer */
    switch (type(p)) {
    case math_char_node:
        cur_mlist = new_noad();
        r = math_clone(p);
        nucleus(cur_mlist) = r;
        break;
    case sub_box_node:
        q = math_list(p);
        goto FOUND;
        break;
    case sub_mlist_node:
        cur_mlist = math_list(p);
        break;
    default:
        q = new_null_box();
        goto FOUND;
    }
    save_style = cur_style;
    cur_style = s;
    mlist_penalties = false;
    mlist_to_hlist();
    q = vlink(temp_head);       /* recursive call */
    cur_style = save_style;     /* restore the style */
    setup_cur_size_and_mu();
  FOUND:
    if (is_char_node(q) || (q == null))
        x = hpack(q, 0, additional);
    else if ((vlink(q) == null) && (type(q) <= vlist_node)
             && (shift_amount(q) == 0))
        x = q;                  /* it's already clean */
    else
        x = hpack(q, 0, additional);
    /* Here we save memory space in a common case. */
    q = list_ptr(x);
    if (is_char_node(q)) {
        r = vlink(q);
        if (r != null) {
            if (vlink(r) == null) {
                if (!is_char_node(r)) {
                    if (type(r) == kern_node) {
                        /* unneeded italic correction */
                        flush_node(r);
                        vlink(q) = null;
                    }
                }
            }
        }
    }
    return x;
}

/*
 It is convenient to have a procedure that converts a |math_char|
field to an ``unpacked'' form. The |fetch| routine sets |cur_f| and |cur_c|
to the font code and character code of a given noad field. 
It also takes care of issuing error messages for
nonexistent characters; in such cases, |char_exists(cur_f,cur_c)| will be |false|
after |fetch| has acted, and the field will also have been reset to |null|.
*/
/* The outputs of |fetch| are placed in global variables. */

internal_font_number cur_f;     /* the |font| field of a |math_char| */
quarterword cur_c;              /* the |character| field of a |math_char| */

void fetch(pointer a)
{                               /* unpack the |math_char| field |a| */
    cur_c = math_character(a);
    cur_f = fam_fnt(math_fam(a) + cur_size);
    if (cur_f == null_font) {
        int saved_selector;
        str_number s;
        char *msg;
        char *hlp[] = {
            "Somewhere in the math formula just ended, you used the",
            "stated character from an undefined font family. For example,",
            "plain TeX doesn't allow \\it or \\sl in subscripts. Proceed,",
            "and I'll try to forget that I needed that character.",
            NULL
        };
        saved_selector = selector;
        selector = new_string;
        print_size(cur_size);
        print_char(' ');
        print_int(math_fam(a));
        tprint(" is undefined (character ");
        print_char(cur_c);
        print_char(')');
        s = make_string();
        selector = saved_selector;
        msg = makecstring(s);
        tex_error(msg, hlp);
        free(msg);
        flush_node(a);
        a = null;
    } else {
        if (!(char_exists(cur_f, cur_c))) {
            char_warning(cur_f, cur_c);
            flush_node(a);
            a = null;
        }
    }
}


/*
We need to do a lot of different things, so |mlist_to_hlist| makes two
passes over the given mlist.

The first pass does most of the processing: It removes ``mu'' spacing from
glue, it recursively evaluates all subsidiary mlists so that only the
top-level mlist remains to be handled, it puts fractions and square roots
and such things into boxes, it attaches subscripts and superscripts, and
it computes the overall height and depth of the top-level mlist so that
the size of delimiters for a |left_noad| and a |right_noad| will be known.
The hlist resulting from each noad is recorded in that noad's |new_hlist|
field, an integer field that replaces the |nucleus| or |thickness|.
@^recursion@>

The second pass eliminates all noads and inserts the correct glue and
penalties between nodes.
*/

#define assign_new_hlist(A,B) do {                                      \
    new_hlist(A)=B;                                                     \
  } while (0)

#define choose_mlist(A) do { p=A(q); A(q)=null; } while (0)

/*
Most of the actual construction work of |mlist_to_hlist| is done
by procedures with names
like |make_fraction|, |make_radical|, etc. To illustrate
the general setup of such procedures, let's begin with a couple of
simple ones.
*/

void make_over(pointer q)
{
    pointer p;
    p = overbar(clean_box(nucleus(q), cramped_style(cur_style)),
                3 * default_rule_thickness, default_rule_thickness);
    math_list(nucleus(q)) = p;
    type(nucleus(q)) = sub_box_node;
}

void make_under(pointer q)
{
    pointer p, x, y, r;         /* temporary registers for box construction */
    scaled delta;               /* overall height plus depth */
    x = clean_box(nucleus(q), cur_style);
    p = new_kern(3 * default_rule_thickness);
    vlink(x) = p;
    r = fraction_rule(default_rule_thickness);
    vlink(p) = r;
    pack_direction = math_direction;
    y = vpackage(x, 0, additional, max_dimen);
    delta = height(y) + depth(y) + default_rule_thickness;
    height(y) = height(x);
    depth(y) = delta - height(y);
    math_list(nucleus(q)) = y;
    type(nucleus(q)) = sub_box_node;
}

void make_vcenter(pointer q)
{
    pointer v;                  /* the box that should be centered vertically */
    scaled delta;               /* its height plus depth */
    v = math_list(nucleus(q));
    if (type(v) != vlist_node)
        confusion(maketexstring("vcenter"));    /* this can't happen vcenter */
    delta = height(v) + depth(v);
    height(v) = axis_height(cur_size) + half(delta);
    depth(v) = delta - height(v);
}

/*
@ According to the rules in the \.{DVI} file specifications, we ensure alignment
@^square roots@>
between a square root sign and the rule above its nucleus by assuming that the
baseline of the square-root symbol is the same as the bottom of the rule. The
height of the square-root symbol will be the thickness of the rule, and the
depth of the square-root symbol should exceed or equal the height-plus-depth
of the nucleus plus a certain minimum clearance~|clr|. The symbol will be
placed so that the actual clearance is |clr| plus half the excess.
*/

void make_radical(pointer q)
{
    pointer x, y, p;            /* temporary registers for box construction */
    scaled delta, clr;          /* dimensions involved in the calculation */
    x = clean_box(nucleus(q), cramped_style(cur_style));
    if (cur_style < text_style) {       /* display style */
        clr = default_rule_thickness + (abs(math_x_height(cur_size)) / 4);
    } else {
        clr = default_rule_thickness;
        clr = clr + (abs(clr) / 4);
    }
    y = var_delimiter(left_delimiter(q), cur_size, height(x) + depth(x) + clr +
                      default_rule_thickness);
    left_delimiter(q) = null;
    delta = depth(y) - (height(x) + depth(x) + clr);
    if (delta > 0)
        clr = clr + half(delta);        /* increase the actual clearance */
    shift_amount(y) = -(height(x) + clr);
    p = overbar(x, clr, height(y));
    vlink(y) = p;
    p = hpack(y, 0, additional);
    math_list(nucleus(q)) = p;
    type(nucleus(q)) = sub_box_node;
}

/*
Slants are not considered when placing accents in math mode. The accenter is
centered over the accentee, and the accent width is treated as zero with
respect to the size of the final box.
*/

void make_math_accent(pointer q)
{
    pointer p, r, x, y;            /* temporary registers for box construction */
    quarterword c;              /* accent character */
    internal_font_number f;     /* its font */
    scaled s;                   /* amount to skew the accent to the right */
    scaled h;                   /* height of character being accented */
    scaled delta;               /*space to remove between accent and accentee */
    scaled w;                   /* width of the accentee, not including sub/superscripts */
    fetch(accent_chr(q));
    if (char_exists(cur_f, cur_c)) {
        c = cur_c;
        f = cur_f;
        /* Compute the amount of skew */
        s = 0;
        if (type(nucleus(q)) == math_char_node) {
            fetch(nucleus(q));
            s = get_kern(cur_f, cur_c, skew_char(cur_f));
        }
        x = clean_box(nucleus(q), cramped_style(cur_style));
        w = width(x);
        h = height(x);
        /* Switch to a larger accent if available and appropriate */
        while (1) {
            if (char_tag(f, c) != list_tag)
                break;
            y = char_remainder(f, c);
            if (!char_exists(f, y))
                break;
            if (char_width(f, y) > w)
                break;
            c = y;
        }
        if (h < x_height(f))
            delta = h;
        else
            delta = x_height(f);
        if ((supscr(q) != null) || (subscr(q) != null)) {
            if (type(nucleus(q)) == math_char_node) {
                /* Swap the subscript and superscript into box |x| */
                flush_node_list(x);
                x = new_noad();
                r = math_clone(nucleus(q));
                nucleus(x) = r; 
                r = math_clone(supscr(q));
                supscr(x) = r;
                r = math_clone(subscr(q));
                subscr(x) = r; 
                math_reset(supscr(q));
                math_reset(subscr(q));
                type(nucleus(q)) = sub_mlist_node;
                math_list(nucleus(q)) = x;
                x = clean_box(nucleus(q), cur_style);
                delta = delta + height(x) - h;
                h = height(x);
            }
        }
        y = char_box(f, c);
        shift_amount(y) = s + half(w - width(y));
        width(y) = 0;
        p = new_kern(-delta);
        vlink(p) = x;
        vlink(y) = p;
        pack_direction = math_direction;
        y = vpackage(y, 0, additional, max_dimen);
        width(y) = width(x);
        if (height(y) < h) {
            /* Make the height of box |y| equal to |h| */
            p = new_kern(h - height(y));
            vlink(p) = list_ptr(y);
            list_ptr(y) = p;
            height(y) = h;
        }
        math_list(nucleus(q)) = y;
        type(nucleus(q)) = sub_box_node;
    }
    flush_node(accent_chr(q));
    accent_chr(q) = null;
}

/*
The |make_fraction| procedure is a bit different because it sets
|new_hlist(q)| directly rather than making a sub-box.
*/

void make_fraction(pointer q)
{
    pointer p, v, x, y, z;      /* temporary registers for box construction */
    scaled delta, delta1, delta2, shift_up, shift_down, clr;
    /* dimensions for box calculations */
    if (thickness(q) == default_code)
        thickness(q) = default_rule_thickness;
    /* Create equal-width boxes |x| and |z| for the numerator and denominator,
       and compute the default amounts |shift_up| and |shift_down| by which they
       are displaced from the baseline */
    x = clean_box(numerator(q), num_style(cur_style));
    z = clean_box(denominator(q), denom_style(cur_style));
    math_list(numerator(q)) = null;
    math_list(denominator(q)) = null;
    if (width(x) < width(z))
        x = rebox(x, width(z));
    else
        z = rebox(z, width(x));
    if (cur_style < text_style) {       /* display style */
        shift_up = num1(cur_size);
        shift_down = denom1(cur_size);
    } else {
        shift_down = denom2(cur_size);
        shift_up = (thickness(q) != 0 ? num2(cur_size) : num3(cur_size));
    }
    if (thickness(q) == 0) {
        /* The numerator and denominator must be separated by a certain minimum
           clearance, called |clr| in the following program. The difference between
           |clr| and the actual clearance is |2delta|. */
        clr = (cur_style < text_style ? (7 * default_rule_thickness) :  (3 * default_rule_thickness));
        delta = half(clr - ((shift_up - depth(x)) - (height(z) - shift_down)));
        if (delta > 0) {
            shift_up = shift_up + delta;
            shift_down = shift_down + delta;
        }
    } else {
        /* In the case of a fraction line, the minimum clearance depends on the actual
           thickness of the line. */
        clr    = (cur_style < text_style ? (3 * thickness(q)) : thickness(q) );
        delta  = half(thickness(q));
        delta1 = clr - ((shift_up - depth(x)) - (axis_height(cur_size) + delta));
        delta2 = clr - ((axis_height(cur_size) - delta) - (height(z) - shift_down));
        if (delta1 > 0)
            shift_up = shift_up + delta1;
        if (delta2 > 0)
            shift_down = shift_down + delta2;
    }
    /* Construct a vlist box for the fraction, according to |shift_up| and |shift_down| */
    v = new_null_box();
    type(v) = vlist_node;
    height(v) = shift_up + height(x);
    depth(v) = depth(z) + shift_down;
    width(v) = width(x);        /* this also equals |width(z)| */
    if (thickness(q) == 0) {
        p = new_kern((shift_up - depth(x)) - (height(z) - shift_down));
        vlink(p) = z;
    } else {
        y = fraction_rule(thickness(q));
        p = new_kern((axis_height(cur_size) - delta) - (height(z) - shift_down));
        vlink(y) = p;
        vlink(p) = z;
        p = new_kern((shift_up - depth(x)) - (axis_height(cur_size) + delta));
        vlink(p) = y;
    }
    vlink(x) = p;
    list_ptr(v) = x;
    /* Put the fraction into a box with its delimiters, and make |new_hlist(q)|
       point to it */
    delta = (cur_style < text_style ? delim1(cur_size) : delim2(cur_size));
    x = var_delimiter(left_delimiter(q), cur_size, delta);
    left_delimiter(q) = null;
    vlink(x) = v;
    z = var_delimiter(right_delimiter(q), cur_size, delta);
    right_delimiter(q) = null;
    vlink(v) = z;
    y = hpack(x, 0, additional);
    flush_node(numerator(q)); numerator(q) = null;
    flush_node(denominator(q)); denominator(q) = null;
    assign_new_hlist(q,y);
}


/*
If the nucleus of an |op_noad| is a single character, it is to be
centered vertically with respect to the axis, after first being enlarged
(via a character list in the font) if we are in display style.  The normal
convention for placing displayed limits is to put them above and below the
operator in display style.

The italic correction is removed from the character if there is a subscript
and the limits are not being displayed. The |make_op|
routine returns the value that should be used as an offset between
subscript and superscript.

After |make_op| has acted, |subtype(q)| will be |limits| if and only if
the limits have been set above and below the operator. In that case,
|new_hlist(q)| will already contain the desired final box.
*/

scaled make_op(pointer q)
{
    scaled delta;               /* offset between subscript and superscript */
    pointer p, v, x, y, z;      /* temporary registers for box construction */
    quarterword c;              /* register for character examination */
    scaled shift_up, shift_down;        /* dimensions for box calculation */
    if ((subtype(q) == normal) && (cur_style < text_style))
        subtype(q) = limits;
    if (type(nucleus(q)) == math_char_node) {
        fetch(nucleus(q));
        if ((cur_style < text_style) && (char_tag(cur_f, cur_c) == list_tag)) { /* make it larger */
            c = char_remainder(cur_f, cur_c);
            if (char_exists(cur_f, c)) {
                cur_c = c;
                math_character(nucleus(q)) = c;
            }
        }
        delta = char_italic(cur_f, cur_c);
        x = clean_box(nucleus(q), cur_style);
        if ((subscr(q) != null) && (subtype(q) != limits))
            width(x) = width(x) - delta;        /* remove italic correction */
        shift_amount(x) = half(height(x) - depth(x)) - axis_height(cur_size);
        /* center vertically */
        type(nucleus(q)) = sub_box_node;
        math_list(nucleus(q)) = x;
    } else {
        delta = 0;
    }
    if (subtype(q) == limits) {
        /* The following program builds a vlist box |v| for displayed limits. The
           width of the box is not affected by the fact that the limits may be skewed. */
        x = clean_box(supscr(q), sup_style(cur_style));
        y = clean_box(nucleus(q), cur_style);
        z = clean_box(subscr(q), sub_style(cur_style));
        v = new_null_box();
        type(v) = vlist_node;
        width(v) = width(y);
        if (width(x) > width(v))
            width(v) = width(x);
        if (width(z) > width(v))
            width(v) = width(z);
        x = rebox(x, width(v));
        y = rebox(y, width(v));
        z = rebox(z, width(v));
        shift_amount(x) = half(delta);
        shift_amount(z) = -shift_amount(x);
        height(v) = height(y);
        depth(v) = depth(y);
        /* Attach the limits to |y| and adjust |height(v)|, |depth(v)| to
           account for their presence */
        /* We use |shift_up| and |shift_down| in the following program for the
           amount of glue between the displayed operator |y| and its limits |x| and
           |z|. The vlist inside box |v| will consist of |x| followed by |y| followed
           by |z|, with kern nodes for the spaces between and around them. */
        if (supscr(q) == null) {
            list_ptr(x) = null;
            flush_node(x);
            list_ptr(v) = y;
        } else {
            shift_up = big_op_spacing3 - depth(x);
            if (shift_up < big_op_spacing1)
                shift_up = big_op_spacing1;
            p = new_kern(shift_up);
            vlink(p) = y;
            vlink(x) = p;
            p = new_kern(big_op_spacing5);
            vlink(p) = x;
            list_ptr(v) = p;
            height(v) =
                height(v) + big_op_spacing5 + height(x) + depth(x) + shift_up;
        }
        if (subscr(q) == null) {
            list_ptr(z) = null;
            flush_node(z);
        } else {
            shift_down = big_op_spacing4 - height(z);
            if (shift_down < big_op_spacing2)
                shift_down = big_op_spacing2;
            p = new_kern(shift_down);
            vlink(y) = p;
            vlink(p) = z;
            p = new_kern(big_op_spacing5);
            vlink(z) = p;
            depth(v) =
                depth(v) + big_op_spacing5 + height(z) + depth(z) + shift_down;
        }
        assign_new_hlist(q,v);
    }
    return delta;
}

/*
A ligature found in a math formula does not create a ligature, because
there is no question of hyphenation afterwards; the ligature will simply be
stored in an ordinary |glyph_node|, after residing in an |ord_noad|.

The |type| is converted to |math_text_char| here if we would not want to
apply an italic correction to the current character unless it belongs
to a math font (i.e., a font with |space=0|).

No boundary characters enter into these ligatures.
*/

void make_ord(pointer q)
{
    integer a;                  /* the left-side character for lig/kern testing */
    pointer p, r, s;            /* temporary registers for list manipulation */
    scaled k;                   /* a kern */
    liginfo lig;                /* a ligature */
  RESTART:
    if (subscr(q) == null && 
        supscr(q) == null && 
        type(nucleus(q)) == math_char_node) {
        p = vlink(q);
        if ((p != null) &&
            (type(p) >= ord_noad) && (type(p) <= punct_noad) &&
            (type(nucleus(p)) == math_char_node) &&
            (math_fam(nucleus(p)) == math_fam(nucleus(q)))) {
            type(nucleus(q)) = math_text_char_node;
            fetch(nucleus(q));
            a = cur_c;
            if ((has_kern(cur_f, a)) || (has_lig(cur_f, a))) {
                cur_c = math_character(nucleus(p));
                /* If character |a| has a kern with |cur_c|, attach
                   the kern after~|q|; or if it has a ligature with |cur_c|, combine
                   noads |q| and~|p| appropriately; then |return| if the cursor has
                   moved past a noad, or |goto restart| */

                /* Note that a ligature between an |ord_noad| and another kind of noad
                   is replaced by an |ord_noad|, when the two noads collapse into one.
                   But we could make a parenthesis (say) change shape when it follows
                   certain letters. Presumably a font designer will define such
                   ligatures only when this convention makes sense. */

                if (disable_lig == 0 && has_lig(cur_f, a)) {
                    lig = get_ligature(cur_f, a, cur_c);
                    if (is_valid_ligature(lig)) {
                        check_interrupt();      /* allow a way out of infinite ligature loop */
                        switch (lig_type(lig)) {
                        case 1:
                        case 5:
                            math_character(nucleus(q)) = lig_replacement(lig);  /* \.{=:|}, \.{=:|>} */
                            break;
                        case 2:
                        case 6:
                            math_character(nucleus(p)) = lig_replacement(lig);  /* \.{|=:}, \.{|=:>} */
                            break;
                        case 3:
                        case 7:
                        case 11:
                            r = new_noad();     /* \.{|=:|}, \.{|=:|>}, \.{|=:|>>} */
                            s = new_node(math_char_node,0);
                            nucleus(r) = s ; 
                            math_character(nucleus(r)) = lig_replacement(lig);
                            math_fam(nucleus(r)) = math_fam(nucleus(q));
                            vlink(q) = r;
                            vlink(r) = p;
                            if (lig_type(lig) < 11)
                                type(nucleus(r)) = math_char_node;
                            else
                                type(nucleus(r)) = math_text_char_node; /* prevent combination */
                            break;
                        default:
                            vlink(q) = vlink(p);
                            math_character(nucleus(q)) = lig_replacement(lig);  /* \.{=:} */
                            s = math_clone(subscr(p));
                            subscr(q) = s;
                            s = math_clone(supscr(p));
                            supscr(q) = s;
                            math_reset(subscr(p));      /* just in case */
                            math_reset(supscr(p));
                            flush_node(p);
                            break;
                        }
                        if (lig_type(lig) > 3)
                            return;
                        type(nucleus(q)) = math_char_node;
                        goto RESTART;
                    }
                }
                if (disable_kern == 0 && has_kern(cur_f, a)) {
                    k = get_kern(cur_f, a, cur_c);
                    if (k != 0) {
                        p = new_kern(k);
                        vlink(p) = vlink(q);
                        vlink(q) = p;
                        return;
                    }
                }
            }
        }
    }
}


/*
The purpose of |make_scripts(q,delta)| is to attach the subscript and/or
superscript of noad |q| to the list that starts at |new_hlist(q)|,
given that subscript and superscript aren't both empty. The superscript
will appear to the right of the subscript by a given distance |delta|.

We set |shift_down| and |shift_up| to the minimum amounts to shift the
baseline of subscripts and superscripts based on the given nucleus.
*/

void make_scripts(pointer q, scaled delta)
{
    pointer p, x, y, z;         /* temporary registers for box construction */
    scaled shift_up, shift_down, clr;   /* dimensions in the calculation */
    integer t;                  /* subsidiary size code */
    p = new_hlist(q);
    if (is_char_node(p)) {
        shift_up = 0;
        shift_down = 0;
    } else {
        z = hpack(p, 0, additional);
        t = cur_size;
        if (cur_style < script_style)
            cur_size = script_size;
        else
            cur_size = script_script_size;
        shift_up = height(z) - sup_drop(cur_size);
        shift_down = depth(z) + sub_drop(cur_size);
        cur_size = t;
        list_ptr(z) = null;
        flush_node(z);
    }
    if (supscr(q) == null) {
        /* Construct a subscript box |x| when there is no superscript */
        /* When there is a subscript without a superscript, the top of the subscript
           should not exceed the baseline plus four-fifths of the x-height. */
        x = clean_box(subscr(q), sub_style(cur_style));
        width(x) = width(x) + script_space;
        if (shift_down < sub1(cur_size))
            shift_down = sub1(cur_size);
        clr = height(x) - (abs(math_x_height(cur_size) * 4) / 5);
        if (shift_down < clr)
            shift_down = clr;
        shift_amount(x) = shift_down;
    } else {
        /* Construct a superscript box |x| */
        /*The bottom of a superscript should never descend below the baseline plus
           one-fourth of the x-height. */
        x = clean_box(supscr(q), sup_style(cur_style));
        width(x) = width(x) + script_space;
        if (odd(cur_style))
            clr = sup3(cur_size);
        else if (cur_style < text_style)
            clr = sup1(cur_size);
        else
            clr = sup2(cur_size);
        if (shift_up < clr)
            shift_up = clr;
        clr = depth(x) + (abs(math_x_height(cur_size)) / 4);
        if (shift_up < clr)
            shift_up = clr;

        if (subscr(q) == null) {
            shift_amount(x) = -shift_up;
        } else {
            /* Construct a sub/superscript combination box |x|, with the
               superscript offset by |delta| */
            /* When both subscript and superscript are present, the subscript must be
               separated from the superscript by at least four times |default_rule_thickness|.
               If this condition would be violated, the subscript moves down, after which
               both subscript and superscript move up so that the bottom of the superscript
               is at least as high as the baseline plus four-fifths of the x-height. */
            y = clean_box(subscr(q), sub_style(cur_style));
            width(y) = width(y) + script_space;
            if (shift_down < sub2(cur_size))
                shift_down = sub2(cur_size);
            clr =
                4 * default_rule_thickness - ((shift_up - depth(x)) -
                                              (height(y) - shift_down));
            if (clr > 0) {
                shift_down = shift_down + clr;
                clr =
                    (abs(math_x_height(cur_size) * 4) / 5) - (shift_up -
                                                              depth(x));
                if (clr > 0) {
                    shift_up = shift_up + clr;
                    shift_down = shift_down - clr;
                }
            }
            shift_amount(x) = delta;    /* superscript is |delta| to the right of the subscript */
            p = new_kern((shift_up - depth(x)) - (height(y) - shift_down));
            vlink(x) = p;
            vlink(p) = y;
            pack_direction = math_direction;
            x = vpackage(x, 0, additional, max_dimen);
            shift_amount(x) = shift_down;
        }
    }
    if (new_hlist(q) == null) {
        new_hlist(q) = x;
    } else {
        p = new_hlist(q);
        while (vlink(p) != null)
            p = vlink(p);
        vlink(p) = x;
    }
}

/* 
The |make_left_right| function constructs a left or right delimiter of
the required size and returns the value |open_noad| or |close_noad|. The
|right_noad| and |left_noad| will both be based on the original |style|,
so they will have consistent sizes.

We use the fact that |right_noad-left_noad=close_noad-open_noad|.
*/

small_number make_left_right(pointer q, integer style, scaled max_d,
                             scaled max_hv)
{
    scaled delta, delta1, delta2;       /* dimensions used in the calculation */
    pointer tmp;
    cur_style = style;
    setup_cur_size_and_mu();
    delta2 = max_d + axis_height(cur_size);
    delta1 = max_hv + max_d - delta2;
    if (delta2 > delta1)
        delta1 = delta2;        /* |delta1| is max distance from axis */
    delta = (delta1 / 500) * delimiter_factor;
    delta2 = delta1 + delta1 - delimiter_shortfall;
    if (delta < delta2)
        delta = delta2;
    tmp = var_delimiter(delimiter(q), cur_size, delta);
    delimiter(q) = null;
    assign_new_hlist(q,tmp);
    return (type(q) - (left_noad - open_noad)); /* |open_noad| or |close_noad| */
}


/*
The inter-element spacing in math formulas depends on a $8\times8$ table that
\TeX\ preloads as a 64-digit string. The elements of this string have the
following significance:
$$\vbox{\halign{#\hfil\cr
\.0 means no space;\cr
\.1 means a conditional thin space (\.{\\nonscript\\mskip\\thinmuskip});\cr
\.2 means a thin space (\.{\\mskip\\thinmuskip});\cr
\.3 means a conditional medium space
  (\.{\\nonscript\\mskip\\medmuskip});\cr
\.4 means a conditional thick space
  (\.{\\nonscript\\mskip\\thickmuskip});\cr
\.* means an impossible case.\cr}}$$
This is all pretty cryptic, but {\sl The \TeX book\/} explains what is
supposed to happen, and the string makes it happen.
@:TeXbook}{\sl The \TeX book@>
*/

char math_spacing[] =
    "0234000122*4000133**3**344*0400400*000000234000111*1111112341011";


/* Here is the overall plan of |mlist_to_hlist|, and the list of its
   local variables.
*/

void mlist_to_hlist(void)
{
    pointer mlist;              /* beginning of the given list */
    boolean penalties;          /* should penalty nodes be inserted? */
    integer style;              /* the given style */
    integer save_style;         /* holds |cur_style| during recursion */
    pointer q;                  /* runs through the mlist */
    pointer r;                  /* the most recent noad preceding |q| */
    integer r_type;             /* the |type| of noad |r|, or |op_noad| if |r=null| */
    integer t;                  /* the effective |type| of noad |q| during the second pass */
    pointer p, x, y, z;         /* temporary registers for list construction */
    integer pen;                /* a penalty to be inserted */
    integer s;                  /* the size of a noad to be deleted */
    scaled max_hl, max_d;       /* maximum height and depth of the list translated so far */
    scaled delta;               /* offset between subscript and superscript */
    mlist = cur_mlist;
    penalties = mlist_penalties;
    style = cur_style;          /* tuck global parameters away as local variables */
    q = mlist;
    r = null;
    r_type = op_noad;
    max_hl = 0;
    max_d = 0;
    x = null;
    p = null;
    setup_cur_size_and_mu();
    while (q != null) {
        /* We use the fact that no character nodes appear in an mlist, hence
           the field |type(q)| is always present. */

        /* One of the things we must do on the first pass is change a |bin_noad| to
           an |ord_noad| if the |bin_noad| is not in the context of a binary operator.
           The values of |r| and |r_type| make this fairly easy. */
      RESWITCH:
        delta = 0;
        switch (type(q)) {
        case bin_noad:
            switch (r_type) {
            case bin_noad:
            case op_noad:
            case rel_noad:
            case open_noad:
            case punct_noad:
            case left_noad:
                type(q) = ord_noad;
                goto RESWITCH;
            }
            break;
        case rel_noad:
        case close_noad:
        case punct_noad:
        case right_noad:
            if (r_type == bin_noad)
                type(r) = ord_noad;
            if (type(q) == right_noad)
                goto DONE_WITH_NOAD;
            break;
        case left_noad:
            goto DONE_WITH_NOAD;
            break;
        case fraction_noad:
            make_fraction(q);
            goto CHECK_DIMENSIONS;
            break;
        case op_noad:
            delta = make_op(q);
            if (subtype(q) == limits)
                goto CHECK_DIMENSIONS;
            break;
        case ord_noad:
            make_ord(q);
            break;
        case open_noad:
        case inner_noad:
            break;
        case radical_noad:
            make_radical(q);
            break;
        case over_noad:
            make_over(q);
            break;
        case under_noad:
            make_under(q);
            break;
        case accent_noad:
            make_math_accent(q);
            break;
        case vcenter_noad:
            make_vcenter(q);
            break;
        case style_node:
            cur_style = subtype(q);
            setup_cur_size_and_mu();
            goto DONE_WITH_NODE;
            break;
        case choice_node:
            switch (cur_style / 2) {
            case 0:
                choose_mlist(display_mlist);
                break;          /* |display_style=0| */
            case 1:
                choose_mlist(text_mlist);
                break;          /* |text_style=2| */
            case 2:
                choose_mlist(script_mlist);
                break;          /* |script_style=4| */
            case 3:
                choose_mlist(script_script_mlist);
                break;          /* |script_script_style=6| */
            }                   /* there are no other cases */
            flush_node_list(display_mlist(q));
            flush_node_list(text_mlist(q));
            flush_node_list(script_mlist(q));
            flush_node_list(script_script_mlist(q));
            type(q) = style_node;
            subtype(q) = cur_style;
            if (p != null) {
                z = vlink(q);
                vlink(q) = p;
                while (vlink(p) != null)
                    p = vlink(p);
                vlink(p) = z;
            }
            goto DONE_WITH_NODE;
            break;
        case ins_node:
        case mark_node:
        case adjust_node:
        case whatsit_node:
        case penalty_node:
        case disc_node:
            goto DONE_WITH_NODE;
            break;
        case rule_node:
            if (height(q) > max_hl)
                max_hl = height(q);
            if (depth(q) > max_d)
                max_d = depth(q);
            goto DONE_WITH_NODE;
            break;
        case glue_node:
            /*
               Conditional math glue (`\.{\\nonscript}') results in a |glue_node|
               pointing to |zero_glue|, with |subtype(q)=cond_math_glue|; in such a case
               the node following will be eliminated if it is a glue or kern node and if the
               current size is different from |text_size|. Unconditional math glue
               (`\.{\\muskip}') is converted to normal glue by multiplying the dimensions
               by |cur_mu|.
               @:non_script_}{\.{\\nonscript} primitive@>
             */
            if (subtype(q) == mu_glue) {
                x = glue_ptr(q);
                y = math_glue(x, cur_mu);
                delete_glue_ref(x);
                glue_ptr(q) = y;
                subtype(q) = normal;
            } else if ((cur_size != text_size)
                       && (subtype(q) == cond_math_glue)) {
                p = vlink(q);
                if (p != null)
                    if ((type(p) == glue_node) || (type(p) == kern_node)) {
                        vlink(q) = vlink(p);
                        vlink(p) = null;
                        flush_node_list(p);
                    }
            }
            goto DONE_WITH_NODE;
            break;
        case kern_node:
            math_kern(q, cur_mu);
            goto DONE_WITH_NODE;
            break;
        default:
            tconfusion("mlist1");       /* this can't happen mlist1 */
        }
        /* When we get to the following part of the program, we have ``fallen through''
           from cases that did not lead to |check_dimensions| or |done_with_noad| or
           |done_with_node|. Thus, |q|~points to a noad whose nucleus may need to be
           converted to an hlist, and whose subscripts and superscripts need to be
           appended if they are present.

           If |nucleus(q)| is not a |math_char|, the variable |delta| is the amount
           by which a superscript should be moved right with respect to a subscript
           when both are present.
           @^subscripts@>
           @^superscripts@>
         */
        switch (type(nucleus(q))) {
        case math_char_node:
        case math_text_char_node:
            fetch(nucleus(q));
            if (char_exists(cur_f, cur_c)) {
                delta = char_italic(cur_f, cur_c);
                p = new_glyph(cur_f, cur_c);
                if ((type(nucleus(q)) == math_text_char_node)
                    && (space(cur_f) != 0))
                    delta = 0;  /* no italic correction in mid-word of text font */
                if ((subscr(q) == null) && (delta != 0)) {
                    x = new_kern(delta);
                    vlink(p) = x;
                    delta = 0;
                }
            } else {
                p = null;
            }
            break;
        case sub_box_node:
            p = math_list(nucleus(q));
            break;
        case sub_mlist_node:
            cur_mlist = math_list(nucleus(q));
            save_style = cur_style;
            mlist_penalties = false;
            mlist_to_hlist();   /* recursive call */
            cur_style = save_style;
            setup_cur_size_and_mu();
            p = hpack(vlink(temp_head), 0, additional);
            break;
        default:
            tconfusion("mlist2"); /* this can't happen mlist2 */
        }
        assign_new_hlist(q, p);
        if ((subscr(q) == null) && (supscr(q) == null))
            goto CHECK_DIMENSIONS;
        make_scripts(q, delta);

      CHECK_DIMENSIONS:
        z = hpack(new_hlist(q), 0, additional);
        if (height(z) > max_hl)
            max_hl = height(z);
        if (depth(z) > max_d)
            max_d = depth(z);
        list_ptr(z) = null;
        flush_node(z); /* only drop the \hbox */
      DONE_WITH_NOAD:
        r = q;
        r_type = type(r);
        if (r_type == right_noad) {
            r_type = left_noad;
            cur_style = style;
            setup_cur_size_and_mu();
        }
      DONE_WITH_NODE:
        q = vlink(q);
    }
    if (r_type == bin_noad)
        type(r) = ord_noad;
    /* Make a second pass over the mlist, removing all noads and inserting the
       proper spacing and penalties */

    /* We have now tied up all the loose ends of the first pass of |mlist_to_hlist|.
       The second pass simply goes through and hooks everything together with the
       proper glue and penalties. It also handles the |left_noad| and |right_noad| that
       might be present, since |max_hl| and |max_d| are now known. Variable |p| points
       to a node at the current end of the final hlist.
     */
    p = temp_head;
    vlink(p) = null;
    q = mlist;
    r_type = 0;
    cur_style = style;
    setup_cur_size_and_mu();
  NEXT_NODE:
    while (q != null) {
        /* If node |q| is a style node, change the style and |goto delete_q|;
           otherwise if it is not a noad, put it into the hlist,
           advance |q|, and |goto done|; otherwise set |s| to the size
           of noad |q|, set |t| to the associated type (|ord_noad..
           inner_noad|), and set |pen| to the associated penalty */
        /* Just before doing the big |case| switch in the second pass, the program
           sets up default values so that most of the branches are short. */
        t = ord_noad;
        s = noad_size;
        pen = inf_penalty;
        switch (type(q)) {
        case op_noad:
        case open_noad:
        case close_noad:
        case punct_noad:
        case inner_noad:
            t = type(q);
            break;
        case bin_noad:
            t = bin_noad;
            pen = bin_op_penalty;
            break;
        case rel_noad:
            t = rel_noad;
            pen = rel_penalty;
            break;
        case ord_noad:
        case vcenter_noad:
        case over_noad:
        case under_noad:
            break;
        case radical_noad:
            s = radical_noad_size;
            break;
        case accent_noad:
            s = accent_noad_size;
            break;
        case fraction_noad:
            t = inner_noad;
            s = fraction_noad_size;
            break;
        case left_noad:
        case right_noad:
            t = make_left_right(q, style, max_d, max_hl);
            break;
        case style_node:
            /* Change the current style and |goto delete_q| */
            cur_style = subtype(q);
            s = style_node_size;
            setup_cur_size_and_mu();
            goto DELETE_Q;
            break;
        case whatsit_node:
        case penalty_node:
        case rule_node:
        case disc_node:
        case adjust_node:
        case ins_node:
        case mark_node:
        case glue_node:
        case kern_node:
            vlink(p) = q;
            p = q;
            q = vlink(q);
            vlink(p) = null;
            goto NEXT_NODE;
            break;
        default:
            confusion(maketexstring("mlist3")); /* this can't happen mlist3 */
        }
        /* Append inter-element spacing based on |r_type| and |t| */
        if (r_type > 0) {       /* not the first noad */
            switch (math_spacing[r_type * 8 + t - 9 * ord_noad]) {
            case '0':
                x = 0;
                break;
            case '1':
                x = (cur_style < script_style ? param_thin_mu_skip_code : 0);
                break;
            case '2':
                x = param_thin_mu_skip_code;
                break;
            case '3':
                x = (cur_style < script_style ? param_med_mu_skip_code : 0);
                break;
            case '4':
                x = (cur_style < script_style ? param_thick_mu_skip_code : 0);
                break;
            default:
                confusion(maketexstring("mlist4"));     /* this can't happen mlist4 */
                break;
            }
            if (x != 0) {
                y = math_glue(glue_par(x), cur_mu);
                z = new_glue(y);
                glue_ref_count(y) = null;
                vlink(p) = z;
                p = z;
                subtype(z) = x + 1;     /* store a symbolic subtype */
            }
        }

        /* Append any |new_hlist| entries for |q|, and any appropriate penalties */
        /* We insert a penalty node after the hlist entries of noad |q| if |pen|
           is not an ``infinite'' penalty, and if the node immediately following |q|
           is not a penalty node or a |rel_noad| or absent entirely. */

        if (new_hlist(q) != null) {
            vlink(p) = new_hlist(q);
            do {
                p = vlink(p);
            } while (vlink(p) != null);
        }
        if (penalties && vlink(q) != null && pen < inf_penalty) {
            r_type = type(vlink(q));
            if (r_type != penalty_node && r_type != rel_noad) {
                z = new_penalty(pen);
                vlink(p) = z;
                p = z;
            }
        }
        if (type(q) == right_noad)
            t = open_noad;
        r_type = t;
      DELETE_Q:
        r = q;
        q = vlink(q);
        /* The m-to-hlist conversion takes place in-place, 
           so the various dependant fields may not be freed 
           (as would happen if |flush_node| was called).
           A low-level |free_node| is easier than attempting
           to nullify such dependant fields for all possible
           node and noad types.
         */
        if (nodetype_has_attributes(type(r)))
          delete_attribute_ref(node_attr(r));
        free_node(r, get_node_size(type(r), subtype(r)));
    }
}
