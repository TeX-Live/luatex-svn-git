/* libc replacement functions for win32.

Copyright (C) 1998, 99 Free Software Foundation, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/*
  This does make sense only under WIN32.
  Functions:
    - look_for_cmd() : locates an executable file
    - parse_cmd_line() : splits a command with pipes and redirections
    - build_cmd_line() : builds a command with pipes and redirections (useful ?)
  */

/*
  This part looks for the real location of the program invoked
  by cmd. If it can find the program, that's good. Else 
  command processor is invoked.
*/

#ifdef __MINGW32__

#include <kpathsea/config.h>
#include <kpathsea/c-proto.h>
#include <kpathsea/win32lib.h>
#include <kpathsea/lib.h>
#include <errno.h>

BOOL 
look_for_cmd(const char *cmd, char **app, char **new)
{
  char *env_path;
  char *p, *q;
  char pname[MAXPATHLEN], *fp;
  char *suffixes[] = { ".bat", ".cmd", ".com", ".exe", NULL };
  char **s;
  char *app_name, *new_cmd;

  BOOL go_on;

  *new = *app = NULL;
  new_cmd = app_name = NULL;

  /* We should look for the application name along the PATH,
     and decide to prepend "%COMSPEC% /c " or not to the command line.
     Do nothing for the moment. */

  /* Another way to do that would be to try CreateProcess first without
     invoking cmd, and look at the error code. If it fails because of
     command not found, try to prepend "cmd /c" to the cmd line.
  */

  /* Look for the application name */
  for (p = (char *)cmd; *p && isspace(*p); p++);
  if (*p == '"') {
    q = ++p;
    while(*p && *p != '"') p++;
    if (*p == '\0') {
      fprintf(stderr, "Look_for_cmd: malformed command (\" not terminated)\n");
      return FALSE;
    }
  }
  else
    for (q = p; *p && !isspace(*p); p++);
  /* q points to the beginning of appname, p to the last + 1 char */
  if ((app_name = malloc(p - q + 1)) == NULL) {
    fprintf(stderr, "Look_for_cmd: malloc(app_name) failed.\n");
    return FALSE;
  }
  strncpy(app_name, q, p - q );
  app_name[p - q] = '\0';
  pname[0] = '\0';
#ifdef TRACE
  fprintf(stderr, "popen: app_name = %s\n", app_name);
#endif

  {
    char *tmp = getenv("PATH");
    env_path = xmalloc(strlen(tmp) + 3);
    strcpy(env_path, ".;");
    strcat(env_path, tmp);
  }

  /* Looking for appname on the path */
  for (s = suffixes, go_on = TRUE; go_on; *s++) {
    if (SearchPath(env_path,	/* Address of search path */
		   app_name,	/* Address of filename */
		   *s,		/* Address of extension */
		   MAXPATHLEN,	/* Size of destination buffer */
		   pname,	/* Address of destination buffer */
		   &fp)		/* File part of app_name */
	!= 0) {
#ifdef TRACE
      fprintf(stderr, "%s found with suffix %s\nin %s\n", app_name, *s, pname);
#endif
      new_cmd = xstrdup(cmd);
      free(app_name);
      app_name = xstrdup(pname);
      break;
    }
    go_on = (*s != NULL);
  }
  if (go_on == FALSE) {
    /* the app_name was not found */
#ifdef TRACE
    fprintf(stderr, "%s not found, concatenating comspec\n", app_name);
#endif
    new_cmd = concatn(getenv("COMSPEC"), " /c ", cmd, NULL);
    free(app_name);
    app_name = NULL;
  }
  else {
  }
  if (env_path) free(env_path);

  *new = new_cmd;
  *app = app_name;

  return TRUE;

}

/*
  Command parser. Borrowed from DJGPP.
 */

static BOOL __system_allow_multiple_cmds = FALSE;

typedef enum {
  EMPTY,
  WORDARG,
  REDIR_INPUT,
  REDIR_APPEND,
  REDIR_OUTPUT,
  PIPE,
  SEMICOLON,
  UNMATCHED_QUOTE,
  EOL
} cmd_sym_t;

