% vfpacket.w

% Copyright 1996-2006 Han The Thanh <thanh@@pdftex.org>
% Copyright 2006-2011 Taco Hoekwater <taco@@luatex.org>

% This file is part of LuaTeX.

% LuaTeX is free software; you can redistribute it and/or modify it under
% the terms of the GNU General Public License as published by the Free
% Software Foundation; either version 2 of the License, or (at your
% option) any later version.

% LuaTeX is distributed in the hope that it will be useful, but WITHOUT
% ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
% FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
% License for more details.

% You should have received a copy of the GNU General Public License along
% with LuaTeX; if not, see <http://www.gnu.org/licenses/>.

@ @c
static const char _svn_version[] =
    "$Id$ "
    "$URL$";

#include "ptexlib.h"

@ The |do_vf_packet| procedure is called in order to interpret the
  character packet for a virtual character. Such a packet may contain
  the instruction to typeset a character from the same or an other
  virtual font; in such cases |do_vf_packet| calls itself
  recursively. The recursion level, i.e., the number of times this has
  happened, is kept in the global variable |packet_cur_s| and should
  not exceed |packet_max_recursion|.

@c
#define packet_stack_size 100   /* stack array size */
#define packet_max_recursion 100        /* maximum |do_vf_packet()| recursion level */

typedef struct packet_stack_record_ {
    float c0;
    float c1;
    float c2;
    float c3;
    scaledpos pos;              /* c4, c5 */
} packet_stack_record;

int packet_cur_s = 0;           /* current |do_vf_packet()| recursion level */
static packet_stack_record packet_stack[packet_stack_size];
int packet_stack_ptr = 0;       /* pointer into |packet_stack| */

@ Some macros for processing character packets.
@c
#define packet_number(fw) {    \
    fw = *(vfp++);             \
    fw = fw * 256 + *(vfp++);  \
    fw = fw * 256 + *(vfp++);  \
    fw = fw * 256 + *(vfp++);  \
}

#define packet_scaled(a, fs) {  \
    int fw;                     \
    fw = *(vfp++);              \
    if (fw > 127)               \
        fw = fw - 256;          \
    fw = fw * 256 + *(vfp++);   \
    fw = fw * 256 + *(vfp++);   \
    fw = fw * 256 + *(vfp++);   \
    a = store_scaled_f(fw, fs); \
}

@ count the number of bytes in a command packet
@c
int vf_packet_bytes(charinfo * co)
{
    eight_bits *vf_packets, *vfp;
    unsigned k;
    int cmd;

    vfp = vf_packets = get_charinfo_packets(co);
    if (vf_packets == NULL) {
        return 0;
    }
    while ((cmd = *(vfp++)) != packet_end_code) {
        switch (cmd) {
        case packet_nop_code:
        case packet_pop_code:
        case packet_push_code:
            break;
        case packet_char_code:
        case packet_down_code:
        case packet_font_code:
        case packet_image_code:
        case packet_node_code:
        case packet_right_code:
            vfp += 4;
            break;
        case packet_rule_code:
            vfp += 8;
            break;
        case packet_special_code:
            packet_number(k);   /* +4 */
            vfp += (int) k;
            break;
        default:
            pdf_error("vf", "invalid DVI command (1)");
        }
    };
    return (vfp - vf_packets);
}

@ typeset the \.{DVI} commands in the
   character packet for character |c| in current font |f|
@c
const char *packet_command_names[] = {
    "char", "font", "pop", "push", "special", "image",
    "right", "down", "rule", "node", "nop", "end", "scale", "lua", NULL
};

@ @c
static float packet_float(eight_bits ** vfpp)
{
    unsigned int i;
    union U {
        float a;
        eight_bits b[sizeof(float)];
    } u;
    eight_bits *vfp = *vfpp;
    for (i = 0; i < sizeof(float); i++)
        u.b[i] = *(vfp++);
    *vfpp = vfp;
    return u.a;
}

