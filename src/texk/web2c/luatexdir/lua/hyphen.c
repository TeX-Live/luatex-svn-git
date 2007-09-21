/* Libhnj is dual licensed under LGPL and MPL. Boilerplate for both
 * licenses follows.
 */

/* LibHnj - a library for high quality hyphenation and justification
 * Copyright (C) 1998 Raph Levien, 
 * 	     (C) 2001 ALTLinux, Moscow (http://www.alt-linux.org), 
 *           (C) 2001 Peter Novodvorsky (nidd@cs.msu.su)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
 * Boston, MA  02111-1307  USA.
*/

/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "MPL"); you may not use this file except in
 * compliance with the MPL.  You may obtain a copy of the MPL at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the MPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the MPL
 * for the specific language governing rights and limitations under the
 * MPL.
 *
 */
#include <stdlib.h> /* for NULL, malloc */
#include <stdio.h>  /* for fprintf */
#include <string.h> /* for strdup */
#include <stdlib.h> /* for malloc used by substring inclusion*/

#define MAXPATHS 40960

#ifdef UNX
#include <unistd.h> /* for exit */
#endif

#define VERBOSE

#include "hnjalloc.h"
#include "hyphen.h"

#define free(a)

static char *
hnj_strdup (const char *s)
{
  char *new;
  int l;

  l = strlen (s);
  new = hnj_malloc (l + 1);
  memcpy (new, s, l);
  new[l] = 0;
  return new;
}

/* a little bit of a hash table implementation. This simply maps strings
   to state numbers */

typedef struct _HashTab HashTab;
typedef struct _HashEntry HashEntry;

/* A cheap, but effective, hack. */
#define HASH_SIZE 31627

struct _HashTab {
  HashEntry *entries[HASH_SIZE];
};

struct _HashEntry {
  HashEntry *next;
  char *key;
  int val;
};

//
//
static void die(
  const char*msg
) {
  fprintf(stderr,"%s\n",msg);
  exit(1);
}


// Finds the index of an entry, only used on xxx_key arrays
// Caveat: the table has to be sorted
static int find_in(
  char *tab[],
  int max,
  const char *pat
) {
  int left=0, right=max-1;
  while (left <= right) {
    int mid = ((right-left)/2)+left;
    int v   = strcmp(pat,tab[mid]);
    if (v>0) {
      left = mid + 1;
    } else if (v<0) {
      right = mid -1;
    } else {
      return mid;
    }
  }
  return -1;
}


// Combine two right-aligned number patterns, 0004000 + 020 becomes .a2d4der
// The second pattern needs to be a substring of the first (modulo digits)
static char *combine(
  char *expr,
  const char *subexpr
) {
  int l1 = strlen(expr);
  int l2 = strlen(subexpr);
  int off = l1-l2;
  int j;
  // this works also for utf8 sequences because the substring is identical
  // to the last substring-length bytes of expr except for the (single byte)
  // hyphenation encoders
  for (j=0; j<l2; j++) {
    if (subexpr[j]>expr[off+j]) {
      expr[off+j] = subexpr[j];
    }
  }
  return expr;
}


// used by partition (which is used by qsort_arr)
//
static void swap2(
  char *a[],
  char *b[],
  int i,
  int j
) {
  if (i==j) return;
  char*v;
  v=a[i]; a[i]=a[j]; a[j]=v;
  v=b[i]; b[i]=b[j]; b[j]=v;
}


// used by qsort_arr
//
static int partition(
  char *a[],
  char *b[],
  int left,
  int right,
  int p
) {
  const char *pivotValue = a[p];
  int i;
  swap2(a,b,p,right); // Move pivot to end
  p = left;
  for (i=left; i<right; i++) {
    if (strcmp(a[i],pivotValue)<=0) {
      swap2(a,b,p,i);
      p++;
    }
  }
  swap2(a,b,right,p); // Move pivot to its final place
  return p;
}


// not full recursive: tail end the biggest part
//
static void qsort_arr(
  char *a[],
  char *b[],
  int left,
  int right
) {
  while (right > left) {
    int p = left + (right-left)/2; //select a pivot
    p = partition(a,b, left, right, p);
    if ((p-1) - left < right - (p+1)) {
      qsort_arr(a,b, left, p-1);
      left  = p+1;
    } else {
      qsort_arr(a,b, p+1, right);
      right = p-1;
    }
  }
}


// --------------------------------------------------------------------
// ORIGINAL CODE
// --------------------------------------------------------------------


