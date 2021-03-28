/* Copyright (C) 2000-2008 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "fontforgevw.h"
#include "ttf.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#ifdef __need_size_t
/* This is a bug on the mac, someone defines this and leaves it defined */
/*  that means when I load stddef.h it only defines size_t and doesn't */
/*  do offset_of, which is what I need */
# undef __need_size_t
#endif
#include <stddef.h>
#include <string.h>
#include <utype.h>
#include <ustring.h>



/* ************************************************************************** */
/* ******************************* Parse feat ******************************* */
/* ************************************************************************** */

#include <gfile.h>

struct nameid {
    uint16 strid;
    uint16 platform, specific, language;
    char *utf8_str;
    struct nameid *next;
};

struct tablekeywords {
    char *name;
    int size;			/* 1=>byte, 2=>short, 4=>int32 */
    int cnt;			/* normally 1, but 10 for panose, -1 for infinite */
    int offset;			/* -1 => parse but don't store */
};

struct tablevalues {
    int index;			/* in the table key structure above */
    int value;
    uint8 panose_vals[10];
    struct tablevalues *next;
};

enum feat_type { ft_lookup_start, ft_lookup_end, ft_feat_start, ft_feat_end,
    ft_table, ft_names, ft_gdefclasses, ft_lcaret, ft_tablekeys,
    ft_sizeparams,
    ft_subtable, ft_script, ft_lang, ft_lookupflags, ft_langsys,
    ft_pst, ft_pstclass, ft_fpst, ft_ap, ft_lookup_ref };
struct feat_item {
    uint16 /* enum feat_type */ type;
    uint8 ticked;
    union {
	SplineChar *sc;		/* For psts, aps */
	char *class;		/* List of glyph names for kerning by class, lcarets */
	char *lookup_name;	/* for lookup_start/ lookup_ref */
	uint32 tag;		/* for feature/script/lang tag */
	int *params;		/* size params */
	struct tablekeywords *offsets;
	char **gdef_classes;
    } u1;
    union {
	PST *pst;
		/* For kerning by class we'll generate an invalid pst with the class as the "paired" field */
	FPST *fpst;
	AnchorPoint *ap;
	int lookupflags;
	struct scriptlanglist *sl;	/* Default langsyses for features/langsys */
	int exclude_dflt;		/* for lang tags */
	struct nameid *names;		/* size params */
	struct tablevalues *tvals;
	int16 *lcaret;
    } u2;
    char *mark_class;			/* For mark to base-ligature-mark, names of all marks which attach to this anchor */
    struct feat_item *next, *lookup_next;
};

static int strcmpD(const void *_str1, const void *_str2) {
    const char *str1 = *(const char **)_str1, *str2 = *(const char **) _str2;
return( strcmp(str1,str2));
}

/* Order glyph classes just so we can do a simple string compare to check for */
/*  class match. So the order doesn't really matter, just so it is consistent */
static char *fea_canonicalClassOrder(char *class) {
    int name_cnt, i;
    char *pt, **names, *cpt;
    char *temp = copy(class);

    name_cnt = 0;
    for ( pt = class; ; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	for ( ; *pt!=' ' && *pt!='\0'; ++pt );
	++name_cnt;
    }

    names = galloc(name_cnt*sizeof(char *));
    name_cnt = 0;
    for ( pt = temp; ; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	for ( names[name_cnt++]=pt ; *pt!=' ' && *pt!='\0'; ++pt );
	if ( *pt==' ' )
	    *pt++ = '\0';
    }

    qsort(names,name_cnt,sizeof(char *),strcmpD);
    cpt = class;
    for ( i=0; i<name_cnt; ++i ) {
	strcpy(cpt,names[i]);
	cpt += strlen(cpt);
	*cpt++ = ' ';
    }
    if ( name_cnt!=0 )
	cpt[-1] = '\0';
    free(names);
    free(temp);

return( class );
}

static int fea_classesIntersect(char *class1, char *class2) {
    char *pt1, *start1, *pt2, *start2;
    int ch1, ch2;

    for ( pt1=class1 ; ; ) {
        while ( *pt1==' ' ) ++pt1;
        if ( *pt1=='\0' )
            return( 0 );
        for ( start1 = pt1; *pt1!=' ' && *pt1!='\0'; ++pt1 );
        ch1 = *pt1; *pt1 = '\0';
        for ( pt2=class2 ; ; ) {
            while ( *pt2==' ' ) ++pt2;
            if ( *pt2=='\0' )
                break;
            for ( start2 = pt2; *pt2!=' ' && *pt2!='\0'; ++pt2 );
            ch2 = *pt2; *pt2 = '\0';
            if ( strcmp(start1,start2)==0 ) {
                *pt2 = ch2; *pt1 = ch1;
                return( 1 );
            }
            *pt2 = ch2;
        }
        *pt1 = ch1;
    }
}


#define SKIP_SPACES(s, i)                       \
    do {                                        \
        while ((s)[i] == ' ')                   \
            i++;                                \
    }                                           \
    while (0)

#define FIND_SPACE(s, i)                        \
    do {                                        \
        while ((s)[i] != ' ' && (s)[i] != '\0') \
            i++;                                \
    }                                           \
    while (0)


static char *fea_classesSplit(char *class1, char *class2) {
    char *intersection;
    int len = strlen(class1), len2 = strlen(class2);
    int ix;
    int i, j, i_end, j_end;
    int length;
    int match_found;

    if ( len2>len ) len = len2;
    intersection = galloc(len+1);
    ix = 0;

    i = 0;
    SKIP_SPACES(class1, i);
    while (class1[i] != '\0') {
        i_end = i;
        FIND_SPACE(class1, i_end);

        length = i_end - i;

        match_found = 0;
        j = 0;
        SKIP_SPACES(class2, j);
        while (!match_found && class2[j] != '\0') {
            j_end = j;
            FIND_SPACE(class2, j_end);

            if (length == j_end - j && strncmp(class1 + i, class2 + j, length) == 0) {
                match_found = 1;

                if (ix != 0) {
                    intersection[ix] = ' ';
                    ix++;
                }
                memcpy(intersection + ix, class1 + i, length * sizeof (char));
                ix += length;

                SKIP_SPACES(class1, i_end);
                memmove(class1 + i, class1 + i_end, (strlen(class1 + i_end) + 1) * sizeof (char));
                SKIP_SPACES(class2, j_end);
                memmove(class2 + j, class2 + j_end, (strlen(class2 + j_end) + 1) * sizeof (char));
            } else {
                j = j_end;
                SKIP_SPACES(class2, j);
            }
        }
        if (!match_found) {
            i = i_end;
            SKIP_SPACES(class1, i);
        }
    }
    intersection[ix] = '\0';
    return( intersection );
}

#define MAXT	40
#define MAXI	5
struct parseState {
    char tokbuf[MAXT+1];
    long value;
    enum toktype { tk_name, tk_class, tk_int, tk_char, tk_cid, tk_eof,
/* keywords */
	tk_firstkey,
	tk_anchor=tk_firstkey, tk_anonymous, tk_by, tk_caret, tk_cursive, tk_device,
	tk_enumerate, tk_excludeDFLT, tk_exclude_dflt, tk_feature, tk_from,
	tk_ignore, tk_ignoreDFLT, tk_ignoredflt, tk_IgnoreBaseGlyphs,
	tk_IgnoreLigatures, tk_IgnoreMarks, tk_include, tk_includeDFLT,
	tk_include_dflt, tk_language, tk_languagesystem, tk_lookup,
	tk_lookupflag, tk_mark, tk_nameid, tk_NULL, tk_parameters, tk_position,
	tk_required, tk_RightToLeft, tk_script, tk_substitute, tk_subtable,
	tk_table, tk_useExtension
    } type;
    uint32 tag;
    int could_be_tag;
    FILE *inlist[MAXI];
    int inc_depth;
    int line[MAXI];
    char *filename[MAXI];
    int err_count;
    unsigned int warned_about_not_cid: 1;
    unsigned int lookup_in_sf_warned: 1;
    unsigned int in_vkrn: 1;
    unsigned int backedup: 1;
    unsigned int skipping: 1;
    SplineFont *sf;
    struct scriptlanglist *def_langsyses;
    struct glyphclasses { char *classname, *glyphs; struct glyphclasses *next; } *classes;
    struct feat_item *sofar;
    int base;			/* normally numbers are base 10, but in the case of languages in stringids, they can be octal or hex */
    OTLookup *created, *last;	/* Ordered, but not sorted into GSUB, GPOS yet */
    AnchorClass *accreated;
};

static struct keywords {
    char *name;
    enum toktype tok;
} fea_keywords[] = {
/* list must be in toktype order */
    { "name", tk_name }, { "glyphclass", tk_class }, { "integer", tk_int },
    { "random character", tk_char}, { "cid", tk_cid }, { "EOF", tk_eof },
/* keywords now */
    { "anchor", tk_anchor },
    { "anonymous", tk_anonymous },
    { "by", tk_by },
    { "caret", tk_caret },
    { "cursive", tk_cursive },
    { "device", tk_device },
    { "enumerate", tk_enumerate },
    { "excludeDFLT", tk_excludeDFLT },
    { "exclude_dflt", tk_exclude_dflt },
    { "feature", tk_feature },
    { "from", tk_from },
    { "ignore", tk_ignore },
    { "IgnoreBaseGlyphs", tk_IgnoreBaseGlyphs },
    { "IgnoreLigatures", tk_IgnoreLigatures },
    { "IgnoreMarks", tk_IgnoreMarks },
    { "include", tk_include },
    { "includeDFLT", tk_includeDFLT },
    { "include_dflt", tk_include_dflt },
    { "language", tk_language },
    { "languagesystem", tk_languagesystem },
    { "lookup", tk_lookup },
    { "lookupflag", tk_lookupflag },
    { "mark", tk_mark },
    { "nameid", tk_nameid },
    { "NULL", tk_NULL },
    { "parameters", tk_parameters },
    { "position", tk_position },
    { "required", tk_required },
    { "RightToLeft", tk_RightToLeft },
    { "script", tk_script },
    { "substitute", tk_substitute },
    { "subtable", tk_subtable },
    { "table", tk_table },
    { "useExtension", tk_useExtension },
/* synonyms */
    { "sub", tk_substitute },
    { "pos", tk_position },
    { "enum", tk_enumerate },
    { "anon", tk_anonymous },
    { NULL, 0 }
};

static struct tablekeywords hhead_keys[] = {
    { "CaretOffset", sizeof(short), 1, -1 },		/* Don't even know what this is! */
    { "Ascender", sizeof(short), 1, offsetof(struct pfminfo,hhead_ascent)+offsetof(SplineFont,pfminfo) },
    { "Descender", sizeof(short), 1, offsetof(struct pfminfo,hhead_descent)+offsetof(SplineFont,pfminfo) },
    { "LineGap", sizeof(short), 1, offsetof(struct pfminfo,linegap)+offsetof(SplineFont,pfminfo) },
    { NULL, 0, 0, 0 }
};

static struct tablekeywords vhead_keys[] = {
    { "VertTypoAscender", sizeof(short), 1, -1 },
    { "VertTypoDescender", sizeof(short), 1, -1 },
    { "VertTypoLineGap", sizeof(short), 1, offsetof(struct pfminfo,vlinegap)+offsetof(SplineFont,pfminfo) },
    { NULL, 0, 0, 0 }
};

static struct tablekeywords os2_keys[] = {
    { "FSType", sizeof(short), 1, offsetof(struct pfminfo,fstype)+offsetof(SplineFont,pfminfo) },
    { "Panose", sizeof(uint8), 10, offsetof(struct pfminfo,panose)+offsetof(SplineFont,pfminfo) },
    { "UnicodeRange", sizeof(short), -1, -1 },
    { "CodePageRange", sizeof(short), -1, -1 },
    { "TypoAscender", sizeof(short), 1, offsetof(struct pfminfo,os2_typoascent)+offsetof(SplineFont,pfminfo) },
    { "TypoDescender", sizeof(short), 1, offsetof(struct pfminfo,os2_typodescent)+offsetof(SplineFont,pfminfo) },
    { "TypoLineGap", sizeof(short), 1, offsetof(struct pfminfo,os2_typolinegap)+offsetof(SplineFont,pfminfo) },
    { "winAscent", sizeof(short), 1, offsetof(struct pfminfo,os2_winascent)+offsetof(SplineFont,pfminfo) },
    { "winDescent", sizeof(short), 1, offsetof(struct pfminfo,os2_windescent)+offsetof(SplineFont,pfminfo) },
    { "XHeight", sizeof(short), 1, -1 },
    { "CapHeight", sizeof(short), 1, -1 },
    { "WeightClass", sizeof(short), 1, offsetof(struct pfminfo,weight)+offsetof(SplineFont,pfminfo) },
    { "WidthClass", sizeof(short), 1, offsetof(struct pfminfo,width)+offsetof(SplineFont,pfminfo) },
    { "Vendor", sizeof(short), 1, offsetof(struct pfminfo,os2_vendor)+offsetof(SplineFont,pfminfo) },
    { NULL, 0, 0, 0 }
};


static void fea_ParseTok(struct parseState *tok);