void do_vf_packet(PDF pdf, internal_font_number vf_f, int c)
{
    int cmd, fs_f, save_stack_ptr = packet_stack_ptr;
    unsigned k;
    internal_font_number lf;
    charinfo *co;
    scaledpos size;
    packet_stack_record cur_mat;
    eight_bits *vf_packets, *vfp;
    scaled i;
    str_number s;
    float f;

    posstructure localpos;      /* the position structure local within this function */
    posstructure *refpos;       /* the list origin pos. on the page, provided by the caller */

    co = get_charinfo(vf_f, c);
    vfp = vf_packets = get_charinfo_packets(co);
    if (vf_packets == NULL)
        return;

    lf = 0;                     /* for -Wall */
    packet_cur_s++;
    if (packet_cur_s >= packet_max_recursion)
        overflow("max level recursion of virtual fonts", packet_max_recursion);

    cur_mat.c0 = 1.0;
    cur_mat.c1 = 0.0;
    cur_mat.c2 = 0.0;
    cur_mat.c3 = 1.0;
    cur_mat.pos.h = 0;
    cur_mat.pos.v = 0;

    refpos = pdf->posstruct;
    pdf->posstruct = &localpos; /* use local structure for recursion */
    localpos.pos = refpos->pos;
    localpos.dir = dir_TLT;     /* invariably for vf */

    fs_f = font_size(vf_f);
    while ((cmd = *(vfp++)) != packet_end_code) {
#ifdef DEBUG
        if (cmd > packet_end_code) {
            fprintf(stdout, "do_vf_packet(%i,%i) command code = illegal \n",
                    vf_f, c);
        } else {
            fprintf(stdout, "do_vf_packet(%i,%i) command code = %s\n", vf_f, c,
                    packet_command_names[cmd]);
        }
#endif
        switch (cmd) {
        case packet_font_code:
            packet_number(lf);
            break;
        case packet_push_code:
            packet_stack[packet_stack_ptr] = cur_mat;
            packet_stack_ptr++;
            if (packet_stack_ptr == packet_stack_size)
                pdf_error("vf", "pdf_stack_ptr overflow");
            break;
        case packet_pop_code:
            if (packet_stack_ptr == save_stack_ptr)
                pdf_error("vf", "pdf_stack_ptr underflow");
            packet_stack_ptr--;
            cur_mat = packet_stack[packet_stack_ptr];
            break;
        case packet_char_code:
            packet_number(k);
            if (!char_exists(lf, (int) k)) {
                char_warning(lf, (int) k);
            } else {
                if (has_packet(lf, (int) k))
                    do_vf_packet(pdf, lf, (int) k);
                else
                    backend_out[glyph_node] (pdf, lf, (int) k);
            }
            cur_mat.pos.h = cur_mat.pos.h + char_width(lf, (int) k);
            break;
        case packet_rule_code:
            packet_scaled(size.v, fs_f);        /* height (where is depth?) */
            packet_scaled(size.h, fs_f);
            if (size.h > 0 && size.v > 0)
                pdf_place_rule(pdf, 0, size);   /* the 0 is unused */
            cur_mat.pos.h = cur_mat.pos.h + size.h;
            break;
        case packet_right_code:
            packet_scaled(i, fs_f);
            cur_mat.pos.h = cur_mat.pos.h + i;
            break;
        case packet_down_code:
            packet_scaled(i, fs_f);
            cur_mat.pos.v = cur_mat.pos.v + i;
            break;
        case packet_special_code:
            packet_number(k);
            str_room(k);
            while (k > 0) {
                k--;
                append_char(*(vfp++));
            }
            s = make_string();
            pdf_literal(pdf, s, scan_special, false);
            flush_str(s);
            break;
        case packet_lua_code:
            packet_number(k);
            if (luaL_loadbuffer
                (Luas, (const char *) vfp, (size_t) k, "packet_lua_code")
                || lua_pcall(Luas, 0, LUA_MULTRET, 0))
                lua_error(Luas);
            vfp += k;
            break;
        case packet_image_code:
            packet_number(k);
            vf_out_image(pdf, k);
            break;
        case packet_node_code:
            packet_number(k);
            hlist_out(pdf, (halfword) k);
            break;
        case packet_nop_code:
            break;
        case packet_scale_code:
            f = packet_float(&vfp);
            cur_mat.c0 = cur_mat.c0 * f;
            cur_mat.c3 = cur_mat.c3 * f;
            /* pdf->pstruct->scale = f; *//* scale is still NOP */
            pdf->pstruct->need_tm = true;
            pdf->pstruct->need_tf = true;
            break;
        default:
            pdf_error("vf", "invalid DVI command (2)");
        }
        synch_pos_with_cur(&localpos, refpos, cur_mat.pos);     /* trivial case, always TLT */
    }
    pdf->posstruct = refpos;
    packet_cur_s--;
    packet_stack_ptr = save_stack_ptr;
}

