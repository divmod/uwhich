#!/bin/sh

# JSO: I hacked this script to generate the .arg files at runtime in the temp
# directory from the arg_list file. I know there's a make_args.pl script, but
# this works too. 9-24-2007

source config.test;

if [[ ! "$#" -eq 1 ]]; then
	echo -e "usage: $0 abs_path_to_tar.gz_handin_file";
	exit 1;
fi;

# JSO: Note: the argument for the tarball must be an absolute path.

TARGZ=$1;
TC_DIR=`pwd`;
SRCDIR=`mktemp -d /tmp/cs343.tests.XXXXXX`;
TMP=`mktemp -d /tmp/cs343.tests.XXXXXX`;
DIRINPATH=`mktemp -d /tmp/cs343.tests.XXXXXX`;
OUTPUT=`mktemp -d /tmp/cs343.tests.XXXXXX`;
chmod go-rwx ${SRCDIR} ${TMP} ${OUTPUT} ${DIRINPATH} || exit 1;

function cleanUp()
{
	rm -Rf ${SRCDIR} ${TMP} ${OUTPUT} ${DIRINPATH};
	rm -Rf ~/eecs343.dir.in.home;
# rm -Rf ~/eecs343_execable_in_home; # commented out for gdb testing of file
}

echo "Testing ${TARGZ}";
echo;

# Untar sources
echo "UNTAR";
cd ${SRCDIR} || { cleanUp; exit 1; }
pwd
ls
tar xvfz ${TARGZ} || { cleanUp; exit 1; }

echo;

if [[ `ls *.c |wc -w` -eq 0 ]]; then
    echo "error: no source files (*.c) found";
	cleanUp;
    exit 1;
fi;

if [ ! -f Makefile -a ! -f makefile ]; then
	echo "warning: Makefile is missing";
	echo;
fi;

# Check source files for exploits
for file in `ls *`; do
	for word in ${KEYWORDS}; do
		if [[ `cat $file | tr '\n' " " | sed "s/[[:space:]]*//g" | grep -c -e ${word}` -gt 0 ]]; then
			echo "warning: your source code has been identified as a potential security exploit."
			echo "triggering word was $word in file $file";
		fi;
	done;
done;

# Setup the environment 
cd ${TMP} || { cleanUp; exit 1; }