/* Return a copy of a word between BEG and (excluding) END with all
   quoting characters removed from it.  */

static char *
__unquote (char *to, const char *beg, const char *end)
{
  const char *s = beg;
  char *d = to;
  int quote = 0;

  while (s < end)
    {
      switch (*s)
	{
	case '"':
	case '\'':
	  if (!quote)
	    quote = *s;
	  else if (quote == *s)
	    quote = 0;
	  s++;
	  break;
	case '\\':
	  if (s[1] == '"' || s[1] == '\''
	      || (s[1] == ';'
		  && (__system_allow_multiple_cmds)))
	    s++;
	  /* Fall-through.  */
	default:
	  *d++ = *s++;
	  break;
	}
    }

  *d = 0;
  return to;
}

/* A poor-man's lexical analyzer for simplified command processing.

   It only knows about these:

     redirection and pipe symbols
     semi-colon `;' (that possibly ends a command)
     argument quoting rules with quotes and `\'
     whitespace delimiters of words (except in quoted args)

   Returns the type of next symbol and pointers to its first and (one
   after) the last characters.

   Only `get_sym' and `unquote' should know about quoting rules.  */

static cmd_sym_t
get_sym (char *s, char **beg, char **end)
{
  int in_a_word = 0;

  while (isspace (*s))
    s++;

  *beg = s;
  
  do {
    *end = s + 1;

    if (in_a_word
	&& (!*s || strchr ("<>| \t\n", *s)
	    || ((__system_allow_multiple_cmds) && *s == ';')))
      {
	--*end;
	return WORDARG;
      }

    switch (*s)
      {
      case '<':
	return REDIR_INPUT;
      case '>':
	if (**end == '>')
	  {
	    ++*end;
	    return REDIR_APPEND;
	  }
	return REDIR_OUTPUT;
      case '|':
	return PIPE;
      case ';':
	if (__system_allow_multiple_cmds)
	  return SEMICOLON;
	else
	  in_a_word = 1;
	break;
      case '\0':
	--*end;
	return EOL;
      case '\\':
	if (s[1] == '"' || s[1] == '\''
	    || (s[1] == ';' && (__system_allow_multiple_cmds)))
	  s++;
	in_a_word = 1;
	break;
      case '\'':
      case '"':
	{
	  char quote = *s++;

	  while (*s && *s != quote)
	    {
	      if (*s++ == '\\' && (*s == '"' || *s == '\''))
		s++;
	    }
	  *end = s;
	  if (!*s)
	    return UNMATCHED_QUOTE;
	  in_a_word = 1;
	  break;
	}
      default:
	in_a_word = 1;
	break;
      }

    s++;

  } while (1);
}

