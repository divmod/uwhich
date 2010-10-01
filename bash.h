/* -*-C-*-
*******************************************************************************
*
* File:         bash.h
* RCS:          $Id: bash.h,v 1.3 2010/09/16 15:57:16 fabianb Exp $
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

/* From bash-3.2 / general.h / line 228 */
/* Some defines for calling file status functions. */
#define FS_EXISTS         0x1
#define FS_EXECABLE       0x2
#define FS_EXEC_PREFERRED 0x4
#define FS_EXEC_ONLY      0x8
#define FS_DIRECTORY      0x10
#define FS_NODIRS         0x20
#define FS_READABLE       0x40

/* From bash-3.2 / general.h / line 69 */
#define savestring(x) (char *)strcpy( (char *) malloc(1 + strlen (x)), (x))

extern int file_status(const char *name);
extern int absolute_program(const char *string);
extern char *get_next_path_element(char const* path_list, int *path_index_pointer);
extern char *make_full_pathname(const char *path, const char *name, int name_len);
extern int uidget();
