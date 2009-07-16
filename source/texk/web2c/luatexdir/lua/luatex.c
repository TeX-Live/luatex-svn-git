/* luatex.c
   
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

#include "lua/luatex-api.h"
#include <ptexlib.h>
#include <zlib.h>

static const char _svn_version[] =
    "$Id$ $URL$";


/* Read and write dump files through zlib */

/* Earlier versions recast *f from FILE * to gzFile, but there is
 * no guarantee that these have the same size, so a static variable 
 * is needed.
 */

static gzFile gz_fmtfile = NULL;

void do_zdump(char *p, int item_size, int nitems, FILE * out_file)
{
    int err;
    (void) out_file;
    if (nitems == 0)
        return;
    if (gzwrite(gz_fmtfile, (void *) p, item_size * nitems) !=
        item_size * nitems) {
        fprintf(stderr, "! Could not write %d %d-byte item(s): %s.\n", nitems,
                item_size, gzerror(gz_fmtfile, &err));
        uexit(1);
    }
}

void do_zundump(char *p, int item_size, int nitems, FILE * in_file)
{
    int err;
    (void) in_file;
    if (nitems == 0)
        return;
    if (gzread(gz_fmtfile, (void *) p, item_size * nitems) <= 0) {
        fprintf(stderr, "Could not undump %d %d-byte item(s): %s.\n",
                nitems, item_size, gzerror(gz_fmtfile, &err));
        uexit(1);
    }
}

#define COMPRESSION "R3"

boolean zopen_w_input(FILE ** f, int format, const_string fopen_mode)
{
    int callbackid;
    int res;
    char *fnam;
    callbackid = callback_defined(find_format_file_callback);
    if (callbackid > 0) {
        res = run_callback(callbackid, "S->S", (nameoffile + 1), &fnam);
        if (res && fnam && strlen(fnam) > 0) {
            xfree(nameoffile);
            nameoffile = xmalloc(strlen(fnam) + 2);
            memcpy((nameoffile + 1), fnam, strlen(fnam));
            *(nameoffile + strlen(fnam) + 1) = 0;
            *f = xfopen(fnam, fopen_mode);
            if (*f == NULL) {
                return 0;
            }
        } else {
            return 0;
        }
    } else {
        res = open_input(f, format, fopen_mode);
    }
    if (res) {
        gz_fmtfile = gzdopen(fileno(*f), "rb" COMPRESSION);
    }
    return res;
}

boolean zopen_w_output(FILE ** f, const_string fopen_mode)
{
    int res = 1;
    if (luainit) {
        *f = fopen((const_string) (nameoffile + 1), fopen_mode);
        if (*f == NULL) {
            return 0;
        }
    } else {
        res = open_output(f, fopen_mode);
    }
    if (res) {
        gz_fmtfile = gzdopen(fileno(*f), "wb" COMPRESSION);
    }
    return res;
}

void zwclose(FILE * f)
{
    (void) f;
    gzclose(gz_fmtfile);
}

/* create the dvi or pdf file */

int open_outfile(FILE ** f, char *name, char *mode)
{
    FILE *res;
    res = fopen(name, mode);
    if (res != NULL) {
        *f = res;
        return 1;
    }
    return 0;
}


/* the caller sets tfm_buffer=NULL and tfm_size=0 */

int readbinfile(FILE * f, unsigned char **tfm_buffer, integer * tfm_size)
{
    void *buf;
    int size;
    if (fseek(f, 0, SEEK_END) == 0) {
        size = ftell(f);
        if (size > 0) {
            buf = xmalloc(size);
            if (fseek(f, 0, SEEK_SET) == 0) {
                if (fread((void *) buf, size, 1, f) == 1) {
                    *tfm_buffer = (unsigned char *) buf;
                    *tfm_size = (integer) size;
                    return 1;
                }
            }
        }
    }                           /* seek failed, or zero-sized file */
    return 0;
}
