/*
Copyright (c) 2007 Taco Hoekwater <taco@latex.org>

This file is part of luatex.

luatex is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

luatex is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with luatex; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

This is texlang.c
*/

#include "luatex-api.h"
#include <ptexlib.h>

#include "../lua/nodes.h"

/* functions from the fontforge unicode library */

extern unsigned int *utf82u_strcpy(unsigned int *ubuf,const char *utf8buf);
extern unsigned int u_strlen(unsigned int *ubuf);
extern char *utf8_idpb(char *w,unsigned int i);

#define noVERBOSE

#define MAX_TEX_LANGUAGES 256

int use_new_hyphenation = 0;

static struct tex_language *tex_languages[MAX_TEX_LANGUAGES] = {NULL};
static int next_lang_id = 0;

struct tex_language *
new_language (void) {
  struct tex_language* lang;
  if (next_lang_id<MAX_TEX_LANGUAGES) {
	lang = xmalloc(sizeof(struct tex_language));
	tex_languages[next_lang_id] = lang;
	lang->id = next_lang_id++;
	lang->lhmin = 2;
	lang->rhmin = 3;
	lang->exceptions = 0;
	lang->patterns = NULL;
	return lang;
  } else {
	return NULL;
  }
}

void 
load_patterns (struct tex_language *lang, unsigned char *buffer) {
  if (lang->patterns==NULL) {
	lang->patterns = hnj_hyphen_new();
  } else {
	hnj_hyphen_clear(lang->patterns);
  }
  hnj_hyphen_load (lang->patterns,buffer);
}

#define STORE_WORD()                    \
  if (w>0) {                            \
    word[w] = 0;                        \
    *s = 0;                             \
    lua_pushlstring(L,(char *)word,w);  \
    lua_pushlstring(L,value,(s-value)); \
    lua_rawset(L,-3);                   \
    w=0;                                \
}

#define STORE_CHAR(x) { word[w] = x ; if (w<MAX_WORD_LEN) w++; }
 
void
load_hyphenation (struct tex_language *lang, unsigned char *buffer) {
  char *s, *value;
  unsigned char word [MAX_WORD_LEN+1];
  int w = 0;
  lua_State *L = Luas[0];
  if (lang->exceptions==0) {
    lua_newtable(L);
    lang->exceptions = luaL_ref(L,LUA_REGISTRYINDEX);
  }
  lua_rawgeti(L, LUA_REGISTRYINDEX, lang->exceptions);
  s = value = (char *)buffer;
  while (*s) {
	if (isspace(*s)) {
      STORE_WORD();
      value = s+1;
    } else if (*s == '-') {	 /* skip */
    } else if (*s == '=') {
      STORE_CHAR('-');
	  *s = '-' ; 
    } else {
      STORE_CHAR(*s); 
    }
    s++;
  }
  STORE_WORD();   /* fix a trailing word */
}


int
hyphenate_string(struct tex_language *lang, char *w, char **ret) {
  int i;
  unsigned len;
  unsigned int wword [(4*MAX_WORD_LEN)+1] = {0};
  char hyphenated[(2*MAX_WORD_LEN)+1] = {0};
  char hyphens[MAX_WORD_LEN] = {0};
  char *hy = hyphenated;
  if (lang->exceptions!=0) {
	lua_State *L = Luas[0];
	lua_rawgeti(L,LUA_REGISTRYINDEX,lang->exceptions);
	if (lua_istable(L,-1)) { /* ?? */
	  lua_pushstring(L,w);    /* word table */
	  lua_rawget(L,-2);    
	  if (lua_isstring(L,-1)) { /* ?? table */
		hy = (char *)lua_tostring(L,-1);
		lua_pop(L,2);
		/* TODO: process special {}{}{} stuff into ret */
		*ret = xstrdup(hy);
		return 1; 
	  } else {
		lua_pop(L,2); 
	  }
	} else {
	  lua_pop(L,1); 
	}
  };
  len = strlen(w);
  if (len>(4*MAX_WORD_LEN)) {
	*ret = "word too long";
	return 0;
  }
  if (len==0) {
	*ret = xstrdup(w);
	return 1;
  }
  (void)utf82u_strcpy((unsigned int *)wword,w);
  len = u_strlen((unsigned int *)wword); 
  if (len>MAX_WORD_LEN) {
	*ret = "word too long";
	return 0;
  }
  if (len==0) {
	*ret = xstrdup(w);
	return 1;
  }
  (void)hnj_hyphen_hyphenate(lang->patterns,(int *)wword,len,hyphens);
  for (i=0;i<len;i++) {
	hy = utf8_idpb(hy,wword[i]);
	if (i<(len-lang->rhmin) && i>=(lang->lhmin-1) && hyphens[i]!='0')
	  *hy++ = '-';
  }
  *hy=0;
  *ret = xstrdup(hyphenated);
  return 1;
}




