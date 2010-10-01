/* -*-C-*-
*******************************************************************************
*
* File:         bash.c
* RCS:          $Id: bash.c,v 1.4 2010/09/16 15:57:16 fabianb Exp $
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

#include <sys/types.h>
#include <linux/stat.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include "bash.h"

static char* extract_colon_unit (char const* string, int* p_index);

/*
 *
 * Everything below is from bash-3.2.
 *
 */

/* 
 * From bash-3.2 / shell.h / line 105
 * Information about the current user.
 */
struct user_info {
  uid_t uid, euid;
  gid_t gid, egid;
  char *user_name;
  char *shell;          /* shell from the password file */
  char *home_dir;
};

/* 
 * From bash-3.2 / shell.c / line 111
 * Information about the current user.
 */
struct user_info current_user =
  {
    (uid_t)-1, (uid_t)-1, (gid_t)-1, (gid_t)-1,
    (char *)NULL, (char *)NULL, (char *)NULL
  };


/*
 * Simple macro for conditionally calling free on a non-null pointer.
 * From bash-3.2 / general.h / line 153
 */
#define FREE(s)  do { if (s) free (s); } while (0)


/* 
 * uidget
 * 
 * arguments: none
 *
 * returns: int: non-zero if we're running setuid or setgid.
 * 
 * From bash-3.2 / shell.c / line 1136
 * Fetch the current set of uids and gids in current_user.
 */
int
uidget ()
{
  uid_t u;

  u = getuid ();
  if (current_user.uid != u)
    {
      FREE (current_user.user_name);
      FREE (current_user.shell);
      FREE (current_user.home_dir);
      current_user.user_name = current_user.shell = current_user.home_dir = (char *)NULL;
    }
  current_user.uid = u;
  current_user.gid = getgid ();
  current_user.euid = geteuid ();
  current_user.egid = getegid ();

  /* See whether or not we are running setuid or setgid. */
  return (current_user.uid != current_user.euid) ||
    (current_user.gid != current_user.egid);
}


/* From bash-3.2 / general.c / line 870 */
static int ngroups, maxgroups;

/* From bash-3.2 / general.c / line 872 */
/* The set of groups that this user is a member of. */
static gid_t *group_array = (gid_t *)NULL;

/* From bash-3.2 / lib/sh/oslib.c / line 245 */
#define DEFAULT_MAXGROUPS 64


/*
 * getmaxgroups
 *
 * arguments: none
 *
 * returns: int: maximum number of groups
 * 
 * From bash-3.2 / lib/sh/oslib.c / line 247
 * Gets the maximum possible number of groups.
 */
int
getmaxgroups ()
{
  static int maxgroups = -1;

  if (maxgroups > 0)
    return maxgroups;

  maxgroups = sysconf (_SC_NGROUPS_MAX);

  if (maxgroups <= 0)
    maxgroups = DEFAULT_MAXGROUPS;

  return maxgroups;
}


/*
 * initialize_group_array
 *
 * arguments: none
 *
 * returns: void
 *
 * From bash-3.2 / general.c / line 879
 * Allocates memory for storing the array of groups that a user belongs to.
 */
static void
initialize_group_array ()
{
  register int i;

  if (maxgroups == 0)
    maxgroups = getmaxgroups ();

  ngroups = 0;
  group_array = (gid_t *)realloc (group_array, maxgroups * sizeof (gid_t));

  ngroups = getgroups (maxgroups, group_array);

  /* If getgroups returns nothing, or the OS does not support getgroups(),
     make sure the groups array includes at least the current gid. */
  if (ngroups == 0)
    {
      group_array[0] = current_user.gid;
      ngroups = 1;
    }

  /* If the primary group is not in the groups array, add it as group_array[0]
     and shuffle everything else up 1, if there's room. */
  for (i = 0; i < ngroups; i++)
    if (current_user.gid == (gid_t)group_array[i])
      break;
  if (i == ngroups && ngroups < maxgroups)
    {
      for (i = ngroups; i > 0; i--)
        group_array[i] = group_array[i - 1];
      group_array[0] = current_user.gid;
      ngroups++;
    }

  /* If the primary group is not group_array[0], swap group_array[0] and
     whatever the current group is.  The vast majority of systems should
     not need this; a notable exception is Linux. */
  if (group_array[0] != current_user.gid)
    {
      for (i = 0; i < ngroups; i++)
        if (group_array[i] == current_user.gid)
          break;
      if (i < ngroups)
        {
          group_array[i] = group_array[0];
          group_array[0] = current_user.gid;
        }
    }
}


/*
 * group_member
 *
 * arguments:
 *   gid_t gid: the group id that we're interested in
 *
 * returns: int: 1 if we're in the group, else 0.
 *
 * From bash-3.2 / general.c / line 931
 * Return non-zero if GID is one that we have in our groups list.
 */
int
group_member (gid_t gid)
{
  register int i;

  /* Short-circuit if possible, maybe saving a call to getgroups(). */
  if (gid == current_user.gid || gid == current_user.egid)
    return (1);

  if (ngroups == 0)
    initialize_group_array ();

  /* In case of error, the user loses. */
  if (ngroups <= 0)
    return (0);

  /* Search through the list looking for GID. */
  for (i = 0; i < ngroups; i++)
    if (gid == (gid_t)group_array[i])
      return (1);

  return (0);
}


/*
 * file_status
 *
 * arguments:
 *   char const* name: pointer to the full path of a file
 *
 * returns: int: bitset of FS_* constants as defined in bash.h
 *
 * From bash-3.2 / findcmd.c / line 75
 * Return some flags based on information about this file.
 * The EXISTS bit is non-zero if the file is found.
 * The EXECABLE bit is non-zero the file is executble.
 * Zero is returned if the file is not found.
 */
