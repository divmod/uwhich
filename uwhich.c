/* -*-C-*-
*******************************************************************************
*
* File:         bash.h
* RCS:          $Id: uwhich.c,v 1.13 2010/09/27 17:18:10 mas939 Exp $
* Description:  a micro-which project for EECS 343 for
*           Northwestern University.
*
* based on: which v2.x -- print full path of executables
* Copyright (C) 1999, 2003, 2007, 2008  Carlo Wood <carlo@gnu.org>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* Author: Fabian E. Bustamante AquaLab Research Group Department of
* Electrical Engineering and Computer Science Northwestern University
* Created: Thu Sep 16, 2010 at 10:42:17 Modified: Language: C Package:
* N/A Status: Experimental (Do Not Distribute)
*
* (C) Copyright 2010, Northwestern University, all rights reserved.
*
*******************************************************************************
*/

#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "getopt.h"
#include "bash.h"

/*
 * print_usage
 * 
 * arguments:
 *   FILE *out: stream to which the usage text should be printed
 * 
 * returns: void
 * 
 * Prints the usage message for this program.
 */
static void print_usage(FILE *out)
{
  fprintf(out, "Usage: uwhich [options] [--] COMMAND [...]\n");
  fprintf(out, "Write the full path of COMMAND(s) to standard output.\n\n");
  fprintf(out, "  --version, -[vV] Print version and exit successfully.\n");
  fprintf(out, "  --help,          Print this help and exit successfully.\n");
  fprintf(out, "  --skip-dot       Skip directories in PATH that start with a dot.\n");
  fprintf(out, "  --skip-tilde     Skip directories in PATH that start with a tilde.\n");
  fprintf(out, "  --show-dot       Don't expand a dot to current directory in output.\n");
  fprintf(out, "  --show-tilde     Output a tilde for HOME directory for non-root.\n");
  fprintf(out, "  --all, -a        Print all matches in PATH, not just the first\n");
} /* print_usage */


/*
 * print_version
 * 
 * arguments: none
 * 
 * returns: void
 * 
 * Prints the version message to standard output.
 */
static void print_version()
{
  fprintf(stdout, "uwhich - A micro-which project for Northwestern EECS 343.\n"
	  "Adapted from GNU which v2.20, Copyright (C) 1999-2008 Carlo Wood.\n");
} /* print_version */


/*
 * print_fail
 * 
 * arguments:
 *   const char *name: the name of the file you're looking for
 *   const char *path_list: the PATH environment variable string
 * 
 * returns: void
 * 
 * Prints a failure message to standard error.
 */
static void print_fail(const char *name, const char *path_list)
{
  fprintf(stderr, "which: no %s in (%s)\n", name, path_list);
} /* print_fail */


/*
 * tilde_expand
 *
 * arguments:
 *   const char *path: the path that starts with "~/"
 *
 * returns: char *: a new string with "~" expanded to the
 *                  user's home directory.
 *
 * In a string that starts with "~/", replaces "~" with
 * the user's home directory path.
 */
static char *tilde_expand(const char *path)
{
 // This has been implemented, but not tested

  char *newpath;
	char *homepath = savestring(getenv("HOME"));
	int home_len, path_len;
	home_len = strlen(homepath);
	path_len = strlen(path);

	newpath = (char *) malloc (home_len + path_len - 1);
	strcpy(newpath, homepath);
	strcpy(newpath + home_len, path + 1);

//  newpath = (char *) malloc(strlen(path) + 1);
//  strcpy(newpath, path);


  return newpath;
} /* tilde_expand */


/*
 * home: array and length of the home directory string
 */
//static char home[256];
//static size_t homelen = 0;


static int absolute_path_given;
static int found_path_starts_with_dot;
static char *abs_path;

static int skip_dot = 0, skip_tilde = 0;
static int show_dot = 0, show_tilde = 0, show_all = 0;


