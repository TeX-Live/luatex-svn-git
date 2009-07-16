/* managed-sa.c
   
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


#include <ptexlib.h>

static const char __svn_version[] =
    "$Id$ $URL$";

static void store_sa_stack(sa_tree a, integer n, integer v, integer gl)
{
    sa_stack_item st;
    st.code = n;
    st.value = v;
    st.level = gl;
    if (a->stack == NULL) {
        a->stack = Mxmalloc_array(sa_stack_item, a->stack_size);
    } else if (((a->stack_ptr) + 1) >= a->stack_size) {
        a->stack_size += a->stack_step;
        a->stack = Mxrealloc_array(a->stack, sa_stack_item, a->stack_size);
    }
    (a->stack_ptr)++;
    a->stack[a->stack_ptr] = st;
}

static void skip_in_stack(sa_tree a, integer n)
{
    int p = a->stack_ptr;
    if (a->stack == NULL)
        return;
    while (p > 0) {
        if (a->stack[p].code == n && a->stack[p].level > 0) {
            a->stack[p].level = -(a->stack[p].level);
        }
        p--;
    }
}

sa_tree_item get_sa_item(const sa_tree head, const integer n)
{
    register int h;
    register int m;
    if (head->tree != NULL) {
        h = HIGHPART_PART(n);
        if (head->tree[h] != NULL) {
            m = MIDPART_PART(n);
            if (head->tree[h][m] != NULL) {
                return head->tree[h][m][LOWPART_PART(n)];
            }
        }
    }
    return head->dflt;
}

void set_sa_item(sa_tree head, integer n, sa_tree_item v, integer gl)
{
    int h, m, l;
    int i;
    h = HIGHPART_PART(n);
    m = MIDPART_PART(n);
    l = LOWPART_PART(n);
    if (head->tree == NULL) {
        head->tree =
            (sa_tree_item ***) Mxcalloc_array(sa_tree_item **, HIGHPART);
    } 
    if (head->tree[h] == NULL) {
        head->tree[h] =
            (sa_tree_item **) Mxcalloc_array(sa_tree_item *, MIDPART);
    }
    if (head->tree[h][m] == NULL) {
        head->tree[h][m] =
            (sa_tree_item *) Mxmalloc_array(sa_tree_item, LOWPART);
        for (i = 0; i < LOWPART; i++) {
            head->tree[h][m][i] = head->dflt;
        }
    }
    if (gl <= 1) {
        skip_in_stack(head, n);
    } else {
        store_sa_stack(head, n, head->tree[h][m][l], gl);
    }
    head->tree[h][m][l] = v;
}

void rawset_sa_item(sa_tree head, integer n, integer v)
{
    head->tree[HIGHPART_PART(n)][MIDPART_PART(n)][LOWPART_PART(n)] = v;
}

void clear_sa_stack(sa_tree a)
{
    if (a->stack != NULL) {
        Mxfree(a->stack);
    }
    a->stack = NULL;
    a->stack_ptr = 0;
    a->stack_size = a->stack_step;
}

void destroy_sa_tree(sa_tree a)
{
    int h, m;
    if (a == NULL)
        return;
    if (a->tree != NULL) {
        for (h = 0; h < HIGHPART; h++) {
            if (a->tree[h] != NULL) {
                for (m = 0; m < MIDPART; m++) {
                    if (a->tree[h][m] != NULL) {
                        Mxfree(a->tree[h][m]);
                    }
                }
                Mxfree(a->tree[h]);
            }
        }
        Mxfree(a->tree);
    }
    if (a->stack != NULL) {
        Mxfree(a->stack);
    }
    Mxfree(a);
}


sa_tree copy_sa_tree(sa_tree b)
{
    int h, m;
    sa_tree a = (sa_tree) Mxmalloc_array(sa_tree_head, 1);
    a->stack_step = b->stack_step;
    a->stack_size = b->stack_size;
    a->dflt = b->dflt;
    a->stack = NULL;
    a->stack_ptr = 0;
    a->tree = NULL;
    if (b->tree != NULL) {
        a->tree = (sa_tree_item ***) Mxcalloc_array(void *, HIGHPART);
        for (h = 0; h < HIGHPART; h++) {
            if (b->tree[h] != NULL) {
                a->tree[h] = (sa_tree_item **) Mxcalloc_array(void *, MIDPART);
                for (m = 0; m < MIDPART; m++) {
                    if (b->tree[h][m] != NULL) {
                        a->tree[h][m] = Mxmalloc_array(sa_tree_item, LOWPART);
                        memcpy(a->tree[h][m],b->tree[h][m],sizeof(sa_tree_item)*LOWPART);
                    }
                }
            }
        }
    }
    return a;
}

/* The main reason to fill in the lowest entry branches here immediately
is that most of the sparse arrays have a bias toward ASCII values. 

Allocating those here immediately improves the chance of the structure
|a->tree[0][0][x]| being close together in actual memory locations 
*/