//
//
/* a char* hash function from ASU - adapted from Gtk+ */
static unsigned int
hnj_string_hash (const char *s)
{
  const char *p;
  unsigned int h=0, g;

  for(p = s; *p != '\0'; p += 1) {
    h = ( h << 4 ) + *p;
    if ( ( g = h & 0xf0000000 ) ) {
      h = h ^ (g >> 24);
      h = h ^ g;
    }
  }
  return h /* % M */;
}


//
//
static HashTab *hnj_hash_new (void) {
  HashTab *hashtab;
  int i;

  hashtab = hnj_malloc (sizeof(HashTab));
  for (i = 0; i < HASH_SIZE; i++)
    hashtab->entries[i] = NULL;

  return hashtab;
}


//
//
static void hnj_hash_free(
  HashTab *hashtab
) {
  int i;
  HashEntry *e, *next;

  for (i = 0; i < HASH_SIZE; i++) {
    for (e = hashtab->entries[i]; e; e = next) {
      next = e->next;
      hnj_free (e->key);
      hnj_free (e);
    }
  }
  hnj_free (hashtab);
}


//
/* assumes that key is not already present! */
static void hnj_hash_insert(
  HashTab *hashtab,
  const char *key,
  int val
) {
  int i;
  HashEntry *e;

  i = hnj_string_hash (key) % HASH_SIZE;
  e = hnj_malloc (sizeof(HashEntry));
  e->next = hashtab->entries[i];
  e->key = hnj_strdup (key);
  e->val = val;
  hashtab->entries[i] = e;
}


//
/* return val if found, otherwise -1 */
static int hnj_hash_lookup(
  HashTab *hashtab,
  const char *key
) {
  int i;
  HashEntry *e;

  i = hnj_string_hash (key) % HASH_SIZE;
  for (e = hashtab->entries[i]; e; e = e->next) {
    if (!strcmp (key, e->key)) {
      return e->val;
    }
  }
  return -1;
}


//
/* Get the state number, allocating a new state if necessary. */
static int hnj_get_state(
  HyphenDict *dict,
  HashTab *hashtab,
  const char *string
) {
  int state_num;

  state_num = hnj_hash_lookup (hashtab, string);

  if (state_num >= 0)
    return state_num;

  hnj_hash_insert (hashtab, string, dict->num_states);
  /* predicate is true if dict->num_states is a power of two */
  if (!(dict->num_states & (dict->num_states - 1))) {
    dict->states = hnj_realloc(
      dict->states,
      (dict->num_states << 1) * sizeof(HyphenState));
  }
  dict->states[dict->num_states].match = NULL;
  dict->states[dict->num_states].fallback_state = -1;
  dict->states[dict->num_states].num_trans = 0;
  dict->states[dict->num_states].trans = NULL;
  return dict->num_states++;
}


//
/* add a transition from state1 to state2 through ch - assumes that the
   transition does not already exist */
static void hnj_add_trans(
  HyphenDict *dict,
  int state1,
  int state2,
  int uni_ch
) {
  int num_trans;

  num_trans = dict->states[state1].num_trans;
  if (num_trans == 0) {
    dict->states[state1].trans = hnj_malloc(sizeof(HyphenTrans));
  } else if (!(num_trans & (num_trans - 1))) {
    dict->states[state1].trans = hnj_realloc(
      dict->states[state1].trans,
      (num_trans << 1) * sizeof(HyphenTrans));
  }
  dict->states[state1].trans[num_trans].uni_ch = uni_ch;
  dict->states[state1].trans[num_trans].new_state = state2;
  dict->states[state1].num_trans++;
}


#ifdef VERBOSE
HashTab *global;

//
//
static char *get_state_str(
  int state
) {
  int i;
  HashEntry *e;

  for (i = 0; i < HASH_SIZE; i++)
    for (e = global->entries[i]; e; e = e->next)
      if (e->val == state)
	return e->key;
  return NULL;
}
#endif


/* I've changed the semantics a bit here: hnj_hyphen_load used to
   operate on a file, but now the argument is a string buffer.
 */

static size_t
next_pattern(char *target, int max, const char *buf) {
  int i = 0;
  char *rover = (char *)buf;
  target[0] = 0;
  while ((rover-buf)<max && *rover && isspace(*rover)) {
	rover++;
  }
  while (*rover) {
	if ((rover-buf)<max && !isspace(*rover)) {
	  target[i++] = *rover++;
	} else {
	  target[i] = 0;
	  return (rover-buf);
	}
  }
  return 0; /* zero sensed */
}

