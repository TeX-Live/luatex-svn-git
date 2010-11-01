% txt2zlib.w

% Copyright 2010 Taco Hoekwater <taco@@luatex.org>
% Copyright 2010 Hartmut Henkel <hartmut@@luatex.org>

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

% $Id$
% $URL$

@
@c

#include<stdlib.h>
#include<stdio.h>
#include<zlib.h>

#define INBUFINITLEN 10
#define LINELEN 16
#define STRUCTNAME "pdfdriver_lua_zlib_struct"

char header[] =
    "   Copyright 2010 Taco Hoekwater <taco@@luatex.org>\n"
    "   Copyright 2010 Hartmut Henkel <hartmut@@luatex.org>\n"
    "\n"
    "   This file is part of LuaTeX.\n"
    "\n"
    "   LuaTeX is free software; you can redistribute it and/or modify it under\n"
    "   the terms of the GNU General Public License as published by the Free\n"
    "   Software Foundation; either version 2 of the License, or (at your\n"
    "   option) any later version.\n"
    "\n"
    "   LuaTeX is distributed in the hope that it will be useful, but WITHOUT\n"
    "   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\n"
    "   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public\n"
    "   License for more details.\n"
    "\n"
    "   You should have received a copy of the GNU General Public License along\n"
    "   with LuaTeX; if not, see <http://www.gnu.org/licenses/>.";

int main(int argc, char *argv[])
{
    int i, j, c, err;
    Byte *inbuf = NULL, *outbuf;
    FILE *fin, *fout;
    uLong inbuflen, uncomprLen = 0, outbuflen, comprLen;

    if (argc != 3) {
        fprintf(stderr,
                "%s: need exactly two arguments (input file name, output file name).\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }
    fin = fopen(argv[1], "r");
    if (fin == NULL) {
        fprintf(stderr, "%s: can't open %s for reading.\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }
    fout = fopen(argv[2], "w");
    if (fout == NULL) {
        fprintf(stderr, "%s: can't open %s for writing.\n", argv[0], argv[2]);
        exit(EXIT_FAILURE);
    }
    if ((inbuf = malloc(INBUFINITLEN * sizeof(Byte))) == NULL)
        exit(EXIT_FAILURE);
    inbuflen = INBUFINITLEN;
    while ((c = fgetc(fin)) != EOF) {
        if (uncomprLen == inbuflen - 1) {
            inbuflen = inbuflen * 1.5 + 1;
            if ((inbuf = realloc(inbuf, inbuflen)) == NULL)
                exit(EXIT_FAILURE);
        }
        inbuf[uncomprLen] = (Byte) c;
        uncomprLen++;
    }

    outbuflen = comprLen = uncomprLen * 1.1 + 50;
    if ((outbuf = malloc(outbuflen * sizeof(Byte))) == NULL)
        exit(EXIT_FAILURE);

    err = compress(outbuf, &comprLen, inbuf, uncomprLen);

    if (err != Z_OK) {
        fprintf(stderr, "compress error: %d\n", err);
        exit(EXIT_FAILURE);
    }

    fprintf(fout, "/* %s\n\n", argv[2]);
    fprintf(fout, "%s */\n\n", header);
    fprintf(fout, "#include \"ptexlib.h\"\n\n");

    fprintf(fout, "struct zlib_struct {\n");
    fprintf(fout, "    uLong uncomprLen;\n");
    fprintf(fout, "    uLong comprLen;\n");
    fprintf(fout, "    Byte compr[%ld];\n", comprLen);

    fprintf(fout, "} %s = {\n", STRUCTNAME);
    fprintf(fout, "    %ld,\n", uncomprLen);
    fprintf(fout, "    %ld,\n", comprLen);
    fprintf(fout, "    {\n");

    for (i = 0, j = 0; i < comprLen; i++) {
        if (j == 0)
            fprintf(fout, "        ");
        fprintf(fout, "0x%02x", outbuf[i]);
        if (i < comprLen - 1) {
            fprintf(fout, ",");
            if (j == LINELEN - 1) {
                fprintf(fout, "\n");
                j = 0;
            } else {
                fprintf(fout, " ");
                j++;
            }
        }
    }
    fprintf(fout, "\n    }\n};\n");

    fclose(fin);
    fclose(fout);
    return EXIT_SUCCESS;
}