/*
  What we allow :
  [cmd] [arg1] ... [argn] < [redinput] | [cmd2] | ... | [cmdn] > [redoutput]
*/
void *parse_cmdline(char *line, char **input, char **output)
{
  BOOL again, needcmd = TRUE, bSuccess = TRUE, append_out = FALSE;
  char *beg = line, *end, *new_end;
  cmd_sym_t token = EMPTY, prev_token = EMPTY;
  int ncmd = 0, narg = 1;
  char **fp;
  char ***cmd;
  char *dummy_input;			/* So that we could pass NULL */
  char *dummy_output;			/* instead of a real ??put */

  if (input == NULL) input = &dummy_input;
  if (output == NULL) output = &dummy_output;

  *input = NULL;
  *output = NULL;
  cmd = xmalloc(MAX_PIPES*sizeof(char **));
  cmd[ncmd] = NULL;
#ifdef TRACE
  fprintf(stderr, "line = %s\n", line);
#endif
  do {
    again = FALSE;
    prev_token = token;
    token = get_sym (beg, &beg, &end);	/* get next symbol */
#ifdef TRACE
    fprintf(stderr, "token = %s\n", beg);
#endif	
    switch (token) {
    case WORDARG:
      if (prev_token == REDIR_INPUT
	  || prev_token == REDIR_OUTPUT) {
	fprintf(stderr, "Ambigous input/output redirect.");
	bSuccess = FALSE;
	goto leave;
      }
      /* First word we see is the program to run.  */
      if (needcmd) {
	narg = 1;
	cmd[ncmd] = xmalloc(narg * sizeof(char *));
	cmd[ncmd][narg - 1] = xmalloc(end - beg + 1);
	__unquote (cmd[ncmd][narg - 1], beg, end); /* unquote and copy to prog */
	if (cmd[ncmd][narg - 1][0] == '(') {
	  fprintf(stderr, "parse_cmdline(%s): Parenthesized groups not allowed.\n", line);
	  bSuccess = FALSE;
	  goto leave;
	}
	needcmd = FALSE;
      }
      else {
	narg++;
	cmd[ncmd] = xrealloc(cmd[ncmd], narg * sizeof(char *));
	cmd[ncmd][narg - 1] = xmalloc(end - beg + 1);
	__unquote (cmd[ncmd][narg - 1], beg, end); /* unquote and copy to prog */
      }
      beg = end; /* go forward */
      again = TRUE;
      break;

    case REDIR_INPUT:
    case REDIR_OUTPUT:
    case REDIR_APPEND:
      if (token == REDIR_INPUT) {
	if (*input) {
	  fprintf(stderr, "Ambiguous input redirect.");
	  errno = EINVAL;
	  bSuccess = FALSE;
	  goto leave;
	}
	fp = input;
      }
      else if (token == REDIR_OUTPUT || token == REDIR_APPEND) {
	if (*output) {
	  fprintf(stderr, "Ambiguous output redirect.");
	  errno = EINVAL;
	  bSuccess = FALSE;
	  goto leave;
	}
	fp = output;
	if (token == REDIR_APPEND)
	  append_out = TRUE;
      }
      if (get_sym (end, &end, &new_end) != WORDARG) {
	fprintf(stderr, "Target of redirect is not a filename.");
	errno = EINVAL;
	bSuccess = FALSE;
	goto leave;
      }
      *fp = (char *)xmalloc (new_end - end + 1);
      memcpy (*fp, end, new_end - end);
      (*fp)[new_end - end] = '\0';
      beg = new_end;
      again = TRUE;
      break;
    case PIPE:
      if (*output) {
	fprintf(stderr, "Ambiguous output redirect.");
	errno = EINVAL;
	bSuccess = FALSE;
	goto leave;
      }
      narg++;
      cmd[ncmd] = xrealloc(cmd[ncmd], narg * sizeof(char *));
      cmd[ncmd][narg - 1] = NULL;
      ncmd++; 
      needcmd = TRUE;
      beg = end;
      again = TRUE;
      break;
    case SEMICOLON:
    case EOL:
      if (needcmd) {
	fprintf(stderr, "No command name seen.");
	errno = EINVAL;
	bSuccess = FALSE;
	goto leave;
      }
      narg++;
      cmd[ncmd] = xrealloc(cmd[ncmd], narg * sizeof(char *));
      cmd[ncmd][narg - 1] = NULL;
      ncmd++;
      cmd[ncmd] = NULL;
      again = FALSE;
      break;
	  
    case UNMATCHED_QUOTE:
      fprintf(stderr, "Unmatched quote character.");
      errno = EINVAL;
      bSuccess = FALSE;
      goto leave;
    default:
      fprintf(stderr, "I cannot grok this.");
      errno = EINVAL;
      bSuccess = FALSE;
      goto leave;

    }

  }	while (again);

 leave:
  if (!bSuccess) {
    int i;
    char **p;
    /* Need to free everything that was allocated */
    for (i = 0; i < ncmd; i++) {
      for (p = cmd[i]; *p; p++) 
	free(*p);
      free(cmd[i]);
    }
    if (cmd[ncmd]) {
      for (i = 0; i < narg; i++)
	free(cmd[ncmd][i]);
      free(cmd[ncmd]);
    }
    free(cmd);
    *cmd = NULL;
  }
  return cmd;
}

static char *
quote_elt(char *elt) 
{
  char *p;
  for (p = elt; *p; p++)
    if (isspace(*p))
      return concat3("\"", elt, "\"");

  return xstrdup(elt);
}