sa_tree new_sa_tree(integer size, sa_tree_item dflt)
{
    sa_tree_head *a;
    a = (sa_tree_head *) xmalloc(sizeof(sa_tree_head));
    a->dflt = dflt;
    a->stack = NULL;
    a->tree = (sa_tree_item ***) Mxcalloc_array(sa_tree_item **, HIGHPART);
    a->tree[0] = (sa_tree_item **) Mxcalloc_array(sa_tree_item *, MIDPART);
    a->stack_size = size;
    a->stack_step = size;
    a->stack_ptr = 0;
    return (sa_tree) a;
}

void restore_sa_stack(sa_tree head, integer gl)
{
    sa_stack_item st;
    if (head->stack == NULL)
        return;
    while (head->stack_ptr > 0 && abs(head->stack[head->stack_ptr].level) >= gl) {
        st = head->stack[head->stack_ptr];
        if (st.level > 0) {
            rawset_sa_item(head, st.code, st.value);
        }
        (head->stack_ptr)--;
    }
}


void dump_sa_tree(sa_tree a)
{
    boolean f;
    unsigned int x;
    int h, m, l;
    assert(a != NULL);
    dump_int(a->stack_step);
    dump_int(a->dflt);
    if (a->tree != NULL) {
        dump_int(1);
        for (h = 0; h < HIGHPART; h++) {
            if (a->tree[h] != NULL) {
                f = 1;
                dump_qqqq(f);
                for (m = 0; m < MIDPART; m++) {
                    if (a->tree[h][m] != NULL) {
                        f = 1;
                        dump_qqqq(f);
                        for (l = 0; l < LOWPART; l++) {
                            x = a->tree[h][m][l];
                            dump_int(x);
                        }
                    } else {
                        f = 0;
                        dump_qqqq(f);
                    }
                }
            } else {
                f = 0;
                dump_qqqq(f);
            }
        }
    } else {
        dump_int(0);
    }
}


sa_tree undump_sa_tree(void)
{
    integer x;
    int h, m, l;
    boolean f;
    sa_tree a = (sa_tree) Mxmalloc_array(sa_tree_head, 1);
    undump_int(x);
    a->stack_step = x;
    a->stack_size = x;
    undump_int(x);
    a->dflt = x;
    a->stack = Mxmalloc_array(sa_stack_item, a->stack_size);
    a->stack_ptr = 0;
    a->tree = NULL;
    undump_int(x);
    if (x == 0)
        return a;
    a->tree = (sa_tree_item ***) Mxcalloc_array(void *, HIGHPART);
    for (h = 0; h < HIGHPART; h++) {
        undump_qqqq(f);
        if (f > 0) {
            a->tree[h] = (sa_tree_item **) Mxcalloc_array(void *, MIDPART);
            for (m = 0; m < MIDPART; m++) {
                undump_qqqq(f);
                if (f > 0) {
                    a->tree[h][m] = Mxmalloc_array(sa_tree_item, LOWPART);
                    for (l = 0; l < LOWPART; l++) {
                        undump_int(x);
                        a->tree[h][m][l] = (unsigned int) x;
                    }
                }
            }
        }
    }
    return a;
}
