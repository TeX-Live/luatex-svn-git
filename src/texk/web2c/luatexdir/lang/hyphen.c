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

#include <ctype.h>

//#define VERBOSE

#include "hnjalloc.h"
#include "hyphen.h"

/* SHOULD BE MOVED TO SEPARATE LIBRARY */
static unsigned char * hnj_strdup(
  const unsigned char *s
) {
  unsigned char *new;
  int l;

  l = strlen ((char*)s);
  new = hnj_malloc (l + 1);
  memcpy (new, s, l);
  new[l] = 0;
  return new;
}

// --------------------------------------------------------------------
//
// Type definitions
//
// --------------------------------------------------------------------

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
  unsigned char *key;
  int state;
};

/* State machine */
typedef struct _HyphenState HyphenState;
typedef struct _HyphenTrans HyphenTrans;
#define MAX_CHARS 256
#define MAX_NAME 20

struct _HyphenDict {
  int num_states;
  char cset[MAX_NAME];
  HyphenState *states;
  HashTab hashtab;
};

struct _HyphenState {
  char *match;
  int fallback_state;
  int num_trans;
  HyphenTrans *trans;
};

struct _HyphenTrans {
  int uni_ch;
  int new_state;
};


//
//
static void die(
  const char*msg
) {
  fprintf(stderr,"FATAL: %s\n",msg);
  exit(1);
}


// Finds the index of an entry, only used on xxx_key arrays
// Caveat: the table has to be sorted
static int find_in(
  unsigned char *tab[],
  int max,
  const unsigned char *pat,
  int len
) {
  int left=0, right=max-1;
  while (left <= right) {
    int mid = ((right-left)/2)+left;
    int v   = strncmp((char*)pat,(char*)tab[mid],len);
    if (v>0) {
      left = mid + 1;
    } else if (v<0 || tab[mid][len]!=0) {
      right = mid -1;
    } else {
      return mid;
    }
  }
  return -1;
}


// Combine two right-aligned number patterns, 04000 + 020 becomes 04020
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
    if (expr[off+j]<subexpr[j]) expr[off+j] = subexpr[j];
  }
  return expr;
}


// used by partition (which is used by qsort_arr)
//
static void swap2(
  unsigned char *a[],
           char *b[],
  int i,
  int j
) {
  if (i==j) return;
  unsigned char*w;
           char*v;
  w=a[i]; a[i]=a[j]; a[j]=w;
  v=b[i]; b[i]=b[j]; b[j]=v;
}