/* static (commented out for mingw; SK) */ char *
quote_args(char **argv)
{
  int i;
  char *line = NULL, *new_line;
  char *new_argv;

  if (!argv)
    return line;

  line = quote_elt(argv[0]);
  for (i = 1; argv[i]; i++) {
    new_argv = quote_elt(argv[i]);
    new_line = concat3(line, " ", new_argv);
    free(line);
    free(new_argv);
    line = new_line;
  }

  return line;
}

char *
build_cmdline(char ***cmd, char *input, char *output)
{
  int ncmd;
  char *line = NULL, *new_line;

  if (!cmd)
    return line;

  line = quote_args(cmd[0]);
  if (input) {
    new_line = concat3(line, " < ", quote_elt(input));
    free(line);
    line = new_line;
  }
  for(ncmd = 1; cmd[ncmd]; ncmd++) {
    new_line = concat3(line, " | ", quote_args(cmd[ncmd]));
    free(line);
    line = new_line;
  }

  if (output) {
    new_line = concat3(line, " > ", quote_elt(output));
    free(line);
    line = new_line;
  }

  return line;
}

#ifdef TEST
void 
display_cmd(char ***cmd, char *input, char *output, char *errput)
{
  int ncmd = 0, narg;
  
  if (cmd) {
    for (ncmd = 0; cmd[ncmd]; ncmd++) {
      printf("cmd[%d] = %s ", ncmd, cmd[ncmd][0]);
      for(narg = 1; cmd[ncmd][narg] != NULL; narg++)
	printf("%s ", cmd[ncmd][narg]);
      printf("\n");
    }
  }
  printf("input = %s\n", (input ? input : "<stdin>"));
  printf("output = %s\n", (output ? output : "<stdout>"));
  printf("errput = %s\n", (errput ? errput : "<stderr>"));
}

main(int argc, char *argv[])
{
  char *input;
  char *output;
  char ***cmd;
  char *line;

  printf("%s\n", line = "foo a b");
  if (cmd = parse_cmdline(line, &input, &output)) {
    display_cmd(cmd, input, output, NULL);
  }
  printf("%s\n", line = "foo < a > b");
  if (cmd = parse_cmdline(line, &input, &output)) {
    display_cmd(cmd, input, output, NULL);
  }
  printf("%s\n", line = "foo > a < b");
  if (cmd = parse_cmdline(line, &input, &output)) {
    display_cmd(cmd, input, output, NULL);
  }
  printf("%s\n", line = "foo | a b");
  if (cmd = parse_cmdline(line, &input, &output)) {
    display_cmd(cmd, input, output, NULL);
  }
  printf("%s\n", line = "foo a | b");
  if (cmd = parse_cmdline(line, &input, &output)) {
    display_cmd(cmd, input, output, NULL);
  }
  printf("%s\n", line = "foo | a > b");
  if (cmd = parse_cmdline(line, &input, &output)) {
    display_cmd(cmd, input, output, NULL);
  }
  printf("%s\n", line = "foo < a | b");
  if (cmd = parse_cmdline(line, &input, &output)) {
    display_cmd(cmd, input, output, NULL);
  }
  printf("%s\n", line = "foo < a | b");
  if (cmd = parse_cmdline(line, &input, &output)) {
    display_cmd(cmd, input, output, NULL);
  }
  printf("%s\n", line = "foo < a | b | c > d");
  if (cmd = parse_cmdline(line, &input, &output)) {
    display_cmd(cmd, input, output, NULL);
  }
  printf("%s\n", line = "foo > a | b < c");
  if (cmd = parse_cmdline(line, &input, &output)) {
    display_cmd(cmd, input, output, NULL);
  }

  printf("%s\n", line = "\"C:\\Program Files\\WinEdt\\WinEdt.exe\" -F \"[Open('%f');SelLine(%l,8)]\"");
  if (cmd = parse_cmdline(line, &input, &output)) {
    display_cmd(cmd, input, output, NULL);
  }

}
#endif

#endif
