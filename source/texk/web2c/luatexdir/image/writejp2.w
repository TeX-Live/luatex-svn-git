% writejp2.w

% Copyright 2011 Taco Hoekwater <taco@@luatex.org>
% Copyright 2011 Hartmut Henkel <hartmut@@luatex.org>

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

@ Basic JPEG~2000 image support. Section and Table references below:
Information technology --- JPEG~2000 image coding system: Core coding system.
ISO/IEC 15444-1, Second edition, 2004-09-15, file |15444-1-annexi.pdf|.

@c
#include <assert.h>
#include "ptexlib.h"
#include "image/image.h"
#include "image/writejp2.h"
#include "image/writejbig2.h"   /* read2bytes(), read4bytes() */

#define BOX_JP   0x6A502020     /* Table 1.2 -- Defined boxes */
#define BOX_FTYP 0x66747970
#define BOX_JP2H 0x6a703268
#define BOX_IHDR 0x69686472
#define BOX_BPCC 0x62706363
#define BOX_COLR 0x636D6170
#define BOX_CDEF 0x63646566
#define BOX_RES  0x72657320
#define BOX_RESC 0x72657363
#define BOX_RESD 0x72657364
#define BOX_JP2C 0x6A703263

typedef struct {
    unsigned int tbox;          /* 1.4 Box definition */
    unsigned long lbox;
} hdr_struct;

static unsigned long read8bytes(FILE * f)
{
    unsigned long l = read4bytes(f);
    l = (l << 32) + read4bytes(f);
    return l;
}

static hdr_struct read_boxhdr(image_dict * idict)
{
    hdr_struct hdr;
    hdr.lbox = read4bytes(img_file(idict));
    hdr.tbox = read4bytes(img_file(idict));
    if (hdr.lbox == 1)
        hdr.lbox = read8bytes(img_file(idict));
    if (hdr.lbox == 0 && hdr.tbox != BOX_JP2C)
        pdftex_fail("reading JP2 image failed (LBox == 0)");
    return hdr;
}

static void scan_ihdr(image_dict * idict, unsigned long epos_s)
{
    unsigned long epos;
    unsigned int height, width, nc;     /* 1.5.3.1 Image Header box */
    unsigned char bpc, c, unkc, ipr;
    height = read4bytes(img_file(idict));
    width = read4bytes(img_file(idict));
    img_ysize(idict) = (int) height;
    img_xsize(idict) = (int) width;
    nc = read2bytes(img_file(idict));
    bpc = (unsigned char) xgetc(img_file(idict));
    c = (unsigned char) xgetc(img_file(idict));
    unkc = (unsigned char) xgetc(img_file(idict));
    ipr = (unsigned char) xgetc(img_file(idict));
    epos = xftell(img_file(idict), img_filepath(idict));
    if (epos != epos_s)
        pdftex_fail("reading JP2 image failed (ihdr box size inconsistent)");
}

/* Resolution scanning is work in progress */

static void scan_resc(image_dict * idict, unsigned long epos_s)
{
    unsigned long epos;
    unsigned int vrcn, vrcd, hrcn, hrcd;        /* 1.5.3.7.1 Capture Resolution box */
    unsigned char vrce, hrce;
    vrcn = read2bytes(img_file(idict));
    vrcd = read2bytes(img_file(idict));
    hrcn = read2bytes(img_file(idict));
    hrcd = read2bytes(img_file(idict));
    vrce = (unsigned char) xgetc(img_file(idict));
    hrce = (unsigned char) xgetc(img_file(idict));
    epos = xftell(img_file(idict), img_filepath(idict));
    if (epos != epos_s)
        pdftex_fail("reading JP2 image failed (resc box size inconsistent)");
}

static void scan_resd(image_dict * idict, unsigned long epos_s)
{
    unsigned long epos;
    unsigned int vrdn, vrdd, hrdn, hrdd;        /* 1.5.3.7.2 Default Display Resolution box */
    unsigned char vrde, hrde;
    vrdn = read2bytes(img_file(idict));
    vrdd = read2bytes(img_file(idict));
    hrdn = read2bytes(img_file(idict));
    hrdd = read2bytes(img_file(idict));
    vrde = (unsigned char) xgetc(img_file(idict));
    hrde = (unsigned char) xgetc(img_file(idict));
    epos = xftell(img_file(idict), img_filepath(idict));
    if (epos != epos_s)
        pdftex_fail("reading JP2 image failed (resd box size inconsistent)");
}

static void scan_res(image_dict * idict, unsigned long epos_s)
{
    hdr_struct hdr;
    unsigned long spos, epos;
    epos = xftell(img_file(idict), img_filepath(idict));
    while (1) {
        spos = epos;
        hdr = read_boxhdr(idict);       /* 1.5.3.7 Resolution box (superbox) */
        epos = spos + hdr.lbox;
        switch (hdr.tbox) {
        case (BOX_RESC):
            scan_resc(idict, epos);
            break;
        case (BOX_RESD):
            scan_resd(idict, epos);
            break;
        default:;
        }
        if (epos > epos_s)
            pdftex_fail("reading JP2 image failed (res box size inconsistent)");
        if (epos == epos_s)
            break;
        xfseek(img_file(idict), (long int) epos, SEEK_SET, img_filepath(idict));
    }
}