/*
 * find_command_in_path
 * 
 * arguments:
 *   const char *name: the name of the file you're looking for
 *   const char *path_list: the PATH environment variable string
 *   int *path_index: a pointer to the current offset into the
 *                    path_list variable. Passed as a pointer so
 *                    sub-functions can all easily update the same
 *                    variable.
 *                    (incremented as each element of the path is searched)
 * 
 * returns: char *: the full path name of the found file,
 *                  or NULL on failure
 * 
 * Iterates over the elements in path_list, and returns the full path to
 * the first file that is executable, with the given name.
 */
static char *find_command_in_path(const char *name, const char *path_list, int *path_index)
{
  char *found = NULL, *full_path;
  int status, name_len;

  name_len = strlen(name);

  /*
   * Behavior of `which` depends on the format of the user's query.
   * 
   * For an "absolute path" (def: contains any slashes) (e.g. /bin/runnable.sh)
   * `which` will only search the given directory (e.g. "/bin") for the
   * executable (e.g. "runnable.sh").
   *
   * For any non-absolute path, `which` will search the directories in the PATH.
   */
  if (!absolute_program(name))
    // We have a path (name) with no slashes. Going to search the PATH.
    absolute_path_given = 0;
  else
    {
      /*
       * We have an absolute path.
       *
       * Set the path_list variable to the string up to the last slash in name.
       * Set the name variable to the string after the last slash.
       */
      char *p;
      absolute_path_given = 1;

      if (abs_path)
	free(abs_path);
      if (*name != '.' && *name != '/' && *name != '~')
	{
	  /* 
	   * We don't have an explicit absolute path.
	   * Assume the path is in reference to the current directory,
	   * and prepend "./". Save this string in abs_path.
	   */
	  abs_path = (char *)malloc(3 + name_len);
	  strcpy(abs_path, "./");
	  strcat(abs_path, name);
	}
      else
	{
	  /* 
	   * Our path starts with ".", "/", or "~".
	   * Just copy the whole thing to abs_path.
	   */
	  abs_path = (char *)malloc(1 + name_len);
	  strcpy(abs_path, name);
	}

      /*
       * Split the abs_path variable as described above.
       */
      path_list = abs_path;
      p = strrchr(abs_path, '/');
      *p++ = 0;
      name = p;
    }

  /*
   * Search the paths in path_list (starting from index path_index) for
   * an executable file with the given name
   */
		while (path_list && path_list[*path_index])
    {
      char *path;

      /*
       * Put the directory to search in the "path" variable.
       * Advance value at path_index to the next element in the PATH variable.
       *
       * When we reach the end of the path_list variable, path_list[*path_index]
       * will be null, and we'll break out of the while.
       */
			if (absolute_path_given)
			{
				path = savestring(path_list);
				*path_index = strlen(path);
			}
			else
				path = get_next_path_element(path_list, path_index);

			if (!path)
				break;

			/*
			 * Special case: when the path starts with "~/", it is a reference to
			 * the user's home directory. Replace it with the absolute path
			 * to the user's home directory.
			 */
			if (*path == '~' && *(path + 1) == '/')
			{
				char *t = tilde_expand(path);
				free(path);
				path = t;

				// UNIMPLEMENTED: Skip this element of the PATH if skip_tilde
				if (skip_tilde) {
					path = get_next_path_element(path_list,path_index);
				}
			}

			/* 
			 * UNIMPLEMENTED: (Optional) Skip this element of the PATH if we
			 * don't have an absolute path and we have skip_dot
			 */

			found_path_starts_with_dot = (*path == '.' && *(path + 1) == '/');

			full_path = make_full_pathname(path, name, name_len);
			free(path);

			status = file_status(full_path); // implemented!

			/*
			 * If we found an executable file, escape the loop and return the result.
			 */
			if ((status & FS_EXISTS) && (status & FS_EXECABLE))
			{
				found = full_path;
				break;
			}

			free(full_path);
		}

		return (found);
} /* find_command_in_path */

static char cwd[256];
static size_t cwdlen = 0;


/*
 * get_current_working_directory
 *
 * arguments: none
 * 
 * returns: void
 *
 * Gets the current working directory, and stores it in cwd.
 * Also stores the length of the string in cwdlen.
 */