// used by qsort_arr
//
static int partition(
  unsigned char *a[],
           char *b[],
  int left,
  int right,
  int p
) {
  const unsigned char *pivotValue = a[p];
  int i;
  swap2(a,b,p,right); // Move pivot to end
  p = left;
  for (i=left; i<right; i++) {
    if (strcmp((char*)a[i],(char*)pivotValue)<=0) {
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
  unsigned char *a[],
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
hnj_string_hash (const unsigned char *s)
{
  const unsigned char *p;
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
/* assumes that key is not already present! */
static void hnj_hash_insert(
  HashTab *hashtab,
  const unsigned char *key,
  int state
) {
  int i;
  HashEntry *e;

  i = hnj_string_hash (key) % HASH_SIZE;
  e = hnj_malloc (sizeof(HashEntry));
  e->next = hashtab->entries[i];
  e->key = hnj_strdup (key);
  e->state = state;
  hashtab->entries[i] = e;
}


//
/* return state if found, otherwise -1 */
static int hnj_hash_lookup(
  HashTab *hashtab,
  const unsigned char *key
) {
  int i;
  HashEntry *e;

  i = hnj_string_hash (key) % HASH_SIZE;
  for (e = hashtab->entries[i]; e; e = e->next) {
    if (!strcmp ((char*)key, (char*)e->key)) {
      return e->state;
    }
  }
  return -1;
}


//
/* Get the state number, allocating a new state if necessary. */
static int hnj_get_state(
  HyphenDict *dict,
  const unsigned char *string,
  int *state_num
) {
  *state_num = hnj_hash_lookup(&dict->hashtab, string);

  if (*state_num >= 0)
    return *state_num;

  hnj_hash_insert(&dict->hashtab, string, dict->num_states);
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

  if (uni_ch<32 || (uni_ch>126 && uni_ch<160)) {
    fprintf(stderr,"Character out of bounds: u%04x \n",uni_ch);
    exit(1);
  }
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

//
//
static unsigned char *get_state_str(
  int state
) {
  int i;
  HashEntry *e;

  for (i = 0; i < HASH_SIZE; i++)
    for (e = global->entries[i]; e; e = e->next)
      if (e->state == state)
	return e->key;
  return NULL;
}
#endif


/* I've changed the semantics a bit here: hnj_hyphen_load used to
   operate on a file, but now the argument is a string buffer.
 */

static const unsigned char* next_pattern(
  size_t* length,
  const unsigned char **buf
) {
  const unsigned char *rover = *buf;
  while (*rover && isspace(*rover)) rover++;
  const unsigned char *here = rover;
  while (*rover) {
    if (isspace(*rover)) {
      *length = rover-here;
      *buf = rover;
      return here;
    }
    rover++;
  }
  *length = rover-here;
  *buf = rover;
  return *length ? here : NULL; /* zero sensed */
}

//
//
static void init_dict(
  HyphenDict* dict
) {
  int i;
  dict->num_states = 1;
  dict->states = hnj_malloc (sizeof(HyphenState));
  dict->states[0].match = NULL;
  dict->states[0].fallback_state = -1;
  dict->states[0].num_trans = 0;
  dict->states[0].trans = NULL;
  for (i = 0; i < HASH_SIZE; i++)
    dict->hashtab.entries[i] = NULL;
  hnj_hash_insert(&dict->hashtab, (unsigned char*)"", 0);
}


//
//
static void clear_dict(
  HyphenDict* dict
) {
  int state_num;
  for (state_num = 0; state_num < dict->num_states; state_num++) {
    HyphenState *hstate = &dict->states[state_num];
    if (hstate->match) hnj_free (hstate->match);
    if (hstate->trans) hnj_free (hstate->trans);
  }
  hnj_free (dict->states);
  int i;
  HashEntry *e, *next;

  for (i = 0; i < HASH_SIZE; i++) {
    for (e = dict->hashtab.entries[i]; e; e = next) {
      next = e->next;
      hnj_free (e->key);
      hnj_free (e);
    }
  }
}


//
//
HyphenDict* hnj_hyphen_new() {
  HyphenDict* dict = hnj_malloc (sizeof(HyphenDict));
  init_dict(dict);
  return dict;
}


//
//
void hnj_hyphen_clear(
  HyphenDict* dict
) {
  clear_dict(dict);
  init_dict(dict);
}


//
//
void hnj_hyphen_free(
  HyphenDict *dict
) {
  clear_dict(dict);
  hnj_free(dict);
}


//
//
void hnj_hyphen_load(
  HyphenDict* dict,
  const unsigned char *f
) {
  int state_num, last_state;
  int i, j = 0;
  int ch;
  int found;
  HashEntry *e;
  int p;

  unsigned char *pattab_key[MAXPATHS];
           char *pattab_val[MAXPATHS];
  int   patterns = 0;
  unsigned char *newpattab_key[MAXPATHS];
           char *newpattab_val[MAXPATHS];
  int   newpatterns = 0;

  size_t l = 0;


//***************************************

  const unsigned char* format;
  while((format = next_pattern(&l,&f))!=NULL) {
    int i,j,e;
    //printf("%s\n",format);
    for (i=0,j=0,e=0; i<l; i++) {
      if (format[i]>='0'&&format[i]<='9') j++;
      if (format[i]>=0x80) e++;
    }
    // l-e   => number of _characters_ not _bytes_
    // l-e-j => number of pattern characters
    unsigned char *pat = (unsigned char*) malloc(1+l-j);
             char *org = (         char*) malloc(2+l-e-j);
    // remove hyphenation encoders (digits) from pat
    org[0] = '0';
    for (i=0,j=0,e=0; i<l; i++) {
      unsigned char c = format[i];
      if (c>=0x80) {
        pat[j+e++] = c;
      } else if (c<'0' || c>'9') {
        pat[e+j++] = c;
        org[j]   = '0';
      } else {
        org[j]   = c;
      }
    }
    pat[e+j]   = 0;
    org[j+1] = 0;
    pattab_key[patterns]   = pat;
    pattab_val[patterns++] = org;
    if (patterns>=MAXPATHS) die("too many base patterns");
  }
  // As we use binairy search, make sure it is sorted
  qsort_arr(pattab_key,pattab_val,0,patterns-1);

  for (p=0; p<patterns; p++) {
    unsigned char *pat = pattab_key[p];
    int   patsize = strlen((char*)pat);
    int   j,l;
    for (l=1; l<=patsize; l++) {
      if (pat[l]>=0x80) continue; // Do not clip an utf8 sequence
      for (j=1; j<=l; j++) {
        int i = l-j;
        int  subpat_ndx;
        if (pat[i]>=0x80) continue; // Do not start halfway an utf8 sequence
        if ((subpat_ndx = find_in(pattab_key,patterns,pat+i,j))>=0) {
          int   newpat_ndx;
          if ((newpat_ndx = find_in(newpattab_key,newpatterns,pat,l))<0) {
            unsigned char *newpat=(unsigned char*)malloc(l+1);
            strncpy((char*)newpat, (char*)pat,l); newpat[l]=0;
            int e=0;
            for (i=0; i<l; i++) if (newpat[i]>=0x80) e++;
            char *neworg = malloc(l+2-e);
            sprintf(neworg,"%0*d",l+1-e,0); // fill with right amount of '0'
            newpattab_key[newpatterns]   = newpat;
            newpattab_val[newpatterns++] = combine(neworg,pattab_val[subpat_ndx]);
            if (newpatterns>MAXPATHS) die("to many new patterns");
          } else {
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
    static unsigned char mask[] = {0x3F,0x1F,0xF,0x7};
    unsigned char *word    = newpattab_key[p];
             char *pattern = newpattab_val[p];
    int j = strlen((char*)word);
#ifdef VERBOSE
    printf ("word %s pattern %s, j = %d\n", word, pattern, j);
#endif
    state_num = hnj_get_state( dict, word, &found );
    dict->states[state_num].match = pattern;

    /* now, put in the prefix transitions */
    while (found < 0) {
      j--;
      last_state = state_num;
      ch = word[j];
      if (ch>=0x80) {
        int i=1;
        while (word[j-i]>=0x80 && word[j-i]<0xC0) i++;
        ch = word[j-i] & mask[i];
        int m = j-i;
        while (i--) {
          ch = (ch<<6)+(0x3F & word[j-i]);
        }
        j = m;
      }
      word[j] = '\0';
      state_num = hnj_get_state (dict, word, &found);
      hnj_add_trans (dict, state_num, last_state, ch);
    }
    free(newpattab_key[p]);
    // free(newpattab_val[p]); assigned to .match
  }

//***************************************

  /* put in the fallback states */
  for (i = 0; i < HASH_SIZE; i++) {
    for (e = dict->hashtab.entries[i]; e; e = e->next) {
      if (e->state) {
        for (j = 1; 1; j++) {
          state_num = hnj_hash_lookup(&dict->hashtab, e->key + j);
          if (state_num >= 0) break;
        }
        /* KBH: FIXME state 0 fallback_state should always be -1? */
        dict->states[e->state].fallback_state = state_num;
      }
    }
  }
#ifdef VERBOSE
  for (i = 0; i < HASH_SIZE; i++) {
    for (e = dict->hashtab->entries[i]; e; e = e->next) {
      printf ("%d string %s state %d, fallback=%d\n", i, e->key, e->state,
        dict->states[e->state].fallback_state);
      for (j = 0; j < dict->states[e->state].num_trans; j++) {
        printf (" u%4x->%d\n", (int)dict->states[e->state].trans[j].uni_ch,
          dict->states[e->state].trans[j].new_state);
      }
    }
  }
#endif
}


#define MAX_WORD 256


//
//
void hnj_hyphen_hyphenate(
  HyphenDict *dict,
  const int *word,
  int word_size,
  char *hyphens
) {
  int prep_word_buf[MAX_WORD];
  int *prep_word;
  int i, ext_word_len, k;
  int state;
  int ch;
  HyphenState *hstate;
  char *match;
  int offset;

  if (word_size + 3 < MAX_WORD)
    prep_word = prep_word_buf;
  else
    prep_word = hnj_malloc((word_size + 3)*sizeof(int));

  ext_word_len = 0;
  prep_word[ext_word_len++] = '.';
  for (i = 0; i < word_size; i++) prep_word[ext_word_len++] = word[i];
  prep_word[ext_word_len++] = '.';
  prep_word[ext_word_len] = 0;
  // NB: less-equal because there is always one more hyphenation point
  //     than number of characters:  ^.^w^o^r^d^.^
  for (i = 0; i <= ext_word_len; i++) hyphens[i] = '0';    

  /* now, run the finite state machine */
  state = 0;
  for (i = 0; i < ext_word_len; i++) {
    ch = prep_word[i];
    for (;;) {
      if (state == -1) {
        state = 0;
        goto try_next_letter;
      }          

#ifdef VERBOSE
      printf("%*s%s%c",i-strlen(get_state_str(state)),"",get_state_str(state),(char)ch);
#endif

      hstate = &dict->states[state];
      for (k = 0; k < hstate->num_trans; k++) {
        if (hstate->trans[k].uni_ch == ch) {
          state = hstate->trans[k].new_state;
#ifdef VERBOSE
          printf(" state %d\n",state);
#endif
          /* Additional optimization is possible here - especially,
             elimination of trailing zeroes from the match. Leading zeroes
             have already been optimized. */
          match = dict->states[state].match;
          if (match) {
            // +2 because:
            //  1 string length is one bigger than offset
            //  1 hyphenation starts before first character
            offset = i + 2 - strlen (match);
#ifdef VERBOSE
            printf ("%*s%s\n", offset,"", match);
#endif
            /* This is a linear search because I tried a binary search and
               found it to be just a teeny bit slower. */
            int m;
            for (m = 0; match[m]; m++) {
              if (hyphens[offset+m] < match[m]) hyphens[offset+m] = match[m];
            }
          }
          goto try_next_letter;
        }
      }
      state = hstate->fallback_state;
#ifdef VERBOSE
      printf (" back to %d\n", state);
#endif
    }
    /* KBH: we need this to make sure we keep looking in a word */
    /* for patterns even if the current character is not known in state 0 */
    /* since patterns for hyphenation may occur anywhere in the word */
try_next_letter: ;
  }
#ifdef VERBOSE
  putchar(' ');
  for (i = 0; i < ext_word_len; i++) {
    putchar(' ');
    putchar((char)prep_word[i]);
  }
  putchar ('\n');
  for (i = 0; i <= ext_word_len; i++) {
    putchar(' ');
    putchar(hyphens[i]);
  }
  putchar ('\n');
#endif

  // pattern is ^.^w^o^r^d^.^   word_len=4, ext_word_len=6, hyphens=7
  // convert to     ^ ^ ^       so drop first two and stop after word_len-1
  for (i = 0; i < word_size-1; i++) {
    if ((hyphens[i+2] & 1)==0)
      hyphens[i] = '0';
    else
      hyphens[i] = hyphens[i + 2];
  }
  /* Let TeX decide if first n, last m characters can have a hyphen */
//hyphens[0] = '0';
//for (; i < word_size; i++) hyphens[i] = '0';
//hyphens[word_size] = '\0';
  hyphens[word_size-1] = '\0';

  if (prep_word != prep_word_buf) hnj_free (prep_word);
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
