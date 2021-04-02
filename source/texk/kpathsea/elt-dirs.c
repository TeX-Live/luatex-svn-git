/* elt-dirs.c: Translate a path element to its corresponding director{y,ies}.

   Copyright 1993, 1994, 1995, 1996, 1997, 2008, 2009, 2010, 2011, 2016,
   2017, 2018 Karl Berry.
   Copyright 1997, 1998, 1999, 2000, 2005 Olaf Weber.

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

#include <kpathsea/config.h>

#include <kpathsea/c-pathch.h>
#include <kpathsea/expand.h>
#include <kpathsea/fn.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/xopendir.h>

/* To avoid giving prototypes for all the routines and then their real
   definitions, we give all the subroutines first.  The entry point is
   the last routine in the file.  */

/* Make a copy of DIR (unless it's null) and save it in L.  Ensure that
   DIR ends with a DIR_SEP for the benefit of later searches.  */

static void
dir_list_add (str_llist_type *l, string dir)
{
  char last_char = dir[strlen (dir) - 1];
  string saved_dir
    = IS_DIR_SEP_CH (last_char) || IS_DEVICE_SEP (last_char)
      ? xstrdup (dir)
      : concat (dir, DIR_SEP_STRING);

  str_llist_add (l, saved_dir);
}


/* If DIR is a directory, add it to the list L.  */

static void
checked_dir_list_add (kpathsea kpse, str_llist_type *l, string dir)
{
    if (kpathsea_dir_p (kpse, dir))
    dir_list_add (l, dir);
}

/* The cache.  Typically, several paths have the same element; for
   example, /usr/local/lib/texmf/fonts//.  We don't want to compute the
   expansion of such a thing more than once.  Even though we also cache
   the dir_links call, that's not enough -- without this path element
   caching as well, the execution time doubles.  */

/* Associate KEY with VALUE.  We implement the cache as a simple linear
   list, since it's unlikely to ever be more than a dozen or so elements
   long.  We don't bother to check here if PATH has already been saved;
   we always add it to our list.  We copy KEY but not VALUE; not sure
   that's right, but it seems to be all that's needed.  */

static void
cache (kpathsea kpse, const_string key,  str_llist_type *value)
{
  kpse->cache_length++;
  XRETALLOC (kpse->the_cache, kpse->cache_length, cache_entry);
  kpse->the_cache[kpse->cache_length - 1].key = xstrdup (key);
  kpse->the_cache[kpse->cache_length - 1].value = value;
}


/* To retrieve, just check the list in order.  */

static str_llist_type *
cached (kpathsea kpse, const_string key)
{
  unsigned p;

  for (p = 0; p < kpse->cache_length; p++)
    {
      if (FILESTRCASEEQ (kpse->the_cache[p].key, key))
        return kpse->the_cache[p].value;
    }

  return NULL;
}

/* Handle the magic path constructs.  */

/* Declare recursively called routine.  */
static void expand_elt (kpathsea, str_llist_type *, string, unsigned);


/* POST is a pointer into the original element (which may no longer be
   ELT) to just after the doubled DIR_SEP, perhaps to the null.  Append
   subdirectories of ELT (up to ELT_LENGTH, which must be a /) to
   STR_LIST_PTR.  ELT must not be the empty string (or NULL).  */

#ifdef WIN32
/* Shared across recursive calls, it acts like a stack. */
static char dirname[MAX_PATH*2];
static wchar_t dirnamew[MAX_PATH];
static char *potname;
#endif