static void get_current_working_directory(void)
{
	// We can assume cwd won't change during the execution of this program.
	if (cwdlen)
		return;

	// UNIMPLEMENTED: Get the current working directory from the environment

	// If we don't have an absolute directory... exit with error.
	if (*cwd != '/')
	{
		fprintf(stderr, "Can't get current working directory\n");
		exit(-1);
	}

	// UNIMPLEMENTED: Set cwdlen to the length of cwd

	// Ensure that cwd ends with a slash, to make it easy to concatenate with.
	if (cwd[cwdlen - 1] != '/')
	{
		cwd[cwdlen++] = '/';
		cwd[cwdlen] = 0;
	}
} /* get_current_working_directory */


/*
 * path_clean_up
 *
 * arguments:
 *   const char *path: the path string to be "cleaned"
 *
 * returns: char *: a "cleaned" path string. The string is statically
 *                  allocated, so it doesn't need to be freed. However, it
 *                  should be copied before path_clean_up is called again.
 *
 * Makes sure that path is absolute--it starts with a "/". Takes out references
 * to "." and "..", and modifies the resulting path as necessary.
 */
static char *path_clean_up(const char *path)
{
	static char result[256];

	/*
	 * p1 iterates over the given path.
	 * p2 points to the current index in the result string.
	 */
	const char *p1 = path;
	char *p2 = result;

	int saw_slash = 0, saw_slash_dot = 0, saw_slash_dot_dot = 0;

	/*
	 * If we don't have a path starting with "/", copy cwd to result,
	 * and make p2 point to the string index immediately after the
	 * cwd's trailing slash.
	 */
	if (*p1 != '/')
	{
		get_current_working_directory();
		strcpy(result, cwd);
		saw_slash = 1;
		p2 = &result[cwdlen];
	}

	/*
	 * Iterate through path (incrementing p1 until hitting the null character)...
	 */
	do
	{
		/* 
		 * Simple case: copy the next character from path to result
		 */
		if (!saw_slash || *p1 != '/')
			*p2++ = *p1;

		/*
		 * handle "/./" -> "/"
		 */
		if (saw_slash_dot && (*p1 == '/'))
			p2 -= 2;

		/*
		 * Need to go up a directory. Saw "/../"
		 */
		if (saw_slash_dot_dot && (*p1 == '/'))
		{
			int cnt = 0;

			/*
			 * Decrement p2 until we've seen 3 "/"'s... the two for "/../"
			 * and then the previous one too. This takes us up a level in the tree.
			 */
			do
			{
				// Don't fall off the start of the result array...
				if (--p2 < result)
				{
					strcpy(result, path);
					return result;
				}

				//
				if (*p2 == '/')
					++cnt;
			}
			while (cnt != 3);

			// Increment p2 to get past the "/"
			++p2;
		}
		saw_slash_dot_dot = saw_slash_dot && (*p1 == '.');
		saw_slash_dot = saw_slash && (*p1 == '.');
		saw_slash = (*p1 == '/');
	}
	while (*p1++);

	return result;
} /* path_clean_up */


/*
 * path_search
 *
 * arguments:
 *   const char *cmd: the executable file name that you're searching for
 *   const char *path_list: the PATH environment variable
 *
 * returns: int: 1 => found a result, 2 => no match
 *
 * Iteratively calls find_command_in_path. Implements logic to show one
 * or all matches ( do..while(next) ).
 */
int path_search(const char *cmd, const char *path_list)
{
	char *result = NULL;
	int found_something = 0;

	if (path_list && *path_list != '\0')
	{
		int next;

		// 
		int path_index = 0;

		// need to add a pointer to the path_index integer to pass to other functions

		int * path_index_ptr = &(path_index);
		
		/*
		 * Main which functionality. Find the command, and format/print the result.
		 *
		 * Run code at least once. If the show_all option is set, loop until break.
		 */
		do
		{
			next = show_all;
//			result = find_command_in_path(cmd, path_list, path_index);
				result = find_command_in_path(cmd, path_list, path_index_ptr);
			if (result)
			{
				const char *full_path = path_clean_up(result);

				/*
				 * UNIMPLEMENTED: set in_home if the result is in the user's
				 * home directory
				 */ 
				int in_home;
				if (*full_path == '~') in_home = 1;
				else in_home = 0;
				

				/* 
				 * UNIMPLEMENTED: 
				 *
				 * Print "./" if full_path starts with cwd and show_dot option
				 * is set.
				 */
				
				if (show_dot) printf("./");
				
				/*
				 * UNIMPLEMENTED:
				 * 
				 * If we're within the home directory, don't ever print "./".
				 *
				 * Depending on show_tilde option, print "~/".
				 */
				if (!in_home) {
					if (show_tilde) {
						printf("~/");
					}
				}

				/*
				 * UNIMPLEMENTED:
				 *
				 * If we've already printed "~/" or "./", just print the rest
				 * of the path. Else, print the full path.
				 */

				if (show_dot || show_tilde) {
					printf("%s\n", (full_path + 2));
				}
				else {
				printf("%s\n", full_path);
				}
				free(result);
				found_something = 1;
			}
			else
				break;
		}
		while (next);
	}

	return found_something;
} /* path_search */


