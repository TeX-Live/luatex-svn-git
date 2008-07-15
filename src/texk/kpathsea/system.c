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
    - win32_system() rewritten
  */

#ifdef __MINGW32__

#include <kpathsea/config.h>
#include <kpathsea/c-proto.h>
#include <kpathsea/win32lib.h>
#include <kpathsea/lib.h>
#include <errno.h>

/* from lookcmd.c */
extern void *parse_cmdline(char *line, char **input, char **output);
extern char *build_cmdline(char ***cmd, char *input, char *output);

/*
  It has been proven that system() fails to retrieve exit codes
  under Win9x. This is a workaround for this bug.
*/

int __cdecl win32_system(const char *cmd, int async)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  DWORD ret = 0;
  HANDLE hIn, hOut, hPipeIn, hPipeOut;
  SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
  int i;

  char  *app_name, *new_cmd;
  char  *red_input, *red_output, ***cmd_pipe;

  /* Reset errno ??? */
  errno = 0;

  /* Admittedly, the command interpreter will allways be found. */
  if (! cmd) {
    errno = 0;
#ifdef _TRACE
    fprintf(stderr, "system: (null) command.\n");
#endif
    return 1;
  }

  if (look_for_cmd(cmd, &app_name, &new_cmd) == FALSE) {
    /* Failed to find the command or malformed cmd */
    errno = ENOEXEC;
#ifdef _TRACE
    fprintf(stderr, "system: failed to find command.\n");
#endif
    return -1;
  }

  cmd_pipe = parse_cmdline(new_cmd, &red_input, &red_output);

  for (i = 0; cmd_pipe[i]; i++) {

    /* free the cmd and build the current one */
    if (new_cmd) free(new_cmd);

    new_cmd = build_cmdline(&cmd_pipe[i], NULL, NULL);

    /* First time, use red_input if available */
    if (i == 0) {
      if (red_input) {
	hIn = CreateFile(red_input,
			 GENERIC_READ,
			 FILE_SHARE_READ | FILE_SHARE_WRITE,
			 &sa,
			 OPEN_EXISTING,
			 FILE_ATTRIBUTE_NORMAL,
			 NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
#ifdef _TRACE
	  fprintf(stderr, "system: failed to open hIn (%s) with error %d.\n", red_input, GetLastError());
#endif
	  errno = EIO;
	  return -1;
	}
      }
      else {
	hIn = GetStdHandle(STD_INPUT_HANDLE);
      }
    }
    /* Last time, use red_output if available */
    if (cmd_pipe[i+1] == NULL) {
      if (red_output) {
	hOut = CreateFile(red_output,
			  GENERIC_WRITE,
			  FILE_SHARE_READ | FILE_SHARE_WRITE,
			  &sa,
			  OPEN_ALWAYS,
			  FILE_ATTRIBUTE_NORMAL,
			  NULL);
	if (hOut == INVALID_HANDLE_VALUE) {
#ifdef _TRACE
	  fprintf(stderr, "system: failed to open hOut (%s) with error %d.\n", red_output, GetLastError());
#endif
	  errno = EIO;
	  return -1;
	}
      }
      else {
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
      }
    }

#if 0
    /* FIXME : implement pipes !!! */
#endif

    ZeroMemory( &si, sizeof(STARTUPINFO) );
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;
    si.hStdInput = hIn;
    si.hStdOutput = hOut;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

#ifdef _TRACE
    fprintf(stderr, "Executing: %s\n", new_cmd);
#endif
    if (CreateProcess(app_name,
		      new_cmd,
		      NULL,
		      NULL,
		      TRUE,
		      0,
		      NULL,
		      NULL,
		      &si,
		      &pi) == 0) {
      fprintf(stderr, "win32_system(%s) call failed (Error %d).\n", cmd, GetLastError());
      return -1;
    }
    
    /* Only the process handle is needed */
    CloseHandle(pi.hThread);
    
    if (async == 0) {
      if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_OBJECT_0) {
	if (GetExitCodeProcess(pi.hProcess, &ret) == 0) {
	  fprintf(stderr, "Failed to retrieve exit code: %s (Error %d)\n", cmd, GetLastError());
	  ret = -1;
	}
      }
      else {
	fprintf(stderr, "Failed to wait for process termination: %s (Error %d)\n", cmd, GetLastError());
	ret = -1;
      }
    }

    CloseHandle(pi.hProcess);

    if (red_input) CloseHandle(hIn);
    if (red_output) CloseHandle(hOut);
  }

  if (new_cmd) free(new_cmd);
  if (app_name) free(app_name);
    
  return ret;
}

#endif