static void fea_handle_include(struct parseState *tok) {
    FILE *in;
    char namebuf[1025], *pt, *filename;
    int ch;

    fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!='(' ) {
	LogError(_("Unparseable include on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return;
    }

    in = tok->inlist[tok->inc_depth];
    ch = getc(in);
    while ( isspace(ch))
	ch = getc(in);
    pt = namebuf;
    while ( ch!=EOF && ch!=')' && pt<namebuf+sizeof(namebuf)-1 ) {
	*pt++ = ch;
	ch = getc(in);
    }
    if ( ch!=EOF && ch!=')' ) {
	while ( ch!=EOF && ch!=')' )
	    ch = getc(in);
	LogError(_("Include filename too long on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
    while ( pt>=namebuf+1 && isspace(pt[-1]) )
	--pt;
    *pt = '\0';
    if ( ch!=')' ) {
	if ( ch==EOF )
	    LogError(_("End of file in include on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	else
	    LogError(_("Missing close parenthesis in include on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return;
    }

    if ( pt==namebuf ) {
	LogError(_("No filename specified in include on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return;
    }

    if ( tok->inc_depth>=MAXI-1 ) {
	LogError(_("Includes nested too deeply on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return;
    }

    if ( *namebuf=='/' ||
	    ( pt = strrchr(tok->filename[tok->inc_depth],'/') )==NULL )
	filename=copy(namebuf);
    else {
	*pt = '\0';
	filename = GFileAppendFile(tok->filename[tok->inc_depth],namebuf,false);
	*pt = '/';
    }
    in = fopen(filename,"r");
    if ( in==NULL ) {
	LogError(_("Could not open include file (%s) on line %d of %s"),
		filename, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	free(filename);
return;
    }

    ++tok->inc_depth;
    tok->filename[tok->inc_depth] = filename;
    tok->inlist[tok->inc_depth] = in;
    tok->line[tok->inc_depth] = 1;
    fea_ParseTok(tok);
}

static void fea_ParseTok(struct parseState *tok) {
    FILE *in = tok->inlist[tok->inc_depth];
    int ch, peekch = 0;
    char *pt, *start;

    if ( tok->backedup ) {
	tok->backedup = false;
return;
    }

  skip_whitespace:
    ch = getc(in);
    while ( isspace(ch) || ch=='#' ) {
	if ( ch=='#' )
	    while ( (ch=getc(in))!=EOF && ch!='\n' && ch!='\r' );
	if ( ch=='\n' || ch=='\r' ) {
	    if ( ch=='\r' ) {
		ch = getc(in);
		if ( ch!='\n' )
		    ungetc(ch,in);
	    }
	    ++tok->line[tok->inc_depth];
	}
	ch = getc(in);
    }

    tok->could_be_tag = 0;
    if ( ch==EOF ) {
	if ( tok->inc_depth>0 ) {
	    fclose(tok->inlist[tok->inc_depth]);
	    free(tok->filename[tok->inc_depth]);
	    in = tok->inlist[--tok->inc_depth];
  goto skip_whitespace;
	}
	tok->type = tk_eof;
	strcpy(tok->tokbuf,"EOF");
return;
    }

    start = pt = tok->tokbuf;
    if ( ch=='\\' || ch=='-' ) {
	peekch=getc(in);
	ungetc(peekch,in);
    }

    if ( isdigit(ch) || ch=='+' || ((ch=='-' || ch=='\\') && isdigit(peekch)) ) {
	tok->type = tk_int;
	if ( ch=='-' || ch=='+' ) {
	    if ( ch=='-' ) {
		*pt++ = ch;
		start = pt;
	    }
	    ch = getc(in);
	} else if ( ch=='\\' ) {
	    ch = getc(in);
	    tok->type = tk_cid;
	}
	while ( (isdigit( ch ) ||
		(tok->base==0 && (ch=='x' || ch=='X' || (ch>='a' && ch<='f') || (ch>='A' && ch<='F'))))
		&& pt<tok->tokbuf+15 ) {
	    *pt++ = ch;
	    ch = getc(in);
	}
	if ( isdigit(ch)) {
	    LogError(_("Number too long on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else if ( pt==start ) {
	    LogError(_("Missing number on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
	ungetc(ch,in);
	*pt = '\0';
	tok->value = strtol(tok->tokbuf,NULL,tok->base);
return;
    } else if ( ch=='@' || ch=='_' || ch=='\\' || isalnum(ch)) {	/* Names can't start with dot */
	int check_keywords = true;
	tok->type = tk_name;
	if ( ch=='@' ) {
	    tok->type = tk_class;
	    *pt++ = ch;
	    start = pt;
	    ch = getc(in);
	    check_keywords = false;
	} else if ( ch=='\\' ) {
	    ch = getc(in);
	    check_keywords = false;
	}
	while (( isalnum(ch) || ch=='_' || ch=='.' ) && pt<start+31 ) {
	    *pt++ = ch;
	    ch = getc(in);
	}
	*pt = '\0';
	ungetc(ch,in);
	if ( isalnum(ch) || ch=='_' || ch=='.' ) {
	    LogError(_("Name too long on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else if ( pt==start ) {
	    LogError(_("Missing name on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}

	if ( check_keywords ) {
	    int i;
	    for ( i=tk_firstkey; fea_keywords[i].name!=NULL; ++i ) {
		if ( strcmp(fea_keywords[i].name,tok->tokbuf)==0 ) {
		    tok->type = fea_keywords[i].tok;
	    break;
		}
	    }
	    if ( tok->type==tk_include )
		fea_handle_include(tok);
	}
	if ( tok->type==tk_name && pt-tok->tokbuf<=4 && pt!=tok->tokbuf ) {
	    unsigned char tag[4];
	    tok->could_be_tag = true;
	    memset(tag,' ',4);
	    tag[0] = tok->tokbuf[0];
	    if ( tok->tokbuf[1]!='\0' ) {
		tag[1] = tok->tokbuf[1];
		if ( tok->tokbuf[2]!='\0' ) {
		    tag[2] = tok->tokbuf[2];
		    if ( tok->tokbuf[3]!='\0' )
			tag[3] = tok->tokbuf[3];
		}
	    }
	    tok->tag = (tag[0]<<24) | (tag[1]<<16) | (tag[2]<<8) | tag[3];
	}
    } else {
	/* I've already handled the special characters # @ and \ */
	/*  so don't treat them as errors here, if they occur they will be out of context */
	if ( ch==';' || ch==',' || ch=='-' || ch=='=' || ch=='\'' || ch=='"' ||
		ch=='{' || ch=='}' ||
		ch=='[' || ch==']' ||
		ch=='<' || ch=='>' ||
		ch=='(' || ch==')' ) {
	    tok->type = tk_char;
	    tok->tokbuf[0] = ch;
	    tok->tokbuf[1] = '\0';
	} else {
	    if ( !tok->skipping ) {
		LogError(_("Unexpected character (0x%02X) on line %d of %s"), ch, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
  goto skip_whitespace;
	}
    }
}

static void fea_ParseTag(struct parseState *tok) {
    /* The tag used for OS/2 doesn't get parsed properly */
    /* So if we know we are looking for a tag do some fixups */

    fea_ParseTok(tok);
    if ( tok->type==tk_name && tok->could_be_tag &&
	    tok->tag==CHR('O','S',' ',' ') ) {
	FILE *in = tok->inlist[tok->inc_depth];
	int ch;
	ch = getc(in);
	if ( ch=='/' ) {
	    ch = getc(in);
	    if ( ch=='2' ) {
		tok->tag = CHR('O','S','/','2');
	    } else {
		tok->tag = CHR('O','S','/',' ');
		ungetc(ch,in);
	    }
	} else
	    ungetc(ch,in);
    }
}

static void fea_UnParseTok(struct parseState *tok) {
    tok->backedup = true;
}

static int fea_ParseDeciPoints(struct parseState *tok) {
    /* When parsing size features floating point numbers are allowed */
    /*  but they should be converted to ints by multiplying by 10 */
    /* (not my convention) */

    fea_ParseTok(tok);
    if ( tok->type==tk_int ) {
	FILE *in = tok->inlist[tok->inc_depth];
	char *pt = tok->tokbuf + strlen(tok->tokbuf);
	int ch;
	ch = getc(in);
	if ( ch=='.' ) {
	    *pt++ = ch;
	    while ( (ch = getc(in))!=EOF && isdigit(ch)) {
		if ( pt<tok->tokbuf+sizeof(tok->tokbuf)-1 )
		    *pt++ = ch;
	    }
	    *pt = '\0';
	    tok->value = rint(strtod(tok->tokbuf,NULL)*10.0);
	}
	if ( ch!=EOF )
	    ungetc(ch,in);
    } else {
	LogError(_("Expected '%s' on line %d of %s"), fea_keywords[tk_int],
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	tok->value = -1;
    }
return( tok->value );
}

static void fea_TokenMustBe(struct parseState *tok, enum toktype type, int ch) {
    fea_ParseTok(tok);
    if ( type==tk_char && (tok->type!=type || tok->tokbuf[0]!=ch) ) {
	LogError(_("Expected '%c' on line %d of %s"), ch, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else if ( type!=tk_char && tok->type!=type ) {
	LogError(_("Expected '%s' on line %d of %s"), fea_keywords[type],
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
}

static void fea_skip_to_semi(struct parseState *tok) {
    int nest=0;

    while ( tok->type!=tk_char || tok->tokbuf[0]!=';' || nest>0 ) {
	fea_ParseTok(tok);
	if ( tok->type==tk_char ) {
	    if ( tok->tokbuf[0]=='{' ) ++nest;
	    else if ( tok->tokbuf[0]=='}' ) --nest;
	    if ( nest<0 )
    break;
	}
	if ( tok->type==tk_eof )
    break;
    }
}

static void fea_skip_to_close_curly(struct parseState *tok) {
    int nest=0;

    tok->skipping = true;
    /* The table blocks have slightly different syntaxes and can take strings */
    /* and floating point numbers. So don't complain about unknown chars when */
    /*  in a table (that's skipping) */
    while ( tok->type!=tk_char || tok->tokbuf[0]!='}' || nest>0 ) {
	fea_ParseTok(tok);
	if ( tok->type==tk_char ) {
	    if ( tok->tokbuf[0]=='{' ) ++nest;
	    else if ( tok->tokbuf[0]=='}' ) --nest;
	}
	if ( tok->type==tk_eof )
    break;
    }
    tok->skipping = false;
}

static void fea_now_semi(struct parseState *tok) {
    if ( tok->type!=tk_char || tok->tokbuf[0]!=';' ) {
	LogError(_("Expected ';' at statement end on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	fea_skip_to_semi(tok);
	++tok->err_count;
return;
    }
}

static void fea_end_statement(struct parseState *tok) {
    fea_ParseTok(tok);
    fea_now_semi(tok);
}

static struct glyphclasses *fea_lookup_class(struct parseState *tok,char *classname) {
    struct glyphclasses *test;

    for ( test=tok->classes; test!=NULL; test=test->next ) {
	if ( strcmp(classname,test->classname)==0 )
return( test );
    }
return( NULL );
}

static char *fea_lookup_class_complain(struct parseState *tok,char *classname) {
    struct glyphclasses *test;

    for ( test=tok->classes; test!=NULL; test=test->next ) {
	if ( strcmp(classname,test->classname)==0 )
return( copy( test->glyphs) );
    }
    LogError(_("Use of undefined glyph class, %s, on line %d of %s"), classname, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
    ++tok->err_count;
return( NULL );
}

static void fea_AddClassDef(struct parseState *tok,char *classname,char *contents) {
    struct glyphclasses *test;

    test = fea_lookup_class(tok,classname);
    if ( test==NULL ) {
	test=chunkalloc(sizeof(struct glyphclasses));
	test->classname = classname;
	test->next = tok->classes;
	tok->classes = test;
    } else {
	free(classname);
	free(test->glyphs);
    }
    test->glyphs = contents;
}

static int fea_AddGlyphs(char **_glyphs, int *_max, int cnt, char *contents ) {
    int len = strlen(contents);
    char *glyphs = *_glyphs;
    /* Append a glyph name, etc. to a glyph class */

    if ( glyphs==NULL ) {
	glyphs = copy(contents);
	cnt = *_max = len;
    } else {
	if ( *_max-cnt <= len+1 )
	    glyphs = grealloc(glyphs,(*_max+=200+len+1)+1);
	glyphs[cnt++] = ' ';
	strcpy(glyphs+cnt,contents);
	cnt += strlen(contents);
    }
    free(contents);
    *_glyphs = glyphs;
return( cnt );
}

static char *fea_cid_validate(struct parseState *tok,int cid) {
    int i, max;
    SplineFont *maxsf;
    SplineChar *sc;
    EncMap *map;

    if ( tok->sf->subfontcnt==0 ) {
	if ( !tok->warned_about_not_cid ) {
	    LogError(_("Reference to a CID in a non-CID-keyed font on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    tok->warned_about_not_cid = true;
	}
	++tok->err_count;
return(NULL);
    }
    max = 0; maxsf = NULL;
    for ( i=0; i<tok->sf->subfontcnt; ++i ) {
	SplineFont *sub = tok->sf->subfonts[i];
	if ( cid<sub->glyphcnt && sub->glyphs[cid]!=NULL )
return( sub->glyphs[cid]->name );
	if ( sub->glyphcnt>max ) {
	    max = sub->glyphcnt;
	    maxsf = sub;
	}
    }
    /* Not defined, try to create it */
    if ( maxsf==NULL )		/* No subfonts */
return( NULL );
    if ( cid>=maxsf->glyphcnt ) {
	struct cidmap *cidmap = FindCidMap(tok->sf->cidregistry,tok->sf->ordering,tok->sf->supplement,tok->sf);
	if ( cidmap==NULL || cid>=MaxCID(cidmap) )
return( NULL );
	SFExpandGlyphCount(maxsf,MaxCID(cidmap));
    }
    if ( cid>=maxsf->glyphcnt )
return( NULL );
    map = EncMap1to1(maxsf->glyphcnt);
    sc = SFMakeChar(maxsf,map,cid);
    EncMapFree(map);
    if ( sc==NULL )
return( NULL );
return( copy( sc->name ));
}

static SplineChar *fea_glyphname_get(struct parseState *tok,char *name) {
    SplineFont *sf = tok->sf;
    SplineChar *sc = SFGetChar(sf,-1,name);
    int enc, gid;

    if ( sf->subfontcnt!=0 ) {
	LogError(_("Reference to a glyph name in a CID-keyed font on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return(sc);
    }

    if ( sc!=NULL || strcmp(name,"NULL")==0 )
return( sc );
    enc = SFFindSlot(sf,sf->fv->map,-1,name);
    if ( enc!=-1 ) {
	sc = SFMakeChar(sf,sf->fv->map,enc);
	if ( sc!=NULL ) {
	    sc->widthset = true;
	    free(sc->name);
	    sc->name = copy(name);
	}
return( sc );
    }
    
    for ( gid=sf->glyphcnt-1; gid>=0; --gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
	if ( strcmp(sc->name,name)==0 )
return( sc );
    }

/* Don't encode it (not in current encoding), just add it, so we needn't */
/*  mess with maps or selections */
    SFExpandGlyphCount(sf,sf->glyphcnt+1);
    sc = SFSplineCharCreate(sf);
    sc->name = copy(name);
    sc->unicodeenc = UniFromName(name,ui_none,&custom);
    sc->parent = sf;
    sc->vwidth = (sf->ascent+sf->descent);
    sc->width = 6*sc->vwidth/10;
    sc->widthset = true;		/* So we don't lose the glyph */
    sc->orig_pos = sf->glyphcnt-1;
    sf->glyphs[sc->orig_pos] = sc;
return( sc );
}

static char *fea_glyphname_validate(struct parseState *tok,char *name) {
    SplineChar *sc = fea_glyphname_get(tok,name);

    if ( sc==NULL )
return( NULL );

return( copy( sc->name ));
}

static char *fea_ParseGlyphClass(struct parseState *tok) {
    char *glyphs = NULL;

    if ( tok->type==tk_class ) {
	glyphs = fea_lookup_class_complain(tok,tok->tokbuf);
    } else if ( tok->type!=tk_char || tok->tokbuf[0]!='[' ) {
	LogError(_("Expected '[' in glyph class definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
return( NULL );
    } else {
	char *contents = NULL;
	int cnt=0, max=0;
	int last_val = 0, range_type, range_len = 0;
	char last_glyph[MAXT+1];
	char *pt1, *start1, *pt2, *start2 = NULL;
	int v1, v2;

	forever {
	    fea_ParseTok(tok);
	    if ( tok->type==tk_char && tok->tokbuf[0]==']' )
	break;
	    if ( tok->type==tk_class ) {
		contents = fea_lookup_class_complain(tok,tok->tokbuf);
		last_val=-1; last_glyph[0] = '\0';
	    } else if ( tok->type==tk_cid ) {
		last_val = tok->value; last_glyph[0] = '\0';
		contents = fea_cid_validate(tok,tok->value);
	    } else if ( tok->type==tk_name ) {
		strcpy(last_glyph,tok->tokbuf); last_val = -1;
		contents = fea_glyphname_validate(tok,tok->tokbuf);
	    } else if ( tok->type==tk_char && tok->tokbuf[0]=='-' ) {
		fea_ParseTok(tok);
		if ( last_val!=-1 && tok->type==tk_cid ) {
		    if ( last_val>=tok->value ) {
			LogError(_("Invalid CID range in glyph class on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
			++tok->err_count;
		    }
		    /* Last val has already been added to the class */
		    /* and we'll add the current value later */
		    for ( ++last_val; last_val<tok->value; ++last_val ) {
			contents = fea_cid_validate(tok,last_val);
			if ( contents!=NULL )
			    cnt = fea_AddGlyphs(&glyphs,&max,cnt,contents);
		    }
		    contents = fea_cid_validate(tok,tok->value);
		} else if ( last_glyph[0]!='\0' && tok->type==tk_name ) {
		    range_type=0;
		    if ( strlen(last_glyph)==strlen(tok->tokbuf) &&
			    strcmp(last_glyph,tok->tokbuf)<0 ) {
			start1=NULL;
			for ( pt1=last_glyph, pt2=tok->tokbuf;
				*pt1!='\0'; ++pt1, ++pt2 ) {
			    if ( *pt1!=*pt2 ) {
				if ( start1!=NULL ) {
				    range_type=0;
			break;
				}
			        start1 = pt1; start2 = pt2;
			        if ( !isdigit(*pt1) || !isdigit(*pt2))
				    range_type = 1;
				else {
				    for ( range_len=0; range_len<3 && isdigit(*pt1) && isdigit(*pt2);
					    ++range_len, ++pt1, ++pt2 );
				    range_type = 2;
			            --pt1; --pt2;
				}
			    }
			}
		    }
		    if ( range_type==0 ) {
			LogError(_("Invalid glyph name range in glyph class on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
			++tok->err_count;
		    } else if ( range_type==1 || range_len==1 ) {
			/* Single letter changes */
			v1 = *start1; v2 = *start2;
			for ( ++v1; v1<=v2; ++v1 ) {
			    *start1 = v1;
			    contents = fea_glyphname_validate(tok,start1);
			    if ( v1==v2 )
			break;
			    if ( contents!=NULL )
				cnt = fea_AddGlyphs(&glyphs,&max,cnt,contents);
			}
		    } else {
			v1 = strtol(start1,NULL,10);
			v2 = strtol(start2,NULL,10);
			for ( ++v1; v1<=v2; ++v1 ) {
			    if ( range_len==2 )
				sprintf( last_glyph, "%.*s%02d%s", (int) (start2-tok->tokbuf),
					tok->tokbuf, v1, start2+2 );
			    else
				sprintf( last_glyph, "%.*s%03d%s", (int) (start2-tok->tokbuf),
					tok->tokbuf, v1, start2+3 );
			    contents = fea_glyphname_validate(tok,last_glyph);
			    if ( v1==v2 )
			break;
			    if ( contents!=NULL )
				cnt = fea_AddGlyphs(&glyphs,&max,cnt,contents);
			}
		    }
		} else {
		    LogError(_("Unexpected token in glyph class range on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		    if ( tok->type==tk_char && tok->tokbuf[0]==']' )
	break;
		}
		last_val=-1; last_glyph[0] = '\0';
	    } else if ( tok->type == tk_NULL ) {
		contents = copy("NULL");
	    } else {
		LogError(_("Expected glyph name, cid, or class in glyph class definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	break;
	    }
	    if ( contents!=NULL )
		cnt = fea_AddGlyphs(&glyphs,&max,cnt,contents);
	}
	if ( glyphs==NULL )
	    glyphs = copy("");	/* Is it legal to have an empty class? I can't think of any use for one */
    }
return( glyphs );
}

static char *fea_ParseGlyphClassGuarded(struct parseState *tok) {
    char *ret = fea_ParseGlyphClass(tok);
    if ( ret==NULL )
	ret = copy("");
return( ret );
}

static void fea_ParseLookupFlags(struct parseState *tok) {
    int val = 0;
    struct feat_item *item;

    fea_ParseTok(tok);
    if ( tok->type==tk_int ) {
	val = tok->value;
	fea_end_statement(tok);
    } else {
	while ( tok->type==tk_RightToLeft || tok->type==tk_IgnoreBaseGlyphs ||
		tok->type==tk_IgnoreMarks || tok->type==tk_IgnoreLigatures ) {
	    if ( tok->type == tk_RightToLeft )
		val |= pst_r2l;
	    else if ( tok->type == tk_IgnoreBaseGlyphs )
		val |= pst_ignorebaseglyphs;
	    else if ( tok->type == tk_IgnoreMarks )
		val |= pst_ignorecombiningmarks;
	    else if ( tok->type == tk_IgnoreLigatures )
		val |= pst_ignoreligatures;
	    fea_ParseTok(tok);
	    if ( tok->type == tk_char && tok->tokbuf[0]==';' )
	break;
	    else if ( tok->type==tk_char && tok->tokbuf[0]!=',' ) {
		LogError(_("Expected ';' in lookupflags on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		fea_skip_to_semi(tok);
	break;
	    }
	    fea_ParseTok(tok);
	}
	if ( tok->type != tk_char || tok->tokbuf[0]!=';' ) {
	    LogError(_("Unexpected token in lookupflags on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	    fea_skip_to_semi(tok);
	} else if ( val==0 ) {
	    LogError(_("No flags specified in lookupflags on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
    }

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_lookupflags;
    item->u2.lookupflags = val;
    item->next = tok->sofar;
    tok->sofar = item;
}

static void fea_ParseGlyphClassDef(struct parseState *tok) {
    char *classname = copy(tok->tokbuf );
    char *contents;

    fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!='=' ) {
	LogError(_("Expected '=' in glyph class definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    fea_ParseTok(tok);
    contents = fea_ParseGlyphClass(tok);
    if ( contents==NULL ) {
	fea_skip_to_semi(tok);
return;
    }
    fea_AddClassDef(tok,classname,copy(contents));
    fea_end_statement(tok);
}

static void fea_ParseLangSys(struct parseState *tok, int inside_feat) {
    uint32 script, lang;
    struct scriptlanglist *sl;
    int l;

    fea_ParseTok(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag ) {
	LogError(_("Expected tag in languagesystem on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    script = tok->tag;

    fea_ParseTok(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag ) {
	LogError(_("Expected tag in languagesystem on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    lang = tok->tag;

    for ( sl=tok->def_langsyses; sl!=NULL && sl->script!=script; sl=sl->next );
    if ( sl==NULL ) {
	sl = chunkalloc(sizeof(struct scriptlanglist));
	sl->script = script;
	sl->next = tok->def_langsyses;
	tok->def_langsyses = sl;
    }
    for ( l=0; l<sl->lang_cnt; ++l ) {
	uint32 language = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
	if ( language==lang )
    break;
    }
    if ( l<sl->lang_cnt )
	/* Er... this combination is already in the list. I guess that's ok */;
    else if ( sl->lang_cnt<MAX_LANG )
	sl->langs[sl->lang_cnt++] = lang;
    else {
	sl->morelangs = grealloc(sl->morelangs,(sl->lang_cnt+1)*sizeof(uint32));
	sl->morelangs[sl->lang_cnt++ - MAX_LANG] = lang;
    }
    fea_end_statement(tok);

    if ( inside_feat ) {
	struct feat_item *item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_langsys;
	item->u2.sl = SListCopy(tok->def_langsyses);
	item->next = tok->sofar;
	tok->sofar = item;
    }
}

struct markedglyphs {
    unsigned int has_marks: 1;		/* Are there any marked glyphs in the entire sequence? */
    unsigned int is_cursive: 1;		/* Only in a position sequence */
    unsigned int is_mark: 1;		/* Only in a position sequence/mark keyword=>mark2mark */
    unsigned int is_name: 1;		/* Otherwise a class */
    unsigned int is_lookup: 1;		/* Or a lookup when parsing a subs replacement list */
    uint16 mark_count;			/* 0=>unmarked, 1=>first mark, etc. */
    char *name_or_class;		/* Glyph name / class contents */
    struct vr *vr;			/* A value record. Only in position sequences */
    int ap_cnt;				/* Number of anchor points */
    AnchorPoint **anchors;
    char *lookupname;
    struct markedglyphs *next;
};

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static void fea_ParseDeviceTable(struct parseState *tok,DeviceTable *adjust)
#else
static void fea_ParseDeviceTable(struct parseState *tok)
#endif
	{
    int first = true;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    int min=0, max= -1;
    int8 values[512];

    memset(values,0,sizeof(values));
#endif

    fea_TokenMustBe(tok,tk_device,'\0');
    if ( tok->type!=tk_device )
return;

    forever {
	fea_ParseTok(tok);
	if ( first && tok->type==tk_NULL ) {
	    fea_TokenMustBe(tok,tk_char,'>');
    break;
	} else if ( tok->type==tk_char && tok->tokbuf[0]=='>' ) {
    break;
	} else if ( tok->type!=tk_int ) {
	    LogError(_("Expected integer in device table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    int pixel = tok->value;
#endif
	    fea_TokenMustBe(tok,tk_int,'\0');
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( pixel>=sizeof(values) || pixel<0 )
		LogError(_("Pixel size too big in device table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    else {
		values[pixel] = tok->value;
		if ( max==-1 )
		    min=max=pixel;
		else if ( pixel<min ) min = pixel;
		else if ( pixel>max ) max = pixel;
	    }
#endif
	}
	first = false;
    }
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( max!=-1 ) {
	int i;
	adjust->first_pixel_size = min;
	adjust->last_pixel_size = max;
	adjust->corrections = galloc(max-min+1);
	for ( i=min; i<=max; ++i )
	    adjust->corrections[i-min] = values[i];
    }
#endif
}

static void fea_ParseCaret(struct parseState *tok) {
    int val=0;

    fea_TokenMustBe(tok,tk_caret,'\0');
    if ( tok->type!=tk_caret )
return;
    fea_ParseTok(tok);
    if ( tok->type!=tk_int ) {
	LogError(_("Expected integer in caret on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else
	val = tok->value;
    fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!='>' ) {
	LogError(_("Expected '>' in caret on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
    tok->value = val;
}

static AnchorPoint *fea_ParseAnchor(struct parseState *tok) {
    AnchorPoint *ap = NULL;

    if ( tok->type==tk_anchor ) {
	fea_ParseTok(tok);
	if ( tok->type==tk_NULL )
	    ap = NULL;
	else if ( tok->type==tk_int ) {
	    ap = chunkalloc(sizeof(AnchorPoint));
	    ap->me.x = tok->value;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    ap->me.y = tok->value;
	    fea_ParseTok(tok);
	    if ( tok->type==tk_int ) {
		ap->ttf_pt_index = tok->value;
		ap->has_ttf_pt = true;
		fea_TokenMustBe(tok,tk_char,'>');
	    } else if ( tok->type==tk_char && tok->tokbuf[0]=='<' ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		fea_ParseDeviceTable(tok,&ap->xadjust);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok,&ap->yadjust);
#else
		fea_ParseDeviceTable(tok);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok);
#endif
		fea_TokenMustBe(tok,tk_char,'>');
	    } else if ( tok->type!=tk_char || tok->tokbuf[0]!='>' ) {
		LogError(_("Expected '>' in anchor on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	} else {
	    LogError(_("Expected integer in anchor on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
    } else {
	LogError(_("Expected 'anchor' keyword in anchor on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
return( ap );
}

static int fea_findLookup(struct parseState *tok,char *name ) {
    struct feat_item *feat;

    for ( feat=tok->sofar; feat!=NULL; feat=feat->next ) {
	if ( feat->type==ft_lookup_start && strcmp(name,feat->u1.lookup_name)==0 )
return( true );
    }

    if ( SFFindLookup(tok->sf,name)!=NULL ) {
	if ( !tok->lookup_in_sf_warned ) {
	    ff_post_notice(_("Refers to Font"),_("Reference to a lookup which is not in the feature file but which is in the font, %.50s"), name );
	    tok->lookup_in_sf_warned = true;
	}
return( true );
    }

return( false );
}

static void fea_ParseBroket(struct parseState *tok,struct markedglyphs *last) {
    /* We've read the open broket. Might find: value record, anchor, lookup */
    /* (lookups are my extension) */
    struct vr *vr;

    fea_ParseTok(tok);
    if ( tok->type==tk_lookup ) {
	fea_TokenMustBe(tok,tk_name,'\0');
	if ( last->mark_count==0 ) {
	    LogError(_("Lookups may only be specified after marked glyphs on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
	if ( !fea_findLookup(tok,tok->tokbuf) ) {
	    LogError(_("Lookups must be defined before being used on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else
	    last->lookupname = copy(tok->tokbuf);
	fea_TokenMustBe(tok,tk_char,'>');
    } else if ( tok->type==tk_anchor ) {
	last->anchors = grealloc(last->anchors,(++last->ap_cnt)*sizeof(AnchorPoint *));
	last->anchors[last->ap_cnt-1] = fea_ParseAnchor(tok);
    } else if ( tok->type==tk_NULL ) {
	/* NULL value record. Adobe documents it and doesn't implement it */
	/* Not sure what it's good for */
	fea_TokenMustBe(tok,tk_char,'>');
    } else if ( tok->type==tk_int ) {
	last->vr = vr = chunkalloc(sizeof( struct vr ));
	vr->xoff = tok->value;
	fea_ParseTok(tok);
	if ( tok->type==tk_char && tok->tokbuf[0]=='>' ) {
	    if ( tok->in_vkrn )
		vr->v_adv_off = vr->xoff;
	    else
		vr->h_adv_off = vr->xoff;
	    vr->xoff = 0;
	} else if ( tok->type!=tk_int ) {
	    LogError(_("Expected integer in value record on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else {
	    vr->yoff = tok->value;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    vr->h_adv_off = tok->value;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    vr->v_adv_off = tok->value;
	    fea_ParseTok(tok);
	    if ( tok->type==tk_char && tok->tokbuf[0]=='<' ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		vr->adjust = chunkalloc(sizeof(struct valdev));
		fea_ParseDeviceTable(tok,&vr->adjust->xadjust);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok,&vr->adjust->yadjust);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok,&vr->adjust->xadv);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok,&vr->adjust->yadv);
#else
		fea_ParseDeviceTable(tok);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok);
		fea_TokenMustBe(tok,tk_char,'<');
		fea_ParseDeviceTable(tok);
#endif
		fea_TokenMustBe(tok,tk_char,'>');
	    } else if ( tok->type!=tk_char || tok->tokbuf[0]!='>' ) {
		LogError(_("Expected '>' in value record on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	}
    }
}

static struct markedglyphs *fea_ParseMarkedGlyphs(struct parseState *tok,
	int is_pos, int allow_marks, int allow_lookups) {
    int mark_cnt = 0, last_mark=0, is_cursive = false, is_mark=false;
    struct markedglyphs *head=NULL, *last=NULL, *prev=NULL, *cur;
    int first = true;
    char *contents;

    forever {
	fea_ParseTok(tok);
	cur = NULL;
	if ( first && is_pos && tok->type == tk_cursive )
	    is_cursive = true;
	else if ( first && is_pos && tok->type == tk_mark )
	    is_mark = true;
	else if ( tok->type==tk_name || tok->type == tk_cid ) {
	    if ( tok->type == tk_name )
		contents = fea_glyphname_validate(tok,tok->tokbuf);
	    else
		contents = fea_cid_validate(tok,tok->value);
	    if ( contents!=NULL ) {
		cur = chunkalloc(sizeof(struct markedglyphs));
		cur->is_cursive = is_cursive;
		cur->is_mark = is_mark;
		cur->is_name = true;
		cur->name_or_class = contents;
	    }
	} else if ( tok->type == tk_class || (tok->type==tk_char && tok->tokbuf[0]=='[')) {
	    cur = chunkalloc(sizeof(struct markedglyphs));
	    cur->is_cursive = is_cursive;
	    cur->is_mark = is_mark;
	    cur->is_name = false;
	    cur->name_or_class = fea_ParseGlyphClassGuarded(tok);
	} else if ( allow_marks && tok->type==tk_char &&
		(tok->tokbuf[0]=='\'' || tok->tokbuf[0]=='"') && last!=NULL ) {
	    if ( last_mark!=tok->tokbuf[0] || (prev!=NULL && prev->mark_count==0)) {
		++mark_cnt;
		last_mark = tok->tokbuf[0];
	    }
	    last->mark_count = mark_cnt;
	} else if ( is_pos && last!=NULL && last->vr==NULL && tok->type == tk_int ) {
	    last->vr = chunkalloc(sizeof(struct vr));
	    if ( tok->in_vkrn )
		last->vr->v_adv_off = tok->value;
	    else
		last->vr->h_adv_off = tok->value;
	} else if ( is_pos && last!=NULL && tok->type == tk_char && tok->tokbuf[0]=='<' ) {
	    fea_ParseBroket(tok,last);
	} else if ( !is_pos && allow_lookups && tok->type == tk_char && tok->tokbuf[0]=='<' ) {
	    fea_TokenMustBe(tok,tk_lookup,'\0');
	    fea_TokenMustBe(tok,tk_name,'\0');
	    cur = chunkalloc(sizeof(struct markedglyphs));
	    cur->is_name = false;
	    cur->is_lookup = true;
	    cur->lookupname = copy(tok->tokbuf);
	    fea_TokenMustBe(tok,tk_char,'>');
	} else
    break;
	if ( cur!=NULL ) {
	    prev = last;
	    if ( last==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
	first = false;
    }
    if ( head!=NULL && mark_cnt!=0 )
	head->has_marks = true;
    fea_UnParseTok(tok);
return( head );	
}

static void fea_markedglyphsFree(struct markedglyphs *gl) {
    struct markedglyphs *next;
    int i;

    while ( gl!=NULL ) {
	next = gl->next;
	free(gl->name_or_class);
	free(gl->lookupname);
	for ( i=0; i<gl->ap_cnt; ++i )
	    AnchorPointsFree(gl->anchors[i]);
	free(gl->anchors);
	if ( gl->vr!=NULL ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    ValDevFree(gl->vr->adjust);
#endif
	    chunkfree(gl->vr,sizeof(struct vr));
	}
	gl = next;
    }
}
    
static struct feat_item *fea_AddAllLigPosibilities(struct parseState *tok,struct markedglyphs *glyphs,
	SplineChar *sc,char *sequence_start,char *next, struct feat_item *sofar) {
    char *start, *pt, ch;
    SplineChar *temp;
    char *after;
    struct feat_item *item;

    start = glyphs->name_or_class;
    forever {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	temp = fea_glyphname_get(tok,start);
	*pt = ch; start = pt;
	if ( temp==NULL )
    continue;
	strcpy(next,temp->name);
	after = next+strlen(next);
	if ( glyphs->next!=NULL ) {
	    *after++ = ' ';
	    sofar = fea_AddAllLigPosibilities(tok,glyphs->next,sc,sequence_start,after,sofar);
	} else {
	    *after = '\0';
	    item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_pst;
	    item->next = sofar;
	    sofar = item;
	    item->u1.sc = sc;
	    item->u2.pst = chunkalloc(sizeof(PST));
	    item->u2.pst->type = pst_ligature;
	    item->u2.pst->u.lig.components = copy(sequence_start);
	    item->u2.pst->u.lig.lig = sc;
	}
    }
return( sofar );
}

static struct markedglyphs *fea_glyphs_to_names(struct markedglyphs *glyphs,
	int cnt,char **to) {
    struct markedglyphs *g;
    int len, i;
    char *names, *pt;

    len = 0;
    for ( g=glyphs, i=0; i<cnt; ++i, g=g->next )
	len += strlen( g->name_or_class ) +1;
    names = pt = galloc(len+1);
    for ( g=glyphs, i=0; i<cnt; ++i, g=g->next ) {
	strcpy(pt,g->name_or_class);
	pt += strlen( pt );
	*pt++ = ' ';
    }
    if ( pt!=names )
	pt[-1] = '\0';
    else
	*pt = '\0';
    *to = names;
return( g );
}

static struct feat_item *fea_process_pos_single(struct parseState *tok,
	struct markedglyphs *glyphs, struct feat_item *sofar) {
    char *start, *pt, ch;
    struct feat_item *item;
    SplineChar *sc;

    start = glyphs->name_or_class;
    forever {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	sc = fea_glyphname_get(tok,start);
	*pt = ch; start = pt;
	if ( sc!=NULL ) {
	    item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_pst;
	    item->next = sofar;
	    sofar = item;
	    item->u1.sc = sc;
	    item->u2.pst = chunkalloc(sizeof(PST));
	    item->u2.pst->type = pst_position;
	    item->u2.pst->u.pos = glyphs->vr[0];
	}
    }
return( sofar );
}

static struct feat_item *fea_process_pos_pair(struct parseState *tok,
	struct markedglyphs *glyphs, struct feat_item *sofar, int enumer) {
    char *start, *pt, ch, *start2, *pt2, ch2;
    struct feat_item *item;
    struct vr vr[2];
    SplineChar *sc, *sc2;

    memset(vr,0,sizeof(vr));
    if ( glyphs->vr==NULL )
	vr[0] = *glyphs->next->vr;
    else {
	vr[0] = *glyphs->vr;
	if ( glyphs->next->vr!=NULL )
	    vr[1] = *glyphs->next->vr;
    }
    if ( enumer || (glyphs->is_name && glyphs->next->is_name)) {
	start = glyphs->name_or_class;
	forever {
	    while ( *start==' ' ) ++start;
	    if ( *start=='\0' )
	break;
	    for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	    ch = *pt; *pt = '\0';
	    sc = fea_glyphname_get(tok,start);
	    *pt = ch; start = pt;
	    if ( sc!=NULL ) {
		start2 = glyphs->next->name_or_class;
		forever {
		    while ( *start2==' ' ) ++start2;
		    if ( *start2=='\0' )
		break;
		    for ( pt2=start2; *pt2!='\0' && *pt2!=' '; ++pt2 );
		    ch2 = *pt2; *pt2 = '\0';
		    sc2 = fea_glyphname_get(tok,start2);
		    *pt2 = ch2; start2 = pt2;
		    if ( sc2!=NULL ) {
			item = chunkalloc(sizeof(struct feat_item));
			item->type = ft_pst;
			item->next = sofar;
			sofar = item;
			item->u1.sc = sc;
			item->u2.pst = chunkalloc(sizeof(PST));
			item->u2.pst->type = pst_pair;
			item->u2.pst->u.pair.paired = copy(sc2->name);
			item->u2.pst->u.pair.vr = chunkalloc(sizeof( struct vr[2]));
			memcpy(item->u2.pst->u.pair.vr,vr,sizeof(vr));
		    }
		}
	    }
	}
    } else {
	item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_pstclass;
	item->next = sofar;
	sofar = item;
	item->u1.class = copy(glyphs->name_or_class);
	item->u2.pst = chunkalloc(sizeof(PST));
	item->u2.pst->type = pst_pair;
	item->u2.pst->u.pair.paired = copy(glyphs->next->name_or_class);
	item->u2.pst->u.pair.vr = chunkalloc(sizeof( struct vr[2]));
	memcpy(item->u2.pst->u.pair.vr,vr,sizeof(vr));
    }
return( sofar );
}

static struct feat_item *fea_process_sub_single(struct parseState *tok,
	struct markedglyphs *glyphs, struct markedglyphs *rpl,
	struct feat_item *sofar ) {
    char *start, *pt, ch, *start2, *pt2, ch2;
    struct feat_item *item;
    SplineChar *sc, *temp;

    if ( rpl->is_name ) {
	temp = fea_glyphname_get(tok,rpl->name_or_class);
	if ( temp!=NULL ) {
	    start = glyphs->name_or_class;
	    if ( start==NULL ) {
		LogError(_("Internal state messed up on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
return( sofar );
	    }
	    forever {
		while ( *start==' ' ) ++start;
		if ( *start=='\0' )
	    break;
		for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
		ch = *pt; *pt = '\0';
		sc = fea_glyphname_get(tok,start);
		*pt = ch; start = pt;
		if ( sc!=NULL ) {
		    item = chunkalloc(sizeof(struct feat_item));
		    item->type = ft_pst;
		    item->next = sofar;
		    sofar = item;
		    item->u1.sc = sc;
		    item->u2.pst = chunkalloc(sizeof(PST));
		    item->u2.pst->type = pst_substitution;
		    item->u2.pst->u.subs.variant = copy(temp->name);
		}
	    }
	}
    } else if ( !glyphs->is_name ) {
	start = glyphs->name_or_class;
	start2 = rpl->name_or_class;
	forever {
	    while ( *start==' ' ) ++start;
	    while ( *start2==' ' ) ++start2;
	    if ( *start=='\0' && *start2=='\0' )
	break;
	    else if ( *start=='\0' || *start2=='\0' ) {
		LogError(_("When a single substitution is specified by glyph classes, those classes must be of the same length on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	break;
	    }
	    for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	    ch = *pt; *pt = '\0';
	    for ( pt2=start2; *pt2!='\0' && *pt2!=' '; ++pt2 );
	    ch2 = *pt2; *pt2 = '\0';
	    sc = fea_glyphname_get(tok,start);
	    temp = fea_glyphname_get(tok,start2);
	    *pt = ch; start = pt;
	    *pt2 = ch2; start2 = pt2;
	    if ( sc==NULL || temp==NULL )
	continue;
	    item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_pst;
	    item->next = sofar;
	    sofar = item;
	    item->u1.sc = sc;
	    item->u2.pst = chunkalloc(sizeof(PST));
	    item->u2.pst->type = pst_substitution;
	    item->u2.pst->u.subs.variant = copy(temp->name);
	}
    } else {
	LogError(_("When a single substitution's replacement is specified by a glyph class, the thing being replaced must also be a class on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
return( sofar );
}

static struct feat_item *fea_process_sub_ligature(struct parseState *tok,
	struct markedglyphs *glyphs, struct markedglyphs *rpl,
	struct feat_item *sofar ) {
    SplineChar *sc;
    struct markedglyphs *g;

    /* I store ligatures backwards, in the ligature glyph not the glyphs being substituted */
    sc = fea_glyphname_get(tok,rpl->name_or_class);
    if ( sc!=NULL ) {
	int len=0;
	char *space;
	for ( g=glyphs; g!=NULL && g->mark_count==glyphs->mark_count; g=g->next )
	    len += strlen(g->name_or_class)+1;
	space = galloc(len+1);
	sofar = fea_AddAllLigPosibilities(tok,glyphs,sc,space,space,sofar);
	free(space);
    }
return( sofar );
}

static FPST *fea_markedglyphs_to_fpst(struct parseState *tok,struct markedglyphs *glyphs,
	int is_pos,int is_ignore) {
    struct markedglyphs *g;
    int bcnt=0, ncnt=0, fcnt=0, cnt;
    int all_single=true;
    int mmax = 0;
    int i;
    FPST *fpst;
    struct fpst_rule *r;
    struct feat_item *item, *head = NULL;

    for ( g=glyphs; g!=NULL && g->mark_count==0; g=g->next ) {
	++bcnt;
	if ( !g->is_name ) all_single = false;
    }
    for ( ; g!=NULL ; g=g->next ) {
	if ( !g->is_name ) all_single = false;
	if ( g->mark_count==0 )
	    ++fcnt;
	else {
	    /* if we found some unmarked glyphs between two runs of marked */
	    /*  they don't count as lookaheads */
	    ncnt += fcnt + 1;
	    fcnt = 0;
	    if ( g->mark_count>mmax ) mmax = g->mark_count;
	}
    }

    fpst = chunkalloc(sizeof(FPST));
    fpst->type = is_pos ? pst_chainpos : pst_chainsub;
    fpst->format = all_single ? pst_glyphs : pst_coverage;
    fpst->rule_cnt = 1;
    fpst->rules = r = gcalloc(1,sizeof(struct fpst_rule));
    if ( is_ignore )
	mmax = 0;
    r->lookup_cnt = mmax;
    r->lookups = gcalloc(mmax,sizeof(struct seqlookup));
    for ( i=0; i<mmax; ++i )
	r->lookups[i].seq = i;

    if ( all_single ) {
	g = fea_glyphs_to_names(glyphs,bcnt,&r->u.glyph.back);
	g = fea_glyphs_to_names(g,ncnt,&r->u.glyph.names);
	g = fea_glyphs_to_names(g,fcnt,&r->u.glyph.fore);
    } else {
	r->u.coverage.ncnt = ncnt;
	r->u.coverage.bcnt = bcnt;
	r->u.coverage.fcnt = fcnt;
	r->u.coverage.ncovers = galloc(ncnt*sizeof(char*));
	r->u.coverage.bcovers = galloc(bcnt*sizeof(char*));
	r->u.coverage.fcovers = galloc(fcnt*sizeof(char*));
	for ( i=0, g=glyphs; i<bcnt; ++i, g=g->next )
	    r->u.coverage.bcovers[i] = copy(g->name_or_class);
	for ( i=0; i<ncnt; ++i, g=g->next )
	    r->u.coverage.ncovers[i] = copy(g->name_or_class);
	for ( i=0; i<fcnt; ++i, g=g->next )
	    r->u.coverage.fcovers[i] = copy(g->name_or_class);
    }

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_fpst;
    item->next = tok->sofar;
    tok->sofar = item;
    item->u2.fpst = fpst;

    if ( is_pos ) {
	for ( g=glyphs; g!=NULL && g->mark_count==0; g=g->next );
	for ( i=0; g!=NULL; ++i ) {
	    head = NULL;
	    if ( g->lookupname!=NULL ) {
		head = chunkalloc(sizeof(struct feat_item));
		head->type = ft_lookup_ref;
		head->u1.lookup_name = copy(g->lookupname);
	    } else if ( (g->next==NULL || g->next->mark_count!=g->mark_count) &&
		    g->vr!=NULL ) {
		head = fea_process_pos_single(tok,g,NULL);
	    } else if ( g->next!=NULL && g->mark_count==g->next->mark_count &&
		    (g->vr!=NULL || g->next->vr!=NULL ) &&
		    ( g->next->next==NULL || g->next->next->mark_count!=g->mark_count)) {
		head = fea_process_pos_pair(tok,g,NULL,false);
	    } else {
		LogError(_("Unparseable contextual sequence on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    r->lookups[i].lookup = (OTLookup *) head;
	    cnt = g->mark_count;
	    while ( g!=NULL && g->mark_count == cnt )	/* skip everything involved here */
		g=g->next;
	    for ( ; g!=NULL && g->mark_count==0; g=g->next ); /* skip any uninvolved glyphs */
	}
    }

return( fpst );
}

static void fea_ParseIgnore(struct parseState *tok) {
    struct markedglyphs *glyphs;
    int is_pos;
    FPST *fpst;
    /* ignore [pos|sub] <marked glyph sequence> (, <marked g sequence>)* */

    fea_ParseTok(tok);
    if ( tok->type==tk_position )
	is_pos = true;
    else if ( tok->type == tk_substitute )
	is_pos = false;
    else {
	LogError(_("The ignore keyword must be followed by either position or substitute on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	is_pos = true;
    }
    forever {
	glyphs = fea_ParseMarkedGlyphs(tok,false/* don't parse value records, etc*/,
		true/*allow marks*/,false/* no lookups */);
	fpst = fea_markedglyphs_to_fpst(tok,glyphs,false,true);
	if ( is_pos )
	    fpst->type = pst_chainpos;
	fea_markedglyphsFree(glyphs);
	fea_ParseTok(tok);
	if ( tok->type!=tk_char || tok->tokbuf[0]!=',' )
    break;
    }
	    
    fea_now_semi(tok);
}    
    
static void fea_ParseSubstitute(struct parseState *tok) {
    /* name by name => single subs */
    /* class by name => single subs */
    /* class by class => single subs */
    /* name by <glyph sequence> => multiple subs */
    /* name from <class> => alternate subs */
    /* <glyph sequence> by name => ligature */
    /* <marked glyph sequence> by <name> => context chaining */
    /* <marked glyph sequence> by <lookup name>* => context chaining */
    /* [ignore sub] <marked glyph sequence> (, <marked g sequence>)* */
    struct markedglyphs *glyphs = fea_ParseMarkedGlyphs(tok,false,true,false),
	    *g, *rpl, *rp;
    int cnt, i;
    SplineChar *sc;
    struct feat_item *item, *head;

    fea_ParseTok(tok);
    for ( cnt=0, g=glyphs; g!=NULL; g=g->next, ++cnt );
    if ( glyphs==NULL ) {
	LogError(_("Empty subsitute on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else if ( !glyphs->has_marks ) {
	/* Non-contextual */
	if ( cnt==1 && glyphs->is_name && tok->type==tk_from ) {
	    /* Alternate subs */
	    char *alts;
	    fea_ParseTok(tok);
	    alts = fea_ParseGlyphClassGuarded(tok);
	    sc = fea_glyphname_get(tok,glyphs->name_or_class);
	    if ( sc!=NULL ) {
		item = chunkalloc(sizeof(struct feat_item));
		item->type = ft_pst;
		item->next = tok->sofar;
		tok->sofar = item;
		item->u1.sc = sc;
		item->u2.pst = chunkalloc(sizeof(PST));
		item->u2.pst->type = pst_alternate;
		item->u2.pst->u.alt.components = alts;
	    }
	} else if ( cnt>=1 && tok->type==tk_by ) {
	    rpl = fea_ParseMarkedGlyphs(tok,false,false,false);
	    if ( rpl==NULL ) {
		LogError(_("No substitution specified on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    } else if ( rpl->has_marks ) {
		LogError(_("No marked glyphs allowed in replacement on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    } else {
		if ( cnt==1 && rpl->next==NULL ) {
		    tok->sofar = fea_process_sub_single(tok,glyphs,rpl,tok->sofar);
		} else if ( cnt==1 && glyphs->is_name && rpl->next!=NULL && rpl->is_name ) {
		    /* Multiple substitution */
		    int len=0;
		    char *mult;
		    for ( g=rpl; g!=NULL; g=g->next )
			len += strlen(g->name_or_class)+1;
		    mult = galloc(len+1);
		    len = 0;
		    for ( g=rpl; g!=NULL; g=g->next ) {
			strcpy(mult+len,g->name_or_class);
			len += strlen(g->name_or_class);
			mult[len++] = ' ';
		    }
		    mult[len-1] = '\0';
		    sc = fea_glyphname_get(tok,glyphs->name_or_class);
		    if ( sc!=NULL ) {
			item = chunkalloc(sizeof(struct feat_item));
			item->type = ft_pst;
			item->next = tok->sofar;
			tok->sofar = item;
			item->u1.sc = sc;
			item->u2.pst = chunkalloc(sizeof(PST));
			item->u2.pst->type = pst_multiple;
			item->u2.pst->u.mult.components = mult;
		    }
		} else if ( cnt>1 && rpl->is_name && rpl->next==NULL ) {
		    tok->sofar = fea_process_sub_ligature(tok,glyphs,rpl,tok->sofar);
		    /* Ligature */
		} else {
		    LogError(_("Unparseable glyph sequence in substitution on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		}
	    }
	    fea_markedglyphsFree(rpl);
	} else {
	    LogError(_("Expected 'by' or 'from' keywords in substitution on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
    } else {
	/* Contextual */
	FPST *fpst = fea_markedglyphs_to_fpst(tok,glyphs,false,false);
	struct fpst_rule *r = fpst->rules;
	if ( tok->type!=tk_by ) {
	    LogError(_("Expected 'by' keyword in substitution on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
	rpl = fea_ParseMarkedGlyphs(tok,false,false,true);
	for ( g=glyphs; g!=NULL && g->mark_count==0; g=g->next );
	for ( i=0, rp=rpl; g!=NULL && rp!=NULL; ++i, rp=rp->next ) {
	    if ( rp->lookupname!=NULL ) {
		head = chunkalloc(sizeof(struct feat_item));
		head->type = ft_lookup_ref;
		head->u1.lookup_name = copy(rp->lookupname);
	    } else if ( g->next==NULL || g->next->mark_count!=g->mark_count ) {
		head = fea_process_sub_single(tok,g,rp,NULL);
	    } else if ( g->next!=NULL && g->mark_count==g->next->mark_count ) {
		head = fea_process_sub_ligature(tok,g,rpl,NULL);
	    } else {
		LogError(_("Unparseable contextual sequence on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    r->lookups[i].lookup = (OTLookup *) head;
	    cnt = g->mark_count;
	    while ( g!=NULL && g->mark_count == cnt )	/* skip everything involved here */
		g=g->next;
	    for ( ; g!=NULL && g->mark_count!=0; g=g->next ); /* skip any uninvolved glyphs */
	}
	
	fea_markedglyphsFree(rpl);
    }
		    
    fea_end_statement(tok);
    fea_markedglyphsFree(glyphs);
}

static void fea_ParseMarks(struct parseState *tok) {
    /* mark name|class <anchor> */
    char *contents = NULL;
    SplineChar *sc = NULL;
    AnchorPoint *ap;
    char *start, *pt;
    int ch;

    fea_ParseTok(tok);
    if ( tok->type==tk_name )
	sc = fea_glyphname_get(tok,tok->tokbuf);
    else if ( tok->type==tk_class )
	contents = fea_lookup_class_complain(tok,tok->tokbuf);
    else if ( tok->type==tk_char && tok->tokbuf[0]=='[' )
	contents = fea_ParseGlyphClass(tok);
    else {
	LogError(_("Expected glyph name or class in mark statement on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    if ( sc==NULL && contents==NULL ) {
	fea_skip_to_semi(tok);
return;
    }

    fea_TokenMustBe(tok,tk_char,'<');
    fea_TokenMustBe(tok,tk_anchor,'\0');
    ap = fea_ParseAnchor(tok);
    ap->type = at_mark;
    fea_end_statement(tok);

    if ( ap!=NULL ) {
	pt = contents;
	forever {
	    struct feat_item *item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_ap;
	    item->u2.ap = ap;
	    item->next = tok->sofar;
	    tok->sofar = item;
	    start = pt;
	    if ( contents==NULL ) {
		item->u1.sc = sc;
	break;
	    }
	    while ( *pt!='\0' && *pt!=' ' )
		++pt;
	    ch = *pt; *pt = '\0';
	    sc = fea_glyphname_get(tok,start);
	    *pt = ch;
	    while ( isspace(*pt)) ++pt;
	    if ( sc==NULL ) {
		tok->sofar = item->next;	/* Oops, remove it */
		chunkfree(item,sizeof(*item));
		if ( *pt=='\0' ) {
		    AnchorPointsFree(ap);
	break;
		}
	    } else {
		item->u1.sc = sc;
		if ( *pt=='\0' )
	break;
		ap = AnchorPointsCopy(ap);
	    }
	}
    }
    free(contents);
}

static void fea_ParsePosition(struct parseState *tok, int enumer) {
    /* name <vr> => single pos */
    /* class <vr> => single pos */
    /* name|class <vr> name|class <vr> => pair pos */
    /* name|class name|class <vr> => pair pos */
    /* cursive name|class <anchor> <anchor> => cursive positioning */
    /* name|class <anchor> mark name|class => mark to base pos */
	/* Must be preceded by a mark statement */
    /* name|class <anchor> <anchor>+ mark name|class => mark to ligature pos */
	/* Must be preceded by a mark statement */
    /* mark name|class <anchor> mark name|class => mark to base pos */
	/* Must be preceded by a mark statement */
    /* <marked glyph pos sequence> => context chaining */
    /* [ignore pos] <marked glyph sequence> (, <marked g sequence>)* */
    struct markedglyphs *glyphs = fea_ParseMarkedGlyphs(tok,true,true,false), *g;
    int cnt, i;
    struct feat_item *item;
    char *start, *pt, ch;
    SplineChar *sc;

    fea_ParseTok(tok);
    for ( cnt=0, g=glyphs; g!=NULL; g=g->next, ++cnt );
    if ( glyphs==NULL ) {
	LogError(_("Empty position on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else if ( !glyphs->has_marks ) {
	/* Non-contextual */
	if ( glyphs->is_cursive ) {
	    if ( cnt!=1 || glyphs->ap_cnt!=2 ) {
		LogError(_("Invalid cursive position on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    } else {
		start = glyphs->name_or_class;
		if ( glyphs->anchors[1]!=NULL )
		    glyphs->anchors[1]->type = at_cexit;
		forever {
		    while ( *start==' ' ) ++start;
		    if ( *start=='\0' )
		break;
		    for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
		    ch = *pt; *pt = '\0';
		    sc = fea_glyphname_get(tok,start);
		    *pt = ch; start = pt;
		    if ( sc!=NULL ) {
			item = chunkalloc(sizeof(struct feat_item));
			item->type = ft_ap;
			item->next = tok->sofar;
			tok->sofar = item;
			item->u1.sc = sc;
			if ( glyphs->anchors[0]!=NULL ) {
			    glyphs->anchors[0]->type = at_centry;
			    glyphs->anchors[0]->next = glyphs->anchors[1];
			    item->u2.ap = AnchorPointsCopy(glyphs->anchors[0]);
			} else
			    item->u2.ap = AnchorPointsCopy(glyphs->anchors[1]);
		    }
		}
	    }
	} else if ( cnt==1 && glyphs->vr!=NULL ) {
	    tok->sofar = fea_process_pos_single(tok,glyphs,tok->sofar);
	} else if ( cnt==2 && (glyphs->vr!=NULL || glyphs->next->vr!=NULL) ) {
	    tok->sofar = fea_process_pos_pair(tok,glyphs,tok->sofar, enumer);
	} else if ( cnt==1 && glyphs->ap_cnt>=1 && tok->type == tk_mark ) {
	    /* Mark to base, mark to mark, mark to ligature */
	    char *mark_class;
	    AnchorPoint *head=NULL, *last=NULL;
	    if ( tok->type!=tk_mark ) {
		LogError(_("A mark glyph (or class of marks) must be specified here on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    fea_ParseTok(tok);
	    if ( tok->type==tk_name )
		mark_class = copy(tok->tokbuf);
	    else
		mark_class = fea_canonicalClassOrder(fea_ParseGlyphClassGuarded(tok));
	    fea_ParseTok(tok);
	    if ( glyphs->is_mark && glyphs->ap_cnt>1 ) {
		LogError(_("Mark to base anchor statements may only have one anchor on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
	    }
	    if ( mark_class!=NULL ) {
		for ( i=0; i<glyphs->ap_cnt; ++i ) {
		    if ( glyphs->anchors[i]==NULL )
			/* Nothing to be done */;
		    else {
			if ( glyphs->ap_cnt>1 ) {
			    glyphs->anchors[i]->type = at_baselig;
			    glyphs->anchors[i]->lig_index = i;
			} else if ( glyphs->is_mark )
			    glyphs->anchors[i]->type = at_basemark;
			else
			    glyphs->anchors[i]->type = at_basechar;
			if ( head==NULL )
			    head = glyphs->anchors[i];
			else
			    last->next = glyphs->anchors[i];
			last = glyphs->anchors[i];
		    }
		}

		start = glyphs->name_or_class;
		forever {
		    while ( *start==' ' ) ++start;
		    if ( *start=='\0' )
		break;
		    for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
		    ch = *pt; *pt = '\0';
		    sc = fea_glyphname_get(tok,start);
		    *pt = ch; start = pt;
		    if ( sc!=NULL ) {
			item = chunkalloc(sizeof(struct feat_item));
			item->type = ft_ap;
			item->next = tok->sofar;
			tok->sofar = item;
			item->u1.sc = sc;
			item->u2.ap = AnchorPointsCopy(head);
			item->mark_class = copy(mark_class);
		    }
		}

		/* So we can free them properly */
		for ( ; head!=NULL; head = last ) {
		    last = head->next;
		    head->next = NULL;
		}
	    }
	} else {
	    LogError(_("Unparseable glyph sequence in position on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	}
    } else {
	/* Contextual */
	(void) fea_markedglyphs_to_fpst(tok,glyphs,true,false);
    }
    fea_now_semi(tok);
    fea_markedglyphsFree(glyphs);
}

static enum otlookup_type fea_LookupTypeFromItem(struct feat_item *item) {
    switch ( item->type ) {
      case ft_pst: case ft_pstclass:
	switch ( item->u2.pst->type ) {
	  case pst_position:
return( gpos_single );
	  case pst_pair:
return( gpos_pair );
	  case pst_substitution:
return( gsub_single );
	  case pst_alternate:
return( gsub_alternate );
	  case pst_multiple:
return( gsub_multiple );
	  case pst_ligature:
return( gsub_ligature );
	  default:
return( ot_undef );		/* Can't happen */
	}
      break;
      case ft_ap:
	switch( item->u2.ap->type ) {
	  case at_centry: case at_cexit:
return( gpos_cursive );
	  case at_mark:
return( ot_undef );		/* Can be used in three different lookups. Not enough info */
	  case at_basechar:
return( gpos_mark2base );
	  case at_baselig:
return( gpos_mark2ligature );
	  case at_basemark:
return( gpos_mark2mark );
	  default:
return( ot_undef );		/* Can't happen */
	}
      break;
      case ft_fpst:
	switch( item->u2.fpst->type ) {
	  case pst_chainpos:
return( gpos_contextchain );
	  case pst_chainsub:
return( gsub_contextchain );
	  default:
return( ot_undef );		/* Can't happen */
	}
      break;
      default:
return( ot_undef );		/* Can happen */
    }
}

static struct feat_item *fea_AddFeatItem(struct parseState *tok,enum feat_type type,uint32 tag) {
    struct feat_item *item;

    item = chunkalloc(sizeof(struct feat_item));
    item->type = type;
    item->u1.tag = tag;
    item->next = tok->sofar;
    tok->sofar = item;
return( item );
}

static int fea_LookupSwitch(struct parseState *tok) {
    int enumer = false;

    switch ( tok->type ) {
      case tk_class:
	fea_ParseGlyphClassDef(tok);
      break;
      case tk_lookupflag:
	fea_ParseLookupFlags(tok);
      break;
      case tk_mark:
	fea_ParseMarks(tok);
      break;
      case tk_ignore:
	fea_ParseIgnore(tok);
      break;
      case tk_enumerate:
	fea_TokenMustBe(tok,tk_position,'\0');
	enumer = true;
	/* Fall through */;  
      case tk_position:
	fea_ParsePosition(tok,enumer);
      break;
      case tk_substitute:
	fea_ParseSubstitute(tok);
	enumer = false;
      break;
      case tk_subtable:
	fea_AddFeatItem(tok,ft_subtable,0);
	fea_TokenMustBe(tok,tk_char,';');
      break;
      case tk_char:
	if ( tok->tokbuf[0]=='}' )
return( 2 );
	/* Fall through */
      default:
return( 0 );
    }
return( 1 );
}

static void fea_ParseLookupDef(struct parseState *tok, int could_be_stat ) {
    char *lookup_name;
    struct feat_item *item, *first_after_mark;
    enum otlookup_type lookuptype;
    int has_marks;
    int ret;

    fea_ParseTok(tok);
    if ( tok->type!=tk_name ) {
	LogError(_("Expected name in lookup on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    lookup_name = copy(tok->tokbuf);
    fea_ParseTok(tok);
    if ( could_be_stat && tok->type==tk_char && tok->tokbuf[0]==';' ) {
	item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_lookup_ref;
	item->u1.lookup_name = lookup_name;
	item->next = tok->sofar;
	tok->sofar = item;
return;
    } else if ( tok->type==tk_useExtension )		/* I just ignore this */
	fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!='{' ) {
	LogError(_("Expected '{' in feature definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_lookup_start;
    item->u1.lookup_name = lookup_name;
    item->next = tok->sofar;
    tok->sofar = item;

    first_after_mark = NULL;
    forever {
	fea_ParseTok(tok);
	if ( tok->err_count>100 )
    break;
	if ( tok->type==tk_eof ) {
	    LogError(_("Unexpected end of file in lookup definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
return;
	} else if ( (ret = fea_LookupSwitch(tok))==0 ) {
	    LogError(_("Unexpected token, %s, in lookup definition on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    if ( tok->type==tk_name && strcmp(tok->tokbuf,"subs")==0 )
		LogError(_(" Perhaps you meant to use the keyword 'sub' rather than 'subs'?") );
	    ++tok->err_count;
return;
	} else if ( ret==2 )
    break;
	/* Ok, mark classes must either contain exactly the same glyphs, or */
	/*  they may not intersect at all */
	/* Mark2* lookups are not well documented (because adobe FDK doesn't */
	/*  support them) but I'm going to assume that if I have some mark */
	/*  statements, then some pos statement, then another mark statement */
	/*  that I begin a new subtable with the second set of marks (and a */
	/*  different set of mark classes) */
	if ( tok->sofar!=NULL && tok->sofar->type==ft_subtable )
	    first_after_mark = NULL;
	else if ( tok->sofar!=NULL && tok->sofar->type==ft_ap ) {
	    if ( tok->sofar->u2.ap->type == at_mark )
		first_after_mark = NULL;
	    else if ( tok->sofar->mark_class==NULL )
		/* we don't have to worry about Cursive connections */;
	    else if ( first_after_mark == NULL )
		first_after_mark = tok->sofar;
	    else {
		struct feat_item *f;
		for ( f = tok->sofar->next; f!=NULL; f=f->next ) {
		    if ( f->type==ft_lookup_start || f->type==ft_subtable )
		break;
		    if ( f->type!=ft_ap || f->mark_class==NULL )
		continue;
		    if ( strcmp(tok->sofar->mark_class,f->mark_class)==0 )
		continue;		/* same glyphs, that's ok */
		    else if ( fea_classesIntersect(tok->sofar->mark_class,f->mark_class)) {
			LogError(_("Mark classes must either be exactly the same or contain no common glyphs\n But the class on line %d of %s contains a match."),
				tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
			++tok->err_count;
		    }
		    if ( f==first_after_mark )
		break;
		}
	    }
	}
    }
    fea_ParseTok(tok);
    if ( tok->type!=tk_name || strcmp(tok->tokbuf,lookup_name)!=0 ) {
	LogError(_("Expected %s in lookup definition on line %d of %s"),
		lookup_name, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
    fea_end_statement(tok);

    /* Make sure all entries in this lookup of the same lookup type */
    lookuptype = ot_undef;
    has_marks = false;
    for ( item=tok->sofar ; item!=NULL && item->type!=ft_lookup_start; item=item->next ) {
	enum otlookup_type cur = fea_LookupTypeFromItem(item);
	if ( item->type==ft_ap && item->u2.ap->type == at_mark )
	    has_marks = true;
	if ( cur==ot_undef )	/* Some entries in the list (lookupflags) have no type */
	    /* Tum, ty, tum tum */;
	else if ( lookuptype==ot_undef )
	    lookuptype = cur;
	else if ( lookuptype!=cur ) {
	    LogError(_("All entries in a lookup must have the same type on line %d of %s"),
		    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
    break;
	}
    }
    if ( lookuptype==ot_undef ) {
	LogError(_("This lookup has no effect, I can't figure out its type on line %d of %s"),
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    } else if ( has_marks && lookuptype!=gpos_mark2base &&
		lookuptype!=gpos_mark2mark &&
		lookuptype!=gpos_mark2ligature ) {
	LogError(_("Mark glyphs may not be specified with this type of lookup on line %d of %s"),
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_lookup_end;
    /* item->u1.lookup_name = lookup_name; */
    item->next = tok->sofar;
    tok->sofar = item;
}

static struct nameid *fea_ParseNameId(struct parseState *tok,int strid) {
    int platform = 3, specific = 1, language = 0x409;
    struct nameid *nm;
    char *start, *pt;
    int max, ch, value;
    FILE *in = tok->inlist[tok->inc_depth];
    /* nameid <id> [<string attibute>] string; */
    /*  "nameid" and <id> will already have been parsed when we get here */
    /* <string attribute> := <platform> | <platform> <specific> <language> */
    /* <patform>==3 => <specific>=1 <language>=0x409 */
    /* <platform>==1 => <specific>=0 <lang>=0 */
    /* string in double quotes \XXXX escapes to UCS2 (for 3) */
    /* string in double quotes \XX escapes to MacLatin (for 1) */
    /* I only store 3,1 strings */

    fea_ParseTok(tok);
    if ( tok->type == tk_int ) {
	if ( tok->value!=3 && tok->value!=1 ) {
	    LogError(_("Invalid platform for string on line %d of %s"),
		    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else if ( tok->value==1 ) {
	    specific = language = 0;
	}
	fea_ParseTok(tok);
	if ( tok->type == tk_int ) {
	    specific = tok->value;
	    tok->base = 0;
	    fea_TokenMustBe(tok,tk_int,'\0');
	    language = tok->value;
	    tok->base = 10;
	    fea_ParseTok(tok);
	}
    }
    if ( tok->type!=tk_char || tok->tokbuf[0]!='"' ) {
	LogError(_("Expected string on line %d of %s"),
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
	nm = NULL;
    } else {
	if ( platform==3 && specific==1 ) {
	    nm = chunkalloc(sizeof(struct nameid));
	    nm->strid = strid;
	    nm->platform = platform;
	    nm->specific = specific;
	    nm->language = language;
	} else
	    nm = NULL;
	max = 0;
	pt = start = NULL;
	while ( (ch=getc(in))!=EOF && ch!='"' ) {
	    if ( ch=='\n' || ch=='\r' )
	continue;		/* Newline characters are ignored here */
				/*  may be specified with backslashes */
	    if ( ch=='\\' ) {
		int i, dmax = platform==3 ? 4 : 2;
		value = 0;
		for ( i=0; i<dmax; ++i ) {
		    ch = getc(in);
		    if ( !ishexdigit(ch)) {
			ungetc(ch,in);
		break;
		    }
		    if ( ch>='a' && ch<='f' )
			ch -= ('a'-10);
		    else if ( ch>='A' && ch<='F' )
			ch -= ('A'-10);
		    else
			ch -= '0';
		    value <<= 4;
		    value |= ch;
		}
	    } else
		value = ch;
	    if ( nm!=NULL ) {
		if ( pt-start+3>=max ) {
		    int off = pt-start;
		    start = grealloc(start,(max+=100)+1);
		    pt = start+off;
		}
		pt = utf8_idpb(pt,value);
	    }
	}
	if ( nm!=NULL ) {
	    if ( pt==NULL )
		nm->utf8_str = copy("");
	    else {
		*pt = '\0';
		nm->utf8_str = copy(start);
		free(start);
	    }
	}
	if ( tok->type!=tk_char || tok->tokbuf[0]!='"' ) {
	    LogError(_("End of file found in string on line %d of %s"),
		    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	} else
	    fea_end_statement(tok);
    }
return( nm );
}

static struct feat_item *fea_ParseParameters(struct parseState *tok, struct feat_item *feat) {
    /* Ok. The only time a parameter keyword may be used is inside a 'size' */
    /*  feature and then it takes 4 numbers */
    /* The first, third and fourth are in decipoints and may be either */
    /*  integers or floats (in which case we must multiply them by 10) */
    int params[4];
    int i;

    memset(params,0,sizeof(params));
    for ( i=0; i<4; ++i ) {
	params[i] = fea_ParseDeciPoints(tok);
	if ( tok->type==tk_char && tok->tokbuf[0]==';' )
    break;
    }
    fea_end_statement(tok);

    if ( feat==NULL ) {
	feat = chunkalloc(sizeof(struct feat_item));
	feat->type = ft_sizeparams;
	feat->next = tok->sofar;
	tok->sofar = feat;
    }
    feat->u1.params = galloc(sizeof(params));
    memcpy(feat->u1.params,params,sizeof(params));
return( feat );
}

static struct feat_item *fea_ParseSizeMenuName(struct parseState *tok, struct feat_item *feat) {
    /* Sizemenuname takes either 0, 1 or 3 numbers and a string */
    /* if no numbers are given (or the first number is 3) then the string is */
    /*  unicode. Otherwise a mac encoding, treated as single byte */
    /* Since fontforge only supports windows strings here I shall parse and */
    /*  ignore mac strings */
    struct nameid *string;

    string = fea_ParseNameId(tok,-1);

    if ( string!=NULL ) {
	if ( feat==NULL ) {
	    feat = chunkalloc(sizeof(struct feat_item));
	    feat->type = ft_sizeparams;
	    feat->next = tok->sofar;
	    tok->sofar = feat;
	}
	string->next = feat->u2.names;
	feat->u2.names = string;
    }
return( feat );
}

static void fea_ParseFeatureDef(struct parseState *tok) {
    uint32 feat_tag;
    struct feat_item *item, *size_item = NULL;
    int type, ret;

    fea_ParseTok(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag ) {
	LogError(_("Expected tag in feature on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    feat_tag = tok->tag;
    tok->in_vkrn = feat_tag == CHR('v','k','r','n');

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_feat_start;
    item->u1.tag = feat_tag;
    if ( tok->def_langsyses!=NULL )
	item->u2.sl = SListCopy(tok->def_langsyses);
    else {
	item->u2.sl = chunkalloc(sizeof(struct scriptlanglist));
	item->u2.sl->script = DEFAULT_SCRIPT;
	item->u2.sl->lang_cnt = 1;
	item->u2.sl->langs[0] = DEFAULT_LANG;
    }
    item->next = tok->sofar;
    tok->sofar = item;

    fea_ParseTok(tok);
    if ( tok->type!=tk_char || tok->tokbuf[0]!='{' ) {
	LogError(_("Expected '{' in feature definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }

    forever {
	fea_ParseTok(tok);
	if ( tok->err_count>100 )
    break;
	if ( tok->type==tk_eof ) {
	    LogError(_("Unexpected end of file in feature definition on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
return;
	} else if ( (ret = fea_LookupSwitch(tok))==0 ) {
	    switch ( tok->type ) {
	      case tk_lookup:
		fea_ParseLookupDef(tok,true);
	      break;
	      case tk_languagesystem:
		fea_ParseLangSys(tok,true);
	      break;
	      case tk_feature:
		/* can appear inside an 'aalt' feature. I don't support it, but */
		/*  just parse and ignore it */
		if ( feat_tag!=CHR('a','a','l','t')) {
		    LogError(_("Features inside of other features are only permitted for 'aalt' features on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		}
		fea_ParseTok(tok);
		if ( tok->type!=tk_name || !tok->could_be_tag ) {
		    LogError(_("Expected tag on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		}
		fea_end_statement(tok);
	      break;
	      case tk_script:
	      case tk_language:
		/* If no lang specified after script use 'dflt', if no script specified before a language use 'latn' */
		type = tok->type==tk_script ? ft_script : ft_lang;
		fea_ParseTok(tok);
		if ( tok->type!=tk_name || !tok->could_be_tag ) {
		    LogError(_("Expected tag on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		    ++tok->err_count;
		} else {
		    item = fea_AddFeatItem(tok,type,tok->tag);
		    if ( type==ft_lang ) {
			forever {
			    fea_ParseTok(tok);
			    if ( tok->type==tk_include_dflt )
				/* Unneeded */;
			    else if ( tok->type==tk_exclude_dflt )
				item->u2.exclude_dflt = true;
			    else if ( tok->type==tk_required )
				/* Not supported by adobe (or me) */;
			    else if ( tok->type==tk_char && tok->tokbuf[0]==';' )
			break;
			    else {
				LogError(_("Expected ';' on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
				++tok->err_count;
			break;
			    }
			}
		    } else
			fea_end_statement(tok);
		}
	      break;
	      case tk_parameters:
		if ( feat_tag==CHR('s','i','z','e') ) {
		    size_item = fea_ParseParameters(tok, size_item);
	      break;
		}
		/* Fall on through */
	      case tk_name:
		if ( feat_tag==CHR('s','i','z','e') && strcmp(tok->tokbuf,"sizemenuname")==0 ) {
		    size_item = fea_ParseSizeMenuName(tok, size_item);
	      break;
		}
		/* Fall on through */
	      default:
		LogError(_("Unexpected token, %s, in feature definition on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
return;
	    }
	} else if ( ret==2 )
    break;
    }
    
    fea_ParseTok(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag || tok->tag!=feat_tag ) {
	LogError(_("Expected '%c%c%c%c' in lookup definition on line %d of %s"),
		feat_tag>>24, feat_tag>>16, feat_tag>>8, feat_tag,
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
    fea_end_statement(tok);

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_feat_end;
    item->u1.tag = feat_tag;
    item->next = tok->sofar;
    tok->sofar = item;

    tok->in_vkrn = false;
}

static void fea_ParseNameTable(struct parseState *tok) {
    struct nameid *head=NULL, *string;
    struct feat_item *item;
    /* nameid <id> [<string attibute>] string; */

    forever {
	fea_ParseTok(tok);
	if ( tok->type != tk_nameid )
    break;
	fea_TokenMustBe(tok,tk_int,'\0');
	string = fea_ParseNameId(tok,tok->value);
	if ( string!=NULL ) {
	    string->next = head;
	    head = string;
	}
    }

    if ( head!=NULL ) {
	item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_names;
	item->next = tok->sofar;
	tok->sofar = item;
	item->u2.names = head;
    }
    if ( tok->type!=tk_char || tok->tokbuf[0]!='}' ) {
	LogError(_("Expected closing curly brace on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
    }
}

static void fea_ParseTableKeywords(struct parseState *tok, struct tablekeywords *keys) {
    int index;
    struct tablevalues *tv, *head = NULL;
    int i;
    struct feat_item *item;

    forever {
	fea_ParseTok(tok);
	if ( tok->type != tk_name )
    break;
	for ( index=0; keys[index].name!=NULL; ++index ) {
	    if ( strcmp(keys[index].name,tok->tokbuf)==0 )
	break;
	}
	if ( keys[index].name==NULL ) {
	    LogError(_("Unknown field %s on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	    index = -1;
	}
	if ( index!=-1 && keys[index].offset!=-1 ) {
	    tv = chunkalloc(sizeof(struct tablevalues));
	    tv->index = index;
	} else
	    tv = NULL;
	fea_ParseTok(tok);
	if ( strcmp(tok->tokbuf,"Vendor")==0 && tv!=NULL) {
	    /* This takes a 4 character string */
	    /* of course strings aren't part of the syntax, but it takes one anyway */
	    if ( tok->type==tk_name && tok->could_be_tag )
		/* Accept a normal tag, since that's what it really is */
		tv->value = tok->tag;
	    else if ( tok->type==tk_char && tok->tokbuf[0]=='"' ) {
		uint8 foo[4]; int ch;
		FILE *in = tok->inlist[tok->inc_depth];
		memset(foo,' ',sizeof(foo));
		for ( i=0; i<4; ++i ) {
		    ch = getc(in);
		    if ( ch==EOF )
		break;
		    else if ( ch=='"' ) {
			ungetc(ch,in);
		break;
		    }
		    foo[i] = ch;
		}
		while ( (ch=getc(in))!=EOF && ch!='"' );
		tok->value=(foo[0]<<24) | (foo[1]<<16) | (foo[2]<<8) | foo[3];
	    } else {
		LogError(_("Expected string on line %d of %s"),
			tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		chunkfree(tv,sizeof(*tv));
		tv = NULL;
	    }
	    fea_ParseTok(tok);
	} else {
	    if ( tok->type!=tk_int ) {
		LogError(_("Expected integer on line %d of %s"),
			tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		chunkfree(tv,sizeof(*tv));
		tv = NULL;
		fea_ParseTok(tok);
	    } else {
		if ( tv!=NULL )
		    tv->value = tok->value;
		if ( strcmp(keys[index].name,"FontRevision")==0 ) {
		    /* Can take a float */
		    FILE *in = tok->inlist[tok->inc_depth];
		    int ch = getc(in);
		    if ( ch=='.' )
			for ( ch=getc(in); isdigit(ch); ch=getc(in));
		    ungetc(ch,in);
		}
		if ( index!=-1 && keys[index].cnt!=1 ) {
		    int is_panose = strcmp(keys[index].name,"Panose")==0 && tv!=NULL;
		    if ( is_panose )
			tv->panose_vals[0] = tv->value;
		    for ( i=1; ; ++i ) {
			fea_ParseTok(tok);
			if ( tok->type!=tk_int )
		    break;
			if ( is_panose && i<10 && tv!=NULL )
			    tv->panose_vals[i] = tok->value;
		    }
		} else
		    fea_ParseTok(tok);
	    }
	}
	if ( tok->type!=tk_char || tok->tokbuf[0]!=';' ) {
	    LogError(_("Expected semicolon on line %d of %s"),
		    tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
	    fea_skip_to_close_curly(tok);
	    chunkfree(tv,sizeof(*tv));
    break;
	}
	if ( tv!=NULL ) {
	    tv->next = head;
	    head = tv;
	}
    }
    if ( tok->type!=tk_char || tok->tokbuf[0]!='}' ) {
	LogError(_("Expected '}' on line %d of %s"),
		tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_close_curly(tok);
    }
    if ( head!=NULL ) {
	item = chunkalloc(sizeof(struct feat_item));
	item->type = ft_tablekeys;
	item->u1.offsets = keys;
	item->u2.tvals = head;
	item->next = tok->sofar;
	tok->sofar = item;
    }
}

static void fea_ParseGDEFTable(struct parseState *tok) {
    /* GlyphClassDef <base> <lig> <mark> <component>; */
    /* Attach <glyph>|<glyph class> <number>+; */	/* parse & ignore */
    /* LigatureCaret <glyph>|<glyph class> <caret value>+ */
    int i;
    struct feat_item *item;
    int16 *carets=NULL; int len=0, max=0;

    forever {
	fea_ParseTok(tok);
	if ( tok->type!=tk_name )
    break;
	if ( strcmp(tok->tokbuf,"Attach")==0 ) {
	    fea_ParseTok(tok);
	    /* Bug. Not parsing inline classes */
	    if ( tok->type!=tk_class && tok->type!=tk_name ) {
		LogError(_("Expected name or class on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		fea_skip_to_semi(tok);
	    } else {
		forever {
		    fea_ParseTok(tok);
		    if ( tok->type!=tk_int )
		break;
		}
	    }
	} else if ( strcmp(tok->tokbuf,"LigatureCaret")==0 ) {
	    item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_lcaret;
	    item->next = tok->sofar;
	    tok->sofar = item;

	    fea_ParseTok(tok);
	    if ( tok->type==tk_name )
		item->u1.class = fea_glyphname_validate(tok,tok->tokbuf);
	    else if ( tok->type==tk_cid )
		item->u1.class = fea_cid_validate(tok,tok->value);
	    else if ( tok->type == tk_class || (tok->type==tk_char && tok->tokbuf[0]=='['))
		item->u1.class = fea_ParseGlyphClassGuarded(tok);
	    else {
		LogError(_("Expected name or class on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		fea_skip_to_semi(tok);
    continue;
	    }
	    forever {
		fea_ParseTok(tok);
		if ( tok->type==tk_int )
		    /* Not strictly cricket, but I'll accept it */;
		else if ( tok->type==tk_char && tok->tokbuf[0]=='<' )
		    fea_ParseCaret(tok);
		else
	    break;
		if ( len>=max )
		    carets = grealloc(carets,(max+=10)*sizeof(int16));
		carets[len++] = tok->value;
	    }
	    if ( tok->type!=tk_char || tok->tokbuf[0]!=';' ) {
		LogError(_("Expected semicolon on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
		++tok->err_count;
		fea_skip_to_semi(tok);
	    }
	    item->u2.lcaret = galloc((len+1)*sizeof(int16));
	    memcpy(item->u2.lcaret,carets,len*sizeof(int16));
	    item->u2.lcaret[len] = 0;
	} else if ( strcmp(tok->tokbuf,"GlyphClassDef")==0 ) {
	    item = chunkalloc(sizeof(struct feat_item));
	    item->type = ft_gdefclasses;
	    item->u1.gdef_classes = chunkalloc(sizeof(char *[4]));
	    item->next = tok->sofar;
	    tok->sofar = item;
	    for ( i=0; i<4; ++i ) {
		fea_ParseTok(tok);
		item->u1.gdef_classes[i] = fea_ParseGlyphClassGuarded(tok);
	    }
	    fea_ParseTok(tok);
	} else {
	    LogError(_("Expected Attach or LigatureCaret or GlyphClassDef on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
    break;
	}
    }
    if ( tok->type!=tk_char || tok->tokbuf[0]!='}' ) {
	LogError(_("Unexpected token in GDEF on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_close_curly(tok);
    }
    free(carets);
}

static void fea_ParseTableDef(struct parseState *tok) {
    uint32 table_tag;
    struct feat_item *item;

    fea_ParseTag(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag ) {
	LogError(_("Expected tag in table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    table_tag = tok->tag;

    item = chunkalloc(sizeof(struct feat_item));
    item->type = ft_table;
    item->u1.tag = table_tag;
    item->next = tok->sofar;
    tok->sofar = item;
    fea_TokenMustBe(tok,tk_char,'{');
    switch ( table_tag ) {
      case CHR('G','D','E','F'):
	fea_ParseGDEFTable(tok);
      break;
      case CHR('n','a','m','e'):
	fea_ParseNameTable(tok);
      break;

      case CHR('h','h','e','a'):
	fea_ParseTableKeywords(tok,hhead_keys);
      break;
      case CHR('v','h','e','a'):
	fea_ParseTableKeywords(tok,vhead_keys);
      break;
      case CHR('O','S','/','2'):
	fea_ParseTableKeywords(tok,os2_keys);
      break;

      case CHR('h','e','a','d'):
	/* FontRevision <number>.<number>; */
	/* Only one field here, and I don't really support it */
      case CHR('v','m','t','x'):
	/* I don't support 'vmtx' tables */
      case CHR('B','A','S','E'):
	/* I don't support 'BASE' tables */
      default:
	fea_skip_to_close_curly(tok);
      break;
    }

    fea_ParseTag(tok);
    if ( tok->type!=tk_name || !tok->could_be_tag || tok->tag!=table_tag ) {
	LogError(_("Expected matching tag in table on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	++tok->err_count;
	fea_skip_to_semi(tok);
return;
    }
    fea_end_statement(tok);
}

/* ************************************************************************** */
/* ******************************* Free feat ******************************** */
/* ************************************************************************** */

static void NameIdFree(struct nameid *nm) {
    struct nameid *nmnext;

    while ( nm!=NULL ) {
	nmnext = nm->next;
	free( nm->utf8_str );
	chunkfree(nm,sizeof(*nm));
	nm = nmnext;
    }
}

static void TableValsFree(struct tablevalues *tb) {
    struct tablevalues *tbnext;

    while ( tb!=NULL ) {
	tbnext = tb->next;
	chunkfree(tb,sizeof(*tb));
	tb = tbnext;
    }
}

static void fea_featitemFree(struct feat_item *item) {
    struct feat_item *next;
    int i,j;

    while ( item!=NULL ) {
	next = item->next;
	switch ( item->type ) {
	  case ft_lookup_end:
	  case ft_feat_end:
	  case ft_table:
	  case ft_subtable:
	  case ft_script:
	  case ft_lang:
	  case ft_lookupflags:
	    /* Nothing needs freeing */;
	  break;
	  case ft_feat_start:
	  case ft_langsys:
	    ScriptLangListFree( item->u2.sl);
	  break;
	  case ft_lookup_start:
	  case ft_lookup_ref:
	    free( item->u1.lookup_name );
	  break;
	  case ft_sizeparams:
	    free( item->u1.params );
	    NameIdFree( item->u2.names );
	  break;
	  case ft_names:
	    NameIdFree( item->u2.names );
	  break;
	  case ft_gdefclasses:
	    for ( i=0; i<4; ++i )
		free(item->u1.gdef_classes[i]);
	    chunkfree(item->u1.gdef_classes,sizeof(char *[4]));
	  break;
	  case ft_lcaret:
	    free( item->u2.lcaret );
	  break;
	  case ft_tablekeys:
	    TableValsFree( item->u2.tvals );
	  break;
	  case ft_pst:
	    PSTFree( item->u2.pst );
	  break;
	  case ft_pstclass:
	    free( item->u1.class );
	    PSTFree( item->u2.pst );
	  break;
	  case ft_ap:
	    AnchorPointsFree( item->u2.ap );
	    free( item->mark_class );
	  break;
	  case ft_fpst:
	    if ( item->u2.fpst!=NULL ) {
		for ( i=0; i<item->u2.fpst->rule_cnt; ++i ) {
		    struct fpst_rule *r = &item->u2.fpst->rules[i];
		    for ( j=0; j<r->lookup_cnt; ++j ) {
			if ( r->lookups[j].lookup!=NULL ) {
			    struct feat_item *nested = (struct feat_item *) (r->lookups[j].lookup);
			    fea_featitemFree(nested);
			    r->lookups[j].lookup = NULL;
			}
		    }
		}
		FPSTFree(item->u2.fpst);
	    }
	  break;
	  default:
	    IError("Don't know how to free a feat_item of type %d", item->type );
	  break;
	}
	chunkfree(item,sizeof(*item));
	item = next;
    }
}

static void fea_ParseFeatureFile(struct parseState *tok) {

    forever {
	fea_ParseTok(tok);
	if ( tok->err_count>100 )
    break;
	switch ( tok->type ) {
	  case tk_class:
	    fea_ParseGlyphClassDef(tok);
	  break;
	  case tk_lookup:
	    fea_ParseLookupDef(tok,false);
	  break;
	  case tk_languagesystem:
	    fea_ParseLangSys(tok,false);
	  break;
	  case tk_feature:
	    fea_ParseFeatureDef(tok);
	  break;
	  case tk_table:
	    fea_ParseTableDef(tok);
	  break;
	  case tk_anonymous:
	    LogError(_("FontForge does not support anonymous tables on line %d of %s"), tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    fea_skip_to_close_curly(tok);
	  break;
	  case tk_eof:
  goto end_loop;
	  default:
	    LogError(_("Unexpected token, %s, on line %d of %s"), tok->tokbuf, tok->line[tok->inc_depth], tok->filename[tok->inc_depth] );
	    ++tok->err_count;
  goto end_loop;
      }
    }
  end_loop:;
}

/* ************************************************************************** */
/* ******************************* Apply feat ******************************* */
/* ************************************************************************** */
static int fea_FeatItemEndsLookup(enum feat_type type) {
return( type==ft_lookup_end || type == ft_feat_end ||
	    type == ft_table || type == ft_script ||
	    type == ft_lang || type == ft_langsys ||
	    type == ft_lookup_ref );
}

static struct feat_item *fea_SetLookupLink(struct feat_item *nested,
	enum otlookup_type type) {
    struct feat_item *prev = NULL;
    enum otlookup_type found_type;

    while ( nested!=NULL ) {
	/* Stop when we find something which forces a new lookup */
	if ( fea_FeatItemEndsLookup(nested->type) )
    break;
	if ( nested->ticked ) {
	    nested = nested->next;
    continue;
	}
	found_type = fea_LookupTypeFromItem(nested);
	if ( type==ot_undef || found_type == ot_undef || found_type == type ) {
	    if ( nested->type!=ft_ap || nested->u2.ap->type!=at_mark )
		nested->ticked = true;		/* Marks might get used in more than one lookup */
	    if ( prev!=NULL )
		prev->lookup_next = nested;
	    prev = nested;
	}
	nested = nested->next;
    }
return( nested );
}

static void fea_ApplyLookupListPST(struct parseState *tok,
	struct feat_item *lookup_data,OTLookup *otl) {
    struct lookup_subtable *sub = NULL, *last=NULL;
    struct feat_item *l;
    (void)tok;
    /* Simple pst's are easy. We just attach them to their proper glyphs */
    /*  and then clear the feat_item pst slot (so we don't free them later) */
    /* There might be a subtable break */
    /* There might be a lookupflags */

    for ( l = lookup_data; l!=NULL; l=l->lookup_next ) {
	switch ( l->type ) {
	  case ft_lookup_start:
	  case ft_lookupflags:
	    /* Ignore these, already handled them */;
	  break;
	  case ft_subtable:
	    sub = NULL;
	  break;
	  case ft_pst:
	    if ( sub==NULL ) {
		sub = chunkalloc(sizeof(struct lookup_subtable));
		sub->lookup = otl;
		sub->per_glyph_pst_or_kern = true;
		if ( last==NULL )
		    otl->subtables = sub;
		else
		    last->next = sub;
		last = sub;
	    }
	    l->u2.pst->subtable = sub;
	    l->u2.pst->next = l->u1.sc->possub;
	    l->u1.sc->possub = l->u2.pst;
	    l->u2.pst = NULL;			/* So we don't free it later */
	  break;
	  default:
	    IError("Unexpected feature type %d in a PST feature", l->type );
	  break;
	}
    }
}

static OTLookup *fea_ApplyLookupList(struct parseState *tok,
	struct feat_item *lookup_data,int lookup_flag);

static void fea_ApplyLookupListContextual(struct parseState *tok,
	struct feat_item *lookup_data,OTLookup *otl) {
    struct lookup_subtable *sub = NULL, *last=NULL;
    struct feat_item *l;
    int i,j;
    /* Fpst's are almost as easy as psts. We don't worry about subtables */
    /*  (every fpst gets a new subtable, so the statement is irrelevant) */
    /* the only complication is that we must recursively handle a lookup list */
    /* There might be a lookupflags */

    for ( l = lookup_data; l!=NULL; l=l->lookup_next ) {
	switch ( l->type ) {
	  case ft_lookup_start:
	  case ft_lookupflags:
	  case ft_subtable:
	    /* Ignore these, already handled them */;
	  break;
	  case ft_fpst:
	    sub = chunkalloc(sizeof(struct lookup_subtable));
	    sub->lookup = otl;
	    if ( last==NULL )
		otl->subtables = sub;
	    else
		last->next = sub;
	    last = sub;
	    sub->fpst = l->u2.fpst;
	    l->u2.fpst->next = tok->sf->possub;
	    tok->sf->possub = l->u2.fpst;
	    l->u2.fpst = NULL;
	    sub->fpst->subtable = sub;
	    for ( i=0; i<sub->fpst->rule_cnt; ++i ) {
		struct fpst_rule *r = &sub->fpst->rules[i];
		for ( j=0; j<r->lookup_cnt; ++j ) {
		    if ( r->lookups[j].lookup!=NULL ) {
			struct feat_item *nested = (struct feat_item *) (r->lookups[j].lookup);
			fea_SetLookupLink(nested,ot_undef);
			r->lookups[j].lookup = fea_ApplyLookupList(tok,nested,otl->lookup_flags);	/* Not really sure what the lookup flag should be here. */
			fea_featitemFree(nested);
		    }
		}
	    }
	  break;
	  default:
	    IError("Unexpected feature type %d in a FPST feature", l->type );
	  break;
	}
    }
}

static void fea_ApplyLookupListCursive(struct parseState *tok,
	struct feat_item *lookup_data,OTLookup *otl) {
    struct lookup_subtable *sub = NULL, *last=NULL;
    struct feat_item *l;
    AnchorPoint *aplast, *ap;
    AnchorClass *ac = NULL;
    /* Cursive's are also easy. There might be two ap's in the list so slight */
    /*  care needed when adding them to a glyph, and we must create an anchorclass */
    /* There might be a subtable break */
    /* There might be a lookupflags */

    for ( l = lookup_data; l!=NULL; l=l->lookup_next ) {
	switch ( l->type ) {
	  case ft_lookup_start:
	  case ft_lookupflags:
	    /* Ignore these, already handled them */;
	  break;
	  case ft_subtable:
	    sub = NULL;
	  break;
	  case ft_ap:
	    if ( sub==NULL ) {
		sub = chunkalloc(sizeof(struct lookup_subtable));
		sub->lookup = otl;
		sub->anchor_classes = true;
		if ( last==NULL )
		    otl->subtables = sub;
		else
		    last->next = sub;
		last = sub;
		ac = chunkalloc(sizeof(AnchorClass));
		ac->subtable = sub;
		ac->type = act_curs;
		ac->next = tok->accreated;
		tok->accreated = ac;
	    }
	    aplast = NULL;
	    for ( ap=l->u2.ap; ap!=NULL; ap=ap->next ) {
		aplast = ap;
		ap->anchor = ac;
	    }
	    aplast->next = l->u1.sc->anchor;
	    l->u1.sc->anchor = l->u2.ap;
	    l->u2.ap = NULL;			/* So we don't free them later */
	  break;
	  default:
	    IError("Unexpected feature type %d in a cursive feature", l->type );
	  break;
	}
    }
}

static void fea_ApplyLookupListMark2(struct parseState *tok,
	struct feat_item *lookup_data,int mcnt,OTLookup *otl) {
    /* Mark2* lookups are not well documented (because adobe FDK doesn't */
    /*  support them) but I'm going to assume that if I have some mark */
    /*  statements, then some pos statement, then another mark statement */
    /*  that I begin a new subtable with the second set of marks (and a */
    /*  different set of mark classes) */
    char **classes;
    AnchorClass **acs;
    int ac_cnt, i;
    struct lookup_subtable *sub = NULL, *last=NULL;
    struct feat_item *mark_start, *l;
    AnchorPoint *ap, *aplast;

    classes = galloc(mcnt*sizeof(char *));
    acs = galloc(mcnt*sizeof(AnchorClass *));
    ac_cnt = 0;
    while ( lookup_data != NULL && lookup_data->type!=ft_lookup_end ) {
	struct feat_item *orig = lookup_data;
	sub = NULL;
	/* Skip any subtable marks */
	while ( lookup_data!=NULL &&
		(lookup_data->type==ft_subtable ||
		 lookup_data->type==ft_lookup_start ||
		 lookup_data->type==ft_lookupflags ) )
	    lookup_data = lookup_data->lookup_next;

	/* Skip over the marks, we'll deal with them after we know the mark classes */
	mark_start = lookup_data;
	while ( lookup_data!=NULL &&
		((lookup_data->type==ft_ap && lookup_data->u2.ap->type==at_mark) ||
		 lookup_data->type==ft_lookup_start ||
		 lookup_data->type==ft_lookupflags ) )
	    lookup_data = lookup_data->lookup_next;

	/* Now process the base glyphs and figure out the mark classes */
	while ( lookup_data!=NULL &&
		((lookup_data->type==ft_ap && lookup_data->mark_class!=NULL) ||
		 lookup_data->type==ft_lookup_start ||
		 lookup_data->type==ft_lookupflags ) ) {
	    if ( lookup_data->type == ft_ap ) {
		for ( i=0; i<ac_cnt; ++i ) {
		    if ( strcmp(lookup_data->mark_class,classes[i])==0 )
		break;
		}
		if ( i==ac_cnt ) {
		    ++ac_cnt;
		    classes[i] = lookup_data->mark_class;
		    acs[i] = chunkalloc(sizeof(AnchorClass));
		    if ( sub==NULL ) {
			sub = chunkalloc(sizeof(struct lookup_subtable));
			sub->lookup = otl;
			sub->anchor_classes = true;
			if ( last==NULL )
			    otl->subtables = sub;
			else
			    last->next = sub;
			last = sub;
		    }
		    acs[i]->subtable = sub;
		    acs[i]->type = otl->lookup_type==gpos_mark2mark ? act_mkmk :
				    otl->lookup_type==gpos_mark2base ? act_mark :
					    act_mklg;
		    acs[i]->next = tok->accreated;
		    tok->accreated = acs[i];
		}
		aplast = NULL;
		for ( ap=lookup_data->u2.ap; ap!=NULL; ap=ap->next ) {
		    aplast = ap;
		    ap->anchor = acs[i];
		}
		aplast->next = lookup_data->u1.sc->anchor;
		lookup_data->u1.sc->anchor = lookup_data->u2.ap;
		lookup_data->u2.ap = NULL;	/* So we don't free them later */
	    }
	    lookup_data = lookup_data->next;
	}

	/* Now go back and assign the marks to the correct anchor classes */
	for ( l=mark_start; l!=NULL &&
		/* The base aps will have been set to NULL above */
		((l->type==ft_ap && l->u2.ap!=NULL && l->u2.ap->type==at_mark) ||
		 l->type==ft_lookup_start ||
		 l->type==ft_lookupflags ) ;
		l = l->lookup_next ) {
	    if ( l->type==ft_ap ) {
		for ( i=0; i<ac_cnt; ++i ) {
		    if ( fea_classesIntersect(l->u1.sc->name,classes[i])) {
			AnchorPoint *ap = AnchorPointsCopy(l->u2.ap);
			/* We make a copy of this anchor point because marks */
			/*  might be used in more than one lookup. It makes */
			/*  sense for a user to define a set of marks to be */
			/*  used with both a m2base and a m2lig lookup within */
			/*  a feature */
			ap->anchor = acs[i];
			ap->next = l->u1.sc->anchor;
			l->u1.sc->anchor = ap;
		break;
		    }
		}
	    }
	}
	if ( lookup_data==orig )
    break;
    }
}


static int is_blank(const char *s) {
    int i;

    i = 0;
    while (s[i] != '\0' && s[i] == ' ')
        i++;
    return( s[i] == '\0');
}

struct class_set {
    char **classes;
    int cnt, max;
};

/* We've got a set of glyph classes -- but they are the classes that make sense */
/*  to the user and so there's no guarantee that there aren't two classes with */
/*  the same glyph(s) */
/* Simplify the list so that: There are no duplicates classes and each name */
/*  appears in at most one class. This is what we need */
static void fea_canonicalClassSet(struct class_set *set) {
    int i,j,k;

    /* Remove any duplicate classes */
    qsort(set->classes,set->cnt,sizeof(char *), strcmpD);
    for ( i=0; i<set->cnt; ++i ) {
	for ( j=i+1; j<set->cnt; ++j )
	    if ( strcmp(set->classes[i],set->classes[j])!=0 )
	break;
	if ( j>i+1 ) {
	    int off = j-(i+1);
	    for ( k=i+1; k<j; ++k )
		free(set->classes[k]);
	    for ( k=j ; k<set->cnt; ++k )
		set->classes[k-off] = set->classes[k];
	    set->cnt -= off;
	}
    }

    for ( i=0; i < set->cnt - 1; ++i ) {
        for ( j=i+1; j < set->cnt; ++j ) {
            if ( fea_classesIntersect(set->classes[i],set->classes[j]) ) {
                if ( set->cnt>=set->max )
                    set->classes = grealloc(set->classes,(set->max+=20)*sizeof(char *));
                set->classes[set->cnt++] = fea_classesSplit(set->classes[i],set->classes[j]);
            }
        }
    }

    /* Remove empty classes */
    i = 0;
    while (i < set->cnt) {
        if (is_blank(set->classes[i])) {
            free(set->classes[i]);
            for ( k=i+1 ; k < set->cnt; ++k )
                set->classes[k-1] = set->classes[k];
            set->cnt -= 1;
        } else {
            i++;
        }
    }
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static void KCFillDevTab(KernClass *kc,int index,DeviceTable *dt) {
    if ( dt==NULL || dt->corrections == NULL )
return;
    if ( kc->adjusts == NULL )
	kc->adjusts = gcalloc(kc->first_cnt*kc->second_cnt,sizeof(DeviceTable));
    kc->adjusts[index] = *dt;
    kc->adjusts[index].corrections = galloc(dt->last_pixel_size-dt->first_pixel_size+1);
    memcpy(kc->adjusts[index].corrections,dt->corrections,dt->last_pixel_size-dt->first_pixel_size+1);

}

static void KPFillDevTab(KernPair *kp,DeviceTable *dt) {
    if ( dt==NULL || dt->corrections == NULL )
return;
    kp->adjust = chunkalloc(sizeof(DeviceTable));
    *kp->adjust = *dt;
    kp->adjust->corrections = galloc(dt->last_pixel_size-dt->first_pixel_size+1);
    memcpy(kp->adjust->corrections,dt->corrections,dt->last_pixel_size-dt->first_pixel_size+1);
}
#endif

static void fea_fillKernClass(KernClass *kc,struct feat_item *l) {
    int i,j;
    PST *pst;

    while ( l!=NULL && l->type!=ft_subtable ) {
	if ( l->type==ft_pstclass ) {
	    pst = l->u2.pst;
	    for ( i=1; i<kc->first_cnt; ++i ) {
		if ( fea_classesIntersect(kc->firsts[i],l->u1.class) ) {
		    for ( j=1; j<kc->second_cnt; ++j ) {
			if ( fea_classesIntersect(kc->seconds[j],pst->u.pair.paired) ) {
			    /* FontForge only supports kerning classes in one direction at a time, not full value records */
			    if ( pst->u.pair.vr[0].h_adv_off != 0 ) {
				kc->offsets[i*kc->second_cnt+j] = pst->u.pair.vr[0].h_adv_off;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
				if ( pst->u.pair.vr[0].adjust!=NULL )
				    KCFillDevTab(kc,i*kc->second_cnt+j,&pst->u.pair.vr[0].adjust->xadv);
#endif
			    } else if ( pst->u.pair.vr[0].v_adv_off != 0 ) {
				kc->offsets[i*kc->second_cnt+j] = pst->u.pair.vr[0].v_adv_off;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
				if ( pst->u.pair.vr[0].adjust!=NULL )
				    KCFillDevTab(kc,i*kc->second_cnt+j,&pst->u.pair.vr[0].adjust->yadv);
#endif
			    } else if ( pst->u.pair.vr[1].h_adv_off != 0 ) {
				kc->offsets[i*kc->second_cnt+j] = pst->u.pair.vr[1].h_adv_off;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
				if ( pst->u.pair.vr[1].adjust!=NULL )
				    KCFillDevTab(kc,i*kc->second_cnt+j,&pst->u.pair.vr[1].adjust->xadv);
#endif
			    }
			    if ( strcmp(kc->seconds[j],pst->u.pair.paired)==0 )
		    break;
			}
		    }
		    if ( strcmp(kc->firsts[i],l->u1.class)==0 )
	    break;
		}
	    }
	}
	l = l->lookup_next;
    }
}

static void SFKernClassRemoveFree(SplineFont *sf,KernClass *kc) {
    KernClass *prev;

    if ( sf->kerns==kc )
	sf->kerns = kc->next;
    else if ( sf->vkerns==kc )
	sf->vkerns = kc->next;
    else {
	prev = NULL;
	if ( sf->kerns!=NULL )
	    for ( prev=sf->kerns; prev!=NULL && prev->next!=kc; prev=prev->next );
	if ( prev==NULL && sf->vkerns!=NULL )
	    for ( prev=sf->vkerns; prev!=NULL && prev->next!=kc; prev=prev->next );
	if ( prev!=NULL )
	    prev->next = kc->next;
    }
    kc->next = NULL;
    KernClassListFree(kc);
}

static void fea_ApplyLookupListPair(struct parseState *tok,
	struct feat_item *lookup_data,int kmax,OTLookup *otl) {
    /* kcnt is the number of left/right glyph-name-lists we must sort into classes */
    struct feat_item *l, *first;
    struct class_set lefts, rights;
    struct lookup_subtable *sub = NULL, *lastsub=NULL;
    SplineChar *sc, *other;
    PST *pst;
    KernPair *kp;
    KernClass *kc;
    int vkern, kcnt, i;

    memset(&lefts,0,sizeof(lefts));
    memset(&rights,0,sizeof(rights));
    if ( kmax!=0 ) {
	lefts.classes = galloc(kmax*sizeof(char *));
	rights.classes = galloc(kmax*sizeof(char *));
	lefts.max = rights.max = kmax;
    }
    vkern = false;
    for ( l = lookup_data; l!=NULL; ) {
	first = l;
	kcnt = 0;
	while ( l!=NULL && l->type!=ft_subtable ) {
	    if ( l->type == ft_pst ) {
		if ( sub==NULL ) {
		    sub = chunkalloc(sizeof(struct lookup_subtable));
		    sub->lookup = otl;
		    sub->per_glyph_pst_or_kern = true;
		    if ( lastsub==NULL )
			otl->subtables = sub;
		    else
			lastsub->next = sub;
		    lastsub = sub;
		}
		pst = l->u2.pst;
		sc = l->u1.sc;
		l->u2.pst = NULL;
		kp = NULL;
		other = SFGetChar(sc->parent,-1,pst->u.pair.paired);
		if ( pst->u.pair.vr[0].xoff==0 && pst->u.pair.vr[0].yoff==0 &&
			pst->u.pair.vr[1].xoff==0 && pst->u.pair.vr[1].yoff==0 &&
			pst->u.pair.vr[1].v_adv_off==0 &&
			other!=NULL ) {
		    if ( (otl->lookup_flags&pst_r2l) &&
			    (pst->u.pair.vr[0].h_adv_off==0 && pst->u.pair.vr[0].v_adv_off==0 )) {
			kp = chunkalloc(sizeof(KernPair));
			kp->off = pst->u.pair.vr[1].h_adv_off;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			if ( pst->u.pair.vr[1].adjust!=NULL )
			    KPFillDevTab(kp,&pst->u.pair.vr[1].adjust->xadv);
#endif
		    } else if ( !(otl->lookup_flags&pst_r2l) &&
			    (pst->u.pair.vr[1].h_adv_off==0 && pst->u.pair.vr[0].v_adv_off==0 )) {
			kp = chunkalloc(sizeof(KernPair));
			kp->off = pst->u.pair.vr[0].h_adv_off;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			if ( pst->u.pair.vr[0].adjust!=NULL )
			    KPFillDevTab(kp,&pst->u.pair.vr[0].adjust->xadv);
#endif
		    } else if ( (pst->u.pair.vr[0].h_adv_off==0 && pst->u.pair.vr[1].h_adv_off==0 )) {
			vkern = sub->vertical_kerning = true;
			kp = chunkalloc(sizeof(KernPair));
			kp->off = pst->u.pair.vr[0].v_adv_off;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			if ( pst->u.pair.vr[0].adjust!=NULL )
			    KPFillDevTab(kp,&pst->u.pair.vr[0].adjust->yadv);
#endif
		    }
		}
		if ( kp!=NULL ) {
		    kp->sc = other;
		    kp->subtable = sub;
		    if ( vkern ) {
			kp->next = sc->vkerns;
			sc->vkerns = kp;
		    } else {
			kp->next = sc->kerns;
			sc->kerns = kp;
		    }
		    PSTFree(pst);
		} else {
		    pst->subtable = sub;
		    pst->next = sc->possub;
		    sc->possub = pst;
		}
	    } else if ( l->type == ft_pstclass ) {
		lefts.classes[kcnt] = copy(fea_canonicalClassOrder(l->u1.class));
		rights.classes[kcnt++] = copy(fea_canonicalClassOrder(l->u2.pst->u.pair.paired));
	    }
	    l = l->lookup_next;
	}
	if ( kcnt!=0 ) {
	    lefts.cnt = rights.cnt = kcnt;
	    fea_canonicalClassSet(&lefts);
	    fea_canonicalClassSet(&rights);

	    sub = chunkalloc(sizeof(struct lookup_subtable));
	    sub->lookup = otl;
	    if ( lastsub==NULL )
		otl->subtables = sub;
	    else
		lastsub->next = sub;
	    lastsub = sub;

	    if ( sub->kc!=NULL )
		SFKernClassRemoveFree(tok->sf,sub->kc);
	    sub->kc = kc = chunkalloc(sizeof(KernClass));
	    kc->first_cnt = lefts.cnt+1; kc->second_cnt = rights.cnt+1;
	    kc->firsts = galloc(kc->first_cnt*sizeof(char *));
	    kc->seconds = galloc(kc->second_cnt*sizeof(char *));
	    kc->firsts[0] = kc->seconds[0] = NULL;
	    for ( i=0; i<lefts.cnt; ++i )
		kc->firsts[i+1] = lefts.classes[i];
	    for ( i=0; i<rights.cnt; ++i )
		kc->seconds[i+1] = rights.classes[i];
	    kc->subtable = sub;
	    kc->offsets = gcalloc(kc->first_cnt*kc->second_cnt,sizeof(int16));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    kc->adjusts = gcalloc(kc->first_cnt*kc->second_cnt,sizeof(DeviceTable));
#endif
	    fea_fillKernClass(kc,first);
	    if ( sub->vertical_kerning ) {
		kc->next = tok->sf->vkerns;
		tok->sf->vkerns = kc;
	    } else {
		kc->next = tok->sf->kerns;
		tok->sf->kerns = kc;
	    }
	}
	sub = NULL;
	while ( l!=NULL && l->type==ft_subtable )
	    l = l->lookup_next;
    }
    if ( kmax!=0 ) {
	free(lefts.classes);
	free(rights.classes);
    }
}

static OTLookup *fea_ApplyLookupList(struct parseState *tok,
	struct feat_item *lookup_data,int lookup_flag) {
    /* A lookup list might consist just of a lookup_ref so find the lookup named u1.lookup_name */
    /* A lookup_start is optional and provides the lookup name */
    /* A lookupflags is optional and may occur anywhere u2.lookupflags */
    /* An ap is for mark2 types for the mark u1.sc and u2.ap (not grouped in anchor classes yet) */
    /* A fpst is for contextuals u2.fpst (rule.lookups[i].lookup are lookup lists in their own rights that need to become lookups) */
    /* A subtable means a subtable break, make up a new name, ignore multiple subtable entries */
    /* A pst is for simple things u1.sc, u2.pst */
    /* A pstclass is for kerning classes u1.class, u2.pst (paired may be a class list too) */
    /* An ap is for cursive types for the u1.sc and u2.ap (an entry and an exit ap) */
    /* An ap is for mark2 types for the base u1.sc and u2.ap and mark_class */
    OTLookup *otl;
    int kcnt, mcnt;
    struct feat_item *l;
    enum otlookup_type temp;

    if ( lookup_data->type == ft_lookup_ref ) {
	for ( otl=tok->created; otl!=NULL; otl=otl->next )
	    if ( otl->lookup_name!=NULL &&
		    strcmp(otl->lookup_name,lookup_data->u1.lookup_name)==0)
return( otl );
	otl = SFFindLookup(tok->sf,lookup_data->u1.lookup_name);
	if ( otl==NULL )
	    LogError( _("No lookup named %s"),lookup_data->u1.lookup_name );
	    /* Can't give a line number, this is second pass */
return( otl );
    }

    otl = chunkalloc(sizeof(OTLookup));
    otl->lookup_flags = lookup_flag;
    otl->lookup_type = ot_undef;
    if ( tok->last==NULL )
	tok->created = otl;
    else
	tok->last->next = otl;
    tok->last = otl;

    /* Search first for class counts */
    kcnt = mcnt = 0;
    for ( l = lookup_data; l!=NULL; l=l->lookup_next ) {
	if ( l->type == ft_ap && l->mark_class!=NULL )
	    ++mcnt;
	else if ( l->type == ft_pstclass )
	    ++kcnt;
	else if ( l->type == ft_lookupflags )
	    otl->lookup_flags = l->u2.lookupflags;
	else if ( l->type == ft_lookup_start ) {
	    otl->lookup_name = l->u1.lookup_name;
	    l->u1.lookup_name = NULL;			/* So we don't free it later */
	}
	temp = fea_LookupTypeFromItem(l);
	if ( temp==ot_undef )
	    /* Tum ty tum tum. No information */;
	else if ( otl->lookup_type == ot_undef )
	    otl->lookup_type = temp;
	else if ( otl->lookup_type != temp )
	    IError(_("Mismatch lookup types inside a parsed lookup"));
    }
    if ( otl->lookup_type==gpos_mark2base ||
	    otl->lookup_type==gpos_mark2ligature ||
	    otl->lookup_type==gpos_mark2mark )
	fea_ApplyLookupListMark2(tok,lookup_data,mcnt,otl);
    else if ( mcnt!=0 )
	IError(_("Mark anchors provided when nothing can use them"));
    else if ( otl->lookup_type==gpos_cursive )
	fea_ApplyLookupListCursive(tok,lookup_data,otl);
    else if ( otl->lookup_type==gpos_pair )
	fea_ApplyLookupListPair(tok,lookup_data,kcnt,otl);
    else if ( otl->lookup_type==gpos_contextchain ||
	    otl->lookup_type==gsub_contextchain )
	fea_ApplyLookupListContextual(tok,lookup_data,otl);
    else
	fea_ApplyLookupListPST(tok,lookup_data,otl);
return( otl );
}

static struct otfname *fea_NameID2OTFName(struct nameid *names) {
    struct otfname *head=NULL, *cur;

    while ( names!=NULL ) {
	cur = chunkalloc(sizeof(struct otfname));
	cur->lang = names->language;
	cur->name = names->utf8_str;
	names->utf8_str = NULL;
	cur->next = head;
	head = cur;
	names = names->next;
    }
return( head );
}

static void fea_AttachFeatureToLookup(OTLookup *otl,uint32 feat_tag,
	struct scriptlanglist *sl) {
    FeatureScriptLangList *fl;

    if ( otl==NULL )
return;

    for ( fl = otl->features; fl!=NULL && fl->featuretag!=feat_tag; fl=fl->next );
    if ( fl==NULL ) {
	fl = chunkalloc(sizeof(FeatureScriptLangList));
	fl->next = otl->features;
	otl->features = fl;
	fl->featuretag = feat_tag;
	fl->scripts = SListCopy(sl);
    } else
	SLMerge(fl,sl);
}

static void fea_NameID2NameTable(SplineFont *sf, struct nameid *names) {
    struct ttflangname *cur;

    while ( names!=NULL ) {
	for ( cur = sf->names; cur!=NULL && cur->lang!=names->language; cur=cur->next );
	if ( cur==NULL ) {
	    cur = chunkalloc(sizeof(struct ttflangname));
	    cur->lang = names->language;
	    cur->next = sf->names;
	    sf->names = cur;
	}
	free(cur->names[names->strid]);
	cur->names[names->strid] = names->utf8_str;
	names->utf8_str = NULL;
	names = names->next;
    }
}

static void fea_TableByKeywords(SplineFont *sf, struct feat_item *f) {
    struct tablevalues *tv;
    struct tablekeywords *offsets = f->u1.offsets, *cur;
    int i;

    if ( !sf->pfminfo.pfmset ) {
	SFDefaultOS2Info(&sf->pfminfo,sf,sf->fontname);
	sf->pfminfo.pfmset = sf->pfminfo.subsuper_set = sf->pfminfo.panose_set =
	    sf->pfminfo.hheadset = sf->pfminfo.vheadset = true;
    }
    for ( tv = f->u2.tvals; tv!=NULL; tv=tv->next ) {
	cur = &offsets[tv->index];
	if ( cur->offset==-1 )
	    /* We don't support this guy, whatever he may be, but we did parse it */;
	else if ( cur->cnt==1 ) {
	    if ( cur->size==4 )
		*((uint32 *) (((uint8 *) sf) + cur->offset)) = tv->value;
	    else if ( cur->size==2 )
		*((uint16 *) (((uint8 *) sf) + cur->offset)) = tv->value;
	    else
		*((uint8 *) (((uint8 *) sf) + cur->offset)) = tv->value;
	    if ( strcmp(cur->name,"Ascender")==0 )
		sf->pfminfo.hheadascent_add = false;
	    else if ( strcmp(cur->name,"Descender")==0 )
		sf->pfminfo.hheaddescent_add = false;
	    else if ( strcmp(cur->name,"winAscent")==0 )
		sf->pfminfo.winascent_add = false;
	    else if ( strcmp(cur->name,"winDescent")==0 )
		sf->pfminfo.windescent_add = false;
	    else if ( strcmp(cur->name,"TypoAscender")==0 )
		sf->pfminfo.typoascent_add = false;
	    else if ( strcmp(cur->name,"TypoDescender")==0 )
		sf->pfminfo.typodescent_add = false;
	} else if ( cur->cnt==10 && cur->size==1 ) {
	    for ( i=0; i<10; ++i )
		(((uint8 *) sf) + cur->offset)[i] = tv->panose_vals[i];
	}
    }
}

static void fea_GDefGlyphClasses(SplineFont *sf, struct feat_item *f) {
    int i, ch;
    char *pt, *start;
    SplineChar *sc;

    for ( i=0; i<4; ++i ) if ( f->u1.gdef_classes[i]!=NULL ) {
	for ( pt=f->u1.gdef_classes[i]; ; ) {
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='\0' )
	break;
	    for ( start = pt; *pt!=' ' && *pt!='\0'; ++pt );
	    ch = *pt; *pt = '\0';
	    sc = SFGetChar(sf,-1,start);
	    *pt = ch;
	    if ( sc!=NULL )
		sc->glyph_class = i+1;
	}
    }
}

static void fea_GDefLigCarets(SplineFont *sf, struct feat_item *f) {
    int i, ch;
    char *pt, *start;
    SplineChar *sc;
    PST *pst, *prev, *next;

    for ( pt=f->u1.class; ; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	for ( start = pt; *pt!=' ' && *pt!='\0'; ++pt );
	ch = *pt; *pt = '\0';
	sc = SFGetChar(sf,-1,start);
	*pt = ch;
	if ( sc!=NULL ) {
	    for ( prev=NULL, pst=sc->possub; pst!=NULL; pst=next ) {
		next = pst->next;
		if ( pst->type!=pst_lcaret )
		    prev = pst;
		else {
		    if ( prev==NULL )
			sc->possub = next;
		    else
			prev->next = next;
		    pst->next = NULL;
		    PSTFree(pst);
		}
	    }
	    for ( i=0; f->u2.lcaret[i]!=0; ++i );
	    pst = chunkalloc(sizeof(PST));
	    pst->next = sc->possub;
	    sc->possub = pst;
	    pst->type = pst_lcaret;
	    pst->u.lcaret.cnt = i;
	    pst->u.lcaret.carets = f->u2.lcaret;
	    f->u2.lcaret = NULL;
	}
    }
}

static struct feat_item *fea_ApplyFeatureList(struct parseState *tok,
	struct feat_item *feat_data) {
    int lookup_flags = 0;
    uint32 feature_tag = feat_data->u1.tag;
    struct scriptlanglist *sl = feat_data->u2.sl;
    struct feat_item *f, *start;
    OTLookup *otl;
    int saw_script = false;
    enum otlookup_type ltype;

    feat_data->u2.sl = NULL;

    for ( f=feat_data->next; f!=NULL && f->type!=ft_feat_end ; ) {
	if ( f->ticked ) {
	    f = f->next;
    continue;
	}
	switch ( f->type ) {
	  case ft_lookupflags:
	    lookup_flags = f->u2.lookupflags;
	    f = f->next;
    continue;
	  case ft_lookup_ref:
	    otl = fea_ApplyLookupList(tok,f,lookup_flags);
	    fea_AttachFeatureToLookup(otl,feature_tag,sl);
	    f = f->next;
    continue;
	  case ft_lookup_start:
	    start = f;
	    start->lookup_next = f->next;
	    f = fea_SetLookupLink(start->next,ot_undef);
	    if ( f!=NULL && f->type == ft_lookup_end )
		f = f->next;
	    otl = fea_ApplyLookupList(tok,start,lookup_flags);
	    fea_AttachFeatureToLookup(otl,feature_tag,sl);
    continue;
	  case ft_script:
	    ScriptLangListFree(sl);
	    sl = chunkalloc(sizeof(struct scriptlanglist));
	    sl->script = f->u1.tag;
	    sl->lang_cnt = 1;
	    sl->langs[0] = DEFAULT_LANG;
	    saw_script = true;
	    f = f->next;
    continue;
	  case ft_lang:
	    if ( !saw_script ) {
		ScriptLangListFree(sl);
		sl = chunkalloc(sizeof(struct scriptlanglist));
		sl->script = CHR('l','a','t','n');
	    }
	    sl->langs[0] = f->u1.tag;
	    sl->lang_cnt = 1;
	    if ( !f->u2.exclude_dflt ) {
		if ( sl->langs[0]!=DEFAULT_LANG ) {
		    sl->langs[1] = DEFAULT_LANG;
		    sl->lang_cnt = 2;
		}
	    }
	    f = f->next;
    continue;
	  case ft_langsys:
	    ScriptLangListFree(sl);
	    saw_script = false;
	    sl = f->u2.sl;
	    f->u2.sl = NULL;
	    f = f->next;
    continue;
	  case ft_sizeparams:
	    if ( f->u1.params!=NULL ) {
		tok->sf->design_size = f->u1.params[0];
		tok->sf->fontstyle_id = f->u1.params[1];
		tok->sf->design_range_bottom = f->u1.params[2];
		tok->sf->design_range_top = f->u1.params[3];
	    }
	    OtfNameListFree(tok->sf->fontstyle_name);
	    tok->sf->fontstyle_name = fea_NameID2OTFName(f->u2.names);
	    f = f->next;
    continue;
	  case ft_subtable:
	    f = f->next;
    continue;
	  case ft_pst:
	  case ft_pstclass:
	  case ft_ap:
	  case ft_fpst:
	    if ( f->type==ft_ap && f->u2.ap->type==at_mark ) {
		struct feat_item *n, *a;
		/* skip over the marks */
		for ( n=f; n!=NULL && n->type == ft_ap && n->u2.ap->type==at_mark; n=n->next );
		/* find the next thing which can use those marks (might not be anything) */
		for ( a=n; a!=NULL; a=a->next ) {
		    if ( a->ticked )
		continue;
		    if ( fea_FeatItemEndsLookup(a->type) ||
			    a->type==ft_subtable || a->type==ft_ap )
		break;
		}
		if ( a==NULL || fea_FeatItemEndsLookup(a->type) || a->type==ft_subtable ||
			(a->type==ft_ap && a->u2.ap->type == at_mark )) {
		    /* There's nothing else that can use these marks so we are */
		    /*  done with them. Skip over all of them */
		    f = n;
    continue;
		}
		ltype = fea_LookupTypeFromItem(a);
	    } else
		ltype = fea_LookupTypeFromItem(f);
	    start = f;
	    f = fea_SetLookupLink(start,ltype);
	    otl = fea_ApplyLookupList(tok,start,lookup_flags);
	    fea_AttachFeatureToLookup(otl,feature_tag,sl);
    continue;
	  default:
	    IError("Unexpected feature item in feature definition %d", f->type );
	    f = f->next;
	}
    }
    if ( f!=NULL && f->type == ft_feat_end )
	f = f->next;
return( f );
}

static void fea_ApplyFile(struct parseState *tok, struct feat_item *item) {
    struct feat_item *f, *start;

    for ( f=item; f!=NULL ; ) {
	switch ( f->type ) {
	  case ft_lookup_start:
	    start = f;
	    start->lookup_next = f->next;
	    f = fea_SetLookupLink(start->next,ot_undef);
	    if ( f!=NULL && f->type == ft_lookup_end )
		f = f->next;
	    fea_ApplyLookupList(tok,start,0);
    continue;
	  case ft_feat_start:
	    f = fea_ApplyFeatureList(tok,f);
    continue;
	  case ft_table:
	    /* I store things all mushed together, so this tag is useless to me*/
	    /*  ignore it. The stuff inside the table matters though... */
	    f = f->next;
    continue;
	  case ft_names:
	    fea_NameID2NameTable(tok->sf,f->u2.names);
	    f = f->next;
    continue;
	  case ft_tablekeys:
	    fea_TableByKeywords(tok->sf,f);
	    f = f->next;
    continue;
	  case ft_gdefclasses:
	    fea_GDefGlyphClasses(tok->sf,f);
	    f = f->next;
    continue;
	  case ft_lcaret:
	    fea_GDefLigCarets(tok->sf,f);
	    f = f->next;
    continue;
	  default:
	    IError("Unexpected feature item in feature file %d", f->type );
	    f = f->next;
	}
    }
}

static struct feat_item *fea_reverseList(struct feat_item *f) {
    struct feat_item *n = NULL, *p = NULL;

    p = NULL;
    while ( f!=NULL ) {
	n = f->next;
	f->next = p;
	p = f;
	f = n;
    }
return( p );
}

static void fea_NameLookups(struct parseState *tok) {
    SplineFont *sf = tok->sf;
    OTLookup *gpos_last=NULL, *gsub_last=NULL, *otl, *otlnext;
    int gp_cnt=0, gs_cnt=0, acnt;
    AnchorClass *ac, *acnext, *an;

    for ( otl = sf->gpos_lookups; otl!=NULL; otl=otl->next ) {
	otl->lookup_index = gp_cnt++;
	gpos_last = otl;
    }
    for ( otl = sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
	otl->lookup_index = gs_cnt++;
	gsub_last = otl;
    }

    for ( otl = tok->created; otl!=NULL; otl=otlnext ) {
	otlnext = otl->next;
	otl->next = NULL;
	if ( otl->lookup_name!=NULL && SFFindLookup(sf,otl->lookup_name)!=NULL ) {
	    int cnt=0;
	    char *namebuf = galloc(strlen( otl->lookup_name )+8 );
	    /* Name already in use, modify it */
	    do { 
		sprintf(namebuf,"%s-%d", otl->lookup_name, cnt++ );
	    } while ( SFFindLookup(sf,namebuf)!=NULL );
	    free(otl->lookup_name);
	    otl->lookup_name = namebuf;
	}
	if ( otl->lookup_type < gpos_start ) {
	    if ( gsub_last==NULL )
		sf->gsub_lookups = otl;
	    else
		gsub_last->next = otl;
	    gsub_last = otl;
	    otl->lookup_index = gs_cnt++;
	} else {
	    if ( gpos_last==NULL )
		sf->gpos_lookups = otl;
	    else
		gpos_last->next = otl;
	    gpos_last = otl;
	    otl->lookup_index = gp_cnt++;
	}
	NameOTLookup(otl,sf);		/* But only if it has no name */
    }

    /* Now name and attach any unnamed anchor classes (order here doesn't matter) */
    acnt = 0;
    for ( ac=tok->accreated; ac!=NULL; ac=acnext ) {
	acnext = ac->next;
	if ( ac->name==NULL ) {
	    char buf[50];
	    do {
		snprintf(buf,sizeof(buf),_("Anchor-%d"), acnt++ );
		for ( an=sf->anchor; an!=NULL && strcmp(an->name,buf)!=0; an=an->next );
	    } while ( an!=NULL );
	    ac->name = copy(buf);
	}
	ac->next = sf->anchor;
	sf->anchor = ac;
    }

    sf->changed = true;
    FVSetTitles(sf);
    FVRefreshAll(sf);
}
	
void SFApplyFeatureFile(SplineFont *sf,FILE *file,char *filename) {
    struct parseState tok;
    struct glyphclasses *gc, *gcnext;

    memset(&tok,0,sizeof(tok));
    tok.line[0] = 1;
    tok.filename[0] = filename;
    tok.inlist[0] = file;
    tok.base = 10;
    if ( sf->cidmaster ) sf = sf->cidmaster;
    tok.sf = sf;

    fea_ParseFeatureFile(&tok);
    if ( tok.err_count==0 ) {
	tok.sofar = fea_reverseList(tok.sofar);
	fea_ApplyFile(&tok, tok.sofar);
	fea_NameLookups(&tok);
    } else
	ff_post_error("Not applied","There were errors when parsing the feature file and the features have not been applied");
    fea_featitemFree(tok.sofar);
    ScriptLangListFree(tok.def_langsyses);
    for ( gc = tok.classes; gc!=NULL; gc=gcnext ) {
	gcnext = gc->next;
	free(gc->classname); free(gc->glyphs);
	chunkfree(gc,sizeof(struct glyphclasses));
    }
}

void SFApplyFeatureFilename(SplineFont *sf,char *filename) {
    FILE *in = fopen(filename,"r");

    if ( in==NULL ) {
	ff_post_error(_("Cannot open file"),_("Cannot open feature file %.120s"), filename );
return;
    }
    SFApplyFeatureFile(sf,in,filename);
    fclose(in);
}