int
file_status (char const* name)
{
  struct stat finfo;
  int r = 0;

  /* Determine whether this file exists or not. */
	if (stat(name,&finfo) == -1) return r; // -1 signifies error meaning dne, should return 0
	else {
	r = r | FS_EXISTS;	
  /* If the file is a directory, then it is not "executable" in the
     sense of the shell. */
		if ((finfo.st_mode & FS_DIRECTORY) == FS_DIRECTORY)	return (r | FS_DIRECTORY); // returns status flag of exist, is_directory
			// flag checking syntax: (flags & flagbitN) == flagbitN 

/* Find out if the file is actually executable.  By definition, the
     only other criteria is that the file has an execute bit set that
     we can use.  The same with whether or not a file is readable. */

	 if (((finfo.st_mode & FS_EXECABLE) == FS_EXECABLE) && ((finfo.st_mode & FS_READABLE) == FS_READABLE)) return (FS_EXISTS | FS_EXECABLE | FS_READABLE);
		else return (FS_EXISTS);
  /* Root only requires execute permission for any of owner, group or
     others to be able to exec a file, and can read any file. */

// uhh...?

  /* If we are the owner of the file, the owner bits apply. */

  /* If we are in the owning group, the group permissions apply. */

  /* Else we check whether `others' have permission to execute the file */
	}
  return (FS_EXISTS | FS_EXECABLE);
}


/*
 * absolute_program
 *
 * arguments:
 *   char const* string: given path name
 *
 * returns: int: 1 if string contains a '/'
 *
 * From bash-3.2 / general.c / line 534 ; Changes: Using 'strchr' instead of
 * 'xstrchr'.
 * Return 1 if STRING is an absolute program name; it is absolute if it
 * contains any slashes.  This is used to decide whether or not to look
 * up through $PATH.
 */
int
absolute_program (char const* string)
{
  return ((char *)strchr (string, '/') != (char *)NULL);
}


/*
 * substring
 *
 * arguments:
 *   char const* string: the string from which to return a substring
 *   int start: the beginning string offset for the substring
 *   int end: the next string offset after the end of the substring
 * 
 * returns: char *: pointer to a new copy of the substring of the original
 * 
 * From bash-3.2 / stringlib.c / line 124
 * Cons a new string from STRING starting at START and ending at END,
 * not including END.
 */
char *
substring (char const* string, int start, int end)
{
  register int len;
  register char *result;

  len = end - start;
  result = (char *)malloc (len + 1);
  strncpy (result, string + start, len);
  result[len] = '\0';
  return (result);
}


/*
 * extract_colon_unit
 *
 * arguments:
 *   char const* string: a string of colon-separated elements
 *   int* p_index: current offset into string
 *
 * returns: char *: a copy of the next colon-separated element from string,
 *                  starting with the index of the string stored at p_index.
 *
 * From bash-3.2 / general.c / line 644 ; changes: Return NULL
 * instead of 'string' when string == 0.
 * Given a string containing units of information separated by colons,
 * return the next one pointed to by (P_INDEX), or NULL if there are no more.
 * Advance (P_INDEX) to the character after the colon.
 */
char*
extract_colon_unit (char const* string, int* p_index)
{
  int i, start, len;
  char *value;

  if (string == 0)
    return NULL;

  len= strlen (string);
  if ((int)p_index >= len)
    return ((char *)NULL);

  i = *p_index;

  /* Each call to this routine leaves the index pointing at a colon if
     there is more to the path.  If I is > 0, then increment past the
     `:'.  If I is 0, then the path has a leading colon.  Trailing colons
     are handled OK by the `else' part of the if statement; an empty
     string is returned in that case. */
  if (i && string[i] == ':')
    i++;

  for (start = i; string[i] && string[i] != ':'; i++)
    ;

  *p_index = i;

  if (i == start)
    {
      if (string[i])
        (*p_index)++;
      /* Return "" in the case of a trailing `:'. */
      value = (char *)malloc (1);
      value[0] = '\0';
    }
  else
    value = substring (string, start, i);

  return (value);
}


/* 
 * get_next_path_element
 *
 * arguments:
 *   char const* path_list: the PATH environment string
 *   int* path_index_pointer: the current index in the PATH string
 *
 * returns: char *: a copy of the next element in the PATH
 *
 * From bash-2.05b / findcmd.c / line 242
 * Return the next element from PATH_LIST, a colon separated list of
 * paths.  PATH_INDEX_POINTER is the address of an index into PATH_LIST;
 * the index is modified by this function.
 * Return the next element of PATH_LIST or NULL if there are no more.
 */
char*
get_next_path_element (char const* path_list, int* path_index_pointer)
{
  char* path;

  path = extract_colon_unit (path_list, path_index_pointer);

  if (path == 0)
    return (path);

  if (*path == '\0')
    {
      free (path);
      path = savestring (".");
    }

  return (path);
}


/*
 * make_full_pathname
 *
 * arguments:
 *   const char *path: the path component string
 *   const char *name: the name component string
 *   int name_len: the length of the name string
 *
 * returns: char *: a new copy of the path and name concatenated.
 *
 * From bash-1.14.7
 * Turn PATH, a directory, and NAME, a filename, into a full pathname.
 * This allocates new memory and returns it.
 */
char *
make_full_pathname (const char *path, const char *name, int name_len)
{
  char *full_path;
  int path_len;

  path_len = strlen (path);
  full_path = (char *) malloc (2 + path_len + name_len);
  strcpy (full_path, path);
  full_path[path_len] = '/';
  strcpy (full_path + path_len + 1, name);
  return (full_path);
}