@ @c
int *packet_local_fonts(internal_font_number f, int *num)
{
    int c, cmd, lf, k, l, i;
    int localfonts[256] = { 0 };
    int *lfs;
    charinfo *co;

    eight_bits *vf_packets, *vfp;
    k = 0;
    for (c = font_bc(f); c <= font_ec(f); c++) {
        if (quick_char_exists(f, c)) {
            co = get_charinfo(f, c);
            vfp = vf_packets = get_charinfo_packets(co);
            if (vf_packets == NULL)
                continue;
            while ((cmd = *(vfp++)) != packet_end_code) {
                switch (cmd) {
                case packet_font_code:
                    packet_number(lf);
                    for (l = 0; l < k; l++) {
                        if (localfonts[l] == lf) {
                            break;
                        }
                    }
                    if (l == k) {
                        localfonts[k++] = lf;
                    }
                    break;
                case packet_nop_code:
                case packet_pop_code:
                case packet_push_code:
                    break;
                case packet_char_code:
                case packet_down_code:
                case packet_image_code:
                case packet_node_code:
                case packet_right_code:
                    vfp += 4;
                    break;
                case packet_rule_code:
                    vfp += 8;
                    break;
                case packet_special_code:
                    packet_number(i);
                    vfp += i;
                    break;
                default:
                    pdf_error("vf", "invalid DVI command (3)");
                }
            }
        }
    }
    *num = k;
    if (k > 0) {
        lfs = xmalloc((unsigned) ((unsigned) k * sizeof(int)));
        memcpy(lfs, localfonts, (size_t) ((unsigned) k * sizeof(int)));
        return lfs;
    }
    return NULL;
}

@ @c
void
replace_packet_fonts(internal_font_number f, int *old_fontid,
                     int *new_fontid, int count)
{
    int c, cmd, lf, k, l;
    charinfo *co;
    eight_bits *vf_packets, *vfp;

    for (c = font_bc(f); c <= font_ec(f); c++) {
        if (quick_char_exists(f, c)) {
            co = get_charinfo(f, c);
            vfp = vf_packets = get_charinfo_packets(co);
            if (vf_packets == NULL)
                continue;
            while ((cmd = *(vfp++)) != packet_end_code) {
                switch (cmd) {
                case packet_font_code:
                    packet_number(lf);
                    for (l = 0; l < count; l++) {
                        if (old_fontid[l] == lf) {
                            break;
                        }
                    }
                    if (l < count) {
                        k = new_fontid[l];
                        *(vfp - 4) = (eight_bits)
                            ((k & 0xFF000000) >> 24);
                        *(vfp - 3) = (eight_bits)
                            ((k & 0x00FF0000) >> 16);
                        *(vfp - 2) = (eight_bits)
                            ((k & 0x0000FF00) >> 8);
                        *(vfp - 1) = (eight_bits) (k & 0x000000FF);
                    }
                    break;
                case packet_nop_code:
                case packet_pop_code:
                case packet_push_code:
                    break;
                case packet_char_code:
                case packet_down_code:
                case packet_image_code:
                case packet_node_code:
                case packet_right_code:
                    vfp += 4;
                    break;
                case packet_rule_code:
                    vfp += 8;
                    break;
                case packet_special_code:
                    packet_number(k);
                    vfp += k;
                    break;
                default:
                    pdf_error("vf", "invalid DVI command (4)");
                }
            }
        }
    }
}