static void
do_subdir (kpathsea kpse, str_llist_type *str_list_ptr, string elt,
              unsigned elt_length, string post)
{
#ifdef WIN32
  WIN32_FIND_DATAW find_file_data;
  HANDLE hnd;
  int proceed;
  int nlinks = 2;
#else
  DIR *dir;
  struct dirent *e;
#endif /* not WIN32 */
  fn_type name;

  /* Some old compilers don't allow aggregate initialization.  */
  name = fn_copy0 (elt, elt_length);

  assert (elt_length > 0
          && (IS_DIR_SEP_CH (elt[elt_length - 1])
              || IS_DEVICE_SEP (elt[elt_length - 1])));

#if defined (WIN32)
  strcpy(dirname, FN_STRING(name));
  strcat(dirname, "/*.*");         /* "*.*" or "*" -- seems equivalent. */
  get_wstring_from_mbstring(kpse->File_system_codepage, dirname, dirnamew);
  hnd = FindFirstFileW(dirnamew, &find_file_data);

  if (hnd == INVALID_HANDLE_VALUE) {
    fn_free(&name);
    return;
  }

  /* Include top level before subdirectories, if nothing to match.  */
  if (*post == 0)
    dir_list_add (str_list_ptr, FN_STRING (name));
  else {
    /* If we do have something to match, see if it exists.  For
       example, POST might be `pk/ljfour', and they might have a
       directory `$TEXMF/fonts/pk/ljfour' that we should find.  */
    fn_str_grow (&name, post);
    expand_elt (kpse, str_list_ptr, FN_STRING (name), elt_length);
    fn_shrink_to (&name, elt_length);
  }
  proceed = 1;
  while (proceed) {
    if (find_file_data.cFileName[0] != L'.') {
      int links;

      /* Construct the potential subdirectory name.  */
      potname = get_mbstring_from_wstring(kpse->File_system_codepage, find_file_data.cFileName, potname=NULL);
      fn_str_grow (&name, potname);
      free(potname);

      /* Maybe we have cached the leafness of this directory.
                 The function will return 0 if unknown,
                 else the actual (Unix-like) value. */
      links = kpathsea_dir_links (kpse, FN_STRING (name), 0);

      if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        unsigned potential_len = FN_LENGTH (name);
        /* in any case, compute the leafness */
        nlinks++;

        /* It's a directory, so append the separator.  */
        fn_str_grow (&name, DIR_SEP_STRING);
        if (*post != 0) {
          fn_str_grow (&name, post);
          /* Unfortunately we can't check if the new element is
             a leaf directory, because we don't have a directory
             name here, we just have a path spec. This means we
             may descend into a leaf directory cm/pk, if the
             spec is ...fonts//pk//.  */
          expand_elt (kpse, str_list_ptr, FN_STRING (name), potential_len);
          fn_shrink_to (&name, potential_len);
        }
        /* Should we recurse?  To see if the subdirectory is a
           leaf, check if it has two links (one for . and one for
           ..).  This means that symbolic links to directories do
           not affect the leaf-ness.  This is arguably wrong, but
           the only alternative I know of is to stat every entry
           in the directory, and that is unacceptably slow. */

        if (links == 0 || links > 2)
          /* All criteria are met; find subdirectories.  */
        do_subdir (kpse, str_list_ptr, FN_STRING (name),
                     potential_len, post);
        else if (*post == 0)
          /* Nothing to match, no recursive subdirectories to
             look for: we're done with this branch.  Add it.  */
          dir_list_add (str_list_ptr, FN_STRING (name));
      }
      fn_shrink_to (&name, elt_length);
    }
    proceed = FindNextFileW (hnd, &find_file_data);
  }
  /* Update the leafness of name. */
  kpathsea_dir_links(kpse, FN_STRING(name), nlinks);
  fn_free (&name);
  FindClose(hnd);

#else /* not WIN32 */

  /* If we can't open it, quit.  */
  dir = opendir (FN_STRING (name));
  if (dir == NULL)
    {
      fn_free (&name);
      return;
    }

  /* Include top level before subdirectories, if nothing to match.  */
  if (*post == 0)
    dir_list_add (str_list_ptr, FN_STRING (name));
  else
    { /* If we do have something to match, see if it exists.  For
         example, POST might be `pk/ljfour', and they might have a
         directory `$TEXMF/fonts/pk/ljfour' that we should find.  */
      fn_str_grow (&name, post);
      expand_elt (kpse, str_list_ptr, FN_STRING (name), elt_length);
      fn_shrink_to (&name, elt_length);
    }

  while ((e = readdir (dir)) != NULL)
    { /* If it begins with a `.', never mind.  (This allows ``hidden''
         directories that the algorithm won't find.)  */
      if (e->d_name[0] != '.')
        {
          int links;

          /* Construct the potential subdirectory name.  */
          fn_str_grow (&name, e->d_name);

          /* If we can't stat it, or if it isn't a directory, continue.  */
          links = kpathsea_dir_links (kpse, FN_STRING (name), 0);

          if (links >= 0)
            {
              unsigned potential_len = FN_LENGTH (name);

              /* It's a directory, so append the separator.  */
              fn_str_grow (&name, DIR_SEP_STRING);

              if (*post != 0)
                {
                  fn_str_grow (&name, post);
                  /* Unfortunately we can't check if the new element is
                     a leaf directory, because we don't have a directory
                     name here, we just have a path spec. This means we
                     may descend into a leaf directory cm/pk, if the
                     spec is ...fonts//pk//.  */
                  expand_elt (kpse, str_list_ptr, FN_STRING (name),
                              potential_len);
                  fn_shrink_to (&name, potential_len);
                }

              /* Should we recurse?  To see if the subdirectory is a
                 leaf, check if it has two links (one for . and one for
                 ..).  This means that symbolic links to directories do
                 not affect the leaf-ness.  This is arguably wrong, but
                 the only alternative I know of is to stat every entry
                 in the directory, and that is unacceptably slow.

                 The #ifdef here makes all this configurable at
                 compile-time, so that if we're using VMS directories or
                 some such, we can still find subdirectories, even if it
                 is much slower.  */
#ifdef ST_NLINK_TRICK
              /* With SAS/C++ 6.55 on the Amiga, stat sets the st_nlink
                 field to -1 for a file, or to 1 for a directory.
                 Cygwin 1.7 also leaves st_nlink as 1:
                 http://cygwin.com/ml/cygwin-developers/2008-04/msg00110.html
                 */
              if (links != 2)
#endif /* ST_NLINK_TRICK */
                /* All criteria are met; find subdirectories.  */
                  do_subdir (kpse, str_list_ptr, FN_STRING (name),
                           potential_len, post);
#ifdef ST_NLINK_TRICK
              else if (*post == 0)
                /* Nothing to match, no recursive subdirectories to
                   look for: we're done with this branch.  Add it.  */
                dir_list_add (str_list_ptr, FN_STRING (name));
#endif
            }

          /* Remove the directory entry we just checked from `name'.  */
          fn_shrink_to (&name, elt_length);
        }
    }

  fn_free (&name);
  xclosedir (dir);
