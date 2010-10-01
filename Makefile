###############################################################################
#
# File:         Makefile
# RCS:          $Id: Makefile,v 1.3 2010/09/16 15:57:16 fabianb Exp $
# Description:  Guess
# Author:       Fabian E. Bustamante
#               AquaLab Research Group
#               Department of Electrical Engineering and Computer Science
#               Northwestern University
# Created:      Thu Sep 16, 2010 at 10:51:48
# Modified:     Thu Sep 16, 2010 at 10:51:55 fabianb@eecs.northwestern.edu
# Language:     Makefile
# Package:      N/A
# Status:       Experimental (Do Not Distribute)
#
# (C) Copyright 2010, Northwestern University, all rights reserved.
#
###############################################################################

# handin info
TEAM = `whoami`
VERSION = `date +%Y%m%d%H%M%S`
PROJ = uwhich
DELIVERY = Makefile *.h *.c DOC

# programs
CC = gcc
MV = mv
CP = cp
RM = rm
MKDIR = mkdir
TAR = tar cvf
COMPRESS = gzip
CFLAGS = -Wall -g

# files to compile
PROG = uwhich
SOURCES = bash.c uwhich.c
OBJS = ${SOURCES:.c=.o}

all: ${PROG}

test-reg: handin
	HANDIN=`pwd`/${TEAM}-${VERSION}-${PROJ}.tar.gz;\
	cd testsuite;\
	sh ./run_testcase.sh $${HANDIN};

handin: cleanAll
	${TAR} ${TEAM}-${VERSION}-${PROJ}.tar ${DELIVERY}
	${COMPRESS} ${TEAM}-${VERSION}-${PROJ}.tar

.o:
	${CC} ${CFLAGS} *.c

uwhich: ${OBJS}
	${CC} ${CFLAGS} -o $@ ${OBJS}

clean:
	${RM} -f *.o *~

cleanAll: clean
	${RM} -f ${PROG} ${TEAM}-${VERSION}-${PROJ}.tar.gz