//
//
HyphenDict * hnj_hyphen_load(
  const char *f
) {
  HyphenDict *dict;
  HashTab *hashtab;
  char buf[80];
  int state_num, last_state;
  int i, j;
  char ch;
  int found;
  HashEntry *e;
  int p;

  char *pattab_key[MAXPATHS];
  char *pattab_val[MAXPATHS];
  int   patterns = 0;
  char *newpattab_key[MAXPATHS];
  char *newpattab_val[MAXPATHS];
  int   newpatterns = 0;

  size_t l = 0;
  hashtab = hnj_hash_new ();
#ifdef VERBOSE
  global = hashtab;
#endif
  hnj_hash_insert (hashtab, "", 0);

  dict = hnj_malloc (sizeof(HyphenDict));
  dict->num_states = 1;
  dict->states = hnj_malloc (sizeof(HyphenState));
  dict->states[0].match = NULL;
  dict->states[0].fallback_state = -1;
  dict->states[0].num_trans = 0;
  dict->states[0].trans = NULL;

//***************************************

  char format[132]; // 64+65+newline+zero+spare
  while((l = next_pattern(format,(sizeof(format)-1),f))>0) {
	int i,j;
	f += l;
	//printf("%s\n",format);
	for (i=0,j=0; i<l; i++) if (format[i]>='0'&&format[i]<='9') j++;
	char *pat = (char*) malloc(1+l-j);
	char *org = (char*) malloc(2+l-j);
	// remove hyphenation encoders (digits) from pat
	org[0] = '0';
	for (i=0,j=0; i<l; i++) {
	  char c = format[i];
	  if (c<'0' || c>'9') {
		pat[j++] = c;
		org[j]   = '0';
	  } else {
		org[j]   = c;
	  }
	}
	pat[j]   = 0;
	org[j+1] = 0;
	pattab_key[patterns]   = pat;
	pattab_val[patterns++] = org;
	if (patterns>=MAXPATHS) die("too many base patterns");
  }
  // As we use binairy search, make sure it is sorted
  qsort_arr(pattab_key,pattab_val,0,patterns-1);

  for (p=0; p<patterns; p++) {
    char *pat = pattab_key[p];
    int   patsize = strlen(pat);
    int   j,l;
    for (l=1; l<=patsize; l++) {
      for (j=1; j<=l; j++) {
        int i = l-j;
        int  subpat_ndx;
        char subpat[132];
        strncpy(subpat,pat+i,j); subpat[j]=0;
        if ((subpat_ndx = find_in(pattab_key,patterns,subpat))>=0) {
          int   newpat_ndx;
          char *newpat=malloc(l+1);
		  //printf("%s is embedded in %s\n",pattab_val[subpat_ndx],pattab_val[p]);
          strncpy(newpat, pat+0,l); newpat[l]=0;
          if ((newpat_ndx = find_in(newpattab_key,newpatterns,newpat))<0) {
            char *neworg = malloc(l+2);
            sprintf(neworg,"%0*d",l+1,0); // fill with right amount of '0'
            newpattab_key[newpatterns]   = newpat;
            newpattab_val[newpatterns++] = combine(neworg,pattab_val[subpat_ndx]);
            if (newpatterns>MAXPATHS) die("to many new patterns");
			//  printf("%*.*s|%*.*s[%s] (%s|%s) = %s\n",i,i,pat,j,j,pat+i,pat+i+j,pattab_val[p],pattab_val[subpat_ndx],neworg);
          } else {
            free(newpat);
            newpattab_val[newpat_ndx] = combine(
              newpattab_val[newpat_ndx], pattab_val[subpat_ndx] ); 
          }
        }
      }
    }
  }

  for (p=0; p<patterns; p++) {
    free(pattab_key[p]);
    free(pattab_val[p]);
  }
  for (p=0; p<newpatterns; p++) {
    char *word    = newpattab_key[p];
    char *pattern = newpattab_val[p];
    /* Optimize away leading zeroes */
    for (i = 0; pattern[i] == '0'; i++) {}

#ifdef VERBOSE
    printf ("word %s pattern %s, j = %d\n", word, pattern + i, j);
#endif
    found = hnj_hash_lookup( hashtab, word );
    state_num = hnj_get_state( dict, hashtab, word );
    dict->states[state_num].match = hnj_strdup( pattern + i );

    /* now, put in the prefix transitions */
    for (; found < 0 ;j--) {
      last_state = state_num;
      ch = word[j - 1];
      word[j - 1] = '\0';
      found = hnj_hash_lookup (hashtab, word);
      state_num = hnj_get_state (dict, hashtab, word);
      hnj_add_trans (dict, state_num, last_state, ch);
    }
    free(newpattab_key[p]);
    free(newpattab_val[p]);
  }

//***************************************

  /* put in the fallback states */
  for (i = 0; i < HASH_SIZE; i++) {
    for (e = hashtab->entries[i]; e; e = e->next) {
      for (j = 1; 1; j++) {
        state_num = hnj_hash_lookup (hashtab, e->key + j);
        if (state_num >= 0) break;
      }
      /* KBH: FIXME state 0 fallback_state should always be -1? */
      if (e->val) dict->states[e->val].fallback_state = state_num;
    }
  }
#ifdef VERBOSE
  for (i = 0; i < HASH_SIZE; i++) {
    for (e = hashtab->entries[i]; e; e = e->next) {
      printf ("%d string %s state %d, fallback=%d\n", i, e->key, e->val,
        dict->states[e->val].fallback_state);
      for (j = 0; j < dict->states[e->val].num_trans; j++) {
        printf (" %c->%d\n", dict->states[e->val].trans[j].uni_ch,
          dict->states[e->val].trans[j].new_state);
      }
    }
  }
#endif

#ifndef VERBOSE
  hnj_hash_free (hashtab);
#endif

  return dict;
}


