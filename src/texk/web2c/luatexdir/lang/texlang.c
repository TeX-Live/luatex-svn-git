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

#include "nodes.h"
#include "hyphen.h"

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
	lang->uchyph = 0;
	lang->exceptions = 0;
	lang->patterns = NULL;
	return lang;
  } else {
	return NULL;
  }
}

struct tex_language *
get_language (int n) {
  if (n>=0 && n<=MAX_TEX_LANGUAGES) {
    return tex_languages[n];
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
#if 0
  (void)hnj_hyphen_hyphenate(lang->patterns,(int *)wword,len,hyphens);
#endif
  for (i=0;i<len;i++) {
	hy = utf8_idpb(hy,wword[i]);
	if (i<(len-lang->rhmin) && i>=(lang->lhmin-1) && hyphens[i]!='0')
	  *hy++ = '-';
  }
  *hy=0;
  *ret = xstrdup(hyphenated);
  return 1;
}

/* these are just a temporary measure to smooth out the interface between 
   libkhnj (hyphen.c) and luatex (texlang.c).
*/

void set_vlink (halfword t, halfword v) {  
  vlink(t) = v;
}

halfword get_vlink(halfword t) {
  return vlink(t);
}

int get_character(halfword t) {
  return character(t);
}




halfword insert_discretionary ( halfword t,  halfword pre,  halfword post,  int replace) {
  halfword g;
#ifdef verbose
  fprintf(stderr,"disc (%d,%d,%d) after %c\n", pre, post, replace, character(t));
#endif
  g = lua_node_new(disc_node,0);
  vlink(g) = vlink(t);
  vlink(t) = g;
  for (g=pre;g!=null;g =vlink(g)) {
    font(g)=font(t);
    if (node_attr(t)!=null) {
      node_attr(g) = node_attr(t);
      attr_list_ref(node_attr(t)) += 1; 
    }
  }
  for (g=post;g!=null;g =vlink(g)) {
    font(g)=font(t);
    if (node_attr(t)!=null) {
      node_attr(g) = node_attr(t);
      attr_list_ref(node_attr(t)) += 1; 
    }
  }
  if (node_attr(t)!=null) {
    node_attr(vlink(t)) = node_attr(t);
    attr_list_ref(node_attr(t)) += 1; 
  }
  t = vlink(t);
  pre_break(t) = pre;
  post_break(t) = post;
  replace_count(t) = replace;
  return t;
}

halfword insert_character ( halfword t,  int c) {
  halfword g;
  g = lua_node_new(glyph_node,0);
  character(g)=c;
  if (t!=null) {
    vlink(t)=g;
  }
  return g;
}


char *hyphenation_exception(int curlang, char *w) {
  char *ret = NULL;
  lua_State *L = Luas[0];
  lua_checkstack(L,2);
  lua_rawgeti(L,LUA_REGISTRYINDEX,(tex_languages[curlang])->exceptions);
  if (lua_istable(L,-1)) { /* ?? */
    lua_pushstring(L,w);    /* word table */
    lua_rawget(L,-2);
    if (lua_isstring(L,-1)) {
      ret = xstrdup((char *)lua_tostring(L,-1));
    }
    lua_pop(L,2);
    return ret;
  } else {
    lua_pop(L,1);
    return ret;
  }
}

/* TODO : this is all wrong */

void do_exception (halfword wordstart, halfword r, char *replacement) {
  int i;
  halfword g,gg,t,h,hh;
  unsigned len;
  int uword[MAX_WORD_LEN+1] = {0};
  (void)utf82u_strcpy((unsigned int *)uword,replacement);
  len = u_strlen((unsigned int *)uword); 
  i = 0;
  t=wordstart;
  while (i<len) { /* still stuff to do */
    while (vlink(t)!=r && type(t)!=glyph_node)
      t = vlink(t);
    if (vlink(t)==r)
      break;
    if (uword[i+1] == '-') { /* a hyphen follows */
      g = lua_node_new(glyph_node,0);
      font(g) = font(t);
      character(g) = '-';    
      insert_discretionary(t,g,null,0);
      i++;
    } else if (uword[i+1] == '=') { 
      t = vlink(t);
      i++;
    } else if (uword[i+1] == '{') {
      /* a three-part exception */
      int repl = 0;
      i++;
      while (i<len && uword[i+1] != '}') { /* find the replace count */
	repl++; i++;
      }
      i++;
      if (i==len || uword[i+1] != '{') { perror ("broken pattern 1");  uexit(1); }
      i++; 
#ifdef VERBOSE	
      fprintf (stderr,"count: %d\n",repl);
#endif
      g = null; gg =null;
      while (i<len && uword[i+1] != '}') { /* find the prebreak text */
	if (g==null) {
	  gg = lua_node_new(glyph_node,0);
	  g = gg;
	} else {
	  vlink(g) = lua_node_new(glyph_node,0);
	  g = vlink(g);
	}
 	character(g) = uword[i+1];
#ifdef VERBOSE	
	fprintf (stderr,"prebreak: %c\n",uword[i+1]);
#endif
	i++;
      }
      i++;
      if (i==len || uword[i+1] != '{') { perror ("broken pattern 2");  uexit(1); }
      i++;
      h = null; hh =null;
      while (i<len && uword[i+1] != '}') {  /* find the postbreak text */
	if (h==null) {
	  hh = lua_node_new(glyph_node,0);
	  h = hh;
	} else {
	  vlink(h) = lua_node_new(glyph_node,0);
	  h = vlink(h);
	}
	character(h) = uword[i+1];
#ifdef VERBOSE	
	fprintf (stderr,"postbreak: %c\n",uword[i+1]);
#endif
	i++;
      }
      if (i==len) {
	perror ("broken pattern 3");
	uexit(1);
      }
      /* jump over the last right brace */
      t = insert_discretionary(t,gg,hh,repl);

    }
    t = vlink(t);
    i++;
  }
}

typedef struct _lang_variables {
  unsigned char lhmin;
  unsigned char rhmin;
  unsigned char curlang;
  unsigned char uc_hyph;
} lang_variables;

/* todo:  \hyphenation exceptions
 * incomp: first word hyphenated 
 */

halfword find_next_wordstart(halfword r, lang_variables *langdata) {
  int start_ok = 1;
  int l;
  while (r!=null) {
    switch (type(r)) {
    case glue_node:
      start_ok = 1;
      break;
    case glyph_node:
      if (start_ok) {
	l = get_lc_code(character(r));
	if (l>0) {
	  if (langdata->uc_hyph || l == character(r)) {
	    return r;
	  } else {
	    start_ok = 0;
	  }
	}
      }
      break;
    case math_node:
      r = vlink(r);
      while (r && type(r)!=math_node) 
	r = vlink(r);
      break;
    case whatsit_node:
      if (subtype(r)==language_node) {
	langdata->curlang = what_lang(r);
	langdata->lhmin   = what_lhm(r)>0 ? what_lhm(r) : 1;
	langdata->rhmin   = what_rhm(r)>0 ? what_rhm(r) : 1;
      }
      break;
    default:
      start_ok = 0;
      break;
    }
    r = vlink(r);
  }
  return r;
}

void fix_discs (halfword wordstart, halfword r) {
  halfword a,n,s,t;
  int f;
  a = null;
  for (n=wordstart;n!=r;n=vlink(n)) {
    if (type(n)==disc_node) {
      s = lua_node_new(glyph_node, 0);
      character(s) = '-';
      font(s) = f;
      if (a!=null) {
	node_attr(s) = a;
	attr_list_ref(node_attr(s)) += 1; 
      }
      if (pre_break(n) == null) {
	pre_break(n) = s;  
      } else {
	t = pre_break(n);
	while (vlink(t)!=null)
	  t = vlink(t);
	vlink(t) = s;  
      }
    } else {
      f = font(n);
      a = node_attr(n);
    }
  }
}

void 
hnj_hyphenation (halfword head, halfword tail, int init_cur_lang, int init_lhyf, int init_rhyf, int uc_hyph) {
  int lchar, i;
  struct tex_language* lang;
  lang_variables langdata;
  char utf8word[(4*MAX_WORD_LEN)+1] = {0};
  int wordlen = 0;
  char *hy = utf8word;
  char *replacement = NULL;
  halfword r = head, wordstart = null, save_tail = null, left = null, right = null;

  assert (init_cur_lang>=0);
  assert (init_cur_lang<MAX_TEX_LANGUAGES);
  assert (init_lhyf>0);
  assert (init_rhyf>0);
  assert (tail!=null);

  langdata.lhmin = init_lhyf;
  langdata.rhmin = init_rhyf;
  langdata.curlang = init_cur_lang;
  langdata.uc_hyph = uc_hyph;

  save_tail = vlink(tail);
  vlink(tail) = null;
  /* find the first character */
  while (r!=null && type(r)!=glyph_node)
    r =vlink(r);
  /* this will return r, unless the glyph was not a valid start letter */
  r = find_next_wordstart(r, &langdata); 
  while (r!=null) { /* could be while(1), but let's be paranoid */
    wordstart = r;
    while (r!=null && type(r)==glyph_node && (lchar=get_lc_code(character(r)))>0) {
      wordlen++;
      if (wordlen>=langdata.rhmin) {
	if (wordlen==langdata.rhmin)
	  right = wordstart;
	else
	  right = vlink(right);
      }
      r = vlink(r);
      hy = utf8_idpb(hy,lchar);
    }
    if (wordlen>=2 &&
	(lang=tex_languages[langdata.curlang])!=NULL) {
      *hy=0;
      if (lang->exceptions!=0 && 
	  (replacement = hyphenation_exception(langdata.curlang,utf8word))!=NULL) {
#ifdef VERBOSE
	fprintf(stderr,"replacing %s (c=%d) by %s\n",utf8word,langdata.curlang,replacement);
#endif		
	do_exception(wordstart,r,replacement);
	free(replacement);
      } else if (lang->patterns!=NULL && 
		 wordlen >= langdata.lhmin+langdata.rhmin) {
	left = wordstart;
	for (i=langdata.lhmin;i>1;i--)
	  left = vlink(left);
#ifdef VERBOSE
	fprintf(stderr,"hyphenate %s (c=%d,l=%d,r=%d) from %c to %c\n",utf8word,
		langdata.curlang,langdata.lhmin,langdata.rhmin,
		character(left), character(right));
#endif		
	(void)hnj_hyphen_hyphenate(lang->patterns,wordstart,r,wordlen,left,right); 
	fix_discs(wordstart,r);
      }
    }
    wordlen = 0;
    hy = utf8word;
    right = null;
    if (r==null)
      break;
    r = find_next_wordstart(r, &langdata);
  }
  vlink(tail) = save_tail;
}


void 
new_hyphenation (halfword head, halfword tail, int init_cur_lang, 
		 int init_lhyf, int init_rhyf, int uc_hyph) {
  int callback_id = 0;
  lua_State *L = Luas[0];
  callback_id = callback_defined(hyphenate_callback);
  if (head==null || vlink(head)==null)
    return;
  if (callback_id>0) {
    /* */
    lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
    lua_rawgeti(L,-1, callback_id);
    if (!lua_isfunction(L,-1)) {
      lua_pop(L,2);
      return;
    }
    nodelist_to_lua(L,head);
    nodelist_to_lua(L,tail);
    lua_pushnumber(L,init_cur_lang);
    lua_pushnumber(L,init_lhyf);
    lua_pushnumber(L,init_rhyf);
    lua_pushnumber(L,uc_hyph);
    if (lua_pcall(L,6,0,0) != 0) {
      fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
      lua_pop(L,2);
      lua_error(L);
      return;
    } 
    lua_pop(L,1);
  }  else {
    hnj_hyphenation(head,tail,init_cur_lang,init_lhyf,init_rhyf,uc_hyph);
  }
}