#endif /* not WIN32 */
}


/* Assume ELT is non-empty and non-NULL.  Return list of corresponding
   directories (with no terminating NULL entry) in STR_LIST_PTR.  Start
   looking for magic constructs at START.  */

static void
expand_elt (kpathsea kpse, str_llist_type * str_list_ptr, string elt,
               unsigned start)
{
  string dir = elt + start, post;

  while (*dir != 0)
    {
      if (IS_DIR_SEP_CH (*dir))
        {
          /* If two or more consecutive /'s, find subdirectories.  */
          if (IS_DIR_SEP_CH (dir[1]))
            {
              for (post = dir + 1; IS_DIR_SEP_CH (*post); post++) ;
            do_subdir (kpse, str_list_ptr, elt, dir - elt + 1, post);
              return;
            }

          /* No special stuff at this slash.  Keep going.  */
        }

      dir++;
    }

  /* When we reach the end of ELT, it will be a normal filename.  */
  checked_dir_list_add (kpse, str_list_ptr, elt);
}

/*  On win32 we slashify ELT, i.e., change '\\' to '/', and then can use
   IS_DIR_SEP_CH instead of IS_DIR_SEP and need not test for the presence
   of 2-Byte Kanji (CP 932, SJIS) codes.  */

/* The first bits of a path element can be problematic because they
   look like a request to expand a whole disk, rather than a subtree.
   - It can contain a drive specification.
   - It can be a UNC path (w32, but they are part of the single
     Unix specification as well).
   The argument is a string as the function can diddle into the argument
   to canonicalize it, which tends to matter on windows platforms.
   - Always lower-case drive letters a-z, even those filesystem that
     preserve case in filenames do not care about the case of the drive
     letters.
   - Remove unneeded double slashes. The problem is Windows does not
     handle well filenames like c://dir/foo. So canonicalize the names.
     The resulting name will always be shorter than the one passed, so no
     problem.
   - Remove multiple leading slashes to prevent expanding from the root
     of a UNIX filesystem tree.  */

unsigned
kpathsea_normalize_path (kpathsea kpse, string elt)
{
  unsigned ret;
  unsigned i;

#if defined(WIN32)
  for (i = 0; elt[i]; i++) {
    if (elt[i] == '\\')
      elt[i] = '/';
    else if (kpathsea_IS_KANJI(kpse, elt + i))
      i++;
  }
#endif

  if (NAME_BEGINS_WITH_DEVICE(elt)) {
    if (*elt >= 'A' && *elt <= 'Z')
      *elt += 'a' - 'A';
    ret = 2;

  } else if (IS_UNC_NAME(elt)) {
    for (ret = 2; elt[ret] && !IS_DIR_SEP_CH(elt[ret]); ret++)
      ;

  } else
    ret = 0;

  for (i = ret; IS_DIR_SEP_CH(elt[i]); ++i)
    ;
  if (i > ret + 1) {
#ifdef KPSE_DEBUG
  if (KPATHSEA_DEBUG_P (KPSE_DEBUG_STAT))
    DEBUGF2 ("kpse_normalize_path (%s) => %u\n", elt, ret);
#endif /* KPSE_DEBUG */

    memmove (elt + ret + 1, elt + i, strlen (elt + i) + 1);
  }

  return ret;
}