//
//
void hnj_hyphen_free(
  HyphenDict *dict
) {
  int state_num;
  HyphenState *hstate;

  for (state_num = 0; state_num < dict->num_states; state_num++) {
    hstate = &dict->states[state_num];
    if (hstate->match) hnj_free (hstate->match);
    if (hstate->trans) hnj_free (hstate->trans);
  }
  hnj_free (dict->states);
  hnj_free (dict);
}

#define MAX_WORD 256


//
//
int hnj_hyphen_hyphenate(
  HyphenDict *dict,
  const char *word,
  int word_size,
  char *hyphens
) {
  char prep_word_buf[MAX_WORD];
  char *prep_word;
  int i, j, k;
  int state;
  char ch;
  HyphenState *hstate;
  char *match;
  int offset;

  if (word_size + 3 < MAX_WORD)
    prep_word = prep_word_buf;
  else
    prep_word = hnj_malloc (word_size + 3);

  j = 0;
  prep_word[j++] = '.';
  for (i = 0; i < word_size; i++) prep_word[j++] = word[i];
  for (i = 0; i < j; i++) hyphens[i] = '0';    
  prep_word[j++] = '.';
  prep_word[j] = '\0';
#ifdef VERBOSE
  printf ("prep_word = %s\n", prep_word);
#endif

  /* now, run the finite state machine */
  state = 0;
  for (i = 0; i < j; i++) {
    ch = prep_word[i];
    for (;;) {
      if (state == -1) {
        /* return 1; */
	/*  KBH: FIXME shouldn't this be as follows? */
        state = 0;
        goto try_next_letter;
      }          

#ifdef VERBOSE
      char *state_str;
      state_str = get_state_str (state);
	  for (k = 0; k < i - strlen (state_str); k++) putchar (' ');
	  printf ("%s", state_str);
#endif

      hstate = &dict->states[state];
      for (k = 0; k < hstate->num_trans; k++) {
        if (hstate->trans[k].uni_ch == ch) {
          state = hstate->trans[k].new_state;
	  goto found_state;
        }
      }
      state = hstate->fallback_state;
#ifdef VERBOSE
      printf (" falling back, fallback_state %d\n", state);
#endif
    }
found_state:
#ifndef VERBOSE
    printf ("found state %d\n",state);
#endif
    /* Additional optimization is possible here - especially,
       elimination of trailing zeroes from the match. Leading zeroes
       have already been optimized. */
    match = dict->states[state].match;
    if (match) {
      offset = i + 1 - strlen (match);
#ifdef VERBOSE
      for (k = 0; k < offset; k++) putchar (' ');
      printf ("%s\n", match);
#endif
      /* This is a linear search because I tried a binary search and
         found it to be just a teeny bit slower. */
      for (k = 0; match[k]; k++) {
        if (hyphens[offset + k] < match[k]) hyphens[offset + k] = match[k];
      }
    }

      /* KBH: we need this to make sure we keep looking in a word */
      /* for patterns even if the current character is not known in state 0 */
      /* since patterns for hyphenation may occur anywhere in the word */
try_next_letter: ;
  }
#ifdef VERBOSE
  for (i = 0; i < j; i++) putchar (hyphens[i]);
  putchar ('\n');
#endif

  for (i = 0; i < j - 4; i++) {
#if 0
    if (hyphens[i + 1] & 1)
      hyphens[i] = '-';
#else
    hyphens[i] = hyphens[i + 1];
#endif
  }
  hyphens[0] = '0';
  for (; i < word_size; i++) hyphens[i] = '0';
  hyphens[word_size] = '\0';

  if (prep_word != prep_word_buf) hnj_free (prep_word);
  return 0;    
}