static boolean scan_jp2h(image_dict * idict, unsigned long epos_s)
{
    boolean ihdr_found = false;
    hdr_struct hdr;
    unsigned long spos, epos;
    epos = xftell(img_file(idict), img_filepath(idict));
    while (1) {
        spos = epos;
        hdr = read_boxhdr(idict);       /* 1.5.3 JP2 Header box (superbox) */
        epos = spos + hdr.lbox;
        switch (hdr.tbox) {
        case (BOX_IHDR):
            scan_ihdr(idict, epos);
            ihdr_found = true;
            break;
        case (BOX_RES):
            scan_res(idict, epos);
            break;
        default:;
        }
        if (epos > epos_s)
            pdftex_fail
                ("reading JP2 image failed (jp2h box size inconsistent)");
        if (epos == epos_s)
            break;
        xfseek(img_file(idict), (long int) epos, SEEK_SET, img_filepath(idict));
    }
    return ihdr_found;
}

static void close_and_cleanup_jp2(image_dict * idict)
{
    assert(idict != NULL);
    assert(img_file(idict) != NULL);
    assert(img_filepath(idict) != NULL);
    xfclose(img_file(idict), img_filepath(idict));
    img_file(idict) = NULL;
    assert(img_jp2_ptr(idict) != NULL);
    xfree(img_jp2_ptr(idict));
}

void read_jp2_info(PDF pdf, image_dict * idict, img_readtype_e readtype)
{
    boolean ihdr_found = false;
    hdr_struct hdr;
    unsigned long spos, epos;
    assert(img_type(idict) == IMG_TYPE_JP2);
    img_totalpages(idict) = 1;
    img_pagenum(idict) = 1;
    img_xres(idict) = img_yres(idict) = 0;
    assert(img_file(idict) == NULL);
    img_file(idict) = xfopen(img_filepath(idict), FOPEN_RBIN_MODE);
    assert(img_jp2_ptr(idict) == NULL);
    img_jp2_ptr(idict) = xtalloc(1, jp2_img_struct);
    xfseek(img_file(idict), 0, SEEK_END, img_filepath(idict));
    img_jp2_ptr(idict)->length =
        (int) xftell(img_file(idict), img_filepath(idict));
    xfseek(img_file(idict), 0, SEEK_SET, img_filepath(idict));

    assert(sizeof(unsigned long) >= 8);
    spos = epos = 0;

    /* 1.5.1 JPEG 2000 Signature box */
    hdr = read_boxhdr(idict);
    assert(hdr.tbox == BOX_JP); /* has already been checked */
    epos = spos + hdr.lbox;
    xfseek(img_file(idict), (long int) epos, SEEK_SET, img_filepath(idict));

    /* 1.5.2 File Type box */
    spos = epos;
    hdr = read_boxhdr(idict);
    if (hdr.tbox != BOX_FTYP)
        pdftex_fail("reading JP2 image failed (missing ftyp box)");
    epos = spos + hdr.lbox;
    xfseek(img_file(idict), (long int) epos, SEEK_SET, img_filepath(idict));

    while (!ihdr_found) {
        spos = epos;
        hdr = read_boxhdr(idict);
        epos = spos + hdr.lbox;
        switch (hdr.tbox) {
        case BOX_JP2H:
            ihdr_found = scan_jp2h(idict, epos);
            break;
        case BOX_JP2C:
            if (!ihdr_found)
                pdftex_fail("reading JP2 image failed (no ihdr box found)");
            break;
        default:;
        }
        xfseek(img_file(idict), (long int) epos, SEEK_SET, img_filepath(idict));
    }
    if (readtype == IMG_CLOSEINBETWEEN)
        close_and_cleanup_jp2(idict);
}

static void reopen_jp2(PDF pdf, image_dict * idict)
{
    int width, height, xres, yres;
    width = img_xsize(idict);
    height = img_ysize(idict);
    xres = img_xres(idict);
    yres = img_yres(idict);
    read_jp2_info(pdf, idict, IMG_KEEPOPEN);
    if (width != img_xsize(idict) || height != img_ysize(idict)
        || xres != img_xres(idict) || yres != img_yres(idict))
        pdftex_fail("writejp2: image dimensions have changed");
}

void write_jp2(PDF pdf, image_dict * idict)
{
    long unsigned l;
    FILE *f;
    assert(idict != NULL);
    if (img_file(idict) == NULL)
        reopen_jp2(pdf, idict);
    xfseek(img_file(idict), 0, SEEK_SET, img_filepath(idict));
    assert(img_jp2_ptr(idict) != NULL);
    pdf_puts(pdf, "/Type /XObject\n/Subtype /Image\n");
    if (img_attr(idict) != NULL && strlen(img_attr(idict)) > 0)
        pdf_printf(pdf, "%s\n", img_attr(idict));
    pdf_printf(pdf, "/Width %i\n/Height %i\n/Length %i\n",
               (int) img_xsize(idict),
               (int) img_ysize(idict), (int) img_jp2_ptr(idict)->length);
    pdf_puts(pdf, "/Filter /JPXDecode\n>>\nstream\n");
    for (l = (long unsigned int) img_jp2_ptr(idict)->length, f =
         img_file(idict); l > 0; l--)
        pdf_out(pdf, xgetc(f));
    pdf_end_stream(pdf);
    close_and_cleanup_jp2(idict);
}
