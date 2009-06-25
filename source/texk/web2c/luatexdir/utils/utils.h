/* utils.h

   Copyright 1996-2006 Han The Thanh <thanh@pdftex.org>
   Copyright 2006-2009 Taco Hoekwater <taco@luatex.org>

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

/* $Id$ */

#ifndef UTILS_H
#  define UTILS_H

#  define overflow_string(a,b) { overflow(maketexstring(a),b); flush_str(last_tex_string); }

extern char *job_id_string;
extern integer epochseconds;
extern integer microseconds;

void make_subset_tag(fd_entry *);

str_number maketexstring(const char *);

str_number maketexlstring(const char *, size_t);
void append_string(char *s);
__attribute__ ((format(printf, 1, 2)))
void tex_printf(const char *, ...);

__attribute__ ((noreturn, format(printf, 1, 2)))
void pdftex_fail(const char *, ...);
__attribute__ ((format(printf, 1, 2)))
void pdftex_warn(const char *, ...);
void garbage_warning(void);
void tex_error(char *msg, char **hlp);
char *makecstring(integer);
char *makeclstring(integer, size_t *);
void set_job_id(int, int, int, int);
void make_pdftex_banner(void);
char *get_resname_prefix(void);
size_t xfwrite(void *, size_t size, size_t nmemb, FILE *);
int xfflush(FILE *);
int xgetc(FILE *);
int xputc(int, FILE *);
scaled ext_xn_over_d(scaled, scaled, scaled);
void escapehex(poolpointer in);
void unescapehex(poolpointer in);
char *makecfilename(str_number s);
char *stripzeros(char *);
void initversionstring(char **versions);
extern void check_buffer_overflow(int wsize);
extern void check_pool_overflow(int wsize);

extern str_number last_tex_string;
extern char *cur_file_name;
extern size_t last_ptr_index;

void tconfusion(char *s);

#endif                          /* UTILS_H */