#define is_starting_letter(r) (type(r)==glyph_node && subtype(r)==0 && get_lc_code(character(r))!=0)
#define is_skipped(r) ((type(r)==kern_node && subtype(r)==0)                || \
		       (type(r)==whatsit_node && subtype(r)!=language_node) || \
		       (type(r)==glyph_node))

#define forget_word(r)  { while(is_skipped(vlink(r))) r=vlink(r); }

/* todo: \hyphenation exceptions
 *       \savinghyphcodes 
 * incomp: first word hyphenated 
 *         rules for word boundaries are different 
 */

void 
new_hyphenation (halfword head, halfword tail, int init_cur_lang, int init_lhyf, int init_rhyf, int uc_hyph) {
  int lhmin, rhmin,curlang, l,i;
  halfword g,t,r, wordstart;
  struct tex_language* lang;
  int wword[MAX_WORD_LEN+1] = {0};
  char hyphens[MAX_WORD_LEN] = {0};
  int wordlen = 0;

  assert(head!=null);
  if (vlink(head)==null)
    return;
  assert (init_cur_lang>=0);
  assert (init_cur_lang<MAX_TEX_LANGUAGES);
  assert (init_lhyf>0);
  assert (init_rhyf>0);
  assert (tail!=null);
  assert (type(tail)!=glyph_node);

  r = head;
  wordstart = null;
  lhmin = init_lhyf;
  rhmin = init_rhyf;
  curlang = init_cur_lang;

#ifdef VERBOSE
  fprintf(stderr,"start: lang=%d,lhm=%d,rhm=%d\n",curlang,lhmin,rhmin);
#endif

  while(r!=tail && r!=null) {
    r = vlink(r);    
    if (is_starting_letter(r)) {
      l = get_lc_code(character(r));
      if (wordlen==0 && (!uc_hyph) && character(r)!=l) {
	forget_word(r);
      } else {
	if (wordlen==0)
	  wordstart=r;
	wword[wordlen++] = l;
      }
    } else if (is_skipped(r)) {
      /* ignored items */
    } else if (type(r)==math_node) {
      r = vlink(r);
      while (type(r)!=math_node && r!=null)
	r = vlink(r);
    } else {
      if (wordlen>=(lhmin+rhmin) && (tex_languages[curlang]!=NULL)) {
#ifdef VERBOSE
	fprintf(stderr,"found a word: ");
	for (i=0;i<(wordlen);i++) fprintf(stderr,"%c", wword[i]);
	fprintf(stderr,"\n");
#endif	
	lang = tex_languages[curlang];
	(void)hnj_hyphen_hyphenate(lang->patterns,(int *)wword,wordlen,hyphens);
	/* fixup hyphens[] for lhmin and rhmin */
	for (i=0;i<wordlen;i++)   {
	  if (i<(lhmin-1))
	    hyphens[i]='0';
	  if (i>=(wordlen-rhmin))
	    hyphens[i]='0';
	}
#ifdef VERBOSE
	fprintf (stderr,"clean hyphens: %s\n", hyphens);
#endif
	/* if there are no viable hyphenation points in the word,
	   there no point in looping over the node list */
	for (i=0;i<(wordlen-1);i++) {
	  if (hyphens[i]!='0')
	    break;
	}
	/* now seed hyphens, starting from wordstart */
	if (i<(wordlen-1)) {
#ifdef VERBOSE
	  fprintf (stderr,"processing\n");
#endif
	  i = 0;
	  for (t=wordstart;vlink(t)!=r;t=vlink(t)) {
	    if (type(t)==glyph_node) {
	      if (hyphens[i]!='0') {
                g = lua_node_new(disc_node,0);
		vlink(g) = vlink(t);
		vlink(t) = g;
		g = lua_node_new(glyph_node,0);
		font(g) = font(t);
		character(g) = '-';
		if (node_attr(t)!=null) {
		  node_attr(g) = node_attr(t);
		  node_attr(vlink(t)) = node_attr(t);
		  attr_list_ref(node_attr(t)) += 2; 
		}
		t = vlink(t);
		pre_break(t) = g;
	      } 
	      i++;
	    }
	  }
	}
      }
      wordlen = 0;
      if (type(r)==whatsit_node &&  subtype(r)==language_node) {
	curlang = what_lang(r);
	lhmin = what_lhm(r);
	if (lhmin<1) lhmin=1;
	rhmin = what_rhm(r);
	if (rhmin<1) rhmin=1;
#ifdef VERBOSE
	fprintf(stderr,"node(%d): lang=%d,lhm=%d,rhm=%d\n",r,curlang,lhmin,rhmin);
#endif
      }
    }
  }
}