# Compile the code
echo "COMPILE"
gcc -lm -g -Wall -O2 ${SRCDIR}/*.c -o ${TMP}/${BIN} > ${OUTPUT}/gcc.output 2>&1;
WARNING=`grep -c warning ${OUTPUT}/gcc.output`
ERROR=`grep -c error ${OUTPUT}/gcc.output`

echo "${WARNING} warning(s) found while compiling"
echo "${ERROR} error(s) found while compiling"

if [ ${WARNING} -gt 0 -o ${ERROR} -gt 0 ]; then
	echo;
	echo "GCC OUTPUT";
	cat ${OUTPUT}/gcc.output;
fi;

if [ ! -x ${TMP}/${BIN} ]; then
	echo "error: failed to create executable";
	cat ${OUTPUT}/gcc.output;
	cleanUp;
	exit 1;
fi;

echo;


echo "Version";
./${BIN} -v  2>&1;
echo

echo "Usage";
./${BIN}  2>&1;
echo

# JSO: Setup test case arg files
# There's a "make_args.pl" script that should do this, but it's easier to do this at runtime.
# I write these files directly into the temp directory.
TEST_NUM=1
exec < ${TC_DIR}/arg_list
while read argline
do
    echo ${argline} > ${TMP}/test${TEST_NUM}.arglist
    ((TEST_NUM++));
done

### MODIFY THE ENVIRONMENT

## Add an absolute path to the PATH
export PATH=$PATH:${DIRINPATH}
cd ${DIRINPATH}

## Add /sbin to the PATH
export PATH=$PATH:/sbin

## Make files in basic directory
touch ls
chmod u+x ls

touch eecs343_execable_file
chmod u+x eecs343_execable_file

touch eecs343_not_execable_file
chmod -x eecs343_not_execable_file

# removed test case args: --skip-dot --skip-tilde eecs343_other_owner_execable_file
#touch eecs343_other_owner_execable_file
#chmod ugo+rw eecs343_other_owner_execable_file
#chmod go-x eecs343_other_owner_execable_file
#chmod u+x eecs343_other_owner_execable_file
#chown jot836 eecs343_other_owner_execable_file

## Add a directory in the user's home to the PATH
cd

touch eecs343_execable_in_home
chmod u+x eecs343_execable_in_home

mkdir eecs343.dir.in.home
export PATH="$PATH:~/eecs343.dir.in.home"

cd eecs343.dir.in.home

touch ls
chmod u+x ls

touch eecs343_execable_in_home_subdir
chmod u+x eecs343_execable_in_home_subdir

## return to main (TMP) directory
cd ${TMP}

# print the current PATH, for debugging
echo "THE PATH, BEFORE BASIC TEST CASES:"
echo $PATH
echo

# print the files we've created
echo "THE FILES WE'VE CREATED FOR THESE TESTS:"
ls -l ${DIRINPATH}/eecs343_*
ls -l ~/eecs343_*
ls -l ~/eecs343.dir.in.home/*
echo

### RUN THE BASIC TEST CASES BEFORE ADDING RELATIVE STUFF TO THE PATH

# Run test cases
echo "RUN BASIC TEST CASES";
TC_PASSED=0;
for tc in ${BASIC_TESTS}; do
	ARGS=`cat ${TMP}/${tc}.arglist`;
	ARGSORIG="";
	STDINFILE="";
	if [[ -f ${TC_DIR}/${tc}.arg ]]; then
		ARGS="${ARGS} $(<${TC_DIR}/${tc}.arg)";
	fi
	if [[ -f ${TC_DIR}/${tc}.in ]]; then
		ARGS="${ARGS} $(<${TC_DIR}/${tc}.in)";
	fi

	# check for data to be piped in on stdin.
	if [[ -f ${TC_DIR}/${tc}.stdin ]]; then
		STDINFILE="${TC_DIR}/${tc}.stdin";
	fi

	if [[ -f ${TC_DIR}/${tc}.out ]]; then
		cp ${TC_DIR}/${tc}.out ${OUTPUT}/${tc}.orig;
	else
		if [[ -f ${TC_DIR}/${tc}.arg.orig ]]; then
			ARGSORIG="${ARGSORIG} $(<${TC_DIR}/${tc}.arg.orig)";
			if [[ -f ${TC_DIR}/${tc}.in ]]; then
				ARGSORIG="${ARGSORIG} $(<${TC_DIR}/${tc}.in)";
			fi
		else
			ARGSORIG="${ARGS}";
		fi
		
		if [[ ${STDINFILE} != "" ]]; then
		    cat ${STDINFILE} | ${ORIG} ${ARGSORIG} > ${OUTPUT}/${tc}.orig 2>&1;    
		else
		    ${ORIG} ${ARGSORIG} > ${OUTPUT}/${tc}.orig 2>&1;
		fi
		
		#source ${TC_DIR}/which-orig ${ARGSORIG} > ${OUTPUT}/${tc}.orig 2>&1;

	fi

	if [[ ${STDINFILE} != "" ]]; then
	    cat ${STDINFILE} | ./${BIN} ${ARGS} > ${OUTPUT}/${tc}.bin 2>&1;
	else
	    ./${BIN} ${ARGS} > ${OUTPUT}/${tc}.bin 2>&1;
	fi

	if [[ ! -f ${TC_DIR}/${tc}.nil ]]; then
		${DIFF} ${OUTPUT}/${tc}.orig ${OUTPUT}/${tc}.bin >/dev/null;
	fi
	if [[ "$?" -eq 0 ]]; then
		echo "${tc}: PASS";
		((TC_PASSED++));
	else
		echo "${tc}: FAILED";
		echo "ARGUMENTS: ${ARGS}";
		echo "-- HOW IT SHOULD BE ------------------------------------------------------------ YOUR PROGRAM --------------------------------------------------------------";
		diff --side-by-side -W 160 ${OUTPUT}/${tc}.orig ${OUTPUT}/${tc}.bin;
		echo "------------------------------------------------------------------------------------------------------------------------------------------------------------";
	fi
done;

echo;
echo "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=";
echo "${TC_PASSED} basic test cases passed";
echo;

### SET UP FOR THE EXTRA CREDIT TEST CASES

## Add a relative directory to the path
mkdir subdir
export PATH=$PATH:./subdir
cd subdir
touch eecs343_execable_in_subdir
chmod u+x eecs343_execable_in_subdir
cd ..

## Make another relative directory, but don't add it to the PATH. Use to test detection of path specifiers in the query.
mkdir subdir2
cd subdir2
touch eecs343_execable_in_subdir2
chmod u+x eecs343_execable_in_subdir2
cd ..

# print the current PATH, for debugging
echo "THE PATH, BEFORE EXTRA CREDIT TEST CASES:"
echo $PATH
echo

echo "CURRENT WORKING DIRECTORY:"
echo `pwd`
echo

# print the files we've created
echo "THE FILES WE'VE CREATED FOR THE EXTRA CREDIT TESTS:"
ls -l ./subdir/*
ls -l ./subdir2/*
echo

# Run extra credit test cases
echo "RUN EXTRA CREDIT TEST CASES";
ETC_PASSED=0;
for tc in ${EXTRA_TESTS}; do
    ARGS=`cat ${TMP}/${tc}.arglist`;
    ARGSORIG="";
    STDINFILE="";
    if [[ -f ${TC_DIR}/${tc}.arg ]]; then
        ARGS="${ARGS} $(<${TC_DIR}/${tc}.arg)";
    fi
    if [[ -f ${TC_DIR}/${tc}.in ]]; then
        ARGS="${ARGS} $(<${TC_DIR}/${tc}.in)";
    fi

    # check for data to be piped in on stdin.
    if [[ -f ${TC_DIR}/${tc}.stdin ]]; then
	STDINFILE="${TC_DIR}/${tc}.stdin";
    fi

    if [[ -f ${TC_DIR}/${tc}.out ]]; then
        cp ${TC_DIR}/${tc}.out ${OUTPUT}/${tc}.orig;
    else
        if [[ -f ${TC_DIR}/${tc}.arg.orig ]]; then
            ARGSORIG="${ARGSORIG} $(<${TC_DIR}/${tc}.arg.orig)";
            if [[ -f ${TC_DIR}/${tc}.in ]]; then
                ARGSORIG="${ARGSORIG} $(<${TC_DIR}/${tc}.in)";
            fi
        else
            ARGSORIG="${ARGS}";
        fi

	if [[ ${STDINFILE} != "" ]]; then
	    cat ${STDINFILE} | ${ORIG} ${ARGSORIG} > ${OUTPUT}/${tc}.orig 2>&1;    
	else
	    ${ORIG} ${ARGSORIG} > ${OUTPUT}/${tc}.orig 2>&1;
	fi

    fi

    if [[ ${STDINFILE} != "" ]]; then
	cat ${STDINFILE} | ./${BIN} ${ARGS} > ${OUTPUT}/${tc}.bin 2>&1;
    else
	./${BIN} ${ARGS} > ${OUTPUT}/${tc}.bin 2>&1;
    fi
    
    ${DIFF} ${OUTPUT}/${tc}.orig ${OUTPUT}/${tc}.bin >/dev/null;
    if [[ "$?" -eq 0 ]]; then
        echo "${tc}: PASS";
        ((ETC_PASSED++));
    else
        echo "${tc}: FAILED";
	echo "ARGUMENTS: ${ARGS}";
        echo "-- HOW IT SHOULD BE ------------------------------------------------------------ YOUR PROGRAM --------------------------------------------------------------";
        diff --side-by-side -W 160 ${OUTPUT}/${tc}.orig ${OUTPUT}/${tc}.bin;
        echo "------------------------------------------------------------------------------------------------------------------------------------------------------------";
    fi
done;

echo;
echo "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=";
echo "${ETC_PASSED} extra credit test cases passed";
echo;

# Check for memory leaks
echo "CHECK FOR MEMORY LEAKS";
if [[ `which valgrind 2>&1 | grep -c "no valgrind"` -gt 0 ]]; then
	echo "error: valgrind not in PATH";
	echo "error: skip memory leak check";
else
	POS_LEAKS=0;
	DEF_LEAKS=0;
	STILL_REACHABLE=0;

	for tc in ${BASIC_TESTS}; do
		ARGS="";
	    if [[ -f ${TC_DIR}/${tc}.arg ]]; then
	    ARGS="${ARGS} $(<${TC_DIR}/${tc}.arg)";
		fi
		if [[ -f ${TC_DIR}/${tc}.in ]]; then
			ARGS="${ARGS} $(<${TC_DIR}/${tc}.in)";
	    fi
		valgrind -v --tool=memcheck --show-reachable=yes --leak-check=yes ./${BIN} ${ARGS} > ${OUTPUT}/${tc}.valgrind 2>&1;
		NOCHECK=`grep -c "no leaks are possible" ${OUTPUT}/${tc}.valgrind`;
		if [[ $NOCHECK -eq 0 ]]; then
			leaks=`grep "possibly lost:" ${OUTPUT}/${tc}.valgrind | sed "s/.*bytes in[[:space:]]*//" |sed "s/[[:space:]]*blocks.*//";`
			if [[ ${leaks} -gt ${POS_LEAKS} ]]; then
				POS_LEAKS=${leaks};
			fi;
			leaks=`grep "definitely lost:" ${OUTPUT}/${tc}.valgrind | sed "s/.*bytes in[[:space:]]*//" |sed "s/[[:space:]]*blocks.*//";`
			if [[ ${leaks} -gt ${DEF_LEAKS} ]]; then
				DEF_LEAKS=${leaks};
			fi;
			leaks=`grep "still reachable:" ${OUTPUT}/${tc}.valgrind | sed "s/.*bytes in[[:space:]]*//" |sed "s/[[:space:]]blocks.*//";`
			if [[ ${leaks} -gt ${STILL_REACHABLE} ]]; then
				STILL_REACHABLE=${leaks};
			fi;
		fi;
	done;

	echo "${POS_LEAKS} possible leaks";
	echo "${DEF_LEAKS} leaks";
	echo "${STILL_REACHABLE} still reachable";
fi;

	
#Clean up
cleanUp;