/* Here is the entry point.  Returns directory list for ELT.  */

str_llist_type *
kpathsea_element_dirs (kpathsea kpse, string elt)
{
  str_llist_type *ret;
  unsigned i;

#ifdef _WIN32
  char *tname = NULL;
  wchar_t *wtname = NULL;
#endif /* _WIN32 */

  /* If given nothing, return nothing.  */
  if (!elt || !*elt)
    return NULL;

#ifdef _WIN32
/*
  Change encoding of a variable into kpse->File_system_codepage
  to support non-ascii values for the variable.
*/
  if (kpse->File_system_codepage != kpse->Win32_codepage) {
    wtname = get_wstring_from_mbstring (kpse->Win32_codepage,
                                        elt, wtname = NULL);
    tname = get_mbstring_from_wstring (kpse->File_system_codepage,
                                       wtname, tname = NULL);
    elt = tname;
    free(wtname);
  }
#endif /* _WIN32 */

  /* Normalize ELT before looking for a cached value.  */
  i = kpathsea_normalize_path (kpse, elt);

  /* If we've already cached the answer for ELT, return it.  */
  ret = cached (kpse, elt);
#ifdef _WIN32
  if (ret) {
    if (tname) free (tname);
    return ret;
  }
#else
  if (ret)
    return ret;
#endif /* _WIN32 */

  /* We're going to have a real directory list to return.  */
  ret = XTALLOC1 (str_llist_type);
  *ret = NULL;

  /* We handle the hard case in a subroutine.  */
  expand_elt (kpse, ret, elt, i);

  /* Remember the directory list we just found, in case future calls are
     made with the same ELT.  */
  cache (kpse, elt, ret);

#ifdef KPSE_DEBUG
  if (KPATHSEA_DEBUG_P (KPSE_DEBUG_EXPAND))
    {
      DEBUGF1 ("path element %s =>", elt);
      if (ret)
        {
          str_llist_elt_type *e;
          for (e = *ret; e; e = STR_LLIST_NEXT (*e))
            fprintf (stderr, " %s", STR_LLIST (*e));
        }
      putc ('\n', stderr);
      fflush (stderr);
    }
#endif /* KPSE_DEBUG */

#ifdef _WIN32
  if (tname) free (tname);
#endif /* _WIN32 */

  return ret;
}

#ifdef TEST

void
print_element_dirs (const_string elt)
{
  str_llist_type *dirs;

  printf ("Directories of %s:\t", elt ? elt : "(nil)");
  fflush (stdout);

  dirs = kpathsea_element_dirs (kpse_def, elt);

  if (!dirs)
    printf ("(nil)");
  else
    {
      str_llist_elt_type *dir;
      for (dir = *dirs; dir; dir = STR_LLIST_NEXT (*dir))
        {
          string d = STR_LLIST (*dir);
          printf ("%s ", *d ? d : "`'");
        }
    }

  putchar ('\n');
}

int
main ()
{
  /* DEBUG_SET (DEBUG_STAT); */
  /* All lists end with NULL.  */
  print_element_dirs (NULL);    /* */
  print_element_dirs ("");      /* ./ */
  print_element_dirs ("/k");    /* */
  print_element_dirs (".//");   /* ./ ./archive/ */
  print_element_dirs (".//archive");    /* ./ ./archive/ */
#ifdef AMIGA
  print_element_dirs ("TeXMF:AmiWeb2c/texmf/fonts//"); /* lots */
  print_element_dirs ("TeXMF:AmiWeb2c/share/texmf/fonts//bakoma"); /*just one*/
  print_element_dirs ("TeXMF:AmiWeb2c/texmf/fonts//"); /* lots again [cache] */
  print_element_dirs ("TeXMF:");        /* TeXMF: */
  print_element_dirs ("TeXMF:/");       /* TeXMF: and all subdirs */
#else /* not AMIGA */
  print_element_dirs ("/tmp/fonts//");  /* no need to stat anything */
  print_element_dirs ("/usr/local/lib/tex/fonts//");      /* lots */
  print_element_dirs ("/usr/local/lib/tex/fonts//times"); /* just one */
  print_element_dirs ("/usr/local/lib/tex/fonts//"); /* lots again [cache] */
  print_element_dirs ("~karl");         /* tilde expansion */
  print_element_dirs ("$karl");         /* variable expansion */
  print_element_dirs ("~${LOGNAME}");   /* both */
#endif /* not AMIGA */
  return 0;
}

#endif /* TEST */


/*
Local variables:
standalone-compile-command: "gcc -g -I. -I.. -DTEST elt-dirs.c kpathsea.a"
End:
*/