enum opts {
	opt_version,
	opt_skip_dot,
	opt_skip_tilde,
	opt_show_dot,
	opt_show_tilde,
	opt_help
};


static uid_t const superuser = 0;


/*
 * main
 *
 * arguments:
 *   int argc: number of arguments provided on the command line
 *   char *argv[]: array of pointers to arguments
 *                 provided on the command line.
 *
 * returns: int: status 0 => OK, else error.
 *
 * Parses command line arguments, sets options, and calls path_search.
 */
int main(int argc, char *argv[])
{
	// Get the PATH environment variable
	const char *path_list = savestring(getenv("PATH"));
//	const char *path_list = savestring("/usr/bin:~/uwhich");
	int short_option, fail_count = 0;
	static int long_option;
	struct option longopts[] = {
		{"help", 0, &long_option, opt_help},
		{"version", 0, &long_option, opt_version},
		{"skip-dot", 0, &long_option, opt_skip_dot},
		{"skip-tilde", 0, &long_option, opt_skip_tilde},
		{"show-dot", 0, &long_option, opt_show_dot},
		{"show-tilde", 0, &long_option, opt_show_tilde},
		{"all", 0, NULL, 'a'},
		{NULL, 0, NULL, 0}
	};

	// Use getopt to parse options and arguments on argc/argv
	while ((short_option = getopt_long(argc, argv, "aivV", longopts, NULL)) != -1)
	{
		switch (short_option)
		{
			case 0:
				switch (long_option)
				{
					case opt_help:
						print_usage(stdout);
						return 0;
					case opt_version:
						print_version();
						return 0;
					case opt_skip_dot:
						skip_dot = 1;
						break;
					case opt_skip_tilde:
						skip_tilde = 1;
						break;
					case opt_show_dot:
						show_dot = 1;
						break;
					case opt_show_tilde:
						show_tilde = geteuid() != superuser;
						break;
				}
				break;
			case 'a':
				show_all = 1;
				break;
			case 'v':
			case 'V':
				print_version();
				return 0;
		}
	}

	uidget();

	// Get the cwd so we can replace it with "./"
	if (show_dot)
		get_current_working_directory();

	// Get and parse the home directory, if we want to replace it with "~/"
	if (show_tilde)
	{
		char *h = savestring(getenv("HOME"));
		int home_len = strlen(h);
		// Be sure the home directory has a trailing slash, so it's easy
		// to concatenate.
		if (h[home_len - 1] != '/') {
			h[home_len++] = '/';
			h[home_len] = 0;
		}
	}

	argv += optind;
	argc -= optind;

	// If we don't have any more arguments... print usage
	if (argc == 0) 	  
	{
		print_usage(stderr);
		return -1;
	}

	/*
	 * For each remaining argument on the command line, run path_search.
	 * Print a failure message for a query if there's no match.
	 */
	for (; argc > 0; --argc, ++argv)
	{
		int found_something = 0;

		if (!*argv)
			continue;

		if ((show_all || !found_something) && !path_search(*argv, path_list) && !found_something)
		{
			print_fail(absolute_path_given ? strrchr(*argv, '/') + 1 : *argv, absolute_path_given ? abs_path : path_list);
			++fail_count;
		}
	}

	return fail_count;
}