#if 0 /* ignore for the moment */
//
//
int hnj_hyphen_tok_hyphenate(
  HyphenDict *dict,
  TACO_TOKEN *word,
  int hyphen_value
) {
  int cur_pos, state = 0;

  TACO_TOKEN lead = {NULL, TOKEN_TYPE_CHARACTER|'.'};
  TACO_TOKEN tail = {NULL, TOKEN_TYPE_CHARACTER|'.'};

  // insert starting and trailing dots
  lead.next = word;
  while (IS_WORD_PART_TOKEN(TOKEN_TYPE(word->ptr->data))) word = word->ptr;
  tail.ptr  = word->ptr;
  word->ptr = &tail;
  word = &lead;

  // WAS: prefill hyphens with ascii zero's

  /* now, run the finite state machine */
  state = 0;
  for (cur_pos=0; word!=tail.ptr; word=word->ptr) {
    if (IS_CHARACTER_TOKEN(TOKEN_TYPE(word->data))) {
      char *match;
      UniChar ch = word->data & UNI_CHAR_MASK;
      for (;;) {
        HyphenState *hstate;
        int k;
        if (state == -1) {
          /* return 1; */
	  /*  KBH: FIXME shouldn't this be as follows? */
          state = 0;
          goto try_next_letter;
        }          

        hstate = &dict->states[state];
        for (k = 0; k < hstate->num_trans; k++) {
          if (hstate->trans[k].ch == ch) {
            state = hstate->trans[k].new_state;
	    goto found_state;
          }
        }
        state = hstate->fallback_state;
      }
found_state:
      /* Additional optimization is possible here - especially,
         elimination of trailing zeroes from the match. Leading zeroes
         have already been optimized. */
      match = dict->states[state].match;
      if (match) {
        TACO_TOKEN *htoken = &head;
        int offset = cur_pos + 1 - strlen (match);
        assert(offset>=0);
        while (offset) {
          if (IS_CHARACTER_TOKEN(TOKEN_TYPE(htoken->data))) offset--;
          htoken = htoken->ptr;
        }
        for (; *match; htoken=htoken->ptr) {
          if (IS_CHARACTER_TOKEN(TOKEN_TYPE(htoken->data))) {
            int v = (*match++-'0')<<TOKEN_SCRATCH_SHIFT;
            if (v) {
              int w = htoken->data & TOKEN_SCRATCH_MASK;
              if (w<v) {
                htoken->data &= ~TOKEN_SCRATCH_MASK;
                htoken->data |= v;
              }
            }
          }
        }
        assert(htoken==word);
      }
      cur_pos++;
    }
try_next_letter: ;
  }
  // cur_pos is the total amount of characters, including both '.'s
  cur_pos -= 2;
  // cur_pos is the total amount of characters, excluding both '.'s
  int skip_first = left_hyphen_min-1;
  int skip_last  = cur_pos - right_hyphen_min;
  if (compat_hyphen_max_length>skip_last)
    skip_last = compat_hyphen_max_length;
  // we must 0 de scratchs of
  //   the first left_hyphen_min-1, and
  //   the last MAX(right_hyphen_min-1, cur_pos - compat_hyphen_max_length)
  // replace all odd scratches with a following hyphen-token,
  // except when it is already followed by a hyphen: then OR the value
  for (word = lead.next, cur_pos=0; word->ptr != &tail; word=word->ptr) {
    if (IS_CHARACTER_TOKEN(TOKEN_TYPE(word->data))) {
      int w = (word->data & TOKEN_SCRATCH_MASK)>>TOKEN_SCRATCH_SHIFT;
      if (w) {
        word->data &= ~TOKEN_SCRATCH_MASK;
        if (w&1 && cur_pos>=skip_first && cur_pos<skip_last) { // hyphenate
          if (! IS_HYPHEN_TOKEN(TOKEN_TYPE(word->ptr->data))) {
            TACO_TOKEN &hyphen = (TACO_TOKEN*)malloc(sizeof(TACO_TOKEN));
            hyphen->ptr = word.ptr;
            hyphen->data = HYPHEN_TOKEN_TYPE;
            word->ptr = hyphen;
          }
          word->ptr->data |= hyphen_value;
        } else { // no hyphenation
          // even if there's a hyphen following, we keep it, as
          // we assume the party inserting knew what it was doing
        }
      }
      cur_pos++;
    }
  }
  word->data &= ~TOKEN_SCRATCH_MASK; // clear bits of last char (never hyphen)
  word->ptr   = word->ptr->ptr; // remove trailing '.'

  return 0;    
}

#endif
