/* types.h: general types for kpathsea.

   Copyright 1993, 1995, 1996, 2005, 2008 Karl Berry.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library; if not, see <http://www.gnu.org/licenses/>.  */

#ifndef KPATHSEA_TYPES_H
#define KPATHSEA_TYPES_H

/* Booleans.  */
/* NeXT wants to define their own boolean type.  */
#ifndef HAVE_BOOLEAN
#define HAVE_BOOLEAN
typedef int boolean;
/* `true' and `false' are reserved words in C++.  */
#ifndef __cplusplus
#ifndef true
#define true 1
#define false 0
#endif /* not true */
#endif /* not __cplusplus */
#endif /* not HAVE_BOOLEAN */

/* The X library (among other things) defines `FALSE' and `TRUE', and so
   we only want to define them if necessary, for use by application code.  */
#ifndef FALSE
#define FALSE false
#define TRUE true
#endif /* FALSE */

/* The usual null-terminated string.  */
typedef char *string;

/* A pointer to constant data.  (ANSI says `const string' is
   `char * const', which is a constant pointer to non-constant data.)  */
typedef const char *const_string;

/* A generic pointer.  */
typedef void *address;

/* function pointer prototype definitions for recorder */
typedef void (*p_record_input) (const_string);
typedef void (*p_record_output) (const_string);

/* the cache structure from elt-dirs.c */
#include <kpathsea/str-llist.h>

typedef struct
{
  const_string key;
  str_llist_type *value;
} cache_entry;

/* from variable.c  */
typedef struct {
  const_string var;
  boolean expanding;
} expansion_type;


#include <kpathsea/hash.h>
#include <kpathsea/str-list.h>

#include <kpathsea/tex-file.h>

typedef struct kpathsea_instance *kpathsea;

/* todo: what about getopt.c */

typedef struct kpathsea_instance {
  /* from cnf.c */
  p_record_input kpse_record_input;   /* for --recorder */
  p_record_output kpse_record_output; /* for --recorder */
  hash_table_type cnf_hash;           /* used by read_all_cnf */
  boolean doing_cnf_init;             /* for kpse_cnf_get */
  /* from db.c */
  hash_table_type db;                 /* The hash table for all the ls-R's.  */
  hash_table_type alias_db;           /* The hash table for the aliases */
  str_list_type db_dir_list;          /* list of ls-R's */
  /* from debug.c */
  unsigned kpathsea_debug;            /* for --kpathsea-debug */
  /* from dir.c */
  hash_table_type link_table;         /* a hash of links-per-dir */
  /* from elt-dir.c */
  cache_entry *the_cache;
  unsigned cache_length;
  /* from fontmap.c */
  hash_table_type map;                /* the font mapping hash */
  const_string map_path;              /* holds the format path for kpse_fontmap_format */
  /* from hash.c */
  boolean kpse_debug_hash_lookup_int; /* for debugging */
  /* from mingw32.c (conditional) */ 
  char *cached_home_directory;        /* all three are just cache data */ 
  struct win32_volumes *volumes;      /* partial pointer to win32-specific data structure */
  /* from path-elt.c */
  string elt;                         /* statically created buffer for return value */
  unsigned elt_alloc;
  const_string path;                  /* The path we're currently working on.  */
  /* from pathsearch.c */
  boolean first_search;               /* should be initialized to true */
  FILE *log_file;
  boolean first_time;                 /* Need to open the log file? should also be true */
  /* from progname.c */
  string program_invocation_name;     /* called name (conditional) */
  string program_invocation_short_name; /* short called name (conditional) */
  string kpse_program_name;           /* pretended name */
  int ll_verbose;                     /* for symlinks (conditional) */
  int ll_loop;                        /* for symlinks (conditional) */
  /* from tex-file.c */
  const_string kpse_fallback_font;
  const_string kpse_fallback_resolutions_string;
  unsigned *kpse_fallback_resolutions;
  kpse_format_info_type kpse_format_info[kpse_last_format];
  /* from tex-make.c */
  boolean kpse_make_tex_discard_errors;
  FILE *missfont;
  /* from variable.c  */
  expansion_type *expansions; /* The sole variable of this type.  */
  unsigned expansion_len ;
  /* from xputenv.c */
  char **saved_env;       /* these keep track of changed items */
  int saved_count;     
} kpathsea_instance;


#endif /* not KPATHSEA_TYPES_H */
 
